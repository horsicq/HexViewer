
/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2019 */

#ifndef CS_LLVM_MC_MCINSTRDESC_H
#define CS_LLVM_MC_MCINSTRDESC_H

#include "MCRegisterInfo.h"
#include "capstone/platform.h"


enum MCOI_OperandConstraint {
	MCOI_TIED_TO = 0,    // Must be allocated the same register as.
	MCOI_EARLY_CLOBBER   // Operand is an early clobber register operand
};

enum MCOI_OperandFlags {
	MCOI_LookupPtrRegClass = 0,
	MCOI_Predicate,
	MCOI_OptionalDef
};

enum MCOI_OperandType {
	MCOI_OPERAND_UNKNOWN = 0,
	MCOI_OPERAND_IMMEDIATE = 1,
	MCOI_OPERAND_REGISTER = 2,
	MCOI_OPERAND_MEMORY = 3,
	MCOI_OPERAND_PCREL = 4,

	MCOI_OPERAND_FIRST_GENERIC = 6,
	MCOI_OPERAND_GENERIC_0 = 6,
	MCOI_OPERAND_GENERIC_1 = 7,
	MCOI_OPERAND_GENERIC_2 = 8,
	MCOI_OPERAND_GENERIC_3 = 9,
	MCOI_OPERAND_GENERIC_4 = 10,
	MCOI_OPERAND_GENERIC_5 = 11,
	MCOI_OPERAND_LAST_GENERIC = 11,

	MCOI_OPERAND_FIRST_TARGET = 12,
};


typedef struct MCOperandInfo {
	int16_t RegClass;

	uint8_t Flags;

	uint8_t OperandType;

	uint32_t Constraints;
} MCOperandInfo;



enum {
	MCID_Variadic = 0,
	MCID_HasOptionalDef,
	MCID_Pseudo,
	MCID_Return,
	MCID_Call,
	MCID_Barrier,
	MCID_Terminator,
	MCID_Branch,
	MCID_IndirectBranch,
	MCID_Compare,
	MCID_MoveImm,
	MCID_MoveReg,
	MCID_Bitcast,
	MCID_Select,
	MCID_DelaySlot,
	MCID_FoldableAsLoad,
	MCID_MayLoad,
	MCID_MayStore,
	MCID_Predicable,
	MCID_NotDuplicable,
	MCID_UnmodeledSideEffects,
	MCID_Commutable,
	MCID_ConvertibleTo3Addr,
	MCID_UsesCustomInserter,
	MCID_HasPostISelHook,
	MCID_Rematerializable,
	MCID_CheapAsAMove,
	MCID_ExtraSrcRegAllocReq,
	MCID_ExtraDefRegAllocReq,
	MCID_RegSequence,
	MCID_ExtractSubreg,
	MCID_InsertSubreg,
	MCID_Convergent,
	MCID_Add,
	MCID_Trap,
};

typedef struct MCInstrDesc {
	unsigned char  NumOperands;   // Num of args (may be more if variable_ops)
	const MCOperandInfo *OpInfo;   // 'NumOperands' entries about operands
} MCInstrDesc;

bool MCOperandInfo_isPredicate(const MCOperandInfo *m);

bool MCOperandInfo_isOptionalDef(const MCOperandInfo *m);

#endif
