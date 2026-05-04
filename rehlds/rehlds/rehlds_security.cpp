#include "precompiled.h"

cvar_t sv_rehlds_movecmdrate_max_avg = { "sv_rehlds_movecmdrate_max_avg", "1800", 0, 1800.0f, NULL };
cvar_t sv_rehlds_movecmdrate_max_burst = { "sv_rehlds_movecmdrate_max_burst", "5500", 0, 5500.0f, NULL };
cvar_t sv_rehlds_stringcmdrate_max_avg = { "sv_rehlds_stringcmdrate_max_avg", "250", 0, 250.0f, NULL };
cvar_t sv_rehlds_stringcmdrate_max_burst = { "sv_rehlds_stringcmdrate_max_burst", "500", 0, 500.0f, NULL };

cvar_t sv_rehlds_movecmdrate_avg_punish = { "sv_rehlds_movecmdrate_avg_punish", "5", 0, 5.0f, NULL };
cvar_t sv_rehlds_movecmdrate_burst_punish = { "sv_rehlds_movecmdrate_burst_punish", "5", 0, 5.0f, NULL };
cvar_t sv_rehlds_stringcmdrate_avg_punish = { "sv_rehlds_stringcmdrate_avg_punish", "5", 0, 5.0f, NULL };
cvar_t sv_rehlds_stringcmdrate_burst_punish = { "sv_rehlds_stringcmdrate_burst_punish", "5", 0, 5.0f, NULL };

cvar_t sv_rehlds_movecmd_max_ticks = { "sv_rehlds_movecmd_max_ticks", "24", 0, 24.0f, NULL };
cvar_t sv_rehlds_movecmd_max_null_streak = { "sv_rehlds_movecmd_max_null_streak", "0", 0, 0.0f, NULL };
cvar_t sv_rehlds_movecmd_clamp_interp = { "sv_rehlds_movecmd_clamp_interp", "1", 0, 1.0f, NULL };
cvar_t sv_rehlds_movecmdtime_samples = { "sv_rehlds_movecmdtime_samples", "120", 0, 120.0f, NULL };
cvar_t sv_rehlds_movecmdtime_max_error = { "sv_rehlds_movecmdtime_max_error", "300", 0, 300.0f, NULL };
cvar_t sv_rehlds_movecmdtime_max_scale = { "sv_rehlds_movecmdtime_max_scale", "3.0", 0, 3.0f, NULL };
cvar_t sv_rehlds_movecmdtime_min_scale = { "sv_rehlds_movecmdtime_min_scale", "0.5", 0, 0.5f, NULL };
cvar_t sv_rehlds_movecmdtime_punish = { "sv_rehlds_movecmdtime_punish", "-1", 0, -1.0f, NULL };
cvar_t sv_rehlds_movecmdtime_max_warnings = { "sv_rehlds_movecmdtime_max_warnings", "-1", 0, -1.0f, NULL };

CMoveCommandRateLimiter g_MoveCommandRateLimiter;
CStringCommandsRateLimiter g_StringCommandsRateLimiter;
CUserCmdTimeLimiter g_UserCmdTimeLimiter;

CMoveCommandRateLimiter::CMoveCommandRateLimiter() {
	Q_memset(m_AverageMoveCmdRate, 0, sizeof(m_AverageMoveCmdRate));
	Q_memset(m_CurrentMoveCmds, 0, sizeof(m_CurrentMoveCmds));
	m_LastCheckTime = 0.0;
}

void CMoveCommandRateLimiter::UpdateAverageRates(double dt) {
	for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
		m_AverageMoveCmdRate[i] = (2.0 * m_AverageMoveCmdRate[i] / 3.0) + m_CurrentMoveCmds[i] / dt / 3.0;
		m_CurrentMoveCmds[i] = 0;

		CheckAverageRate(i);
	}
}

void CMoveCommandRateLimiter::Frame() {
	double currentTime = realtime;
	double dt = currentTime - m_LastCheckTime;

	if (dt < 0.5) { //refresh avg. rate every 0.5 sec
		return;
	}

	UpdateAverageRates(dt);
	m_LastCheckTime = currentTime;
}

void CMoveCommandRateLimiter::ClientConnected(unsigned int clientId) {
	m_CurrentMoveCmds[clientId] = 0;
	m_AverageMoveCmdRate[clientId] = 0.0f;
}

void CMoveCommandRateLimiter::MoveCommandsIssued(unsigned int clientId, unsigned int numCmds) {
	m_CurrentMoveCmds[clientId] += numCmds;
	CheckBurstRate(clientId);
}

void CMoveCommandRateLimiter::CheckBurstRate(unsigned int clientId) {
	client_t* cl = &g_psvs.clients[clientId];
	if (!cl->active || sv_rehlds_movecmdrate_max_burst.value <= 0.0f) {
		return;
	}

	double dt = realtime - m_LastCheckTime;
	if (dt < 0.2) {
		dt = 0.2; //small intervals may give too high rates
	}
	if ((m_CurrentMoveCmds[clientId] / dt) > sv_rehlds_movecmdrate_max_burst.value) {
		if(sv_rehlds_movecmdrate_burst_punish.value < 0) {
			Con_DPrintf("%s Kicked for move commands flooding (burst) (%.1f)\n", cl->name, (m_CurrentMoveCmds[clientId] / dt));
			SV_DropClient(cl, false, "Kicked for move commands flooding (burst)");
		}
		else
		{
			Con_DPrintf("%s Banned for move commands flooding (burst) (%.1f)\n", cl->name, (m_CurrentMoveCmds[clientId] / dt));
			Cbuf_AddText(va("addip %.1f %s\n", sv_rehlds_movecmdrate_burst_punish.value, NET_BaseAdrToString(cl->netchan.remote_address)));
			SV_DropClient(cl, false, "Banned for move commands flooding (burst)");
		}
	}
}

void CMoveCommandRateLimiter::CheckAverageRate(unsigned int clientId) {
	client_t* cl = &g_psvs.clients[clientId];
	if (!cl->active || sv_rehlds_movecmdrate_max_burst.value <= 0.0f) {
		return;
	}

	if (m_AverageMoveCmdRate[clientId] > sv_rehlds_movecmdrate_max_avg.value) {
		if(sv_rehlds_movecmdrate_avg_punish.value < 0) {
			Con_DPrintf("%s Kicked for move commands flooding (Avg) (%.1f)\n", cl->name, m_AverageMoveCmdRate[clientId]);
			SV_DropClient(cl, false, "Kicked for move commands flooding (Avg)");
		}
		else
		{
			Con_DPrintf("%s Banned for move commands flooding (Avg) (%.1f)\n", cl->name, m_AverageMoveCmdRate[clientId]);
			Cbuf_AddText(va("addip %.1f %s\n", sv_rehlds_movecmdrate_avg_punish.value, NET_BaseAdrToString(cl->netchan.remote_address)));
			SV_DropClient(cl, false, "Banned for move commands flooding (Avg)");
		}
	}
}

CStringCommandsRateLimiter::CStringCommandsRateLimiter() {
	Q_memset(m_AverageStringCmdRate, 0, sizeof(m_AverageStringCmdRate));
	Q_memset(m_CurrentStringCmds, 0, sizeof(m_CurrentStringCmds));
	m_LastCheckTime = 0.0;
}

void CStringCommandsRateLimiter::UpdateAverageRates(double dt) {
	for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
		m_AverageStringCmdRate[i] = (2.0 * m_AverageStringCmdRate[i] / 3.0) + m_CurrentStringCmds[i] / dt / 3.0;
		m_CurrentStringCmds[i] = 0;

		CheckAverageRate(i);
	}
}

void CStringCommandsRateLimiter::Frame() {
	double currentTime = realtime;
	double dt = currentTime - m_LastCheckTime;

	if (dt < 0.5) { //refresh avg. rate every 0.5 sec
		return;
	}

	UpdateAverageRates(dt);
	m_LastCheckTime = currentTime;
}

void CStringCommandsRateLimiter::ClientConnected(unsigned int clientId) {
	m_CurrentStringCmds[clientId] = 0;
	m_AverageStringCmdRate[clientId] = 0.0f;
}

void CStringCommandsRateLimiter::StringCommandIssued(unsigned int clientId) {
	m_CurrentStringCmds[clientId]++;
	CheckBurstRate(clientId);
}

void CStringCommandsRateLimiter::CheckBurstRate(unsigned int clientId) {
	client_t* cl = &g_psvs.clients[clientId];
	if (!cl->active || sv_rehlds_stringcmdrate_max_burst.value <= 0.0f) {
		return;
	}

	double dt = realtime - m_LastCheckTime;
	if (dt < 0.2) {
		dt = 0.2; //small intervals may give too high rates
	}
	if ((m_CurrentStringCmds[clientId] / dt) > sv_rehlds_stringcmdrate_max_burst.value) {
		if(sv_rehlds_stringcmdrate_burst_punish.value < 0) {
			Con_DPrintf("%s Kicked for string commands flooding (burst) (%.1f)\n", cl->name, (m_CurrentStringCmds[clientId] / dt));
			SV_DropClient(cl, false, "Kicked for string commands flooding (burst)");
		}
		else
		{
			Con_DPrintf("%s Banned for string commands flooding (burst) (%.1f)\n", cl->name, (m_CurrentStringCmds[clientId] / dt));
			Cbuf_AddText(va("addip %.1f %s\n", sv_rehlds_stringcmdrate_burst_punish.value, NET_BaseAdrToString(cl->netchan.remote_address)));
			SV_DropClient(cl, false, "Banned for string commands flooding (burst)");
		}
	}
}

void CStringCommandsRateLimiter::CheckAverageRate(unsigned int clientId) {
	client_t* cl = &g_psvs.clients[clientId];
	if (!cl->active || sv_rehlds_stringcmdrate_max_burst.value <= 0.0f) {
		return;
	}

	if (m_AverageStringCmdRate[clientId] > sv_rehlds_stringcmdrate_max_avg.value) {
		if(sv_rehlds_stringcmdrate_avg_punish.value < 0) {
			Con_DPrintf("%s Kicked for string commands flooding (Avg) (%.1f)\n", cl->name, m_AverageStringCmdRate[clientId]);
			SV_DropClient(cl, false, "Kicked for string commands flooding (Avg)");
		}
		else
		{
			Con_DPrintf("%s Banned for string commands flooding (Avg) (%.1f)\n", cl->name, m_AverageStringCmdRate[clientId]);
			Cbuf_AddText(va("addip %.1f %s\n", sv_rehlds_stringcmdrate_avg_punish.value, NET_BaseAdrToString(cl->netchan.remote_address)));
			SV_DropClient(cl, false, "Banned for string commands flooding (Avg)");
		}
	}
}

CUserCmdTimeLimiter::CUserCmdTimeLimiter()
{
	Q_memset(m_States, 0, sizeof(m_States));
}

void CUserCmdTimeLimiter::ClientConnected(unsigned int clientId)
{
	Q_memset(&m_States[clientId], 0, sizeof(m_States[clientId]));
}

double CUserCmdTimeLimiter::TimeDifference(uint64_t start, uint64_t end) const
{
	if (end > start)
	{
		return (end - start) / 1000.0;
	}
	else
	{
		return ((start - end) / 1000.0) * -1.0;
	}
}

#define MAX_EX_INTERP              0.1f
#define MIN_EX_INTERP              0.05f
#define MAX_EX_INTERP_SPECTATOR    0.2f

bool CUserCmdTimeLimiter::CheckLimits(unsigned int clientId, usercmd_t *ucmd)
{
	client_t *cl = &g_psvs.clients[clientId];
	if (!cl->active) {
		return false;
	}

	usercmd_state_t *ust = &m_States[clientId];

	// check move command flood within a single server tick
	if (sv_rehlds_movecmd_max_ticks.value > 0)
	{
		if (ust->ticksThisFrame > (unsigned int)sv_rehlds_movecmd_max_ticks.value) {
			return true;
		}

		ust->ticksThisFrame++;
	}

	// air-stuck (consecutive 0 msec commands)
	// legitimate >1000 FPS clients may occasionally send msec=0 due to byte truncation,
	// but only illegitimate clients send long unbroken streaks of zero time
	if (sv_rehlds_movecmd_max_null_streak.value > 0)
	{
		if (ucmd->msec == 0)
		{
			if (++ust->consecutiveNullCmds > (unsigned int)sv_rehlds_movecmd_max_null_streak.value) {
				return true; // streak exceeded, drop command
			}
		}
		else
		{
			ust->consecutiveNullCmds = 0; // reset streak on valid time
		}
	}

	// check lerp_msec bounds
	if (sv_rehlds_movecmd_clamp_interp.value > 0) {
		int maxexinterp = cl->proxy ? (MAX_EX_INTERP_SPECTATOR * 1000.0f) : (MAX_EX_INTERP * 1000.0f);
		if (ucmd->lerp_msec < 0 || ucmd->lerp_msec > maxexinterp) {
			return true;
		}
	}

	//
	// Time Drift / Speedhack Detection
	//

	if (sv_rehlds_movecmdtime_samples.value <= 0.0f) {
		return false;
	}

	uint64_t now = (uint64_t)(realtime * 1000.0);

	// Initialize states for newly active clients
	if (ust->msecTime == 0) ust->msecTime = now;
	if (ust->joinTime == 0) ust->joinTime = now;
	if (ust->lastUpdateTime == 0) ust->lastUpdateTime = now;

	ust->msecTime += ucmd->msec;

	if (ust->numFrames < (uint64_t)sv_rehlds_movecmdtime_samples.value) {
		ust->avgMsec += ucmd->msec;
		ust->avgServerTime += (now - ust->lastUpdateTime);
		ust->numFrames++;
	} else {
		// normalize averages over sample window
		ust->avgMsec /= (uint64_t)sv_rehlds_movecmdtime_samples.value;
		ust->avgServerTime /= (uint64_t)sv_rehlds_movecmdtime_samples.value;
		ust->numFrames = 0;
	}

	ust->lastUpdateTime = now;

	// calc temporal desync (error) and timescale ratio
	double error = TimeDifference(now, ust->msecTime) * 1000.0;
	double timescale_ratio = 0.0f;

	if (ust->avgMsec != 0.0f && ust->avgServerTime != 0.0f) {
		timescale_ratio = ust->avgMsec / ust->avgServerTime;
	}

	TimeAbuseType abuseType = ABUSE_NONE;

	// abnormal time acceleration (speedhack)
	float maxError = sv_rehlds_movecmdtime_max_error.value;
	if (error > maxError)
	{
		if (timescale_ratio > sv_rehlds_movecmdtime_max_scale.value) {
			abuseType = ABUSE_SPEEDHACK;
		}
	}
	// abnormal time deceleration (slow-mo) or overcharging
	else if (error < -maxError)
	{
		if (error < -(maxError * 2.0f)) {
			ust->msecTime = now;
		}

		if (timescale_ratio < sv_rehlds_movecmdtime_min_scale.value) {
			abuseType = ABUSE_SLOWMO;
		}
	}

	if (abuseType != ABUSE_NONE)
	{
		// revert accumulated time
		ust->msecTime -= ucmd->msec;

		if (sv_rehlds_movecmdtime_max_warnings.value >= 0.0f)
		{
			ust->warnings[(int)abuseType]++;

			// apply punish action if tolerance threshold is exceeded
			if (ust->warnings[(int)abuseType] > (unsigned int)sv_rehlds_movecmdtime_max_warnings.value)
			{
				const char *punishReason = (abuseType == ABUSE_SPEEDHACK) ? "speedhack" : "slowmo";

				if (sv_rehlds_movecmdtime_punish.value < 0.0f) {
					Con_DPrintf("%s Kicked for %s (%.1f)\n", cl->name, punishReason, timescale_ratio);
					SV_DropClient(cl, FALSE, va("Kicked for %s", punishReason));
				} else {
					if (sv_rehlds_movecmdtime_punish.value == 0.0f) {
						Con_DPrintf("%s Permanently banned for %s (%.1f)\n", cl->name, punishReason, timescale_ratio);
					} else {
						Con_DPrintf("%s Banned for %s (%.1f)\n", cl->name, punishReason, timescale_ratio);
					}

					Cbuf_AddText(va("addip %.1f %s\n", sv_rehlds_movecmdtime_punish.value, NET_BaseAdrToString(cl->netchan.remote_address)));
					SV_DropClient(cl, FALSE, va("Banned for %s", punishReason));
				}
			}
		}

		// stop command processing
		return true;
	}
	else
	{
		// recover tolerance if valid packets resume
		if (timescale_ratio <= sv_rehlds_movecmdtime_max_scale.value &&
			timescale_ratio >= sv_rehlds_movecmdtime_min_scale.value)
		{
			ust->accMsecStable += (double)ucmd->msec;

			float recoveryThreshold = sv_rehlds_movecmdtime_samples.value * 2.0f;
			if (ust->accMsecStable >= recoveryThreshold)
			{
				if (ust->warnings[ABUSE_SPEEDHACK] > 0) ust->warnings[ABUSE_SPEEDHACK]--;
				if (ust->warnings[ABUSE_SLOWMO] > 0)    ust->warnings[ABUSE_SLOWMO]--;

				ust->accMsecStable = 0.0;
			}
		}
		else
		{
			ust->accMsecStable = 0;
		}
	}

	return false;
}

void CUserCmdTimeLimiter::Frame()
{
	for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
		m_States[i].ticksThisFrame = 0;
	}
}

void Rehlds_Security_Init() {
#ifdef REHLDS_FIXES
	Cvar_RegisterVariable(&sv_rehlds_movecmdrate_max_avg);
	Cvar_RegisterVariable(&sv_rehlds_movecmdrate_max_burst);
	Cvar_RegisterVariable(&sv_rehlds_stringcmdrate_max_avg);
	Cvar_RegisterVariable(&sv_rehlds_stringcmdrate_max_burst);

	Cvar_RegisterVariable(&sv_rehlds_movecmdrate_avg_punish);
	Cvar_RegisterVariable(&sv_rehlds_movecmdrate_burst_punish);
	Cvar_RegisterVariable(&sv_rehlds_stringcmdrate_avg_punish);
	Cvar_RegisterVariable(&sv_rehlds_stringcmdrate_burst_punish);

	Cvar_RegisterVariable(&sv_rehlds_movecmd_max_ticks);
	Cvar_RegisterVariable(&sv_rehlds_movecmd_max_null_streak);
	Cvar_RegisterVariable(&sv_rehlds_movecmd_clamp_interp);
	Cvar_RegisterVariable(&sv_rehlds_movecmdtime_samples);
	Cvar_RegisterVariable(&sv_rehlds_movecmdtime_max_error);
	Cvar_RegisterVariable(&sv_rehlds_movecmdtime_max_scale);
	Cvar_RegisterVariable(&sv_rehlds_movecmdtime_min_scale);
	Cvar_RegisterVariable(&sv_rehlds_movecmdtime_punish);
	Cvar_RegisterVariable(&sv_rehlds_movecmdtime_max_warnings);
#endif
}

void Rehlds_Security_Shutdown() {
}

void Rehlds_Security_Frame() {
#ifdef REHLDS_FIXES
	g_MoveCommandRateLimiter.Frame();
	g_StringCommandsRateLimiter.Frame();
	g_UserCmdTimeLimiter.Frame();
#endif
}

void Rehlds_Security_ClientConnected(unsigned int clientId) {
#ifdef REHLDS_FIXES
	g_MoveCommandRateLimiter.ClientConnected(clientId);
	g_StringCommandsRateLimiter.ClientConnected(clientId);
	g_UserCmdTimeLimiter.ClientConnected(clientId);
#endif
}
