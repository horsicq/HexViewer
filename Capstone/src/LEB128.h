
/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2019 */

#ifndef CS_LLVM_SUPPORT_LEB128_H
#define CS_LLVM_SUPPORT_LEB128_H

#include "include/capstone/capstone.h"

static inline uint64_t decodeULEB128(const uint8_t *p, unsigned *n)
{
	const uint8_t *orig_p = p;
	uint64_t Value = 0;
	unsigned Shift = 0;
	do {
		Value += (uint64_t)(*p & 0x7f) << Shift;
		Shift += 7;
	} while (*p++ >= 128);
	if (n)
		*n = (unsigned)(p - orig_p);
	return Value;
}

#endif  // LLVM_SYSTEM_LEB128_H
