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

DemoPlayer::DemoPlayer()
{
	m_World = nullptr;
	m_System = nullptr;
	m_Engine = nullptr;
	m_Server = nullptr;
}

bool DemoPlayer::Init(IBaseSystem *system, int serial, char *name)
{
	if (!BaseSystemModule::Init(system, serial, name)) {
		return false;
	}

	if (!name) {
		SetName(DEMOPLAYER_INTERFACE_VERSION);
	}

	m_Engine = (IEngineWrapper *)m_System->GetModule(ENGINEWRAPPER_INTERFACE_VERSION, "");
	if (!m_Engine) {
		m_System->Printf("DemoPlayer::Init: couldn't get engine interface.\n");
		return false;
	}

	m_Server = (IServer *)m_System->GetModule(SERVER_INTERFACE_VERSION, "core", "DemoServer");
	if (!m_Server) {
		m_System->Printf("DemoPlayer::Init: couldn't load server module.\n");
		return false;
	}

	m_Server->RegisterListener(this);
	m_Server->SetDirector(GetDirector());
	m_Server->SetDelayReconnect(false);

	m_World = (IWorld *)m_System->GetModule(WORLD_INTERFACE_VERSION, "core", "DemoWorld");
	if (!m_World) {
		m_System->Printf("DemoPlayer::Init: couldn't load world module.\n");
		return false;
	}

	m_System->RegisterCommand("dem_jump",      this, CMD_ID_JUMP);
	m_System->RegisterCommand("dem_forcehltv", this, CMD_ID_FORCEHLTV);
	m_System->RegisterCommand("dem_pause",     this, CMD_ID_PAUSE);
	m_System->RegisterCommand("dem_speed",     this, CMD_ID_SPEED);
	m_System->RegisterCommand("dem_start",     this, CMD_ID_START);
	m_System->RegisterCommand("dem_save",      this, CMD_ID_SAVE);

	m_World->RegisterListener(this);
	m_DemoStream.Resize(65536);

	SetTimeScale(1);
	SetPaused(false);

	m_Outgoing_sequence = 0;
	m_DeltaFrameSeqNr = 0;
	m_LastFrameSeqNr = 0;
	m_LastCmd = nullptr;

	m_LastFrameTime = 0;
	m_CommandCounter = 0;
	m_PlayerState = DEMOPLAYER_UNDEFINED;
	m_EditorMode = false;
	m_IsSaving = false;
	m_MasterMode = true;

	Q_memset(m_DemoFileName, 0, sizeof(m_DemoFileName));

	m_State = MODULE_RUNNING;
	m_System->DPrintf("DemoPlayer module initialized.\n");
	return true;
}

void DemoPlayer::RunFrame(double time)
{
	BaseSystemModule::RunFrame(time);

	if (m_PlayerState != DEMOPLAYER_UNDEFINED) {
		RunClocks();
	}
}

void DemoPlayer::ReceiveSignal(ISystemModule *module, unsigned int signal, void *data)
{
	int from = module->GetSerial();

	// Server module
	if (from == m_Server->GetSerial())
	{
		if (signal == 6) {
			m_System->Printf("Demo file completely loaded.\n");
			return;
		}
	}

	// World module
	if (from == m_World->GetSerial())
	{
		BitBuffer buf(32);
		switch (signal)
		{
		case 1:
			m_StartTime = 9999;
			break;
		case 2:
			NewGame(m_World);
			break;
		case 5:
		case 6:
			buf.WriteByte(svc_setpause);
			buf.WriteByte(signal == 5 ? true : false);
			break;
		}
	}
}

bool DemoPlayer::SaveGame(char *filename)
{
	if (IsLoading()) {
		return false;
	}

	SetPaused(true);

	m_IsSaving = true;
	auto ret = m_World->SaveAsDemo(filename ? filename : m_DemoFileName, this);
	m_IsSaving = false;

	return ret;
}

double DemoPlayer::GetWorldTime()
{
	return m_WorldTime;
}

double DemoPlayer::GetStartTime()
{
	frame_t *frame = m_World->GetFirstFrame();
	if (!frame) {
		return 0;
	}

	return frame->time;
}

double DemoPlayer::GetEndTime()
{
	frame_t *frame = m_World->GetLastFrame();
	if (!frame) {
		return 0;
	}

	return frame->time;
}

double DemoPlayer::GetPlayerTime()
{
	return m_PlayerTime;
}

bool DemoPlayer::IsLoading()
{
	if (!m_Server) {
		return false;
	}

	return m_Server->IsConnected();
}

bool DemoPlayer::IsActive()
{
	return m_PlayerState != 0;
}

char *DemoPlayer::GetType()
{
	return DEMOPLAYER_INTERFACE_VERSION;
}

char *DemoPlayer::GetStatusLine()
{
	return "No status available.\n";
}

IWorld *DemoPlayer::GetWorld()
{
	return m_World;
}

IDirector *DemoPlayer::GetDirector()
{
	return this;
}

void DemoPlayer::ExecuteCommand(int commandID, char *commandLine)
{
	switch (commandID)
	{
	case CMD_ID_JUMP:
		CMD_Jump(commandLine);
		break;
	case CMD_ID_FORCEHLTV:
		CMD_ForceHLTV(commandLine);
		break;
	case CMD_ID_PAUSE:
		CMD_Pause(commandLine);
		break;
	case CMD_ID_SPEED:
		CMD_Speed(commandLine);
		break;
	case CMD_ID_START:
		CMD_Start(commandLine);
		break;
	case CMD_ID_SAVE:
		CMD_Save(commandLine);
		break;
	default:
		m_System->Printf("ERROR! DemoPlayer::ExecuteCommand: unknown command ID %i.\n", commandID);
		break;
	}
}

void DemoPlayer::CMD_Jump(char *cmdLine)
{
	if (!IsActive()) {
		m_System->Printf("Not viewing a demo.\n");
		return;
	}

	TokenLine params(cmdLine);
	if (params.CountToken() != 2) {
		m_System->Printf("dem_jump <seconds>\n");
		return;
	}

	SetWorldTime(Q_atof(params.GetToken(1)), true);
	SetPaused(true);
}

void DemoPlayer::CMD_ForceHLTV(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2) {
		m_System->Printf("dem_forcehltv <0|1>\n");
		return;
	}

	char *val = params.GetToken(1);
	ForceHLTV(Q_atoi(val) ? true : false);
}

void DemoPlayer::CMD_Save(char *cmdLine)
{
	TokenLine params(cmdLine);
	if (params.CountToken() != 2) {
		m_System->Printf("dem_save <filename>\n");
		return;
	}

	if (!SaveGame(params.GetToken(1))) {
		m_System->Printf("Warning! Could save game as demo file.\n");
		return;
	}
}

void DemoPlayer::CMD_Pause(char *cmdLine)
{
	if (!IsActive()) {
		m_System->Printf("Not viewing a demo.\n");
		return;
	}

	TokenLine params(cmdLine);
	if (params.CountToken() != 2) {
		m_System->Printf("dem_pause <0|1>\n");
		return;
	}

	char *val = params.GetToken(1);
	SetPaused(Q_atoi(val) ? true : false);
}

void DemoPlayer::CMD_Speed(char *cmdLine)
{
	if (!IsActive()) {
		m_System->Printf("Not viewing a demo.\n");
		return;
	}

	TokenLine params(cmdLine);
	if (params.CountToken() != 2) {
		m_System->Printf("dem_speed <replayspeed>\n");
		return;
	}

	float timescale = (float)Q_atof(params.GetToken(1));
	SetTimeScale(timescale);
}

void DemoPlayer::CMD_Start(char *cmdLine)
{
	if (!IsActive()) {
		m_System->Printf("Not viewing a demo.\n");
		return;
	}

	SetWorldTime(GetStartTime(), false);
}

float DemoPlayer::GetTimeScale()
{
	return m_TimeScale;
}

char *DemoPlayer::GetModName()
{
	return "valve";
}

void DemoPlayer::WriteCommands(BitBuffer *stream, float startTime, float endTime)
{
	DirectorCmd *cmd = (DirectorCmd *)m_Commands.FindClosestKey(startTime);
	while (cmd && cmd->GetTime() <= endTime)
	{
		if (cmd->GetTime() > startTime)
		{
			if (m_IsSaving)
			{
				cmd->WriteToStream(stream);
			}
			else
			{
				if (cmd->GetType() == DRC_CMD_TIMESCALE)
				{
					float timescale;
					cmd->GetTimeScaleData(timescale);

					stream->WriteByte(svc_timescale);
					stream->WriteFloat(timescale);
					m_TimeScale = timescale;
				}
				else if (cmd->GetType() == DRC_CMD_CAMPATH)
				{
					float fov;
					vec3_t v;
					int flags;
					cmd->GetCamPathData(v, v, fov, flags);

					if (flags & DRC_FLAG_STARTPATH) {
						WriteCameraPath(cmd, stream);
					}
				}
				else
				{
					cmd->WriteToStream(stream);
				}

				m_LastCmd = cmd;
				BaseSystemModule::FireSignal(2);
				m_System->DPrintf("Director Cmd %s, Time %.2f\n", cmd->GetName(), cmd->GetTime());
			}
		}

		cmd = (DirectorCmd *)m_Commands.GetNext();
	}
}

int DemoPlayer::AddCommand(DirectorCmd *cmd)
{
	int type = cmd->GetType();
	if (type == DRC_CMD_WAYPOINTS) {
		return 0;
	}

	DirectorCmd *newcmd = new DirectorCmd;
	newcmd->Copy(cmd);

	if (!m_Commands.Add(newcmd, newcmd->GetTime())) {
		delete newcmd;
		return 0;
	}

	ReindexCommands();
	return newcmd->m_Index;
}

bool DemoPlayer::RemoveCommand(int index)
{
	DirectorCmd *cmd = (DirectorCmd *)m_Commands.GetFirst();
	while (cmd)
	{
		if (cmd->m_Index == index) {
			break;
		}

		cmd = (DirectorCmd *)m_Commands.GetNext();
	}

	if (cmd)
	{
		m_Commands.Remove(cmd);
		if (m_LastCmd == cmd) {
			m_LastCmd = nullptr;
		}

		delete cmd;
		ReindexCommands();
		return true;
	}

	return false;
}

IObjectContainer *DemoPlayer::GetCommands()
{
	return &m_Commands;
}

void DemoPlayer::SetWorldTime(double time, bool relative)
{
	if (relative)
		m_WorldTime += time;
	else
		m_WorldTime = time;
}

void DemoPlayer::SetTimeScale(float scale)
{
	m_TimeScale = Q_clamp(scale, 0.05f, 4.0f);

	m_DemoStream.WriteByte(svc_timescale);
	m_DemoStream.WriteFloat(m_TimeScale);
}

void DemoPlayer::SetPaused(bool state)
{
	m_IsPaused = state;
}

void DemoPlayer::SetEditMode(bool state)
{
	m_EditorMode = state;
}

bool DemoPlayer::IsEditMode()
{
	return m_EditorMode;
}

void DemoPlayer::Stop()
{
	m_Server->Disconnect();
	m_World->Reset();
	m_PlayerState = DEMOPLAYER_UNDEFINED;
}

void DemoPlayer::ForceHLTV(bool state)
{
	m_ForceHLTV = state;
}

void DemoPlayer::SetMasterMode(bool state)
{
	m_MasterMode = state;
}

bool DemoPlayer::IsMasterMode()
{
	return m_MasterMode;
}

bool DemoPlayer::IsPaused()
{
	return m_IsPaused;
}

void DemoPlayer::GetDemoViewInfo(ref_params_t *rp, float *view, int *viewmodel)
{
	int i;
	usercmd_t *oldcmd;
	movevars_t *oldmv;
	int oldviewport[4];

	frame_t *frame = m_World->GetFrameBySeqNr(m_LastFrameSeqNr);
	frame_t *fromFrame = m_World->GetFrameBySeqNr(m_LastFrameSeqNr - 1);

	if (!frame || !frame->demoInfo) {
		return;
	}

	oldviewport[0] = rp->viewport[0];
	oldviewport[1] = rp->viewport[1];
	oldviewport[2] = rp->viewport[2];
	oldviewport[3] = rp->viewport[3];

	oldmv = rp->movevars;
	oldcmd = rp->cmd;

	demo_info_t *demoInfo = (demo_info_t *)frame->demoInfo;
	Q_memcpy(rp, &demoInfo->rp, sizeof(*rp));

	rp->viewport[0] = oldviewport[0];
	rp->viewport[1] = oldviewport[1];
	rp->viewport[2] = oldviewport[2];
	rp->viewport[3] = oldviewport[3];
	rp->cmd = oldcmd;
	rp->movevars = oldmv;

	view[0] = demoInfo->view[0];
	view[1] = demoInfo->view[1];
	view[2] = demoInfo->view[2];

	*viewmodel = demoInfo->viewmodel;

	if (fromFrame && fromFrame->demoInfo)
	{
		demo_info_t *fromDemoInfo = (demo_info_t *)fromFrame->demoInfo;

		if (fromFrame->time < frame->time)
		{
			float frac = float(m_WorldTime - fromFrame->time) / (frame->time - fromFrame->time);
			for (i = 0; i < 3; i++) {
				rp->vieworg[i] = (demoInfo->rp.vieworg[i] - fromDemoInfo->rp.vieworg[i]) * frac + fromDemoInfo->rp.vieworg[i];
			}

			for (i = 0; i < 3; i++)
			{
				float d = demoInfo->rp.viewangles[i] - fromDemoInfo->rp.viewangles[i];
				if (d > 180) {
					d -= 360;
				}
				else if (d < -180) {
					d += 360;
				}

				rp->viewangles[i] = fromDemoInfo->rp.viewangles[i] + d * frac;
			}

			NormalizeAngles(rp->viewangles);

			for (i = 0; i < 3; i++) {
				rp->simvel[i] = (demoInfo->rp.simvel[i] - fromDemoInfo->rp.simvel[i]) * frac + fromDemoInfo->rp.simvel[i];
			}

			for (i = 0; i < 3; i++) {
				rp->simorg[i] = (demoInfo->rp.simorg[i] - fromDemoInfo->rp.simorg[i]) * frac + fromDemoInfo->rp.simorg[i];
			}

			for (i = 0; i < 3; i++) {
				rp->viewheight[i] = (demoInfo->rp.viewheight[i] - fromDemoInfo->rp.viewheight[i]) * frac + fromDemoInfo->rp.viewheight[i];
			}

			for (i = 0; i < 3; i++) {
				view[i] = (demoInfo->view[i] - fromDemoInfo->view[i]) * frac + fromDemoInfo->view[i];
			}
		}
	}
}

void DemoPlayer::WriteSpawn(BitBuffer *stream)
{
	m_World->WriteSigonData(stream);

	stream->WriteByte(svc_time);
	stream->WriteFloat(1);

	for (int i = 0; i < m_World->GetMaxClients(); i++) {
		m_World->WriteClientUpdate(stream, i);
	}

	m_World->WriteLightStyles(stream);

	stream->WriteByte(svc_signonnum);
	stream->WriteByte(1);
}

void DemoPlayer::WriteCameraPath(DirectorCmd *cmd, BitBuffer *stream)
{
	ObjectList path;
	path.Init();

	float startTime = cmd->GetTime();
	bool firstCmd = true;

	DirectorCmd *command = (DirectorCmd *)m_Commands.FindExactKey(cmd->GetTime());
	while (command && command->GetType() == DRC_CMD_CAMPATH)
	{
		vec3_t v;
		float fov;
		int flags;
		command->GetCamPathData(v, v, fov, flags);

		if (flags & DRC_FLAG_STARTPATH)
		{
			if (!firstCmd) {
				break;
			}

			firstCmd = false;
		}

		path.AddTail(command);
		command = (DirectorCmd *)m_Commands.GetNext();
	}

	command = (DirectorCmd *)path.GetFirst();
	if (!command) {
		m_System->Printf("Warning! No waypoints in camera path!\n");
		return;
	}

	int count = path.CountElements();
	int length = count * (command->m_Size + 2) + 2;
	if (length > 250) {
		m_System->Printf("Warning! Too many waypoints in a camera path!\n");
		return;
	}

	// tell director about it
	// send director message, that something important happed here
	stream->WriteByte(svc_director);
	stream->WriteByte(length);				// command length in bytes
	stream->WriteByte(DRC_CMD_WAYPOINTS);	// event waypoints
	stream->WriteByte(count);				// count waypoints

	while (command)
	{
		float time = (command->GetTime() - startTime) * 100;
		stream->WriteShort((int)time);
		stream->WriteBuf(command->m_Data.GetData(), command->m_Size);

		command = (DirectorCmd *)path.GetNext();
	}
}

void DemoPlayer::ReindexCommands()
{
	int index = 1;
	DirectorCmd *cmd = (DirectorCmd *)m_Commands.GetFirst();
	while (cmd)
	{
		cmd->m_Index = index++;
		cmd = (DirectorCmd *)m_Commands.GetNext();
	}

	BaseSystemModule::FireSignal(1);
}

int DemoPlayer::ReadDemoMessage(unsigned char *buffer, int size)
{
	int lastSeqNr = m_LastFrameSeqNr;
	switch (m_PlayerState)
	{
	case DEMOPLAYER_UNDEFINED:
	case DEMOPLAYER_INITIALIZING:
		return 0;
	case DEMOPLAYER_CONNECTING:
	{
		m_World->WriteNewData(&m_DemoStream);
		m_PlayerState = DEMOPLAYER_CONNECTED;
		break;
	}
	case DEMOPLAYER_CONNECTED:
	{
		m_LastFrameSeqNr = 0;
		m_DeltaFrameSeqNr = 0;
		WriteSpawn(&m_DemoStream);
		m_Engine->SetCvar("spec_pip", "0");

		m_WorldTime = 0;
		m_StartTime = m_PlayerTime;

		SetTimeScale(1);
		SetPaused(false);
		m_PlayerState = DEMOPLAYER_RUNNING;
		break;
	}
	case DEMOPLAYER_RUNNING:
	{
		WriteDatagram(&m_DemoStream);

		int newSeqNr = m_LastFrameSeqNr;
		while (lastSeqNr < newSeqNr)
		{
			frame_t *frame = m_World->GetFrameBySeqNr(++lastSeqNr);
			if (frame && frame->demoData && frame->demoDataSize) {
				BitBuffer stream(frame->demoData, frame->demoDataSize);
				ExecuteDemoFileCommands(&stream);
			}
		}
		break;
	}
	default:
		break;
	}

	int msgsize = m_DemoStream.CurrentSize();
	if (msgsize <= 0) {
		return 0;
	}

	if (msgsize > size) {
		m_System->Printf("ERROR! DemoPlayer::ReadDemoMessage: data overflow (%i bytes).\n", msgsize);
		return 0;
	}

	Q_memcpy(buffer, m_DemoStream.GetData(), msgsize);
	m_DemoStream.FastClear();
	return msgsize;
}

void DemoPlayer::ReadNetchanState(int *incoming_sequence, int *incoming_acknowledged, int *incoming_reliable_acknowledged, int *incoming_reliable_sequence, int *outgoing_sequence, int *reliable_sequence, int *last_reliable_sequence)
{
	*incoming_sequence = m_Outgoing_sequence;
	*incoming_acknowledged = m_Outgoing_sequence;
	*incoming_reliable_acknowledged = 0;
	*incoming_reliable_sequence = 0;
	*outgoing_sequence = m_Outgoing_sequence;
	*reliable_sequence = 0;
	*last_reliable_sequence = 0;
}

void DemoPlayer::SetName(char *newName)
{
	Q_strlcpy(m_Name, newName);
}

void DemoPlayer::RunClocks()
{
	float realTimeDiff = float(m_SystemTime - m_LastClockUpdateTime);

	m_LastClockUpdateTime = m_SystemTime;
	m_PlayerTime = realTimeDiff + m_PlayerTime;

	if (!m_World->IsActive() || m_IsPaused) {
		return;
	}

	m_WorldTime = realTimeDiff + m_WorldTime;

	frame_t *lastFrame = m_World->GetFrameBySeqNr(m_LastFrameSeqNr);
	frame_t *nextFrame = m_World->GetFrameBySeqNr(m_LastFrameSeqNr + 1);
	if (nextFrame && lastFrame)
	{
		if ((nextFrame->time - lastFrame->time) > 2) {
			m_WorldTime = nextFrame->time - 0.01f;
		}
	}

	lastFrame = m_World->GetLastFrame();
	frame_t *firstFrame = m_World->GetFirstFrame();
	if (firstFrame && lastFrame)
	{
		if (m_WorldTime > lastFrame->time) {
			m_WorldTime = lastFrame->time;
		}
		else if (firstFrame->time > m_WorldTime) {
			m_WorldTime = firstFrame->time - 0.01f;
		}
	}
}

void DemoPlayer::NewGame(IWorld *world, IProxy *proxy)
{
	m_World = world;
	m_PlayerTime = 1;
	m_PlayerState = DEMOPLAYER_CONNECTING;

	m_Commands.Clear(true);
	m_LastCmd = nullptr;

	BaseSystemModule::FireSignal(1);

	if (m_World->IsHLTV() || m_ForceHLTV)
	{
		static unsigned char cmd[] = {
			1,				// command length in bytes
			DRC_CMD_START	// tell them for start director
		};

		m_World->AddSignonData(svc_director, cmd, sizeof(cmd));
	}
}

void DemoPlayer::ShutDown()
{
	if (m_State == MODULE_DISCONNECTED) {
		return;
	}

	if (m_World) {
		m_World->ShutDown();
	}

	if (m_Server) {
		m_Server->ShutDown();
	}

	m_DemoStream.Free();
	m_Commands.Clear(true);

	BaseSystemModule::ShutDown();
	m_System->DPrintf("DemoPlayer module Shutdown.\n");
}

char *DemoPlayer::FormatTime(float time)
{
	static char timeCode[16];
	Q_snprintf(timeCode, sizeof(timeCode), "%02u:%02u:%02u", (int)time / 60, (int)time % 60, (int)(time * 100) % 100);
	return timeCode;
}

void DemoPlayer::RemoveFrames(double starttime, double endtime)
{
	;
}

void DemoPlayer::ExecuteDirectorCmd(DirectorCmd *cmd)
{
	switch (cmd->GetType())
	{
	case DRC_CMD_CAMERA:
	{
		float fov;
		int entity;
		vec3_t position, angles;
		cmd->GetCameraData(position, angles, fov, entity);
		cmd->SetCameraData(position, angles, fov, 0);
		cmd->WriteToStream(&m_DemoStream);
		cmd->SetCameraData(position, angles, fov, entity);
		break;
	}
	case DRC_CMD_TIMESCALE:
		float timescale;
		cmd->GetTimeScaleData(timescale);
		SetTimeScale(timescale);
		break;
	default:
		cmd->WriteToStream(&m_DemoStream);
		break;
	}
}

char *DemoPlayer::GetFileName()
{
	return m_DemoFileName;
}

DirectorCmd *DemoPlayer::GetLastCommand()
{
	return m_LastCmd;
}

bool DemoPlayer::LoadGame(char *filename)
{
	if (!m_Server->LoadDemo(m_World, filename, m_ForceHLTV, false)) {
		return false;
	}

	Q_strlcpy(m_DemoFileName, filename);

	m_World->SetBufferSize(-1);
	m_Outgoing_sequence = 0;
	m_LastClockUpdateTime = 0;
	m_LastFrameTime = 0;
	m_PlayerState = DEMOPLAYER_INITIALIZING;
	m_MasterMode = true;

	return true;
}

void DemoPlayer::WriteDatagram(BitBuffer *stream)
{
	frame_t *frame = m_World->GetFrameByTime(m_WorldTime);
	if (!frame) {
		return;
	}

	if (m_LastFrameSeqNr && m_LastFrameSeqNr <= frame->seqnr)
	{
		if (m_LastFrameSeqNr >= frame->seqnr) {
			return;
		}
	}
	else
	{
		m_LastFrameSeqNr = frame->seqnr - 1;
		if (frame->seqnr - 1 >= frame->seqnr) {
			return;
		}
	}

	double time = m_PlayerTime - (m_WorldTime - frame->time);
	stream->WriteByte(svc_time);
	stream->WriteFloat(float(time));

	m_World->WriteFrame(frame, m_LastFrameSeqNr, stream, stream, m_DeltaFrameSeqNr, m_Outgoing_sequence, true);

	if (m_MasterMode) {
		WriteCommands(stream, float(m_LastFrameTime), float(m_WorldTime));
	}

	m_LastFrameTime = m_WorldTime;
	if (stream->IsOverflowed())
	{
		m_System->Printf("Demo data stream overflow.\n");
		stream->Clear();

		m_DeltaFrameSeqNr = 0;
		m_LastFrameSeqNr = 0;
	}
	else
	{
		m_Outgoing_sequence++;
		m_DeltaFrameSeqNr = frame->seqnr;
		m_LastFrameSeqNr = frame->seqnr;
	}
}

void DemoPlayer::ExecuteDemoFileCommands(BitBuffer *stream)
{
	unsigned int cmd;
	while ((cmd = stream->ReadByte()) != -1)
	{
		switch ((DemoCmd)cmd)
		{
		case DemoCmd::StringCmd:
		{
			char szCmdName[64];
			stream->ReadBuf(sizeof(szCmdName), szCmdName);
			szCmdName[sizeof(szCmdName) - 1] = '\0';
			m_Engine->Cbuf_AddText(szCmdName);
			m_Engine->Cbuf_AddText("\n");
			break;
		}
		case DemoCmd::ClientData:
		{
			client_data_t cdat;
			stream->ReadBuf(sizeof(cdat), &cdat);
			m_Engine->DemoUpdateClientData(&cdat);
			break;
		}
		case DemoCmd::Event:
		{
			int flags   = _LittleLong(stream->ReadLong());
			int idx     = _LittleLong(stream->ReadLong());
			float delay = _LittleFloat(stream->ReadFloat());

			event_args_t eargs;
			stream->ReadBuf(sizeof(eargs), &eargs);

			m_Engine->CL_QueueEvent(flags, idx, delay, &eargs);
			break;
		}
		case DemoCmd::WeaponAnim:
		{
			int anim = _LittleLong(stream->ReadLong());
			int body = _LittleLong(stream->ReadLong());

			m_Engine->HudWeaponAnim(anim, body);
			break;
		}
		case DemoCmd::PlaySound:
		{
			int channel = stream->ReadLong();
			int sampleSize = stream->ReadLong();

			char sample[256];
			if (sampleSize < 0 || sampleSize >= (int)sizeof(sample))
			{
				m_System->Printf("WARNING! DemoPlayer::ExecuteDemoFileCommands: invalid sound sample size %i\n", sampleSize);
				return;
			}

			stream->ReadBuf(sampleSize, sample);
			sample[sampleSize] = '\0';

			float attenuation = _LittleFloat(stream->ReadFloat());
			float volume      = _LittleFloat(stream->ReadFloat());
			int flags         = _LittleLong(stream->ReadLong());
			int pitch         = _LittleLong(stream->ReadLong());

			m_Engine->CL_DemoPlaySound(channel, sample, attenuation, volume, flags, pitch);
			break;
		}
		case DemoCmd::PayLoad:
		{
			unsigned char data[32768];
			Q_memset(data, 0, sizeof(data));

			int length = stream->ReadLong();
			if (length < 0 || length > (int)sizeof(data))
			{
				m_System->Printf("WARNING! DemoPlayer::ExecuteDemoFileCommands: invalid payload size %i\n", length);
				return;
			}

			stream->ReadBuf(length, data);

			m_Engine->ClientDLL_ReadDemoBuffer(length, data);
			break;
		}
		default:
			m_System->Printf("WARNING! DemoPlayer::ExecuteDemoFileCommands: unexpected demo file command %i\n", cmd);
			return;
		}
	}
}

void DemoPlayer::RegisterListener(ISystemModule *module)
{
	BaseSystemModule::RegisterListener(module);
}

void DemoPlayer::RemoveListener(ISystemModule *module)
{
	BaseSystemModule::RemoveListener(module);
}

IBaseSystem *DemoPlayer::GetSystem()
{
	return BaseSystemModule::GetSystem();
}

int DemoPlayer::GetSerial()
{
	return BaseSystemModule::GetSerial();
}

char *DemoPlayer::GetName()
{
	return BaseSystemModule::GetName();
}

int DemoPlayer::GetState()
{
	return BaseSystemModule::GetState();
}

int DemoPlayer::GetVersion()
{
	return BaseSystemModule::GetVersion();
}
