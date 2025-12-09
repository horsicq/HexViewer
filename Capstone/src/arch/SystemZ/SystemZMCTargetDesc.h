
/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_SYSTEMZMCTARGETDESC_H
#define CS_SYSTEMZMCTARGETDESC_H

extern const unsigned SystemZMC_GR32Regs[16];
extern const unsigned SystemZMC_GRH32Regs[16];
extern const unsigned SystemZMC_GR64Regs[16];
extern const unsigned SystemZMC_GR128Regs[16];
extern const unsigned SystemZMC_FP32Regs[16];
extern const unsigned SystemZMC_FP64Regs[16];
extern const unsigned SystemZMC_FP128Regs[16];
extern const unsigned SystemZMC_VR32Regs[32];
extern const unsigned SystemZMC_VR64Regs[32];
extern const unsigned SystemZMC_VR128Regs[32];
extern const unsigned SystemZMC_AR32Regs[16];
extern const unsigned SystemZMC_CR64Regs[16];

unsigned SystemZMC_getFirstReg(unsigned Reg);




#endif
