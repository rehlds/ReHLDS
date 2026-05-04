#pragma once

#include "engine.h"

class CMoveCommandRateLimiter {
public:
	CMoveCommandRateLimiter();
	void Frame();
	void MoveCommandsIssued(unsigned int clientId, unsigned int numCmds);
	void ClientConnected(unsigned int clientId);

private:
	void UpdateAverageRates(double currentTime);
	void CheckBurstRate(unsigned int clientId);
	void CheckAverageRate(unsigned int clientId);

private:
	float m_AverageMoveCmdRate[MAX_CLIENTS];
	int m_CurrentMoveCmds[MAX_CLIENTS];
	double m_LastCheckTime;
};

extern CMoveCommandRateLimiter g_MoveCommandRateLimiter;

class CStringCommandsRateLimiter {
public:
	CStringCommandsRateLimiter();
	void Frame();
	void StringCommandIssued(unsigned int clientId);
	void ClientConnected(unsigned int clientId);

private:
	void UpdateAverageRates(double currentTime);
	void CheckBurstRate(unsigned int clientId);
	void CheckAverageRate(unsigned int clientId);

private:
	float m_AverageStringCmdRate[MAX_CLIENTS];
	int m_CurrentStringCmds[MAX_CLIENTS];
	double m_LastCheckTime;
};

extern CStringCommandsRateLimiter g_StringCommandsRateLimiter;

class CUserCmdTimeLimiter {
public:
	CUserCmdTimeLimiter();
	void Frame();
	bool CheckLimits(unsigned int clientId, usercmd_t *ucmd);
	void ClientConnected(unsigned int clientId);

private:

	double TimeDifference(uint64_t start, uint64_t end) const;

	enum TimeAbuseType {
		ABUSE_NONE = -1,
		ABUSE_SPEEDHACK,
		ABUSE_SLOWMO,
		ABUSE_MAX
	};

	struct usercmd_state_t {
		// limits state
		unsigned int ticksThisFrame;
		unsigned int consecutiveNullCmds;

		// time drift state
		uint64_t msecTime;
		uint64_t joinTime;
		uint64_t lastUpdateTime;
		double avgMsec;
		double avgServerTime;
		double accMsecStable;
		uint64_t numFrames;
		unsigned int lastTickTime;
		unsigned int warnings[ABUSE_MAX];
	};

	usercmd_state_t m_States[MAX_CLIENTS];
};

extern CUserCmdTimeLimiter g_UserCmdTimeLimiter;

extern void Rehlds_Security_Init();
extern void Rehlds_Security_Shutdown();
extern void Rehlds_Security_Frame();
extern void Rehlds_Security_ClientConnected(unsigned int clientId);
