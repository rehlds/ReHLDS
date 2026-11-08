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

#include <HLTV/IServer.h>
#include "BaseSystemModule.h"
#include "TokenLine.h"

class IWorld;
class IProxy;
class IDirector;
class IFileSystem;
class NetPacket;
class INetSocket;

#ifdef HLTV_FIXES
const int MAX_SERVER_RATE       = 100000;
#else
const int MAX_SERVER_RATE       = 20000;
#endif

const int MIN_SERVER_RATE       = 1000;

const int MAX_SERVER_UPDATERATE = 100;
const int MIN_SERVER_UPDATERATE = 1;

#define TE_MAX					128

// Sound flags
enum
{
	SND_FL_VOLUME			= BIT(0),	// send volume
	SND_FL_ATTENUATION		= BIT(1),	// send attenuation
	SND_FL_LARGE_INDEX		= BIT(2),	// send sound number as short instead of byte
	SND_FL_PITCH			= BIT(3),	// send pitch
	SND_FL_SENTENCE			= BIT(4),	// set if sound num is actually a sentence num
	SND_FL_STOP				= BIT(5),	// stop the sound
	SND_FL_CHANGE_VOL		= BIT(6),	// change sound vol
	SND_FL_CHANGE_PITCH		= BIT(7),	// change sound pitch
	SND_FL_SPAWNING			= BIT(8)	// we're spawning, used in some cases for ambients (not sent across network)
};

class Server: public IServer, public BaseSystemModule {
public:
	Server() {}
	virtual ~Server() {}

	EXT_FUNC bool Init(IBaseSystem *system, int serial, char *name);
	EXT_FUNC void RunFrame(double time);
	EXT_FUNC void ReceiveSignal(ISystemModule *module, unsigned int signal, void *data);
	EXT_FUNC void ExecuteCommand(int commandID, char *commandLine);
	EXT_FUNC void RegisterListener(ISystemModule *module);
	EXT_FUNC void RemoveListener(ISystemModule *module);
	EXT_FUNC IBaseSystem *GetSystem();
	EXT_FUNC int GetSerial();
	EXT_FUNC char *GetStatusLine();
	EXT_FUNC char *GetType();
	EXT_FUNC char *GetName();
	EXT_FUNC int GetState();
	EXT_FUNC int GetVersion();
	EXT_FUNC void ShutDown();

	EXT_FUNC bool Connect(IWorld *world, NetAddress *address, INetSocket *socket);
	EXT_FUNC bool LoadDemo(IWorld *world, char *filename, bool forceHLTV, bool continuous);
	EXT_FUNC void Reconnect();
	EXT_FUNC void Disconnect();
	EXT_FUNC void Retry();
	EXT_FUNC void StopRetry();
	EXT_FUNC void SendStringCommand(char *command);
	EXT_FUNC void SendHLTVCommand(BitBuffer *msg);
	EXT_FUNC void SetPlayerName(char *newName);
	EXT_FUNC void SetProxy(IProxy *proxy);
	EXT_FUNC void SetDirector(IDirector *director);
	EXT_FUNC void SetDelayReconnect(bool state);
	EXT_FUNC void SetAutoRetry(bool state);
	EXT_FUNC void SetVoiceBlocking(bool state);
	EXT_FUNC void SetRate(int rate);
	EXT_FUNC void SetUpdateRate(int updaterate);
	EXT_FUNC void SetUserInfo(char *key, char *value);
	EXT_FUNC bool SetProtocol(int version);
	EXT_FUNC void SetGameDirectory(const char *defaultDir, const char *gameDir);
	EXT_FUNC bool IsConnected();
	EXT_FUNC bool IsDemoFile();
	EXT_FUNC bool IsGameServer();
	EXT_FUNC bool IsRelayProxy();
	EXT_FUNC bool IsVoiceBlocking();
	EXT_FUNC int GetRate();
	EXT_FUNC int GetUpdateRate();
	EXT_FUNC char *GetPlayerName();
	EXT_FUNC InfoString *GetServerInfoString();

	EXT_FUNC float GetTime();
	EXT_FUNC IWorld *GetWorld();
	EXT_FUNC char *GetDemoFileName();
	EXT_FUNC NetAddress *GetAddress();
	EXT_FUNC bool GetAutoRetry();
	EXT_FUNC char *GetHostName();
	EXT_FUNC float GetPacketLoss();
	EXT_FUNC int GetProtocol();

private:
public:
	void CheckAutoRetry();
	void CheckConnection();
	void ScheduleAutoRetry();
	void AcceptConnection();
	void AcceptBadPassword();
	void AcceptRejection(char *reason);
	void AcceptRedirect(char *toAddress);
	void SendConnectPacket();
	void SendServerCommands();

	enum ServerState {
		SERVER_UNDEFINED,
		SERVER_INITIALIZING,
		SERVER_DISCONNECTED,
		SERVER_CHALLENGING,
		SERVER_AUTHENTICATING,
		SERVER_CONNECTING,
		SERVER_CONNECTED,
		SERVER_RUNNING,
		SERVER_INTERMISSION,
	};

	void SetState(ServerState newState);
	void Challenge();
	void Reset();
	void AcceptChallenge(char *cmdLine);
	void SendUserVar(char *key, char *value);
	char *GetCmdName(int cmd);
	void SetName(char *newName);
	void ProcessMessage(unsigned int seqNr);
	void ProcessEntityUpdate();
	bool ProcessConnectionlessMessage(NetAddress *from, BitBuffer *stream);
	void ClearFrame(bool completely);
	void ParseHLTV();
	void ParseDirector();
	void ParseFileTransferFailed();
	void ParseInfo(BitBuffer *stream, bool detailed);
	void ParseParticle();
	void ParseRoomType();
	void ParseSpawnStaticSound();
	void ParseEventReliable();
	void ParsePings();
	void ParseStopSound();
	void ParseEvent();
	void ParseSound();
	void ParseDeltaPacketEntities();
	void ParsePacketEntities();
	bool ParseUserMessage(int cmd);
	void ParseCustomization();
	void ParseCrosshairAngle();
	void ParseSoundFade();
	void ParseSignonNum();
	void ParseDisconnect();
	void ParseChoke();
	void ParseSetAngle();
	void ParseAddAngle();
	void ParseLightStyle();
	void ParseTime();
	void ParseVersion();
	void ParseBaseline();
	void ParseTempEntity();
	void ParseResourceList();
	void ParseUpdateUserInfo();
	void ParseStuffText();
	void ParseNewUserMsg();
	void ParseResourceRequest();
	void ParseSetView();
	void ParseCDTrack();
	void ParseRestore();
	void ParseMoveVars();
	void ParseDeltaDescription();
	void ParseServerinfo();
	void ParseBad();
	void ParseNop();
	void ParsePrint();
	void ParseVoiceInit();
	void ParseVoiceData();
	void ParseTimeScale();
	void ParseSendExtraInfo();
	void ParseCenterPrint();
	void ParseSetPause();
	void ParseCutscene();
	void ParseWeaponAnim();
	void ParseDecalName();
	void ParseFinale();
	void ParseIntermission();
	void ParseClientData();
	void ParseResourceLocation();
	void ParseSendCvarValue();
	void ParseSendCvarValue2();
	void ParseExec();

protected:
	struct svc_func_s {
		svc_commands_e opcode;
		char *pszname;
		void (Server::*func)();
	};
	static svc_func_s m_ClientFuncs[];

	IWorld *m_World;
	IProxy *m_Proxy;
	IDirector *m_Director;
	IFileSystem *m_FileSystem;
	int m_ChallengeNumber;

	INetSocket *m_ServerSocket;
	NetChannel m_ServerChannel;
	NetAddress m_ServerAddress;

	int m_ServerState;
	char m_HostName[MAX_PATH];
	int m_CurrentRetry;
	InfoString m_UserInfo;
	char m_CDKey[32];
	int m_AuthProtocol;
	frame_t m_Frame;
	unsigned char m_EntityBuffer[MAX_PACKET_ENTITIES * sizeof(entity_state_t)];
	int m_Rate;
	int m_UpdateRate;
	float m_Time;
	BitBuffer m_ReliableData;
	BitBuffer m_UnreliableData;
	BitBuffer m_VoiceData;
	BitBuffer m_UserMessages;
	BitBuffer m_ClientData;
	BitBuffer *m_Instream;
	int m_validSequence;
	bool m_AutoRetry;
	double m_NextAutoRetry;
	bool m_IsHLTV;
	bool m_ForceHLTV;
	bool m_IsGameServer;
	bool m_IsPaused;
	bool m_IsVoiceBlocking;
	DemoFile m_DemoFile;
	demo_info_t m_DemoInfo;
	BitBuffer m_DemoData;
	CRC32_t m_ServerCRC;
	int m_ServerCount;
	int m_ClientPlayerNum;
	int m_Protocol;
	clientdata_t m_ClientDataStruct;
	InfoString m_ServerInfo;
	bool m_DelayReconnect;
	unsigned int m_SeqNrMap[256];

#ifdef HLTV_FIXES
	bool m_PendingIntermission;
#endif
};
