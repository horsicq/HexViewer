
/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifdef CAPSTONE_HAS_MIPS

#include <capstone/platform.h>
#include <stdlib.h>
#include <stdio.h>	// debug
#include <string.h>

#include "MipsInstPrinter.h"
#include "../../MCInst.h"
#include "../../utils.h"
#include "../../SStream.h"
#include "../../MCRegisterInfo.h"
#include "MipsMapping.h"

#include "MipsInstPrinter.h"

static void printUnsignedImm(MCInst *MI, int opNum, SStream *O);
static char *printAliasInstr(MCInst *MI, SStream *O, void *info);
static char *printAlias(MCInst *MI, SStream *OS);


typedef enum Mips_CondCode {
	Mips_FCOND_F,
	Mips_FCOND_UN,
	Mips_FCOND_OEQ,
	Mips_FCOND_UEQ,
	Mips_FCOND_OLT,
	Mips_FCOND_ULT,
	Mips_FCOND_OLE,
	Mips_FCOND_ULE,
	Mips_FCOND_SF,
	Mips_FCOND_NGLE,
	Mips_FCOND_SEQ,
	Mips_FCOND_NGL,
	Mips_FCOND_LT,
	Mips_FCOND_NGE,
	Mips_FCOND_LE,
	Mips_FCOND_NGT,

	Mips_FCOND_T,
	Mips_FCOND_OR,
	Mips_FCOND_UNE,
	Mips_FCOND_ONE,
	Mips_FCOND_UGE,
	Mips_FCOND_OGE,
	Mips_FCOND_UGT,
	Mips_FCOND_OGT,
	Mips_FCOND_ST,
	Mips_FCOND_GLE,
	Mips_FCOND_SNE,
	Mips_FCOND_GL,
	Mips_FCOND_NLT,
	Mips_FCOND_GE,
	Mips_FCOND_NLE,
	Mips_FCOND_GT
} Mips_CondCode;

#define GET_INSTRINFO_ENUM
#include "MipsGenInstrInfo.inc"

static const char *getRegisterName(unsigned RegNo);
static void printInstruction(MCInst *MI, SStream *O, const MCRegisterInfo *MRI);

static void set_mem_access(MCInst *MI, bool status)
{
	MI->csh->doing_mem = status;

	if (MI->csh->detail != CS_OPT_ON)
		return;

	if (status) {
		MI->flat_insn->detail->mips.operands[MI->flat_insn->detail->mips.op_count].type = MIPS_OP_MEM;
		MI->flat_insn->detail->mips.operands[MI->flat_insn->detail->mips.op_count].mem.base = MIPS_REG_INVALID;
		MI->flat_insn->detail->mips.operands[MI->flat_insn->detail->mips.op_count].mem.disp = 0;
	} else {
		MI->flat_insn->detail->mips.op_count++;
	}
}

static bool isReg(MCInst *MI, unsigned OpNo, unsigned R)
{
	return (MCOperand_isReg(MCInst_getOperand(MI, OpNo)) &&
			MCOperand_getReg(MCInst_getOperand(MI, OpNo)) == R);
}

static const char* MipsFCCToString(Mips_CondCode CC)
{
	switch (CC) {
		default: return 0;	// never reach
		case Mips_FCOND_F:
		case Mips_FCOND_T:   return "f";
		case Mips_FCOND_UN:
		case Mips_FCOND_OR:  return "un";
		case Mips_FCOND_OEQ:
		case Mips_FCOND_UNE: return "eq";
		case Mips_FCOND_UEQ:
		case Mips_FCOND_ONE: return "ueq";
		case Mips_FCOND_OLT:
		case Mips_FCOND_UGE: return "olt";
		case Mips_FCOND_ULT:
		case Mips_FCOND_OGE: return "ult";
		case Mips_FCOND_OLE:
		case Mips_FCOND_UGT: return "ole";
		case Mips_FCOND_ULE:
		case Mips_FCOND_OGT: return "ule";
		case Mips_FCOND_SF:
		case Mips_FCOND_ST:  return "sf";
		case Mips_FCOND_NGLE:
		case Mips_FCOND_GLE: return "ngle";
		case Mips_FCOND_SEQ:
		case Mips_FCOND_SNE: return "seq";
		case Mips_FCOND_NGL:
		case Mips_FCOND_GL:  return "ngl";
		case Mips_FCOND_LT:
		case Mips_FCOND_NLT: return "lt";
		case Mips_FCOND_NGE:
		case Mips_FCOND_GE:  return "nge";
		case Mips_FCOND_LE:
		case Mips_FCOND_NLE: return "le";
		case Mips_FCOND_NGT:
		case Mips_FCOND_GT:  return "ngt";
	}
}

static void printRegName(SStream *OS, unsigned RegNo)
{
	SStream_concat(OS, "$%s", getRegisterName(RegNo));
}

void Mips_printInst(MCInst *MI, SStream *O, void *info)
{
	char *mnem;

	switch (MCInst_getOpcode(MI)) {
		default: break;
		case Mips_Save16:
		case Mips_SaveX16:
		case Mips_Restore16:
		case Mips_RestoreX16:
			return;
	}

	mnem = printAliasInstr(MI, O, info);
	if (!mnem) {
		mnem = printAlias(MI, O);
		if (!mnem) {
			printInstruction(MI, O, NULL);
		}
	}

	if (mnem) {
		MCInst_setOpcodePub(MI, Mips_map_insn(mnem));
		cs_mem_free(mnem);
	}
}

static void printOperand(MCInst *MI, unsigned OpNo, SStream *O)
{
	MCOperand *Op;

	if (OpNo >= MI->size)
		return;

	Op = MCInst_getOperand(MI, OpNo);
	if (MCOperand_isReg(Op)) {
		unsigned int reg = MCOperand_getReg(Op);
		printRegName(O, reg);
		reg = Mips_map_register(reg);
		if (MI->csh->detail) {
			if (MI->csh->doing_mem) {
				MI->flat_insn->detail->mips.operands[MI->flat_insn->detail->mips.op_count].mem.base = reg;
			} else {
				MI->flat_insn->detail->mips.operands[MI->flat_insn->detail->mips.op_count].type = MIPS_OP_REG;
				MI->flat_insn->detail->mips.operands[MI->flat_insn->detail->mips.op_count].reg = reg;
				MI->flat_insn->detail->mips.op_count++;
			}
		}
	} else if (MCOperand_isImm(Op)) {
		int64_t imm = MCOperand_getImm(Op);
		if (MI->csh->doing_mem) {
			if (imm) {	// only print Imm offset if it is not 0
				printInt64(O, imm);
			}
			if (MI->csh->detail)
				MI->flat_insn->detail->mips.operands[MI->flat_insn->detail->mips.op_count].mem.disp = imm;
		} else {
			printInt64(O, imm);

			if (MI->csh->detail) {
				MI->flat_insn->detail->mips.operands[MI->flat_insn->detail->mips.op_count].type = MIPS_OP_IMM;
				MI->flat_insn->detail->mips.operands[MI->flat_insn->detail->mips.op_count].imm = imm;
				MI->flat_insn->detail->mips.op_count++;
			}
		}
	}
}

static void printUnsignedImm(MCInst *MI, int opNum, SStream *O)
{
	MCOperand *MO = MCInst_getOperand(MI, opNum);
	if (MCOperand_isImm(MO)) {
		int64_t imm = MCOperand_getImm(MO);
		printInt64(O, imm);

		if (MI->csh->detail) {
			MI->flat_insn->detail->mips.operands[MI->flat_insn->detail->mips.op_count].type = MIPS_OP_IMM;
			MI->flat_insn->detail->mips.operands[MI->flat_insn->detail->mips.op_count].imm = (unsigned short int)imm;
			MI->flat_insn->detail->mips.op_count++;
		}
	} else
		printOperand(MI, opNum, O);
}

static void printUnsignedImm8(MCInst *MI, int opNum, SStream *O)
{
	MCOperand *MO = MCInst_getOperand(MI, opNum);
	if (MCOperand_isImm(MO)) {
		uint8_t imm = (uint8_t)MCOperand_getImm(MO);
		if (imm > HEX_THRESHOLD)
			SStream_concat(O, "0x%x", imm);
		else
			SStream_concat(O, "%u", imm);
		if (MI->csh->detail) {
			MI->flat_insn->detail->mips.operands[MI->flat_insn->detail->mips.op_count].type = MIPS_OP_IMM;
			MI->flat_insn->detail->mips.operands[MI->flat_insn->detail->mips.op_count].imm = imm;
			MI->flat_insn->detail->mips.op_count++;
		}
	} else
		printOperand(MI, opNum, O);
}

static void printMemOperand(MCInst *MI, int opNum, SStream *O)
{

	switch (MCInst_getOpcode(MI)) {
		default:
			break;
		case Mips_SWM32_MM:
		case Mips_LWM32_MM:
		case Mips_SWM16_MM:
		case Mips_LWM16_MM:
			opNum = MCInst_getNumOperands(MI) - 2;
			break;
	}

	set_mem_access(MI, true);
	printOperand(MI, opNum + 1, O);
	SStream_concat0(O, "(");
	printOperand(MI, opNum, O);
	SStream_concat0(O, ")");
	set_mem_access(MI, false);
}

static void printMemOperandEA(MCInst *MI, int opNum, SStream *O)
{
	printOperand(MI, opNum, O);
	SStream_concat0(O, ", ");
	printOperand(MI, opNum + 1, O);
	return;
}

static void printFCCOperand(MCInst *MI, int opNum, SStream *O)
{
	MCOperand *MO = MCInst_getOperand(MI, opNum);
	SStream_concat0(O, MipsFCCToString((Mips_CondCode)MCOperand_getImm(MO)));
}

static void printRegisterPair(MCInst *MI, int opNum, SStream *O)
{
	printRegName(O, MCOperand_getReg(MCInst_getOperand(MI, opNum)));
}

static char *printAlias1(const char *Str, MCInst *MI, unsigned OpNo, SStream *OS)
{
	SStream_concat(OS, "%s\t", Str);
	printOperand(MI, OpNo, OS);
	return cs_strdup(Str);
}

static char *printAlias2(const char *Str, MCInst *MI,
		unsigned OpNo0, unsigned OpNo1, SStream *OS)
{
	char *tmp;

	tmp = printAlias1(Str, MI, OpNo0, OS);
	SStream_concat0(OS, ", ");
	printOperand(MI, OpNo1, OS);

	return tmp;
}

#define GET_REGINFO_ENUM
#include "MipsGenRegisterInfo.inc"

static char *printAlias(MCInst *MI, SStream *OS)
{
	switch (MCInst_getOpcode(MI)) {
		case Mips_BEQ:
		case Mips_BEQ_MM:
			if (isReg(MI, 0, Mips_ZERO) && isReg(MI, 1, Mips_ZERO))
				return printAlias1("b", MI, 2, OS);
			if (isReg(MI, 1, Mips_ZERO))
				return printAlias2("beqz", MI, 0, 2, OS);
			return NULL;
		case Mips_BEQ64:
			if (isReg(MI, 1, Mips_ZERO_64))
				return printAlias2("beqz", MI, 0, 2, OS);
			return NULL;
		case Mips_BNE:
			if (isReg(MI, 1, Mips_ZERO))
				return printAlias2("bnez", MI, 0, 2, OS);
			return NULL;
		case Mips_BNE64:
			if (isReg(MI, 1, Mips_ZERO_64))
				return printAlias2("bnez", MI, 0, 2, OS);
			return NULL;
		case Mips_BGEZAL:
			if (isReg(MI, 0, Mips_ZERO))
				return printAlias1("bal", MI, 1, OS);
			return NULL;
		case Mips_BC1T:
			if (isReg(MI, 0, Mips_FCC0))
				return printAlias1("bc1t", MI, 1, OS);
			return NULL;
		case Mips_BC1F:
			if (isReg(MI, 0, Mips_FCC0))
				return printAlias1("bc1f", MI, 1, OS);
			return NULL;
		case Mips_JALR:
			if (isReg(MI, 0, Mips_RA))
				return printAlias1("jalr", MI, 1, OS);
			return NULL;
		case Mips_JALR64:
			if (isReg(MI, 0, Mips_RA_64))
				return printAlias1("jalr", MI, 1, OS);
			return NULL;
		case Mips_NOR:
		case Mips_NOR_MM:
			if (isReg(MI, 2, Mips_ZERO))
				return printAlias2("not", MI, 0, 1, OS);
			return NULL;
		case Mips_NOR64:
			if (isReg(MI, 2, Mips_ZERO_64))
				return printAlias2("not", MI, 0, 1, OS);
			return NULL;
		case Mips_OR:
			if (isReg(MI, 2, Mips_ZERO))
				return printAlias2("move", MI, 0, 1, OS);
			return NULL;
		default: return NULL;
	}
}

static void printRegisterList(MCInst *MI, int opNum, SStream *O)
{
	int i, e, reg;

	for (i = opNum, e = MCInst_getNumOperands(MI) - 2; i != e; ++i) {
		if (i != opNum)
			SStream_concat0(O, ", ");
		reg = MCOperand_getReg(MCInst_getOperand(MI, i));
		printRegName(O, reg);
		if (MI->csh->detail) {
			MI->flat_insn->detail->mips.operands[MI->flat_insn->detail->mips.op_count].type = MIPS_OP_REG;
			MI->flat_insn->detail->mips.operands[MI->flat_insn->detail->mips.op_count].reg = reg;
			MI->flat_insn->detail->mips.op_count++;
		}
	}
}

#define PRINT_ALIAS_INSTR
#include "MipsGenAsmWriter.inc"

#endif
