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

const uint32 BITTABLE[] =
{
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000,
	0x00000000,
};

const uint32 ROWBITTABLE[] =
{
	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000F, 0x0000001F, 0x0000003F, 0x0000007F,
	0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF,
	0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF,
	0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF,
	0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
	0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF,
	0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF,
	0xFFFFFFFF,
};

const uint32 INVBITTABLE[] =
{
	0xFFFFFFFE, 0xFFFFFFFD, 0xFFFFFFFB, 0xFFFFFFF7,
	0xFFFFFFEF, 0xFFFFFFDF, 0xFFFFFFBF, 0xFFFFFF7F,
	0xFFFFFEFF, 0xFFFFFDFF, 0xFFFFFBFF, 0xFFFFF7FF,
	0xFFFFEFFF, 0xFFFFDFFF, 0xFFFFBFFF, 0xFFFF7FFF,
	0xFFFEFFFF, 0xFFFDFFFF, 0xFFFBFFFF, 0xFFF7FFFF,
	0xFFEFFFFF, 0xFFDFFFFF, 0xFFBFFFFF, 0xFF7FFFFF,
	0xFEFFFFFF, 0xFDFFFFFF, 0xFBFFFFFF, 0xF7FFFFFF,
	0xEFFFFFFF, 0xDFFFFFFF, 0xBFFFFFFF, 0x7FFFFFFF,
	0xFFFFFFFF,
};

BitBuffer::BitBuffer() :
	m_Overflowed(false),
	m_Data(nullptr),
	m_CurByte(nullptr),
	m_CurBit(0),
	m_MaxSize(0),
	m_LittleEndian(false),
	m_OwnData(false)
{
}

BitBuffer::BitBuffer(void *newData, unsigned int size)
{
	m_Data = (unsigned char *)newData;
	m_CurByte = m_Data;

	m_CurBit = 0;
	m_MaxSize = size;
	m_Overflowed = false;
	m_LittleEndian = true;
	m_OwnData = false;
}

BitBuffer::~BitBuffer()
{
	Free();
}

BitBuffer::BitBuffer(unsigned int size)
{
	m_Data = nullptr;
	m_CurByte = nullptr;

	m_CurBit = 0;
	m_MaxSize = size;
	m_Overflowed = false;
	m_LittleEndian = false;
	m_OwnData = false;

	Resize(size);
}

bool BitBuffer::Resize(unsigned int size)
{
	Free();

	m_Data = (unsigned char *)Mem_ZeroMalloc(size + 4);
	m_CurBit = 0;
	m_Overflowed = false;

	if (m_Data)
	{
		m_CurByte = m_Data;
		m_MaxSize = size;

		m_OwnData = true;
		m_LittleEndian = true;
		return true;
	}

	m_MaxSize = 0;
	m_OwnData = false;
	m_CurByte = nullptr;

	return false;
}

void BitBuffer::Clear()
{
	Q_memset(m_Data, 0, m_MaxSize);

	m_CurByte = m_Data;
	m_CurBit = 0;

	m_Overflowed = false;
	m_LittleEndian = true;
}

int BitBuffer::CurrentBit()
{
	return m_CurBit + 8 * (m_CurByte - m_Data);
}

void BitBuffer::Reset()
{
	m_CurByte = m_Data;
	m_CurBit = 0;

	m_Overflowed = false;
	m_LittleEndian = true;
}

void BitBuffer::Free()
{
	if (m_Data && m_OwnData) {
		Mem_Free(m_Data);
	}

	m_Data = nullptr;
	m_CurByte = nullptr;

	m_CurBit = 0;
	m_MaxSize = 0;

	m_OwnData = false;
	m_Overflowed = false;
	m_LittleEndian = true;
}

unsigned int BitBuffer::ReadBits(int numbits)
{
	unsigned int result = 0;
	if (m_LittleEndian)
	{
		if (m_CurByte - m_Data >= m_MaxSize)
		{
			m_Overflowed = true;
			return -1;
		}

		int bits = m_CurBit + numbits;
		if (bits <= 32)
		{
			result = (*(unsigned int *)m_CurByte >> m_CurBit) & ROWBITTABLE[numbits];

			m_CurByte += numbits >> 3;
			m_CurBit += numbits & 7;

			if (m_CurBit > 7)
			{
				m_CurBit &= 7;
				m_CurByte++;
			}
		}
		else
		{
			unsigned int data = *(unsigned int *)m_CurByte >> m_CurBit;
			m_CurByte += 4;
			result = ((ROWBITTABLE[bits & 7] & *(unsigned int *)m_CurByte) << (32 - m_CurBit)) | data;
			m_CurBit = bits & 7;
		}
	}
	else
	{
		int d = numbits;
		while (d > 0)
		{
			--d;
			if (ReadBit()) {
				result |= (1 << d);
			}
		}
	}

	return result;
}

int BitBuffer::ReadBit()
{
	int result;
	if (m_CurByte - m_Data >= m_MaxSize) {
		m_Overflowed = true;
		result = -1;
	}
	else
	{
		if (m_LittleEndian)
		{
			if (m_CurBit == 7)
			{
				m_CurBit = 0;
				result = (*m_CurByte++ >> 7) & 1;
			}
			else
			{
				result = ((unsigned int)*m_CurByte >> m_CurBit++) & 1;
			}
		}
		else
		{
			if (m_CurBit == 7)
			{
				m_CurBit = 0;
				result = *m_CurByte++ & 1;
			}
			else
			{
				result = ((unsigned int)*m_CurByte >> (7 - m_CurBit++)) & 1;
			}
		}
	}

	return result;
}

unsigned int BitBuffer::PeekBits(int numbits)
{
	int oldcurrentBit = m_CurBit;
	unsigned char *oldcurrentByte = m_CurByte;
	unsigned int data = ReadBits(numbits);

	m_CurBit = oldcurrentBit;
	m_CurByte = oldcurrentByte;
	return data;
}

int BitBuffer::ReadChar()
{
	return ReadBits(8);
}

int BitBuffer::ReadByte()
{
	return ReadBits(8);
}

int BitBuffer::ReadShort()
{
	return ReadBits(16);
}

int BitBuffer::ReadWord()
{
	return ReadBits(16);
}

unsigned int BitBuffer::ReadLong()
{
	return ReadBits(32);
}

float BitBuffer::ReadFloat()
{
	union {
		float f;
		int i;
	} dat;

	dat.i = _LittleLong(ReadLong());
	return dat.f;
}

bool BitBuffer::ReadBuf(int iSize, void *pbuf)
{
	if (m_CurByte - m_Data + iSize > m_MaxSize) {
		m_Overflowed = true;
		return false;
	}

	if (m_CurBit)
	{
		int i, j;
		unsigned int *p = (unsigned int *)pbuf;
		for (i = 4; i < iSize; i += 4) {
			*p++ = ReadLong();
		}

		for (j = 0; j < iSize - (i - 4); ++j) {
			*((unsigned char *)p + j) = ReadByte();
		}
	}
	else
	{
		Q_memcpy(pbuf, m_CurByte, iSize);
		m_CurByte += iSize;
	}

	return true;
}

void BitBuffer::WriteBuf(BitBuffer *buf, int length)
{
	WriteBuf(buf->m_CurByte, length);
	buf->SkipBytes(length);
}

char *BitBuffer::ReadString()
{
	int c = 0;
	unsigned int l = 0;
	static char string[8192];

	while ((c = ReadChar(), c) && c != -1 && l < sizeof(string) - 1) {
		string[l++] = c;
	}

	string[l] = '\0';
	return string;
}

char *BitBuffer::ReadStringLine()
{
	int c = 0;
	unsigned int l = 0;
	static char string[2048];

	while ((c = ReadChar(), c) && c != '\n' && c != -1 && l < sizeof(string) - 1) {
		string[l++] = c;
	}

	string[l] = '\0';
	return string;
}

float BitBuffer::ReadAngle()
{
	int c = ReadChar();
	if (c == -1)
	{
		// FIXED: Added check for wrong value, but just return 0 instead of -1 * (360.0 / 256)
		return 0;
	}

	return (float)(c * (360.0 / 256));
}

float BitBuffer::ReadHiresAngle()
{
	int c = ReadShort();
	if (c == -1)
	{
		// FIXED: Added check for wrong value, but just return 0 instead of -1 * (360.0 / 65536)
		return 0;
	}

	return (float)(c * (360.0 / 65536));
}

void BitBuffer::WriteBit(int c)
{
	if (m_CurByte - m_Data >= m_MaxSize) {
		m_Overflowed = true;
		return;
	}

	if (m_LittleEndian)
	{
		if (m_CurBit == 7)
		{
			if (c)
			{
				m_CurByte[0] |= 0x80u;
			}
			else
			{
				m_CurByte[0] &= 0x7Fu;
			}

			m_CurByte++;
			m_CurBit = 0;
		}
		else
		{
			if (c)
			{
				m_CurByte[0] |= BITTABLE[ m_CurBit ];
			}
			else
			{
				m_CurByte[0] &= INVBITTABLE[ m_CurBit ];
			}

			m_CurBit++;
		}
	}
	else
	{
		static unsigned char masks[] = { 0x80u, 0x40u, 0x20u, 0x10u, 0x8u, 0x4u, 0x2u, 0x1u };
		static unsigned char inv_masks[] = { 0x7Fu, 0xBFu, 0xDFu, 0xEFu, 0xF7u, 0xFBu, 0xFDu, 0xFEu };

		if (c)
			m_CurByte[0] |= masks[ m_CurBit ];
		else
			m_CurByte[0] &= inv_masks[ m_CurBit ];

		if (++m_CurBit == 8)
		{
			m_CurBit = 0;
			m_CurByte++;
		}
	}
}

void BitBuffer::WriteBits(unsigned int data, int numbits)
{
	if (m_Overflowed) {
		return;
	}

	if (m_LittleEndian)
	{
		if (m_CurByte - m_Data + (numbits >> 8) > m_MaxSize) {
			m_Overflowed = true;
			return;
		}

		int bits = numbits + m_CurBit;
		if (bits <= 32)
		{
			*(uint32 *)m_CurByte |= (ROWBITTABLE[numbits] & data) << m_CurBit;

			m_CurByte = &m_CurByte[numbits >> 3];
			m_CurBit = m_CurBit + (numbits & 7);

			if (m_CurBit > 7)
			{
				m_CurBit = m_CurBit & 7;
				m_CurByte = m_CurByte + 1;
			}
		}
		else
		{
			*(uint32 *)m_CurByte |= (ROWBITTABLE[numbits] & data) << m_CurBit;

			int leftBits = (32 - m_CurBit);
			m_CurBit = (m_CurBit + numbits) & 7;

			m_CurByte += 4;
			*(uint32 *)m_CurByte |= (ROWBITTABLE[numbits] & data) >> leftBits;
		}

		return;
	}

	int nBitValue = data;
	if (numbits <= 31 && nBitValue >= (1 << numbits) && nBitValue != -1)
		nBitValue = (1 << numbits) - 1;

	while (--numbits > 0)
	{
		if (m_CurByte - m_Data >= m_MaxSize) {
			m_Overflowed = true;
			break;
		}

		WriteBit(nBitValue & (1 << numbits));
	}
}

void BitBuffer::WriteSBits(int data, int numbits)
{
	int idata = data;
	if (numbits < 32)
	{
		int maxnum = (1 << (numbits - 1)) - 1;
		if (data > maxnum || (maxnum = -maxnum, data < maxnum))
		{
			idata = maxnum;
		}
	}

	int sigbits = idata < 0;

	WriteBit(sigbits);
	WriteBits(abs(idata), numbits - 1);
}

void BitBuffer::WriteChar(int c)
{
	WriteBits(c, 8 * sizeof(char));
}

void BitBuffer::WriteByte(int c)
{
	WriteBits(c, 8 * sizeof(uint8));
}

void BitBuffer::WriteShort(int c)
{
	WriteBits(c, 8 * sizeof(int16));
}

void BitBuffer::WriteWord(int c)
{
	WriteBits(c, 8 * sizeof(uint16));
}

void BitBuffer::WriteLong(unsigned int c)
{
	WriteBits(c, 8 * sizeof(uint32));
}

void BitBuffer::WriteFloat(float f)
{
	union {
		float f;
		int i;
	} dat;

	dat.f = f;
	WriteBits(_LittleLong(dat.i), 8 * sizeof(float));
}

void BitBuffer::WriteString(const char *p)
{
	if (p)
	{
		WriteBuf(p, Q_strlen(p) + 1);
	}
	else
	{
		WriteChar('\0');
	}
}

void BitBuffer::WriteBuf(const void *buf, int iSize)
{
	if (!buf || m_Overflowed || !iSize) {
		return;
	}

	if (m_CurByte - m_Data + iSize > m_MaxSize) {
		m_Overflowed = true;
		return;
	}

	if (m_CurBit)
	{
		int i, j;
		unsigned int *p = (unsigned int *)buf;

		for (i = 4; i < iSize; i += 4) {
			WriteLong(*p++);
		}

		for (j = 0; j < (iSize - (i - 4)); j++) {
			WriteChar(*((unsigned char *)p + j));
		}
	}
	else
	{
		Q_memcpy(m_CurByte, buf, iSize);
		m_CurByte += iSize;
	}
}

void BitBuffer::WriteBitData(void *src, int length)
{
	int i;
	unsigned char *p = (unsigned char *)src;
	for (i = 0; i < length; i++, p++)
	{
		WriteChar(*p);
	}
}

void BitBuffer::WriteAngle(float f)
{
	WriteByte((int64)(fmod((double)f, 360.0) * 256.0 / 360.0) & 0xFF);
}

void BitBuffer::WriteHiresAngle(float f)
{
	WriteShort((int64)(fmod((double)f, 360.0) * 65536.0 / 360.0) & 0xFFFF);
}

int BitBuffer::CurrentSize()
{
	return (m_CurBit != 0) + m_CurByte - m_Data;
}

unsigned char *BitBuffer::CurrentByte()
{
	return m_CurByte;
}

int BitBuffer::SpaceLeft()
{
	return m_MaxSize + m_Data - m_CurByte;
}

void BitBuffer::AlignByte()
{
	if (m_CurBit)
	{
		m_CurByte++;
		m_CurBit = 0;
	}
}

int BitBuffer::ReadSBits(int numbits)
{
	int nSignBit = ReadBit();
	int result = ReadBits(numbits - 1);

	if (nSignBit)
	{
		result = -result;
	}

	return result;
}

float BitBuffer::ReadBitAngle(int numbits)
{
	return (float)(ReadBits(numbits) * (360.0 / (1 << numbits)));
}

void BitBuffer::WriteBitAngle(float fAngle, int numbits)
{
	if (numbits >= 32) {
		m_Overflowed = true;
		return;
	}

	unsigned int shift = (1 << numbits);
	unsigned int mask = shift - 1;

	int d = (int)(shift * fmod((double)fAngle, 360.0)) / 360;
	d &= mask;

	WriteBits(d, numbits);
}

char *BitBuffer::ReadBitString()
{
	static char buf[8192];
	buf[0] = '\0';

	char *p = &buf[0];
	for (unsigned int c = ReadChar(); c; c = ReadChar())
	{
		// Prevent infinite cycle if m_Overflowed
		if (m_Overflowed) {
			break;
		}

		if (p >= &buf[sizeof(buf) - 1]) {
			break;
		}

		*p++ = c;
	}

	*p = '\0';
	return buf;
}

void BitBuffer::WriteBitString(const char *p)
{
	const unsigned char *pch = (unsigned char *)p;
	while (*pch)
	{
		WriteChar(*pch++);
	}

	WriteChar('\0');
}

void BitBuffer::StartBitMode()
{
	if (m_CurBit) {
		m_Overflowed = true;
	}
}

void BitBuffer::EndBitMode()
{
	AlignByte();
}

int BitBuffer::ReadBitData(void *dest, int length)
{
	unsigned char *p = (unsigned char *)dest;
	for (int i = 0; i < length; i++) {
		*p++ = ReadByte();
	}

	return length;
}

void BitBuffer::SetBuffer(void *buffer, int size)
{
	Free();

	m_Data = (unsigned char *)buffer;
	m_CurByte = (unsigned char *)buffer;
	m_MaxSize = size;
	m_CurBit = 0;

	m_OwnData = false;
	m_Overflowed = false;
	m_LittleEndian = true;
}

void BitBuffer::ReadBitVec3Coord(float *fa)
{
	int xflag = ReadBit();
	int yflag = ReadBit();
	int zflag = ReadBit();

	if (xflag)
		fa[0] = ReadBitCoord();
	if (yflag)
		fa[1] = ReadBitCoord();
	if (zflag)
		fa[2] = ReadBitCoord();
}

float BitBuffer::ReadBitCoord()
{
	float value = 0;

	int intval = ReadBit();
	int fractval = ReadBit();

	if (intval || fractval)
	{
		int signbit = ReadBit();
		if (intval) {
			intval = ReadBits(12);
		}

		if (fractval) {
			fractval = ReadBits(3);
		}

		value = (float)(fractval / 8.0 + intval);
		if (signbit) {
			value = -value;
		}
	}

	return value;
}

float BitBuffer::ReadCoord()
{
	return (float)(ReadShort() * (1.0 / 8));
}

void BitBuffer::SkipBytes(int numbits)
{
	if (numbits + m_CurByte - m_Data > m_MaxSize) {
		m_Overflowed = true;
	}

	m_CurByte += numbits;
}

void BitBuffer::SkipBits(int numbits)
{
	if (m_LittleEndian)
	{
		if (m_CurByte - m_Data >= m_MaxSize)
		{
			m_Overflowed = true;
			return;
		}

		int bits = m_CurBit + numbits;
		if (bits <= 32)
		{
			m_CurByte += numbits >> 3;
			m_CurBit += numbits & 7;

			if (m_CurBit > 7)
			{
				m_CurBit &= 7;
				m_CurByte++;
			}
		}
		else
		{
			m_CurByte += 4;
			m_CurBit = bits & 7;
		}
	}
	else
	{
		int d = numbits;
		while (d > 0)
		{
			--d;
			if (m_CurBit == 7)
			{
				m_CurByte++;
				m_CurBit = 0;
			}
			else
			{
				m_CurBit++;
			}
		}
	}
}

int BitBuffer::SkipString()
{
	int c = 0, l = 1;
	const int maxString = 8192;

	while ((c = ReadChar(), c) && c != -1 && l < maxString) {
		l++;
	}

	return l;
}

void BitBuffer::FastClear()
{
	int iSize = CurrentSize() + 4;
	if (iSize > m_MaxSize) {
		iSize = m_MaxSize;
	}

	Q_memset(m_Data, 0, iSize);

	m_CurByte = m_Data;
	m_CurBit = 0;

	m_Overflowed = false;
	m_LittleEndian = true;
}

void BitBuffer::ConcatBuffer(BitBuffer *buffer)
{
	WriteBuf(buffer->m_Data, buffer->CurrentSize());
}

void BitBuffer::WriteCoord(float f)
{
	WriteShort((int)(f * 8.0));
}
