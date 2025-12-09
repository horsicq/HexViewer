/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_PPC_MAP_H
#define CS_PPC_MAP_H

#include "capstone/capstone.h"

const char *PPC_reg_name(csh handle, unsigned int reg);

ppc_reg PPC_name_reg(const char *name);

void PPC_get_insn_id(cs_struct *h, cs_insn *insn, unsigned int id);

const char *PPC_insn_name(csh handle, unsigned int id);
const char *PPC_group_name(csh handle, unsigned int id);

struct ppc_alias {
	unsigned int id;	// instruction id
	int cc;	// code condition
	const char *mnem;
};

ppc_insn PPC_map_insn(const char *name);

bool PPC_abs_branch(cs_struct *h, unsigned int id);

ppc_reg PPC_map_register(unsigned int r);

bool PPC_alias_insn(const char *name, struct ppc_alias *alias);

#endif

