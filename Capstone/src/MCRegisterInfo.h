
/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2019 */

#ifndef CS_LLVM_MC_MCREGISTERINFO_H
#define CS_LLVM_MC_MCREGISTERINFO_H

#include "capstone/platform.h"

typedef uint16_t MCPhysReg;
typedef const MCPhysReg* iterator;

typedef struct MCRegisterClass2 {
	iterator RegsBegin;
	const uint8_t *RegSet;
	uint8_t RegsSize;
	uint8_t RegSetSize;
} MCRegisterClass2;

typedef struct MCRegisterClass {
	iterator RegsBegin;
	const uint8_t *RegSet;
	uint16_t RegSetSize;
} MCRegisterClass;

typedef struct MCRegisterDesc {
	uint32_t Name;      // Printable name for the reg (for debugging)
	uint32_t SubRegs;   // Sub-register set, described above
	uint32_t SuperRegs; // Super-register set, described above

	uint32_t SubRegIndices;

	uint32_t RegUnits;

	uint16_t RegUnitLaneMasks;	// ???
} MCRegisterDesc;

typedef struct MCRegisterInfo {
	const MCRegisterDesc *Desc;                 // Pointer to the descriptor array
	unsigned NumRegs;                           // Number of entries in the array
	unsigned RAReg;                             // Return address register
	unsigned PCReg;                             // Program counter register
	const MCRegisterClass *Classes;             // Pointer to the regclass array
	unsigned NumClasses;                        // Number of entries in the array
	unsigned NumRegUnits;                       // Number of regunits.
	uint16_t (*RegUnitRoots)[2];          // Pointer to regunit root table.
	const MCPhysReg *DiffLists;                 // Pointer to the difflists array
	const char *RegStrings;                     // Pointer to the string table.
	const uint16_t *SubRegIndices;              // Pointer to the subreg lookup
	unsigned NumSubRegIndices;                  // Number of subreg indices.
	const uint16_t *RegEncodingTable;           // Pointer to array of register
} MCRegisterInfo;

void MCRegisterInfo_InitMCRegisterInfo(MCRegisterInfo *RI,
		const MCRegisterDesc *D, unsigned NR, unsigned RA,
		unsigned PC,
		const MCRegisterClass *C, unsigned NC,
		uint16_t (*RURoots)[2],
		unsigned NRU,
		const MCPhysReg *DL,
		const char *Strings,
		const uint16_t *SubIndices,
		unsigned NumIndices,
		const uint16_t *RET);

unsigned MCRegisterInfo_getMatchingSuperReg(const MCRegisterInfo *RI, unsigned Reg, unsigned SubIdx, const MCRegisterClass *RC);

unsigned MCRegisterInfo_getSubReg(const MCRegisterInfo *RI, unsigned Reg, unsigned Idx);

const MCRegisterClass* MCRegisterInfo_getRegClass(const MCRegisterInfo *RI, unsigned i);

bool MCRegisterClass_contains(const MCRegisterClass *c, unsigned Reg);

#endif
