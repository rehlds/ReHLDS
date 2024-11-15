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

double realtime;
double rolling_fps;
quakeparms_t host_parms;
qboolean host_initialized;
double host_frametime;
//unsigned short *host_basepal;
int host_framecount;
//int minimum_memory;
client_t *host_client;
qboolean gfNoMasterServer;
//qboolean g_bUsingInGameAdvertisements;
double oldrealtime;
int host_hunklevel;
jmp_buf host_abortserver;
jmp_buf host_enddemo;
unsigned short *host_basepal;
//unsigned char *host_colormap;
//const char *g_InGameAdsAllowed[3];

cvar_t host_name = { "hostname", "Half-Life", 0, 0.0f, NULL };
cvar_t host_speeds = { "host_speeds", "0", 0, 0.0f, NULL };
cvar_t host_profile = { "host_profile", "0", 0, 0.0f, NULL };
cvar_t developer = { "developer", "0", 0, 0.0f, NULL };
cvar_t host_limitlocal = { "host_limitlocal", "0", 0, 0.0f, NULL };
cvar_t skill = { "skill", "1", 0, 0.0f, NULL };
cvar_t deathmatch = { "deathmatch", "0", FCVAR_SERVER, 0.0f, NULL };
cvar_t coop = { "coop", "0", FCVAR_SERVER, 0.0f, NULL };

cvar_t sys_ticrate = { "sys_ticrate", "100.0", 0, 0.0f, NULL };

void sys_timescale_hook_callback(cvar_t *cvar);
cvarhook_t sys_timescale_hook = { sys_timescale_hook_callback, NULL, NULL };
cvar_t sys_timescale = { "sys_timescale", "1.0", 0, 0.0f, NULL };

cvar_t fps_max = { "fps_max", "100.0", FCVAR_ARCHIVE, 0.0f, NULL };
cvar_t host_killtime = { "host_killtime", "0.0", 0, 0.0f, NULL };
cvar_t sv_stats = { "sv_stats", "1", 0, 0.0f, NULL };
cvar_t fps_override = { "fps_override", "0", 0, 0.0f, NULL };
cvar_t host_framerate = { "host_framerate", "0", 0, 0.0f, NULL };
cvar_t pausable = { "pausable", "1", FCVAR_SERVER, 0.0f, NULL };
cvar_t suitvolume = { "suitvolume", "0.25", FCVAR_ARCHIVE, 0.0f, NULL };

NOXREF void Host_EndGame(const char *message, ...)
{
	NOXREFCHECK;
	int oldn;
	va_list argptr;
	char string[1024];

	va_start(argptr,message);
	Q_vsnprintf(string, sizeof(string), message, argptr);
	va_end(argptr);

	Con_DPrintf("%s: %s\n", __func__, string);

	oldn = g_pcls.demonum;

	if (g_psv.active)
		Host_ShutdownServer(FALSE);

	g_pcls.demonum = oldn;

	if (!g_pcls.state)
	{
		Sys_Error("%s: %s\n", __func__, string);
	}

	if (oldn != -1)
	{
		CL_Disconnect_f();
		g_pcls.demonum = oldn;
		Host_NextDemo();
		longjmp(host_enddemo, 1);
	}

	CL_Disconnect();
	Cbuf_AddText("cd stop\n");
	Cbuf_Execute();
	longjmp(host_abortserver, 1);
}

void NORETURN Host_Error(const char *error, ...)
{
	va_list argptr;
	char string[1024];
	static qboolean inerror = FALSE;

	va_start(argptr, error);

	if (inerror)
		Sys_Error("%s: recursively entered", __func__);

	inerror = TRUE;
	SCR_EndLoadingPlaque();
	Q_vsnprintf(string, sizeof(string), error, argptr);
	va_end(argptr);

	if (g_psv.active && developer.value != 0.0 )
		CL_WriteMessageHistory(0, 0);

	Con_Printf("%s: %s\n", __func__, string);
	if (g_psv.active)
		Host_ShutdownServer(FALSE);

	if (g_pcls.state)
	{
		CL_Disconnect();
		g_pcls.demonum = -1;
		inerror = FALSE;
		longjmp(host_abortserver, 1);
	}
	Sys_Error("%s: %s\n", __func__, string);
}

void Host_InitLocal(void)
{
	Host_InitCommands();
	Cvar_RegisterVariable(&host_killtime);
	Cvar_RegisterVariable(&sys_ticrate);
	Cvar_RegisterVariable(&sys_timescale);
	Cvar_HookVariable(sys_timescale.name, &sys_timescale_hook);

	Cvar_RegisterVariable(&fps_max);
	Cvar_RegisterVariable(&fps_override);
	Cvar_RegisterVariable(&host_name);
	Cvar_RegisterVariable(&host_limitlocal);

	sys_timescale.value = 1.0f;

	Cvar_RegisterVariable(&host_framerate);
	Cvar_RegisterVariable(&host_speeds);
	Cvar_RegisterVariable(&host_profile);
	Cvar_RegisterVariable(&mp_logfile);
	Cvar_RegisterVariable(&mp_logecho);
	Cvar_RegisterVariable(&sv_log_onefile);
	Cvar_RegisterVariable(&sv_log_singleplayer);
	Cvar_RegisterVariable(&sv_logsecret);
	Cvar_RegisterVariable(&sv_stats);
	Cvar_RegisterVariable(&developer);
	Cvar_RegisterVariable(&deathmatch);
	Cvar_RegisterVariable(&coop);
	Cvar_RegisterVariable(&pausable);
	Cvar_RegisterVariable(&skill);

	SV_SetMaxclients();
}

NOXREF void Info_WriteVars(FileHandle_t fp)
{
	NOXREFCHECK;
	cvar_t *pcvar;
	char *s;
	char pkey[512];

	static char value[4][512];
	static int valueindex;

	char *o;

	valueindex = (valueindex + 1) % 4;
	s = &g_pcls.userinfo[0];

	if (*s == '\\')
		s++;

	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}

		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		pcvar = Cvar_FindVar(pkey);
		if (!pcvar && pkey[0] != '*')
			FS_FPrintf(fp, "setinfo \"%s\" \"%s\"\n", pkey, value[valueindex]);

		if (!*s)
			return;
		s++;
	}
}

void Host_WriteConfiguration(void)
{
#ifndef SWDS
	FILE *f;
	kbutton_t *ml;
	kbutton_t *jl;
	qboolean bSetFileToReadOnly;
	char nameBuf[4096];

	if (!host_initialized || g_pcls.state == ca_dedicated)
		return;

#ifdef _WIN32
	Sys_GetRegKeyValue("Software\\Valve\\Steam", "rate", rate_.string);
	if (cl_name.string && Q_stricmp(cl_name.string, "unnamed") && Q_stricmp(cl_name.string, "player") && Q_strlen(cl_name.string))
		Sys_GetRegKeyValue("Software\\Valve\\Steam", "LastGameNameUsed", cl_name.string);
#else
	SetRateRegistrySetting(rate_.string);
#endif // _WIN32
	if (Key_CountBindings() <= 1)
	{
		Con_Printf("skipping config.cfg output, no keys bound\n");
		return;
	}

	bSetFileToReadOnly = FALSE;
	f = FS_OpenPathID("config.cfg", "w", "GAMECONFIG");
	if (!f)
	{
		if (!developer.value || !FS_FileExists("../goldsrc/dev_build_all.bat"))
		{
			if (FS_GetLocalPath("config.cfg", nameBuf, sizeof(nameBuf)))
			{
				bSetFileToReadOnly = TRUE;
				chmod(nameBuf, S_IREAD|S_IWRITE);
			}
			f = FS_OpenPathID("config.cfg", "w", "GAMECONFIG");
			if (!f)
			{
				Con_Printf("Couldn't write config.cfg.\n");
				return;
			}
		}
	}

	FS_FPrintf(f, "// This file is overwritten whenever you change your user settings in the game.\n");
	FS_FPrintf(f, "// Add custom configurations to the file \"userconfig.cfg\".\n\n");
	FS_FPrintf(f, "unbindall\n");

	Key_WriteBindings(f);
	Cvar_WriteVariables(f);
	Info_WriteVars(f);

	ml = ClientDLL_FindKey("in_mlook");
	jl = ClientDLL_FindKey("in_jlook");

	if (ml && (ml->state & 1))
		FS_FPrintf(f, "+mlook\n");

	if (jl && (jl->state & 1))
		FS_FPrintf(f, "+jlook\n");

	FS_FPrintf(f, "exec userconfig.cfg\n");
	FS_Close(f);

	if (bSetFileToReadOnly)
	{
		FS_GetLocalPath("config.cfg", nameBuf, sizeof(nameBuf));
		chmod(nameBuf, S_IREAD);
	}
#endif // SWDS
}

void Host_WriteCustomConfig(void)
{
#ifndef SWDS
	FILE *f;
	kbutton_t *ml;
	kbutton_t *jl;
#endif
	char configname[261];
	Q_snprintf(configname, 257, "%s", Cmd_Args());
	if (Q_strstr(configname, "..")
		|| !Q_stricmp(configname, "config")
		|| !Q_stricmp(configname, "autoexec")
		|| !Q_stricmp(configname, "listenserver")
		|| !Q_stricmp(configname, "server")
		|| !Q_stricmp(configname, "userconfig"))
	{
		Con_Printf("skipping writecfg output, invalid filename given\n");
	}
#ifndef SWDS
	else
	{
		if (host_initialized && g_pcls.state != ca_dedicated)
		{
			if (Key_CountBindings() < 2)
				Con_Printf("skipping config.cfg output, no keys bound\n");
			else
			{
				Q_strcat(configname, ".cfg");
				f = FS_OpenPathID(configname, "w", "GAMECONFIG");
				if (!f)
				{
					Con_Printf("Couldn't write %s.\n", configname);
					return;
				}

				FS_FPrintf(f, "unbindall\n");
				Key_WriteBindings(f);
				Cvar_WriteVariables(f);
				Info_WriteVars(f);

				ml = ClientDLL_FindKey("in_mlook");
				jl = ClientDLL_FindKey("in_jlook");

				if (ml && ml->state & 1)
					FS_FPrintf(f, "+mlook\n");

				if (jl && jl->state & 1)
					FS_FPrintf(f, "+jlook\n");

				FS_Close(f);
				Con_Printf("%s successfully created!\n", configname);
			}
		}
	}
#endif // SWDS
}

void SV_ClientPrintf(const char *fmt, ...)
{
	if (!host_client->fakeclient)
	{
		va_list va;
		char string[1024];

		va_start(va, fmt);
		Q_vsnprintf(string, ARRAYSIZE(string) - 1, fmt, va);
		va_end(va);

		g_RehldsHookchains.m_SV_ClientPrintf.callChain(SV_ClientPrintf_internal, string);
	}
}

void EXT_FUNC SV_ClientPrintf_internal(const char *Dest)
{
	char string[1024];

	Q_strlcpy(string, Dest);

	MSG_WriteByte(&host_client->netchan.message, svc_print);
	MSG_WriteString(&host_client->netchan.message, string);
}

void SV_BroadcastPrintf(const char *fmt, ...)
{
	va_list argptr;
	char string[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(string, ARRAYSIZE(string) - 1, fmt, argptr);
	va_end(argptr);

	string[ARRAYSIZE(string) - 1] = 0;

	for (int i = 0; i < g_psvs.maxclients; i++)
	{
		client_t *cl = &g_psvs.clients[i];
		if ((cl->active || cl->spawned) && !cl->fakeclient)
		{
			MSG_WriteByte(&cl->netchan.message, svc_print);
			MSG_WriteString(&cl->netchan.message, string);
		}
	}
	Con_DPrintf("%s", string);
}

void Host_ClientCommands(const char *fmt, ...)
{
	va_list argptr;
	char string[1024];

	va_start(argptr, fmt);
	if (!host_client->fakeclient)
	{
		Q_vsnprintf(string, sizeof(string), fmt, argptr);
		string[sizeof(string) - 1] = 0;

		MSG_WriteByte(&host_client->netchan.message, svc_stufftext);
		MSG_WriteString(&host_client->netchan.message, string);
	}
	va_end(argptr);
}

void EXT_FUNC SV_DropClient_hook(IGameClient* cl, bool crash, const char *string)
{
	SV_DropClient_internal(cl->GetClient(), crash ? TRUE : FALSE, string);
}

void EXT_FUNC SV_DropClient_api(IGameClient* cl, bool crash, const char* fmt, ...)
{
	char buf[1024];
	va_list argptr;

	va_start(argptr, fmt);
	Q_vsnprintf(buf, ARRAYSIZE(buf) - 1, fmt, argptr);
	va_end(argptr);

	g_RehldsHookchains.m_SV_DropClient.callChain(SV_DropClient_hook, cl, crash, buf);
}

void SV_DropClient(client_t *cl, qboolean crash, const char *fmt, ...)
{
	char buf[1024];
	va_list argptr;

	va_start(argptr, fmt);
	Q_vsnprintf(buf, ARRAYSIZE(buf) - 1, fmt, argptr);
	va_end(argptr);

	g_RehldsHookchains.m_SV_DropClient.callChain(SV_DropClient_hook, GetRehldsApiClient(cl), crash != FALSE, buf);
}

void SV_DropClient_internal(client_t *cl, qboolean crash, const char *string)
{
	int i;
	unsigned char final[512];
	float connection_time;

	i = 0;

	if (!crash)
	{
		if (!cl->fakeclient)
		{
			MSG_WriteByte(&cl->netchan.message, svc_disconnect);
			MSG_WriteString(&cl->netchan.message, string);
			final[0] = svc_disconnect;
			Q_strncpy((char *)&final[1], string, Q_min(sizeof(final) - 1, Q_strlen(string) + 1));
			final[sizeof(final) - 1] = 0;
			i = 1 + Q_min(sizeof(final) - 1, Q_strlen(string) + 1);
		}
		if (cl->edict && cl->spawned)
			gEntityInterface.pfnClientDisconnect(cl->edict);

		Sys_Printf("Dropped %s from server\nReason:  %s\n", cl->name, string);

		if (!cl->fakeclient)
			Netchan_Transmit(&cl->netchan, i, final);

		cl->network_userid.m_SteamIDValidated = 0;
		host_client->network_userid.m_SteamIDValidated = 0;
	}

	connection_time = realtime - cl->netchan.connect_time;
	if (connection_time > 60.0)
	{
		++g_psvs.stats.num_sessions;
		g_psvs.stats.cumulative_sessiontime = g_psvs.stats.cumulative_sessiontime + connection_time;
	}

#ifdef REHLDS_FIXES
	// prevent message reading after disconnect
	if (cl == host_client)
		msg_readcount = net_message.cursize;
#endif // REHLDS_FIXES

	Netchan_Clear(&cl->netchan);

	Steam_NotifyClientDisconnect(cl);

	cl->active = FALSE;
	cl->connected = FALSE;
	cl->hasusrmsgs = FALSE;
	cl->fakeclient = FALSE;
	cl->spawned = FALSE;
	cl->fully_connected = FALSE;
	cl->name[0] = 0;
	cl->connection_started = realtime;
	cl->proxy = FALSE;
	COM_ClearCustomizationList(&cl->customdata, FALSE);
#ifdef REHLDS_FIXES
	if (cl->edict)
	{
		// Reset flags, leave FL_DORMANT used by CS
		cl->edict->v.flags &= FL_DORMANT;
		// Since the edict doesn't get deleted, fix it so it doesn't interfere.
		cl->edict->v.takedamage = DAMAGE_NO;	// don't attract autoaim
		cl->edict->v.solid = SOLID_NOT;
	}
#endif // REHLDS_FIXES
	cl->edict = NULL;
	Q_memset(cl->userinfo, 0, sizeof(cl->userinfo));
	Q_memset(cl->physinfo, 0, sizeof(cl->physinfo));

#ifdef REHLDS_FIXES
	g_GameClients[cl - g_psvs.clients]->SetSpawnedOnce(false);
#endif // REHLDS_FIXES

	SV_SendFullClientUpdateForAll(cl);

	NotifyDedicatedServerUI("UpdatePlayers");
}

void Host_ClearClients(qboolean bFramesOnly)
{
	int i;
	int j;
	client_frame_t *frame;
	netadr_t save;

	host_client = g_psvs.clients;
	for (i = 0; i < g_psvs.maxclients; i++, host_client++)
	{
		if (host_client->frames)
		{
			for (j = 0; j < SV_UPDATE_BACKUP; j++)
			{
				frame = &(host_client->frames[j]);
				SV_ClearPacketEntities(frame);
				frame->senttime = 0;
				frame->ping_time = -1;
			}
		}
		if (host_client->netchan.remote_address.type)
		{
			Q_memcpy(&save, &host_client->netchan.remote_address, sizeof(netadr_t));
			Q_memset(&host_client->netchan, 0, sizeof(netchan_t));
			Netchan_Setup(NS_SERVER, &host_client->netchan, save, host_client - g_psvs.clients, (void *)host_client, SV_GetFragmentSize);
		}
		COM_ClearCustomizationList(&host_client->customdata, 0);
	}

	if (bFramesOnly == FALSE)
	{
		host_client = g_psvs.clients;
		for (i = 0; i < g_psvs.maxclientslimit; i++, host_client++)
			SV_ClearFrames(&host_client->frames);

		Q_memset(g_psvs.clients, 0, sizeof(client_t) * g_psvs.maxclientslimit);
		SV_AllocClientFrames();
	}
}

void Host_ShutdownServer(qboolean crash)
{
	int i;
	if (!g_psv.active)
		return;

	SV_ServerShutdown();
	g_psv.active = FALSE;
	NET_ClearLagData(TRUE, TRUE);

	host_client = g_psvs.clients;
	for (i = 0; i < g_psvs.maxclients; i++, host_client++)
	{
		if (host_client->active || host_client->connected)
			SV_DropClient(host_client, crash, "Server shutting down");
	}

	CL_Disconnect();
	SV_ClearEntities();
	SV_ClearCaches();
	FreeAllEntPrivateData();
	Q_memset(&g_psv, 0, sizeof(server_t));
	CL_ClearClientState();

	SV_ClearClientStates();
	Host_ClearClients(FALSE);

	host_client = g_psvs.clients;
	for (i = 0; i < g_psvs.maxclientslimit; i++, host_client++)
		SV_ClearFrames(&host_client->frames);

	Q_memset(g_psvs.clients, 0, sizeof(client_t) * g_psvs.maxclientslimit);
	HPAK_FlushHostQueue();
	Steam_Shutdown();
	Log_Printf("Server shutdown\n");
	Log_Close();
}

void SV_ClearClientStates(void)
{
	int i;
	client_t *cl;

	for (i = 0, cl = g_psvs.clients; i < g_psvs.maxclients; i++, cl++)
	{
		COM_ClearCustomizationList(&cl->customdata, FALSE);
		SV_ClearResourceLists(cl);
	}
}

void Host_CheckDyanmicStructures(void)
{
	int i;
	client_t *cl;

	cl = g_psvs.clients;
	for (i = 0; i < g_psvs.maxclients; i++, cl++)
	{
		if (cl->frames)
			SV_ClearFrames(&cl->frames);
	}
}

void Host_ClearMemory(qboolean bQuiet)
{

	//Engine string pooling
#ifdef REHLDS_FIXES
	Ed_StrPool_Reset();
#endif //REHLDS_FIXES

	CM_FreePAS();
	SV_ClearEntities();

	if (!bQuiet)
		Con_DPrintf("Clearing memory\n");

	D_FlushCaches();
	Mod_ClearAll();
	if (host_hunklevel)
	{
		Host_CheckDyanmicStructures();
		Hunk_FreeToLowMark(host_hunklevel);
	}

	g_pcls.signon = 0;
	SV_ClearCaches();
	Q_memset(&g_psv, 0, sizeof(server_t));
	CL_ClearClientState();
	SV_ClearClientStates();
}

qboolean Host_FilterTime(float time)
{
	float fps;
	static int command_line_ticrate = -1;

	if (host_framerate.value > 0.0f)
	{
		if (Host_IsSinglePlayerGame() || g_pcls.demoplayback)
		{
			host_frametime = sys_timescale.value * host_framerate.value;
			realtime += host_frametime;
			return TRUE;
		}
	}

	realtime += sys_timescale.value * time;
	if (g_bIsDedicatedServer)
	{
		if (command_line_ticrate == -1)
			command_line_ticrate = COM_CheckParm("-sys_ticrate");

		if (command_line_ticrate > 0)
			fps = Q_atof(com_argv[command_line_ticrate + 1]);
		else
			fps = sys_ticrate.value;

		if (fps > 0.0f)
		{
			if (1.0f / (fps + 1.0f) > realtime - oldrealtime)
				return FALSE;
		}
	}
	else
	{
		fps = 31.0f;
		if (g_psv.active || g_pcls.state == ca_disconnected || g_pcls.state == ca_active)
		{
			fps = 0.5f;
			if (fps_max.value >= 0.5f)
				fps = fps_max.value;
		}
		if (!fps_override.value)
		{
			if (fps > 100.0f)
				fps = 100.0f;
		}
		if (g_pcl.maxclients > 1)
		{
			if (fps < 20.0f)
				fps = 20.0f;
		}
		if (gl_vsync.value)
		{
			if (!fps_override.value)
				fps = 100.f;
		}
		if (!g_pcls.timedemo)
		{
			if (sys_timescale.value / (fps + 0.5f) > realtime - oldrealtime)
				return FALSE;
		}
	}

	host_frametime = realtime - oldrealtime;
	oldrealtime = realtime;

	if (host_frametime > 0.25f)
		host_frametime = 0.25f;

	return TRUE;
}

qboolean Master_IsLanGame(void)
{
	return sv_lan.value != 0.0f;
}

void Master_Heartbeat_f(void)
{
	//Steam_ForceHeartbeat in move?
	CRehldsPlatformHolder::get()->SteamGameServer()->ForceHeartbeat();
}

void Host_ComputeFPS(double frametime)
{
	rolling_fps = 0.6 * rolling_fps + 0.4 * frametime;
}

void Host_GetHostInfo(float *fps, int *nActive, int *unused, int *nMaxPlayers, char *pszMap)
{
	if (rolling_fps > 0.0)
	{
		*fps = 1.0 / rolling_fps;
	}
	else
	{
		rolling_fps = 0;
		*fps = 0;
	}

	int clients = 0;
	SV_CountPlayers(&clients);
	*nActive = clients;

	if (unused)
		*unused = 0;

	if (pszMap)
	{
		if (g_psv.name[0])
		{
			Q_strcpy(pszMap, g_psv.name);
		}
		else
		{
			*pszMap = 0;
		}
	}

	*nMaxPlayers = g_psvs.maxclients;
}

void Host_Speeds(double *time)
{
	float pass1, pass2, pass3, pass4, pass5;
	double frameTime;
	double fps;

#ifdef REHLDS_FIXES
	if (host_speeds.value != 0.0f)	// FIXED: do calculations only if host_speeds is enabled
#endif // REHLDS_FIXES
	{
		pass1 = (float)((time[1] - time[0]) * 1000.0);
		pass2 = (float)((time[2] - time[1]) * 1000.0);
		pass3 = (float)((time[3] - time[2]) * 1000.0);
		pass4 = (float)((time[4] - time[3]) * 1000.0);
		pass5 = (float)((time[5] - time[4]) * 1000.0);

		frameTime = (pass5 + pass4 + pass3 + pass2 + pass1) / 1000.0;

		if (frameTime >= 0.0001)
			fps = 1.0 / frameTime;
		else
			fps = 999.0;
#ifndef REHLDS_FIXES
	}

	if (host_speeds.value != 0.0f)
	{
#endif // REHLDS_FIXES
		int ent_count = 0;
		for (int i = 0; i < g_psv.num_edicts; i++)
		{
			if (!g_psv.edicts[i].free)
				++ent_count;
		}
		Con_Printf("%3i fps -- host(%3.0f) sv(%3.0f) cl(%3.0f) gfx(%3.0f) snd(%3.0f) ents(%d)\n", (int)fps, pass1, pass2, pass3, pass4, pass5, ent_count);
	}

#ifndef SWDS
	if (cl_gg.value != 0.0f)	// cvar_t cl_gamegauge
	{
		//sub_1D10B2D
		CL_GGSpeeds(time[3]);
	}
#endif // SWDS
}

void Host_UpdateScreen(void)
{
	if (!gfBackground)
	{
		SCR_UpdateScreen();
		if (cl_inmovie)
		{
			if (*(float *)&scr_con_current == 0.0f)
				VID_WriteBuffer(NULL);
		}
	}
}

void Host_UpdateSounds(void)
{
	if (!gfBackground)
	{
		//S_PrintStats();
	}
}

void Host_CheckConnectionFailure(void)
{
	static int frames = 5;
	if (g_pcls.state == ca_disconnected && (giSubState & 4 || console.value == 0.0f))
	{
		if (frames-- > 0)
			return;

		giActive = DLL_PAUSED;
		frames = 5;
	}
}

void _Host_Frame(float time)
{
	static double host_times[6];
	if (setjmp(host_enddemo))
		return;

	//Unknown_windows_func_01D37CD0();
	if (!Host_FilterTime(time))
		return;

#ifdef REHLDS_FLIGHT_REC
	static long frameCounter = 0;
	if (rehlds_flrec_frame.string[0] != '0') {
		FR_StartFrame(frameCounter);
	}
#endif //REHLDS_FLIGHT_REC

	SystemWrapper_RunFrame(host_frametime);

	if (g_modfuncs.m_pfnFrameBegin)
		g_modfuncs.m_pfnFrameBegin();

	Host_ComputeFPS(host_frametime);
	R_SetStackBase();
	CL_CheckClientState();
	Cbuf_Execute();
	ClientDLL_UpdateClientData();

	if (g_psv.active)
		CL_Move();

	host_times[1] = Sys_FloatTime();
	SV_Frame();
	host_times[2] = Sys_FloatTime();
	SV_CheckForRcon();

	if (!g_psv.active)
		CL_Move();

	ClientDLL_Frame(host_frametime);
	CL_SetLastUpdate();
	CL_ReadPackets();
	CL_RedoPrediction();
	CL_VoiceIdle();
	CL_EmitEntities();
	CL_CheckForResend();

	while (CL_RequestMissingResources())
		;
	CL_UpdateSoundFade();
	Host_CheckConnectionFailure();
	//CL_HTTPUpdate();
	Steam_ClientRunFrame();
	ClientDLL_CAM_Think();
	CL_MoveSpectatorCamera();
	host_times[3] = Sys_FloatTime();
	Host_UpdateScreen();
	host_times[4] = Sys_FloatTime();
	CL_DecayLights();
	Host_UpdateSounds();
	host_times[0] = host_times[5];
	host_times[5] = Sys_FloatTime();

	Host_Speeds(host_times);
	++host_framecount;
	CL_AdjustClock();

	if (sv_stats.value == 1.0f)
		Host_UpdateStats();

	if (host_killtime.value != 0.0 && host_killtime.value < g_psv.time)
	{
		Host_Quit_f();
	}

	//Rehlds Security
	Rehlds_Security_Frame();

#ifdef REHLDS_FLIGHT_REC
	if (rehlds_flrec_frame.string[0] != '0') {
		FR_EndFrame(frameCounter);
	}
	frameCounter++;
#endif //REHLDS_FLIGHT_REC
}

int Host_Frame(float time, int iState, int *stateInfo)
{
	double time1;
	double time2;

	if (setjmp(host_abortserver))
	{
		return giActive;
	}

	if (giActive != 3 || !g_iQuitCommandIssued)
		giActive = iState;

	*stateInfo = 0;
	if (host_profile.value != 0.0f)
		time1 = Sys_FloatTime();

	_Host_Frame(time);
	if (host_profile.value != 0.0f)
		time2 = Sys_FloatTime();

	if (giStateInfo)
	{
		*stateInfo = giStateInfo;
		giStateInfo = 0;
		Cbuf_Execute();
	}

	if (host_profile.value != 0.0f)
	{
		static double timetotal;
		static int timecount;

		timecount++;
		timetotal += time2 - time1;
		if (++timecount >= 1000)
		{
			int m = (timetotal * 1000.0 / (double)timecount);
			int c = 0;
			timecount = 0;
			timetotal = 0.0;
			for (int i = 0; i < g_psvs.maxclients; i++)
			{
				if (g_psvs.clients[i].active)
					++c;
			}

			Con_Printf("host_profile: %2i clients %2i msec\n", c, m);
		}
	}

	return giActive;
}

void CheckGore(void)
{
	float fValue = bLowViolenceBuild ? 0.0f : 1.0f;

	Cvar_SetValue("violence_hblood", fValue);
	Cvar_SetValue("violence_hgibs", fValue);
	Cvar_SetValue("violence_ablood", fValue);
	Cvar_SetValue("violence_agibs", fValue);
}

qboolean Host_IsSinglePlayerGame(void)
{
	if (g_psv.active)
		return g_psvs.maxclients == 1;
	else
		return g_pcl.maxclients == 1;

}

qboolean Host_IsServerActive(void)
{
	return g_psv.active;
}

void Host_Version(void)
{
	char szFileName[MAX_PATH];

	Q_strcpy(gpszVersionString, "1.0.1.4");
	Q_strcpy(gpszProductString, "valve");
	Q_strcpy(szFileName, "steam.inf");
	FileHandle_t fp = FS_Open(szFileName, "r");
	if (fp)
	{
		int bufsize = FS_Size(fp);
		char* buffer = (char*)Mem_Malloc(bufsize + 1);
		FS_Read(buffer, bufsize, 1, fp);
		char *pbuf = buffer;
		FS_Close(fp);
		buffer[bufsize] = 0;
		int gotKeys = 0;

		pbuf = COM_Parse(pbuf);
		if (pbuf)
		{
			while (Q_strlen(com_token) > 0 && gotKeys <= 1)
			{
				if (!Q_strnicmp(com_token, "PatchVersion=", Q_strlen("PatchVersion=")))
				{
					Q_strncpy(gpszVersionString, &com_token[Q_strlen("PatchVersion=")], sizeof(gpszVersionString));
					gpszVersionString[sizeof(gpszVersionString) - 1] = 0;
					if (COM_CheckParm("-steam"))
					{
						char szSteamVersionId[16];
						FS_GetInterfaceVersion(szSteamVersionId, sizeof(szSteamVersionId) - 1);
						Q_snprintf(gpszVersionString, sizeof(gpszVersionString), "%s/%s", &com_token[Q_strlen("PatchVersion=")], szSteamVersionId);
						gpszVersionString[sizeof(gpszVersionString) - 1] = 0;
					}
					++gotKeys;
				}
				else if (!Q_strnicmp(com_token, "ProductName=", Q_strlen("ProductName=")))
				{
					++gotKeys;
					Q_strncpy(gpszProductString, &com_token[Q_strlen("ProductName=")], sizeof(gpszProductString) - 1);
					gpszProductString[sizeof(gpszProductString) - 1] = 0;
				}

				pbuf = COM_Parse(pbuf);
				if (!pbuf)
					break;
			}
		}
		if (buffer)
			Mem_Free(buffer);
	}

	if (g_pcls.state != ca_dedicated)
	{
		Con_DPrintf("Protocol version %i\nExe version %s (%s)\n", PROTOCOL_VERSION, gpszVersionString, gpszProductString);
		Con_DPrintf("Exe build: " __BUILD_TIME__ " " __BUILD_DATE__ " (%i)\n", build_number());
	}
	else
	{
		Con_Printf("Protocol version %i\nExe version %s (%s)\n", PROTOCOL_VERSION, gpszVersionString, gpszProductString);
		Con_Printf("Exe build: " __BUILD_TIME__ " " __BUILD_DATE__ " (%i)\n", build_number());
	}
}

int Host_Init(quakeparms_t *parms)
{
	char versionString[256];

	CRehldsPlatformHolder::get()->srand(CRehldsPlatformHolder::get()->time(NULL));

	Q_memcpy(&host_parms, parms, sizeof(host_parms));
	com_argc = parms->argc;
	com_argv = parms->argv;
	realtime = 0;

	Memory_Init(parms->membase, parms->memsize);

	Voice_RegisterCvars();
	Cvar_RegisterVariable(&console);
	if (COM_CheckParm("-console") || COM_CheckParm("-toconsole") || COM_CheckParm("-dev"))
		Cvar_DirectSet(&console, "1.0");
	Host_InitLocal();
	if (COM_CheckParm("-dev"))
		Cvar_SetValue("developer", 1.0);

	//Engine string pooling
#ifdef REHLDS_FIXES
	Ed_StrPool_Init();
#endif //REHLDS_FIXES

	FR_Init(); //don't put it under REHLDS_FLIGHT_REC to allow recording via Rehlds API

	Cbuf_Init();
	Cmd_Init();
	Cvar_Init();
	Cvar_CmdInit();

#ifdef REHLDS_FLIGHT_REC
	FR_Rehlds_Init();
#endif //REHLDS_FLIGHT_REC

	V_Init();
	Chase_Init();
	COM_Init(parms->basedir);
	Host_ClearSaveDirectory();
	HPAK_Init();
	W_LoadWadFile("gfx.wad");
	W_LoadWadFile("fonts.wad");
	Key_Init();
	Con_Init();
	Decal_Init();
	Mod_Init();
	NET_Init();
	Netchan_Init();
	DELTA_Init();
	SV_Init();
	SystemWrapper_Init();
	Host_Version();

	//Rehlds Security
	Rehlds_Security_Init();


	Q_snprintf(versionString, sizeof(versionString), "%s,%i,%i", gpszVersionString, PROTOCOL_VERSION, build_number());
	Cvar_Set("sv_version", versionString);
	Con_DPrintf("%4.1f Mb heap\n", (double)parms->memsize / (1024.0f * 1024.0f));
	R_InitTextures();
	HPAK_CheckIntegrity("custom");
	Q_memset(&g_module, 0, sizeof(g_module));
	if (g_pcls.state != ca_dedicated)
	{
		//Sys_Error("%s: Only dedicated server mode is supported", __func__);

		color24 *disk_basepal = (color24 *)COM_LoadHunkFile("gfx/palette.lmp");
		if (!disk_basepal)
			Sys_Error("%s: Couldn't load gfx/palette.lmp", __func__);

		host_basepal = (unsigned short *)Hunk_AllocName(sizeof(PackedColorVec) * 256, "palette.lmp");
		for (int i = 0; i < 256; i++)
		{
			PackedColorVec *basepal = (PackedColorVec *)&host_basepal[i];

			basepal->b = disk_basepal->r;
			basepal->g = disk_basepal->g;
			basepal->r = disk_basepal->b;
			basepal->a = 0;//alpha

			disk_basepal++;
		}

		//GL_Init();
		PM_Init(&g_clmove);
		CL_InitEventSystem();
		ClientDLL_Init();
		//VGui_Startup();
		if (!VID_Init(host_basepal))
		{
			//VGui_Shutdown();
			return 0;
		}
		Draw_Init();
		SCR_Init();
		R_Init();
		S_Init();
		//CDAudio_Init();
		//Voice_Init("voice_speex", 1);
		//DemoPlayer_Init();
		CL_Init();
	}
	else
	{
		Cvar_RegisterVariable(&suitvolume);
#ifdef REHLDS_FIXES
		Cvar_RegisterVariable(&r_cachestudio);
#endif
	}

	Cbuf_InsertText("exec valve.rc\n");
	Hunk_AllocName(0, "-HOST_HUNKLEVEL-");
	host_hunklevel = Hunk_LowMark();
	giActive = DLL_ACTIVE;
	scr_skipupdate = FALSE;
	CheckGore();
	host_initialized = TRUE;
	return 1;
}

void Host_Shutdown(void)
{
	static qboolean isdown = FALSE;

	int i;
	client_t *pclient;

	if (isdown)
	{
		printf("recursive shutdown\n");
		return;
	}

	isdown = TRUE;
	if (host_initialized) // Client-side
		Host_WriteConfiguration();

#ifdef REHLDS_FIXES
	Host_ShutdownServer(FALSE);
#else
	SV_ServerShutdown();
#endif

	Voice_Deinit();
	host_initialized = FALSE;

	//CDAudio_Shutdown();
	//VGui_Shutdown();
	if (g_pcls.state != ca_dedicated)
		ClientDLL_Shutdown();

	//Rehlds Security
	Rehlds_Security_Shutdown();

	Cmd_RemoveGameCmds();
	Cmd_Shutdown();
	Cvar_Shutdown();
	HPAK_FlushHostQueue();
	SV_DeallocateDynamicData();

	pclient = g_psvs.clients;
	for (i = 0; i < g_psvs.maxclientslimit; i++, pclient++)
		SV_ClearFrames(&pclient->frames);

	SV_Shutdown();
	SystemWrapper_ShutDown();
	NET_Shutdown();
	S_Shutdown();
	Con_Shutdown();
	ReleaseEntityDlls();
	CL_ShutDownClientStatic();
	CM_FreePAS();

	if (wadpath)
	{
		Mem_Free(wadpath);
		wadpath = NULL;
	}

	if (g_pcls.state != ca_dedicated)
		Draw_Shutdown();

	Draw_DecalShutdown();
	W_Shutdown();
	Log_Printf("Server shutdown\n");
	Log_Close();
	COM_Shutdown();
	CL_Shutdown();
	DELTA_Shutdown();
	//Key_Shutdown();
	realtime = 0.0f;
	g_psv.time = 0.0f;
	g_pcl.time = 0.0f;
}

void sys_timescale_hook_callback(cvar_t *cvar)
{
	int i;
	client_t *client = NULL;

	if (!Host_IsServerActive())
		return;

	for (i = 0; i < g_psvs.maxclients; i++)
	{
		client = &g_psvs.clients[i];

		if (!client->fakeclient && (client->active || client->spawned || client->connected))
		{
			MSG_WriteByte(&client->netchan.message, svc_timescale);
			MSG_WriteFloat(&client->netchan.message, max(0.1f, sys_timescale.value));
		}
	}
}
