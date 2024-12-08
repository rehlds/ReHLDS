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

#include "maintypes.h"

// TODO: I think this defines must be in /common/
const int NUM_EDICTS = 1200;
const int MAX_NAME   = 32;

#include "custom_int.h"
#include "crc.h"
#include "cvar.h"
#include "dll_state.h"
#include "consistency.h"
#include "event.h"
#include "entity_state.h"
#include "delta.h"
#include "delta_packet.h"
#include "model.h"
#include "net.h"
#include "progs.h"
#include "filter.h"
#include "server_static.h"
#include "usermsg.h"
#include "userid.h"
#include "pm_defs.h"
#include "inst_baseline.h"
#include "net_ws.h"
#include "pm_shared/pm_movevars.h"

const int DEFAULT_SOUND_PACKET_VOLUME			= 255;
const float DEFAULT_SOUND_PACKET_ATTENUATION	= 1.0f;
const int DEFAULT_SOUND_PACKET_PITCH			= 100;

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

// Message send destination flags
enum
{
	MSG_FL_NONE			= 0,		// No flags
	MSG_FL_BROADCAST	= BIT(0),	// Broadcast?
	MSG_FL_PVS			= BIT(1),	// Send to PVS
	MSG_FL_PAS			= BIT(2),	// Send to PAS
	MSG_FL_ONE			= BIT(7),	// Send to single client
};

const int RESOURCE_INDEX_BITS = 12;

#ifdef REHLDS_FIXES
const int RESOURCE_MAX_COUNT = BIT(RESOURCE_INDEX_BITS);
#endif // REHLDS_FIXES

typedef enum redirect_e
{
	RD_NONE = 0,
	RD_CLIENT = 1,
	RD_PACKET = 2,
} redirect_t;

typedef struct server_s
{
	qboolean active;
	qboolean paused;
	qboolean loadgame;
	double time;
	double oldtime;
	int lastcheck;
	double lastchecktime;
	char name[64];
	char oldname[64];
	char startspot[64];
	char modelname[64];
	struct model_s *worldmodel;
	CRC32_t worldmapCRC;
	unsigned char clientdllmd5[16];
	resource_t resourcelist[MAX_RESOURCE_LIST];
	int num_resources;
	consistency_t consistency_list[MAX_CONSISTENCY_LIST];
	int num_consistency;
	const char *model_precache[MAX_MODELS];
	struct model_s *models[MAX_MODELS];
	unsigned char model_precache_flags[MAX_MODELS];
	struct event_s event_precache[MAX_EVENTS];
	const char *sound_precache[MAX_SOUNDS];
	short int sound_precache_hashedlookup[MAX_SOUNDS_HASHLOOKUP_SIZE];
	qboolean sound_precache_hashedlookup_built;
	const char *generic_precache[MAX_GENERIC];
	char generic_precache_names[MAX_GENERIC][64];
	int num_generic_names;
	const char *lightstyles[MAX_LIGHTSTYLES];
	int num_edicts;
	int max_edicts;
	edict_t *edicts;
	struct entity_state_s *baselines;
	extra_baselines_t *instance_baselines;
	server_state_t state;
	sizebuf_t datagram;
	unsigned char datagram_buf[MAX_DATAGRAM];
	sizebuf_t reliable_datagram;
	unsigned char reliable_datagram_buf[MAX_DATAGRAM];
	sizebuf_t multicast;
	unsigned char multicast_buf[1024];
	sizebuf_t spectator;
	unsigned char spectator_buf[1024];
	sizebuf_t signon;
	unsigned char signon_data[32768];
} server_t;


struct rehlds_server_t {
	// map for sv.model_precache (for faster resolving of model index by its name)
#if defined(REHLDS_FIXES)
	CStringKeyStaticMap<int, 7, MAX_MODELS * 2> modelsMap; // case-sensitive keys for better performance
#elif defined(REHLDS_OPT_PEDANTIC)
	CICaseStringKeyStaticMap<int, 7, MAX_MODELS * 2> modelsMap; // use case-insensitive keys to conform original engine's behavior
#endif

#ifdef REHLDS_FIXES
	// Extended net buffers
	uint8_t reliableDatagramBuffer[NET_MAX_PAYLOAD];
	uint8_t signonData[NET_MAX_PAYLOAD];
	uint8_t spectatorBuffer[MAX_DATAGRAM];

	// Extended resource list
	resource_t resources[RESOURCE_MAX_COUNT];
	char precachedGenericResourceNames[RESOURCE_MAX_COUNT][MAX_QPATH];
	size_t precachedGenericResourceCount;

	char lightstyleBuffers[MAX_LIGHTSTYLES][MAX_LIGHTSTYLE_SIZE];
#endif
};

typedef struct client_frame_s
{
	double senttime;
	float ping_time;
	clientdata_t clientdata;
	weapon_data_t weapondata[64];
	packet_entities_t entities;
} client_frame_t;

typedef struct client_s
{
	qboolean active;
	qboolean spawned;
	qboolean fully_connected;
	qboolean connected;
	qboolean uploading;
	qboolean hasusrmsgs;
	qboolean has_force_unmodified;
	netchan_t netchan;
	int chokecount;
	int delta_sequence;
	qboolean fakeclient;
	qboolean proxy;
	usercmd_t lastcmd;
	double connecttime;
	double cmdtime;
	double ignorecmdtime;
	float latency;
	float packet_loss;
	double localtime;
	double nextping;
	double svtimebase;
	sizebuf_t datagram;
	byte datagram_buf[MAX_DATAGRAM];
	double connection_started;
	double next_messagetime;
	double next_messageinterval;
	qboolean send_message;
	qboolean skip_message;
	client_frame_t *frames;
	event_state_t events;
	edict_t *edict;
	const edict_t *pViewEntity;
	int userid;
	USERID_t network_userid;
	char userinfo[MAX_INFO_STRING];
	qboolean sendinfo;
	float sendinfo_time;
	char hashedcdkey[64];
	char name[32];
	int topcolor;
	int bottomcolor;
	int entityId;
	resource_t resourcesonhand;
	resource_t resourcesneeded;
	FileHandle_t upload;
	qboolean uploaddoneregistering;
	customization_t customdata;
	int crcValue;
	int lw;
	int lc;
	char physinfo[MAX_INFO_STRING];
	qboolean m_bLoopback;
	uint32 m_VoiceStreams[2];
	double m_lastvoicetime;
	int m_sendrescount;
	qboolean m_bSentNewResponse;
	movevars_t movevars;
} client_t;

enum
{
	SND_ANYPLAYER,
	SND_NOTHOST,
};

typedef struct rcon_failure_s rcon_failure_t;
typedef struct challenge_s challenge_t;

typedef struct deltacallback_s
{
	int *numbase;
	int num;
	qboolean remove;
	qboolean custom;
	qboolean newbl;
	int newblindex;
	qboolean full;
	int offset;
} deltacallback_t;

extern char *pr_strings;
extern char *gNullString;
extern qboolean scr_skipupdate;
extern float scr_centertime_off;
extern float g_LastScreenUpdateTime;

extern int SV_UPDATE_BACKUP;
extern int SV_UPDATE_MASK;

extern globalvars_t gGlobalVariables;
extern server_static_t g_psvs;
extern server_t g_psv;

extern rehlds_server_t g_rehlds_sv;

extern cvar_t sv_lan;
extern cvar_t sv_lan_rate;
extern cvar_t sv_aim;
extern cvar_t sv_allow_autoaim;

extern cvar_t sv_skycolor_r;
extern cvar_t sv_skycolor_g;
extern cvar_t sv_skycolor_b;
extern cvar_t sv_skyvec_x;
extern cvar_t sv_skyvec_y;
extern cvar_t sv_skyvec_z;
extern cvar_t sv_skyname;

extern cvar_t sv_spectatormaxspeed;
extern cvar_t sv_airaccelerate;
extern cvar_t sv_wateraccelerate;
extern cvar_t sv_waterfriction;
extern cvar_t sv_zmax;
extern cvar_t sv_wateramp;
extern cvar_t mapcyclefile;
extern cvar_t motdfile;
extern cvar_t servercfgfile;
extern cvar_t lservercfgfile;
extern cvar_t logsdir;
extern cvar_t bannedcfgfile;
#ifdef REHLDS_FIXES
extern cvar_t listipcfgfile;
extern cvar_t syserror_logfile;
#endif

extern decalname_t sv_decalnames[MAX_BASE_DECALS];
extern int sv_decalnamecount;

extern UserMsg *sv_gpNewUserMsgs;
extern UserMsg *sv_gpUserMsgs;
extern playermove_t g_svmove;

extern int sv_lastnum;
extern extra_baselines_t g_sv_instance_baselines;
extern qboolean g_bOutOfDateRestart;
extern int g_userid;

extern delta_info_t *g_sv_delta;
extern delta_t *g_peventdelta;

extern cvar_t rcon_password;
extern cvar_t sv_enableoldqueries;
extern cvar_t sv_instancedbaseline;
extern cvar_t sv_contact;
extern cvar_t sv_maxupdaterate;
extern cvar_t sv_minupdaterate;
extern cvar_t sv_filterban;
extern cvar_t sv_minrate;
extern cvar_t sv_maxrate;
extern cvar_t sv_logrelay;
extern cvar_t violence_hblood;
extern cvar_t violence_ablood;
extern cvar_t violence_hgibs;
extern cvar_t violence_agibs;
extern cvar_t sv_newunit;
extern cvar_t sv_clienttrace;
extern cvar_t sv_timeout;
extern cvar_t sv_failuretime;

extern cvar_t sv_cheats;
extern cvar_t sv_password;
extern cvar_t sv_proxies;
extern cvar_t sv_outofdatetime;
extern cvar_t mapchangecfgfile;

extern cvar_t mp_logecho;
extern cvar_t mp_logfile;
extern cvar_t sv_allow_download;
extern cvar_t sv_send_logos;
extern cvar_t sv_send_resources;
extern cvar_t sv_log_singleplayer;
extern cvar_t sv_logsecret;
extern cvar_t sv_log_onefile;
extern cvar_t sv_logbans;
extern cvar_t sv_allow_upload;
extern cvar_t sv_max_upload;
extern cvar_t hpk_maxsize;
extern cvar_t sv_visiblemaxplayers;
extern cvar_t sv_downloadurl;
extern cvar_t sv_allow_dlfile;
extern cvar_t sv_version;
extern cvar_t sv_tags;
#ifdef REHLDS_FIXES
extern cvar_t sv_echo_unknown_cmd;
extern cvar_t sv_auto_precache_sounds_in_models;
extern cvar_t sv_delayed_spray_upload;
extern cvar_t sv_rehlds_force_dlmax;
extern cvar_t sv_rehlds_hull_centering;
extern cvar_t sv_rcon_condebug;
extern cvar_t sv_rehlds_userinfo_transmitted_fields;
extern cvar_t sv_rehlds_attachedentities_playeranimationspeed_fix;
extern cvar_t sv_rehlds_local_gametime;
extern cvar_t sv_rehlds_send_mapcycle;
extern cvar_t sv_usercmd_custom_random_seed;
#endif
extern int sv_playermodel;

extern char outputbuf[MAX_ROUTEABLE_PACKET];
extern redirect_t sv_redirected;
extern netadr_t sv_redirectto;

extern cvar_t sv_rcon_minfailures;
extern cvar_t sv_rcon_maxfailures;
extern cvar_t sv_rcon_minfailuretime;
extern cvar_t sv_rcon_banpenalty;

extern cvar_t scr_downloading;

enum GameType_e
{
	GT_Unitialized,
	GT_CZero,
	GT_CZeroRitual,
	GT_TerrorStrike,
	GT_TFC,
	GT_HL1,
	GT_CStrike
};

extern GameType_e g_eGameType;

extern int giNextUserMsg;
extern int hashstrings_collisions;

extern delta_t *g_pplayerdelta;
extern delta_t *g_pentitydelta;
extern delta_t *g_pcustomentitydelta;
extern delta_t *g_pclientdelta;
extern delta_t *g_pweapondelta;
#ifdef REHLDS_OPT_PEDANTIC
extern delta_t *g_pusercmddelta;
#endif

extern int gPacketSuppressed;

extern char localinfo[MAX_LOCALINFO];
extern char localmodels[MAX_MODELS][5];

extern ipfilter_t ipfilters[MAX_IPFILTERS];
extern int numipfilters;
extern userfilter_t userfilters[MAX_USERFILTERS];
extern int numuserfilters;

extern challenge_t g_rg_sv_challenges[MAX_CHALLENGES];

extern rcon_failure_t g_rgRconFailures[32];
extern deltacallback_t g_svdeltacallback;

delta_t *SV_LookupDelta(char *name);
NOXREF void SV_DownloadingModules(void);
void SV_GatherStatistics(void);
void SV_DeallocateDynamicData(void);
void SV_ReallocateDynamicData(void);
void SV_AllocClientFrames(void);
qboolean SV_IsPlayerIndex(int index);
qboolean SV_IsPlayerIndex_wrapped(int index);
void SV_ClearPacketEntities(client_frame_t *frame);
void SV_AllocPacketEntities(client_frame_t *frame, int numents);
void SV_ClearFrames(client_frame_t ** frames);
void SV_Serverinfo_f(void);
void SV_Localinfo_f(void);
void SV_User_f(void);
void SV_Users_f(void);
void SV_CountPlayers(int *clients);
void SV_CountProxies(int *proxies);
void SV_FindModelNumbers(void);
void SV_StartParticle(const vec_t *org, const vec_t *dir, int color, int count);
void SV_StartSound(int recipients, edict_t *entity, int channel, const char *sample, int volume, float attenuation, int fFlags, int pitch);
void SV_StartSound_internal(int recipients, edict_t *entity, int channel, const char *sample, int volume, float attenuation, int fFlags, int pitch);
qboolean SV_BuildSoundMsg(edict_t *entity, int channel, const char *sample, int volume, float attenuation, int fFlags, int pitch, const float *origin, sizebuf_t *buffer);
int SV_HashString(const char *string, int iBounds);
int SV_LookupSoundIndex(const char *sample);
void SV_BuildHashedSoundLookupTable(void);
void SV_AddSampleToHashedLookupTable(const char *pszSample, int iSampleIndex);
qboolean SV_ValidClientMulticast(client_t *client, int soundLeaf, int to);
void SV_Multicast(edict_t *ent, vec_t *origin, int to, qboolean reliable);
void SV_WriteMovevarsToClient(sizebuf_t *message, struct movevars_s *movevars);
void SV_WriteDeltaDescriptionsToClient(sizebuf_t *msg);
void SV_SetMoveVars(struct movevars_s *movevars);
void SV_SendServerinfo(sizebuf_t *msg, client_t *client);
void SV_SendServerinfo_internal(sizebuf_t *msg, client_t *client);
void SV_SendResources(sizebuf_t *msg);
void SV_SendResources_internal(sizebuf_t *msg);
void SV_WriteClientdataToMessage(client_t *client, sizebuf_t *msg);
void SV_WriteSpawn(sizebuf_t *msg);
void SV_SendUserReg(sizebuf_t *msg);
void SV_New_f(void);
void SV_SendRes_f(void);
void SV_Spawn_f(void);
void SV_Spawn_f_internal(void);
void SV_CheckUpdateRate(double *rate);
void SV_RejectConnection(netadr_t *adr, char *fmt, ...);
void SV_RejectConnectionForPassword(netadr_t *adr);
int SV_GetFragmentSize(void *state);
qboolean SV_FilterUser(USERID_t *userid);
int SV_CheckProtocol(netadr_t *adr, int nProtocol);
int SV_CheckProtocol_internal(netadr_t *adr, int nProtocol);
bool SV_CheckChallenge_api(const netadr_t &adr, int nChallengeValue);
int SV_CheckChallenge(netadr_t *adr, int nChallengeValue);
int SV_CheckIPRestrictions(netadr_t *adr, int nAuthProtocol);
int SV_CheckIPRestrictions_internal(netadr_t *adr, int nAuthProtocol);
int SV_CheckIPConnectionReuse(netadr_t *adr);
int SV_FinishCertificateCheck(netadr_t *adr, int nAuthProtocol, char *szRawCertificate, char *userinfo);
int SV_FinishCertificateCheck_internal(netadr_t *adr, int nAuthProtocol, char *szRawCertificate, char *userinfo);
int SV_CheckKeyInfo(netadr_t *adr, char *protinfo, unsigned short *port, int *pAuthProtocol, char *pszRaw, char *cdkey);
int SV_CheckKeyInfo_internal(netadr_t *adr, char *protinfo, unsigned short *port, int *pAuthProtocol, char *pszRaw, char *cdkey);
int SV_CheckForDuplicateSteamID(client_t *client);
qboolean SV_CheckForDuplicateNames(char *userinfo, qboolean bIsReconnecting, int nExcludeSlot);
int SV_CheckUserInfo(netadr_t *adr, char *userinfo, qboolean bIsReconnecting, int nReconnectSlot, char *name);
int SV_CheckUserInfo_internal(netadr_t *adr, char *userinfo, qboolean bIsReconnecting, int nReconnectSlot, char *name);
int SV_FindEmptySlot(netadr_t *adr, int *pslot, client_t ** ppClient);
void SV_ConnectClient(void);
void SV_ConnectClient_internal(void);
void SVC_Ping(void);
int SV_GetChallenge(const netadr_t& adr);
void SVC_GetChallenge(void);
void SVC_ServiceChallenge(void);
void SV_ResetModInfo(void);
int SV_GetFakeClientCount(void);
NOXREF qboolean SV_GetModInfo(char *pszInfo, char *pszDL, int *version, int *size, qboolean *svonly, qboolean *cldll, char *pszHLVersion);
NOXREF qboolean RequireValidChallenge(netadr_t *adr);
NOXREF qboolean ValidInfoChallenge(netadr_t *adr, const char *nugget);
NOXREF int GetChallengeNr(netadr_t *adr);
NOXREF qboolean CheckChallengeNr(netadr_t *adr, int nChallengeValue);
NOXREF void ReplyServerChallenge(netadr_t *adr);
NOXREF qboolean ValidChallenge(netadr_t *adr, int challengeNr);
NOXREF void SVC_InfoString(void);
NOXREF void SVC_Info(qboolean bDetailed);
NOXREF void SVC_PlayerInfo(void);
NOXREF void SVC_RuleInfo(void);
int SVC_GameDllQuery(const char *s);
void SV_FlushRedirect(void);
void SV_EndRedirect(void);
void SV_BeginRedirect(redirect_t rd, netadr_t *addr);
void SV_ResetRcon_f(void);
void SV_AddFailedRcon(netadr_t *adr);
qboolean SV_CheckRconFailure(netadr_t *adr);
int SV_Rcon_Validate(void);
void SV_Rcon(netadr_t *net_from_);
void SV_ConnectionlessPacket(void);
void SV_CheckRate(client_t *cl);
void SV_ProcessFile(client_t *cl, char *filename);
qboolean SV_FilterPacket(void);
void SV_SendBan(void);
void SV_ReadPackets(void);
void SV_CheckTimeouts(void);
int SV_CalcPing(client_t *cl);
void SV_SendFullClientUpdateForAll(client_t *client);
void SV_FullClientUpdate(client_t *cl, sizebuf_t *sb);
void SV_EmitEvents(client_t *cl, packet_entities_t *pack, sizebuf_t *msg);
void SV_EmitEvents_internal(client_t *cl, packet_entities_t *pack, sizebuf_t *msg);
void SV_AddToFatPVS(vec_t *org, mnode_t *node);
unsigned char *SV_FatPVS(float *org);
void SV_AddToFatPAS(vec_t *org, mnode_t *node);
unsigned char *SV_FatPAS(float *org);
int SV_PointLeafnum(vec_t *p);
void TRACE_DELTA(char *fmt, ...);
void SV_SetCallback(int num, qboolean remove, qboolean custom, int *numbase, qboolean full, int offset);
void SV_SetNewInfo(int newblindex);
void SV_WriteDeltaHeader(int num, qboolean remove, qboolean custom, int *numbase, qboolean newbl, int newblindex, qboolean full, int offset);
void SV_InvokeCallback(void);
int SV_FindBestBaseline(int index, entity_state_t ** baseline, entity_state_t *to, int num, qboolean custom);
int SV_CreatePacketEntities(sv_delta_t type, client_t *client, packet_entities_t *to, sizebuf_t *msg);
int SV_CreatePacketEntities_internal(sv_delta_t type, client_t *client, packet_entities_t *to, sizebuf_t *msg);
void SV_EmitPacketEntities(client_t *client, packet_entities_t *to, sizebuf_t *msg);
qboolean SV_ShouldUpdatePing(client_t *client);
NOXREF qboolean SV_HasEventsInQueue(client_t *client);
void SV_GetNetInfo(client_t *client, int *ping, int *packet_loss);
int SV_CheckVisibility(edict_t *entity, unsigned char *pset);
void SV_EmitPings(client_t *client, sizebuf_t *msg);
void SV_EmitPings_internal(client_t *client, sizebuf_t *msg);
void SV_WriteEntitiesToClient(client_t *client, sizebuf_t *msg);
void SV_CleanupEnts(void);
qboolean SV_SendClientDatagram(client_t *client);
void SV_UpdateUserInfo(client_t *client);
void SV_UpdateToReliableMessages(void);
void SV_SkipUpdates(void);
void SV_SendClientMessages(void);
void SV_ExtractFromUserinfo(client_t *cl);
int SV_ModelIndex(const char *name);
void SV_AddResource(resourcetype_t type, const char *name, int size, unsigned char flags, int index);
void SV_AddResource_internal(resourcetype_t type, const char *name, int size, unsigned char flags, int index);
size_t SV_CountResourceByType(resourcetype_t type, resource_t **pResourceList = nullptr, size_t nListMax = 0, size_t *nWidthFileNameMax = nullptr);
void SV_CreateGenericResources(void);
void SV_CreateResourceList(void);
void SV_ClearCaches(void);
void SV_PropagateCustomizations(void);
void SV_WriteVoiceCodec(sizebuf_t *pBuf);
void SV_WriteVoiceCodec_internal(sizebuf_t *pBuf);
void SV_CreateBaseline(void);
void SV_BroadcastCommand(char *fmt, ...);
void SV_BuildReconnect(sizebuf_t *msg);
NOXREF void SV_ReconnectAllClients(void);
void SetCStrikeFlags(void);
void SV_ActivateServer(int runPhysics);
void SV_ActivateServer_internal(int runPhysics);
void SV_ServerShutdown(void);
int SV_SpawnServer(qboolean bIsDemo, char *server, char *startspot);
void SV_LoadEntities(void);
void SV_ClearEntities(void);
int RegUserMsg(const char *pszName, int iSize);
qboolean StringToFilter(const char *s, ipfilter_t *f);
USERID_t *SV_StringToUserID(const char *str);
bool CanBeWrittenWithoutCIDR(const ipfilter_t &f);
void FilterToString(const ipfilter_t &f, char *s);
bool IsFilterIncludesAnotherFilter(const ipfilter_t &f, const ipfilter_t &f2);
void SV_BanId_f(void);
void Host_Kick_f(void);
void SV_RemoveId_f(void);
void SV_WriteId_f(void);
void SV_ListId_f(void);
void SV_AddIP_f(void);
void SV_RemoveIP_f(void);
void SV_ListIP_f(void);
void SV_WriteIP_f(void);
void SV_KickPlayer(int nPlayerSlot, int nReason);
void SV_InactivateClients(void);
void SV_FailDownload(const char *filename);
const char *Q_stristr(const char *pStr, const char *pSearch);
qboolean IsSafeFileToDownload(const char *filename);
void SV_BeginFileDownload_f(void);
void SV_SetMaxclients(void);
void SV_HandleRconPacket(void);
void SV_CheckCmdTimes(void);
void SV_CheckForRcon(void);
qboolean SV_IsSimulating(void);
void SV_CheckMapDifferences(void);
void SV_Frame();
void SV_Frame_Internal();
void SV_Drop_f(void);
void SV_RegisterDelta(char *name, char *loadfile);
void SV_InitDeltas(void);
void SV_InitEncoders(void);
void SV_Init(void);
void SV_Shutdown(void);
qboolean SV_CompareUserID(USERID_t *id1, USERID_t *id2);
qboolean SV_CompareUserID_internal(USERID_t *id1, USERID_t *id2);
char *SV_GetIDString(USERID_t *id);
char *SV_GetIDString_internal(USERID_t *id);
char *SV_GetClientIDString(client_t *client);
int GetGameAppID(void);
qboolean IsGameSubscribed(const char *gameName);
NOXREF qboolean BIsValveGame(void);
