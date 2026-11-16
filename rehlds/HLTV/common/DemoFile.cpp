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

DemoFile::DemoFile() :
	m_FileHandle(FILESYSTEM_INVALID_HANDLE),
	m_DemoChannel(nullptr),
	m_Entries(nullptr),
	m_FileSystem(nullptr)
{
	Reset();
}

void DemoFile::Init(IWorld *world, IServer *server, NetChannel *channel)
{
	m_World = world;
	m_Server = server;
	m_System = world->GetSystem();
	m_FileSystem = m_System->GetFileSystem();

	m_DemoChannel = channel;
	m_Entries = nullptr;

	Reset();
}

void DemoFile::Reset()
{
	Q_memset(m_FileName, 0, sizeof(m_FileName));
	Q_memset(&m_zeroDemoInfo, 0, sizeof(m_zeroDemoInfo));

	CloseFile();
	m_Continuous = true;
}

bool DemoFile::IsRecording()
{
	return m_DemoState == DEMO_RECORDING;
}

void DemoFile::CloseFile()
{
	unsigned char c;
	float f;
	int curpos;
	int i;

	if (m_FileSystem && m_FileHandle != FILESYSTEM_INVALID_HANDLE)
	{
		if (m_DemoState == DEMO_RECORDING)
		{
			c = 5;
			m_FileSystem->Write(&c, sizeof(unsigned char), m_FileHandle);

			f = _LittleFloat(GetDemoTime());
			m_FileSystem->Write(&f, sizeof(float), m_FileHandle);

			i = _LittleLong(m_frameCount);
			m_FileSystem->Write(&i, sizeof(int), m_FileHandle);

			curpos = m_FileSystem->Tell(m_FileHandle);

			m_gameEntry.nFileLength = curpos - m_gameEntry.nOffset;
			m_gameEntry.fTrackTime = GetDemoTime();
			m_gameEntry.nFrames = m_frameCount;

			i = 2;
			m_FileSystem->Write(&i, sizeof(int), m_FileHandle);
			m_FileSystem->Write(&m_loadEntry, sizeof(m_loadEntry), m_FileHandle);
			m_FileSystem->Write(&m_gameEntry, sizeof(m_gameEntry), m_FileHandle);

			m_demoHeader.nDirectoryOffset = curpos;
			m_FileSystem->Seek(m_FileHandle, 0, FILESYSTEM_SEEK_HEAD);
			m_FileSystem->Write(&m_demoHeader, sizeof(m_demoHeader), m_FileHandle);
			m_System->Printf("Completed demo %s.\n", m_FileName);
		}

		m_FileSystem->Close(m_FileHandle);
	}

	if (m_Entries)
	{
		Mem_Free(m_Entries);
		m_Entries = nullptr;
	}

	m_FileHandle = FILESYSTEM_INVALID_HANDLE;
	m_DemoState = DEMO_IDLE;
}

void DemoFile::WriteDemoInfo(demo_info_t *demoInfo)
{
	m_FileSystem->Write(demoInfo, sizeof(*demoInfo), m_FileHandle);
}

void DemoFile::WriteSequenceInfo()
{
	m_FileSystem->Write(&m_DemoChannel->m_outgoing_sequence,              sizeof(int), m_FileHandle);
	m_FileSystem->Write(&m_DemoChannel->m_incoming_sequence,              sizeof(int), m_FileHandle);
	m_FileSystem->Write(&m_DemoChannel->m_last_reliable_sequence,         sizeof(int), m_FileHandle);
	m_FileSystem->Write(&m_DemoChannel->m_reliable_sequence,              sizeof(int), m_FileHandle);
	m_FileSystem->Write(&m_DemoChannel->m_incoming_acknowledged,          sizeof(int), m_FileHandle);
	m_FileSystem->Write(&m_DemoChannel->m_incoming_reliable_sequence,     sizeof(int), m_FileHandle);
	m_FileSystem->Write(&m_DemoChannel->m_incoming_reliable_acknowledged, sizeof(int), m_FileHandle);
}

void DemoFile::WriteDemoStartup(BitBuffer *buffer)
{
	unsigned char c;
	int i;
	float f;

	int len = buffer->CurrentSize();
	if (len <= 0) {
		return;
	}

	c = 0;
	m_FileSystem->Write(&c, sizeof(unsigned char), m_FileHandle);

	f = _LittleFloat(GetDemoTime());
	m_FileSystem->Write(&f, sizeof(float), m_FileHandle);

	i = _LittleLong(m_frameCount);
	m_FileSystem->Write(&i, sizeof(int), m_FileHandle);

	WriteDemoInfo(&m_zeroDemoInfo);
	WriteSequenceInfo();

	i = _LittleLong(len);
	m_FileSystem->Write(&i, sizeof(int), m_FileHandle);
	m_FileSystem->Write(buffer->GetData(), len, m_FileHandle);
}

void DemoFile::WriteDemoMessage(BitBuffer *unreliableData, BitBuffer *reliableData)
{
	int len;
	int i;
	float f;
	unsigned char c;

	len = reliableData->CurrentSize() + unreliableData->CurrentSize();
	if (len <= 0 || m_FileHandle == FILESYSTEM_INVALID_HANDLE) {
		return;
	}

	if (!m_FileSystem) {
		return;
	}

	c = 1;
	m_frameCount++;

	m_FileSystem->Write(&c, sizeof(unsigned char), m_FileHandle);

	f = _LittleFloat(GetDemoTime());
	m_FileSystem->Write(&f, sizeof(float), m_FileHandle);

	i = _LittleLong(m_frameCount);
	m_FileSystem->Write(&i, sizeof(int), m_FileHandle);

	WriteDemoInfo(&m_zeroDemoInfo);
	WriteSequenceInfo();

	i = _LittleLong(len);
	m_FileSystem->Write(&i, sizeof(int), m_FileHandle);
	m_FileSystem->Write(unreliableData->GetData(), unreliableData->CurrentSize(), m_FileHandle);
	m_FileSystem->Write(reliableData->GetData(), reliableData->CurrentSize(), m_FileHandle);
}

void DemoFile::WriteUpdateClientData(client_data_t *cdata)
{
	if (!m_FileSystem || m_FileHandle == FILESYSTEM_INVALID_HANDLE) {
		return;
	}

	unsigned char cmd = 4;
	m_FileSystem->Write(&cmd, sizeof(unsigned char), m_FileHandle);

	float f = _LittleFloat(GetDemoTime());
	m_FileSystem->Write(&f, sizeof(float), m_FileHandle);

	int i = _LittleLong(m_frameCount);
	m_FileSystem->Write(&i, sizeof(int), m_FileHandle);
	m_FileSystem->Write(cdata, sizeof(*cdata), m_FileHandle);
}

float DemoFile::GetDemoTime()
{
	return m_System->GetTime() - m_startTime;
}

void DemoFile::WriteSignonData()
{
	BitBuffer buffer(MAX_POSSIBLE_MSG * 2);
	m_World->WriteNewData(&buffer);
	m_World->WriteSigonData(&buffer);

	buffer.WriteByte(svc_time);
	buffer.WriteFloat(GetDemoTime());

	for (int i = 0; i < m_World->GetMaxClients(); i++) {
		m_World->WriteClientUpdate(&buffer, i);
	}

	m_World->WriteLightStyles(&buffer);

	buffer.WriteByte(svc_signonnum);
	buffer.WriteByte(1);

	WriteDemoStartup(&buffer);
}

bool DemoFile::StartRecording(char *newName)
{
	unsigned char b;
	int i;
	float f;

	if (IsPlaying() || !m_FileSystem) {
		return false;
	}

	if (m_FileHandle != FILESYSTEM_INVALID_HANDLE) {
		CloseFile();
	}

	Q_strlcpy(m_FileName, newName);

	COM_DefaultExtension(m_FileName, ".dem");

	if (m_FileName[0] == '/'
		|| Q_strstr(m_FileName, ":")
		|| Q_strstr(m_FileName, "..")
		|| Q_strstr(m_FileName, "\\"))
	{
		m_System->Printf("WARNING! DemoFile::StartRecording: unable to open %s (contains illegal characters).\n", m_FileName);
		return false;
	}

	const char *pszFileExt = COM_FileExtension(m_FileName);
	if (Q_stricmp(pszFileExt, "dem") != 0)
	{
		m_System->Printf("WARNING! DemoFile::StartRecording: unable to open %s (wrong file extension, must be .dem).\n", m_FileName);
		return false;
	}

	m_FileHandle = m_FileSystem->Open(m_FileName, "wb");
	if (!m_FileHandle) {
		m_System->Printf("WARNING! DemoFile::StartRecording: coudn't open demo file %s.\n", m_FileName);
		return false;
	}

	Q_memset(&m_demoHeader, 0, sizeof(m_demoHeader));
	Q_strcpy(m_demoHeader.szFileStamp, "HLDEMO");

	COM_FileBase(m_World->GetLevelName(), m_demoHeader.szMapName);
	COM_FileBase(m_World->GetGameDir(), m_demoHeader.szDllDir);

	m_demoHeader.mapCRC = 0;
	m_demoHeader.nDemoProtocol = DEMO_PROTOCOL;
	m_demoHeader.nNetProtocolVersion = PROTOCOL_VERSION;
	m_demoHeader.nDirectoryOffset = 0;
	m_FileSystem->Write(&m_demoHeader, sizeof(m_demoHeader), m_FileHandle);

	Q_memset(&m_loadEntry, 0, sizeof(m_loadEntry));
	Q_strcpy(m_loadEntry.szDescription, "LOADING");

	m_loadEntry.nEntryType = DEMO_STARTUP;
	m_loadEntry.nOffset = m_FileSystem->Tell(m_FileHandle);

	m_frameCount = 0;
	m_startTime = m_System->GetTime();

	WriteSignonData();

	b = 5;
	m_FileSystem->Write(&b, sizeof(unsigned char), m_FileHandle);

	f = _LittleFloat(GetDemoTime());
	m_FileSystem->Write(&f, sizeof(float), m_FileHandle);

	i = _LittleLong(m_frameCount);
	m_FileSystem->Write(&i, sizeof(int), m_FileHandle);

	m_loadEntry.nFileLength = m_FileSystem->Tell(m_FileHandle) - m_loadEntry.nOffset;

	Q_memset(&m_gameEntry, 0, sizeof(m_gameEntry));
	Q_snprintf(m_gameEntry.szDescription, sizeof(m_gameEntry.szDescription), "Playback");

	m_gameEntry.nEntryType = DEMO_NORMAL;
	m_gameEntry.nOffset = m_FileSystem->Tell(m_FileHandle);

	b = 2;
	m_FileSystem->Write(&b, sizeof(unsigned char), m_FileHandle);

	f = _LittleFloat(GetDemoTime());
	m_FileSystem->Write(&f, sizeof(float), m_FileHandle);

	i = _LittleLong(m_frameCount);
	m_FileSystem->Write(&i, sizeof(int), m_FileHandle);

	m_DemoState = DEMO_RECORDING;
	m_System->Printf("Start recording to %s.\n", m_FileName);

	return true;
}

bool DemoFile::IsPlaying()
{
	return m_DemoState == DEMO_PLAYING;
}

bool DemoFile::LoadDemo(char *demoname)
{
	if (IsRecording()) {
		m_System->Printf("Cannot load demo, still recording.\n");
		return false;
	}

	if (!m_FileSystem) {
		return false;
	}

	CloseFile();

	Q_strlcpy(m_FileName, demoname);
	Q_strlwr(m_FileName);

	if (!Q_strstr(m_FileName, ".dem")) {
		Q_strlcat(m_FileName, ".dem");
	}

	m_FileHandle = m_FileSystem->Open(m_FileName, "rb");
	if (!m_FileHandle) {
		m_System->Printf("Coudn't open demo file %s.\n", m_FileName);
		return false;
	}

	Q_memset(&m_demoHeader, 0, sizeof(m_demoHeader));
	m_FileSystem->Read(&m_demoHeader, sizeof(m_demoHeader), m_FileHandle);

	if (Q_strcmp(m_demoHeader.szFileStamp, "HLDEMO") != 0) {
		m_System->Printf("%s is not a HL demo file.\n", m_FileName);
		m_FileSystem->Close(m_FileHandle);
		return false;
	}

	if (m_demoHeader.nNetProtocolVersion != PROTOCOL_VERSION || m_demoHeader.nDemoProtocol != DEMO_PROTOCOL) {
		m_System->Printf("WARNING! %s has an outdated demo format.\n", m_FileName);
	}

	int fileMarker = m_FileSystem->Tell(m_FileHandle);
	m_FileSystem->Seek(m_FileHandle, m_demoHeader.nDirectoryOffset, FILESYSTEM_SEEK_HEAD);
	m_FileSystem->Read(&m_EntryNumber, sizeof(int), m_FileHandle);
	m_CurrentEntry = 0;

	if (m_EntryNumber > 0 && m_EntryNumber <= MAX_DEMO_ENTRY)
	{
		m_Entries = (demoentry_t *)Mem_ZeroMalloc(sizeof(demoentry_t) * m_EntryNumber);
		m_FileSystem->Read(m_Entries, sizeof(demoentry_t) * m_EntryNumber, m_FileHandle);
		m_FileSystem->Seek(m_FileHandle, m_Entries[m_CurrentEntry].nOffset, FILESYSTEM_SEEK_HEAD);
	}
	else
	{
		m_EntryNumber = 2;
		m_Entries = nullptr;

		m_System->Printf("WARNING! Demo had bogus number of directory entries!\n");
		m_FileSystem->Seek(m_FileHandle, fileMarker, FILESYSTEM_SEEK_HEAD);
	}

	m_startTime = m_System->GetTime();
	m_nextReadTime = m_System->GetTime();

	m_Continuous = true;
	Q_memset(&m_ServerInfo, 0, sizeof(m_ServerInfo));

	Q_strlcpy(m_ServerInfo.address, m_DemoChannel->m_remote_address.ToBaseString());
	Q_strlcpy(m_ServerInfo.name, m_FileName);
	Q_strlcpy(m_ServerInfo.map, m_demoHeader.szMapName);
	Q_strlcpy(m_ServerInfo.gamedir, m_demoHeader.szDllDir);
	Q_strlcpy(m_ServerInfo.description, "Demo Playback");

	m_ServerInfo.activePlayers = 0;
	m_ServerInfo.maxPlayers = MAX_CLIENTS;
	m_ServerInfo.protocol = m_demoHeader.nNetProtocolVersion;
	m_ServerInfo.type = GetServerType(HLST_Dedicated)[0];
	m_ServerInfo.os = GetServerOS()[0];
	m_ServerInfo.pw = '\0';
	m_ServerInfo.mod = false;
	m_DemoState = DEMO_PLAYING;

	return true;
}

void DemoFile::StopPlayBack()
{
	if (m_DemoState != DEMO_PLAYING) {
		return;
	}

	CloseFile();
	if (m_Server) {
		m_Server->Disconnect();
	}
}

void DemoFile::ReadDemoPacket(BitBuffer *demoData, demo_info_t *demoInfo)
{
	if (!m_FileHandle || (m_nextReadTime > m_System->GetTime() && m_Continuous)) {
		return;
	}

	int msglen; // command length in bytes
	unsigned char msgbuf[MAX_POSSIBLE_MSG];
	float time;
	DemoCmd cmd;
	int frame;
	int channel;
	int sampleSize;

	bool readNextCmd = true;
	while (readNextCmd)
	{
		unsigned int curpos = m_FileSystem->Tell(m_FileHandle);
		if (m_FileSystem->Read(&cmd, sizeof(cmd), m_FileHandle) != sizeof(cmd)) {
			StopPlayBack();
			return;
		}

		m_FileSystem->Read(&time, sizeof(time), m_FileHandle);
		time = _LittleFloat(time);

		m_FileSystem->Read(&frame, sizeof(frame), m_FileHandle);
		frame = _LittleLong(frame);

		if (cmd != DemoCmd::Unknown && cmd != DemoCmd::Read) {
			m_nextReadTime = m_startTime + time;
		}

		if (m_nextReadTime > m_System->GetTime() && m_Continuous) {
			m_FileSystem->Seek(m_FileHandle, curpos, FILESYSTEM_SEEK_HEAD);
			return;
		}

		msglen = 0;

		switch (cmd)
		{
		case DemoCmd::StartTime:
			m_startTime = (float)m_System->GetTime();
			break;
		case DemoCmd::StringCmd:
			msglen = sizeof(char [64]);
			break;
		case DemoCmd::ClientData:
			msglen = sizeof(client_data_t);
			break;
		case DemoCmd::Read:
		{
			if (++m_CurrentEntry >= m_EntryNumber) {
				StopPlayBack();
				return;
			}

			if (m_Entries) {
				m_FileSystem->Seek(m_FileHandle, m_Entries[m_CurrentEntry].nOffset, FILESYSTEM_SEEK_HEAD);
			}
			break;
		}
		case DemoCmd::Event:
			msglen = sizeof(int)			// flags
					+ sizeof(int)			// idx
					+ sizeof(float)			// delay
					+ sizeof(event_args_t);	// eargs
			break;
		case DemoCmd::WeaponAnim:
			msglen = sizeof(int)	// anim
					+ sizeof(int);	// body
			break;
		case DemoCmd::PlaySound:
		{
			m_FileSystem->Read(&channel, sizeof(channel), m_FileHandle);
			channel = _LittleLong(channel);

			m_FileSystem->Read(&sampleSize, sizeof(sampleSize), m_FileHandle);
			sampleSize = _LittleLong(sampleSize);
			msglen = sampleSize
					+ sizeof(float)	// attenuation
					+ sizeof(float)	// volume
					+ sizeof(int)	// flags
					+ sizeof(int);	// pitch
			break;
		}
		case DemoCmd::PayLoad:
		{
			m_FileSystem->Read(&msglen, sizeof(msglen), m_FileHandle);
			msglen = _LittleLong(msglen);
			break;
		}
		default:
			readNextCmd = false;
			break;
		}

		if (msglen < 0 || msglen > MAX_POSSIBLE_MSG)
		{
			m_System->DPrintf("WARNING! DemoFile::ReadDemoPacket: invalid demo command length %i\n", msglen);
			return;
		}

		if (msglen)
		{
			m_FileSystem->Read(msgbuf, msglen, m_FileHandle);

			demoData->WriteByte((unsigned char)cmd);

			switch (cmd)
			{
			case DemoCmd::PlaySound:
				demoData->WriteLong(channel);
				demoData->WriteLong(sampleSize);
				break;
			case DemoCmd::PayLoad:
				demoData->WriteLong(msglen);
				break;
			default:
				break;
			}

			demoData->WriteBuf(msgbuf, msglen);
		}
	}

	ReadDemoInfo(demoInfo);
	ReadSequenceInfo();

	int length;
	if (m_FileSystem->Read(&length, sizeof(length), m_FileHandle) != sizeof(length)) {
		m_System->DPrintf("WARNING! DemoFile::ReadDemoPacket: Bad demo length.\n");
		StopPlayBack();
		return;
	}

	length = _LittleLong(length);
	if (length < 0) {
		m_System->DPrintf("WARNING! DemoFile::ReadDemoPacket: Demo message length < 0.\n");
		StopPlayBack();
		return;
	}

	if (length > MAX_POSSIBLE_MSG) {
		m_System->DPrintf("WARNING! DemoFile::ReadDemoPacket: message length > MAX_POSSIBLE_MSG\n");
		StopPlayBack();
		return;
	}

	if (length > 0)
	{
		if (m_FileSystem->Read(msgbuf, length, m_FileHandle) != length) {
			m_System->DPrintf("WARNING! DemoFile::ReadDemoPacket: Error reading demo message data.\n");
			StopPlayBack();
			return;
		}

		NetPacket *p = new NetPacket;

		p->connectionless = false;
		p->time = m_System->GetTime();
		p->seqnr = m_DemoChannel->m_incoming_sequence;
		p->data.Resize(length);
		p->data.WriteBuf(msgbuf, length);
		p->data.Reset();

		m_DemoChannel->m_incomingPackets.AddHead(p);
	}
}

void DemoFile::ReadDemoInfo(demo_info_t *demoInfo)
{
	m_FileSystem->Read(demoInfo, sizeof(*demoInfo), m_FileHandle);

	demoInfo->rp.cmd = &demoInfo->cmd;
	demoInfo->rp.movevars = &demoInfo->movevars;
}

void DemoFile::ReadSequenceInfo()
{
	m_FileSystem->Read(&m_DemoChannel->m_incoming_sequence,              sizeof(int), m_FileHandle);
	m_FileSystem->Read(&m_DemoChannel->m_incoming_acknowledged,          sizeof(int), m_FileHandle);
	m_FileSystem->Read(&m_DemoChannel->m_incoming_reliable_acknowledged, sizeof(int), m_FileHandle);
	m_FileSystem->Read(&m_DemoChannel->m_incoming_reliable_sequence,     sizeof(int), m_FileHandle);
	m_FileSystem->Read(&m_DemoChannel->m_outgoing_sequence,              sizeof(int), m_FileHandle);
	m_FileSystem->Read(&m_DemoChannel->m_reliable_sequence,              sizeof(int), m_FileHandle);
	m_FileSystem->Read(&m_DemoChannel->m_last_reliable_sequence,         sizeof(int), m_FileHandle);
}

char *DemoFile::GetFileName()
{
	return m_FileName;
}

bool DemoFile::IsContinuous()
{
	return m_Continuous;
}

void DemoFile::SetContinuous(bool state)
{
	m_Continuous = state;
}
