
/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_MIPSINSTPRINTER_H
#define CS_MIPSINSTPRINTER_H

#include "../../MCInst.h"
#include "../../SStream.h"

void Mips_printInst(MCInst *MI, SStream *O, void *info);

#endif
