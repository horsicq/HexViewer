
/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2019 */

#ifndef CS_LLVM_TARGET_ARM_ARMADDRESSINGMODES_H
#define CS_LLVM_TARGET_ARM_ARMADDRESSINGMODES_H

#include "capstone/platform.h"
#include "../../MathExtras.h"

typedef enum ARM_AM_ShiftOpc {
	ARM_AM_no_shift = 0,
	ARM_AM_asr,
	ARM_AM_lsl,
	ARM_AM_lsr,
	ARM_AM_ror,
	ARM_AM_rrx
} ARM_AM_ShiftOpc;

typedef enum ARM_AM_AddrOpc {
	ARM_AM_sub = 0,
	ARM_AM_add
} ARM_AM_AddrOpc;

static inline const char *ARM_AM_getAddrOpcStr(ARM_AM_AddrOpc Op)
{
	return Op == ARM_AM_sub ? "-" : "";
}

static inline const char *ARM_AM_getShiftOpcStr(ARM_AM_ShiftOpc Op)
{
	switch (Op) {
		default: return "";	//llvm_unreachable("Unknown shift opc!");
		case ARM_AM_asr: return "asr";
		case ARM_AM_lsl: return "lsl";
		case ARM_AM_lsr: return "lsr";
		case ARM_AM_ror: return "ror";
		case ARM_AM_rrx: return "rrx";
	}
}

static inline unsigned ARM_AM_getShiftOpcEncoding(ARM_AM_ShiftOpc Op)
{
	switch (Op) {
		default: return (unsigned int)-1;	//llvm_unreachable("Unknown shift opc!");
		case ARM_AM_asr: return 2;
		case ARM_AM_lsl: return 0;
		case ARM_AM_lsr: return 1;
		case ARM_AM_ror: return 3;
	}
}

typedef enum ARM_AM_AMSubMode {
	ARM_AM_bad_am_submode = 0,
	ARM_AM_ia,
	ARM_AM_ib,
	ARM_AM_da,
	ARM_AM_db
} ARM_AM_AMSubMode;

static inline const char *ARM_AM_getAMSubModeStr(ARM_AM_AMSubMode Mode)
{
	switch (Mode) {
		default: return "";
		case ARM_AM_ia: return "ia";
		case ARM_AM_ib: return "ib";
		case ARM_AM_da: return "da";
		case ARM_AM_db: return "db";
	}
}

static inline unsigned rotr32(unsigned Val, unsigned Amt)
{
	return (Val >> Amt) | (Val << ((32-Amt)&31));
}

static inline unsigned rotl32(unsigned Val, unsigned Amt)
{
	return (Val << Amt) | (Val >> ((32-Amt)&31));
}

static inline unsigned getSORegOpc(ARM_AM_ShiftOpc ShOp, unsigned Imm)
{
	return ShOp | (Imm << 3);
}

static inline unsigned getSORegOffset(unsigned Op)
{
	return Op >> 3;
}

static inline ARM_AM_ShiftOpc ARM_AM_getSORegShOp(unsigned Op)
{
	return (ARM_AM_ShiftOpc)(Op & 7);
}

static inline unsigned getSOImmValImm(unsigned Imm)
{
	return Imm & 0xFF;
}

static inline unsigned getSOImmValRot(unsigned Imm)
{
	return (Imm >> 8) * 2;
}

static inline unsigned getSOImmValRotate(unsigned Imm)
{
	unsigned TZ, RotAmt;
	if ((Imm & ~255U) == 0) return 0;

	TZ = CountTrailingZeros_32(Imm);

	RotAmt = TZ & ~1;

	if ((rotr32(Imm, RotAmt) & ~255U) == 0)
		return (32-RotAmt)&31;  // HW rotates right, not left.

	if (Imm & 63U) {
		unsigned TZ2 = CountTrailingZeros_32(Imm & ~63U);
		unsigned RotAmt2 = TZ2 & ~1;
		if ((rotr32(Imm, RotAmt2) & ~255U) == 0)
			return (32-RotAmt2)&31;  // HW rotates right, not left.
	}

	return (32-RotAmt)&31;  // HW rotates right, not left.
}

static inline int getSOImmVal(unsigned Arg)
{
	unsigned RotAmt;
	if ((Arg & ~255U) == 0) return Arg;

	RotAmt = getSOImmValRotate(Arg);

	if (rotr32(~255U, RotAmt) & Arg)
		return -1;

	return rotl32(Arg, RotAmt) | ((RotAmt>>1) << 8);
}

static inline bool isSOImmTwoPartVal(unsigned V)
{
	V = rotr32(~255U, getSOImmValRotate(V)) & V;
	if (V == 0)
		return false;

	V = rotr32(~255U, getSOImmValRotate(V)) & V;
	return V == 0;
}

static inline unsigned getSOImmTwoPartFirst(unsigned V)
{
	return rotr32(255U, getSOImmValRotate(V)) & V;
}

static inline unsigned getSOImmTwoPartSecond(unsigned V)
{
	V = rotr32(~255U, getSOImmValRotate(V)) & V;

	return V;
}

static inline unsigned getThumbImmValShift(unsigned Imm)
{
	if ((Imm & ~255U) == 0) return 0;

	return CountTrailingZeros_32(Imm);
}

static inline bool isThumbImmShiftedVal(unsigned V)
{
	V = (~255U << getThumbImmValShift(V)) & V;
	return V == 0;
}

static inline unsigned getThumbImm16ValShift(unsigned Imm)
{
	if ((Imm & ~65535U) == 0) return 0;

	return CountTrailingZeros_32(Imm);
}

static inline bool isThumbImm16ShiftedVal(unsigned V)
{
	V = (~65535U << getThumbImm16ValShift(V)) & V;
	return V == 0;
}

static inline unsigned getThumbImmNonShiftedVal(unsigned V)
{
	return V >> getThumbImmValShift(V);
}


static inline int getT2SOImmValSplatVal(unsigned V)
{
	unsigned u, Vs, Imm;
	if ((V & 0xffffff00) == 0)
		return V;

	Vs = ((V & 0xff) == 0) ? V >> 8 : V;
	Imm = Vs & 0xff;
	u = Imm | (Imm << 16);

	if (Vs == u)
		return (((Vs == V) ? 1 : 2) << 8) | Imm;

	if (Vs == (u | (u << 8)))
		return (3 << 8) | Imm;

	return -1;
}

static inline int getT2SOImmValRotateVal(unsigned V)
{
	unsigned RotAmt = CountLeadingZeros_32(V);
	if (RotAmt >= 24)
		return -1;

	if ((rotr32(0xff000000U, RotAmt) & V) == V)
		return (rotr32(V, 24 - RotAmt) & 0x7f) | ((RotAmt + 8) << 7);

	return -1;
}

static inline int getT2SOImmVal(unsigned Arg)
{
	int Rot;
	int Splat = getT2SOImmValSplatVal(Arg);
	if (Splat != -1)
		return Splat;

	Rot = getT2SOImmValRotateVal(Arg);
	if (Rot != -1)
		return Rot;

	return -1;
}

static inline unsigned getT2SOImmValRotate(unsigned V)
{
	unsigned RotAmt;

	if ((V & ~255U) == 0)
		return 0;

	RotAmt = CountTrailingZeros_32(V);
	return (32 - RotAmt) & 31;
}

static inline bool isT2SOImmTwoPartVal (unsigned Imm)
{
	unsigned V = Imm;
	if (getT2SOImmValSplatVal(V) != -1)
		return false;
	V = rotr32 (~255U, getT2SOImmValRotate(V)) & V;
	if (V == 0)
		return false;

	if (getT2SOImmVal(V) != -1) return true;

	V = Imm;
	if (getT2SOImmValSplatVal(V & 0xff00ff00U) != -1)
		V &= ~0xff00ff00U;
	else if (getT2SOImmValSplatVal(V & 0x00ff00ffU) != -1)
		V &= ~0x00ff00ffU;
	if (getT2SOImmVal(V) != -1) return true;

	return false;
}

static inline unsigned getT2SOImmTwoPartFirst(unsigned Imm)
{
	unsigned V = rotr32 (~(unsigned int)255, getT2SOImmValRotate(Imm)) & Imm;
	if (getT2SOImmVal(V) != -1) return V;

	if (getT2SOImmValSplatVal(Imm & 0xff00ff00U) != -1)
		return Imm & 0xff00ff00U;

	return Imm & 0x00ff00ffU;
}

static inline unsigned getT2SOImmTwoPartSecond(unsigned Imm)
{
	Imm ^= getT2SOImmTwoPartFirst(Imm);
	return Imm;
}


static inline unsigned ARM_AM_getAM2Opc(ARM_AM_AddrOpc Opc, unsigned Imm12, ARM_AM_ShiftOpc SO,
		unsigned IdxMode)
{
	bool isSub = Opc == ARM_AM_sub;
	return Imm12 | ((int)isSub << 12) | (SO << 13) | (IdxMode << 16) ;
}

static inline unsigned getAM2Offset(unsigned AM2Opc)
{
	return AM2Opc & ((1 << 12)-1);
}

static inline ARM_AM_AddrOpc getAM2Op(unsigned AM2Opc)
{
	return ((AM2Opc >> 12) & 1) ? ARM_AM_sub : ARM_AM_add;
}

static inline ARM_AM_ShiftOpc getAM2ShiftOpc(unsigned AM2Opc)
{
	return (ARM_AM_ShiftOpc)((AM2Opc >> 13) & 7);
}

static inline unsigned getAM2IdxMode(unsigned AM2Opc)
{
	return (AM2Opc >> 16);
}


static inline unsigned getAM3Opc(ARM_AM_AddrOpc Opc, unsigned char Offset,
		unsigned IdxMode)
{
	bool isSub = Opc == ARM_AM_sub;
	return ((int)isSub << 8) | Offset | (IdxMode << 9);
}

static inline unsigned char getAM3Offset(unsigned AM3Opc)
{
	return AM3Opc & 0xFF;
}

static inline ARM_AM_AddrOpc getAM3Op(unsigned AM3Opc)
{
	return ((AM3Opc >> 8) & 1) ? ARM_AM_sub : ARM_AM_add;
}

static inline unsigned getAM3IdxMode(unsigned AM3Opc)
{
	return (AM3Opc >> 9);
}


static inline ARM_AM_AMSubMode getAM4SubMode(unsigned Mode)
{
	return (ARM_AM_AMSubMode)(Mode & 0x7);
}

static inline unsigned getAM4ModeImm(ARM_AM_AMSubMode SubMode)
{
	return (int)SubMode;
}


static inline unsigned ARM_AM_getAM5Opc(ARM_AM_AddrOpc Opc, unsigned char Offset)
{
	bool isSub = Opc == ARM_AM_sub;
	return ((int)isSub << 8) | Offset;
}
static inline unsigned char ARM_AM_getAM5Offset(unsigned AM5Opc)
{
	return AM5Opc & 0xFF;
}
static inline ARM_AM_AddrOpc ARM_AM_getAM5Op(unsigned AM5Opc)
{
	return ((AM5Opc >> 8) & 1) ? ARM_AM_sub : ARM_AM_add;
}


static inline unsigned getAM5FP16Opc(ARM_AM_AddrOpc Opc, unsigned char Offset)
{
	bool isSub = Opc == ARM_AM_sub;
	return ((int)isSub << 8) | Offset;
}

static inline unsigned char getAM5FP16Offset(unsigned AM5Opc)
{
	return AM5Opc & 0xFF;
}

static inline ARM_AM_AddrOpc getAM5FP16Op(unsigned AM5Opc)
{
	return ((AM5Opc >> 8) & 1) ? ARM_AM_sub : ARM_AM_add;
}



static inline unsigned createNEONModImm(unsigned OpCmode, unsigned Val)
{
	return (OpCmode << 8) | Val;
}
static inline unsigned getNEONModImmOpCmode(unsigned ModImm)
{
	return (ModImm >> 8) & 0x1f;
}
static inline unsigned getNEONModImmVal(unsigned ModImm)
{
	return ModImm & 0xff;
}

static inline uint64_t ARM_AM_decodeNEONModImm(unsigned ModImm, unsigned *EltBits)
{
	unsigned OpCmode = getNEONModImmOpCmode(ModImm);
	unsigned Imm8 = getNEONModImmVal(ModImm);
	uint64_t Val = 0;
	unsigned ByteNum;

	if (OpCmode == 0xe) {
		Val = Imm8;
		*EltBits = 8;
	} else if ((OpCmode & 0xc) == 0x8) {
		ByteNum = (OpCmode & 0x6) >> 1;
		Val = (uint64_t)Imm8 << (8 * ByteNum);
		*EltBits = 16;
	} else if ((OpCmode & 0x8) == 0) {
		ByteNum = (OpCmode & 0x6) >> 1;
		Val = (uint64_t)Imm8 << (8 * ByteNum);
		*EltBits = 32;
	} else if ((OpCmode & 0xe) == 0xc) {
		ByteNum = 1 + (OpCmode & 0x1);
		Val = (Imm8 << (8 * ByteNum)) | (0xffff >> (8 * (2 - ByteNum)));
		*EltBits = 32;
	} else if (OpCmode == 0x1e) {
		for (ByteNum = 0; ByteNum < 8; ++ByteNum) {
			if ((ModImm >> ByteNum) & 1)
				Val |= (uint64_t)0xff << (8 * ByteNum);
		}
		*EltBits = 64;
	} else {
	}
	return Val;
}

ARM_AM_AMSubMode getLoadStoreMultipleSubMode(int Opcode);

static inline float getFPImmFloat(unsigned Imm)
{
	union {
		uint32_t I;
		float F;
	} FPUnion;

	uint8_t Sign = (Imm >> 7) & 0x1;
	uint8_t Exp = (Imm >> 4) & 0x7;
	uint8_t Mantissa = Imm & 0xf;


	FPUnion.I = 0;
	FPUnion.I |= ((uint32_t) Sign) << 31;
	FPUnion.I |= ((Exp & 0x4) != 0 ? 0 : 1) << 30;
	FPUnion.I |= ((Exp & 0x4) != 0 ? 0x1f : 0) << 25;
	FPUnion.I |= (Exp & 0x3) << 23;
	FPUnion.I |= Mantissa << 19;
	return FPUnion.F;
}

#endif

