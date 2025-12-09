
/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2019 */

#ifndef CS_ARMINSTPRINTER_H
#define CS_ARMINSTPRINTER_H

#include "../../MCInst.h"
#include "../../MCRegisterInfo.h"
#include "../../SStream.h"

void ARM_printInst(MCInst *MI, SStream *O, void *Info);
void ARM_post_printer(csh handle, cs_insn *pub_insn, char *mnem, MCInst *mci);

void ARM_getRegName(cs_struct *handle, int value);

void ARM_addVectorDataType(MCInst *MI, arm_vectordata_type vd);

void ARM_addVectorDataSize(MCInst *MI, int size);

void ARM_addReg(MCInst *MI, int reg);

void ARM_addUserMode(MCInst *MI);

void ARM_addSysReg(MCInst *MI, arm_sysreg reg);

#endif
