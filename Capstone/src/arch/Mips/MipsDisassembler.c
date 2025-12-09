
/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifdef CAPSTONE_HAS_MIPS

#include <stdio.h>
#include <string.h>

#include "capstone/platform.h"

#include "MipsDisassembler.h"

#include "../../utils.h"

#include "../../MCRegisterInfo.h"
#include "../../SStream.h"

#include "../../MathExtras.h"

#include "../../MCFixedLenDisassembler.h"
#include "../../MCInst.h"
#include "../../MCRegisterInfo.h"
#include "../../MCDisassembler.h"

static DecodeStatus DecodeGPR64RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeCPU16RegsRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeGPRMM16RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeGPRMM16ZeroRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeGPRMM16MovePRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeGPR32RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodePtrRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeDSPRRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeFGR64RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeFGR32RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeCCRRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeFCCRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeCCRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeFGRCCRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeHWRegsRegisterClass(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeAFGR64RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeACC64DSPRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeHI32DSPRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeLO32DSPRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeMSA128BRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeMSA128HRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeMSA128WRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeMSA128DRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeMSACtrlRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeCOP2RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeBranchTarget(MCInst *Inst,
		unsigned Offset, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeJumpTarget(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeBranchTarget21(MCInst *Inst,
		unsigned Offset, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeBranchTarget26(MCInst *Inst,
		unsigned Offset, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeBranchTarget7MM(MCInst *Inst,
		unsigned Offset, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeBranchTarget10MM(MCInst *Inst,
		unsigned Offset, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeBranchTargetMM(MCInst *Inst,
		unsigned Offset, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeJumpTargetMM(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeMem(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeCacheOp(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeCacheOpR6(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeCacheOpMM(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeSyncI(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeMSA128Mem(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeMemMMImm4(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeMemMMSPImm5Lsl2(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeMemMMGPImm7Lsl2(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeMemMMReglistImm4Lsl2(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeMemMMImm12(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeMemMMImm16(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeFMem(MCInst *Inst, unsigned Insn,
		uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeFMem2(MCInst *Inst, unsigned Insn,
		uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeFMem3(MCInst *Inst, unsigned Insn,
		uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeFMemCop2R6(MCInst *Inst, unsigned Insn,
		uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeSpecial3LlSc(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeAddiur2Simm7(MCInst *Inst,
		unsigned Value, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeUImm6Lsl2(MCInst *Inst,
		unsigned Value, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeLiSimm7(MCInst *Inst,
		unsigned Value, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeSimm4(MCInst *Inst,
		unsigned Value, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeSimm16(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeLSAImm(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeInsSize(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeExtSize(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeSimm19Lsl2(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeSimm18Lsl3(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeSimm9SP(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeANDI16Imm(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeUImm5lsl2(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeSimm23Lsl2(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeINSVE_DF_4(MCInst *MI,
		uint32_t insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeAddiGroupBranch_4(MCInst *MI,
		uint32_t insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeDaddiGroupBranch_4(MCInst *MI,
		uint32_t insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeBlezlGroupBranch_4(MCInst *MI,
		uint32_t insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeBgtzlGroupBranch_4(MCInst *MI,
		uint32_t insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeBgtzGroupBranch_4(MCInst *MI,
		uint32_t insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeBlezGroupBranch_4(MCInst *MI,
		uint32_t insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeRegListOperand(MCInst *Inst,
		uint32_t insn, uint64_t Address, const MCRegisterInfo *Decoder);

static DecodeStatus DecodeRegListOperand16(MCInst *Inst,
		uint32_t insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeMovePRegPair(MCInst *Inst,
		uint32_t insn, uint64_t Address, MCRegisterInfo *Decoder);

#define GET_SUBTARGETINFO_ENUM
#include "MipsGenSubtargetInfo.inc"

static uint64_t getFeatureBits(int mode)
{
	uint64_t Bits = (uint64_t)-1;	// include every features at first

	Bits &= ~Mips_FeatureMips1;

	Bits &= ~Mips_FeatureMicroMips;

	if (mode & CS_MODE_16) {
	} else if (mode & CS_MODE_32) {
		Bits &= ~Mips_FeatureMips16;
		Bits &= ~Mips_FeatureFP64Bit;
		Bits &= ~Mips_FeatureMips64r2;
		Bits &= ~Mips_FeatureMips32r6;
		Bits &= ~Mips_FeatureMips64r6;
	} else if (mode & CS_MODE_64) {
		Bits &= ~Mips_FeatureMips16;
		Bits &= ~Mips_FeatureMips64r6;
		Bits &= ~Mips_FeatureMips32r6;
	} else if (mode & CS_MODE_MIPS32R6) {
		Bits |= Mips_FeatureMips32r6;
		Bits &= ~Mips_FeatureMips16;
		Bits &= ~Mips_FeatureFP64Bit;
		Bits &= ~Mips_FeatureMips64r6;
		Bits &= ~Mips_FeatureMips64r2;
	}

	if (mode & CS_MODE_MICRO) {
		Bits |= Mips_FeatureMicroMips;
		Bits &= ~Mips_FeatureMips4_32r2;
		Bits &= ~Mips_FeatureMips2;
	}

	return Bits;
}

#include "MipsGenDisassemblerTables.inc"

#define GET_REGINFO_ENUM
#include "MipsGenRegisterInfo.inc"

#define GET_REGINFO_MC_DESC
#include "MipsGenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#include "MipsGenInstrInfo.inc"

void Mips_init(MCRegisterInfo *MRI)
{


	MCRegisterInfo_InitMCRegisterInfo(MRI, MipsRegDesc, 394,
			0, 0,
			MipsMCRegisterClasses, 62,
			0, 0,
			MipsRegDiffLists,
			0,
			MipsSubRegIdxLists, 12,
			0);
}

static void readInstruction16(unsigned char *code, uint32_t *insn,
		bool isBigEndian)
{
	if (isBigEndian)
		*insn = (code[0] << 8) | code[1];
	else
		*insn = (code[1] << 8) | code[0];
}

static void readInstruction32(unsigned char *code, uint32_t *insn, bool isBigEndian, bool isMicroMips)
{

	if (isBigEndian) {
		*insn =
			(code[3] << 0) | (code[2] << 8) | (code[1] << 16) | ((uint32_t) code[0] << 24);
	} else {
		if (isMicroMips) {
			*insn = (code[2] << 0) | (code[3] << 8) | (code[0] << 16) |
				((uint32_t) code[1] << 24);
		} else {
			*insn = (code[0] << 0) | (code[1] << 8) | (code[2] << 16) |
				((uint32_t) code[3] << 24);
		}
	}
}

static DecodeStatus MipsDisassembler_getInstruction(int mode, MCInst *instr,
		const uint8_t *code, size_t code_len,
		uint16_t *Size,
		uint64_t Address, bool isBigEndian, MCRegisterInfo *MRI)
{
	uint32_t Insn;
	DecodeStatus Result;

	if (instr->flat_insn->detail) {
		memset(instr->flat_insn->detail, 0, offsetof(cs_detail, mips)+sizeof(cs_mips));
	}

	if (mode & CS_MODE_MICRO) {
		if (code_len < 2)
			return MCDisassembler_Fail;

		readInstruction16((unsigned char*)code, &Insn, isBigEndian);

		Result = decodeInstruction(DecoderTableMicroMips16, instr, Insn, Address, MRI, mode);
		if (Result != MCDisassembler_Fail) {
			*Size = 2;
			return Result;
		}

		if (code_len < 4)
			return MCDisassembler_Fail;

		readInstruction32((unsigned char*)code, &Insn, isBigEndian, true);

		Result = decodeInstruction(DecoderTableMicroMips32, instr, Insn, Address, MRI, mode);
		if (Result != MCDisassembler_Fail) {
			*Size = 4;
			return Result;
		}
		return MCDisassembler_Fail;
	}

	if (code_len < 4)
		return MCDisassembler_Fail;

	readInstruction32((unsigned char*)code, &Insn, isBigEndian, false);

	if ((mode & CS_MODE_MIPS2) && ((mode & CS_MODE_MIPS3) == 0)) {
		Result = decodeInstruction(DecoderTableCOP3_32, instr, Insn, Address, MRI, mode);
		if (Result != MCDisassembler_Fail) {
			*Size = 4;
			return Result;
		}
	}

	if ((mode & CS_MODE_MIPS32R6) && (mode & CS_MODE_MIPS64)) {
		Result = decodeInstruction(DecoderTableMips32r6_64r6_GP6432, instr, Insn,
				Address, MRI, mode);
		if (Result != MCDisassembler_Fail) {
			*Size = 4;
			return Result;
		}
	}

	if (mode & CS_MODE_MIPS32R6) {
		Result = decodeInstruction(DecoderTableMips32r6_64r632, instr, Insn,
				Address, MRI, mode);
		if (Result != MCDisassembler_Fail) {
			*Size = 4;
			return Result;
		}
	}

	if (mode & CS_MODE_MIPS64) {
		Result = decodeInstruction(DecoderTableMips6432, instr, Insn,
				Address, MRI, mode);
		if (Result != MCDisassembler_Fail) {
			*Size = 4;
			return Result;
		}
	}

	Result = decodeInstruction(DecoderTableMips32, instr, Insn, Address, MRI, mode);
	if (Result != MCDisassembler_Fail) {
		*Size = 4;
		return Result;
	}

	return MCDisassembler_Fail;
}

bool Mips_getInstruction(csh ud, const uint8_t *code, size_t code_len, MCInst *instr,
		uint16_t *size, uint64_t address, void *info)
{
	cs_struct *handle = (cs_struct *)(uintptr_t)ud;

	DecodeStatus status = MipsDisassembler_getInstruction(handle->mode, instr,
			code, code_len,
			size,
			address, MODE_IS_BIG_ENDIAN(handle->mode), (MCRegisterInfo *)info);

	return status == MCDisassembler_Success;
}

static unsigned getReg(const MCRegisterInfo *MRI, unsigned RC, unsigned RegNo)
{
	const MCRegisterClass *rc = MCRegisterInfo_getRegClass(MRI, RC);
	return rc->RegsBegin[RegNo];
}

static DecodeStatus DecodeINSVE_DF_4(MCInst *MI, uint32_t insn,
		uint64_t Address, const MCRegisterInfo *Decoder)
{
	typedef DecodeStatus (*DecodeFN)(MCInst *, unsigned, uint64_t, const MCRegisterInfo *);
	uint32_t tmp = fieldFromInstruction(insn, 17, 5);
	unsigned NSize = 0;
	DecodeFN RegDecoder = NULL;

	if ((tmp & 0x18) == 0x00) { // INSVE_B
		NSize = 4;
		RegDecoder = DecodeMSA128BRegisterClass;
	} else if ((tmp & 0x1c) == 0x10) { // INSVE_H
		NSize = 3;
		RegDecoder = DecodeMSA128HRegisterClass;
	} else if ((tmp & 0x1e) == 0x18) { // INSVE_W
		NSize = 2;
		RegDecoder = DecodeMSA128WRegisterClass;
	} else if ((tmp & 0x1f) == 0x1c) { // INSVE_D
		NSize = 1;
		RegDecoder = DecodeMSA128DRegisterClass;
	} //else llvm_unreachable("Invalid encoding");

	if (NSize == 0 || RegDecoder == NULL)
		return MCDisassembler_Fail;

	tmp = fieldFromInstruction(insn, 6, 5);
	if (RegDecoder(MI, tmp, Address, Decoder) == MCDisassembler_Fail)
		return MCDisassembler_Fail;

	if (RegDecoder(MI, tmp, Address, Decoder) == MCDisassembler_Fail)
		return MCDisassembler_Fail;

	tmp = fieldFromInstruction(insn, 16, NSize);
	MCOperand_CreateImm0(MI, tmp);

	tmp = fieldFromInstruction(insn, 11, 5);
	if (RegDecoder(MI, tmp, Address, Decoder) == MCDisassembler_Fail)
		return MCDisassembler_Fail;

	MCOperand_CreateImm0(MI, 0);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeAddiGroupBranch_4(MCInst *MI, uint32_t insn,
		uint64_t Address, const MCRegisterInfo *Decoder)
{

	uint32_t Rs = fieldFromInstruction(insn, 21, 5);
	uint32_t Rt = fieldFromInstruction(insn, 16, 5);
	uint32_t Imm = (uint32_t)SignExtend64(fieldFromInstruction(insn, 0, 16), 16) * 4;
	bool HasRs = false;

	if (Rs >= Rt) {
		MCInst_setOpcode(MI, Mips_BOVC);
		HasRs = true;
	} else if (Rs != 0 && Rs < Rt) {
		MCInst_setOpcode(MI, Mips_BEQC);
		HasRs = true;
	} else
		MCInst_setOpcode(MI, Mips_BEQZALC);

	if (HasRs)
		MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rs));

	MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rt));
	MCOperand_CreateImm0(MI, Imm);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeDaddiGroupBranch_4(MCInst *MI, uint32_t insn,
		uint64_t Address, const MCRegisterInfo *Decoder)
{

	uint32_t Rs = fieldFromInstruction(insn, 21, 5);
	uint32_t Rt = fieldFromInstruction(insn, 16, 5);
	uint32_t Imm = (uint32_t)SignExtend64(fieldFromInstruction(insn, 0, 16), 16) * 4;
	bool HasRs = false;

	if (Rs >= Rt) {
		MCInst_setOpcode(MI, Mips_BNVC);
		HasRs = true;
	} else if (Rs != 0 && Rs < Rt) {
		MCInst_setOpcode(MI, Mips_BNEC);
		HasRs = true;
	} else
		MCInst_setOpcode(MI, Mips_BNEZALC);

	if (HasRs)
		MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rs));

	MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rt));
	MCOperand_CreateImm0(MI, Imm);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeBlezlGroupBranch_4(MCInst *MI, uint32_t insn,
		uint64_t Address, const MCRegisterInfo *Decoder)
{

	uint32_t Rs = fieldFromInstruction(insn, 21, 5);
	uint32_t Rt = fieldFromInstruction(insn, 16, 5);
	uint32_t Imm = (uint32_t)SignExtend64(fieldFromInstruction(insn, 0, 16), 16) * 4;
	bool HasRs = false;

	if (Rt == 0)
		return MCDisassembler_Fail;
	else if (Rs == 0)
		MCInst_setOpcode(MI, Mips_BLEZC);
	else if (Rs == Rt)
		MCInst_setOpcode(MI, Mips_BGEZC);
	else {
		HasRs = true;
		MCInst_setOpcode(MI, Mips_BGEC);
	}

	if (HasRs)
		MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rs));

	MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rt));

	MCOperand_CreateImm0(MI, Imm);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeBgtzlGroupBranch_4(MCInst *MI, uint32_t insn,
		uint64_t Address, const MCRegisterInfo *Decoder)
{

	bool HasRs = false;

	uint32_t Rs = fieldFromInstruction(insn, 21, 5);
	uint32_t Rt = fieldFromInstruction(insn, 16, 5);
	uint32_t Imm = (uint32_t)SignExtend64(fieldFromInstruction(insn, 0, 16), 16) * 4;

	if (Rt == 0)
		return MCDisassembler_Fail;
	else if (Rs == 0)
		MCInst_setOpcode(MI, Mips_BGTZC);
	else if (Rs == Rt)
		MCInst_setOpcode(MI, Mips_BLTZC);
	else {
		MCInst_setOpcode(MI, Mips_BLTC);
		HasRs = true;
	}

	if (HasRs)
		MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rs));

	MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rt));
	MCOperand_CreateImm0(MI, Imm);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeBgtzGroupBranch_4(MCInst *MI, uint32_t insn,
		uint64_t Address, const MCRegisterInfo *Decoder)
{

	uint32_t Rs = fieldFromInstruction(insn, 21, 5);
	uint32_t Rt = fieldFromInstruction(insn, 16, 5);
	uint32_t Imm = (uint32_t)SignExtend64(fieldFromInstruction(insn, 0, 16), 16) * 4;
	bool HasRs = false;
	bool HasRt = false;

	if (Rt == 0) {
		MCInst_setOpcode(MI, Mips_BGTZ);
		HasRs = true;
	} else if (Rs == 0) {
		MCInst_setOpcode(MI, Mips_BGTZALC);
		HasRt = true;
	} else if (Rs == Rt) {
		MCInst_setOpcode(MI, Mips_BLTZALC);
		HasRs = true;
	} else {
		MCInst_setOpcode(MI, Mips_BLTUC);
		HasRs = true;
		HasRt = true;
	}

	if (HasRs)
		MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rs));

	if (HasRt)
		MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rt));

	MCOperand_CreateImm0(MI, Imm);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeBlezGroupBranch_4(MCInst *MI, uint32_t insn,
		uint64_t Address, const MCRegisterInfo *Decoder)
{

	uint32_t Rs = fieldFromInstruction(insn, 21, 5);
	uint32_t Rt = fieldFromInstruction(insn, 16, 5);
	uint32_t Imm = (uint32_t)SignExtend64(fieldFromInstruction(insn, 0, 16), 16) * 4;
	bool HasRs = false;

	if (Rt == 0)
		return MCDisassembler_Fail;
	else if (Rs == 0)
		MCInst_setOpcode(MI, Mips_BLEZALC);
	else if (Rs == Rt)
		MCInst_setOpcode(MI, Mips_BGEZALC);
	else {
		HasRs = true;
		MCInst_setOpcode(MI, Mips_BGEUC);
	}

	if (HasRs)
		MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rs));

	MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rt));

	MCOperand_CreateImm0(MI, Imm);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeCPU16RegsRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	return MCDisassembler_Fail;
}

static DecodeStatus DecodeGPR64RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_GPR64RegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeGPRMM16RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 7)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_GPRMM16RegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeGPRMM16ZeroRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 7)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_GPRMM16ZeroRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeGPRMM16MovePRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 7)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_GPRMM16MovePRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeGPR32RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_GPR32RegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodePtrRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	if (Inst->csh->mode & CS_MODE_MIPS64)
		return DecodeGPR64RegisterClass(Inst, RegNo, Address, Decoder);

	return DecodeGPR32RegisterClass(Inst, RegNo, Address, Decoder);
}

static DecodeStatus DecodeDSPRRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	return DecodeGPR32RegisterClass(Inst, RegNo, Address, Decoder);
}

static DecodeStatus DecodeFGR64RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_FGR64RegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeFGR32RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_FGR32RegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeCCRRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_CCRRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeFCCRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 7)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_FCCRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeCCRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 7)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_CCRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeFGRCCRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_FGRCCRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeMem(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(Insn & 0xffff, 16);
	unsigned Reg = fieldFromInstruction(Insn, 16, 5);
	unsigned Base = fieldFromInstruction(Insn, 21, 5);
	int opcode = MCInst_getOpcode(Inst);

	Reg = getReg(Decoder, Mips_GPR32RegClassID, Reg);
	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	if (opcode == Mips_SC || opcode == Mips_SCD) {
		MCOperand_CreateReg0(Inst, Reg);
	}

	MCOperand_CreateReg0(Inst, Reg);
	MCOperand_CreateReg0(Inst, Base);
	MCOperand_CreateImm0(Inst, Offset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeCacheOp(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(Insn & 0xffff, 16);
	unsigned Hint = fieldFromInstruction(Insn, 16, 5);
	unsigned Base = fieldFromInstruction(Insn, 21, 5);

	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	MCOperand_CreateReg0(Inst, Base);
	MCOperand_CreateImm0(Inst, Offset);
	MCOperand_CreateImm0(Inst, Hint);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeCacheOpMM(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(Insn & 0xfff, 12);
	unsigned Base = fieldFromInstruction(Insn, 16, 5);
	unsigned Hint = fieldFromInstruction(Insn, 21, 5);

	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	MCOperand_CreateReg0(Inst, Base);
	MCOperand_CreateImm0(Inst, Offset);
	MCOperand_CreateImm0(Inst, Hint);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeCacheOpR6(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	int Offset = fieldFromInstruction(Insn, 7, 9);
	unsigned Hint = fieldFromInstruction(Insn, 16, 5);
	unsigned Base = fieldFromInstruction(Insn, 21, 5);

	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	MCOperand_CreateReg0(Inst, Base);
	MCOperand_CreateImm0(Inst, Offset);
	MCOperand_CreateImm0(Inst, Hint);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeSyncI(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(Insn & 0xffff, 16);
	unsigned Base = fieldFromInstruction(Insn, 21, 5);

	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	MCOperand_CreateReg0(Inst, Base);
	MCOperand_CreateImm0(Inst, Offset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMSA128Mem(MCInst *Inst, unsigned Insn,
		uint64_t Address, const MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(fieldFromInstruction(Insn, 16, 10), 10);
	unsigned Reg = fieldFromInstruction(Insn, 6, 5);
	unsigned Base = fieldFromInstruction(Insn, 11, 5);

	Reg = getReg(Decoder, Mips_MSA128BRegClassID, Reg);
	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	MCOperand_CreateReg0(Inst, Reg);
	MCOperand_CreateReg0(Inst, Base);

	switch(MCInst_getOpcode(Inst)) {
		default:
			return MCDisassembler_Fail;
			break;
		case Mips_LD_B:
		case Mips_ST_B:
			MCOperand_CreateImm0(Inst, Offset);
			break;
		case Mips_LD_H:
		case Mips_ST_H:
			MCOperand_CreateImm0(Inst, Offset * 2);
			break;
		case Mips_LD_W:
		case Mips_ST_W:
			MCOperand_CreateImm0(Inst, Offset * 4);
			break;
		case Mips_LD_D:
		case Mips_ST_D:
			MCOperand_CreateImm0(Inst, Offset * 8);
			break;
	}

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMemMMImm4(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Offset = Insn & 0xf;
	unsigned Reg = fieldFromInstruction(Insn, 7, 3);
	unsigned Base = fieldFromInstruction(Insn, 4, 3);

	switch (MCInst_getOpcode(Inst)) {
		case Mips_LBU16_MM:
		case Mips_LHU16_MM:
		case Mips_LW16_MM:
			if (DecodeGPRMM16RegisterClass(Inst, Reg, Address, Decoder)
					== MCDisassembler_Fail)
				return MCDisassembler_Fail;
			break;
		case Mips_SB16_MM:
		case Mips_SH16_MM:
		case Mips_SW16_MM:
			if (DecodeGPRMM16ZeroRegisterClass(Inst, Reg, Address, Decoder)
					== MCDisassembler_Fail)
				return MCDisassembler_Fail;
			break;
	}

	if (DecodeGPRMM16RegisterClass(Inst, Base, Address, Decoder)
			== MCDisassembler_Fail)
		return MCDisassembler_Fail;

	switch (MCInst_getOpcode(Inst)) {
		case Mips_LBU16_MM:
			if (Offset == 0xf)
				MCOperand_CreateImm0(Inst, -1);
			else
				MCOperand_CreateImm0(Inst, Offset);
			break;
		case Mips_SB16_MM:
			MCOperand_CreateImm0(Inst, Offset);
			break;
		case Mips_LHU16_MM:
		case Mips_SH16_MM:
			MCOperand_CreateImm0(Inst, Offset << 1);
			break;
		case Mips_LW16_MM:
		case Mips_SW16_MM:
			MCOperand_CreateImm0(Inst, Offset << 2);
			break;
	}

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMemMMSPImm5Lsl2(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Offset = Insn & 0x1F;
	unsigned Reg = fieldFromInstruction(Insn, 5, 5);

	Reg = getReg(Decoder, Mips_GPR32RegClassID, Reg);

	MCOperand_CreateReg0(Inst, Reg);
	MCOperand_CreateReg0(Inst, Mips_SP);
	MCOperand_CreateImm0(Inst, Offset << 2);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMemMMGPImm7Lsl2(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Offset = Insn & 0x7F;
	unsigned Reg = fieldFromInstruction(Insn, 7, 3);

	Reg = getReg(Decoder, Mips_GPR32RegClassID, Reg);

	MCOperand_CreateReg0(Inst, Reg);
	MCOperand_CreateReg0(Inst, Mips_GP);
	MCOperand_CreateImm0(Inst, Offset << 2);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMemMMReglistImm4Lsl2(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(Insn & 0xf, 4);

	if (DecodeRegListOperand16(Inst, Insn, Address, Decoder) == MCDisassembler_Fail)
		return MCDisassembler_Fail;

	MCOperand_CreateReg0(Inst, Mips_SP);
	MCOperand_CreateImm0(Inst, Offset * 4);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMemMMImm12(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(Insn & 0x0fff, 12);
	unsigned Reg = fieldFromInstruction(Insn, 21, 5);
	unsigned Base = fieldFromInstruction(Insn, 16, 5);

	Reg = getReg(Decoder, Mips_GPR32RegClassID, Reg);
	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	switch (MCInst_getOpcode(Inst)) {
		case Mips_SWM32_MM:
		case Mips_LWM32_MM:
			if (DecodeRegListOperand(Inst, Insn, Address, Decoder)
					== MCDisassembler_Fail)
				return MCDisassembler_Fail;
			MCOperand_CreateReg0(Inst, Base);
			MCOperand_CreateImm0(Inst, Offset);
			break;
		case Mips_SC_MM:
			MCOperand_CreateReg0(Inst, Reg);
		default:
			MCOperand_CreateReg0(Inst, Reg);
			if (MCInst_getOpcode(Inst) == Mips_LWP_MM || MCInst_getOpcode(Inst) == Mips_SWP_MM)
				MCOperand_CreateReg0(Inst, Reg + 1);

			MCOperand_CreateReg0(Inst, Base);
			MCOperand_CreateImm0(Inst, Offset);
	}

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMemMMImm16(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(Insn & 0xffff, 16);
	unsigned Reg = fieldFromInstruction(Insn, 21, 5);
	unsigned Base = fieldFromInstruction(Insn, 16, 5);

	Reg = getReg(Decoder, Mips_GPR32RegClassID, Reg);
	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	MCOperand_CreateReg0(Inst, Reg);
	MCOperand_CreateReg0(Inst, Base);
	MCOperand_CreateImm0(Inst, Offset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeFMem(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(Insn & 0xffff, 16);
	unsigned Reg = fieldFromInstruction(Insn, 16, 5);
	unsigned Base = fieldFromInstruction(Insn, 21, 5);

	Reg = getReg(Decoder, Mips_FGR64RegClassID, Reg);
	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	MCOperand_CreateReg0(Inst, Reg);
	MCOperand_CreateReg0(Inst, Base);
	MCOperand_CreateImm0(Inst, Offset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeFMem2(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(Insn & 0xffff, 16);
	unsigned Reg = fieldFromInstruction(Insn, 16, 5);
	unsigned Base = fieldFromInstruction(Insn, 21, 5);

	Reg = getReg(Decoder, Mips_COP2RegClassID, Reg);
	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	MCOperand_CreateReg0(Inst, Reg);
	MCOperand_CreateReg0(Inst, Base);
	MCOperand_CreateImm0(Inst, Offset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeFMem3(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(Insn & 0xffff, 16);
	unsigned Reg = fieldFromInstruction(Insn, 16, 5);
	unsigned Base = fieldFromInstruction(Insn, 21, 5);

	Reg = getReg(Decoder, Mips_COP3RegClassID, Reg);
	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	MCOperand_CreateReg0(Inst, Reg);
	MCOperand_CreateReg0(Inst, Base);
	MCOperand_CreateImm0(Inst, Offset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeFMemCop2R6(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(Insn & 0x07ff, 11);
	unsigned Reg = fieldFromInstruction(Insn, 16, 5);
	unsigned Base = fieldFromInstruction(Insn, 11, 5);

	Reg = getReg(Decoder, Mips_COP2RegClassID, Reg);
	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	MCOperand_CreateReg0(Inst, Reg);
	MCOperand_CreateReg0(Inst, Base);
	MCOperand_CreateImm0(Inst, Offset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeSpecial3LlSc(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder)
{
	int64_t Offset = SignExtend64((Insn >> 7) & 0x1ff, 9);
	unsigned Rt = fieldFromInstruction(Insn, 16, 5);
	unsigned Base = fieldFromInstruction(Insn, 21, 5);

	Rt = getReg(Decoder, Mips_GPR32RegClassID, Rt);
	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	if (MCInst_getOpcode(Inst) == Mips_SC_R6 ||
			MCInst_getOpcode(Inst) == Mips_SCD_R6) {
		MCOperand_CreateReg0(Inst, Rt);
	}

	MCOperand_CreateReg0(Inst, Rt);
	MCOperand_CreateReg0(Inst, Base);
	MCOperand_CreateImm0(Inst, Offset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeHWRegsRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	if (RegNo != 29)
		return  MCDisassembler_Fail;

	MCOperand_CreateReg0(Inst, Mips_HWR29);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeAFGR64RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 30 || RegNo % 2)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_AFGR64RegClassID, RegNo /2);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeACC64DSPRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo >= 4)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_ACC64DSPRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeHI32DSPRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo >= 4)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_HI32DSPRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeLO32DSPRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo >= 4)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_LO32DSPRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMSA128BRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_MSA128BRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMSA128HRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_MSA128HRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMSA128WRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_MSA128WRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMSA128DRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_MSA128DRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMSACtrlRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 7)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_MSACtrlRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeCOP2RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_COP2RegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeBranchTarget(MCInst *Inst,
		unsigned Offset, uint64_t Address, const MCRegisterInfo *Decoder)
{
	uint64_t TargetAddress = (SignExtend32(Offset, 16) * 4) + Address + 4;
	MCOperand_CreateImm0(Inst, TargetAddress);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeJumpTarget(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder)
{
	uint64_t TargetAddress = (fieldFromInstruction(Insn, 0, 26) << 2) | ((Address + 4) & ~0x0FFFFFFF);
	MCOperand_CreateImm0(Inst, TargetAddress);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeBranchTarget21(MCInst *Inst,
		unsigned Offset, uint64_t Address, const MCRegisterInfo *Decoder)
{
	int32_t BranchOffset = SignExtend32(Offset, 21) * 4;

	MCOperand_CreateImm0(Inst, BranchOffset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeBranchTarget26(MCInst *Inst,
		unsigned Offset, uint64_t Address, const MCRegisterInfo *Decoder)
{
	int32_t BranchOffset = SignExtend32(Offset, 26) * 4;

	MCOperand_CreateImm0(Inst, BranchOffset);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeBranchTarget7MM(MCInst *Inst,
		unsigned Offset, uint64_t Address, MCRegisterInfo *Decoder)
{
	int32_t BranchOffset = SignExtend32(Offset, 7) * 2;
	MCOperand_CreateImm0(Inst, BranchOffset);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeBranchTarget10MM(MCInst *Inst,
		unsigned Offset, uint64_t Address, MCRegisterInfo *Decoder)
{
	int32_t BranchOffset = SignExtend32(Offset, 10) * 2;
	MCOperand_CreateImm0(Inst, BranchOffset);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeBranchTargetMM(MCInst *Inst,
		unsigned Offset, uint64_t Address, const MCRegisterInfo *Decoder)
{
	int32_t BranchOffset = SignExtend32(Offset, 16) * 2;
	MCOperand_CreateImm0(Inst, BranchOffset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeJumpTargetMM(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned JumpOffset = fieldFromInstruction(Insn, 0, 26) << 1;
	MCOperand_CreateImm0(Inst, JumpOffset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeAddiur2Simm7(MCInst *Inst,
		unsigned Value, uint64_t Address, MCRegisterInfo *Decoder)
{
	if (Value == 0)
		MCOperand_CreateImm0(Inst, 1);
	else if (Value == 0x7)
		MCOperand_CreateImm0(Inst, -1);
	else
		MCOperand_CreateImm0(Inst, Value << 2);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeUImm6Lsl2(MCInst *Inst,
		unsigned Value, uint64_t Address, MCRegisterInfo *Decoder)
{
	MCOperand_CreateImm0(Inst, Value << 2);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeLiSimm7(MCInst *Inst,
		unsigned Value, uint64_t Address, MCRegisterInfo *Decoder)
{
	if (Value == 0x7F)
		MCOperand_CreateImm0(Inst, -1);
	else
		MCOperand_CreateImm0(Inst, Value);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeSimm4(MCInst *Inst,
		unsigned Value, uint64_t Address, MCRegisterInfo *Decoder)
{
	MCOperand_CreateImm0(Inst, SignExtend32(Value, 4));

	return MCDisassembler_Success;
}

static DecodeStatus DecodeSimm16(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder)
{
	MCOperand_CreateImm0(Inst, SignExtend32(Insn, 16));

	return MCDisassembler_Success;
}

static DecodeStatus DecodeLSAImm(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder)
{
	MCOperand_CreateImm0(Inst, Insn + 1);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeInsSize(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder)
{
	int Pos = (int)MCOperand_getImm(MCInst_getOperand(Inst, 2));
	int Size = (int) Insn - Pos + 1;
	MCOperand_CreateImm0(Inst, SignExtend32(Size, 16));

	return MCDisassembler_Success;
}

static DecodeStatus DecodeExtSize(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder)
{
	int Size = (int)Insn  + 1;

	MCOperand_CreateImm0(Inst, SignExtend32(Size, 16));

	return MCDisassembler_Success;
}

static DecodeStatus DecodeSimm19Lsl2(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder)
{
	MCOperand_CreateImm0(Inst, SignExtend32(Insn, 19) * 4);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeSimm18Lsl3(MCInst *Inst,
		unsigned Insn, uint64_t Address, const MCRegisterInfo *Decoder)
{
	MCOperand_CreateImm0(Inst, SignExtend32(Insn, 18) * 8);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeSimm9SP(MCInst *Inst, unsigned Insn,
		uint64_t Address, MCRegisterInfo *Decoder)
{
	int32_t DecodedValue;

	switch (Insn) {
		case 0: DecodedValue = 256; break;
		case 1: DecodedValue = 257; break;
		case 510: DecodedValue = -258; break;
		case 511: DecodedValue = -257; break;
		default: DecodedValue = SignExtend32(Insn, 9); break;
	}
	MCOperand_CreateImm0(Inst, DecodedValue * 4);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeANDI16Imm(MCInst *Inst, unsigned Insn,
		uint64_t Address, MCRegisterInfo *Decoder)
{
	int32_t DecodedValues[] = {128, 1, 2, 3, 4, 7, 8, 15, 16, 31, 32, 63, 64,
		255, 32768, 65535};

	if (Insn >= 16)
		return MCDisassembler_Fail;

	MCOperand_CreateImm0(Inst, DecodedValues[Insn]);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeUImm5lsl2(MCInst *Inst, unsigned Insn,
		uint64_t Address, MCRegisterInfo *Decoder)
{
	MCOperand_CreateImm0(Inst, Insn << 2);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeRegListOperand(MCInst *Inst, unsigned Insn,
		uint64_t Address, const MCRegisterInfo *Decoder)
{
	unsigned Regs[] = {Mips_S0, Mips_S1, Mips_S2, Mips_S3, Mips_S4, Mips_S5,
		Mips_S6, Mips_FP};
	unsigned RegNum;
	unsigned int i;

	unsigned RegLst = fieldFromInstruction(Insn, 21, 5);
	if (RegLst == 0)
		return MCDisassembler_Fail;

	RegNum = RegLst & 0xf;
	for (i = 0; i < MIN(RegNum, ARR_SIZE(Regs)); i++)
		MCOperand_CreateReg0(Inst, Regs[i]);

	if (RegLst & 0x10)
		MCOperand_CreateReg0(Inst, Mips_RA);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeRegListOperand16(MCInst *Inst, unsigned Insn,
		uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Regs[] = {Mips_S0, Mips_S1, Mips_S2, Mips_S3};
	unsigned RegLst = fieldFromInstruction(Insn, 4, 2);
	unsigned RegNum = RegLst & 0x3;
	unsigned int i;

	for (i = 0; i <= RegNum; i++)
		MCOperand_CreateReg0(Inst, Regs[i]);

	MCOperand_CreateReg0(Inst, Mips_RA);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMovePRegPair(MCInst *Inst, unsigned Insn,
		uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned RegPair = fieldFromInstruction(Insn, 7, 3);

	switch (RegPair) {
		default:
			return MCDisassembler_Fail;
		case 0:
			MCOperand_CreateReg0(Inst, Mips_A1);
			MCOperand_CreateReg0(Inst, Mips_A2);
			break;
		case 1:
			MCOperand_CreateReg0(Inst, Mips_A1);
			MCOperand_CreateReg0(Inst, Mips_A3);
			break;
		case 2:
			MCOperand_CreateReg0(Inst, Mips_A2);
			MCOperand_CreateReg0(Inst, Mips_A3);
			break;
		case 3:
			MCOperand_CreateReg0(Inst, Mips_A0);
			MCOperand_CreateReg0(Inst, Mips_S5);
			break;
		case 4:
			MCOperand_CreateReg0(Inst, Mips_A0);
			MCOperand_CreateReg0(Inst, Mips_S6);
			break;
		case 5:
			MCOperand_CreateReg0(Inst, Mips_A0);
			MCOperand_CreateReg0(Inst, Mips_A1);
			break;
		case 6:
			MCOperand_CreateReg0(Inst, Mips_A0);
			MCOperand_CreateReg0(Inst, Mips_A2);
			break;
		case 7:
			MCOperand_CreateReg0(Inst, Mips_A0);
			MCOperand_CreateReg0(Inst, Mips_A3);
			break;
	}

	return MCDisassembler_Success;
}

static DecodeStatus DecodeSimm23Lsl2(MCInst *Inst, unsigned Insn,
		uint64_t Address, MCRegisterInfo *Decoder)
{
	MCOperand_CreateImm0(Inst, SignExtend32(Insn, 23) * 4);
	return MCDisassembler_Success;
}

#endif
