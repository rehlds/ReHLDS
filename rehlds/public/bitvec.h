/*
*
*    Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*    This product contains software technology licensed from Id
*    Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*    All Rights Reserved.
*
*    Use, distribution, and modification of this source code and/or resulting
*    object code is restricted to non-commercial enhancements to products from
*    Valve LLC.  All other use, distribution, or modification is prohibited
*    without written permission from Valve LLC.
*
*/

#pragma once

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

class CBitVecAccessor
{
public:
	CBitVecAccessor(uint32_t *pDWords, int iBit);

	void operator=(int val);
	operator uint32_t();

private:
	uint32_t *m_pDWords;
	int	m_iBit;
};

// Support functions

#define LOG2_BITS_PER_INT	5
#define BITS_PER_INT		32

#if _WIN32
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#endif

inline int FirstBitInWord(unsigned int elem, int offset)
{
#if _WIN32
	if (!elem)
		return -1;

	unsigned long out;
	_BitScanForward(&out, elem);
	return out + offset;

#else
	static unsigned firstBitLUT[256] =
	{
		0,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,
		3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,
		3,0,1,0,2,0,1,0,7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,
		3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0
	};

	unsigned elemByte;

	elemByte = (elem & 0xFF);
	if (elemByte)
		return offset + firstBitLUT[elemByte];

	elem >>= 8;
	offset += 8;
	elemByte = (elem & 0xFF);
	if (elemByte)
		return offset + firstBitLUT[elemByte];

	elem >>= 8;
	offset += 8;
	elemByte = (elem & 0xFF);
	if (elemByte)
		return offset + firstBitLUT[elemByte];

	elem >>= 8;
	offset += 8;
	elemByte = (elem & 0xFF);
	if (elemByte)
		return offset + firstBitLUT[elemByte];

	return -1;
#endif
}

inline unsigned GetEndMask(int numBits)
{
	static unsigned bitStringEndMasks[] =
	{
		0xffffffff,
		0x00000001,
		0x00000003,
		0x00000007,
		0x0000000f,
		0x0000001f,
		0x0000003f,
		0x0000007f,
		0x000000ff,
		0x000001ff,
		0x000003ff,
		0x000007ff,
		0x00000fff,
		0x00001fff,
		0x00003fff,
		0x00007fff,
		0x0000ffff,
		0x0001ffff,
		0x0003ffff,
		0x0007ffff,
		0x000fffff,
		0x001fffff,
		0x003fffff,
		0x007fffff,
		0x00ffffff,
		0x01ffffff,
		0x03ffffff,
		0x07ffffff,
		0x0fffffff,
		0x1fffffff,
		0x3fffffff,
		0x7fffffff,
	};

	return bitStringEndMasks[numBits % BITS_PER_INT];
}


inline int CalcNumIntsForBits(int numBits)	{ return (numBits + (BITS_PER_INT - 1)) / BITS_PER_INT; }

// http://bits.stephan-brumme.com/PopulationCount.html
// http://graphics.stanford.edu/~seander/bithacks.html#PopulationCountSetParallel
inline unsigned int PopulationCount(uint32_t v)
{
	uint32_t const w = v - ((v >> 1) & 0x55555555);
	uint32_t const x = (w & 0x33333333) + ((w >> 2) & 0x33333333);
	return (((x + (x >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
}

inline unsigned int PopulationCount(uint64_t v)
{
	uint64_t const w = v - ((v >> 1) & 0x5555555555555555ull);
	uint64_t const x = (w & 0x3333333333333333ull) + ((w >> 2) & 0x3333333333333333ull);
	return (((x + (x >> 4)) & 0x0F0F0F0F0F0F0F0Full) * 0x0101010101010101ull) >> 56;
}

inline unsigned int PopulationCount(uint16_t v)
{
	uint16_t const w = v - ((v >> 1) & 0x5555);
	uint16_t const x = (w & 0x3333) + ((w >> 2) & 0x3333);
	return (((x + (x >> 4)) & 0x0F0F) * 0x101) >> 8;
}


inline unsigned int PopulationCount(uint8_t v)
{
	uint8_t const w = v - ((v >> 1) & 0x55);
	uint8_t const x = (w & 0x33) + ((w >> 2) & 0x33);
	return (x + (x >> 4)) & 0x0F;
}

#define BitVec_Bit(bitNum) (1 << ((bitNum) & (BITS_PER_INT - 1)))
#define BitVec_BitInByte(bitNum) (1 << ((bitNum) & 7))
#define BitVec_Int(bitNum) ((bitNum) >> LOG2_BITS_PER_INT)

// template CBitVecT
//
// Defines the operations relevant to any bit array. Simply requires a base
// class that implements GetNumBits(), Base(), GetNumDWords() & ValidateOperand()
//
// CVarBitVec and CBitVec<int> are the actual classes generally used
// by clients
//

template <class BASE_OPS>
class CBitVecT: public BASE_OPS
{
public:
	CBitVecT();
	explicit CBitVecT(int numBits);			// Must be initialized with the number of bits

	void Init(int val = 0);

	// Access the bits like an array.
	CBitVecAccessor	operator[](int i);
	uint32_t operator[](int i)const;

	// Do NOT override bitwise operators (see note in header)
	void And(const CBitVecT &andStr, CBitVecT *out) const;
	void Or(const CBitVecT &orStr, CBitVecT *out) const;
	void Xor(const CBitVecT &orStr, CBitVecT *out) const;

	void Not(CBitVecT *out) const;

	void CopyTo(CBitVecT *out) const;
	void Copy(const CBitVecT<BASE_OPS> &other, int nBits = -1);
	bool Compare(const CBitVecT<BASE_OPS> &other, int nBits = -1) const;

	bool IsAllClear() const;		// Are all bits zero?
	bool IsAllSet() const;		// Are all bits one?

	uint32_t Get(uint32_t bitNum) const;
	bool IsBitSet(int bitNum) const;
	void Set(int bitNum);
	void Set(int bitNum, bool bNewVal);
	void Clear(int bitNum);

	bool TestAndSet(int bitNum);

	void Set(uint32_t offset, uint32_t mask);
	void Clear(uint32_t offset, uint32_t mask);
	uint32_t Get(uint32_t offset, uint32_t mask);

	void SetAll();		// Sets all bits
	void ClearAll();	// Clears all bits

	uint32_t GetDWord(int i) const;
	void SetDWord(int i, uint32_t val);
	unsigned int PopulationCount()const;

	CBitVecT<BASE_OPS>&	operator=(const CBitVecT<BASE_OPS> &other)	{ other.CopyTo(this); return *this; }
	bool operator==(const CBitVecT<BASE_OPS> &other)				{ return Compare(other); }
	bool operator!=(const CBitVecT<BASE_OPS> &other)				{ return !operator==(other); }

	static void GetOffsetMaskForBit(uint32_t bitNum, uint32_t *pOffset, uint32_t *pMask)
	{
		*pOffset = BitVec_Int(bitNum);
		*pMask = BitVec_Bit(bitNum);
	}
};

// class CVarBitVecBase
//
// Defines the operations necessary for a variable sized bit array
template <typename BITCOUNTTYPE>
class CVarBitVecBase
{
public:
	bool IsFixedSize() const		{ return false; }
	int GetNumBits() const			{ return m_numBits; }
	void Resize(int numBits, bool bClearAll = false);		// resizes bit array

	int GetNumDWords() const		{ return m_numInts; }
	uint32_t *Base()				{ return m_pInt;	}
	const uint32_t *Base() const	{ return m_pInt;	}

	void Attach(uint32_t *pBits, int numBits);
	bool Detach(uint32_t **ppBits, int *pNumBits);

	int FindNextSetBit(int iStartBit) const; // returns -1 if no set bit was found

protected:
	CVarBitVecBase();
	explicit CVarBitVecBase(int numBits);
	CVarBitVecBase(const CVarBitVecBase<BITCOUNTTYPE> &from);
	CVarBitVecBase &operator=(const CVarBitVecBase<BITCOUNTTYPE> &from);
	~CVarBitVecBase();

	void ValidateOperand(const CVarBitVecBase<BITCOUNTTYPE> &operand) const	{ assert(GetNumBits() == operand.GetNumBits()); }
	unsigned GetEndMask() const { return ::GetEndMask(GetNumBits()); }

private:

	BITCOUNTTYPE m_numBits;			// Number of bits in the bitstring
	BITCOUNTTYPE m_numInts;			// Number of ints to needed to store bitstring
	uint32_t m_iBitStringStorage;	// If the bit string fits in one int, it goes here
	uint32_t *m_pInt;				// Array of ints containing the bitstring

	void AllocInts(int numInts);	// Free the allocated bits
	void ReallocInts(int numInts);
	void FreeInts();				// Free the allocated bits
};

// class CFixedBitVecBase
//
// Defines the operations necessary for a fixed sized bit array.
//

template <int bits> struct BitCountToEndMask_t { };
template <> struct BitCountToEndMask_t< 0> { enum { MASK = 0xffffffff }; };
template <> struct BitCountToEndMask_t< 1> { enum { MASK = 0x00000001 }; };
template <> struct BitCountToEndMask_t< 2> { enum { MASK = 0x00000003 }; };
template <> struct BitCountToEndMask_t< 3> { enum { MASK = 0x00000007 }; };
template <> struct BitCountToEndMask_t< 4> { enum { MASK = 0x0000000f }; };
template <> struct BitCountToEndMask_t< 5> { enum { MASK = 0x0000001f }; };
template <> struct BitCountToEndMask_t< 6> { enum { MASK = 0x0000003f }; };
template <> struct BitCountToEndMask_t< 7> { enum { MASK = 0x0000007f }; };
template <> struct BitCountToEndMask_t< 8> { enum { MASK = 0x000000ff }; };
template <> struct BitCountToEndMask_t< 9> { enum { MASK = 0x000001ff }; };
template <> struct BitCountToEndMask_t<10> { enum { MASK = 0x000003ff }; };
template <> struct BitCountToEndMask_t<11> { enum { MASK = 0x000007ff }; };
template <> struct BitCountToEndMask_t<12> { enum { MASK = 0x00000fff }; };
template <> struct BitCountToEndMask_t<13> { enum { MASK = 0x00001fff }; };
template <> struct BitCountToEndMask_t<14> { enum { MASK = 0x00003fff }; };
template <> struct BitCountToEndMask_t<15> { enum { MASK = 0x00007fff }; };
template <> struct BitCountToEndMask_t<16> { enum { MASK = 0x0000ffff }; };
template <> struct BitCountToEndMask_t<17> { enum { MASK = 0x0001ffff }; };
template <> struct BitCountToEndMask_t<18> { enum { MASK = 0x0003ffff }; };
template <> struct BitCountToEndMask_t<19> { enum { MASK = 0x0007ffff }; };
template <> struct BitCountToEndMask_t<20> { enum { MASK = 0x000fffff }; };
template <> struct BitCountToEndMask_t<21> { enum { MASK = 0x001fffff }; };
template <> struct BitCountToEndMask_t<22> { enum { MASK = 0x003fffff }; };
template <> struct BitCountToEndMask_t<23> { enum { MASK = 0x007fffff }; };
template <> struct BitCountToEndMask_t<24> { enum { MASK = 0x00ffffff }; };
template <> struct BitCountToEndMask_t<25> { enum { MASK = 0x01ffffff }; };
template <> struct BitCountToEndMask_t<26> { enum { MASK = 0x03ffffff }; };
template <> struct BitCountToEndMask_t<27> { enum { MASK = 0x07ffffff }; };
template <> struct BitCountToEndMask_t<28> { enum { MASK = 0x0fffffff }; };
template <> struct BitCountToEndMask_t<29> { enum { MASK = 0x1fffffff }; };
template <> struct BitCountToEndMask_t<30> { enum { MASK = 0x3fffffff }; };
template <> struct BitCountToEndMask_t<31> { enum { MASK = 0x7fffffff }; };

template <int NUM_BITS>
class CFixedBitVecBase
{
public:
	bool IsFixedSize() const		{ return true; }
	int GetNumBits() const			{ return NUM_BITS; }
	void Resize(int numBits, bool bClearAll = false)
	{
		// for syntatic consistency (for when using templates)
		assert(numBits == NUM_BITS);
		if (bClearAll)
			memset(m_Ints, 0, NUM_INTS * sizeof(uint32_t));
	}

	int GetNumDWords() const		{ return NUM_INTS; }
	uint32_t *Base()				{ return m_Ints;	}
	const uint32_t *Base() const	{ return m_Ints;	}

	int FindNextSetBit(int iStartBit) const; // returns -1 if no set bit was found

protected:
	CFixedBitVecBase() {}
	explicit CFixedBitVecBase(int numBits)
	{
		// doesn't make sense, really. Supported to simplify templates & allow easy replacement of variable
		assert(numBits == NUM_BITS);
	}

	void ValidateOperand(const CFixedBitVecBase<NUM_BITS> &operand) const {} // no need, compiler does so statically

public: // for test code
	unsigned GetEndMask() const { return static_cast<unsigned>(BitCountToEndMask_t<NUM_BITS % BITS_PER_INT>::MASK); }

private:
	enum
	{
		NUM_INTS = (NUM_BITS + (BITS_PER_INT - 1)) / BITS_PER_INT
	};

	uint32_t m_Ints[(NUM_BITS + (BITS_PER_INT - 1)) / BITS_PER_INT];
};

//
// The actual classes used
//

// inheritance instead of typedef to allow forward declarations
template <class CountType = unsigned short>
class CVarBitVecT: public CBitVecT<CVarBitVecBase<CountType>>
{
public:
	CVarBitVecT()
	{
	}

	explicit CVarBitVecT(int numBits) :
		CBitVecT<CVarBitVecBase<CountType>>(numBits)
	{
	}
};

class CVarBitVec: public CVarBitVecT<unsigned short>
{
public:
	CVarBitVec() :
		CVarBitVecT<unsigned short>()
	{
	}

	explicit CVarBitVec(int nBitCount) :
		CVarBitVecT<unsigned short>(nBitCount)
	{
	}
};

class CLargeVarBitVec: public CBitVecT<CVarBitVecBase<int>>
{
public:
	CLargeVarBitVec()
	{
	}

	explicit CLargeVarBitVec(int numBits)
		: CBitVecT<CVarBitVecBase<int>>(numBits)
	{
	}
};

template <int NUM_BITS>
class CBitVec: public CBitVecT<CFixedBitVecBase<NUM_BITS>>
{
public:
	CBitVec()
	{
	}

	explicit CBitVec(int numBits) :
		CBitVecT<CFixedBitVecBase<NUM_BITS>>(numBits)
	{
	}
};

typedef CBitVec<32> CDWordBitVec;

template <typename BITCOUNTTYPE>
inline CVarBitVecBase<BITCOUNTTYPE>::CVarBitVecBase()
{
	memset(this, 0, sizeof(*this));
}

template <typename BITCOUNTTYPE>
inline CVarBitVecBase<BITCOUNTTYPE>::CVarBitVecBase(int numBits)
{
	assert(numBits);
	m_numBits	= numBits;

	// Figure out how many ints are needed
	m_numInts = CalcNumIntsForBits(numBits);
	m_pInt = NULL;
	AllocInts(m_numInts);
}

template <typename BITCOUNTTYPE>
inline CVarBitVecBase<BITCOUNTTYPE>::CVarBitVecBase(const CVarBitVecBase<BITCOUNTTYPE> &from)
{
	if (from.m_numInts)
	{
		m_numBits = from.m_numBits;
		m_numInts = from.m_numInts;
		m_pInt = NULL;
		AllocInts(m_numInts);
		memcpy(m_pInt, from.m_pInt, m_numInts * sizeof(int));
	}
	else
	{
		memset(this, 0, sizeof(*this));
	}
}

template <typename BITCOUNTTYPE>
inline CVarBitVecBase<BITCOUNTTYPE> &CVarBitVecBase<BITCOUNTTYPE>::operator=(const CVarBitVecBase<BITCOUNTTYPE> &from)
{
	Resize(from.GetNumBits());
	if (m_pInt)
		memcpy(m_pInt, from.m_pInt, m_numInts * sizeof(int));

	return (*this);
}

template <typename BITCOUNTTYPE>
inline CVarBitVecBase<BITCOUNTTYPE>::~CVarBitVecBase()
{
	FreeInts();
}

template <typename BITCOUNTTYPE>
inline void CVarBitVecBase<BITCOUNTTYPE>::Attach(uint32_t *pBits, int numBits)
{
	FreeInts();
	m_numBits = numBits;
	m_numInts = CalcNumIntsForBits(numBits);
	if (m_numInts > 1)
	{
		m_pInt = pBits;
	}
	else
	{
		m_iBitStringStorage = *pBits;
		m_pInt = &m_iBitStringStorage;
		free(pBits);
	}
}

template <typename BITCOUNTTYPE>
inline bool CVarBitVecBase<BITCOUNTTYPE>::Detach(uint32_t **ppBits, int *pNumBits)
{
	if (!m_numBits)
	{
		return false;
	}

	*pNumBits = m_numBits;
	if (m_numInts > 1)
	{
		*ppBits = m_pInt;
	}
	else
	{
		*ppBits = (uint32_t *)malloc(sizeof(uint32_t));
		**ppBits = m_iBitStringStorage;
		free(m_pInt);
	}

	memset(this, 0, sizeof(*this));
	return true;
}

template <class BASE_OPS>
inline CBitVecT<BASE_OPS>::CBitVecT()
{
	static_assert(sizeof(int) == 4, "bit vectors store bits in 32-bit ints");

	// Initialize bitstring by clearing all bits
	ClearAll();
}

template <class BASE_OPS>
inline CBitVecT<BASE_OPS>::CBitVecT(int numBits)
 : BASE_OPS(numBits)
{
	static_assert(sizeof(int) == 4, "bit vectors store bits in 32-bit ints");

	// Initialize bitstring by clearing all bits
	ClearAll();
}

template <class BASE_OPS>
inline CBitVecAccessor CBitVecT<BASE_OPS>::operator[](int i)
{
	assert(i >= 0 && i < this->GetNumBits());
	return CBitVecAccessor(this->Base(), i);
}

template <class BASE_OPS>
inline uint32_t CBitVecT<BASE_OPS>::operator[](int i)const
{
	assert(i >= 0 && i < this->GetNumBits());
	return this->Base()[i >> 5] & (1 << (i & 31));
}

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Init(int val)
{
	if (this->Base())
		memset(this->Base(), (val) ? 0xff : 0, this->GetNumDWords() * sizeof(int));
}

template <class BASE_OPS>
inline uint32_t CBitVecT<BASE_OPS>::Get(uint32_t bitNum) const
{
	assert(bitNum < (uint32_t)this->GetNumBits());
	const uint32_t *pInt = this->Base() + BitVec_Int(bitNum);
	return (*pInt & BitVec_Bit(bitNum));
}

template <class BASE_OPS>
inline bool CBitVecT<BASE_OPS>::IsBitSet(int bitNum) const
{
	assert(bitNum >= 0 && bitNum < this->GetNumBits());
	const uint32_t *pInt = this->Base() + BitVec_Int(bitNum);
	return ((*pInt & BitVec_Bit(bitNum)) != 0);
}

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Set(int bitNum)
{
	assert(bitNum >= 0 && bitNum < this->GetNumBits());
	uint32_t *pInt = this->Base() + BitVec_Int(bitNum);
	*pInt |= BitVec_Bit(bitNum);
}

template <class BASE_OPS>
inline bool CBitVecT<BASE_OPS>::TestAndSet(int bitNum)
{
	assert(bitNum >= 0 && bitNum < this->GetNumBits());
	uint32_t bitVecBit = BitVec_Bit(bitNum);
	uint32_t *pInt = this->Base() + BitVec_Int(bitNum);
	bool bResult = ((*pInt & bitVecBit) != 0);
	*pInt |= bitVecBit;
	return bResult;
}

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Clear(int bitNum)
{
	assert(bitNum >= 0 && bitNum < this->GetNumBits());
	uint32_t *pInt = this->Base() + BitVec_Int(bitNum);
	*pInt &= ~BitVec_Bit(bitNum);
}

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Set(int bitNum, bool bNewVal)
{
	uint32_t *pInt = this->Base() + BitVec_Int(bitNum);
	uint32_t bitMask = BitVec_Bit(bitNum);
	if (bNewVal)
	{
		*pInt |= bitMask;
	}
	else
	{
		*pInt &= ~bitMask;
	}
}

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Set(uint32_t offset, uint32_t mask)
{
	uint32_t *pInt = this->Base() + offset;
	*pInt |= mask;
}

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Clear(uint32_t offset, uint32_t mask)
{
	uint32_t *pInt = this->Base() + offset;
	*pInt &= ~mask;
}

template <class BASE_OPS>
inline uint32_t CBitVecT<BASE_OPS>::Get(uint32_t offset, uint32_t mask)
{
	uint32_t *pInt = this->Base() + offset;
	return (*pInt & mask);
}

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::And(const CBitVecT &addStr, CBitVecT *out) const
{
	this->ValidateOperand(addStr);
	this->ValidateOperand(*out);

	uint32_t *pDest				= out->Base();
	const uint32_t *pOperand1	= this->Base();
	const uint32_t *pOperand2	= addStr.Base();

	for (int i = this->GetNumDWords() - 1; i >= 0 ; --i)
	{
		pDest[i] = pOperand1[i] & pOperand2[i];
	}
}

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Or(const CBitVecT &orStr, CBitVecT *out) const
{
	this->ValidateOperand(orStr);
	this->ValidateOperand(*out);

	uint32_t *pDest				= out->Base();
	const uint32_t *pOperand1	= this->Base();
	const uint32_t *pOperand2	= orStr.Base();

	for (int i = this->GetNumDWords() - 1; i >= 0; --i)
	{
		pDest[i] = pOperand1[i] | pOperand2[i];
	}
}

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Xor(const CBitVecT &xorStr, CBitVecT *out) const
{
	uint32_t *pDest				= out->Base();
	const uint32_t *pOperand1	= this->Base();
	const uint32_t *pOperand2	= xorStr.Base();

	for (int i = this->GetNumDWords() - 1; i >= 0; --i)
	{
		pDest[i] = pOperand1[i] ^ pOperand2[i];
	}
}

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Not(CBitVecT *out) const
{
	this->ValidateOperand(*out);

	uint32_t *pDest				= out->Base();
	const uint32_t *pOperand	= this->Base();

	for (int i = this->GetNumDWords() - 1; i >= 0; --i)
	{
		pDest[i] = ~(pOperand[i]);
	}
}

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::CopyTo(CBitVecT *out) const
{
	out->Resize(this->GetNumBits());

	this->ValidateOperand(*out);
	assert(out != this);

	memcpy(out->Base(), this->Base(), this->GetNumDWords() * sizeof(int));
}

template <class BASE_OPS>
inline bool CBitVecT<BASE_OPS>::IsAllClear() const
{
	// Number of available bits may be more than the number
	// actually used, so make sure to mask out unused bits
	// before testing for zero
	(const_cast<CBitVecT *>(this))->Base()[this->GetNumDWords() - 1] &= CBitVecT<BASE_OPS>::GetEndMask(); // external semantics of const retained

	for (int i = this->GetNumDWords() - 1; i >= 0; --i)
	{
		if (this->Base()[i] !=0)
		{
			return false;
		}
	}
	return true;
}

template <class BASE_OPS>
inline bool CBitVecT<BASE_OPS>::IsAllSet() const
{
	// Number of available bits may be more than the number
	// actually used, so make sure to mask out unused bits
	// before testing for set bits
	(const_cast<CBitVecT *>(this))->Base()[this->GetNumDWords() - 1] |= ~CBitVecT<BASE_OPS>::GetEndMask(); // external semantics of const retained

	for (int i = this->GetNumDWords() - 1; i >= 0; i--)
	{
		if (this->Base()[i] != ~0)
			return false;
	}

	return true;
}

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::SetAll()
{
	if (this->Base())
		memset(this->Base(), 0xff, this->GetNumDWords() * sizeof(int));
}

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::ClearAll()
{
	if (this->Base())
		memset(this->Base(), 0, this->GetNumDWords() * sizeof(int));
}

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Copy(const CBitVecT<BASE_OPS> &other, int nBits)
{
	if (nBits == - 1)
	{
		nBits = other.GetNumBits();
	}

	this->Resize(nBits);

	this->ValidateOperand(other);
	assert(&other != this);

	memcpy(this->Base(), other.Base(), this->GetNumDWords() * sizeof(uint32_t));
}

template <class BASE_OPS>
inline bool CBitVecT<BASE_OPS>::Compare(const CBitVecT<BASE_OPS> &other, int nBits) const
{
	if (nBits == - 1)
	{
		if (other.GetNumBits() != this->GetNumBits())
			return false;

		nBits = other.GetNumBits();
	}

	if (nBits > other.GetNumBits() || nBits > this->GetNumBits())
		return false;

	(const_cast<CBitVecT *>(this))->Base()[this->GetNumDWords() - 1] &= CBitVecT<BASE_OPS>::GetEndMask(); // external semantics of const retained
	(const_cast<CBitVecT *>(&other))->Base()[this->GetNumDWords() - 1] &= other.CBitVecT<BASE_OPS>::GetEndMask(); // external semantics of const retained

	int nBytes = (nBits + 7) >> 3;

	return (memcmp(this->Base(), other.Base(), nBytes) == 0);
}

template <class BASE_OPS>
inline uint32_t CBitVecT<BASE_OPS>::GetDWord(int i) const
{
	assert(i >= 0 && i < this->GetNumDWords());
	return this->Base()[i];
}

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::SetDWord(int i, uint32_t val)
{
	assert(i >= 0 && i < this->GetNumDWords());
	this->Base()[i] = val;
}

template <class BASE_OPS>
inline uint32_t CBitVecT<BASE_OPS>::PopulationCount() const
{
	int nDwordCount = this->GetNumDWords();
	const uint32_t *pBase = this->Base();
	uint32_t nCount = 0;
	for (int i = 0; i < nDwordCount; i++)
	{
		nCount += ::PopulationCount(pBase[i]);
	}

	return nCount;
}

inline unsigned GetStartBitMask(int startBit)
{
	static unsigned int g_StartMask[32] =
	{
		0xffffffff,
		0xfffffffe,
		0xfffffffc,
		0xfffffff8,
		0xfffffff0,
		0xffffffe0,
		0xffffffc0,
		0xffffff80,
		0xffffff00,
		0xfffffe00,
		0xfffffc00,
		0xfffff800,
		0xfffff000,
		0xffffe000,
		0xffffc000,
		0xffff8000,
		0xffff0000,
		0xfffe0000,
		0xfffc0000,
		0xfff80000,
		0xfff00000,
		0xffe00000,
		0xffc00000,
		0xff800000,
		0xff000000,
		0xfe000000,
		0xfc000000,
		0xf8000000,
		0xf0000000,
		0xe0000000,
		0xc0000000,
		0x80000000,
	};

	return g_StartMask[startBit & 31];
}

inline bool BitVec_IsBitSet(const uint32_t *pBase, int nBitNum)
{
	const uint32_t *pInt = pBase + BitVec_Int(nBitNum);
	return ((*pInt & BitVec_Bit(nBitNum)) != 0);
}

inline void Bitvec_Set(uint32_t *pBits, int nBitNum)
{
	uint32_t *pInt = pBits + BitVec_Int(nBitNum);
	*pInt |= BitVec_Bit(nBitNum);
}

inline void Bitvec_Unset(uint32_t *pBits, int nBitNum)
{
	uint32_t *pInt = pBits + BitVec_Int(nBitNum);
	*pInt &= ~BitVec_Bit(nBitNum);
}

inline bool BitVec_TestAndSetBit(uint32_t *pBase, int nBitNum)
{
	uint32_t *pInt = pBase + BitVec_Int(nBitNum);
	uint32_t nBits = *pInt;
	uint32_t nSetMask = BitVec_Bit(nBitNum);
	*pInt = nBits | nSetMask;
	return (nBits & nSetMask) ? true : false;
}

inline void BitVec_ClearAll(uint32_t *pDst, int nDWords, uint32_t nEndMask)
{
	if (nDWords > 1)
	{
		memset(pDst, 0, (nDWords - 1) * sizeof(uint32_t));
	}

	pDst[nDWords - 1] &= ~nEndMask;
}

inline void BitVec_ClearAll(uint32_t *pDst, int nDWords)
{
	memset(pDst, 0, nDWords * sizeof(uint32_t));
}

inline void BitVec_SetAll(uint32_t *pDst, int nDWords, uint32_t nEndMask)
{
	if (nDWords > 1)
	{
		memset(pDst, 0xff, (nDWords - 1) * sizeof(uint32_t));
	}

	pDst[nDWords - 1] |= nEndMask;
}

inline void BitVec_SetAll(uint32_t *pDst, int nDWords)
{
	memset(pDst, 0xff, nDWords * sizeof(uint32_t));
}

inline void BitVec_Or(uint32_t *pDst, const uint32_t *pSrc, int nDWords, uint32_t nEndMask)
{
	uint32_t *pEnd = pDst + nDWords - 1;
	for (; pDst < pEnd; pSrc++, pDst++)
	{
		*pDst = *pDst | *pSrc;
	}
	*pDst = *pDst | (*pSrc & nEndMask);
}

inline void BitVec_Or(uint32_t *pDst, const uint32_t *pSrc, int nDWords)
{
	uint32_t *pEnd = pDst + nDWords;
	for (; pDst < pEnd; pSrc++, pDst++)
	{
		*pDst = *pDst | *pSrc;
	}
}

inline void BitVec_AndNot(uint32_t *pDst, const uint32_t *pSrc, const uint32_t *pAndNot, int nDWords)
{
	uint32_t *pEnd = pDst + nDWords;
	for (; pDst < pEnd; pSrc++, pDst++, pAndNot++)
	{
		*pDst = *pSrc & ~(*pAndNot);
	}
}

inline bool BitVec_IsAnySet(const uint32_t *pDst, int nDWords)
{
	const uint32_t *pEnd = pDst + nDWords;
	for (; pDst < pEnd; pDst++)
	{
		if (*pDst != 0)
			return true;
	}

	return false;
}

inline int BitVec_CountNewBits(const uint32_t *pOld, const uint32_t *pNew, int nDWords, uint32_t nEndMask)
{
	// NOTE - this assumes that any unused bits are unchanged between pOld and pNew
	int nNewBits = 0;

	const uint32_t *pEnd = pNew + nDWords - 1;
	for (; pNew < pEnd; pOld++, pNew++)
	{
		uint32_t diff = *pNew & ~*pOld;
		nNewBits += PopulationCount(diff);
	}

	nNewBits += PopulationCount((*pNew & ~*pOld) & nEndMask);

	return nNewBits;
}

template <typename BITCOUNTTYPE>
inline int CVarBitVecBase<BITCOUNTTYPE>::FindNextSetBit(int startBit) const
{
	if (startBit < GetNumBits())
	{
		int wordIndex = BitVec_Int(startBit);
		unsigned int startMask = GetStartBitMask(startBit);
		int lastWord = GetNumDWords() - 1;

		// handle non dword lengths
		if ((GetNumBits() % BITS_PER_INT) != 0)
		{
			unsigned int elem = Base()[wordIndex];
			elem &= startMask;
			if (wordIndex == lastWord)
			{
				elem &= (GetEndMask());

				// there's a bit remaining in this word
				if (elem)
					return FirstBitInWord(elem, wordIndex << 5);
			}
			else
			{
				// there's a bit remaining in this word
				if (elem)
					return FirstBitInWord(elem, wordIndex << 5);

				// iterate the words
				for (int i = wordIndex+1; i < lastWord; i++)
				{
					elem = Base()[i];
					if (elem)
						return FirstBitInWord(elem, i << 5);
				}

				elem = Base()[lastWord] & GetEndMask();
				if (elem)
					return FirstBitInWord(elem, lastWord << 5);
			}
		}
		else
		{
			const uint32_t *pCurElem = Base() + wordIndex;
			unsigned int elem = *pCurElem;
			elem &= startMask;
			do
			{
				if (elem)
					return FirstBitInWord(elem, wordIndex << 5);

				pCurElem++;
				elem = *pCurElem;
				wordIndex++;
			}
			while (wordIndex <= lastWord);
		}
	}

	return -1;
}

template <int NUM_BITS>
inline int CFixedBitVecBase<NUM_BITS>::FindNextSetBit(int startBit) const
{
	if (startBit < NUM_BITS)
	{
		int wordIndex = BitVec_Int(startBit);
		unsigned int startMask = GetStartBitMask(startBit);

		// handle non dword lengths
		if ((NUM_BITS % BITS_PER_INT) != 0)
		{
			unsigned int elem = Base()[wordIndex];
			elem &= startMask;
			if (wordIndex == NUM_INTS - 1)
			{
				elem &= (GetEndMask());

				// there's a bit remaining in this word
				if (elem)
					return FirstBitInWord(elem, wordIndex << 5);
			}
			else
			{
				// there's a bit remaining in this word
				if (elem)
					return FirstBitInWord(elem, wordIndex << 5);

				// iterate the words
				for (int i = wordIndex+1; i < NUM_INTS - 1; i++)
				{
					elem = Base()[i];
					if (elem)
						return FirstBitInWord(elem, i << 5);
				}

				elem = Base()[NUM_INTS - 1] & GetEndMask();
				if (elem)
					return FirstBitInWord(elem, (NUM_INTS - 1) << 5);
			}
		}
		else
		{
			const uint32_t *pCurElem = Base() + wordIndex;
			unsigned int elem = *pCurElem;
			elem &= startMask;
			do
			{
				if (elem)
					return FirstBitInWord(elem, wordIndex << 5);

				pCurElem++;
				elem = *pCurElem;
				wordIndex++;
			}
			while(wordIndex <= NUM_INTS - 1);
		}
	}

	return -1;
}

// Unrolled loops for some common sizes

template <>
inline void CBitVecT<CFixedBitVecBase<256>>::And(const CBitVecT &addStr, CBitVecT *out) const
{
	uint32_t *pDest				= out->Base();
	const uint32_t *pOperand1	= Base();
	const uint32_t *pOperand2	= addStr.Base();

	pDest[0] = pOperand1[0] & pOperand2[0];
	pDest[1] = pOperand1[1] & pOperand2[1];
	pDest[2] = pOperand1[2] & pOperand2[2];
	pDest[3] = pOperand1[3] & pOperand2[3];
	pDest[4] = pOperand1[4] & pOperand2[4];
	pDest[5] = pOperand1[5] & pOperand2[5];
	pDest[6] = pOperand1[6] & pOperand2[6];
	pDest[7] = pOperand1[7] & pOperand2[7];
}

template <>
inline bool CBitVecT<CFixedBitVecBase<256>>::IsAllClear() const
{
	const uint32_t *pInts = Base();
	return (pInts[0] == 0 && pInts[1] == 0 && pInts[2] == 0 && pInts[3] == 0 && pInts[4] == 0 && pInts[5] == 0 && pInts[6] == 0 && pInts[7] == 0);
}

template <>
inline void CBitVecT<CFixedBitVecBase<256>>::CopyTo(CBitVecT *out) const
{
	uint32_t *pDest			= out->Base();
	const uint32_t *pInts	= Base();

	pDest[0] = pInts[0];
	pDest[1] = pInts[1];
	pDest[2] = pInts[2];
	pDest[3] = pInts[3];
	pDest[4] = pInts[4];
	pDest[5] = pInts[5];
	pDest[6] = pInts[6];
	pDest[7] = pInts[7];
}

template <>
inline void CBitVecT<CFixedBitVecBase<128>>::And(const CBitVecT &addStr, CBitVecT *out) const
{
	uint32_t *pDest				= out->Base();
	const uint32_t *pOperand1	= Base();
	const uint32_t *pOperand2	= addStr.Base();

	pDest[0] = pOperand1[0] & pOperand2[0];
	pDest[1] = pOperand1[1] & pOperand2[1];
	pDest[2] = pOperand1[2] & pOperand2[2];
	pDest[3] = pOperand1[3] & pOperand2[3];
}

template <>
inline bool CBitVecT<CFixedBitVecBase<128>>::IsAllClear() const
{
	const uint32_t *pInts = Base();
	return (pInts[0] == 0 && pInts[1] == 0 && pInts[2] == 0 && pInts[3] == 0);
}

template <>
inline void CBitVecT<CFixedBitVecBase<128>>::CopyTo(CBitVecT *out) const
{
	uint32_t *pDest			= out->Base();
	const uint32_t *pInts	= Base();

	pDest[0] = pInts[0];
	pDest[1] = pInts[1];
	pDest[2] = pInts[2];
	pDest[3] = pInts[3];
}

template <>
inline void CBitVecT<CFixedBitVecBase<96>>::And(const CBitVecT &addStr, CBitVecT *out) const
{
	uint32_t *pDest				= out->Base();
	const uint32_t *pOperand1	= Base();
	const uint32_t *pOperand2	= addStr.Base();

	pDest[0] = pOperand1[0] & pOperand2[0];
	pDest[1] = pOperand1[1] & pOperand2[1];
	pDest[2] = pOperand1[2] & pOperand2[2];
}

template <>
inline bool CBitVecT<CFixedBitVecBase<96>>::IsAllClear() const
{
	const uint32_t *pInts = Base();
	return (pInts[0] == 0 && pInts[1] == 0 && pInts[2] == 0);
}

template <>
inline void CBitVecT<CFixedBitVecBase<96>>::CopyTo(CBitVecT *out) const
{
	uint32_t *pDest			= out->Base();
	const uint32_t *pInts	= Base();

	pDest[0] = pInts[0];
	pDest[1] = pInts[1];
	pDest[2] = pInts[2];
}

template <>
inline void CBitVecT<CFixedBitVecBase<64>>::And(const CBitVecT &addStr, CBitVecT *out) const
{
	uint32_t *pDest				= out->Base();
	const uint32_t *pOperand1	= Base();
	const uint32_t *pOperand2	= addStr.Base();

	pDest[0] = pOperand1[0] & pOperand2[0];
	pDest[1] = pOperand1[1] & pOperand2[1];
}

template <>
inline bool CBitVecT<CFixedBitVecBase<64>>::IsAllClear() const
{
	const uint32_t *pInts = Base();
	return (pInts[0] == 0 && pInts[1] == 0);
}

template <>
inline void CBitVecT<CFixedBitVecBase<64>>::CopyTo(CBitVecT *out) const
{
	uint32_t *pDest			= out->Base();
	const uint32_t *pInts	= Base();

	pDest[0] = pInts[0];
	pDest[1] = pInts[1];
}

template <>
inline void CBitVecT<CFixedBitVecBase<32>>::And(const CBitVecT &addStr, CBitVecT *out) const
{
	uint32_t *pDest				= out->Base();
	const uint32_t *pOperand1	= Base();
	const uint32_t *pOperand2	= addStr.Base();

	pDest[0] = pOperand1[0] & pOperand2[0];
}

template <>
inline bool CBitVecT<CFixedBitVecBase<32>>::IsAllClear() const
{
	const uint32_t *pInts = Base();
	return (pInts[0] == 0);
}

template <>
inline void CBitVecT<CFixedBitVecBase<32>>::CopyTo(CBitVecT *out) const
{
	uint32_t *pDest = out->Base();
	const uint32_t *pInts = Base();

	pDest[0] = pInts[0];
}

template <>
inline uint32_t CBitVecT<CFixedBitVecBase<32>>::Get(uint32_t bitNum) const
{
	return (*Base() & BitVec_Bit(bitNum));
}

template <>
inline bool CBitVecT<CFixedBitVecBase<32>>::IsBitSet(int bitNum) const
{
	return ((*Base() & BitVec_Bit(bitNum)) != 0);
}

template <>
inline void CBitVecT<CFixedBitVecBase<32>>::Set(int bitNum)
{
	*Base() |= BitVec_Bit(bitNum);
}

template <>
inline void CBitVecT<CFixedBitVecBase<32>>::Clear(int bitNum)
{
	*Base() &= ~BitVec_Bit(bitNum);
}

template <>
inline void CBitVecT<CFixedBitVecBase<32>>::Set(int bitNum, bool bNewVal)
{
	uint32_t bitMask = BitVec_Bit(bitNum);
	if (bNewVal)
	{
		*Base() |= bitMask;
	}
	else
	{
		*Base() &= ~bitMask;
	}
}

// Resizes the bit string to a new number of bits
template <typename BITCOUNTTYPE>
inline void CVarBitVecBase<BITCOUNTTYPE>::Resize(int resizeNumBits, bool bClearAll)
{
	assert(resizeNumBits >= 0 && ((BITCOUNTTYPE)resizeNumBits == resizeNumBits));

	int newIntCount = CalcNumIntsForBits(resizeNumBits);
	if (newIntCount != GetNumDWords())
	{
		if (Base())
		{
			ReallocInts(newIntCount);
			if (!bClearAll && resizeNumBits >= GetNumBits())
			{
				Base()[GetNumDWords() - 1] &= GetEndMask();
				memset(Base() + GetNumDWords(), 0, (newIntCount - GetNumDWords()) * sizeof(int));
			}
		}
		else
		{
			// Figure out how many ints are needed
			AllocInts(newIntCount);

			// Initialize bitstring by clearing all bits
			bClearAll = true;
		}

		m_numInts = newIntCount;
	}
	else if (!bClearAll && resizeNumBits >= GetNumBits() && Base())
	{
		Base()[GetNumDWords() - 1] &= GetEndMask();
	}

	if (bClearAll && Base())
	{
		memset(Base(), 0, newIntCount * sizeof(int));
	}

	// store the new size and end mask
	m_numBits = resizeNumBits;
}

// Allocate the storage for the ints
template <typename BITCOUNTTYPE>
inline void CVarBitVecBase<BITCOUNTTYPE>::AllocInts(int numInts)
{
	assert(!m_pInt);

	if (numInts == 0)
		return;

	if (numInts == 1)
	{
		m_pInt = &m_iBitStringStorage;
		return;
	}

	m_pInt = (uint32_t *)malloc(numInts * sizeof(int));
}

// Reallocate the storage for the ints
template <typename BITCOUNTTYPE>
inline void CVarBitVecBase<BITCOUNTTYPE>::ReallocInts(int numInts)
{
	assert(Base());
	if (numInts == 0)
	{
		FreeInts();
		return;
	}

	if (m_pInt == &m_iBitStringStorage)
	{
		if (numInts != 1)
		{
			m_pInt = ((uint32_t *)malloc(numInts * sizeof(int)));
			*m_pInt = m_iBitStringStorage;
		}

		return;
	}

	if (numInts == 1)
	{
		m_iBitStringStorage = *m_pInt;
		free(m_pInt);
		m_pInt = &m_iBitStringStorage;
		return;
	}

	m_pInt = (uint32_t *)realloc(m_pInt, numInts * sizeof(int));
}

// Free storage allocated with AllocInts
template <typename BITCOUNTTYPE>
inline void CVarBitVecBase<BITCOUNTTYPE>::FreeInts()
{
	if (m_numInts > 1)
		free(m_pInt);

	m_pInt = NULL;
}

// CBitVecAccessor inlines
inline CBitVecAccessor::CBitVecAccessor(uint32_t *pDWords, int iBit)
{
	m_pDWords = pDWords;
	m_iBit = iBit;
}

inline void CBitVecAccessor::operator=(int val)
{
	if (val)
		m_pDWords[m_iBit >> 5] |= (1 << (m_iBit & 31));
	else
		m_pDWords[m_iBit >> 5] &= ~(unsigned long)(1 << (m_iBit & 31));
}

inline CBitVecAccessor::operator uint32_t()
{
	return m_pDWords[m_iBit >> 5] & (1 << (m_iBit & 31));
}
