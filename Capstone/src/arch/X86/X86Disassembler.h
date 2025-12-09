
/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2019 */

#ifndef CS_X86_DISASSEMBLER_H
#define CS_X86_DISASSEMBLER_H

#include "capstone/capstone.h"

#include "../../MCInst.h"

#include "../../MCRegisterInfo.h"
#include "X86DisassemblerDecoderCommon.h"

bool X86_getInstruction(csh handle, const uint8_t *code, size_t code_len,
		MCInst *instr, uint16_t *size, uint64_t address, void *info);

void X86_init(MCRegisterInfo *MRI);

#endif
