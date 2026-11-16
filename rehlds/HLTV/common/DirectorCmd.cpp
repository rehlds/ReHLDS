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

// define DRC_CMD_*
char *DirectorCmd::m_CMD_Name[] =
{
	"NONE",
	"START",
	"EVENT",
	"MODE",
	"CAMERA",
	"TIMESCALE",
	"MESSAGE",
	"SOUND",
	"STATUS",
	"BANNER",
	"STUFFTEXT",
	"CHASE",
	"INEYE",
	"MAP",
	"CAMPATH",
	"WAYPOINTS"
};

DirectorCmd::DirectorCmd() :
	m_Size(0), m_Index(0)
{
	Clear();
}

DirectorCmd::~DirectorCmd()
{
	Clear();
}

bool DirectorCmd::GetEventData(int &entity1, int &entity2, int &flags)
{
	if (m_Type != DRC_CMD_EVENT) {
		return false;
	}

	m_Data.Reset();
	entity1 = m_Data.ReadWord();
	entity2 = m_Data.ReadWord();
	flags = m_Data.ReadLong();

	return true;
}

bool DirectorCmd::GetModeData(int &mode)
{
	if (m_Type != DRC_CMD_MODE) {
		return false;
	}

	m_Data.Reset();
	mode = m_Data.ReadByte();

	return true;
}

bool DirectorCmd::GetChaseData(int &entity1, int &entity2, float &distance, int &flags)
{
	if (m_Type != DRC_CMD_CHASE) {
		return false;
	}

	m_Data.Reset();
	entity1 = m_Data.ReadByte();
	entity2 = m_Data.ReadByte();
	distance = m_Data.ReadFloat();
	flags = m_Data.ReadByte();

	return true;
}

bool DirectorCmd::GetInEyeData(int &player)
{
	if (m_Type != DRC_CMD_INEYE) {
		return false;
	}

	m_Data.Reset();
	player = m_Data.ReadByte();

	return true;
}

bool DirectorCmd::GetMapData(int &entity, float &angle, float &distance)
{
	if (m_Type != DRC_CMD_MAP) {
		return false;
	}

	m_Data.Reset();
	entity = m_Data.ReadByte();
	angle = m_Data.ReadFloat();
	distance = m_Data.ReadFloat();

	return true;
}

bool DirectorCmd::GetCameraData(vec_t *position, vec_t *angles, float &fov, int &entity)
{
	if (m_Type != DRC_CMD_CAMERA) {
		return false;
	}

	m_Data.Reset();

	position[0] = m_Data.ReadCoord();
	position[1] = m_Data.ReadCoord();
	position[2] = m_Data.ReadCoord();

	angles[0] = m_Data.ReadCoord();
	angles[1] = m_Data.ReadCoord();
	angles[2] = m_Data.ReadCoord();

	fov = (float)m_Data.ReadByte();
	entity = m_Data.ReadWord();

	return true;
}

bool DirectorCmd::GetCamPathData(vec_t *position, vec_t *angles, float &fov, int &flags)
{
	if (m_Type != DRC_CMD_CAMPATH) {
		return false;
	}

	m_Data.Reset();

	position[0] = m_Data.ReadCoord();
	position[1] = m_Data.ReadCoord();
	position[2] = m_Data.ReadCoord();

	angles[0] = m_Data.ReadCoord();
	angles[1] = m_Data.ReadCoord();
	angles[2] = m_Data.ReadCoord();

	fov = (float)m_Data.ReadByte();
	flags = m_Data.ReadByte();

	return true;
}

bool DirectorCmd::GetSoundData(char *name, float &volume)
{
	if (m_Type != DRC_CMD_SOUND) {
		return false;
	}

	m_Data.Reset();
	Q_strcpy(name, m_Data.ReadString());
	volume = m_Data.ReadFloat();

	return true;
}

float DirectorCmd::GetTime()
{
	return m_Time;
}

int DirectorCmd::GetType()
{
	return m_Type;
}

char *DirectorCmd::GetName()
{
	return m_CMD_Name[m_Type];
}

bool DirectorCmd::GetTimeScaleData(float &factor)
{
	if (m_Type != DRC_CMD_TIMESCALE) {
		return false;
	}

	m_Data.Reset();
	factor = m_Data.ReadFloat();

	return true;
}

bool DirectorCmd::GetWayPointsData(int &number)
{
	if (m_Type != DRC_CMD_WAYPOINTS) {
		return false;
	}

	m_Data.Reset();
	number = m_Data.ReadByte();

	return true;
}

bool DirectorCmd::GetMessageData(int &effect, int &color, vec_t *position, float &fadein, float &fadeout, float &holdtime, float &fxtime, char *text)
{
	if (m_Type != DRC_CMD_MESSAGE) {
		return false;
	}

	m_Data.Reset();

	effect = m_Data.ReadByte();
	color = m_Data.ReadLong();

	position[0] = m_Data.ReadFloat();
	position[1] = m_Data.ReadFloat();

	fadein = m_Data.ReadFloat();
	fadeout = m_Data.ReadFloat();
	holdtime = m_Data.ReadFloat();
	fxtime = m_Data.ReadFloat();
	Q_strcpy(text, m_Data.ReadString());

	return true;
}

bool DirectorCmd::GetStatusData(int &slots, int &spectators, int &proxies)
{
	if (m_Type != DRC_CMD_STATUS) {
		return false;
	}

	m_Data.Reset();
	slots = m_Data.ReadLong();
	spectators = m_Data.ReadLong();
	proxies = m_Data.ReadWord();

	return true;
}

bool DirectorCmd::GetBannerData(char *filename)
{
	if (m_Type != DRC_CMD_BANNER) {
		return false;
	}

	m_Data.Reset();
	Q_strcpy(filename, m_Data.ReadString());

	return true;
}

bool DirectorCmd::GetStuffTextData(char *commands)
{
	if (m_Type != DRC_CMD_STUFFTEXT) {
		return false;
	}

	m_Data.Reset();
	Q_strcpy(commands, m_Data.ReadString());

	return true;
}

void DirectorCmd::SetEventData(int entity1, int entity2, int flags)
{
	m_Type = DRC_CMD_EVENT;
	Resize(8);

	m_Data.WriteWord(entity1);
	m_Data.WriteWord(entity2);
	m_Data.WriteLong(flags);
}

void DirectorCmd::SetChaseData(int entity1, int entity2, float distance, int flags)
{
	m_Type = DRC_CMD_CHASE;
	Resize(9);

	m_Data.WriteWord(entity1);
	m_Data.WriteWord(entity2);
	m_Data.WriteFloat(distance);
	m_Data.WriteByte(flags);
}

void DirectorCmd::SetInEyeData(int entity)
{
	m_Type = DRC_CMD_INEYE;
	Resize(2);

	m_Data.WriteWord(entity);
}

void DirectorCmd::SetMapData(int entity, float angle, float distance)
{
	m_Type = DRC_CMD_MAP;
	Resize(10);

	m_Data.WriteWord(entity);
	m_Data.WriteFloat(angle);
	m_Data.WriteFloat(distance);
}

void DirectorCmd::SetStartData()
{
	m_Type = DRC_CMD_START;
	Resize(0);
}

void DirectorCmd::SetModeData(int mode)
{
	m_Type = DRC_CMD_MODE;
	Resize(1);

	m_Data.WriteByte(mode);
}

void DirectorCmd::SetCameraData(vec_t *position, vec_t *angles, float fov, int entity)
{
	m_Type = DRC_CMD_CAMERA;
	Resize(15);

	m_Data.WriteCoord(position[0]);
	m_Data.WriteCoord(position[1]);
	m_Data.WriteCoord(position[2]);
	m_Data.WriteCoord(angles[0]);
	m_Data.WriteCoord(angles[1]);
	m_Data.WriteCoord(angles[2]);
	m_Data.WriteByte((int)fov);
	m_Data.WriteWord(entity);
}

void DirectorCmd::SetCamPathData(vec_t *position, vec_t *angles, float fov, int flags)
{
	m_Type = DRC_CMD_CAMPATH;
	Resize(14);

	m_Data.WriteCoord(position[0]);
	m_Data.WriteCoord(position[1]);
	m_Data.WriteCoord(position[2]);
	m_Data.WriteCoord(angles[0]);
	m_Data.WriteCoord(angles[1]);
	m_Data.WriteCoord(angles[2]);
	m_Data.WriteByte((int)fov);
	m_Data.WriteByte(flags);
}

void DirectorCmd::SetSoundData(char *name, float volume)
{
	int len = Q_strlen(name);
	m_Type = DRC_CMD_SOUND;
	Resize(len + 5);

	m_Data.WriteString(name);
	m_Data.WriteFloat(volume);
}

void DirectorCmd::SetTimeScaleData(float factor)
{
	m_Type = DRC_CMD_TIMESCALE;

	Resize(sizeof(float));
	m_Data.WriteFloat(factor);
}

void DirectorCmd::SetTime(float time)
{
	m_Time = time;
}

void DirectorCmd::SetMessageData(int effect, unsigned int color, vec_t *position, float fadein, float fadeout, float holdtime, float fxtime, char *text)
{
	int len = Q_strlen(text);
	m_Type = DRC_CMD_MESSAGE;
	Resize(len + 30);

	m_Data.WriteByte(effect);
	m_Data.WriteLong(color);
	m_Data.WriteFloat(position[0]);
	m_Data.WriteFloat(position[1]);
	m_Data.WriteFloat(fadein);
	m_Data.WriteFloat(fadeout);
	m_Data.WriteFloat(holdtime);
	m_Data.WriteFloat(fxtime);
	m_Data.WriteString(text);
}

void DirectorCmd::Copy(DirectorCmd *cmd)
{
	Clear();

	m_Time = cmd->m_Time;
	m_Type = cmd->m_Type;
	m_Size = cmd->m_Size;
	m_Index = cmd->m_Index;

	m_Data.Resize(m_Size);
	m_Data.WriteBuf(cmd->m_Data.GetData(), m_Size);
}

void DirectorCmd::SetStatusData(int slots, int spectators, int proxies)
{
	m_Type = DRC_CMD_STATUS;
	Resize(10);

	m_Data.WriteLong(slots);
	m_Data.WriteLong(spectators);
	m_Data.WriteWord(proxies);
}

void DirectorCmd::SetBannerData(char *filename)
{
	m_Type = DRC_CMD_BANNER;

	int len = Q_strlen(filename);
	Resize(len + 1);
	m_Data.WriteString(filename);
}

void DirectorCmd::SetStuffTextData(char *commands)
{
	m_Type = DRC_CMD_STUFFTEXT;

	int len = Q_strlen(commands);
	Resize(len + 1);
	m_Data.WriteString(commands);
}

void DirectorCmd::SetWayPoints(int number)
{
	m_Type = DRC_CMD_WAYPOINTS;

	Resize(1);
	m_Data.WriteByte(number);
}

bool DirectorCmd::ReadFromStream(BitBuffer *stream)
{
	char *string;
	unsigned char *start;
	int length;

	if (!stream) {
		return false;
	}

	Clear();
	m_Type = stream->ReadByte();

	switch (m_Type)
	{
	case DRC_CMD_START:
		Resize(0);
		break;
	case DRC_CMD_EVENT:
		Resize(8);
		m_Data.WriteBuf(stream, 8);
		break;
	case DRC_CMD_MODE:
		Resize(1);
		m_Data.WriteBuf(stream, 1);
		break;
	case DRC_CMD_CAMERA:
		Resize(15);
		m_Data.WriteBuf(stream, 15);
		break;
	case DRC_CMD_TIMESCALE:
		Resize(4);
		m_Data.WriteBuf(stream, 4);
		break;
	case DRC_CMD_MESSAGE:
	{
		start = stream->m_CurByte;
		stream->SkipBytes(29);

		string = stream->ReadString();
		length = Q_strlen(string);
		Resize(length + 30);

		m_Data.WriteBuf(start, 29);
		m_Data.WriteBuf(string, length + 1);
		break;
	}
	case DRC_CMD_SOUND:
	{
		string = stream->ReadString();
		length = Q_strlen(string);
		Resize(length + 5);

		m_Data.WriteBuf(string, length + 1);
		m_Data.WriteFloat(stream->ReadFloat());

		break;
	}
	case DRC_CMD_STATUS:
		Resize(10);
		m_Data.WriteBuf(stream, 10);
		break;
	case DRC_CMD_BANNER:
	{
		string = stream->ReadString();
		length = Q_strlen(string) + 1;

		Resize(length);
		m_Data.WriteBuf(string, length);
		break;
	}
	case DRC_CMD_STUFFTEXT:
	{
		string = stream->ReadString();
		length = Q_strlen(string) + 1;

		Resize(length);
		m_Data.WriteBuf(string, length);
		break;
	}
	case DRC_CMD_CHASE:
		Resize(7);
		m_Data.WriteBuf(stream, 7);
		break;
	case DRC_CMD_INEYE:
		Resize(1);
		m_Data.WriteBuf(stream, 1);
		break;
	case DRC_CMD_MAP:
		Resize(10);
		m_Data.WriteBuf(stream, 15);
		break;
	case DRC_CMD_CAMPATH:
		Resize(14);
		m_Data.WriteBuf(stream, 14);
		break;
	case DRC_CMD_WAYPOINTS:
	{
		Resize(1);
		length = stream->ReadByte();
		if (length < 0) {
			return false;
		}
		m_Data.WriteByte(length);
		stream->SkipBytes(length * 14);
		break;
	}
	default:
	case DRC_CMD_NONE:
		return false;
	}

	return true;
}

void DirectorCmd::WriteToStream(BitBuffer *stream)
{
	if (!stream) {
		return;
	}

	if ((m_Type > DRC_CMD_NONE && m_Type <= DRC_CMD_LAST) && m_Size < 255)
	{
		stream->WriteByte(svc_director);
		stream->WriteByte(m_Size + 1);
		stream->WriteByte(m_Type);
		stream->WriteBuf(m_Data.GetData(), m_Size);
	}
}

const char *DirectorCmd::ToString()
{
	int i1, i2, i3;
	float f1, f2, f3, f4;
	vec3_t v1, v2;

	char *t1 = m_CMD_Name[m_Type];
	char t2[1024];

	static char s[2048];
	Q_memset(s, 0, sizeof(s));

	switch (m_Type)
	{
	case DRC_CMD_START:
		Q_sprintf(s, "%s", t1);
		break;
	case DRC_CMD_EVENT:
		GetEventData(i1, i2, i3);
		Q_sprintf(s, "%s %i %i %i", t1, i1, i2, i3);
		break;
	case DRC_CMD_MODE:
		GetModeData(i1);
		Q_sprintf(s, "%s %i", t1, i1);
		break;
	case DRC_CMD_CAMERA:
		GetCameraData(v1, v2, f1, i1);
		Q_sprintf(s, "%s (%.1f %.1f %.1f) (%.1f %.1f %.1f) %.1f %i", t1, v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], f1, i1);
		break;
	case DRC_CMD_TIMESCALE:
		GetTimeScaleData(f1);
		Q_sprintf(s, "%s %.2f", t1, f1);
		break;
	case DRC_CMD_MESSAGE:
		GetMessageData(i1, i2, v1, f1, f2, f3, f4, t2);
		Q_sprintf(s, "%s \"%s\" %i %x (%.2f %.2f) %.1f, %.1f %.1f %.1f", t1, t2, i1, i2, v1[0], v1[1], f1, f2, f3, f4);
		break;
	case DRC_CMD_SOUND:
		GetSoundData(t2, f1);
		Q_sprintf(s, "%s \"%s\" %.2f", t1, t2, f1);
		break;
	case DRC_CMD_STATUS:
		GetStatusData(i1, i2, i3);
		Q_sprintf(s, "%s %i %i %i", t1, i1, i2, i3);
		break;
	case DRC_CMD_BANNER:
		GetBannerData(t2);
		Q_sprintf(s, "%s \"%s\"", t1, t2);
		break;
	case DRC_CMD_STUFFTEXT:
		GetStuffTextData(t2);
		Q_sprintf(s, "%s \"%s\"", t1, t2);
		break;
	case DRC_CMD_CHASE:
		GetChaseData(i1, i2, f1, i3);
		Q_sprintf(s, "%s %i %i %.1f %i", t1, i1, i2, f1, i3);
		break;
	case DRC_CMD_INEYE:
		GetInEyeData(i1);
		Q_sprintf(s, "%s %i", t1, i1);
		break;
	case DRC_CMD_MAP:
		GetMapData(i1, f1, f2);
		Q_sprintf(s, "%s %i %.1f %.1f", t1, i1, f1, f2);
		break;
	case DRC_CMD_CAMPATH:
		GetCamPathData(v1, v2, f1, i1);
		Q_sprintf(s, "%s (%.1f %.1f %.1f) (%.1f %.1f %.1f) %.1f %i", t1, v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], f1, i1);
		break;
	case DRC_CMD_WAYPOINTS:
		GetWayPointsData(i1);
		Q_sprintf(s, "%s %i", t1, i1);
		break;
	default:
	case DRC_CMD_NONE:
		return nullptr;
	}

	return s;
}

void DirectorCmd::FromString(char *string)
{
	;
}

void DirectorCmd::Clear()
{
	m_Type = 0;
	m_Time = 0;

	m_Data.Free();
}

void DirectorCmd::Resize(int size)
{
	m_Data.Resize(size);
	m_Size = size;
}
