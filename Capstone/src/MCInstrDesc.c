/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2019 */

#include "MCInstrDesc.h"

bool MCOperandInfo_isPredicate(const MCOperandInfo *m)
{
	return m->Flags & (1 << MCOI_Predicate);
}

bool MCOperandInfo_isOptionalDef(const MCOperandInfo *m)
{
	return m->Flags & (1 << MCOI_OptionalDef);
}
