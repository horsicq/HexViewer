#ifndef CS_RISCVBASEINFO_H
#define CS_RISCVBASEINFO_H
#include "../../cs_priv.h"


enum {
	IRISCVII_InstFormatPseudo = 0,
  	IRISCVII_InstFormatR = 1,
  	IRISCVII_InstFormatR4 = 2,
  	IRISCVII_InstFormatI = 3,
  	IRISCVII_InstFormatS = 4,
  	IRISCVII_InstFormatB = 5,
  	IRISCVII_InstFormatU = 6,
  	IRISCVII_InstFormatJ = 7,
  	IRISCVII_InstFormatCR = 8,
  	IRISCVII_InstFormatCI = 9,
  	IRISCVII_InstFormatCSS = 10,
  	IRISCVII_InstFormatCIW = 11,
 	IRISCVII_InstFormatCL = 12,
  	IRISCVII_InstFormatCS = 13,
  	IRISCVII_InstFormatCA = 14,
  	IRISCVII_InstFormatCB = 15,
  	IRISCVII_InstFormatCJ = 16,
  	IRISCVII_InstFormatOther = 17,

  	IRISCVII_InstFormatMask = 31	
};

enum {
	RISCVII_MO_None,
	RISCVII_MO_LO,
	RISCVII_MO_HI,
	RISCVII_MO_PCREL_HI,
};

enum FenceField {
  	RISCVFenceField_I = 8,
  	RISCVFenceField_O = 4,
 	RISCVFenceField_R = 2,
  	RISCVFenceField_W = 1
};

enum RoundingMode {
  	RISCVFPRndMode_RNE = 0,
  	RISCVFPRndMode_RTZ = 1,
  	RISCVFPRndMode_RDN = 2,
  	RISCVFPRndMode_RUP = 3,
  	RISCVFPRndMode_RMM = 4,
  	RISCVFPRndMode_DYN = 7,
  	RISCVFPRndMode_Invalid
};

static const char *roundingModeToString(enum RoundingMode RndMode)
{
  	switch (RndMode) {
  	default:
    		CS_ASSERT(0 && "Unknown floating point rounding mode");
  	case RISCVFPRndMode_RNE:
    		return "rne";
  	case RISCVFPRndMode_RTZ:
    		return "rtz";
  	case RISCVFPRndMode_RDN:
    		return "rdn";
  	case RISCVFPRndMode_RUP:
    		return "rup";
  	case RISCVFPRndMode_RMM:
    		return "rmm";
  	case RISCVFPRndMode_DYN:
    		return "dyn";
  	}
}

static bool RISCVFPRndMode_isValidRoundingMode(unsigned Mode)
{
  	switch (Mode) {
  	default:
    		return false;
  	case RISCVFPRndMode_RNE:
  	case RISCVFPRndMode_RTZ:
  	case RISCVFPRndMode_RDN:
  	case RISCVFPRndMode_RUP:
  	case RISCVFPRndMode_RMM:
  	case RISCVFPRndMode_DYN:
    		return true;
  	}
}

#endif
