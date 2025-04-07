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

CGameClient** g_GameClients;

CGameClient::CGameClient(int id, client_t* cl)
	: m_NetChan(&cl->netchan)
#ifdef REHLDS_FIXES
	, m_localGameTimeBase(0)
#endif
{
	m_Id = id;
	m_pClient = cl;
	m_bSpawnedOnce = false;
}

int EXT_FUNC CGameClient::GetId()
{
	return m_Id;
}

struct usercmd_s* EXT_FUNC CGameClient::GetLastCmd()
{
	return &m_pClient->lastcmd;
}

bool EXT_FUNC CGameClient::IsActive()
{
	return m_pClient->active != FALSE;
}

void EXT_FUNC CGameClient::SetActive(bool active)
{
	m_pClient->active = active ? TRUE : FALSE;
}

bool EXT_FUNC CGameClient::IsSpawned()
{
	return m_pClient->spawned != FALSE;
}

void EXT_FUNC CGameClient::SetSpawned(bool spawned)
{
	m_pClient->spawned = spawned ? TRUE : FALSE;
}

bool EXT_FUNC CGameClient::IsProxy()
{
	return m_pClient->proxy != FALSE;
}

void EXT_FUNC CGameClient::SetProxy(bool proxy)
{
	m_pClient->proxy = proxy ? TRUE : FALSE;
}

bool EXT_FUNC CGameClient::IsConnected() {
	return m_pClient->connected != FALSE;
}

void EXT_FUNC CGameClient::SetConnected(bool connected) {
	m_pClient->connected = connected ? TRUE : FALSE;
}

uint32 EXT_FUNC CGameClient::GetVoiceStream(int stream_id) {
	if (stream_id >= 0 && stream_id < ARRAYSIZE(m_pClient->m_VoiceStreams))
		return m_pClient->m_VoiceStreams[stream_id];
	return 0;
}

void EXT_FUNC CGameClient::SetLastVoiceTime(double time) {
	m_pClient->m_lastvoicetime = time;
}

double EXT_FUNC CGameClient::GetLastVoiceTime() {
	return m_pClient->m_lastvoicetime;
}

bool EXT_FUNC CGameClient::GetLoopback() {
	return m_pClient->m_bLoopback != FALSE;
}

INetChan* EXT_FUNC CGameClient::GetNetChan()
{
	return &m_NetChan;
}

sizebuf_t* EXT_FUNC CGameClient::GetDatagram()
{
	return &m_pClient->datagram;
}

edict_t* EXT_FUNC CGameClient::GetEdict()
{
	return m_pClient->edict;
}

USERID_t* EXT_FUNC CGameClient::GetNetworkUserID()
{
	return &m_pClient->network_userid;
}

const char* EXT_FUNC CGameClient::GetName()
{
	return m_pClient->name;
}

client_t* EXT_FUNC CGameClient::GetClient()
{
	return m_pClient;
}

bool EXT_FUNC CGameClient::IsFakeClient()
{
	return m_pClient->fakeclient;
}

void EXT_FUNC CGameClient::SetFakeClient(bool state)
{
	m_pClient->fakeclient = state ? TRUE : FALSE;
}

bool EXT_FUNC CGameClient::IsFullyConnected()
{
	return m_pClient->fully_connected;
}

void EXT_FUNC CGameClient::SetFullyConnected(bool state)
{
	m_pClient->fully_connected = state ? TRUE : FALSE;
}

bool EXT_FUNC CGameClient::IsUploading()
{
	return m_pClient->uploading != FALSE;
}

void EXT_FUNC CGameClient::SetUploading(bool state)
{
	m_pClient->uploading = state ? TRUE : FALSE;
}

bool EXT_FUNC CGameClient::IsHasUserMsgs()
{
	return m_pClient->hasusrmsgs != FALSE;
}

void EXT_FUNC CGameClient::SetHasUserMsgs(bool value)
{
	m_pClient->hasusrmsgs = value ? TRUE : FALSE;
}

bool EXT_FUNC CGameClient::HasForceUnmodified()
{
	return m_pClient->has_force_unmodified != FALSE;
}

void EXT_FUNC CGameClient::SetHasForceUnmodified(bool value)
{
	m_pClient->has_force_unmodified = value ? TRUE : FALSE;
}

int EXT_FUNC CGameClient::GetChokeCount()
{
	return m_pClient->chokecount;
}

void EXT_FUNC CGameClient::SetChokeCount(int count)
{
	m_pClient->chokecount = count;
}

int EXT_FUNC CGameClient::GetDeltaSequence()
{
	return m_pClient->delta_sequence;
}

void EXT_FUNC CGameClient::SetDeltaSequence(int value)
{
	m_pClient->delta_sequence = value;
}

void EXT_FUNC CGameClient::SetLastCmd(usercmd_t *ucmd)
{
	m_pClient->lastcmd = *ucmd;
}

double EXT_FUNC CGameClient::GetConnectTime()
{
	return m_pClient->connecttime;
}

void EXT_FUNC CGameClient::SetConnectTime(double time)
{
	m_pClient->connecttime = time;
}

double EXT_FUNC CGameClient::GetCmdTime()
{
	return m_pClient->cmdtime;
}

void EXT_FUNC CGameClient::SetCmdTime(double time)
{
	m_pClient->cmdtime = time;
}

double EXT_FUNC CGameClient::GetIgnoreCmdTime()
{
	return m_pClient->ignorecmdtime;
}

void EXT_FUNC CGameClient::SetIgnoreCmdTime(double time)
{
	m_pClient->ignorecmdtime = time;
}

float EXT_FUNC CGameClient::GetLatency()
{
	return m_pClient->latency;
}

void EXT_FUNC CGameClient::SetLatency(float latency)
{
	m_pClient->latency = latency;
}

float EXT_FUNC CGameClient::GetPacketLoss()
{
	return m_pClient->packet_loss;
}

void EXT_FUNC CGameClient::SetPacketLoss(float packetLoss)
{
	m_pClient->packet_loss = packetLoss;
}

double EXT_FUNC CGameClient::GetLocalTime()
{
	return m_pClient->localtime;
}

void EXT_FUNC CGameClient::SetLocalTime(double time)
{
	m_pClient->localtime = time;
}

double EXT_FUNC CGameClient::GetSvTimeBase()
{
	return m_pClient->svtimebase;
}

void EXT_FUNC CGameClient::SetSvTimeBase(double time)
{
	m_pClient->svtimebase = time;
}

double EXT_FUNC CGameClient::GetConnectionStartedTime()
{
	return m_pClient->connection_started;
}

void EXT_FUNC CGameClient::SetConnectionStartedTime(double time)
{
	m_pClient->connection_started = time;
}

double EXT_FUNC CGameClient::GetNextMessageTime()
{
	return m_pClient->next_messagetime;
}

void EXT_FUNC CGameClient::SetNextMessageTime(double time)
{
	m_pClient->next_messagetime = time;
}

double EXT_FUNC CGameClient::GetNextMessageIntervalTime()
{
	return m_pClient->next_messageinterval;
}

void EXT_FUNC CGameClient::SetNextMessageIntervalTime(double time_interval)
{
	m_pClient->next_messageinterval = time_interval;
}

bool EXT_FUNC CGameClient::GetSendMessageState()
{
	return m_pClient->send_message != FALSE;
}

void EXT_FUNC CGameClient::SetSendMessageState(bool state)
{
	m_pClient->send_message = state ? TRUE : FALSE;
}

bool EXT_FUNC CGameClient::GetSkipMessageState()
{
	return m_pClient->skip_message != FALSE;
}

void EXT_FUNC CGameClient::SetSkipMessageState(bool state)
{
	m_pClient->skip_message = state ? TRUE : FALSE;
}

bool EXT_FUNC CGameClient::GetSendInfoState()
{
	return m_pClient->sendinfo != FALSE;
}

void EXT_FUNC CGameClient::SetSendInfoState(bool state)
{
	m_pClient->sendinfo = state ? TRUE : FALSE;
}

float EXT_FUNC CGameClient::GetSendInfoTime()
{
	return m_pClient->sendinfo_time;
}

void EXT_FUNC CGameClient::SetSendInfoTime(float time)
{
	m_pClient->sendinfo_time = time;
}

client_frame_t *EXT_FUNC CGameClient::GetFrames()
{
	return m_pClient->frames;
}

event_state_t *EXT_FUNC CGameClient::GetEvents()
{
	return &m_pClient->events;
}

const edict_t *EXT_FUNC CGameClient::GetViewEntity()
{
	return m_pClient->pViewEntity;
}

void EXT_FUNC CGameClient::SetViewEntity(const edict_t *entity)
{
	m_pClient->pViewEntity = entity;
}

int EXT_FUNC CGameClient::GetUserID()
{
	return m_pClient->userid;
}

void EXT_FUNC CGameClient::SetUserID(int iUserID)
{
	m_pClient->userid = iUserID;
}

char *EXT_FUNC CGameClient::GetUserInfo()
{
	return m_pClient->userinfo;
}

char *EXT_FUNC CGameClient::GetHashedCDKey()
{
	return m_pClient->hashedcdkey;
}

int EXT_FUNC CGameClient::GetTopColor()
{
	return m_pClient->topcolor;
}

void EXT_FUNC CGameClient::SetTopColor(int color)
{
	m_pClient->topcolor = color;
}

int EXT_FUNC CGameClient::GetBottomColor()
{
	return m_pClient->bottomcolor;
}

void EXT_FUNC CGameClient::SetBottomColor(int color)
{
	m_pClient->bottomcolor = color;
}

resource_t *EXT_FUNC CGameClient::GetResourcesOnHand()
{
	return &m_pClient->resourcesonhand;
}

resource_t *EXT_FUNC CGameClient::GetResourcesNeeded()
{
	return &m_pClient->resourcesneeded;
}

FileHandle_t EXT_FUNC CGameClient::GetUploadFileHandle()
{
	return m_pClient->upload;
}

void EXT_FUNC CGameClient::SetUploadFileHandle(FileHandle_t fhFile)
{
	m_pClient->upload = fhFile;
}

bool EXT_FUNC CGameClient::IsUploadDoneRegistering()
{
	return m_pClient->uploaddoneregistering != FALSE;
}

void EXT_FUNC CGameClient::SetUploadDoneRegistering(bool state)
{
	m_pClient->uploaddoneregistering = state ? TRUE : FALSE;
}

customization_t *EXT_FUNC CGameClient::GetCustomizationData()
{
	return &m_pClient->customdata;
}

int EXT_FUNC CGameClient::GetCRC32MapValue()
{
	return m_pClient->crcValue;
}

void EXT_FUNC CGameClient::SetCRC32MapValue(int crcMapValue)
{
	m_pClient->crcValue = crcMapValue;
}

bool EXT_FUNC CGameClient::IsClientPredictingWeapons()
{
	return m_pClient->lw != FALSE;
}

void EXT_FUNC CGameClient::SetClientPredictingWeapons(bool state)
{
	m_pClient->lw = state ? TRUE : FALSE;
}

bool EXT_FUNC CGameClient::IsClientLagCompensation()
{
	return m_pClient->lc != FALSE;
}

void EXT_FUNC CGameClient::SetClientLagCompensation(bool state)
{
	m_pClient->lc = state ? TRUE : FALSE;
}

char *EXT_FUNC CGameClient::GetPhysInfo()
{
	return m_pClient->physinfo;
}

void EXT_FUNC CGameClient::SetVoiceStream(int stream_id, int value)
{
	if (stream_id < 0 || stream_id >= ARRAYSIZE(m_pClient->m_VoiceStreams))
		return;

	m_pClient->m_VoiceStreams[stream_id] = value;
}

int EXT_FUNC CGameClient::GetSendResourceCount()
{
	return m_pClient->m_sendrescount;
}

void EXT_FUNC CGameClient::SetSendResourceCount(int count)
{
	m_pClient->m_sendrescount = count;
}

bool EXT_FUNC CGameClient::IsSentNewResponse()
{
	return m_pClient->m_bSentNewResponse != FALSE;
}

void EXT_FUNC CGameClient::SetSentNewResponse(bool state)
{
	m_pClient->m_bSentNewResponse = state ? TRUE : FALSE;
}

CNetChan::CNetChan(netchan_t* chan)
{
	m_pNetChan = chan;
}

const netadr_t* EXT_FUNC CNetChan::GetRemoteAdr()
{
	return &m_pNetChan->remote_address;
}

sizebuf_t* EXT_FUNC CNetChan::GetMessageBuf()
{
	return &m_pNetChan->message;
}

netchan_t* EXT_FUNC CNetChan::GetChan()
{
	return m_pNetChan;
}

void CNetChan::Clear()
{
	for (int i = 0; i < MAX_STREAMS; i++)
	{
		for (int j = 0; j < NET_DECOMPRESS_MAX_TIMES; j++)
			m_FragStats[i].decompress_failure_times[j] = 0;
		m_FragStats[i].num_decompress_failures = 0;
	}
}


int EXT_FUNC CRehldsServerStatic::GetMaxClients()
{
	return g_psvs.maxclients;
}

int EXT_FUNC CRehldsServerStatic::GetMaxClientsLimit()
{
	return g_psvs.maxclientslimit;
}

client_t *EXT_FUNC CRehldsServerStatic::GetNextClient_t(client_t *client)
{
	return ++client;
}

int EXT_FUNC CRehldsServerStatic::GetSpawnCount()
{
	return g_psvs.spawncount;
}

void EXT_FUNC CRehldsServerStatic::SetSpawnCount(int count)
{
	g_psvs.spawncount = count;
}

server_log_t *EXT_FUNC CRehldsServerStatic::GetLog()
{
	return &g_psvs.log;
}

bool EXT_FUNC CRehldsServerStatic::IsSecure()
{
	return g_psvs.isSecure != FALSE;
}

void EXT_FUNC CRehldsServerStatic::SetSecure(bool value)
{
	g_psvs.isSecure = value ? TRUE : FALSE;
}

bool EXT_FUNC CRehldsServerStatic::IsLogActive()
{
	return g_psvs.log.active ? true : false;
}

IGameClient* EXT_FUNC CRehldsServerStatic::GetClient(int id)
{
	if (id < 0 || id >= g_psvs.maxclientslimit)
		Sys_Error("%s: invalid id provided: %d", __func__, id);

	return g_GameClients[id];
}

client_t* EXT_FUNC CRehldsServerStatic::GetClient_t(int id)
{
	if (id < 0 || id >= g_psvs.maxclientslimit)
		Sys_Error("%s: invalid id provided: %d", __func__, id);

	return &g_psvs.clients[id];
}

int EXT_FUNC CRehldsServerStatic::GetIndexOfClient_t(client_t* client)
{
	if (client < g_psvs.clients || client >= &g_psvs.clients[g_psvs.maxclientslimit])
		return -1;

	if (((size_t)client - (size_t)g_psvs.clients) % sizeof(client_t))
		return -1;

	return int(client - g_psvs.clients);
}



const char* EXT_FUNC CRehldsServerData::GetModelName() {
	return g_psv.modelname;
}

void EXT_FUNC CRehldsServerData::SetModelName(const char* modelname) {
	Q_strncpy(g_psv.modelname, modelname, ARRAYSIZE(g_psv.modelname) - 1);
	g_psv.modelname[ARRAYSIZE(g_psv.modelname) - 1] = '\0';
}

const char* EXT_FUNC CRehldsServerData::GetName() {
	return g_psv.name;
}

void EXT_FUNC CRehldsServerData::SetName(const char* name) {
	Q_strncpy(g_psv.name, name, ARRAYSIZE(g_psv.name) - 1);
	g_psv.name[ARRAYSIZE(g_psv.name) - 1] = '\0';
}

uint32 EXT_FUNC CRehldsServerData::GetWorldmapCrc() {
	return g_psv.worldmapCRC;
}

void EXT_FUNC CRehldsServerData::SetWorldmapCrc(uint32 crcValue) {
	g_psv.worldmapCRC = crcValue;
}

uint8* EXT_FUNC CRehldsServerData::GetClientDllMd5() {
	return g_psv.clientdllmd5;
}

sizebuf_t* EXT_FUNC CRehldsServerData::GetDatagram() {
	return &g_psv.datagram;
}

sizebuf_t* EXT_FUNC CRehldsServerData::GetReliableDatagram() {
	return &g_psv.reliable_datagram;
}

void EXT_FUNC CRehldsServerData::SetConsistencyNum(int num) {
	g_psv.num_consistency = num;
}

int EXT_FUNC CRehldsServerData::GetConsistencyNum() {
	return g_psv.num_consistency;
}

int EXT_FUNC CRehldsServerData::GetResourcesNum() {
	return g_psv.num_resources;
}

int EXT_FUNC CRehldsServerData::GetDecalNameNum() {
	return sv_decalnamecount;
}

void EXT_FUNC CRehldsServerData::SetDecalNameNum(int num) {
	sv_decalnamecount = num;
}

double EXT_FUNC CRehldsServerData::GetTime() {
	return g_psv.time;
}

double EXT_FUNC CRehldsServerData::GetOldTime() {
	return g_psv.oldtime;
}

void EXT_FUNC CRehldsServerData::SetResourcesNum(int num) {
	g_psv.num_resources = num;
}

struct resource_s *EXT_FUNC CRehldsServerData::GetResource(int index) {
#ifdef REHLDS_FIXES
	return &g_rehlds_sv.resources[index];
#else // REHLDS_FIXES
	return &g_psv.resourcelist[index];
#endif // REHLDS_FIXES
}

bool EXT_FUNC CRehldsServerData::IsActive()
{
	return g_psv.active != FALSE;
}

void EXT_FUNC CRehldsServerData::SetActive(bool state)
{
	g_psv.active = state ? TRUE : FALSE;
}

bool EXT_FUNC CRehldsServerData::IsPaused()
{
	return g_psv.paused != FALSE;
}

void EXT_FUNC CRehldsServerData::SetPaused(bool state)
{
	g_psv.paused = state ? TRUE : FALSE;
}

int EXT_FUNC CRehldsServerData::GetLastIndexCheckInPVS()
{
	return g_psv.lastcheck;
}

void EXT_FUNC CRehldsServerData::SetLastIndexCheckInPVS(int id)
{
	g_psv.lastcheck = id;
}

double EXT_FUNC CRehldsServerData::GetLastIndexCheckTimeInPVS()
{
	return g_psv.lastchecktime;
}

void EXT_FUNC CRehldsServerData::SetLastIndexCheckTimeInPVS(double time)
{
	g_psv.lastchecktime = time;
}

const char *EXT_FUNC CRehldsServerData::GetOldName()
{
	return g_psv.oldname;
}

void EXT_FUNC CRehldsServerData::SetOldName(const char *name)
{
	Q_strncpy(g_psv.oldname, name, ARRAYSIZE(g_psv.oldname) - 1);
	g_psv.oldname[ARRAYSIZE(g_psv.oldname) - 1] = '\0';
}

const char *EXT_FUNC CRehldsServerData::GetStartSpotName()
{
	return g_psv.startspot;
}

void EXT_FUNC CRehldsServerData::SetStartSpotName(const char *startspot)
{
	Q_strncpy(g_psv.startspot, startspot, ARRAYSIZE(g_psv.startspot) - 1);
	g_psv.startspot[ARRAYSIZE(g_psv.startspot) - 1] = '\0';
}

model_t *EXT_FUNC CRehldsServerData::GetWorldModel()
{
	return g_psv.worldmodel;
}

void EXT_FUNC CRehldsServerData::SetWorldModel(model_t *model)
{
	g_psv.worldmodel = model;
}

consistency_t *EXT_FUNC CRehldsServerData::GetConsistency(int index)
{
	if (index < 0 || index >= MAX_CONSISTENCY_LIST)
		return NULL;

	return &g_psv.consistency_list[index];
}

model_t *EXT_FUNC CRehldsServerData::GetModel(int index)
{
	if (index < 0 || index >= MAX_MODELS)
		return NULL;

	return g_psv.models[index];
}

event_t *EXT_FUNC CRehldsServerData::GetEventPrecache(int index)
{
	if (index < 0 || index >= MAX_EVENTS)
		return NULL;

	return &g_psv.event_precache[index];
}

entity_state_t *EXT_FUNC CRehldsServerData::GetEntityBaseline(int index)
{
	if (index < 0 || index >= g_psv.max_edicts)
		return NULL;

	return &g_psv.baselines[index];
}

extra_baselines_t *EXT_FUNC CRehldsServerData::GetEntityInstanceBaselines()
{
	return g_psv.instance_baselines;
}

int EXT_FUNC CRehldsServerData::GetNumGenericNames()
{
	return g_psv.num_generic_names;
}

void EXT_FUNC CRehldsServerData::SetNumGenericNames(int num)
{
	g_psv.num_generic_names = num;
}

int EXT_FUNC CRehldsServerData::GetNumEdicts()
{
	return g_psv.num_edicts;
}

void EXT_FUNC CRehldsServerData::SetNumEdicts(int num_edicts)
{
	g_psv.num_edicts = num_edicts;
}

int EXT_FUNC CRehldsServerData::GetMaxEdicts()
{
	return g_psv.max_edicts;
}

void EXT_FUNC CRehldsServerData::SetMaxEdicts(int max_edicts)
{
	g_psv.max_edicts = max_edicts;
}

edict_t *EXT_FUNC CRehldsServerData::GetEdict(int index)
{
	if (index < 0 || index >= g_psv.max_edicts)
		return NULL;

	return &g_psv.edicts[index];
}

server_state_t EXT_FUNC CRehldsServerData::GetState()
{
	return g_psv.state;
}

void EXT_FUNC CRehldsServerData::SetState(server_state_t st)
{
	g_psv.state = st;
}

sizebuf_t *EXT_FUNC CRehldsServerData::GetMulticastBuf()
{
	return &g_psv.multicast;
}

sizebuf_t *EXT_FUNC CRehldsServerData::GetSpectatorBuf()
{
	return &g_psv.spectator;
}

sizebuf_t *EXT_FUNC CRehldsServerData::GetSignonBuf()
{
	return &g_psv.signon;
}

void Rehlds_Interfaces_FreeClients()
{
	if (g_GameClients == NULL)
		return;

	for (int i = 0; i < g_psvs.maxclientslimit; i++)
	{
		delete g_GameClients[i];
	}

	free(g_GameClients);
	g_GameClients = NULL;
}

void Rehlds_Interfaces_InitClients()
{
	Rehlds_Interfaces_FreeClients();

	g_GameClients = (CGameClient**)malloc(sizeof(CGameClient*) * g_psvs.maxclientslimit);
	for (int i = 0; i < g_psvs.maxclientslimit; i++)
	{
		g_GameClients[i] = new CGameClient(i, &g_psvs.clients[i]);
	}
}

IGameClient* GetRehldsApiClient(client_t* cl)
{
	if (cl == NULL)
		return NULL; //I think it's logical.

	int idx = cl - g_psvs.clients;
	if (idx < 0 || idx >= g_psvs.maxclientslimit)
	{
		Sys_Error("%s: Invalid client index %d", __func__, idx);
	}

	return g_GameClients[idx];
}

ISteamGameServer* EXT_FUNC CRehldsServerData::GetSteamGameServer() {
	return ::SteamGameServer();
}

netadr_t* EXT_FUNC CRehldsServerData::GetNetFrom() {
	return &net_from;
}

void EXT_FUNC CRehldsServerData::SetNetFrom(netadr_t *from) {
	net_from = *from;
}
