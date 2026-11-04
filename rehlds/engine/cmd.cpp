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

int cmd_argc;
char *cmd_argv[80];

// Complete arguments string
char *cmd_args;

sizebuf_t cmd_text;
cmd_source_t cmd_source;
qboolean cmd_wait;
cmdalias_t *cmd_alias;

//int trashtest;
//int *trashspot;

cmd_function_t *cmd_functions;
char *const cmd_null_string = "";

void Cmd_Wait_f(void)
{
	cmd_wait = 1;
}

void Cbuf_Init(void)
{
	SZ_Alloc("cmd_text", &cmd_text, MAX_CMD_BUFFER);
}

// As new commands are generated from the console or keybindings,
// the text is added to the end of the command buffer.
void Cbuf_AddText(const char *text)
{
	int len = Q_strlen(text);

	if (cmd_text.cursize + len >= cmd_text.maxsize)
	{
		Con_Printf("%s: overflow\n", __func__);
		return;
	}

	SZ_Write(&cmd_text, text, len);
}

// When a command wants to issue other commands immediately, the text is
// inserted at the beginning of the buffer, before any remaining unexecuted
// commands.
void Cbuf_InsertText(const char *text)
{

	int addLen = Q_strlen(text);
	int currLen = cmd_text.cursize;

	if (cmd_text.cursize + addLen >= cmd_text.maxsize)
	{
		Con_Printf("%s: overflow\n", __func__);
		return;
	}

#ifdef REHLDS_FIXES
	if (currLen)
		Q_memmove(cmd_text.data + addLen, cmd_text.data, currLen);

	Q_memcpy(cmd_text.data, text, addLen);
	cmd_text.cursize += addLen;

#else
	char *temp = NULL;
	if (currLen)
	{

		temp = (char *)Z_Malloc(currLen);	// TODO: Optimize: better use memmove without need for a temp buffer
		Q_memcpy(temp, cmd_text.data, currLen);
		SZ_Clear(&cmd_text);
	}

	Cbuf_AddText(text);

	if (currLen)
	{
		SZ_Write(&cmd_text, temp, currLen);
		Z_Free(temp);
	}
#endif // REHLDS_FIXES
}

void Cbuf_InsertTextLines(const char *text)
{
	int addLen = Q_strlen(text);
	int currLen = cmd_text.cursize;

	if (cmd_text.cursize + addLen + 2 >= cmd_text.maxsize)
	{
		Con_Printf("%s: overflow\n", __func__);
		return;
	}

#ifdef REHLDS_FIXES
	if (currLen)
		Q_memmove(cmd_text.data + addLen + 2, cmd_text.data, currLen);

	cmd_text.data[0] = '\n'; // TODO: Why we need leading \n, if there is no commands in the start?
	Q_memcpy(&cmd_text.data[1], text, addLen);
	cmd_text.data[addLen + 1] = '\n';

	cmd_text.cursize += addLen + 2;

#else

	char *temp = NULL;
	if (currLen)
	{

		temp = (char *)Z_Malloc(currLen);
		Q_memcpy(temp, cmd_text.data, currLen);
		SZ_Clear(&cmd_text);
	}

	Cbuf_AddText("\n");	// TODO: Why we need leading \n, if there is no commands in the start?
	Cbuf_AddText(text);
	Cbuf_AddText("\n");

	if (currLen)
	{
		SZ_Write(&cmd_text, temp, currLen);
		Z_Free(temp);
	}
#endif // REHLDS_FIXES
}

// Pulls off \n terminated lines of text from the command buffer and sends
// them through Cmd_ExecuteString.  Stops when the buffer is empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function!
void Cbuf_Execute(void)
{
	int i;
	char *text;
	char line[MAX_CMD_LINE] = {0};
	int quotes;
	bool commented;

	while (cmd_text.cursize)
	{
		// find a \n or ; line break
		text = (char *)cmd_text.data;

		quotes = 0;
		commented = false;

#ifdef REHLDS_FIXES
		if (cmd_text.cursize > 1 && text[0] == '/' && text[1] == '/')
			commented = true;
#endif

		for (i = 0; i < cmd_text.cursize; i++)
		{
			if (text[i] == '"')
				quotes++;

			if (!(quotes & 1) && !commented && text[i] == ';')
				break;	// don't break if inside a quoted or commented out strings

			if (text[i] == '\n')
				break;
		}

#ifdef REHLDS_FIXES
		// save `i` if we truncate command
		int len;

		if (i > MAX_CMD_LINE - 1)
			len = MAX_CMD_LINE - 1;
		else
			len = i;

		Q_memcpy(line, text, len);
		line[len] = 0;
#else // REHLDS_FIXES
		if (i > MAX_CMD_LINE - 1)
		{
			i = MAX_CMD_LINE - 1;
		}

		Q_memcpy(line, text, i);
		line[i] = 0;
#endif // REHLDS_FIXES

		// delete the text from the command buffer and move remaining commands down
		// this is necessary because commands (exec, alias) can insert data at the
		// beginning of the text buffer

		if (i == cmd_text.cursize)
		{
			cmd_text.cursize = 0;
		}
		else
		{
			i++;
			cmd_text.cursize -= i;
#ifdef REHLDS_FIXES
			// dst overlaps src
			Q_memmove(text, text + i, cmd_text.cursize);
#else // REHLDS_FIXES
			Q_memcpy(text, text + i, cmd_text.cursize);
#endif // REHLDS_FIXES
		}

		// execute the command line
		if (line[0])
			Cmd_ExecuteString(line, src_command);

		if (cmd_wait)
		{
			// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait = FALSE;
			break;
		}
	}
}

void Cmd_StuffCmds_f(void)
{
	int i;
	int s;
	char *build;

	if (Cmd_Argc() != 1)
	{
		Con_Printf("stuffcmds : execute command line parameters\n");
		return;
	}

	if (com_argc <= 1)
	{
		return;
	}

	// Get total length for the command line parameters
	s = 0;
	for (i = 1; i < com_argc; i++)
	{
		if (com_argv[i])
		{
			s += Q_strlen(com_argv[i]) + 1;
		}
	}

	if (s == 0)
	{
		return;
	}

	// Create buffer able to get all arguments
	build = (char *)Z_Malloc(s + com_argc * 2);
	build[0] = 0;

	//  Iterate thru arguments searching for ones starting with +
	for (i = 1; i < com_argc; i++)
	{
		if (com_argv[i] && com_argv[i][0] == '+')
		{
			// Add command or cvar
			Q_strcat(build, &com_argv[i][1]);
			// Then add all following parameters till we meet argument with + or -, which means next command/cvar/parameter
			i++;
			while (i < com_argc && com_argv[i] && com_argv[i][0] != '+' && com_argv[i][0] != '-')
			{
				Q_strcat(build, " ");
				Q_strcat(build, com_argv[i]);
				i++;
			}
			// End up with new line which split commands for command processor
			Q_strcat(build, "\n");
			i--;
		}
	}

	if (build[0] != 0)
	{
		Cbuf_InsertText(build);
	}

	// Free buffers
	Z_Free(build);
}

void Cmd_Exec_f(void)
{
	const char *pszFileName;
	const char *pszFileExt;
	char *pszFileData;
	int nAddLen;
	FileHandle_t hFile;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("exec <filename> : execute a script file\n");
		return;
	}

	pszFileName = Cmd_Argv(1);
	if (!pszFileName || pszFileName[0] == 0)
	{
		return;
	}

	if (Q_strstr(pszFileName, "\\")
		|| Q_strstr(pszFileName, ":")
		|| Q_strstr(pszFileName, "~")
		|| Q_strstr(pszFileName, "..")
		|| *pszFileName == '/')
	{
		Con_Printf("exec %s: invalid path.\n", pszFileName);
		return;
	}

	pszFileExt = COM_FileExtension((char *)pszFileName);
	if (Q_stricmp(pszFileExt, "cfg") && Q_stricmp(pszFileExt, "rc"))
	{
		Con_Printf("exec %s: not a .cfg or .rc file\n", pszFileName);
		return;
	}

	hFile = FS_OpenPathID(pszFileName, "rb", "GAMECONFIG");
	if (!hFile)
	{
		hFile = FS_OpenPathID(pszFileName, "rb", "GAME");
	}
	if (!hFile)
	{
		hFile = FS_Open(pszFileName, "rb");
	}

	if (!hFile)
	{
		if (!Q_strstr(pszFileName, "autoexec.cfg")
			&& !Q_strstr(pszFileName, "userconfig.cfg")
			&& !Q_strstr(pszFileName, "hw/opengl.cfg")
			&& !Q_strstr(pszFileName, "joystick.cfg")
			&& !Q_strstr(pszFileName, "game.cfg"))
		{
			Con_Printf("couldn't exec %s\n", pszFileName);
		}

		return;
	}

	nAddLen = FS_Size(hFile);
	pszFileData = (char *)Mem_Malloc(nAddLen + 1);

	if (!pszFileData)
	{
		Con_Printf("exec: not enough space for %s", pszFileName);
		FS_Close(hFile);
		return;
	}

	FS_Read(pszFileData, nAddLen, 1, hFile);
	pszFileData[nAddLen] = 0;
	FS_Close(hFile);

	char *configContents = pszFileData;
#ifdef REHLDS_FIXES
	if (configContents[0] == char(0xEF) && configContents[1] == char(0xBB) && configContents[2] == char(0xBF))
	{
		configContents += 3;
		nAddLen -= 3;
	}
#endif

	Con_DPrintf("execing %s\n", pszFileName);

	if (cmd_text.cursize + nAddLen + 2 < cmd_text.maxsize)
	{
		Cbuf_InsertTextLines(configContents);
	}
	else
	{
		char *pszDataPtr = configContents;
		while (pszDataPtr && *pszDataPtr)
		{
			Cbuf_Execute();	// TODO: This doesn't obey the rule to first execute commands from the file, and then the others in the buffer
			pszDataPtr = COM_ParseLine(pszDataPtr); // TODO: COM_ParseLine can be const char*
			if (com_token[0])
				Cbuf_InsertTextLines(com_token);
		}
	}

	Mem_Free(pszFileData);
}

void Cmd_Echo_f(void)
{
	int i;
	int c = Cmd_Argc();

	for (i = 1; i < c; i++)
	{
		Con_Printf("%s ", Cmd_Argv(i));
	}

	Con_Printf("\n");
}

char *CopyString(const char *in)
{
	char *out = (char *)Z_Malloc(Q_strlen(in) + 1);
	Q_strcpy(out, in);
	return out;
}

void Cmd_Alias_f(void)
{
	cmdalias_t *a;
	const char *s;
	char cmd[MAX_CMD_LINE];
	int i, c;

	if (Cmd_Argc() == 1)
	{
		// Output all aliases
		Con_Printf("Current alias commands:\n");

		for (a = cmd_alias; a; a = a->next)
		{
			Con_Printf("%s : %s", a->name, a->value);	// Don't need \n here, because each alias value is appended with it
		}

		return;
	}

	s = Cmd_Argv(1);

	if (Q_strlen(s) >= MAX_ALIAS_NAME)
	{
		Con_Printf("Alias name is too long\n");
		return;
	}

	if (Cvar_FindVar(s))
	{
		Con_Printf("Alias name is invalid\n");
		return;
	}

#ifndef REHLDS_FIXES
	SetCStrikeFlags();	// DONE: Do this once somewhere at the server start
#endif
	if ((g_eGameType == GT_CStrike || g_eGameType == GT_CZero) &&
		(!Q_stricmp(s, "cl_autobuy")
		|| !Q_stricmp(s, "cl_rebuy")
		|| !Q_stricmp(s, "gl_ztrick")
		|| !Q_stricmp(s, "gl_ztrick_old")
		|| !Q_stricmp(s, "gl_d3dflip")))
	{
		Con_Printf("Alias name is invalid\n");
		return;
	}

	// Say hello to my little friend! (c)
	if (g_eGameType == GT_TFC && (!Q_stricmp(s, "_special") || !Q_stricmp(s, "special")))
	{
		Con_Printf("Alias name is invalid\n");
		return;
	}

	// Gather arguments into one string
	cmd[0] = 0;
	c = Cmd_Argc();
	for (i = 2; i < c; i++)
	{
		Q_strncat(cmd, Cmd_Argv(i), MAX_CMD_LINE - 2 - Q_strlen(cmd));	// always have a space for \n or ' ' and \0

		if (i != c)
		{
			Q_strcat(cmd, " ");
		}
	}
	Q_strcat(cmd, "\n");

	// Search for existing alias
	for (a = cmd_alias; a; a = a->next)
	{
		if (!Q_stricmp(a->name, s))
		{
			if (!Q_strcmp(a->value, cmd))
			{
				// Same value on the alias, return
				return;
			}
			// Release value, will realloc
			Z_Free(a->value);
			break;
		}
	}

	if (!a)
	{
		// Alloc new alias
		a = (cmdalias_t *)Z_Malloc(sizeof(cmdalias_t));
		a->next = cmd_alias;
		cmd_alias = a;

		Q_strncpy(a->name, s, ARRAYSIZE(a->name) - 1);
		a->name[ARRAYSIZE(a->name) - 1] = 0;
	}

	a->value = CopyString(cmd);
}

struct cmd_function_s *Cmd_GetFirstCmd(void)
{
	return cmd_functions;
}

void Cmd_Init(void)
{
	Cmd_AddCommand("stuffcmds", Cmd_StuffCmds_f);
	Cmd_AddCommand("exec", Cmd_Exec_f);
	Cmd_AddCommand("echo", Cmd_Echo_f);
	Cmd_AddCommand("alias", Cmd_Alias_f);
	Cmd_AddCommand("cmd", Cmd_ForwardToServer);
	Cmd_AddCommand("wait", Cmd_Wait_f);
	Cmd_AddCommand("cmdlist", Cmd_CmdList_f);
}

void Cmd_Shutdown(void)
{
	for (int i = 0; i < cmd_argc; i++)
	{
		Z_Free(cmd_argv[i]);
	}
	Q_memset(cmd_argv, 0, sizeof(cmd_argv));
	cmd_argc = 0;
	cmd_args = NULL;

	cmd_functions = NULL;	// TODO: Check that memory from functions is released too
}

int EXT_FUNC Cmd_Argc(void)
{
#ifndef SWDS
	g_engdstAddrs->Cmd_Argc();
#endif

	return cmd_argc;
}

const char* EXT_FUNC Cmd_Argv(int arg)
{
#ifndef SWDS
	g_engdstAddrs->Cmd_Argv(&arg);
#endif

	if (arg >= 0 && arg < cmd_argc)
	{
		return cmd_argv[arg];
	}
	return "";	// TODO: Possibly better to return NULL here, but require to check all usages
}

const char* EXT_FUNC Cmd_Args(void)
{
#ifndef SWDS
	g_engdstAddrs->Cmd_Args();
#endif

	return cmd_args;
}

/*
Parses the given string into command line tokens.
Takes a null terminated string. Does not need to be \n terminated.
Breaks the string up into arg tokens.
*/
void EXT_FUNC Cmd_TokenizeString(char *text)
{
	int i;
	int arglen;

	// clear args from the last string
	for (i = 0; i < cmd_argc; i++)
	{
		Z_Free(cmd_argv[i]);
		cmd_argv[i] = NULL;
	}
	cmd_argc = 0;
	cmd_args = NULL;

	while (true)
	{
		// Skip whitespace up to a \n
#ifdef REHLDS_FIXES
		while (*text && (uint8_t)*text <= ' ' && *text != '\n')
#else // REHLDS_FIXES
		while (*text && *text <= ' ' && *text != '\n')
#endif // REHLDS_FIXES
		{
			++text;
		}

		// A newline separates commands in the buffer
		if (*text == '\n' || *text == 0)
		{
			break;
		}

		// Store complete args string pointer
		if (cmd_argc == 1)
		{
			cmd_args = text;
		}

		// Break into token
		text = COM_Parse(text);

		// If nothing was parsed
		if (!text)
		{
			return;
		}

		arglen = Q_strlen(com_token) + 1;
		if (arglen >= 516)	// TODO: What is that magic number?
		{
			return;
		}

		cmd_argv[cmd_argc] = (char *)Z_Malloc(arglen);
		Q_strcpy(cmd_argv[cmd_argc++], com_token);

		if (cmd_argc >= MAX_CMD_TOKENS)
		{
			// Will not fit any more
			return;
		}
	}
}

NOXREF cmd_function_t *Cmd_FindCmd(const char *cmd_name)
{
	NOXREFCHECK;

	cmd_function_t *cmd;

	for (cmd = cmd_functions; cmd; cmd = cmd->next)
	{
		if (!Q_stricmp(cmd_name, cmd->name))
		{
			return cmd;
		}
	}

	return NULL;
}

cmd_function_t *Cmd_FindCmdPrev(const char *cmd_name)
{
	cmd_function_t *cmd = NULL;

	if (cmd_functions == NULL)
	{
		return NULL;
	}

	for (cmd = cmd_functions; cmd->next; cmd = cmd->next)
	{
		if (!Q_stricmp(cmd_name, cmd->next->name))
		{
			return cmd;
		}
	}

	return NULL;
}

void Cmd_InsertCommand(cmd_function_t *cmd)
{
	cmd_function_t *c, **p;

	// Commands list is alphabetically sorted, search where to push
	c = cmd_functions;
	p = &cmd_functions;
	while (c)
	{
		if (Q_stricmp(c->name, cmd->name) > 0)
		{
			// Current command name is bigger, insert before it
			cmd->next = c;
			*p = cmd;
			return;
		}
		p = &c->next;
		c = c->next;
	}

	// All commands in the list are lower then the new one
	cmd->next = NULL;
	*p = cmd;
}

// Use this for engine inside call only, not from user code, because it doesn't alloc string for the name.
void Cmd_AddCommand(const char *cmd_name, xcommand_t function)
{
	cmd_function_t *cmd;

	if (host_initialized)
	{
		Sys_Error("%s: called after host_initialized", __func__);
	}

	// Check in variables list
	if (Cvar_FindVar(cmd_name) != NULL)
	{
		Con_Printf("%s: \"%s\" already defined as a var\n", __func__, cmd_name);
		return;
	}

	// Check if this command is already defined
	if (Cmd_Exists(cmd_name))
	{
		Con_Printf("%s: \"%s\" already defined\n", __func__, cmd_name);
		return;
	}

	// Create cmd_function
	cmd = (cmd_function_t *)Hunk_Alloc(sizeof(cmd_function_t));
	cmd->name = cmd_name;
	cmd->function = function ? function : Cmd_ForwardToServer;
	cmd->flags = 0;

	Cmd_InsertCommand(cmd);
}

// Use this for call from user code, because it alloc string for the name.
void Cmd_AddMallocCommand(const char *cmd_name, xcommand_t function, int flag)
{
	cmd_function_t *cmd;

	// Check in variables list
	if (Cvar_FindVar(cmd_name) != NULL)
	{
		Con_Printf("%s: \"%s\" already defined as a var\n", __func__, cmd_name);
		return;
	}

	// Check if this command is already defined
	if (Cmd_Exists(cmd_name))
	{
		Con_Printf("%s: \"%s\" already defined\n", __func__, cmd_name);
		return;
	}

	// Create cmd_function
	cmd = (cmd_function_t *)Mem_Malloc(sizeof(cmd_function_t));
	cmd->name = CopyString(cmd_name);	// alloc string, so it will not dissapear on side modules unloading and to maintain the same name during run
	cmd->function = function ? function : Cmd_ForwardToServer;
	cmd->flags = flag;

	Cmd_InsertCommand(cmd);
}

NOXREF void Cmd_AddHUDCommand(const char *cmd_name, xcommand_t function)
{
	NOXREFCHECK;

	Cmd_AddMallocCommand(cmd_name, function, FCMD_HUD_COMMAND);
}

void Cmd_AddWrapperCommand(const char *cmd_name, xcommand_t function)
{
	Cmd_AddMallocCommand(cmd_name, function, FCMD_WRAPPER_COMMAND);
}

void EXT_FUNC Cmd_AddGameCommand(const char *cmd_name, xcommand_t function)
{
	Cmd_AddMallocCommand(cmd_name, function, FCMD_GAME_COMMAND);
}

void EXT_FUNC Cmd_RemoveCmd(const char *cmd_name)
{
	auto prev = Cmd_FindCmdPrev(cmd_name);

	if (prev) {
		auto cmd = prev->next;
		prev->next = cmd->next;

		Z_Free((void*)cmd->name);
		Mem_Free(cmd);
	}
}

void Cmd_RemoveMallocedCmds(int flag)
{
	cmd_function_t *c, **p;

	c = cmd_functions;
	p = &cmd_functions;
	while (c)
	{
		if (c->flags & flag)
		{
			*p = c->next;
			Z_Free((void*)c->name);
			Mem_Free(c);
			c = *p;
			continue;
		}
		p = &c->next;
		c = c->next;
	}
}

NOXREF void Cmd_RemoveHudCmds(void)
{
	NOXREFCHECK;

	Cmd_RemoveMallocedCmds(FCMD_HUD_COMMAND);
}

void Cmd_RemoveGameCmds(void)
{
	Cmd_RemoveMallocedCmds(FCMD_GAME_COMMAND);
}

void Cmd_RemoveWrapperCmds(void)
{
	Cmd_RemoveMallocedCmds(FCMD_WRAPPER_COMMAND);
}

qboolean Cmd_Exists(const char *cmd_name)
{
	cmd_function_t *cmd = cmd_functions;

	while (cmd)
	{
		if (!Q_stricmp(cmd_name, cmd->name))
		{
			return TRUE;
		}

		cmd = cmd->next;
	}

	return FALSE;
}

NOXREF const char *Cmd_CompleteCommand(const char *search, int forward)
{
	NOXREFCHECK;

	// TODO: We have a command name length limit here: prepare for unforeseen consequences!
	static char lastpartial[256];
	char partial[256];
	cmd_function_t *cmd;
	int len;
	char *pPartial;

	Q_strncpy(partial, search, 255);
	partial[255] = 0;
	len = Q_strlen(partial);

	// Trim tail spaces
	for (pPartial = partial + len - 1; pPartial >= partial && *pPartial == ' '; pPartial--, len--)
	{
		*pPartial = 0;
	}

	if (!len)
	{
		return NULL;
	}

	if (!Q_stricmp(partial, lastpartial))
	{
		// Same partial, find this then next/prev cvar, if any.
		// TODO: But where it match for entered by user partial? Because we store full name
		cmd = Cmd_FindCmd(partial);
		if (cmd)
		{
			cmd = forward == 1 ? cmd->next : Cmd_FindCmdPrev(cmd->name);
			if (cmd)
			{
				Q_strncpy(lastpartial, cmd->name, 255);
				lastpartial[255] = 0;
				return cmd->name;
			}
		}
	}

	// Find first matching cvar
	for (cmd = cmd_functions; cmd != NULL; cmd = cmd->next)
	{
		if (!Q_strnicmp(partial, cmd->name, len))
		{
			// Store matched cvar name
			Q_strncpy(lastpartial, cmd->name, 255);
			lastpartial[255] = 0;
			return cmd->name;
		}
	}

	return NULL;
}

bool EXT_FUNC ValidateCmd_API(const char* cmd, cmd_source_t src, IGameClient* client) {
	return true;
}

void EXT_FUNC Cmd_ExecuteString_internal(const char* cmdName, cmd_source_t src, IGameClient* client) {
	// Search in functions
	cmd_function_t *cmd = cmd_functions;
	while (cmd)
	{
		if (!Q_stricmp(cmdName, cmd->name))
		{
			cmd->function();

			if (g_pcls.demorecording && (cmd->flags & FCMD_HUD_COMMAND) && !g_pcls.spectator)
			{
				CL_RecordHUDCommand(cmd->name);
			}

			return;
		}

		cmd = cmd->next;
	}

	// Search in aliases
	cmdalias_t *a = cmd_alias;
	while (a)
	{
		if (!Q_stricmp(cmdName, a->name))
		{

			Cbuf_InsertText(a->value);
			return;
		}

		a = a->next;
	}

	// Search in cvars
	if (!Cvar_Command())
	{
		// Send to a server if nothing processed locally and connected
		if (g_pcls.state >= ca_connected)
			Cmd_ForwardToServer();
#ifdef REHLDS_FIXES
		else if (sv_echo_unknown_cmd.string[0] == '1' && src == src_command)
			Con_Printf("unknown command \"%s\"\n", cmdName);
#endif
	}
}

void Cmd_ExecuteString(char *text, cmd_source_t src)
{
	cmd_source = src;
	Cmd_TokenizeString(text);

	if (!Cmd_Argc())
	{
		return;
	}

	IGameClient* cl = (src == src_client) ? GetRehldsApiClient(host_client) : NULL;
	if (!g_RehldsHookchains.m_ValidateCommand.callChain(ValidateCmd_API, cmd_argv[0], src, cl))
		return;

	g_RehldsHookchains.m_ExecuteServerStringCmd.callChain(Cmd_ExecuteString_internal, cmd_argv[0], src, cl);
}

qboolean Cmd_ForwardToServerInternal(sizebuf_t *pBuf)
{
	const char *cmd_name = Cmd_Argv(0);

	if (g_pcls.state <= ca_disconnected)
	{
		if (Q_stricmp(cmd_name, "setinfo"))
		{
			Con_Printf("Can't \"%s\", not connected\n", cmd_name);
		}

		return FALSE;
	}

	if (g_pcls.demoplayback || g_bIsDedicatedServer)
	{
		return FALSE;
	}

	char tempData[4096], buffername[64];
	sizebuf_t tempBuf;

	Q_sprintf(buffername, "%s::%s", __func__, nameof_variable(tempBuf));

	tempBuf.buffername = buffername;
	tempBuf.data = (byte *)tempData;
	tempBuf.maxsize = 4096;
	tempBuf.cursize = 0;
	tempBuf.flags = SIZEBUF_ALLOW_OVERFLOW;

	MSG_WriteByte(&tempBuf, clc_stringcmd);

	if (Q_stricmp(cmd_name, "cmd"))
	{
		SZ_Print(&tempBuf, cmd_name);
		SZ_Print(&tempBuf, " ");
	}

	SZ_Print(&tempBuf, Cmd_Argc() <= 1 ? "\n" : Cmd_Args());

	if (tempBuf.flags & SIZEBUF_OVERFLOWED)
	{
		return FALSE;
	}

	if (tempBuf.cursize + pBuf->cursize <= pBuf->maxsize)
	{
		SZ_Write(pBuf, tempBuf.data, tempBuf.cursize);
		return TRUE;
	}

	return FALSE;
}

void Cmd_ForwardToServer(void)
{
	if (Q_stricmp(Cmd_Argv(0), "cmd") || Q_stricmp(Cmd_Argv(1), "dlfile"))
	{
		Cmd_ForwardToServerInternal(&g_pcls.netchan.message);
	}
}

qboolean Cmd_ForwardToServerUnreliable(void)
{
	return Cmd_ForwardToServerInternal(&g_pcls.datagram);
}

// Returns the position (1 to argc-1) in the command's argument list
// where the given parameter apears, or 0 if not present.
NOXREF int Cmd_CheckParm(const char *parm)
{
	NOXREFCHECK;

	if (!parm)
	{
		Sys_Error("%s: NULL", __func__);
	}

	int c = Cmd_Argc();

	for (int i = 1; i < c; i++)
	{
		if (!Q_stricmp(Cmd_Argv(i), parm))
		{
			return i;
		}
	}

	return 0;
}

void Cmd_CmdList_f(void)
{
	cmd_function_t *cmd;
	int iCmds;
	int iArgs;
	const char *partial, *arg1;
	int ipLen;
	char szTemp[MAX_PATH];
	FileHandle_t f;
	FileHandle_t fp;
	qboolean bLogging;

	iCmds = 0;
	partial = NULL;
	f = NULL;
	fp = NULL;
	bLogging = FALSE;

	iArgs = Cmd_Argc();
	if (iArgs > 1)
	{
		arg1 = Cmd_Argv(1);

		if (!Q_stricmp(arg1, "?"))
		{
			Con_Printf("CmdList           : List all commands\nCmdList [Partial] : List commands starting with 'Partial'\nCmdList log [Partial] : Logs commands to file \"cmdlist.txt\" in the gamedir.\n");
			return;
		}

		if (!Q_stricmp(arg1, "log"))
		{
			// Open log
			int i;
			for (i = 0; i < 100; i++)
			{
				Q_snprintf(szTemp, ARRAYSIZE(szTemp) - 1, "cmdlist%02d.txt", i);
				szTemp[ARRAYSIZE(szTemp) - 1] = 0;

				fp = FS_Open(szTemp, "r");
				if (!fp)
				{
					break;
				}
				FS_Close(fp);
			}

			if (i >= 100)
			{
				Con_Printf("Can't cmdlist! Too many existing cmdlist output files in the gamedir!\n");
				return;
			}

			f = FS_Open(szTemp, "wt");
			if (!f)
			{
				Con_Printf("Couldn't open \"%s\" for writing!\n", szTemp);
				return;
			}
			bLogging = TRUE;

			// Get next argument into partial, if present
			if (iArgs >= 2)
			{
				partial = Cmd_Argv(2);
				ipLen = Q_strlen(partial);
			}
		}
		else
		{
			partial = arg1;
			ipLen = Q_strlen(partial);
		}
	}

	// Print commands
	Con_Printf("Command List\n--------------\n");

	for (cmd = cmd_functions; cmd; cmd = cmd->next)
	{
		if (partial && Q_strnicmp(cmd->name, partial, ipLen))
		{
			continue;
		}

		Con_Printf("%s\n", cmd->name);

		if (bLogging)
		{
			FS_FPrintf(f, "%s\n", cmd->name);
		}

		iCmds++;
	}

	if (partial && *partial)
	{
		Con_Printf("--------------\n%3i Commands for [%s]\nCmdList ? for syntax\n", iCmds, partial);
	}
	else
	{
		Con_Printf("--------------\n%3i Total Commands\nCmdList ? for syntax\n", iCmds);
	}

	// Close log
	if (bLogging)
	{
		FS_Close(f);
		Con_Printf("cmdlist logged to %s\n", szTemp);
	}
}
