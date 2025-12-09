
/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2019 */

#ifndef CS_LLVM_AARCH64INSTPRINTER_H
#define CS_LLVM_AARCH64INSTPRINTER_H

#include "../../MCInst.h"
#include "../../MCRegisterInfo.h"
#include "../../SStream.h"

void AArch64_printInst(MCInst *MI, SStream *O, void *);

void AArch64_post_printer(csh handle, cs_insn *pub_insn, char *insn_asm, MCInst *mci);

#endif
