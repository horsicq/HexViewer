
/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2019 */

#ifndef CS_X86_INSTPRINTER_H
#define CS_X86_INSTPRINTER_H

#include "../../MCInst.h"
#include "../../SStream.h"

void X86_Intel_printInst(MCInst *MI, SStream *OS, void *Info);
void X86_ATT_printInst(MCInst *MI, SStream *OS, void *Info);

#endif
