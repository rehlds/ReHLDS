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

char serverinfo[MAX_INFO_STRING];

char gpszVersionString[32];
char gpszProductString[32];

char *Info_Serverinfo(void)
{
	return serverinfo;
}

#ifndef COM_Functions_region

unsigned char COM_Nibble(char c)
{
	if (c >= '0' && c <= '9')
	{
		return (unsigned char)(c - '0');
	}

	if (c >= 'A' && c <= 'F')
	{
		return (unsigned char)(c - 'A' + 0x0A);
	}

	if (c >= 'a' && c <= 'f')
	{
		return (unsigned char)(c - 'a' + 0x0A);
	}

	return '0';
}

void COM_HexConvert(const char *pszInput, int nInputLength, unsigned char *pOutput)
{
	unsigned char *p;
	int i;
	const char *pIn;

	p = pOutput;
	for (i = 0; i < nInputLength - 1; i += 2)
	{
		pIn = &pszInput[i];
		if (pIn[0] == 0 || pIn[1] == 0)
			break;

		*p = COM_Nibble(pIn[0]) << 4 | COM_Nibble(pIn[1]);

		p++;
	}
}

NOXREF char *COM_BinPrintf(unsigned char *buf, int nLen)
{
	NOXREFCHECK;
	static char szReturn[4096];
	unsigned char c;
	char szChunk[10];
	int i;

	Q_memset(szReturn, 0, sizeof(szReturn));

	for (i = 0; i < nLen; i++)
	{
		c = (unsigned char)buf[i];

		Q_snprintf(szChunk, sizeof(szChunk), "%02x", c);
		Q_strncat(szReturn, szChunk, sizeof(szReturn) - Q_strlen(szReturn) - 1);
	}
	return szReturn;
}

void COM_ExplainDisconnection(qboolean bPrint, char *fmt, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	Q_strncpy(gszDisconnectReason, string, sizeof(gszDisconnectReason) - 1);
	gszDisconnectReason[sizeof(gszDisconnectReason) - 1] = 0;
	gfExtendedError = 1;
	if (bPrint)
	{
		if (gszDisconnectReason[0] != '#')
			Con_Printf("%s\n", gszDisconnectReason);
	}
}

NOXREF void COM_ExtendedExplainDisconnection(qboolean bPrint, char *fmt, ...)
{
	NOXREFCHECK;

	va_list argptr;
	static char string[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	Q_strncpy(gszExtendedDisconnectReason, string, sizeof(gszExtendedDisconnectReason) - 1);
	gszExtendedDisconnectReason[sizeof(gszExtendedDisconnectReason) - 1] = 0;
	if (bPrint)
	{
		if (gszExtendedDisconnectReason[0] != '#')
			Con_Printf("%s\n", gszExtendedDisconnectReason);
	}
}

#endif // COM_Functions_region


#ifndef Byte_Functions_region

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

qboolean bigendien;
short (*BigShort)(short l);
short (*LittleShort)(short l);
int (*BigLong)(int l);
int (*LittleLong)(int l);
float (*BigFloat)(float l);
float (*LittleFloat)(float l);

int LongSwap(int l)
{
	return bswap(l);
}

int LongNoSwap(int l)
{
	return l;
}

short ShortSwap(short l)
{
	return bswap(l);
}

short ShortNoSwap(short l)
{
	return l;
}

float FloatSwap(float f)
{
	/*union
	{
		float f;
		byte b[4];
	} dat1, dat2;

	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];

	return dat2.f;*/
	//unsigned long u = bswap(*(unsigned long *)&f);
	//return *(float *)&u;
	return bswap(f);
}

float FloatNoSwap(float f)
{
	return f;
}

#endif // Byte_Functions_region

#ifndef MSG_Functions_region

// MESSAGE IO FUNCTIONS
// Handles byte ordering and avoids alignment errors

int msg_badread;
int msg_readcount;

// Some bit tables...
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

void MSG_WriteChar(sizebuf_t *sb, int c)
{
	unsigned char *buf = (unsigned char *)SZ_GetSpace(sb, 1);
	*(char *)buf = (char)c;
}

void MSG_WriteByte(sizebuf_t *sb, int c)
{
	unsigned char *buf = (unsigned char *)SZ_GetSpace(sb, 1);
	*(byte *)buf = (byte)c;
}

void MSG_WriteShort(sizebuf_t *sb, int c)
{
	unsigned char *buf = (unsigned char *)SZ_GetSpace(sb, 2);
	*(int16 *)buf = (int16)c;
}

void MSG_WriteWord(sizebuf_t *sb, int c)
{
	unsigned char *buf = (unsigned char *)SZ_GetSpace(sb, 2);
	*(uint16 *)buf = (uint16)c;
}

void MSG_WriteLong(sizebuf_t *sb, int c)
{
	unsigned char *buf = (unsigned char *)SZ_GetSpace(sb, 4);
	*(uint32 *)buf = (uint32)c;
}

void MSG_WriteFloat(sizebuf_t *sb, float f)
{
	int i = LittleLong(*(int *)&f);
	SZ_Write(sb, &i, 4);
}

void MSG_WriteString(sizebuf_t *sb, const char *s)
{
	if (s)
	{
		SZ_Write(sb, s, Q_strlen(s) + 1);
	}
	else
	{
		SZ_Write(sb, "", 1);
	}
}

void MSG_WriteBuf(sizebuf_t *sb, int iSize, void *buf)
{
	if (buf)
	{
		SZ_Write(sb, buf, iSize);
	}
}

void MSG_WriteAngle(sizebuf_t *sb, float f)
{
	MSG_WriteByte(sb, (int64)(fmod((double)f, 360.0) * 256.0 / 360.0) & 0xFF);
}

void MSG_WriteHiresAngle(sizebuf_t *sb, float f)
{
	MSG_WriteShort(sb, (int64)(fmod((double)f, 360.0) * 65536.0 / 360.0) & 0xFFFF);
}

void MSG_WriteUsercmd(sizebuf_t *buf, usercmd_t *to, usercmd_t *from)
{
	delta_t **ppdesc;

	ppdesc = DELTA_LookupRegistration("usercmd_t");
	MSG_StartBitWriting(buf);
	DELTA_WriteDelta((byte *)from, (byte *)to, 1, *ppdesc, 0);
	MSG_EndBitWriting(buf);
}

typedef struct bf_write_s
{
	// For enhanced and safe bits writing functions
#if defined(REHLDS_FIXES)

#pragma pack(push, 1)
	union {
		uint64 u64;
		uint32 u32[2];
		uint8 u8[8];
	} pendingData;
	uint64 sse_highbits;
#pragma pack(pop)

	int nCurOutputBit;
	sizebuf_t *pbuf;

#else // defined(REHLDS_FIXES)

	int nCurOutputBit;
	unsigned char *pOutByte;
	sizebuf_t *pbuf;

#endif // defined(REHLDS_FIXES)
} bf_write_t;

typedef struct bf_read_s
{
	int nMsgReadCount;	// was msg_readcount
	sizebuf_t *pbuf;
	int nBitFieldReadStartByte;
	int nBytesRead;
	int nCurInputBit;
	unsigned char *pInByte;
} bf_read_t;

// Bit field reading/writing storage.
bf_read_t bfread;
ALIGN16 bf_write_t bfwrite;


void COM_BitOpsInit(void)
{
	Q_memset(&bfwrite, 0, sizeof(bf_write_t));
	Q_memset(&bfread, 0, sizeof(bf_read_t));
}

// Enhanced and safe bits writing functions
#if defined(REHLDS_FIXES)

void MSG_WBits_MaybeFlush() {
	if (bfwrite.nCurOutputBit < 32)
		return;

	uint32* pDest = (uint32*)SZ_GetSpace(bfwrite.pbuf, 4);
	if (!(bfwrite.pbuf->flags & SIZEBUF_OVERFLOWED))
		*pDest = bfwrite.pendingData.u32[0];

	bfwrite.pendingData.u32[0] = bfwrite.pendingData.u32[1];
	bfwrite.pendingData.u32[1] = 0;
	bfwrite.nCurOutputBit -= 32;
}

void MSG_WriteBits(uint32 data, int numbits)
{
	uint32 maxval = _mm_cvtsi128_si32(_mm_slli_epi64(_mm_cvtsi32_si128(1), numbits)) - 1; //maxval = (1 << numbits) - 1
	if (data > maxval)
		data = maxval;

	MSG_WBits_MaybeFlush();

	__m128i pending = _mm_load_si128((__m128i*) &bfwrite.pendingData.u64);

	__m128i mmdata = _mm_slli_epi64(_mm_cvtsi32_si128(data), bfwrite.nCurOutputBit); //mmdata = data << bfwrite.nCurOutputBit
	pending = _mm_or_si128(pending, mmdata);

	_mm_store_si128((__m128i*) &bfwrite.pendingData.u64, pending);
	bfwrite.nCurOutputBit += numbits;
}

void MSG_WriteOneBit(int nValue) {
	MSG_WriteBits(nValue, 1);
}

void MSG_StartBitWriting(sizebuf_t *buf)
{
	bfwrite.nCurOutputBit = 0;
	bfwrite.pbuf = buf;
	bfwrite.pendingData.u64 = 0;
}

void MSG_EndBitWriting(sizebuf_t *buf)
{
	int bytesNeed = bfwrite.nCurOutputBit / 8;
	if ((bfwrite.nCurOutputBit % 8) || bytesNeed == 0) {
		bytesNeed++;
	}

	uint8* pData = (uint8*)SZ_GetSpace(bfwrite.pbuf, bytesNeed);
	if (!(bfwrite.pbuf->flags & SIZEBUF_OVERFLOWED)) {
		for (int i = 0; i < bytesNeed; i++) {
			pData[i] = bfwrite.pendingData.u8[i];
		}
	}

}

#else // defined(REHLDS_FIXES)

void MSG_WriteOneBit(int nValue)
{
	if (bfwrite.nCurOutputBit >= 8)
	{
		SZ_GetSpace(bfwrite.pbuf, 1);
		bfwrite.nCurOutputBit = 0;
		++bfwrite.pOutByte;
	}

	if (!(bfwrite.pbuf->flags & SIZEBUF_OVERFLOWED))
	{
		if (nValue)
		{
			*bfwrite.pOutByte |= BITTABLE[bfwrite.nCurOutputBit];
		}
		else
		{
			*bfwrite.pOutByte &= INVBITTABLE[bfwrite.nCurOutputBit * 4];
		}

		bfwrite.nCurOutputBit++;
	}
}

void MSG_StartBitWriting(sizebuf_t *buf)
{
	bfwrite.nCurOutputBit = 0;
	bfwrite.pbuf = buf;
	bfwrite.pOutByte = &buf->data[buf->cursize];
}

void MSG_EndBitWriting(sizebuf_t *buf)
{
	if (!(bfwrite.pbuf->flags & SIZEBUF_OVERFLOWED))
	{
		*bfwrite.pOutByte &= 255 >> (8 - bfwrite.nCurOutputBit);
		SZ_GetSpace(bfwrite.pbuf, 1);
		bfwrite.nCurOutputBit = 0;
		bfwrite.pOutByte = 0;
		bfwrite.pbuf = 0;
	}
}

void MSG_WriteBits(uint32 data, int numbits)
{
	if (numbits < 32)
	{
		if (data >= (uint32)(1 << numbits))
			data = ROWBITTABLE[numbits];
	}

	int surplusBytes = 0;
	if ((uint32)bfwrite.nCurOutputBit >= 8)
	{
		surplusBytes = 1;
		bfwrite.nCurOutputBit = 0;
		++bfwrite.pOutByte;
	}

	int bits = numbits + bfwrite.nCurOutputBit;
	if (bits <= 32)
	{
		int bytesToWrite = bits >> 3;
		int bitsLeft = bits & 7;
		if (!bitsLeft)
			--bytesToWrite;
		SZ_GetSpace(bfwrite.pbuf, surplusBytes + bytesToWrite);
		if (!(bfwrite.pbuf->flags & SIZEBUF_OVERFLOWED))
		{
			*(uint32 *)bfwrite.pOutByte = (data << bfwrite.nCurOutputBit) | *(uint32 *)bfwrite.pOutByte & ROWBITTABLE[bfwrite.nCurOutputBit];
			bfwrite.nCurOutputBit = 8;
			if (bitsLeft)
				bfwrite.nCurOutputBit = bitsLeft;
			bfwrite.pOutByte = &bfwrite.pOutByte[bytesToWrite];
		}
	}
	else
	{
		SZ_GetSpace(bfwrite.pbuf, surplusBytes + 4);
		if (!(bfwrite.pbuf->flags & SIZEBUF_OVERFLOWED))
		{
			*(uint32 *)bfwrite.pOutByte = (data << bfwrite.nCurOutputBit) | *(uint32 *)bfwrite.pOutByte & ROWBITTABLE[bfwrite.nCurOutputBit];
			int leftBits = 32 - bfwrite.nCurOutputBit;
			bfwrite.nCurOutputBit = bits & 7;
			bfwrite.pOutByte += 4;
			*(uint32 *)bfwrite.pOutByte = data >> leftBits;
		}
	}
}

#endif //defined(REHLDS_FIXES)

qboolean MSG_IsBitWriting(void)
{
	return bfwrite.pbuf != 0;
}

void MSG_WriteSBits(int data, int numbits)
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

	MSG_WriteOneBit(sigbits);
	MSG_WriteBits(abs(idata), numbits - 1);
}

void MSG_WriteBitString(const char *p)
{
#ifdef REHLDS_FIXES
	const uint8_t *pch = (uint8_t *)p;
#else // REHLDS_FIXES
	char *pch = (char *)p;
#endif // REHLDS_FIXES

	while (*pch)
	{
		MSG_WriteBits(*pch, 8);
		++pch;
	}

	MSG_WriteBits(0, 8);
}

void MSG_WriteBitData(void *src, int length)
{
	int i;
	byte *p = (byte *)src;

	for (i = 0; i < length; i++, p++)
	{
		MSG_WriteBits(*p, 8);
	}
}

void MSG_WriteBitAngle(float fAngle, int numbits)
{
	if (numbits > 22)
	{
		Sys_Error("%s: Can't write bit angle with more than 22 bits precision\n", __func__);
	}

	uint32 shift = (1 << numbits);
	uint32 mask = shift - 1;

	int d = (int)(shift * fmod((double)fAngle, 360.0)) / 360;
	d &= mask;

	MSG_WriteBits(d, numbits);
}

float MSG_ReadBitAngle(int numbits)
{
	return (float)(MSG_ReadBits(numbits) * (360.0 / (1 << numbits)));
}

int MSG_CurrentBit(void)
{
	int nbits;

	if (bfread.pbuf)
	{
		nbits = bfread.nCurInputBit + 8 * bfread.nBytesRead;
	}
	else
	{
		nbits = 8 * msg_readcount;
	}
	return nbits;
}

qboolean MSG_IsBitReading(void)
{
	return bfread.pbuf != 0;
}

void MSG_StartBitReading(sizebuf_t *buf)
{
	bfread.nCurInputBit = 0;
	bfread.nBytesRead = 0;
	bfread.nBitFieldReadStartByte = msg_readcount;
	bfread.pbuf = buf;
	bfread.pInByte = &buf->data[msg_readcount];
	bfread.nMsgReadCount = msg_readcount + 1;

	if (msg_readcount + 1 > buf->cursize)
	{
		msg_badread = 1;
	}
}

void MSG_EndBitReading(sizebuf_t *buf)
{
	if (bfread.nMsgReadCount > buf->cursize)
	{
		msg_badread = 1;
	}

	msg_readcount = bfread.nMsgReadCount;
	bfread.nBitFieldReadStartByte = 0;
	bfread.nCurInputBit = 0;
	bfread.nBytesRead = 0;
	bfread.pInByte = 0;
	bfread.pbuf = 0;
}

int MSG_ReadOneBit(void)
{
	int nValue;

	if (msg_badread)
	{
		nValue = 1;
	}
	else
	{
		if (bfread.nCurInputBit >= 8)
		{
			++bfread.nMsgReadCount;
			bfread.nCurInputBit = 0;
			++bfread.nBytesRead;
			++bfread.pInByte;
		}

		if (bfread.nMsgReadCount <= bfread.pbuf->cursize)
		{
			nValue = (*bfread.pInByte & BITTABLE[bfread.nCurInputBit]) != 0;
			++bfread.nCurInputBit;
		}
		else
		{
			nValue = 1;
			msg_badread = 1;
		}
	}

	return nValue;
}

uint32 MSG_ReadBits(int numbits)
{
	uint32 result;

#ifdef REHLDS_FIXES
	if (numbits > 32) {
		Sys_Error("%s: invalid numbits %d\n", __func__, numbits);
	}
#endif // REHLDS_FIXES

	if (msg_badread)
	{
		result = 1;
	}
	else
	{
		if (bfread.nCurInputBit >= 8)
		{
			++bfread.nMsgReadCount;
			++bfread.nBytesRead;
			++bfread.pInByte;

			bfread.nCurInputBit = 0;
		}

		uint32 bits = (bfread.nCurInputBit + numbits) & 7;

		if ((unsigned int)(bfread.nCurInputBit + numbits) <= 32)
		{
			result = (*(unsigned int *)bfread.pInByte >> bfread.nCurInputBit) & ROWBITTABLE[numbits];

			uint32 bytes = (bfread.nCurInputBit + numbits) >> 3;

			if (bits)
			{
				bfread.nCurInputBit = bits;
			}
			else
			{
				bfread.nCurInputBit = 8;
				bytes--;
			}

			bfread.pInByte += bytes;
			bfread.nMsgReadCount += bytes;
			bfread.nBytesRead += bytes;
		}
		else
		{
			result = ((*(unsigned int *)(bfread.pInByte + 4) & ROWBITTABLE[bits]) << (32 - bfread.nCurInputBit)) | (*(unsigned int *)bfread.pInByte >> bfread.nCurInputBit);
			bfread.nCurInputBit = bits;
			bfread.pInByte += 4;
			bfread.nMsgReadCount += 4;
			bfread.nBytesRead += 4;
		}

		if (bfread.nMsgReadCount > bfread.pbuf->cursize)
		{
			result = 1;
			msg_badread = 1;
		}
	}

	return result;
}

uint32 MSG_PeekBits(int numbits)
{
	bf_read_t savebf = bfread;
	uint32 r = MSG_ReadBits(numbits);
	bfread = savebf;

	return r;
}

int MSG_ReadSBits(int numbits)
{
	int nSignBit = MSG_ReadOneBit();
	int result = MSG_ReadBits(numbits - 1);

	if (nSignBit)
	{
		result = -result;
	}

	return result;
}

char *MSG_ReadBitString(void)
{
	static char buf[8192];

	char *p = &buf[0];

	for (char c = MSG_ReadBits(8); c; c = MSG_ReadBits(8))
	{
#ifdef REHLDS_FIXES
		if (msg_badread)	// Prevent infinite cycle if msg_badread
		{
			break;
		}
#endif
		*p++ = c;
	}

	*p = 0;

	return buf;
}

int MSG_ReadBitData(void *dest, int length)
{
	if (length > 0)
	{
		int i = length;
		unsigned char * p = (unsigned char *)dest;

		do
		{
			*p = (unsigned char)MSG_ReadBits(8);
			p++;
			--i;
		}
		while (i);
	}

	return length;
}

float MSG_ReadBitCoord(void)
{
	float value = 0;

	int intval = MSG_ReadOneBit();
	int fractval = MSG_ReadOneBit();

	if (intval || fractval)
	{
		int signbit = MSG_ReadOneBit();

		if (intval)
		{
			intval = MSG_ReadBits(12);
		}

		if (fractval)
		{
			fractval = MSG_ReadBits(3);
		}

		value = (float)(fractval / 8.0 + intval);

		if (signbit)
		{
			value = -value;
		}
	}

	return value;
}

void MSG_WriteBitCoord(const float f)
{
	int signbit = f <= -0.125;
	int intval = abs((int32)f);
	int fractval = abs((int32)f * 8) & 7;

	MSG_WriteOneBit(intval);
	MSG_WriteOneBit(fractval);

	if (intval || fractval)
	{
		MSG_WriteOneBit(signbit);
		if (intval)
			MSG_WriteBits(intval, 12);
		if (fractval)
			MSG_WriteBits(fractval, 3);
	}
}

void MSG_ReadBitVec3Coord(vec3_t fa)
{
	int xflag = MSG_ReadOneBit();
	int yflag = MSG_ReadOneBit();
	int zflag = MSG_ReadOneBit();

	if (xflag)
		fa[0] = MSG_ReadBitCoord();
	if (yflag)
		fa[1] = MSG_ReadBitCoord();
	if (zflag)
		fa[2] = MSG_ReadBitCoord();
}

void MSG_WriteBitVec3Coord(const vec3_t fa)
{
	bool xflag = fa[0] <= -0.125 || fa[0] >= 0.125;
	bool yflag = fa[1] <= -0.125 || fa[1] >= 0.125;
	bool zflag = fa[2] <= -0.125 || fa[2] >= 0.125;

	MSG_WriteOneBit(xflag);
	MSG_WriteOneBit(yflag);
	MSG_WriteOneBit(zflag);

	if (xflag)
		MSG_WriteBitCoord(fa[0]);
	if (yflag)
		MSG_WriteBitCoord(fa[1]);
	if (zflag)
		MSG_WriteBitCoord(fa[2]);
}

float MSG_ReadCoord(void)
{
	return (float)(MSG_ReadShort() * (1.0 / 8));
}

void MSG_WriteCoord(sizebuf_t *sb, const float f)
{
	MSG_WriteShort(sb, (int)(f * 8.0));
}

void MSG_ReadVec3Coord(sizebuf_t *sb, vec3_t fa)
{
	if (MSG_IsBitReading())
	{
		MSG_ReadBitVec3Coord(fa);
	}
	else
	{
		MSG_StartBitReading(sb);
		MSG_ReadBitVec3Coord(fa);
		MSG_EndBitReading(sb);
	}
}

void MSG_WriteVec3Coord(sizebuf_t *sb, const vec3_t fa)
{
	MSG_StartBitWriting(sb);
	MSG_WriteBitVec3Coord(fa);
	MSG_EndBitWriting(sb);
}

void MSG_BeginReading(void)
{
	msg_readcount = 0;
	msg_badread = 0;
}

int MSG_ReadChar(void)
{
	int c;

	if (msg_readcount < net_message.cursize)
	{
		c = net_message.data[msg_readcount];
		msg_readcount++;
	}
	else
	{
		msg_badread = 1;
		c = -1;
	}

	return c;
}

int MSG_ReadByte(void)
{
	int c;

	if (msg_readcount < net_message.cursize)
	{
		c = net_message.data[msg_readcount];
		msg_readcount++;
	}
	else
	{
		msg_badread = 1;
		c = -1;
	}

	return c;
}

int MSG_ReadShort(void)
{
	int c;

	if (msg_readcount + 2 <= net_message.cursize )
	{
		c = *(int16 *)&net_message.data[msg_readcount];
		msg_readcount += 2;
	}
	else
	{
		msg_badread = 1;
		c = -1;
	}

	return c;
}

NOXREF int MSG_ReadWord(void)
{
	NOXREFCHECK;

	int c;

	if (msg_readcount + 2 <= net_message.cursize)
	{
		c = *(uint16 *)&net_message.data[msg_readcount];
		msg_readcount += 2;
	}
	else
	{
		msg_badread = 1;
		c = -1;
	}

	return c;
}

int MSG_ReadLong(void)
{
	int c;

	if (msg_readcount + 4 <= net_message.cursize)
	{
		c = *(uint32 *)&net_message.data[msg_readcount];
		msg_readcount += 4;
	}
	else
	{
		msg_badread = 1;
		c = -1;
	}

	return c;
}

float MSG_ReadFloat(void)
{
	float f;

	union
	{
		unsigned char    b[4];
		float   f;
		int     l;
	} dat;

	if (msg_readcount + 4 <= net_message.cursize)
	{
		dat.b[0] = net_message.data[msg_readcount];
		dat.b[1] = net_message.data[msg_readcount + 1];
		dat.b[2] = net_message.data[msg_readcount + 2];
		dat.b[3] = net_message.data[msg_readcount + 3];
		msg_readcount += 4;

		dat.l = LittleLong(dat.l);

		return dat.f;
	}
	else
	{
		msg_badread = 1;
		f = -1.0;
	}

	return f;
}

int MSG_ReadBuf(int iSize, void *pbuf)
{
	if (msg_readcount + iSize <= net_message.cursize)
	{
		Q_memcpy(pbuf, &net_message.data[msg_readcount], iSize);
		msg_readcount += iSize;

		return 1;
	}

	msg_badread = 1;
	return -1;
}

char *MSG_ReadString(void)
{
	int c = 0, l = 0;
	static char string[8192];

	while ((c = MSG_ReadChar(), c) && c != -1 && l < ARRAYSIZE(string) - 1)
	{
		string[l++] = c;
	}
	string[l] = 0;

	return string;
}

char *MSG_ReadStringLine(void)
{
	int c = 0, l = 0;
	static char string[2048];

	while ((c = MSG_ReadChar(), c) && c != '\n' && c != -1 && l < ARRAYSIZE(string) - 1)
	{
		string[l++] = c;
	}
	string[l] = 0;

	return string;
}

float MSG_ReadAngle(void)
{
	int c = MSG_ReadChar();
#ifdef REHLDS_FIXES
	if (c == -1)	// FIXED: Added check for wrong value, but just return 0 instead of -1 * (360.0 / 256)
	{
		return 0;
	}
#endif
	return (float)(c * (360.0 / 256));
}

float MSG_ReadHiresAngle(void)
{
	int c = MSG_ReadShort();
#ifdef REHLDS_FIXES
	if (c == -1)	// FIXED: Added check for wrong value, but just return 0 instead of -1 * (360.0 / 65536)
	{
		return 0;
	}
#endif
	return (float)(MSG_ReadShort() * (360.0 / 65536));
}

void MSG_ReadUsercmd(usercmd_t *to, usercmd_t* from)
{
	MSG_StartBitReading(&net_message);
#ifdef REHLDS_OPT_PEDANTIC
	DELTA_ParseDelta((byte *)from, (byte *)to, g_pusercmddelta);
#else
	DELTA_ParseDelta((byte *)from, (byte *)to, SV_LookupDelta("usercmd_t"));
#endif
	MSG_EndBitReading(&net_message);
	COM_NormalizeAngles(to->viewangles);
}

#endif // MSG_Functions_region

#ifndef SZ_Functions_region

void SZ_Alloc(const char *name, sizebuf_t *buf, int startsize)
{
	buf->buffername = name;

	if (startsize < 256)
	{
		startsize = 256;
	}

	buf->data = (byte *)Hunk_AllocName(startsize, name);
	buf->maxsize = startsize;
	buf->cursize = 0;
	buf->flags = SIZEBUF_CHECK_OVERFLOW;
}

void SZ_Clear(sizebuf_t *buf)
{
	buf->flags &= ~SIZEBUF_OVERFLOWED;
	buf->cursize = 0;
}

qboolean SZ_HasSpace(sizebuf_t *buf, int length)
{
	if ((buf->cursize + length) > buf->maxsize)
		return FALSE;

	return TRUE;
}

qboolean SZ_HasSpaceToRead(const sizebuf_t *buf, int length)
{
	if ((msg_readcount + length) > buf->maxsize)
		return FALSE;

	return TRUE;
}

qboolean SZ_HasSomethingToRead(const sizebuf_t *buf, int length)
{
	if ((msg_readcount + length) > buf->cursize)
		return FALSE;

	return TRUE;
}

void *EXT_FUNC SZ_GetSpace(sizebuf_t *buf, int length)
{
	void *data;
	const char *buffername = buf->buffername ? buf->buffername : "???";


	if (length < 0)
	{
		Sys_Error("%s: %i negative length on %s", __func__, length, buffername);
	}

	if (!SZ_HasSpace(buf, length))
	{
#ifdef REHLDS_FIXES
		if (!(buf->flags & SIZEBUF_ALLOW_OVERFLOW))
		{
			if (!buf->maxsize)
			{
				Sys_Error("%s: tried to write to an uninitialized sizebuf_t: %s", __func__, buffername);
			}
			else if (length > buf->maxsize)
			{
				Sys_Error("%s: %i is > full buffer size on %s", __func__, length, buffername);
			}
			else
			{
				Sys_Error("%s: overflow without FSB_ALLOWOVERFLOW set on %s", __func__, buffername);
			}
		}

		if (length > buf->maxsize)
		{
			Con_DPrintf("%s: %i is > full buffer size on %s, ignoring", __func__, length, buffername);
		}
#else // REHLDS_FIXES
		if (!(buf->flags & SIZEBUF_ALLOW_OVERFLOW))
		{
			if (!buf->maxsize)
			{
				Sys_Error("%s: Tried to write to an uninitialized sizebuf_t: %s", __func__, buffername);
			}
			else
			{
				Sys_Error("%s: overflow without FSB_ALLOWOVERFLOW set on %s", __func__, buffername);
			}
		}

		if (length > buf->maxsize)
		{
			if (!(buf->flags & SIZEBUF_ALLOW_OVERFLOW))
			{
				Sys_Error("%s: %i is > full buffer size on %s", __func__, length, buffername);
			}

			Con_DPrintf("%s: %i is > full buffer size on %s, ignoring", __func__, length, buffername);
		}
#endif // REHLDS_FIXES

		Con_Printf("%s: overflow on %s\n", __func__, buffername);

		SZ_Clear(buf);
		buf->flags |= SIZEBUF_OVERFLOWED;
	}

	data = &buf->data[buf->cursize];
	buf->cursize = length + buf->cursize;

	return data;
}

void SZ_Write(sizebuf_t *buf, const void *data, int length)
{
	unsigned char *pData = (unsigned char *)SZ_GetSpace(buf, length);

	if (!(buf->flags & SIZEBUF_OVERFLOWED))
	{
		Q_memcpy(pData, data, length);
	}
}

void SZ_Print(sizebuf_t *buf, const char *data)
{
	unsigned char *pData;
	int len = Q_strlen(data) + 1;

	if (buf->data[buf->cursize - 1])
	{
		pData = (unsigned char *)SZ_GetSpace(buf, len);
	}
	else
	{
		pData = (unsigned char *)SZ_GetSpace(buf, len - 1) - 1;
	}

	if (!(buf->flags & SIZEBUF_OVERFLOWED))
	{
		Q_memcpy(pData, data, len);
	}
}

#endif // SZ_Functions_region


#ifndef COM_Functions_region

int com_argc;
char **com_argv;

char com_token[COM_TOKEN_LEN];

qboolean com_ignorecolons;
qboolean s_com_token_unget;
char *com_last_in_quotes_data = NULL;
char com_clientfallback[MAX_PATH];
char com_gamedir[MAX_PATH];
char com_cmdline[COM_MAX_CMD_LINE];

cache_user_t *loadcache;
unsigned char *loadbuf;
int loadsize;

const unsigned char mungify_table[] =
{
	0x7A, 0x64, 0x05, 0xF1,
	0x1B, 0x9B, 0xA0, 0xB5,
	0xCA, 0xED, 0x61, 0x0D,
	0x4A, 0xDF, 0x8E, 0xC7
};

const unsigned char mungify_table2[] =
{
	0x05, 0x61, 0x7A, 0xED,
	0x1B, 0xCA, 0x0D, 0x9B,
	0x4A, 0xF1, 0x64, 0xC7,
	0xB5, 0x8E, 0xDF, 0xA0
};

unsigned char mungify_table3[] =
{
	0x20, 0x07, 0x13, 0x61,
	0x03, 0x45, 0x17, 0x72,
	0x0A, 0x2D, 0x48, 0x0C,
	0x4A, 0x12, 0xA9, 0xB5
};

NOXREF char *COM_SkipPath(char *pathname)
{
	NOXREFCHECK;

	char *last = pathname;

	while (*pathname)
	{
		if (*pathname == '/' || *pathname == '\\')
			last = pathname + 1;
		pathname++;
	}
	return last;
}

void COM_StripExtension(char *in, char *out)
{
	char *c, *d = NULL;
	int i;

	// Search for the first dot after the last path separator
	c = in;
	while (*c)
	{
		if (*c == '/' || *c == '\\')
		{
			d = NULL;	// reset dot location on path separator
		}
		else if (d == NULL && *c == '.')
		{
			d = c;		// store first dot location in the file name
		}
		c++;
	}

	if (out == in)
	{
		if (d != NULL)
		{
			*d = 0;
		}
	}
	else
	{
		if (d != NULL)
		{
			i = d - in;
			Q_memcpy(out, in, i);
			out[i] = 0;
		}
		else
		{
			Q_strcpy(out, in);
		}
	}
}

char *COM_FileExtension(char *in)
{
#ifdef REHLDS_FIXES
	int len = Q_strlen(in);
	if (len <= 0)
		return "";  // no extension

	char *src = in + len - 1;

	// back up until a . or the start
	while (src >= in && !PATHSEPARATOR(*src))
	{
		if (*src == '.')
		{
			src++; // skip dot

			if (*src != '\0')
			{
				return src;
			}

			break;
		}

		src--;
	}

	return "";  // no extension
#else // #ifdef REHLDS_FIXES
	static char exten[MAX_PATH];
	char *c, *d = NULL;
	int i;

	// Search for the first dot after the last path separator
	c = in;
	while (*c)
	{
		if (*c == '/' || *c == '\\')
		{
			d = NULL;	// reset dot location on path separator
		}
		else if (d == NULL && *c == '.')
		{
			d = c;		// store first dot location in the file name
		}
		c++;
	}

	if (d == NULL)
	{
		return "";
	}

	d++;	// skip dot
	// Copy extension
	for (i = 0; i < (ARRAYSIZE(exten) - 1) && *d; i++, d++)
	{
		exten[i] = *d;
	}
	exten[i] = 0;
	return exten;
#endif // #ifdef REHLDS_FIXES
}

// Fills "out" with the file name without path and extension
void COM_FileBase(const char *in, char *out)
{
	COM_FileBase_s(in, out, -1);
}

// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
const char *COM_FileBase_s(const char *in, char *out, int size)
{
	if (!in || !in[0])
	{
		*out = '\0';
		return NULL;
	}

	int len = Q_strlen(in);
	if (len <= 0)
		return NULL;

	// scan backward for '.'
	int end = len - 1;
	while (end && in[end] != '.' && !PATHSEPARATOR(in[end]))
		end--;

	// no '.', copy to end
	if (in[end] != '.')
	{
		end = len - 1;
	}
	else
	{
		// Found ',', copy to left of '.'
		end--;
	}

	// Scan backward for '/'
	int start = len - 1;
	while (start >= 0 && !PATHSEPARATOR(in[start]))
		start--;

	if (start < 0 || !PATHSEPARATOR(in[start]))
	{
		start = 0;
	}
	else
	{
		start++;
	}

	// Length of new sting
	int maxcopy = end - start + 1;
	if (size >= 0 && maxcopy >= size)
		return NULL;

	// Copy partial string
	Q_strncpy(out, &in[start], maxcopy);
	out[maxcopy] = '\0';
	return out;
}

void COM_DefaultExtension(char *path, char *extension)
{
	char *src;
	src = path + Q_strlen(path) - 1;

	while (*src != '/' && *src != '\\' && src != path)
	{
		if (*src == '.')
		{
			return;
		}

		src--;
	}

	Q_strcat(path, extension);
}

void COM_UngetToken(void)
{
	s_com_token_unget = 1;
}

char *COM_Parse(char *data)
{
	int c;
	uchar32 wchar;
	int len;

	if (s_com_token_unget)
	{
		s_com_token_unget = 0;
		return data;
	}

	len = 0;
	com_token[0] = 0;

	if (!data)
	{
		return NULL;
	}

	if (com_last_in_quotes_data == data)
	{
		// continue to parse quoted string
		com_last_in_quotes_data = NULL;
		goto inquotes;
	}

skipwhite:
	// skip whitespace
	while (!V_UTF8ToUChar32(data, &wchar) && wchar <= 32)
	{
		if (!wchar)
			return NULL;
		data = Q_UnicodeAdvance(data, 1);
	}

	c = *data;

	// skip // comments till the next line
	if (c == '/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;	// start over new line
	}

	// handle quoted strings specially: copy till the end or another quote
	if (c == '\"')
	{
		data++;	// skip starting quote
		while (true)
		{
inquotes:
			c = *data++;	// get char and advance
			if (!c)	// EOL
			{
				com_token[len] = 0;
				return data - 1;	// we are done with that, but return data to show that token is present
			}
			if (c == '\"')	// closing quote
			{
				com_token[len] = 0;
				return data;
			}

			com_token[len] = c;
			len++;

			if (len == COM_TOKEN_LEN - 1)	// check if buffer is full
			{
				// remember in-quotes state
				com_last_in_quotes_data = data;

				com_token[len] = 0;
				return data;
			}
		}
	}

	// parse single characters
	if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',' || (!com_ignorecolons && c == ':'))
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data + 1;
	}

	// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
		if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',' || (!com_ignorecolons && c == ':'))
			break;
	} while (len < COM_TOKEN_LEN - 1 && (c < 0 || c > 32));

	com_token[len] = 0;
	return data;
}

char *COM_ParseLine(char *data)
{
#ifdef REHLDS_FIXES
	unsigned int c;
#else
	int c;
#endif
	int len;

	if (s_com_token_unget)
	{
		s_com_token_unget = 0;
		return data;
	}

	len = 0;
	com_token[0] = 0;

	if (!data)
	{
		return NULL;
	}

	c = *data;

	// parse a line out of the data
#ifdef REHLDS_FIXES
	while ((c >= ' ' || c == '\t') && (len < COM_TOKEN_LEN - 1))
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	}
#else
	do
	{
		com_token[len] = c;	// TODO: Here c may be any ASCII, \n for example, but we are copy it in the token
		data++;
		len++;
		c = *data;
	} while (c >= ' ' && (len < COM_TOKEN_LEN - 1));	// TODO: Will break on \t, may be it shouldn't?
#endif

	com_token[len] = 0;

	if (c == 0) // end of file
	{
		return NULL;
	}

	// skip end of the line (CR, LF, etc.., but not TAB)
	while ((c = *data) < ' ' && c != '\t')
	{
		if (c == 0)
		{
			return NULL; // end of file;
		}
		data++;
	}

	return data;
}

int COM_TokenWaiting(char *buffer)
{
	char *p;

	p = buffer;
	while (*p && *p != '\n')
	{
		if (!isspace(*p) || isalnum(*p))
			return 1;

		p++;
	}

	return 0;
}

int COM_CheckParm(const char *parm)
{
	int i;

	for (i = 1 ; i < com_argc; i++)
	{
		if (!com_argv[i])
		{
			continue;
		}

		if (!Q_strcmp(parm, (const char*)com_argv[i]))
		{
			return i;
		}
	}

	return 0;
}

void COM_InitArgv(int argc, char *argv[])
{
	qboolean safe = 0;

	static char *safeargvs[NUM_SAFE_ARGVS] =
	{
		"-stdvid", "-nolan", "-nosound", "-nocdaudio", "-nojoy", "-nomouse", "-dibonly"
	};
	static char *largv[MAX_NUM_ARGVS + NUM_SAFE_ARGVS + 1];

	int i, j;
	char *c;

	// Reconstruct full command line
	com_cmdline[0] = 0;
	for (i = 0, j = 0; i < MAX_NUM_ARGVS && i < argc && j < COM_MAX_CMD_LINE - 1; i++)
	{
		c = argv[i];
		if (*c)
		{
			while (*c && j < COM_MAX_CMD_LINE - 1)
			{
				com_cmdline[j++] = *c++;
			}
			if (j >= COM_MAX_CMD_LINE - 1)
			{
				break;
			}
			com_cmdline[j++] = ' ';
		}
	}
	com_cmdline[j] = 0;

	// Copy args pointers to our array
	for (com_argc = 0; (com_argc < MAX_NUM_ARGVS) && (com_argc < argc); com_argc++)
	{
		largv[com_argc] = argv[com_argc];

		if (!Q_strcmp("-safe", argv[com_argc]))
		{
			safe = 1;
		}
	}

	// Add arguments introducing more failsafeness
	if (safe)
	{
		// force all the safe-mode switches. Note that we reserved extra space in
		// case we need to add these, so we don't need an overflow check
		for (int i = 0; i < NUM_SAFE_ARGVS; i++)
		{
			largv[com_argc] = safeargvs[i];
			com_argc++;
		}
	}

	largv[com_argc] = " ";
	com_argv = largv;
}

void COM_Init(char *basedir)
{
	unsigned short swaptest = 1;

	if (*(byte *)&swaptest == 1)
	{
		bigendien = 0;
		BigShort = ShortSwap;
		LittleShort = ShortNoSwap;
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		BigFloat = FloatSwap;
		LittleFloat = FloatNoSwap;
	}
	else
	{
		bigendien = 1;
		BigShort = ShortNoSwap;
		LittleShort = ShortSwap;
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		BigFloat = FloatNoSwap;
		LittleFloat = FloatSwap;
	}

	COM_BitOpsInit();
}

char *va(char *format, ...)
{
	va_list argptr;
	static int current = 0;
	static char string[16][1024];

	current = (current + 1) % 16;

	va_start(argptr, format);
	Q_vsnprintf(string[current], ARRAYSIZE(string[current]), format, argptr);
	va_end(argptr);

	return string[current];
}

NOXREF char *vstr(vec_t *v)
{
	NOXREFCHECK;

	static int idx = 0;
	static char string[16][1024];

	idx++;
	idx &= 15;

	Q_snprintf(string[idx], ARRAYSIZE(string[idx]), "%.4f %.4f %.4f", v[0], v[1], v[2]);
	return string[idx];
}

NOXREF int memsearch(unsigned char *start, int count, int search)
{
	NOXREFCHECK;

	for (int i = 0; i < count; i++)
	{
		if (start[i] == search)
		{
			return i;
		}
	}

	return -1;
}

NOXREF void COM_WriteFile(char *filename, void *data, int len)
{
	NOXREFCHECK;

	char path[MAX_PATH];
	Q_snprintf(path, MAX_PATH - 1, "%s", filename);
	path[MAX_PATH - 1] = 0;

	COM_FixSlashes(path);
	COM_CreatePath(path);

	FileHandle_t fp = FS_Open(path, "wb");

	if (fp)
	{
		Sys_Printf("%s: %s\n", __func__, path);
		FS_Write(data, len, 1, fp);
		FS_Close(fp);
	}
	else
	{
		Sys_Printf("%s: failed on %s\n", __func__, path);
	}
}

void COM_FixSlashes(char *pname)
{
	while (*pname)
	{
#ifdef _WIN32
		if (*pname == '/')
		{
			*pname = '\\';
		}
#else
		if (*pname == '\\')
		{
			*pname = '/';
		}
#endif

		pname++;
	}
}

void COM_CreatePath(char *path)
{
	char *ofs;
	char old;

	if (*path == 0)
	{
		return;
	}

	for (ofs = path + 1; *ofs; ofs++)
	{
		if (*ofs == '/' || *ofs == '\\')
		{
			old = *ofs;
			*ofs = 0;
			FS_CreateDirHierarchy(path, 0);
			*ofs = old;
		}
	}
}

NOXREF void COM_CopyFile(char *netpath, char *cachepath)
{
	NOXREFCHECK;

	int count;
	int remaining;
	char buf[4096];

	FileHandle_t out;
	FileHandle_t in = FS_Open(netpath, "rb");

	if (!in)
	{
		return;
	}

	count = FS_Size(in);
	COM_CreatePath(cachepath);

	for (out = FS_Open(cachepath, "wb"); count; count -= remaining)
	{
		remaining = count;

		if (remaining > 4096)
		{
			remaining = 4096;
		}

		FS_Read(buf, remaining, 1, in);
		FS_Write(buf, remaining, 1, out);
	}

	FS_Close(in);
	FS_Close(out);
}

NOXREF int COM_ExpandFilename(char *filename)
{
	NOXREFCHECK;

	char netpath[MAX_PATH];

	FS_GetLocalPath(filename, netpath, ARRAYSIZE(netpath));
	Q_strcpy(filename, netpath);
	return *filename != 0;
}

// small helper function shared by lots of modules
qboolean COM_IsAbsolutePath(const char *pStr)
{
	if (strchr(pStr, ':') || pStr[0] == '/' || pStr[0] == '\\')
		return FALSE;

	return TRUE;
}

qboolean COM_IsValidPath(const char *pszFilename)
{
	if (!pszFilename)
		return FALSE;

	if (Q_strlen(pszFilename) <= 0    ||
		Q_strstr(pszFilename, "\\\\") ||	// to protect network paths
		Q_strstr(pszFilename, ":")    ||	// to protect absolute paths
		Q_strstr(pszFilename, "..")   ||	// to protect relative paths
		Q_strstr(pszFilename, "~")    ||
		Q_strstr(pszFilename, "\n")   ||	// CFileSystem_Stdio::FS_fopen doesn't allow this
		Q_strstr(pszFilename, "\r"))		// CFileSystem_Stdio::FS_fopen doesn't allow this
	{
		return FALSE;
	}

	return TRUE;
}

int EXT_FUNC COM_FileSize(const char *filename)
{
	FileHandle_t fp;
	int iSize;

	iSize = -1;
	fp = FS_Open(filename, "rb");
	if (fp)
	{
		iSize = FS_Size(fp);
		FS_Close(fp);
	}
	return iSize;
}

unsigned char* EXT_FUNC COM_LoadFile(const char *path, int usehunk, int *pLength)
{
	if (!path || !path[0])
		return NULL;

	char base[MAX_PATH];
	unsigned char *buf = NULL;

#ifndef SWDS
	g_engdstAddrs->COM_LoadFile(&path, &usehunk, &pLength);
#endif

	if (pLength)
	{
		*pLength = 0;
	}

	FileHandle_t hFile = FS_Open(path, "rb");

	if (!hFile)
	{
		return NULL;
	}

	int len = FS_Size(hFile);
	if (!COM_FileBase_s(path, base, sizeof(base)))
		Sys_Error("%s: Bad path length: %s", __func__, path);

	base[32] = '\0';

	switch (usehunk)
	{
	case 0:
		buf = (unsigned char *)Z_Malloc(len + 1);
		break;

	case 1:
		buf = (unsigned char *)Hunk_AllocName(len + 1, base);
		break;

	case 2:
		buf = (unsigned char *)Hunk_TempAlloc(len + 1);
		break;

	case 3:
		buf = (unsigned char *)Cache_Alloc(loadcache, len + 1, base);
		break;

	case 4:
		if (len + 1 <= loadsize)
		{
			buf = loadbuf;
		}
		else
		{
			buf = (unsigned char *)Hunk_TempAlloc(len + 1);
		}
		break;

	case 5:
		buf = (unsigned char *)Mem_Malloc(len + 1);
		break;

	default:
#ifdef REHLDS_FIXES
		FS_Close(hFile);
#endif
		Sys_Error("%s: bad usehunk", __func__);
	}

	if (!buf)
	{
#ifdef REHLDS_FIXES
		FS_Close(hFile);
#endif
		Sys_Error("%s: not enough space for %s", __func__, path);
	}

	FS_Read(buf, len, 1, hFile);
	FS_Close(hFile);

	buf[len] = 0;

	if (pLength)
	{
		*pLength = len;
	}

	return buf;
}

void EXT_FUNC COM_FreeFile(void *buffer)
{
#ifndef SWDS
	g_engdstAddrs->COM_FreeFile();
#endif

	if (buffer)
	{
		Mem_Free(buffer);
	}
}

void COM_CopyFileChunk(FileHandle_t dst, FileHandle_t src, int nSize)
{
	int copysize;
	char copybuf[COM_COPY_CHUNK_SIZE];

	copysize = nSize;

	while (copysize > COM_COPY_CHUNK_SIZE)
	{
		FS_Read(copybuf, COM_COPY_CHUNK_SIZE, 1, src);
		FS_Write(copybuf, COM_COPY_CHUNK_SIZE, 1, dst);
		copysize -= COM_COPY_CHUNK_SIZE;
	}

	FS_Read(copybuf, copysize, 1, src);
	FS_Write(copybuf, copysize, 1, dst);
	FS_Flush(src);
	FS_Flush(dst);
}

NOXREF unsigned char *COM_LoadFileLimit(char *path, int pos, int cbmax, int *pcbread, FileHandle_t *phFile)
{
	NOXREFCHECK;
	FileHandle_t hFile;
	unsigned char *buf;
	char base[32];
	int len;
	int cbload;

	hFile = *phFile;
	if (!hFile)
	{
		hFile = FS_Open(path, "rb");
		if (!hFile)
			return NULL;
	}

	len = FS_Size(hFile);
	if (len < pos)
	{
#ifdef REHLDS_FIXES
		FS_Close(hFile);
#endif
		Sys_Error("%s: invalid seek position for %s", __func__, path);
	}

	FS_Seek(hFile, pos, FILESYSTEM_SEEK_HEAD);

	if (len > cbmax)
		cbload = cbmax;
	else
		cbload = len;

	*pcbread = cbload;

	if (path)
		COM_FileBase(path, base);

	buf = (unsigned char *)Hunk_TempAlloc(cbload + 1);
	if (!buf)
	{
		if (path)
		{
#ifdef REHLDS_FIXES
			FS_Close(hFile);
#endif
			Sys_Error("%s: not enough space for %s", __func__, path);
		}

		FS_Close(hFile);
		return NULL;
	}

	buf[cbload] = 0;
	FS_Read(buf, cbload, 1, hFile);
	*phFile = hFile;

	return buf;
}

unsigned char *COM_LoadHunkFile(char *path)
{
	return COM_LoadFile(path, 1, NULL);
}

unsigned char *COM_LoadTempFile(char *path, int *pLength)
{
	return COM_LoadFile(path, 2, pLength);
}

void EXT_FUNC COM_LoadCacheFile(char *path, struct cache_user_s *cu)
{
	loadcache = cu;
	COM_LoadFile(path, 3, 0);
}

NOXREF unsigned char *COM_LoadStackFile(char *path, void *buffer, int bufsize, int *length)
{
	NOXREFCHECK;

	loadbuf = (unsigned char *)buffer;
	loadsize = bufsize;

	return COM_LoadFile(path, 4, length);
}

void COM_Shutdown(void)
{
	// Do nothing.
}

NOXREF void COM_AddAppDirectory(char *pszBaseDir, const char *appName)
{
	NOXREFCHECK;

	FS_AddSearchPath(pszBaseDir, "PLATFORM");
}

void COM_AddDefaultDir(char *pszDir)
{
	if (pszDir && *pszDir)
	{
		FileSystem_AddFallbackGameDir(pszDir);
	}
}

void COM_StripTrailingSlash(char *ppath)
{
	int len = Q_strlen(ppath);

	if (len > 0)
	{
		if ((ppath[len - 1] == '\\') || (ppath[len - 1] == '/'))
		{
			ppath[len - 1] = 0;
		}
	}
}

void COM_ParseDirectoryFromCmd(const char *pCmdName, char *pDirName, const char *pDefault)
{
	const char *pParameter = NULL;
	int cmdParameterIndex = COM_CheckParm(pCmdName);

	if (cmdParameterIndex && cmdParameterIndex < com_argc - 1)
	{
		pParameter = com_argv[cmdParameterIndex + 1];

		if (*pParameter == '-' || *pParameter == '+')
		{
			pParameter = NULL;
		}
	}

	// Found a valid parameter on the cmd line?
	if (pParameter)
	{
		// Grab it
		Q_strcpy(pDirName, pParameter);
	}
	else if (pDefault)
	{
		// Ok, then use the default
		Q_strcpy(pDirName, pDefault);
	}
	else
	{
		// If no default either, then just terminate the string
		pDirName[0] = 0;
	}

	COM_StripTrailingSlash(pDirName);
}

// TODO: finish me!
qboolean COM_SetupDirectories(void)
{
	char pDirName[512];

	com_clientfallback[0] = 0;
	com_gamedir[0] = 0;

	COM_ParseDirectoryFromCmd("-basedir", pDirName, "valve");
	COM_ParseDirectoryFromCmd("-game", com_gamedir, pDirName);

	if (FileSystem_SetGameDirectory(pDirName, (const char *)(com_gamedir[0] != 0 ? com_gamedir : 0)))
	{
		Info_SetValueForStarKey(Info_Serverinfo(), "*gamedir", com_gamedir, MAX_INFO_STRING);
		return 1;
	}

	return 0;
}

void COM_CheckPrintMap(dheader_t *header, const char *mapname, qboolean bShowOutdated)
{
	if (header->version == HLBSP_VERSION)
	{
		if (!bShowOutdated)
		{
			Con_Printf("%s\n", mapname);
		}
	}
	else
	{
		if (bShowOutdated)
		{
			Con_Printf("OUTDATED:  %s\n", mapname);
		}
	}
}

void COM_ListMaps(char *pszSubString)
{
	dheader_t header;
	FileHandle_t fp;

	char mapwild[64];
	char curDir[4096];
	char pFileName[64];
	const char *findfn;

	int nSubStringLen = 0;

	if (pszSubString && *pszSubString)
	{
		nSubStringLen = Q_strlen(pszSubString);
	}

	Con_Printf("-------------\n");

	for (int bShowOutdated = 1; bShowOutdated >= 0; bShowOutdated--)
	{
		Q_strcpy(mapwild, "maps/*.bsp");
		findfn = Sys_FindFirst(mapwild, NULL);

		while (findfn != NULL)
		{
			if (Q_snprintf(curDir, ARRAYSIZE(curDir), "maps/%s", findfn) < ARRAYSIZE(curDir))
			{
				FS_GetLocalPath(curDir, curDir, ARRAYSIZE(curDir));

				if (Q_strstr(curDir, com_gamedir) && (!nSubStringLen || !Q_strnicmp(findfn, pszSubString, nSubStringLen)))
				{
					if (Q_snprintf(pFileName, ARRAYSIZE(pFileName), "maps/%s", findfn) < ARRAYSIZE(pFileName))
					{
						Q_memset(&header, 0, sizeof(dheader_t));

						fp = FS_Open(pFileName, "rb");

						if (fp)
						{
							FS_Read(&header, sizeof(dheader_t), 1, fp);
							FS_Close(fp);
						}

						COM_CheckPrintMap(&header, findfn, bShowOutdated != 0);
					}
					else
					{
						Con_Printf("Map name too long: %s\n", findfn);
					}
				}
			}
			else
			{
				Con_Printf("Map name too long: %s\n", findfn);
			}

			findfn = Sys_FindNext(NULL);
		}

		Sys_FindClose();
	}
}

void COM_Log(char *pszFile, char *fmt, ...)
{
	char *pfilename;
	char string[1024];

	if (!pszFile)
	{
#ifdef REHLDS_FIXES
		pfilename = "hllog.txt";
#else
		// Why so serious?
		pfilename = "c:\\hllog.txt";
#endif
	}
	else
	{
		pfilename = pszFile;
	}

	va_list argptr;
	va_start(argptr, fmt);
	Q_vsnprintf(string, ARRAYSIZE(string) - 1, fmt, argptr);
	va_end(argptr);

	string[ARRAYSIZE(string) - 1] = 0;

	FileHandle_t fp = FS_Open(pfilename, "a+t");

	if (fp)
	{
		FS_FPrintf(fp, "%s", string);
		FS_Close(fp);
	}
}

unsigned char* EXT_FUNC COM_LoadFileForMe(const char *filename, int *pLength)
{
	return COM_LoadFile(filename, 5, pLength);
}

int EXT_FUNC COM_CompareFileTime(char *filename1, char *filename2, int *iCompare)
{
	int ft1;
	int ft2;

	*iCompare = 0;

	if (filename1 && filename2)
	{
		ft1 = FS_GetFileTime(filename1);
		ft2 = FS_GetFileTime(filename2);

		if (ft1 >= ft2)
		{
			if (ft1 > ft2)
			{
				*iCompare = 1;
			}

			return 1;
		}
		else
		{
			*iCompare = -1;
			return 1;
		}
	}

	return 0;
}

void EXT_FUNC COM_GetGameDir(char *szGameDir)
{
	if (szGameDir)
	{
		Q_snprintf(szGameDir, MAX_PATH, "%s", com_gamedir);
	}
}

int COM_EntsForPlayerSlots(int nPlayers)
{
	int numedicts = gmodinfo.num_edicts;
	int p = COM_CheckParm("-num_edicts");

	if (p && p < com_argc - 1)
	{
		p = Q_atoi(com_argv[p + 1]);

		if (numedicts < p)
		{
			numedicts = p;
		}
	}

	return (numedicts + 15 * (nPlayers - 1));
}

void COM_NormalizeAngles(vec_t *angles)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		if (angles[i] > 180.0)
		{
			angles[i] = (float)(fmod((double)angles[i], 360.0) - 360.0);
		}
		else if (angles[i] < -180.0)
		{
			angles[i] = (float)(fmod((double)angles[i], 360.0) + 360.0);
		}
	}
}

// Anti-proxy/aimbot obfuscation code
// COM_UnMunge should reversably fixup the data
void COM_Munge(unsigned char *data, int len, int seq)
{
	int i;
	int mungelen;
	int c;
	int *pc;
	unsigned char *p;
	int j;

	mungelen = len & ~3;
	mungelen /= 4;

	for (i = 0; i < mungelen; i++)
	{
		pc = (int *)&data[i * 4];
		c = *pc;
		c ^= ~seq;
		c = LongSwap(c);

		p = (unsigned char *)&c;
		for (j = 0; j < 4; j++)
		{
			*p++ ^= (0xa5 | (j << j) | j | mungify_table[(i + j) & 0x0f]);
		}

		c ^= seq;
		*pc = c;
	}
}

void COM_UnMunge(unsigned char *data, int len, int seq)
{
	int i;
	int mungelen;
	int c;
	int *pc;
	unsigned char *p;
	int j;

	mungelen = len & ~3;
	mungelen /= 4;

	for (i = 0; i < mungelen; i++)
	{
		pc = (int *)&data[i * 4];
		c = *pc;
		c ^= seq;

		p = (unsigned char *)&c;
		for (j = 0; j < 4; j++)
		{
			*p++ ^= (0xa5 | (j << j) | j | mungify_table[(i + j) & 0x0f]);
		}

		c = LongSwap(c);
		c ^= ~seq;
		*pc = c;
	}
}

#ifdef REHLDS_FIXES
// unrolled version
void COM_Munge2(unsigned char *data, int len, int seq)
{
	unsigned int *pc;
	unsigned int *end;
	unsigned int mSeq;

	mSeq = bswap(~seq) ^ seq;
	len /= 4;
	end = (unsigned int *)data + (len & ~15);

	for (pc = (unsigned int *)data; pc < end; pc += 16)
	{
		pc[0]  = bswap(pc[0])  ^ mSeq ^ 0xFFFFE7A5;
		pc[1]  = bswap(pc[1])  ^ mSeq ^ 0xBFEFFFE5;
		pc[2]  = bswap(pc[2])  ^ mSeq ^ 0xFFBFEFFF;
		pc[3]  = bswap(pc[3])  ^ mSeq ^ 0xBFEFBFED;
		pc[4]  = bswap(pc[4])  ^ mSeq ^ 0xBFAFEFBF;
		pc[5]  = bswap(pc[5])  ^ mSeq ^ 0xFFBFAFEF;
		pc[6]  = bswap(pc[6])  ^ mSeq ^ 0xFFEFBFAD;
		pc[7]  = bswap(pc[7])  ^ mSeq ^ 0xFFFFEFBF;
		pc[8]  = bswap(pc[8])  ^ mSeq ^ 0xFFEFF7EF;
		pc[9]  = bswap(pc[9])  ^ mSeq ^ 0xBFEFE7F5;
		pc[10] = bswap(pc[10]) ^ mSeq ^ 0xBFBFE7E5;
		pc[11] = bswap(pc[11]) ^ mSeq ^ 0xFFAFB7E7;
		pc[12] = bswap(pc[12]) ^ mSeq ^ 0xBFFFAFB5;
		pc[13] = bswap(pc[13]) ^ mSeq ^ 0xBFAFFFAF;
		pc[14] = bswap(pc[14]) ^ mSeq ^ 0xFFAFA7FF;
		pc[15] = bswap(pc[15]) ^ mSeq ^ 0xFFEFA7A5;
	}

	switch(len & 15)
	{
	case 15:
		pc[14] = bswap(pc[14]) ^ mSeq ^ 0xFFAFA7FF;
	case 14:
		pc[13] = bswap(pc[13]) ^ mSeq ^ 0xBFAFFFAF;
	case 13:
		pc[12] = bswap(pc[12]) ^ mSeq ^ 0xBFFFAFB5;
	case 12:
		pc[11] = bswap(pc[11]) ^ mSeq ^ 0xFFAFB7E7;
	case 11:
		pc[10] = bswap(pc[10]) ^ mSeq ^ 0xBFBFE7E5;
	case 10:
		pc[9] = bswap(pc[9])   ^ mSeq ^ 0xBFEFE7F5;
	case 9:
		pc[8] = bswap(pc[8])   ^ mSeq ^ 0xFFEFF7EF;
	case 8:
		pc[7] = bswap(pc[7])   ^ mSeq ^ 0xFFFFEFBF;
	case 7:
		pc[6] = bswap(pc[6])   ^ mSeq ^ 0xFFEFBFAD;
	case 6:
		pc[5] = bswap(pc[5])   ^ mSeq ^ 0xFFBFAFEF;
	case 5:
		pc[4] = bswap(pc[4])   ^ mSeq ^ 0xBFAFEFBF;
	case 4:
		pc[3] = bswap(pc[3])   ^ mSeq ^ 0xBFEFBFED;
	case 3:
		pc[2] = bswap(pc[2])   ^ mSeq ^ 0xFFBFEFFF;
	case 2:
		pc[1] = bswap(pc[1])   ^ mSeq ^ 0xBFEFFFE5;
	case 1:
		pc[0] = bswap(pc[0])   ^ mSeq ^ 0xFFFFE7A5;
	}
}
#else // REHLDS_FIXES

void COM_Munge2(unsigned char *data, int len, int seq)
{
	int i;
	int mungelen;
	int c;
	int *pc;
	unsigned char *p;
	int j;

	mungelen = len & ~3;
	mungelen /= 4;

	for (i = 0; i < mungelen; i++)
	{
		pc = (int *)&data[i * 4];
		c = *pc;
		c ^= ~seq;
		c = LongSwap(c);

		p = (unsigned char *)&c;
		for (j = 0; j < 4; j++)
		{
			*p++ ^= (0xa5 | (j << j) | j | mungify_table2[(i + j) & 0x0f]);
		}

		c ^= seq;
		*pc = c;
	}
}
#endif // REHLDS_FIXES

#ifdef REHLDS_FIXES
// unrolled version
void COM_UnMunge2(unsigned char *data, int len, int seq)
{
	unsigned int *pc;
	unsigned int *end;
	unsigned int mSeq;

	mSeq = bswap(~seq) ^ seq;
	len /= 4;
	end = (unsigned int *)data + (len & ~15);

	for (pc = (unsigned int *)data; pc < end; pc += 16)
	{
		pc[0]  = bswap(pc[0]  ^ mSeq ^ 0xFFFFE7A5);
		pc[1]  = bswap(pc[1]  ^ mSeq ^ 0xBFEFFFE5);
		pc[2]  = bswap(pc[2]  ^ mSeq ^ 0xFFBFEFFF);
		pc[3]  = bswap(pc[3]  ^ mSeq ^ 0xBFEFBFED);
		pc[4]  = bswap(pc[4]  ^ mSeq ^ 0xBFAFEFBF);
		pc[5]  = bswap(pc[5]  ^ mSeq ^ 0xFFBFAFEF);
		pc[6]  = bswap(pc[6]  ^ mSeq ^ 0xFFEFBFAD);
		pc[7]  = bswap(pc[7]  ^ mSeq ^ 0xFFFFEFBF);
		pc[8]  = bswap(pc[8]  ^ mSeq ^ 0xFFEFF7EF);
		pc[9]  = bswap(pc[9]  ^ mSeq ^ 0xBFEFE7F5);
		pc[10] = bswap(pc[10] ^ mSeq ^ 0xBFBFE7E5);
		pc[11] = bswap(pc[11] ^ mSeq ^ 0xFFAFB7E7);
		pc[12] = bswap(pc[12] ^ mSeq ^ 0xBFFFAFB5);
		pc[13] = bswap(pc[13] ^ mSeq ^ 0xBFAFFFAF);
		pc[14] = bswap(pc[14] ^ mSeq ^ 0xFFAFA7FF);
		pc[15] = bswap(pc[15] ^ mSeq ^ 0xFFEFA7A5);
	}

	switch(len & 15)
	{
	case 15:
		pc[14] = bswap(pc[14] ^ mSeq ^ 0xFFAFA7FF);
	case 14:
		pc[13] = bswap(pc[13] ^ mSeq ^ 0xBFAFFFAF);
	case 13:
		pc[12] = bswap(pc[12] ^ mSeq ^ 0xBFFFAFB5);
	case 12:
		pc[11] = bswap(pc[11] ^ mSeq ^ 0xFFAFB7E7);
	case 11:
		pc[10] = bswap(pc[10] ^ mSeq ^ 0xBFBFE7E5);
	case 10:
		pc[9] = bswap(pc[9]   ^ mSeq ^ 0xBFEFE7F5);
	case 9:
		pc[8] = bswap(pc[8]   ^ mSeq ^ 0xFFEFF7EF);
	case 8:
		pc[7] = bswap(pc[7]   ^ mSeq ^ 0xFFFFEFBF);
	case 7:
		pc[6] = bswap(pc[6]   ^ mSeq ^ 0xFFEFBFAD);
	case 6:
		pc[5] = bswap(pc[5]   ^ mSeq ^ 0xFFBFAFEF);
	case 5:
		pc[4] = bswap(pc[4]   ^ mSeq ^ 0xBFAFEFBF);
	case 4:
		pc[3] = bswap(pc[3]   ^ mSeq ^ 0xBFEFBFED);
	case 3:
		pc[2] = bswap(pc[2]   ^ mSeq ^ 0xFFBFEFFF);
	case 2:
		pc[1] = bswap(pc[1]   ^ mSeq ^ 0xBFEFFFE5);
	case 1:
		pc[0] = bswap(pc[0]   ^ mSeq ^ 0xFFFFE7A5);
	}
}
#else // REHLDS_FIXES

void COM_UnMunge2(unsigned char *data, int len, int seq)
{
	int i;
	int mungelen;
	int c;
	int *pc;
	unsigned char *p;
	int j;

	mungelen = len & ~3;
	mungelen /= 4;

	for (i = 0; i < mungelen; i++)
	{
		pc = (int *)&data[i * 4];
		c = *pc;
		c ^= seq;

		p = (unsigned char *)&c;
		for (j = 0; j < 4; j++)
		{
			*p++ ^= (0xa5 | (j << j) | j | mungify_table2[(i + j) & 0x0f]);
		}

		c = LongSwap(c);
		c ^= ~seq;
		*pc = c;
	}
}
#endif // REHLDS_FIXES

void COM_Munge3(unsigned char *data, int len, int seq)
{
	int i;
	int mungelen;
	int c;
	int *pc;
	unsigned char *p;
	int j;

	mungelen = len & ~3;
	mungelen /= 4;

	for (i = 0; i < mungelen; i++)
	{
		pc = (int *)&data[i * 4];
		c = *pc;
		c ^= ~seq;
		c = LongSwap(c);

		p = (unsigned char *)&c;
		for (j = 0; j < 4; j++)
		{
			*p++ ^= (0xa5 | (j << j) | j | mungify_table3[(i + j) & 0x0f]);
		}

		c ^= seq;
		*pc = c;
	}
}

NOXREF void COM_UnMunge3(unsigned char *data, int len, int seq)
{
	NOXREFCHECK;

	int i;
	int mungelen;
	int c;
	int *pc;
	unsigned char *p;
	int j;

	mungelen = len & ~3;
	mungelen /= 4;

	for (i = 0; i < mungelen; i++)
	{
		pc = (int *)&data[i * 4];
		c = *pc;
		c ^= seq;

		p = (unsigned char *)&c;
		for (j = 0; j < 4; j++)
		{
			*p++ ^= (0xa5 | (j << j) | j | mungify_table3[(i + j) & 0x0f]);
		}

		c = LongSwap(c);
		c ^= ~seq;
		*pc = c;
	}
}

typedef struct
{
	unsigned char chunkID[4];
	long chunkSize;
	short wFormatTag;
	unsigned short wChannels;
	unsigned long dwSamplesPerSec;
	unsigned long dwAvgBytesPerSec;
	unsigned short wBlockAlign;
	unsigned short wBitsPerSample;
} FormatChunk;

const int WAVE_HEADER_LENGTH = 128;

unsigned int EXT_FUNC COM_GetApproxWavePlayLength(const char *filepath)
{
	char buf[WAVE_HEADER_LENGTH + 1];
	int filelength;
	FileHandle_t hFile;
	FormatChunk format;

	hFile = FS_Open(filepath, "rb");

	if (hFile)
	{
		filelength = FS_Size(hFile);

		if (filelength <= WAVE_HEADER_LENGTH)
			return 0;

		FS_Read(buf, WAVE_HEADER_LENGTH, 1, hFile);
		FS_Close(hFile);

		buf[WAVE_HEADER_LENGTH] = 0;

		if (!Q_strnicmp(buf, "RIFF", 4) && !Q_strnicmp(&buf[8], "WAVE", 4) && !Q_strnicmp(&buf[12], "fmt ", 4))
		{
			Q_memcpy(&format, &buf[12], sizeof(FormatChunk));

			filelength -= WAVE_HEADER_LENGTH;

			if (format.dwAvgBytesPerSec > 999)
			{
				return filelength / (format.dwAvgBytesPerSec / 1000);
			}

			return 1000 * filelength / format.dwAvgBytesPerSec;
		}
	}

	return 0;
}

#endif // COM_Functions_region
