
/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2019 */

#ifndef CS_ARMBASEINFO_H
#define CS_ARMBASEINFO_H

#include "capstone/arm.h"

#define GET_REGINFO_ENUM
#include "ARMGenRegisterInfo.inc"

typedef enum ARMCC_CondCodes { // Meaning (integer)          Meaning (floating-point)
	ARMCC_EQ,            // Equal                      Equal
	ARMCC_NE,            // Not equal                  Not equal, or unordered
	ARMCC_HS,            // Carry set                  >, ==, or unordered
	ARMCC_LO,            // Carry clear                Less than
	ARMCC_MI,            // Minus, negative            Less than
	ARMCC_PL,            // Plus, positive or zero     >, ==, or unordered
	ARMCC_VS,            // Overflow                   Unordered
	ARMCC_VC,            // No overflow                Not unordered
	ARMCC_HI,            // Unsigned higher            Greater than, or unordered
	ARMCC_LS,            // Unsigned lower or same     Less than or equal
	ARMCC_GE,            // Greater than or equal      Greater than or equal
	ARMCC_LT,            // Less than                  Less than, or unordered
	ARMCC_GT,            // Greater than               Greater than
	ARMCC_LE,            // Less than or equal         <, ==, or unordered
	ARMCC_AL             // Always (unconditional)     Always (unconditional)
} ARMCC_CondCodes;

inline static ARMCC_CondCodes ARMCC_getOppositeCondition(ARMCC_CondCodes CC)
{
	switch (CC) {
		case ARMCC_EQ: return ARMCC_NE;
		case ARMCC_NE: return ARMCC_EQ;
		case ARMCC_HS: return ARMCC_LO;
		case ARMCC_LO: return ARMCC_HS;
		case ARMCC_MI: return ARMCC_PL;
		case ARMCC_PL: return ARMCC_MI;
		case ARMCC_VS: return ARMCC_VC;
		case ARMCC_VC: return ARMCC_VS;
		case ARMCC_HI: return ARMCC_LS;
		case ARMCC_LS: return ARMCC_HI;
		case ARMCC_GE: return ARMCC_LT;
		case ARMCC_LT: return ARMCC_GE;
		case ARMCC_GT: return ARMCC_LE;
		case ARMCC_LE: return ARMCC_GT;
		default: return ARMCC_AL;
	}
}

inline static const char *ARMCC_ARMCondCodeToString(ARMCC_CondCodes CC)
{
	switch (CC) {
		case ARMCC_EQ:  return "eq";
		case ARMCC_NE:  return "ne";
		case ARMCC_HS:  return "hs";
		case ARMCC_LO:  return "lo";
		case ARMCC_MI:  return "mi";
		case ARMCC_PL:  return "pl";
		case ARMCC_VS:  return "vs";
		case ARMCC_VC:  return "vc";
		case ARMCC_HI:  return "hi";
		case ARMCC_LS:  return "ls";
		case ARMCC_GE:  return "ge";
		case ARMCC_LT:  return "lt";
		case ARMCC_GT:  return "gt";
		case ARMCC_LE:  return "le";
		case ARMCC_AL:  return "al";
		default: return "";
	}
}

inline static const char *ARM_PROC_IFlagsToString(unsigned val)
{
	switch (val) {
		case ARM_CPSFLAG_F: return "f";
		case ARM_CPSFLAG_I: return "i";
		case ARM_CPSFLAG_A: return "a";
		default: return "";
	}
}

inline static const char *ARM_PROC_IModToString(unsigned val)
{
	switch (val) {
		case ARM_CPSMODE_IE: return "ie";
		case ARM_CPSMODE_ID: return "id";
		default: return "";
	}
}

inline static const char *ARM_MB_MemBOptToString(unsigned val, bool HasV8)
{
	switch (val + 1) {
		default: return "BUGBUG";
		case ARM_MB_SY:    return "sy";
		case ARM_MB_ST:    return "st";
		case ARM_MB_LD: return HasV8 ? "ld" : "#0xd";
		case ARM_MB_RESERVED_12: return "#0xc";
		case ARM_MB_ISH:   return "ish";
		case ARM_MB_ISHST: return "ishst";
		case ARM_MB_ISHLD: return HasV8 ?  "ishld" : "#9";
		case ARM_MB_RESERVED_8: return "#8";
		case ARM_MB_NSH:   return "nsh";
		case ARM_MB_NSHST: return "nshst";
		case ARM_MB_NSHLD: return HasV8 ? "nshld" : "#5";
		case ARM_MB_RESERVED_4: return "#4";
		case ARM_MB_OSH:   return "osh";
		case ARM_MB_OSHST: return "oshst";
		case ARM_MB_OSHLD: return HasV8 ? "oshld" : "#1";
		case ARM_MB_RESERVED_0: return "#0";
	}
}

enum ARM_ISB_InstSyncBOpt {
    ARM_ISB_RESERVED_0 = 0,
    ARM_ISB_RESERVED_1 = 1,
    ARM_ISB_RESERVED_2 = 2,
    ARM_ISB_RESERVED_3 = 3,
    ARM_ISB_RESERVED_4 = 4,
    ARM_ISB_RESERVED_5 = 5,
    ARM_ISB_RESERVED_6 = 6,
    ARM_ISB_RESERVED_7 = 7,
    ARM_ISB_RESERVED_8 = 8,
    ARM_ISB_RESERVED_9 = 9,
    ARM_ISB_RESERVED_10 = 10,
    ARM_ISB_RESERVED_11 = 11,
    ARM_ISB_RESERVED_12 = 12,
    ARM_ISB_RESERVED_13 = 13,
    ARM_ISB_RESERVED_14 = 14,
    ARM_ISB_SY = 15
};

inline static const char *ARM_ISB_InstSyncBOptToString(unsigned val)
{
	switch (val) {
		default: // never reach
		case ARM_ISB_RESERVED_0:  return "#0x0";
		case ARM_ISB_RESERVED_1:  return "#0x1";
		case ARM_ISB_RESERVED_2:  return "#0x2";
		case ARM_ISB_RESERVED_3:  return "#0x3";
		case ARM_ISB_RESERVED_4:  return "#0x4";
		case ARM_ISB_RESERVED_5:  return "#0x5";
		case ARM_ISB_RESERVED_6:  return "#0x6";
		case ARM_ISB_RESERVED_7:  return "#0x7";
		case ARM_ISB_RESERVED_8:  return "#0x8";
		case ARM_ISB_RESERVED_9:  return "#0x9";
		case ARM_ISB_RESERVED_10: return "#0xa";
		case ARM_ISB_RESERVED_11: return "#0xb";
		case ARM_ISB_RESERVED_12: return "#0xc";
		case ARM_ISB_RESERVED_13: return "#0xd";
		case ARM_ISB_RESERVED_14: return "#0xe";
		case ARM_ISB_SY:          return "sy";
	}
}

static inline bool isARMLowRegister(unsigned Reg)
{
	switch (Reg) {
		case ARM_R0:  case ARM_R1:  case ARM_R2:  case ARM_R3:
		case ARM_R4:  case ARM_R5:  case ARM_R6:  case ARM_R7:
			return true;
		default:
			return false;
	}
}

enum ARMII_IndexMode {
	ARMII_IndexModeNone  = 0,
	ARMII_IndexModePre   = 1,
	ARMII_IndexModePost  = 2,
	ARMII_IndexModeUpd   = 3
};

typedef enum ARMII_AddrMode {
	ARMII_AddrModeNone    = 0,
	ARMII_AddrMode1       = 1,
	ARMII_AddrMode2       = 2,
	ARMII_AddrMode3       = 3,
	ARMII_AddrMode4       = 4,
	ARMII_AddrMode5       = 5,
	ARMII_AddrMode6       = 6,
	ARMII_AddrModeT1_1    = 7,
	ARMII_AddrModeT1_2    = 8,
	ARMII_AddrModeT1_4    = 9,
	ARMII_AddrModeT1_s    = 10, // i8 * 4 for pc and sp relative data
	ARMII_AddrModeT2_i12  = 11,
	ARMII_AddrModeT2_i8   = 12,
	ARMII_AddrModeT2_so   = 13,
	ARMII_AddrModeT2_pc   = 14, // +/- i12 for pc relative data
	ARMII_AddrModeT2_i8s4 = 15, // i8 * 4
	ARMII_AddrMode_i12    = 16
} ARMII_AddrMode;

inline static const char *ARMII_AddrModeToString(ARMII_AddrMode addrmode)
{
	switch (addrmode) {
		case ARMII_AddrModeNone:    return "AddrModeNone";
		case ARMII_AddrMode1:       return "AddrMode1";
		case ARMII_AddrMode2:       return "AddrMode2";
		case ARMII_AddrMode3:       return "AddrMode3";
		case ARMII_AddrMode4:       return "AddrMode4";
		case ARMII_AddrMode5:       return "AddrMode5";
		case ARMII_AddrMode6:       return "AddrMode6";
		case ARMII_AddrModeT1_1:    return "AddrModeT1_1";
		case ARMII_AddrModeT1_2:    return "AddrModeT1_2";
		case ARMII_AddrModeT1_4:    return "AddrModeT1_4";
		case ARMII_AddrModeT1_s:    return "AddrModeT1_s";
		case ARMII_AddrModeT2_i12:  return "AddrModeT2_i12";
		case ARMII_AddrModeT2_i8:   return "AddrModeT2_i8";
		case ARMII_AddrModeT2_so:   return "AddrModeT2_so";
		case ARMII_AddrModeT2_pc:   return "AddrModeT2_pc";
		case ARMII_AddrModeT2_i8s4: return "AddrModeT2_i8s4";
		case ARMII_AddrMode_i12:    return "AddrMode_i12";
	}
}

enum ARMII_TOF {

	ARMII_MO_NO_FLAG,

	ARMII_MO_LO16,

	ARMII_MO_HI16,

	ARMII_MO_LO16_NONLAZY,

	ARMII_MO_HI16_NONLAZY,

	ARMII_MO_LO16_NONLAZY_PIC,

	ARMII_MO_HI16_NONLAZY_PIC,

	ARMII_MO_PLT
};

enum {

	ARMII_AddrModeMask  = 0x1f, // The AddrMode enums are declared in ARMBaseInfo.h

	ARMII_IndexModeShift = 5,
	ARMII_IndexModeMask  = 3 << ARMII_IndexModeShift,

	ARMII_FormShift     = 7,
	ARMII_FormMask      = 0x3f << ARMII_FormShift,

	ARMII_Pseudo        = 0  << ARMII_FormShift,

	ARMII_MulFrm        = 1  << ARMII_FormShift,

	ARMII_BrFrm         = 2  << ARMII_FormShift,
	ARMII_BrMiscFrm     = 3  << ARMII_FormShift,

	ARMII_DPFrm         = 4  << ARMII_FormShift,
	ARMII_DPSoRegFrm    = 5  << ARMII_FormShift,

	ARMII_LdFrm         = 6  << ARMII_FormShift,
	ARMII_StFrm         = 7  << ARMII_FormShift,
	ARMII_LdMiscFrm     = 8  << ARMII_FormShift,
	ARMII_StMiscFrm     = 9  << ARMII_FormShift,
	ARMII_LdStMulFrm    = 10 << ARMII_FormShift,

	ARMII_LdStExFrm     = 11 << ARMII_FormShift,

	ARMII_ArithMiscFrm  = 12 << ARMII_FormShift,
	ARMII_SatFrm        = 13 << ARMII_FormShift,

	ARMII_ExtFrm        = 14 << ARMII_FormShift,

	ARMII_VFPUnaryFrm   = 15 << ARMII_FormShift,
	ARMII_VFPBinaryFrm  = 16 << ARMII_FormShift,
	ARMII_VFPConv1Frm   = 17 << ARMII_FormShift,
	ARMII_VFPConv2Frm   = 18 << ARMII_FormShift,
	ARMII_VFPConv3Frm   = 19 << ARMII_FormShift,
	ARMII_VFPConv4Frm   = 20 << ARMII_FormShift,
	ARMII_VFPConv5Frm   = 21 << ARMII_FormShift,
	ARMII_VFPLdStFrm    = 22 << ARMII_FormShift,
	ARMII_VFPLdStMulFrm = 23 << ARMII_FormShift,
	ARMII_VFPMiscFrm    = 24 << ARMII_FormShift,

	ARMII_ThumbFrm      = 25 << ARMII_FormShift,

	ARMII_MiscFrm       = 26 << ARMII_FormShift,

	ARMII_NGetLnFrm     = 27 << ARMII_FormShift,
	ARMII_NSetLnFrm     = 28 << ARMII_FormShift,
	ARMII_NDupFrm       = 29 << ARMII_FormShift,
	ARMII_NLdStFrm      = 30 << ARMII_FormShift,
	ARMII_N1RegModImmFrm= 31 << ARMII_FormShift,
	ARMII_N2RegFrm      = 32 << ARMII_FormShift,
	ARMII_NVCVTFrm      = 33 << ARMII_FormShift,
	ARMII_NVDupLnFrm    = 34 << ARMII_FormShift,
	ARMII_N2RegVShLFrm  = 35 << ARMII_FormShift,
	ARMII_N2RegVShRFrm  = 36 << ARMII_FormShift,
	ARMII_N3RegFrm      = 37 << ARMII_FormShift,
	ARMII_N3RegVShFrm   = 38 << ARMII_FormShift,
	ARMII_NVExtFrm      = 39 << ARMII_FormShift,
	ARMII_NVMulSLFrm    = 40 << ARMII_FormShift,
	ARMII_NVTBLFrm      = 41 << ARMII_FormShift,


	ARMII_UnaryDP       = 1 << 13,

	ARMII_Xform16Bit    = 1 << 14,

	ARMII_ThumbArithFlagSetting = 1 << 18,

	ARMII_DomainShift   = 15,
	ARMII_DomainMask    = 7 << ARMII_DomainShift,
	ARMII_DomainGeneral = 0 << ARMII_DomainShift,
	ARMII_DomainVFP     = 1 << ARMII_DomainShift,
	ARMII_DomainNEON    = 2 << ARMII_DomainShift,
	ARMII_DomainNEONA8  = 4 << ARMII_DomainShift,

	ARMII_ShiftTypeShift = 4,

	ARMII_M_BitShift     = 5,
	ARMII_ShiftImmShift  = 5,
	ARMII_ShiftShift     = 7,
	ARMII_N_BitShift     = 7,
	ARMII_ImmHiShift     = 8,
	ARMII_SoRotImmShift  = 8,
	ARMII_RegRsShift     = 8,
	ARMII_ExtRotImmShift = 10,
	ARMII_RegRdLoShift   = 12,
	ARMII_RegRdShift     = 12,
	ARMII_RegRdHiShift   = 16,
	ARMII_RegRnShift     = 16,
	ARMII_S_BitShift     = 20,
	ARMII_W_BitShift     = 21,
	ARMII_AM3_I_BitShift = 22,
	ARMII_D_BitShift     = 22,
	ARMII_U_BitShift     = 23,
	ARMII_P_BitShift     = 24,
	ARMII_I_BitShift     = 25,
	ARMII_CondShift      = 28
};

typedef struct MClassSysReg {
	const char *Name;
	arm_sysreg sysreg;
	uint16_t M1Encoding12;
	uint16_t M2M3Encoding8;
	uint16_t Encoding;
	int FeaturesRequired[2];	// 2 is enough for MClassSysRegsList
} MClassSysReg;

enum TraceSyncBOpt {
	CSYNC = 0
};

const MClassSysReg *lookupMClassSysRegByM2M3Encoding8(uint16_t encoding);
const MClassSysReg *lookupMClassSysRegByM1Encoding12(uint16_t M1Encoding12);

static inline const MClassSysReg *lookupMClassSysRegAPSRNonDeprecated(unsigned SYSm)
{
	return lookupMClassSysRegByM2M3Encoding8((1<<9) | (SYSm & 0xFF));
}

static inline const MClassSysReg *lookupMClassSysRegBy8bitSYSmValue(unsigned SYSm)
{
	return lookupMClassSysRegByM2M3Encoding8((1<<8) | (SYSm & 0xFF));
}

static inline bool MClassSysReg_isInRequiredFeatures(const MClassSysReg *TheReg, int TestFeatures)
{
	return (TheReg->FeaturesRequired[0] == TestFeatures || TheReg->FeaturesRequired[1] == TestFeatures);
}

static inline const MClassSysReg *lookupMClassSysRegBy12bitSYSmValue(unsigned SYSm)
{
  return lookupMClassSysRegByM1Encoding12(SYSm);
}

static inline const char *ARM_TSB_TraceSyncBOptToString(unsigned val)
{
	switch (val) {
		default:
			return NULL;

		case CSYNC:
			return "csync";
	}
}

#endif
