/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_SPARC_MAP_H
#define CS_SPARC_MAP_H

#include "capstone/capstone.h"

const char *Sparc_reg_name(csh handle, unsigned int reg);

void Sparc_get_insn_id(cs_struct *h, cs_insn *insn, unsigned int id);

const char *Sparc_insn_name(csh handle, unsigned int id);

const char *Sparc_group_name(csh handle, unsigned int id);

sparc_reg Sparc_map_register(unsigned int r);

sparc_reg Sparc_map_insn(const char *name);

sparc_cc Sparc_map_ICC(const char *name);

sparc_cc Sparc_map_FCC(const char *name);

sparc_hint Sparc_map_hint(const char *name);

#endif

