
/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2019 */

#ifndef CS_LLVM_SUPPORT_MATHEXTRAS_H
#define CS_LLVM_SUPPORT_MATHEXTRAS_H

#if defined(_WIN32_WCE) && (_WIN32_WCE < 0x800)
#include "windowsce/intrin.h"
#elif defined(_MSC_VER)
#include <intrin.h>
#endif

#ifndef __cplusplus
#if defined (WIN32) || defined (WIN64) || defined (_WIN32) || defined (_WIN64)
#define inline /* inline */
#endif
#endif


static inline uint32_t Hi_32(uint64_t Value) {
	return (uint32_t)(Value >> 32);
}

static inline uint32_t Lo_32(uint64_t Value) {
	return (uint32_t)(Value);
}

static inline bool isUIntN(unsigned N, uint64_t x) {
	return x == (x & (~0ULL >> (64 - N)));
}


static inline bool isMask_32(uint32_t Value) {
	return Value && ((Value + 1) & Value) == 0;
}

static inline bool isMask_64(uint64_t Value) {
	return Value && ((Value + 1) & Value) == 0;
}

static inline bool isShiftedMask_32(uint32_t Value) {
	return isMask_32((Value - 1) | Value);
}

static inline bool isShiftedMask_64(uint64_t Value) {
	return isMask_64((Value - 1) | Value);
}

static inline bool isPowerOf2_32(uint32_t Value) {
	return Value && !(Value & (Value - 1));
}

static inline unsigned CountLeadingZeros_32(uint32_t Value) {
	unsigned Count; // result
#if __GNUC__ >= 4
#if !defined(__ppc__) && !defined(__ppc64__)
	if (!Value) return 32;
#endif
	Count = __builtin_clz(Value);
#else
	unsigned Shift;
	if (!Value) return 32;
	Count = 0;
	for (Shift = 32 >> 1; Shift; Shift >>= 1) {
		uint32_t Tmp = Value >> Shift;
		if (Tmp) {
			Value = Tmp;
		} else {
			Count |= Shift;
		}
	}
#endif
	return Count;
}

static inline unsigned CountLeadingOnes_32(uint32_t Value) {
	return CountLeadingZeros_32(~Value);
}

static inline unsigned CountLeadingZeros_64(uint64_t Value) {
	unsigned Count; // result
#if __GNUC__ >= 4
#if !defined(__ppc__) && !defined(__ppc64__)
	if (!Value) return 64;
#endif
	Count = __builtin_clzll(Value);
#else
#ifndef _MSC_VER
	unsigned Shift;
	if (sizeof(long) == sizeof(int64_t))
	{
		if (!Value) return 64;
		Count = 0;
		for (Shift = 64 >> 1; Shift; Shift >>= 1) {
			uint64_t Tmp = Value >> Shift;
			if (Tmp) {
				Value = Tmp;
			} else {
				Count |= Shift;
			}
		}
	}
	else
#endif
	{
		uint32_t Hi = Hi_32(Value);

		if (Hi) {
			Count = CountLeadingZeros_32(Hi);
		} else {
			uint32_t Lo = Lo_32(Value);
			Count = CountLeadingZeros_32(Lo)+32;
		}
	}
#endif
	return Count;
}

static inline unsigned CountLeadingOnes_64(uint64_t Value) {
	return CountLeadingZeros_64(~Value);
}

static inline unsigned CountTrailingZeros_32(uint32_t Value) {
#if __GNUC__ >= 4
	return Value ? __builtin_ctz(Value) : 32;
#else
	static const unsigned Mod37BitPosition[] = {
		32, 0, 1, 26, 2, 23, 27, 0, 3, 16, 24, 30, 28, 11, 0, 13,
		4, 7, 17, 0, 25, 22, 31, 15, 29, 10, 12, 6, 0, 21, 14, 9,
		5, 20, 8, 19, 18
	};
	return Mod37BitPosition[((1 + ~Value) & Value) % 37];
#endif
}

static inline unsigned CountTrailingOnes_32(uint32_t Value) {
	return CountTrailingZeros_32(~Value);
}

static inline unsigned CountTrailingZeros_64(uint64_t Value) {
#if __GNUC__ >= 4
	return Value ? __builtin_ctzll(Value) : 64;
#else
	static const unsigned Mod67Position[] = {
		64, 0, 1, 39, 2, 15, 40, 23, 3, 12, 16, 59, 41, 19, 24, 54,
		4, 64, 13, 10, 17, 62, 60, 28, 42, 30, 20, 51, 25, 44, 55,
		47, 5, 32, 65, 38, 14, 22, 11, 58, 18, 53, 63, 9, 61, 27,
		29, 50, 43, 46, 31, 37, 21, 57, 52, 8, 26, 49, 45, 36, 56,
		7, 48, 35, 6, 34, 33, 0
	};
	return Mod67Position[((1 + ~Value) & Value) % 67];
#endif
}

static inline unsigned CountTrailingOnes_64(uint64_t Value) {
	return CountTrailingZeros_64(~Value);
}

static inline unsigned CountPopulation_32(uint32_t Value) {
#if __GNUC__ >= 4
	return __builtin_popcount(Value);
#else
	uint32_t v = Value - ((Value >> 1) & 0x55555555);
	v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
	return (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
#endif
}

static inline unsigned CountPopulation_64(uint64_t Value) {
#if __GNUC__ >= 4
	return __builtin_popcountll(Value);
#else
	uint64_t v = Value - ((Value >> 1) & 0x5555555555555555ULL);
	v = (v & 0x3333333333333333ULL) + ((v >> 2) & 0x3333333333333333ULL);
	v = (v + (v >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
	return (uint64_t)((v * 0x0101010101010101ULL) >> 56);
#endif
}

static inline unsigned Log2_32(uint32_t Value) {
	return 31 - CountLeadingZeros_32(Value);
}

static inline unsigned Log2_64(uint64_t Value) {
	return 63 - CountLeadingZeros_64(Value);
}

static inline unsigned Log2_32_Ceil(uint32_t Value) {
	return 32-CountLeadingZeros_32(Value-1);
}

static inline unsigned Log2_64_Ceil(uint64_t Value) {
	return 64-CountLeadingZeros_64(Value-1);
}

static inline uint64_t GreatestCommonDivisor64(uint64_t A, uint64_t B) {
	while (B) {
		uint64_t T = B;
		B = A % B;
		A = T;
	}
	return A;
}

static inline double BitsToDouble(uint64_t Bits) {
	union {
		uint64_t L;
		double D;
	} T;
	T.L = Bits;
	return T.D;
}

static inline float BitsToFloat(uint32_t Bits) {
	union {
		uint32_t I;
		float F;
	} T;
	T.I = Bits;
	return T.F;
}

static inline uint64_t DoubleToBits(double Double) {
	union {
		uint64_t L;
		double D;
	} T;
	T.D = Double;
	return T.L;
}

static inline uint32_t FloatToBits(float Float) {
	union {
		uint32_t I;
		float F;
	} T;
	T.F = Float;
	return T.I;
}

static inline uint64_t MinAlign(uint64_t A, uint64_t B) {
	return (A | B) & (1 + ~(A | B));
}

static inline uint64_t NextPowerOf2(uint64_t A) {
	A |= (A >> 1);
	A |= (A >> 2);
	A |= (A >> 4);
	A |= (A >> 8);
	A |= (A >> 16);
	A |= (A >> 32);
	return A + 1;
}

static inline uint64_t RoundUpToAlignment(uint64_t Value, uint64_t Align) {
	return ((Value + Align - 1) / Align) * Align;
}

static inline uint64_t OffsetToAlignment(uint64_t Value, uint64_t Align) {
	return RoundUpToAlignment(Value, Align) - Value;
}

static inline int64_t abs64(int64_t x) {
	return (x < 0) ? -x : x;
}

static inline int32_t SignExtend32(uint32_t X, unsigned B) {
	return (int32_t)(X << (32 - B)) >> (32 - B);
}

static inline int64_t SignExtend64(uint64_t X, unsigned B) {
	return (int64_t)(X << (64 - B)) >> (64 - B);
}

static inline unsigned int countLeadingZeros(int x)
{
	int i;
	const unsigned bits = sizeof(x) * 8;
	unsigned count = bits;

	if (x < 0) {
		return 0;
	}
	for (i = bits; --i; ) {
		if (x == 0) break;
		count--;
		x >>= 1;
	}

	return count;
}

#endif
