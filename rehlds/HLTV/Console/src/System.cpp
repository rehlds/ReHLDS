/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/

#include "precompiled.h"
#include "SteamApiCompat.h"

System gSystem;

int System::Run()
{
	if (!gSystem.Init(&gSystem, 0, "Console"))
	{
		gSystem.Sleep(2000);
		return 0;
	}

	gSystem.RunFrame(0);
	gSystem.ShutDown();
	return 0;
}

char *System::GetBaseDir()
{
	return COM_GetBaseDir();
}

void Sys_Printf(const char *fmt, ...)
{
	// Dump text to debugging console.
	va_list argptr;
	static char string[8192];

	va_start(argptr, fmt);
	Q_vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	// Get Current text and append it.
	gSystem.Printf("%s", string);
}

double System::GetTime()
{
	return m_SystemTime;
}

void System::SetFPS(float fps)
{
	m_MaxFPS = fps > 0 ? (1 / fps) : 0;
}

void System::RedirectOutput(char *buffer, int maxSize)
{
	m_RedirectBuffer = buffer;
	m_RedirectSize = maxSize;

	if (m_RedirectBuffer) {
		Q_memset(m_RedirectBuffer, 0, m_RedirectSize);
	}
}

void System::Printf(char *fmt, ...)
{
	va_list argptr;
	static char string[8192];

	va_start(argptr, fmt);
	Q_vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	Log(string);

	if (m_RedirectBuffer && Q_strlen(string) + Q_strlen(m_RedirectBuffer) + 1 < m_RedirectSize) {
		Q_strcat(m_RedirectBuffer, string);
	}

	m_Console.Print(string);
}

void System::DPrintf(char *fmt, ...)
{
	va_list argptr;
	static char string[8192];
	va_start(argptr, fmt);
	if (m_Developer)
	{
		Q_vsnprintf(string, sizeof(string), fmt, argptr);
		Log(string);

		if (m_RedirectBuffer && Q_strlen(string) + Q_strlen(m_RedirectBuffer) + 1 < m_RedirectSize) {
			Q_strcat(m_RedirectBuffer, string);
		}

		m_Console.Print(string);
	}

	va_end(argptr);
}

void System::FreeFile(unsigned char *fileHandle)
{
	if (!fileHandle)
		return;

	free(fileHandle);
}

bool System::RegisterCommand(char *name, ISystemModule *module, int commandID)
{
	command_t *cmd = (command_t *)m_Commands.GetFirst();
	while (cmd)
	{
		if (Q_stricmp(cmd->name, name) == 0) {
			Printf("WARNING! System::RegisterCommand: command \"%s\" already exists.\n", name);
			return false;
		}

		cmd = (command_t *)m_Commands.GetNext();
	}

	cmd = (command_t *)malloc(sizeof(command_t));

	Q_strlcpy(cmd->name, name);
	cmd->module = module;
	cmd->commandID = commandID;

	m_Commands.Add(cmd);
	return true;
}

void System::ExecuteString(const char *commands)
{
	if (!commands || !commands[0])
		return;

	// Remove format characters to block format string attacks
	COM_RemoveEvilChars(const_cast<char *>(commands));

	bool bInQuote = false;

	char *pszDest;
	char singleCmd[256] = {0};

	const char *pszSource = commands;
	while (*pszSource)
	{
		// Parse out single commands and execute them
		pszDest = singleCmd;

		unsigned int i;
		for (i = 0; i < ARRAYSIZE(singleCmd); i++)
		{
			const char c = *pszSource;

			if (c == '"')
			{
				bInQuote = !bInQuote;
			}
			else if ((c == ';' && !bInQuote) || !c)
			{
				// End of command and not in a quoted string
				break;
			}

			*pszDest++ = c;
			pszSource++;
		}

		if (i >= ARRAYSIZE(singleCmd))
		{
			Printf("WARNING! System::ExecuteString: Command token too long.\n");
			return;
		}

		*pszDest = '\0';

		char *pszCmd = singleCmd;
		while (*pszCmd == ' ')
			pszCmd++; // skip leading whitespaces

		DispatchCommand(pszCmd);
	}
}

void System::Errorf(char *fmt, ...)
{
	va_list argptr;
	static char string[1024];
	va_start(argptr, fmt);
	Q_vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	Printf("***** FATAL ERROR *****\n");
	Printf("%s", string);
	Printf("*** STOPPING SYSTEM ***\n");

	Stop();
}

char *System::CheckParam(char *param)
{
	return m_Parameters.CheckToken(param);
}

ISystemModule *System::FindModule(char *type, char *name)
{
	if (!type || !type[0])
		return nullptr;

	ISystemModule *module = (ISystemModule *)m_Modules.GetFirst();
	while (module)
	{
		if (Q_stricmp(type, module->GetType()) == 0
		&& (!name || Q_stricmp(name, module->GetName()) == 0)) {
			return module;
		}

		module = (ISystemModule *)m_Modules.GetNext();
	}

	return nullptr;
}

bool System::AddModule(ISystemModule *module, char *name)
{
	if (!module)
		return false;

	if (!module->Init(this, m_SerialCounter, name))
	{
		Printf("ERROR! System::AddModule: couldn't initialize module %s.\n", name);
		return false;
	}

	m_Modules.AddHead(module);
	m_SerialCounter++;

	return true;
}

ISystemModule *System::GetModule(char *interfacename, char *library, char *instancename)
{
	ISystemModule *module = FindModule(interfacename, instancename);
	if (module) {
		return module;
	}

	library_t *lib = GetLibrary(library);
	if (!lib) {
		return nullptr;
	}

	module = (ISystemModule *)lib->createInterfaceFn(interfacename, nullptr);
	if (!module) {
		DPrintf("ERROR! System::GetModule: interface \"%s\" not found in library %s.\n", interfacename, lib->name);
		return nullptr;
	}

	AddModule(module, instancename);
	return module;
}

void System::Stop()
{
	m_State = MODULE_DISCONNECTED;
}

bool System::RemoveModule(ISystemModule *module)
{
	if (!module) {
		return true;
	}

	module->ShutDown();

	command_t *cmd = (command_t *)m_Commands.GetFirst();
	while (cmd)
	{
		if (cmd->module->GetSerial() == module->GetSerial()) {
			m_Commands.Remove(cmd);
			free(cmd);
		}

		cmd = (command_t *)m_Commands.GetNext();
	}

	ISystemModule *mod = (ISystemModule *)m_Modules.GetFirst();
	while (mod)
	{
		if (mod->GetSerial() == module->GetSerial()) {
			m_Modules.Remove(mod);
			return true;
		}

		mod = (ISystemModule *)m_Modules.GetNext();
	}

	return false;
}

void System::GetCommandMatches(char *string, ObjectList *pMatchList)
{
	int len = Q_strlen(string);
	command_t *cmd = (command_t *)m_Commands.GetFirst();
	while (cmd)
	{
		if (!Q_strncmp(cmd->name, string, len)) {
			pMatchList->AddTail(cmd);
		}

		cmd = (command_t *)m_Commands.GetNext();
	}
}

System::LocalCommandID_s System::m_LocalCmdReg[] = {
	{ "developer",     CMD_ID_DEVELOPER,     &System::CMD_Developer },
	{ "exec",          CMD_ID_EXEC,          &System::CMD_Exec },
	{ "modules",       CMD_ID_MODULES,       &System::CMD_Modules },
	{ "cmdlist",       CMD_ID_CMDLIST,       &System::CMD_CmdList },
	{ "help",          CMD_ID_CMDLIST,       &System::CMD_CmdList },
	{ "logfile",       CMD_ID_LOGFILE,       &System::CMD_Logfile },
	{ "quit",          CMD_ID_QUIT,          &System::CMD_Quit },
	{ "exit",          CMD_ID_QUIT,          &System::CMD_Quit },
	{ "loadmodule",    CMD_ID_LOADMODULE,    &System::CMD_LoadModule },
	{ "unloadmodule",  CMD_ID_UNLOADMODULE,  &System::CMD_UnloadModule },
	{ "title",         CMD_ID_TITLE,         &System::CMD_Title },
	{ "showcon",       CMD_ID_SHOWCON,       &System::CMD_ShowCon },
	{ "echo",          CMD_ID_ECHO,          &System::CMD_Echo },
	{ "version",       CMD_ID_VERSION,       &System::CMD_Version },
};

bool System::Init(IBaseSystem *system, int serial, char *name)
{
	if (!BaseSystemModule::Init(system, serial, name))
		return false;

	m_State = serial;
	m_SerialCounter = serial + 1;

	if (!name) {
		SetName("system");
	}

	Q_strlcpy(m_BaseDir, GetBaseDir());

	m_Developer = CheckParam("-dev") ? true : false;
	if (!m_Console.Init(this))
	{
		Error();
		return false;
	}

	m_SystemTime = 0;
	m_LastTime = 0;

	if (!InitTimer() || !InitFileSystem())
		return false;

	m_Commands.Init();
	m_Modules.Init();
	m_Libraries.Init();

	m_LogFile = nullptr;
	Q_memset(m_LogFileName, 0, sizeof(m_LogFileName));
	Q_memset(m_Factorylist, 0, sizeof(m_Factorylist));

	m_RedirectBuffer = nullptr;
	m_RedirectSize = 0;
	m_Tick = 0;
	m_SleepMSecs = 1;
	m_VGUIModule = nullptr;
	m_VGUIStarted = false;
	SeedRandomNumberGenerator();

	if (CheckParam("-highpriority"))
	{
		m_SleepMSecs = 0;
#ifdef _WIN32
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif // _WIN32
	}

	auto pszValue = CheckParam("-maxfps");
	SetFPS(pszValue ? (float)Q_atof(pszValue) : 100);

	for (auto& cmd : m_LocalCmdReg) {
		RegisterCommand(cmd.name, this, cmd.id);
	}

	SetTitle("Console");
	GetModule(PROXY_INTERFACE_VERSION, "proxy");
	SetTitle("HLTV");
	m_State = MODULE_RUNNING;

	DPrintf("System initialized.\n");
	ExecuteCommandLine();
	Printf("Type 'help' for a list of commands.\n");

	return true;
}

bool System::StartVGUI()
{
#ifndef _WIN32
	return false;
#else
	/*if (!m_FileSystemModule) {
		Printf("System::StartVGUI: filesystem not loaded.\n");
		return false;
	}

	if (CheckParam("-novgui")) {
		return false;
	}

	m_VGUIModule = Sys_LoadModule(VGUI2_LIB);

	m_Factorylist[0] = Sys_GetFactoryThis();
	m_Factorylist[1] = Sys_GetFactory(m_VGUIModule);
	m_Factorylist[2] = Sys_GetFactory(m_FileSystemModule);

	if (vgui2::VGuiControls_Init("HLTV", m_Factorylist, ARRAYSIZE(m_Factorylist)))
	{
		vgui2::filesystem()->AddSearchPath("platform", "PLATFORM");
		vgui2::localize()->AddFile(vgui2::filesystem(), "Resource/platform_%language%.txt");
		vgui2::localize()->AddFile(vgui2::filesystem(), "Resource/vgui_%language%.txt");

		m_MainPanel = new Panel(nullptr, "MainPanel");
		vgui2::surface()->SetEmbeddedPanel(m_MainPanel->GetVPanel());

		if (!vgui2::scheme()->LoadSchemeFromFile("platform/Resource/TrackerScheme.res", "DEFAULT")) {
			Printf("System::StartVGUI: Could not load TrackerScheme.res\n");
			return false;
		}

		// Start vgui
		vgui2::ivgui()->Start();
		m_VGUIStarted = true;
		Printf("VGUI initialized.\n");
		return true;
	}
	else
	{
		Printf("System::StartVGUI: Could not initialize vgui.\n");
		return false;
	}*/

	return false;
#endif // _WIN32
}

void System::StopVGUI()
{
	if (!m_VGUIStarted)
		return;

	/*vgui2::surface()->Shutdown();
	if (m_MainPanel) {
		delete m_MainPanel;
		m_MainPanel = nullptr;
	}

	Sys_UnloadModule(m_VGUIModule);*/
	Printf("VGUI shutdown.\n");
}

void System::SetName(char *newName)
{
	Q_strlcpy(m_Name, newName);
}

void System::RunFrame(double time)
{
	float fps = 0.0f;
	float nextConsoleUpdate = 0.0f;

	BaseSystemModule::RunFrame(time);
	while ((m_State == MODULE_RUNNING || m_State == MODULE_CONNECTING))
	{
		UpdateTime();
		double timeDiff = m_SystemTime - m_LastTime;
		if (m_LastTime > 0 && timeDiff <= 0) {
			Printf("WARNING! System::RunFrame: system time difference <= 0.\n");
			timeDiff = 0.001;
		}

		if (m_MaxFPS > timeDiff) {
			Sleep(m_SleepMSecs);
			continue;
		}

		m_Tick++;
		m_LastTime = m_SystemTime;
		fps = (float)(0.1 / timeDiff) + fps * 0.9f;

		if (m_SystemTime > nextConsoleUpdate)
		{
			ExecuteString(GetInput());
			m_Console.SetStatusLine(m_StatusLine);
			nextConsoleUpdate = (float)(m_SystemTime * 0.1);
		}

		/*if (m_MainPanel) {
			vgui2::ivgui()->RunFrame();
		}*/

		if (m_State == MODULE_CONNECTING) {
			Sleep(1);
			continue;
		}

		ISystemModule *module = (ISystemModule *)m_Modules.GetFirst();
		while (module)
		{
			if (m_State == MODULE_DISCONNECTED)
				break;

			module->RunFrame(m_SystemTime);
			module = (ISystemModule *)m_Modules.GetNext();
		}
	}
}

char *System::GetStatusLine()
{
	static char string[80];
	Q_sprintf(string, " Libraries %i, Modules %i, Commands %i.\n", m_Libraries.CountElements(), m_Modules.CountElements(), m_Commands.CountElements());
	return string;
}

char *System::GetType()
{
	return BASESYSTEM_INTERFACE_VERSION;
}

IFileSystem *System::GetFileSystem()
{
	return m_FileSystem;
}

void System::ShutDown()
{
	m_Commands.Clear(true);

	ISystemModule *module = (ISystemModule *)m_Modules.GetFirst();
	while (module)
	{
		module->ShutDown();
		module = (ISystemModule *)m_Modules.GetNext();
	}

	library_t *lib = (library_t *)m_Libraries.RemoveTail();
	while (lib)
	{
		if (lib->handle) {
			Sys_UnloadModule(lib->handle);
		}
		free(lib);
		lib = (library_t *)m_Libraries.RemoveTail();
	}

	CloseLogFile();
	if (m_FileSystem) {
		m_FileSystem->Unmount();
	}

	Sys_UnloadModule(m_FileSystemModule);
	StopVGUI();

	if (m_Developer) {
		Sleep(3000);
	}

	m_Console.ShutDown();
	m_State = MODULE_DISCONNECTED;
}

bool System::InitFileSystem()
{
	char *filesystemmodule = STDIO_FILESYSTEM_LIB;
	m_FileSystemModule = Sys_LoadModule(filesystemmodule);
	if (!m_FileSystemModule)
	{
		Errorf("Couldn't load %s.\n", filesystemmodule);
		return false;
	}

	CreateInterfaceFn filesystemFactory = Sys_GetFactory(m_FileSystemModule);
	if (!filesystemFactory)
		return false;

	m_FileSystem = (IFileSystem *)filesystemFactory(FILESYSTEM_INTERFACE_VERSION, nullptr);
	if (!m_FileSystem)
	{
		Errorf("Couldn't get IFileSystem from %s.\n", filesystemmodule);
		return false;
	}

	m_FileSystem->Mount();
	m_FileSystem->AddSearchPath(m_BaseDir, "ROOT");

	Printf("FileSystem initialized");
	DPrintf(" (%s, %s)", filesystemmodule, m_BaseDir);
	Printf(".\n");

	return true;
}

bool System::InitTimer()
{
	if (!m_Counter.Init()) {
		Errorf("Couldn't get system timer.\n");
		return false;
	}

	m_SystemTime = 0;
	m_LastTime = 0;
	return true;
}

void System::Error()
{
	Errorf("%s\n", strerror(errno));
}

void System::CMD_CmdList(char *cmdLine)
{
	Printf("Registerd console commands:");

	command_t *cmd = (command_t *)m_Commands.GetFirst();
	while (cmd)
	{
		Printf("\n %s", cmd->name);
		DPrintf(" (%s)", cmd->module->GetName());

		cmd = (command_t *)m_Commands.GetNext();
	}

	Printf("\n");
}

void System::CMD_Title(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2) {
		Printf("Syntax: title <text>\n");
		return;
	}

	SetTitle(params.GetToken(1));
}

void System::CMD_Version(char *cmdLine)
{
	Printf("ReHLTV version: " APP_VERSION "\n");
	Printf("Build date: " __TIME__ " " __DATE__ "\n");
	Printf("Build from: " APP_COMMIT_URL APP_COMMIT_SHA "\n");

	char szSteamInfo[2560];
	SteamApiCompat::Describe(szSteamInfo, sizeof(szSteamInfo));
	Printf("%s", szSteamInfo);
}

void System::CMD_Echo(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() < 2) {
		Printf("Syntax: title <text>\n");
		return;
	}

	Printf("%s\n", params.GetRestOfLine(1));
}

void System::CMD_Logfile(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2)
	{
		Printf("Syntax: logfile <0|1|filename>\n");
		if (m_LogFile) {
			Printf("Console is logged to \"%s\".\n", m_LogFileName);
		}
		else {
			Printf("Logging is off.\n");
		}

		return;
	}

	char *param = params.GetToken(1);
	switch (param[0]) {
	// log enabled
	case '1':
	{
		if (m_LogFile) {
			Printf("Console is logged to \"%s\".\n", m_LogFileName);
			return;
		}

		char filename[MAX_PATH];
		Q_sprintf(filename, "logfile%s.txt", COM_TimeString());
		OpenLogFile(filename);
		break;
	}
	// log disabled
	case '0':
		CloseLogFile();
		break;
	 // log enabled with the provided file name.
	default:
		OpenLogFile(param);
		break;
	}
}

void System::CMD_ShowCon(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2) {
		Printf("Syntax: showcon <0|1>\n");
		return;
	}

	char *param = params.GetToken(1);
	ShowConsole((param[0] != '0') ? true : false);
}

void System::CMD_Modules(char *cmdLine)
{
	ISystemModule *module = (ISystemModule *)m_Modules.GetFirst();
	while (module)
	{
		Printf("Module %s, Interface %s, Version %i\n",
			module->GetName(),
			module->GetType(),
			module->GetVersion());

		module = (ISystemModule *)m_Modules.GetNext();
	}

	Printf("--- %i modules in total ---\n", m_Modules.CountElements());
}

void System::CMD_Exec(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2) {
		Printf("Syntax: exec <file.cfg>\n");
		return;
	}

	ExecuteFile(params.GetToken(1));
}

void System::CMD_Developer(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2) {
		Printf("Syntax: developer <0|1>\n");
		Printf("Developer mode is %s.\n", m_Developer ? "on" : "off");
		return;
	}

	m_Developer = Q_atoi(params.GetToken(1)) ? true : false;
}

void System::CMD_Quit(char *cmdLine)
{
	Stop();
}

void System::OpenLogFile(char *filename)
{
	CloseLogFile();
	Q_strlcpy(m_LogFileName, filename);

	m_LogFile = m_FileSystem->Open(m_LogFileName, "wt");
	if (!m_LogFile)
	{
		Q_memset(m_LogFileName, 0, sizeof(m_LogFileName));
		Printf("System::OpenLogFile: error while opening logfile.\n");
	}
}

void System::CloseLogFile()
{
	if (!m_LogFile || !m_FileSystem)
		return;

	m_FileSystem->Flush(m_LogFile);
	m_FileSystem->Close(m_LogFile);
	m_LogFile = nullptr;

	Printf("Console log file closed.\n");
	Q_memset(m_LogFileName, 0, sizeof(m_LogFileName));
}

void System::Log(char *string)
{
	if (!m_LogFile)
		return;

	m_FileSystem->Write(string, Q_strlen(string), m_LogFile);
}

void System::ExecuteFile(char *filename)
{
	char line[256];
	FileHandle_t cfgfile;

	cfgfile = m_FileSystem->Open(filename, "rt");
	if (!cfgfile) {
		Printf("Couldn't open config file %s.\n", filename);
		return;
	}

	Printf("Executing file %s.\n", filename);
	while ((m_FileSystem->ReadLine(line, sizeof(line) - 1, cfgfile)))
	{
		char *remark = Q_strstr(line, "//");
		if (remark) {
			*remark = '\0';
		}

		ExecuteString(line);
	}

	m_FileSystem->Close(cfgfile);
}

bool System::DispatchCommand(char *command)
{
	if (!command || !command[0])
		return true;

	TokenLine params(command);
	command_t *cmd = (command_t *)m_Commands.GetFirst();
	while (cmd)
	{
		if (Q_stricmp(cmd->name, params.GetToken(0)) == 0) {
			cmd->module->ExecuteCommand(cmd->commandID, command);
			return true;
		}

		cmd = (command_t *)m_Commands.GetNext();
	}

	Printf("WARNING! System::DispatchCommand: command \"%s\" not registered.\n", command);
	return false;
}

void System::ExecuteCommandLine()
{
	int i = 0;
	char string[256];
	char *token = m_Parameters.GetToken(i);
	while (token)
	{
		if (token[0] != '+') {
			token = m_Parameters.GetToken(++i);
			continue;
		}

		Q_memset(string, 0, sizeof(string));
		Q_strlcpy(string, token + 1);

		// next iteration
		token = m_Parameters.GetToken(++i);
		while (token && !(token[0] == '+' || token[0] == '-'))
		{
			Q_strlcat(string, " ");
			Q_strlcat(string, token);

			token = m_Parameters.GetToken(++i);
		}

		ExecuteString(string);
	}
}

void System::UpdateTime()
{
	m_SystemTime = m_Counter.GetCurTime();
}

const char *System::GetInput()
{
	return m_Console.GetLine();
}

void System::CMD_LoadModule(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() < 2) {
		Printf("Syntax: loadmodule <module> [<library>] [<name>]\n");
		return;
	}

	switch (params.CountToken()) {
	case 2:
		GetModule(params.GetToken(1), params.GetToken(1));
		break;
	case 3:
		GetModule(params.GetToken(1), params.GetToken(2));
		break;
	default:
		GetModule(params.GetToken(1), params.GetToken(2), params.GetToken(3));
		break;
	}
}

void System::CMD_UnloadModule(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() < 2) {
		Printf("Syntax: unloadmodule <module> [<name>]\n");
		return;
	}

	ISystemModule *module = nullptr;
	switch (params.CountToken()) {
	case 2:
		module = FindModule(params.GetToken(1));
		break;
	case 3:
		module = FindModule(params.GetToken(1), params.GetToken(2));
		break;
	}

	if (!module) {
		Printf("Module not found.\n");
		return;
	}

	RemoveModule(module);
}

System::library_t *System::GetLibrary(char *name)
{
	char fixedname[MAX_PATH];
	Q_strlcpy(fixedname, name);
	COM_FixSlashes(fixedname);

	library_t *lib = (library_t *)m_Libraries.GetFirst();
	while (lib)
	{
		if (Q_stricmp(lib->name, name) == 0) {
			return lib;
		}

		lib = (library_t *)m_Libraries.GetNext();
	}

	lib = (library_t *)malloc(sizeof(library_t));
	if (!lib) {
		DPrintf("ERROR! System::GetLibrary: out of memory (%s).\n", name);
		return nullptr;
	}

	Q_snprintf(lib->name, sizeof(lib->name), "%s." LIBRARY_PREFIX, fixedname);
	lib->handle = (CSysModule *)Sys_LoadModule(lib->name);
	if (!lib->handle) {
		DPrintf("WARNING! System::GetLibrary: coulnd't load library (%s).\n", lib->name);
		free(lib);
		return nullptr;
	}

	lib->createInterfaceFn = (CreateInterfaceFn)Sys_GetFactory(lib->handle);
	if (!lib->createInterfaceFn) {
		DPrintf("ERROR! System::GetLibrary: coulnd't get object factory(%s).\n", lib->name);
		free(lib);
		return nullptr;
	}

	m_Libraries.Add(lib);
	DPrintf("Loaded library %s.\n", lib->name);

	return lib;
}

unsigned int System::GetTick()
{
	return m_Tick;
}

void System::ExecuteCommand(int commandID, char *commandLine)
{
	for (auto& cmd : m_LocalCmdReg)
	{
		if (cmd.pfnCmd && cmd.id == commandID) {
			(this->*cmd.pfnCmd)(commandLine);
			return;
		}
	}

	Printf("ERROR! System::ExecuteCommand: unknown command ID %i.\n", commandID);
}

void System::SetTitle(char *text)
{
	// "[ReHLTV <version>] <title>", like the "[ReHLDS <version>]" title of the hlds console window
	char buffer[256];
	Q_snprintf(buffer, sizeof(buffer), "[ReHLTV " APP_VERSION "] %s", text ? text : "");
	m_Console.SetTitle(buffer);
}

bool System::InitVGUI(IVGuiModule *module)
{
	if (!m_VGUIStarted && !StartVGUI()) {
		return false;
	}

#ifdef _WIN32
	module->Initialize(m_Factorylist, ARRAYSIZE(m_Factorylist));
	module->PostInitialize();
	//module->SetParent(m_MainPanel->GetVPanel());
#endif // _WIN32

	return true;
}

Panel *System::GetPanel()
{
#ifdef _WIN32
	if (m_VGUIStarted || StartVGUI()) {
		return m_MainPanel;
	}
#endif // _WIN32

	return nullptr;
}

void System::SetStatusLine(char *text)
{
	Q_strlcpy(m_StatusLine, text);
}

void System::ShowConsole(bool visible)
{
	m_Console.SetVisible(visible);
}

void System::LogConsole(char *filename)
{
	if (filename) {
		OpenLogFile(filename);
		return;
	}

	CloseLogFile();
}

unsigned char *System::LoadFile(const char *name, int *length)
{
	if (!m_FileSystem) {
		return nullptr;
	}

	if (length) {
		*length = 0;
	}

	FileHandle_t hFile = m_FileSystem->Open(name, "rb", 0);
	if (!hFile) {
		return nullptr;
	}

	m_FileSystem->Seek(hFile, 0, FILESYSTEM_SEEK_TAIL);

	int len = m_FileSystem->Tell(hFile);
	unsigned char *buf = (unsigned char *)malloc(len + 1);
	if (!buf)
	{
		Printf("WARNING! System::LoadFile: not enough space for %s.\n", name);
		return nullptr;
	}

	buf[len] = '\0';
	m_FileSystem->Seek(hFile, 0, FILESYSTEM_SEEK_HEAD);
	m_FileSystem->Read(buf, len, hFile);
	m_FileSystem->Close(hFile);

	if (length) {
		*length = len;
	}

	return buf;
}

#ifdef _WIN32

#ifdef HLTV_FIXES
// Load steamclient.dll from the hltv.exe folder before the proxy initializes Steam.
// Without this, steam_api may take the steamclient.dll of an installed Steam client from
// HKCU\Software\Valve\Steam\ActiveProcess\SteamClientDll (the value stays there after Steam exits),
// and the proxy randomly fails to initialize Steam. Same as Sys_PreloadSteamClient() in hlds.exe.
// Disable with -nolocalsteamclient.
static void PreloadSteamClient(const char *cmdLine)
{
	if (cmdLine && strstr(cmdLine, "-nolocalsteamclient"))
		return;

	char path[MAX_PATH];
	DWORD len = GetModuleFileNameA(nullptr, path, sizeof(path));
	if (len == 0 || len >= sizeof(path))
		return;

	char *slash = strrchr(path, '\\');
	if (!slash)
		return;

	const char steamClient[] = "steamclient.dll";
	if ((size_t)(slash + 1 - path) + sizeof(steamClient) > sizeof(path))
		return;

	strcpy(slash + 1, steamClient);
	if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)
		return; // no local steamclient.dll, leave the choice to steam_api

	LoadLibraryExA(path, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
}
#endif // HLTV_FIXES

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#ifdef HLTV_FIXES
	return CatchAndWriteMiniDump([=]()
	{
		PreloadSteamClient(lpCmdLine);
#endif
		gSystem.BuildCommandLine(lpCmdLine);
		return gSystem.Run();
#ifdef HLTV_FIXES
	}, lpCmdLine);
#endif
}

void System::BuildCommandLine(char *argv)
{
	m_Parameters.SetLine(argv);
}

void System::Sleep(int msec)
{
	::Sleep(msec);
}

#else // _WIN32

int main(int argc, char *argv[])
{
	gSystem.BuildCommandLine(argc, argv);
	Q_snprintf(g_szEXEName, sizeof(g_szEXEName), "%s", argv[0]);
	return gSystem.Run();
}

#define MAX_LINUX_CMDLINE 2048

void System::BuildCommandLine(int argc, char **argv)
{
	int len = 0;
	char string[MAX_LINUX_CMDLINE];

	for (int i = 1; i < argc && len < MAX_LINUX_CMDLINE; i++)
	{
		len += Q_strlen(argv[i]) + 1;

		if (i > 1) {
			Q_strlcat(string, " ");
		}

		Q_strlcat(string, argv[i]);
	}

	m_Parameters.SetLine(string);
}

void System::Sleep(int msec)
{
	usleep(1000 * msec);
}

#endif // _WIN32
