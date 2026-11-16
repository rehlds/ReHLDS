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

#pragma once

#include "TokenLine.h"
#include "interface.h"
#include "textconsole.h"
#include "BaseSystemModule.h"
#include "ObjectList.h"

class Panel;
class System: public IBaseSystem, public BaseSystemModule {
public:
	System() {}
	~System() {}

	double GetTime();
	unsigned int GetTick();
	void SetFPS(float fps);
	void Printf(char *fmt, ...);
	void DPrintf(char *fmt, ...);
	void RedirectOutput(char *buffer, int maxSize);
	IFileSystem *GetFileSystem();
	unsigned char *LoadFile(const char *name, int *length);
	void FreeFile(unsigned char *fileHandle);
	void SetTitle(char *pszTitle);
	void SetStatusLine(char *pszStatus);
	void ShowConsole(bool visible);
	void LogConsole(char *filename);
	bool InitVGUI(IVGuiModule *module);
	Panel *GetPanel();
	bool RegisterCommand(char *name, ISystemModule *module, int commandID);
	void GetCommandMatches(char *string, ObjectList *pMatchList);
	void ExecuteString(const char *commands);
	void ExecuteFile(char *filename);
	void Errorf(char *fmt, ...);
	char *CheckParam(char *param);
	bool AddModule(ISystemModule *module, char *name);
	ISystemModule *GetModule(char *interfacename, char *library, char *instancename = nullptr);
	bool RemoveModule(ISystemModule *module);
	void Stop();
	bool Init(IBaseSystem *system, int serial, char *name);
	void RunFrame(double time);
	void ExecuteCommand(int commandID, char *commandLine);
	char *GetType();
	char *GetStatusLine();
	void ShutDown();
	char *GetBaseDir();

	int Run();
	void Sleep(int msec);
	void BuildCommandLine(char *argv);
	void BuildCommandLine(int argc, char **argv);

protected:
	struct command_t {
		char name[32];
		int commandID;
		ISystemModule *module;
	};

	struct library_t {
		char name[MAX_PATH];
		CSysModule *handle;
		CreateInterfaceFn createInterfaceFn;
	};

	library_t *GetLibrary(char *name);
	ISystemModule *FindModule(char *type, char *name = nullptr);
	bool InitTimer();
	bool InitFileSystem();
	void Error();
	void OpenLogFile(char *filename);
	void CloseLogFile();
	void Log(char *string);
	bool DispatchCommand(char *command);
	void ExecuteCommandLine();
	void UpdateTime();
	const char *GetInput();
	bool StartVGUI();
	void StopVGUI();
	void SetName(char *newName);

protected:
	enum LocalCommandIDs {
		CMD_ID_DEVELOPER = 1,
		CMD_ID_EXEC,
		CMD_ID_MODULES,
		CMD_ID_CMDLIST,
		CMD_ID_LOGFILE,
		CMD_ID_QUIT,
		CMD_ID_LOADMODULE,
		CMD_ID_UNLOADMODULE,
		CMD_ID_TITLE,
		CMD_ID_SHOWCON,
		CMD_ID_ECHO,
		CMD_ID_VERSION
	};

	void CMD_Developer(char *cmdLine);
	void CMD_Exec(char *cmdLine);
	void CMD_Title(char *cmdLine);
	void CMD_Modules(char *cmdLine);
	void CMD_CmdList(char *cmdLine);
	void CMD_ShowCon(char *cmdLine);
	void CMD_Logfile(char *cmdLine);
	void CMD_Quit(char *cmdLine);
	void CMD_UnloadModule(char *cmdLine);
	void CMD_LoadModule(char *cmdLine);
	void CMD_Echo(char *cmdLine);
	void CMD_Version(char *cmdLine);

	struct LocalCommandID_s {
		char *name;
		LocalCommandIDs id;
		void (System::*pfnCmd)(char *cmdLine);
	};
	static LocalCommandID_s m_LocalCmdReg[];

private:
	double m_LastTime;

#ifdef _WIN32
	CTextConsoleWin32 m_Console;
#else
	CTextConsoleUnix m_Console;
#endif // _WIN32

	CCounter m_Counter;
	CSysModule *m_FileSystemModule;
	IFileSystem *m_FileSystem;
	CSysModule *m_VGUIModule;
	TokenLine m_Parameters;

	char m_BaseDir[MAX_PATH];
	char m_StatusLine[80];

	FileHandle_t m_LogFile;
	char m_LogFileName[MAX_PATH];

	ObjectList m_Modules;
	ObjectList m_Libraries;
	ObjectList m_Commands;

	float m_MaxFPS;
	char *m_RedirectBuffer;
	unsigned int m_RedirectSize;
	bool m_Developer;

	unsigned int m_SerialCounter;
	unsigned int m_Tick;
	unsigned int m_SleepMSecs;

	CreateInterfaceFn m_Factorylist[3];

	bool m_VGUIStarted;
	Panel *m_MainPanel;
};

extern System gSystem;

void Sys_Printf(const char *fmt, ...);
