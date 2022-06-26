//===----------------------------- Registers.hpp --------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//
//  Models register sets for supported processors.
//
//===----------------------------------------------------------------------===//
#ifndef __REGISTERS_HPP__
#define __REGISTERS_HPP__

#include <sys/endian.h>
#include <cassert>
#include <cstdint>

namespace _Unwind {

enum {
  REGNO_X86_EAX = 0,
  REGNO_X86_ECX = 1,
  REGNO_X86_EDX = 2,
  REGNO_X86_EBX = 3,
  REGNO_X86_ESP = 4,
  REGNO_X86_EBP = 5,
  REGNO_X86_ESI = 6,
  REGNO_X86_EDI = 7,
  REGNO_X86_EIP = 8,
};

class Registers_x86 {
public:
  enum {
    LAST_REGISTER = REGNO_X86_EIP,
    LAST_RESTORE_REG = REGNO_X86_EIP,
    RETURN_OFFSET = 0,
    RETURN_MASK = 0,
  };

  __dso_hidden Registers_x86();

  static int dwarf2regno(int num) { return num; }

  bool validRegister(int num) const {
    return num >= REGNO_X86_EAX && num <= REGNO_X86_EDI;
  }

  uint32_t getRegister(int num) const {
    assert(validRegister(num));
    return reg[num];
  }

  void setRegister(int num, uint32_t value) {
    assert(validRegister(num));
    reg[num] = value;
  }

  uint32_t getIP() const { return reg[REGNO_X86_EIP]; }

  void setIP(uint32_t value) { reg[REGNO_X86_EIP] = value; }

  uint32_t getSP() const { return reg[REGNO_X86_ESP]; }

  void setSP(uint32_t value) { reg[REGNO_X86_ESP] = value; }

  bool validFloatVectorRegister(int num) const { return false; }

  void copyFloatVectorRegister(int num, uint32_t addr) {
  }

  __dso_hidden void jumpto() const __dead;

private:
  uint32_t reg[REGNO_X86_EIP + 1];
};

enum {
  REGNO_X86_64_RAX = 0,
  REGNO_X86_64_RDX = 1,
  REGNO_X86_64_RCX = 2,
  REGNO_X86_64_RBX = 3,
  REGNO_X86_64_RSI = 4,
  REGNO_X86_64_RDI = 5,
  REGNO_X86_64_RBP = 6,
  REGNO_X86_64_RSP = 7,
  REGNO_X86_64_R8 = 8,
  REGNO_X86_64_R9 = 9,
  REGNO_X86_64_R10 = 10,
  REGNO_X86_64_R11 = 11,
  REGNO_X86_64_R12 = 12,
  REGNO_X86_64_R13 = 13,
  REGNO_X86_64_R14 = 14,
  REGNO_X86_64_R15 = 15,
  REGNO_X86_64_RIP = 16,
};

class Registers_x86_64 {
public:
  enum {
    LAST_REGISTER = REGNO_X86_64_RIP,
    LAST_RESTORE_REG = REGNO_X86_64_RIP,
    RETURN_OFFSET = 0,
    RETURN_MASK = 0,
  };

  __dso_hidden Registers_x86_64();

  static int dwarf2regno(int num) { return num; }

  bool validRegister(int num) const {
    return num >= REGNO_X86_64_RAX && num <= REGNO_X86_64_R15;
  }

  uint64_t getRegister(int num) const {
    assert(validRegister(num));
    return reg[num];
  }

  void setRegister(int num, uint64_t value) {
    assert(validRegister(num));
    reg[num] = value;
  }

  uint64_t getIP() const { return reg[REGNO_X86_64_RIP]; }

  void setIP(uint64_t value) { reg[REGNO_X86_64_RIP] = value; }

  uint64_t getSP() const { return reg[REGNO_X86_64_RSP]; }

  void setSP(uint64_t value) { reg[REGNO_X86_64_RSP] = value; }

  bool validFloatVectorRegister(int num) const { return false; }

  void copyFloatVectorRegister(int num, uint64_t addr) {
  }

  __dso_hidden void jumpto() const __dead;

private:
  uint64_t reg[REGNO_X86_64_RIP + 1];
};

enum {
  DWARF_PPC32_R0 = 0,
  DWARF_PPC32_R31 = 31,
  DWARF_PPC32_F0 = 32,
  DWARF_PPC32_F31 = 63,
  DWARF_PPC32_LR = 65,
  DWARF_PPC32_CTR = 66,
  DWARF_PPC32_CR = 70,
  DWARF_PPC32_XER = 76,
  DWARF_PPC32_V0 = 77,
  DWARF_PPC32_SIGRETURN = 99,
  DWARF_PPC32_V31 = 108,

  REGNO_PPC32_R0 = 0,
  REGNO_PPC32_R1 = 1,
  REGNO_PPC32_R31 = 31,
  REGNO_PPC32_LR = 32,
  REGNO_PPC32_CR = 33,
  REGNO_PPC32_SRR0 = 34,

  REGNO_PPC32_F0 = REGNO_PPC32_SRR0 + 1,
  REGNO_PPC32_F31 = REGNO_PPC32_F0 + 31,
  REGNO_PPC32_V0 = REGNO_PPC32_F31 + 1,
  REGNO_PPC32_V31 = REGNO_PPC32_V0 + 31,

  REGNO_PPC32_CTR = REGNO_PPC32_V31 + 1,
  REGNO_PPC32_XER = REGNO_PPC32_CTR + 1,
  REGNO_PPC32_SIGRETURN = REGNO_PPC32_XER + 1
};

class Registers_ppc32 {
public:
  enum {
    LAST_REGISTER = REGNO_PPC32_SIGRETURN,
    LAST_RESTORE_REG = REGNO_PPC32_SIGRETURN,
    RETURN_OFFSET = 0,
    RETURN_MASK = 0,
  };

  __dso_hidden Registers_ppc32();

  static int dwarf2regno(int num) {
    if (num >= DWARF_PPC32_R0 && num <= DWARF_PPC32_R31)
      return REGNO_PPC32_R0 + (num - DWARF_PPC32_R0);
    if (num >= DWARF_PPC32_F0 && num <= DWARF_PPC32_F31)
      return REGNO_PPC32_F0 + (num - DWARF_PPC32_F0);
    if (num >= DWARF_PPC32_V0 && num <= DWARF_PPC32_V31)
      return REGNO_PPC32_V0 + (num - DWARF_PPC32_V0);
    switch (num) {
    case DWARF_PPC32_LR:
      return REGNO_PPC32_LR;
    case DWARF_PPC32_CR:
      return REGNO_PPC32_CR;
    case DWARF_PPC32_CTR:
      return REGNO_PPC32_CTR;
    case DWARF_PPC32_XER:
      return REGNO_PPC32_XER;
    case DWARF_PPC32_SIGRETURN:
      return REGNO_PPC32_SIGRETURN;
    default:
      return LAST_REGISTER + 1;
    }
  }

  bool validRegister(int num) const {
    return (num >= 0 && num <= REGNO_PPC32_SRR0) ||
	(num >= REGNO_PPC32_CTR && num <= REGNO_PPC32_SIGRETURN);
  }

  uint64_t getRegister(int num) const {
    assert(validRegister(num));
    switch (num) {
    case REGNO_PPC32_CTR:
      return ctr_reg;
    case REGNO_PPC32_XER:
      return xer_reg;
    case REGNO_PPC32_SIGRETURN:
      return sigreturn_reg;
    default:
      return reg[num];
    }
  }

  void setRegister(int num, uint64_t value) {
    assert(validRegister(num));
    switch (num) {
    case REGNO_PPC32_CTR:
      ctr_reg = value;
      break;
    case REGNO_PPC32_XER:
      xer_reg = value;
      break;
    case REGNO_PPC32_SIGRETURN:
      sigreturn_reg = value;
      break;
    default:
      reg[num] = value;
    }
  }

  uint64_t getIP() const { return reg[REGNO_PPC32_SRR0]; }

  void setIP(uint64_t value) { reg[REGNO_PPC32_SRR0] = value; }

  uint64_t getSP() const { return reg[REGNO_PPC32_R1]; }

  void setSP(uint64_t value) { reg[REGNO_PPC32_R1] = value; }

  bool validFloatVectorRegister(int num) const {
    return (num >= REGNO_PPC32_F0 && num <= REGNO_PPC32_F31) ||
           (num >= REGNO_PPC32_V0 && num <= REGNO_PPC32_V31);
  }

  void copyFloatVectorRegister(int num, uint64_t addr_) {
    const void *addr = reinterpret_cast<const void *>(addr_);
    if (num >= REGNO_PPC32_F0 && num <= REGNO_PPC32_F31)
      memcpy(fpreg + (num - REGNO_PPC32_F0), addr, sizeof(fpreg[0]));
    else
      memcpy(vecreg + (num - REGNO_PPC32_V0), addr, sizeof(vecreg[0]));
  }

  __dso_hidden void jumpto() const __dead;

private:
  struct vecreg_t {
    uint64_t low, high;
  };
  uint32_t reg[REGNO_PPC32_SRR0 + 1];
  uint32_t dummy;
  uint64_t fpreg[32];
  vecreg_t vecreg[64];
  uint32_t ctr_reg;
  uint32_t xer_reg;
  uint32_t sigreturn_reg;
};

enum {
  DWARF_AARCH64_X0 = 0,
  DWARF_AARCH64_X30 = 30,
  DWARF_AARCH64_SP = 31,
  DWARF_AARCH64_V0 = 64,
  DWARF_AARCH64_V31 = 95,
  DWARF_AARCH64_SIGRETURN = 96,

  REGNO_AARCH64_X0 = 0,
  REGNO_AARCH64_X30 = 30,
  REGNO_AARCH64_SP = 31,
  REGNO_AARCH64_V0 = 32,
  REGNO_AARCH64_V31 = 63,
  REGNO_AARCH64_SIGRETURN = 64,
};

class Registers_aarch64 {
public:
  enum {
    LAST_RESTORE_REG = REGNO_AARCH64_SIGRETURN,
    LAST_REGISTER = REGNO_AARCH64_SIGRETURN,
    RETURN_OFFSET = 0,
    RETURN_MASK = 0,
  };

  __dso_hidden Registers_aarch64();

  static int dwarf2regno(int num) {
    if (num >= DWARF_AARCH64_X0 && num <= DWARF_AARCH64_X30)
      return REGNO_AARCH64_X0 + (num - DWARF_AARCH64_X0);
    if (num == DWARF_AARCH64_SP)
      return REGNO_AARCH64_SP;
    if (num >= DWARF_AARCH64_V0 && num <= DWARF_AARCH64_V31)
      return REGNO_AARCH64_V0 + (num - DWARF_AARCH64_V0);
    if (num == DWARF_AARCH64_SIGRETURN)
      return REGNO_AARCH64_SIGRETURN;
    return LAST_REGISTER + 1;
  }

  bool validRegister(int num) const {
    return (num >= DWARF_AARCH64_X0 && num <= DWARF_AARCH64_SP) ||
	num == DWARF_AARCH64_SIGRETURN;
  }

  uint64_t getRegister(int num) const {
    assert(validRegister(num));
    if (num == REGNO_AARCH64_SIGRETURN)
      return sigreturn_reg;
    return reg[num];
  }

  void setRegister(int num, uint64_t value) {
    assert(validRegister(num));
    if (num == REGNO_AARCH64_SIGRETURN)
      sigreturn_reg = value;
    else
      reg[num] = value;
  }

  uint64_t getIP() const { return reg[REGNO_AARCH64_X30]; }

  void setIP(uint64_t value) { reg[REGNO_AARCH64_X30] = value; }

  uint64_t getSP() const { return reg[REGNO_AARCH64_SP]; }

  void setSP(uint64_t value) { reg[REGNO_AARCH64_SP] = value; }

  bool validFloatVectorRegister(int num) const {
    return (num >= REGNO_AARCH64_V0 && num <= REGNO_AARCH64_V31);
  }

  void copyFloatVectorRegister(int num, uint64_t addr_) {
    const void *addr = reinterpret_cast<const void *>(addr_);
    memcpy(vecreg + (num - REGNO_AARCH64_V0), addr, 16);
  }

  __dso_hidden void jumpto() const __dead;

private:
  uint64_t reg[REGNO_AARCH64_SP + 1];
  uint64_t vecreg[64];
  uint64_t sigreturn_reg;
};

enum {
  DWARF_ARM32_R0 = 0,
  DWARF_ARM32_R15 = 15,
  DWARF_ARM32_SPSR = 128,
  DWARF_ARM32_S0 = 64,
  DWARF_ARM32_S31 = 95,
  DWARF_ARM32_D0 = 256,
  DWARF_ARM32_D31 = 287,
  REGNO_ARM32_R0 = 0,
  REGNO_ARM32_SP = 13,
  REGNO_ARM32_R15 = 15,
  REGNO_ARM32_SPSR = 16,
  REGNO_ARM32_D0 = 17,
  REGNO_ARM32_D15 = 32,
  REGNO_ARM32_D31 = 48,
  REGNO_ARM32_S0 = 49,
  REGNO_ARM32_S31 = 80,
};

#define	FLAGS_VFPV2_USED		0x1
#define	FLAGS_VFPV3_USED		0x2
#define	FLAGS_LEGACY_VFPV2_REGNO	0x4
#define	FLAGS_EXTENDED_VFPV2_REGNO	0x8

class Registers_arm32 {
public:
  enum {
    LAST_REGISTER = REGNO_ARM32_S31,
    LAST_RESTORE_REG = REGNO_ARM32_S31,
    RETURN_OFFSET = 0,
    RETURN_MASK = 0,
  };

  __dso_hidden Registers_arm32();

  static int dwarf2regno(int num) {
    if (num >= DWARF_ARM32_R0 && num <= DWARF_ARM32_R15)
      return REGNO_ARM32_R0 + (num - DWARF_ARM32_R0);
    if (num == DWARF_ARM32_SPSR)
      return REGNO_ARM32_SPSR;
    if (num >= DWARF_ARM32_D0 && num <= DWARF_ARM32_D31)
      return REGNO_ARM32_D0 + (num - DWARF_ARM32_D0);
    if (num >= DWARF_ARM32_S0 && num <= DWARF_ARM32_S31)
      return REGNO_ARM32_S0 + (num - DWARF_ARM32_S0);
    return LAST_REGISTER + 1;
  }

  bool validRegister(int num) const {
    return num >= 0 && num <= REGNO_ARM32_SPSR;
  }

  uint64_t getRegister(int num) const {
    assert(validRegister(num));
    return reg[num];
  }

  void setRegister(int num, uint64_t value) {
    assert(validRegister(num));
    reg[num] = value;
  }

  uint64_t getIP() const { return reg[REGNO_ARM32_R15]; }

  void setIP(uint64_t value) { reg[REGNO_ARM32_R15] = value; }

  uint64_t getSP() const { return reg[REGNO_ARM32_SP]; }

  void setSP(uint64_t value) { reg[REGNO_ARM32_SP] = value; }

  bool validFloatVectorRegister(int num) const {
    return (num >= REGNO_ARM32_D0 && num <= REGNO_ARM32_S31);
  }

  void copyFloatVectorRegister(int num, uint64_t addr_) {
    assert(validFloatVectorRegister(num));
    const void *addr = reinterpret_cast<const void *>(addr_);
    if (num >= REGNO_ARM32_S0 && num <= REGNO_ARM32_S31) {
      /*
       * XXX
       * There are two numbering schemes for VFPv2 registers: s0-s31
       * (used by GCC) and d0-d15 (used by LLVM). We won't support both
       * schemes simultaneously in a same frame.
       */
      assert((flags & FLAGS_EXTENDED_VFPV2_REGNO) == 0);
      flags |= FLAGS_LEGACY_VFPV2_REGNO;
      if ((flags & FLAGS_VFPV2_USED) == 0) {
        lazyVFPv2();
        flags |= FLAGS_VFPV2_USED;
      }
      /*
       * Emulate single precision register as half of the
       * corresponding double register.
       */
      int dnum = (num - REGNO_ARM32_S0) / 2;
      int part = (num - REGNO_ARM32_S0) % 2;
#if _BYTE_ORDER == _BIG_ENDIAN
      part = 1 - part;
#endif
      memcpy((uint8_t *)(fpreg + dnum) + part * sizeof(fpreg[0]) / 2,
        addr, sizeof(fpreg[0]) / 2);
    } else {
      if (num <= REGNO_ARM32_D15) {
	/*
	 * XXX
	 * See XXX comment above.
	 */
        assert((flags & FLAGS_LEGACY_VFPV2_REGNO) == 0);
        flags |= FLAGS_EXTENDED_VFPV2_REGNO;
        if ((flags & FLAGS_VFPV2_USED) == 0) {
          lazyVFPv2();
          flags |= FLAGS_VFPV2_USED;
        }
      } else {
        if ((flags & FLAGS_VFPV3_USED) == 0) {
          lazyVFPv3();
          flags |= FLAGS_VFPV3_USED;
        }
      }
      memcpy(fpreg + (num - REGNO_ARM32_D0), addr, sizeof(fpreg[0]));
    }
  }

  __dso_hidden void lazyVFPv2();
  __dso_hidden void lazyVFPv3();
  __dso_hidden void jumpto() const __dead;

private:
  uint32_t reg[REGNO_ARM32_SPSR + 1];
  uint32_t flags;
  uint64_t fpreg[32];
};

#undef	FLAGS_VFPV2_USED
#undef	FLAGS_VFPV3_USED
#undef	FLAGS_LEGACY_VFPV2_REGNO
#undef	FLAGS_EXTENDED_VFPV2_REGNO

enum {
  DWARF_VAX_R0 = 0,
  DWARF_VAX_R15 = 15,
  DWARF_VAX_PSW = 16,

  REGNO_VAX_R0 = 0,
  REGNO_VAX_R14 = 14,
  REGNO_VAX_R15 = 15,
  REGNO_VAX_PSW = 16,
};

class Registers_vax {
public:
  enum {
    LAST_REGISTER = REGNO_VAX_PSW,
    LAST_RESTORE_REG = REGNO_VAX_PSW,
    RETURN_OFFSET = 0,
    RETURN_MASK = 0,
  };

  __dso_hidden Registers_vax();

  static int dwarf2regno(int num) {
    if (num >= DWARF_VAX_R0 && num <= DWARF_VAX_R15)
      return REGNO_VAX_R0 + (num - DWARF_VAX_R0);
    if (num == DWARF_VAX_PSW)
      return REGNO_VAX_PSW;
    return LAST_REGISTER + 1;
  }

  bool validRegister(int num) const {
    return num >= 0 && num <= LAST_RESTORE_REG;
  }

  uint64_t getRegister(int num) const {
    assert(validRegister(num));
    return reg[num];
  }

  void setRegister(int num, uint64_t value) {
    assert(validRegister(num));
    reg[num] = value;
  }

  uint64_t getIP() const { return reg[REGNO_VAX_R15]; }

  void setIP(uint64_t value) { reg[REGNO_VAX_R15] = value; }

  uint64_t getSP() const { return reg[REGNO_VAX_R14]; }

  void setSP(uint64_t value) { reg[REGNO_VAX_R14] = value; }

  bool validFloatVectorRegister(int num) const {
    return false;
  }

  void copyFloatVectorRegister(int num, uint64_t addr_) {
  }

  __dso_hidden void jumpto() const __dead;

private:
  uint32_t reg[REGNO_VAX_PSW + 1];
};

enum {
  DWARF_M68K_A0 = 0,
  DWARF_M68K_A7 = 7,
  DWARF_M68K_D0 = 8,
  DWARF_M68K_D7 = 15,
  DWARF_M68K_FP0 = 16,
  DWARF_M68K_FP7 = 23,
  DWARF_M68K_PC = 24,
  // DWARF pseudo-register that is an alternate that may be used
  // for the return address.
  DWARF_M68K_ALT_PC = 25,

  REGNO_M68K_A0 = 0,
  REGNO_M68K_A7 = 7,
  REGNO_M68K_D0 = 8,
  REGNO_M68K_D7 = 15,
  REGNO_M68K_PC = 16,
  REGNO_M68K_FP0 = 17,
  REGNO_M68K_FP7 = 24,
};

class Registers_M68K {
public:
  enum {
    LAST_REGISTER = REGNO_M68K_FP7,
    LAST_RESTORE_REG = REGNO_M68K_FP7,
    RETURN_OFFSET = 0,
    RETURN_MASK = 0,
  };

  __dso_hidden Registers_M68K();

  static int dwarf2regno(int num) {
    if (num >= DWARF_M68K_A0 && num <= DWARF_M68K_A7)
      return REGNO_M68K_A0 + (num - DWARF_M68K_A0);
    if (num >= DWARF_M68K_D0 && num <= DWARF_M68K_D7)
      return REGNO_M68K_D0 + (num - DWARF_M68K_D0);
    if (num >= DWARF_M68K_FP0 && num <= DWARF_M68K_FP7)
      return REGNO_M68K_FP0 + (num - DWARF_M68K_FP0);
    if (num == DWARF_M68K_PC || num == DWARF_M68K_ALT_PC)
      return REGNO_M68K_PC;
    return LAST_REGISTER + 1;
  }

  bool validRegister(int num) const {
    return num >= 0 && num <= REGNO_M68K_PC;
  }

  uint64_t getRegister(int num) const {
    assert(validRegister(num));
    return reg[num];
  }

  void setRegister(int num, uint64_t value) {
    assert(validRegister(num));
    reg[num] = value;
  }

  uint64_t getIP() const { return reg[REGNO_M68K_PC]; }

  void setIP(uint64_t value) { reg[REGNO_M68K_PC] = value; }

  uint64_t getSP() const { return reg[REGNO_M68K_A7]; }

  void setSP(uint64_t value) { reg[REGNO_M68K_A7] = value; }

  bool validFloatVectorRegister(int num) const {
    return num >= REGNO_M68K_FP0 && num <= REGNO_M68K_FP7;
  }

  void copyFloatVectorRegister(int num, uint64_t addr_) {
    assert(validFloatVectorRegister(num));
    const void *addr = reinterpret_cast<const void *>(addr_);
    memcpy(fpreg + (num - REGNO_M68K_FP0), addr, sizeof(fpreg[0]));
  }

  __dso_hidden void jumpto() const __dead;

private:
  typedef uint32_t fpreg_t[3];

  uint32_t reg[REGNO_M68K_PC + 1];
  uint32_t dummy;
  fpreg_t fpreg[8];
};

enum {
  DWARF_SH3_R0 = 0,
  DWARF_SH3_R15 = 15,
  DWARF_SH3_PC = 16,
  DWARF_SH3_PR = 17,
  DWARF_SH3_GBR = 18,
  DWARF_SH3_MACH = 20,
  DWARF_SH3_MACL = 21,
  DWARF_SH3_SR = 22,

  REGNO_SH3_R0 = 0,
  REGNO_SH3_R15 = 15,
  REGNO_SH3_PC = 16,
  REGNO_SH3_PR = 17,
  REGNO_SH3_GBR = 18,
  REGNO_SH3_MACH = 20,
  REGNO_SH3_MACL = 21,
  REGNO_SH3_SR = 22,
};

class Registers_SH3 {
public:
  enum {
    LAST_REGISTER = REGNO_SH3_SR,
    LAST_RESTORE_REG = REGNO_SH3_SR,
    RETURN_OFFSET = 0,
    RETURN_MASK = 0,
  };

  __dso_hidden Registers_SH3();

  static int dwarf2regno(int num) {
    if (num >= DWARF_SH3_R0 && num <= DWARF_SH3_R15)
      return REGNO_SH3_R0 + (num - DWARF_SH3_R0);
    switch (num) {
    case DWARF_SH3_PC:
      return REGNO_SH3_PC;
    case DWARF_SH3_PR:
      return REGNO_SH3_PR;
    case DWARF_SH3_GBR:
      return REGNO_SH3_GBR;
    case DWARF_SH3_MACH:
      return REGNO_SH3_MACH;
    case DWARF_SH3_MACL:
      return REGNO_SH3_MACL;
    case DWARF_SH3_SR:
      return REGNO_SH3_SR;
    default:
      return LAST_REGISTER + 1;
    }
  }

  bool validRegister(int num) const {
    return (num >= 0 && num <= REGNO_SH3_GBR) ||
	(num >= REGNO_SH3_MACH && num <= REGNO_SH3_SR);
  }

  uint64_t getRegister(int num) const {
    assert(validRegister(num));
    return reg[num];
  }

  void setRegister(int num, uint64_t value) {
    assert(validRegister(num));
    reg[num] = value;
  }

  uint64_t getIP() const { return reg[REGNO_SH3_PC]; }

  void setIP(uint64_t value) { reg[REGNO_SH3_PC] = value; }

  uint64_t getSP() const { return reg[REGNO_SH3_R15]; }

  void setSP(uint64_t value) { reg[REGNO_SH3_R15] = value; }

  bool validFloatVectorRegister(int num) const { return false; }

  void copyFloatVectorRegister(int num, uint64_t addr_) {}

  __dso_hidden void jumpto() const __dead;

private:
  uint32_t reg[REGNO_SH3_SR + 1];
};

enum {
  DWARF_SPARC64_R0 = 0,
  DWARF_SPARC64_R31 = 31,
  DWARF_SPARC64_PC = 32,

  REGNO_SPARC64_R0 = 0,
  REGNO_SPARC64_R14 = 14,
  REGNO_SPARC64_R15 = 15,
  REGNO_SPARC64_R31 = 31,
  REGNO_SPARC64_PC = 32,
};

class Registers_SPARC64 {
public:
  enum {
    LAST_REGISTER = REGNO_SPARC64_PC,
    LAST_RESTORE_REG = REGNO_SPARC64_PC,
    RETURN_OFFSET = 8,
    RETURN_MASK = 0,
  };
  typedef uint64_t reg_t;

  __dso_hidden Registers_SPARC64();

  static int dwarf2regno(int num) {
    if (num >= DWARF_SPARC64_R0 && num <= DWARF_SPARC64_R31)
      return REGNO_SPARC64_R0 + (num - DWARF_SPARC64_R0);
    if (num == DWARF_SPARC64_PC)
      return REGNO_SPARC64_PC;
    return LAST_REGISTER + 1;
  }

  bool validRegister(int num) const {
    return num >= 0 && num <= REGNO_SPARC64_PC;
  }

  uint64_t getRegister(int num) const {
    assert(validRegister(num));
    return reg[num];
  }

  void setRegister(int num, uint64_t value) {
    assert(validRegister(num));
    reg[num] = value;
  }

  uint64_t getIP() const { return reg[REGNO_SPARC64_PC]; }

  void setIP(uint64_t value) { reg[REGNO_SPARC64_PC] = value; }

  uint64_t getSP() const { return reg[REGNO_SPARC64_R14]; }

  void setSP(uint64_t value) { reg[REGNO_SPARC64_R14] = value; }

  bool validFloatVectorRegister(int num) const { return false; }

  void copyFloatVectorRegister(int num, uint64_t addr_) {}

  __dso_hidden void jumpto() const __dead;

private:
  uint64_t reg[REGNO_SPARC64_PC + 1];
};

enum {
  DWARF_SPARC_R0 = 0,
  DWARF_SPARC_R31 = 31,
  DWARF_SPARC_PC = 32,

  REGNO_SPARC_R0 = 0,
  REGNO_SPARC_R14 = 14,
  REGNO_SPARC_R15 = 15,
  REGNO_SPARC_R31 = 31,
  REGNO_SPARC_PC = 32,
};

class Registers_SPARC {
public:
  enum {
    LAST_REGISTER = REGNO_SPARC_PC,
    LAST_RESTORE_REG = REGNO_SPARC_PC,
    RETURN_OFFSET = 8,
    RETURN_MASK = 0,
  };
  typedef uint32_t reg_t;

  __dso_hidden Registers_SPARC();

  static int dwarf2regno(int num) {
    if (num >= DWARF_SPARC_R0 && num <= DWARF_SPARC_R31)
      return REGNO_SPARC_R0 + (num - DWARF_SPARC_R0);
    if (num == DWARF_SPARC_PC)
      return REGNO_SPARC_PC;
    return LAST_REGISTER + 1;
  }

  bool validRegister(int num) const {
    return num >= 0 && num <= REGNO_SPARC_PC;
  }

  uint64_t getRegister(int num) const {
    assert(validRegister(num));
    return reg[num];
  }

  void setRegister(int num, uint64_t value) {
    assert(validRegister(num));
    reg[num] = value;
  }

  uint64_t getIP() const { return reg[REGNO_SPARC_PC]; }

  void setIP(uint64_t value) { reg[REGNO_SPARC_PC] = value; }

  uint64_t getSP() const { return reg[REGNO_SPARC_R14]; }

  void setSP(uint64_t value) { reg[REGNO_SPARC_R14] = value; }

  bool validFloatVectorRegister(int num) const { return false; }

  void copyFloatVectorRegister(int num, uint64_t addr_) {}

  __dso_hidden void jumpto() const __dead;

private:
  uint32_t reg[REGNO_SPARC_PC + 1];
};

enum {
  DWARF_ALPHA_R0 = 0,
  DWARF_ALPHA_R30 = 30,
  DWARF_ALPHA_F0 = 32,
  DWARF_ALPHA_F30 = 62,
  DWARF_ALPHA_SIGRETURN = 64,

  REGNO_ALPHA_R0 = 0,
  REGNO_ALPHA_R26 = 26,
  REGNO_ALPHA_R30 = 30,
  REGNO_ALPHA_PC = 31,
  REGNO_ALPHA_F0 = 32,
  REGNO_ALPHA_F30 = 62,
  REGNO_ALPHA_SIGRETURN = 64,
};

class Registers_Alpha {
public:
  enum {
    LAST_REGISTER = REGNO_ALPHA_SIGRETURN,
    LAST_RESTORE_REG = REGNO_ALPHA_SIGRETURN,
    RETURN_OFFSET = 0,
    RETURN_MASK = 0,
  };
  typedef uint32_t reg_t;

  __dso_hidden Registers_Alpha();

  static int dwarf2regno(int num) { return num; }

  bool validRegister(int num) const {
    return (num >= 0 && num <= REGNO_ALPHA_PC) ||
	num == REGNO_ALPHA_SIGRETURN;
  }

  uint64_t getRegister(int num) const {
    assert(validRegister(num));
    if (num == REGNO_ALPHA_SIGRETURN)
      return sigreturn_reg;
    else
      return reg[num];
  }

  void setRegister(int num, uint64_t value) {
    assert(validRegister(num));
    if (num == REGNO_ALPHA_SIGRETURN)
      sigreturn_reg = value;
    else
      reg[num] = value;
  }

  uint64_t getIP() const { return reg[REGNO_ALPHA_PC]; }

  void setIP(uint64_t value) { reg[REGNO_ALPHA_PC] = value; }

  uint64_t getSP() const { return reg[REGNO_ALPHA_R30]; }

  void setSP(uint64_t value) { reg[REGNO_ALPHA_R30] = value; }

  bool validFloatVectorRegister(int num) const {
    return num >= REGNO_ALPHA_F0 && num <= REGNO_ALPHA_F30;
  }

  void copyFloatVectorRegister(int num, uint64_t addr_) {
    assert(validFloatVectorRegister(num));
    const void *addr = reinterpret_cast<const void *>(addr_);
    memcpy(fpreg + (num - REGNO_ALPHA_F0), addr, sizeof(fpreg[0]));
  }

  __dso_hidden void jumpto() const __dead;

private:
  uint64_t reg[REGNO_ALPHA_PC + 1];
  uint64_t fpreg[31];
  uint64_t sigreturn_reg;
};

enum {
  DWARF_HPPA_R1 = 1,
  DWARF_HPPA_R31 = 31,
  DWARF_HPPA_FR4L = 32,
  DWARF_HPPA_FR31H = 87,
  DWARF_HPPA_SIGRETURN = 89,

  REGNO_HPPA_PC = 0,
  REGNO_HPPA_R1 = 1,
  REGNO_HPPA_R2 = 2,
  REGNO_HPPA_R30 = 30,
  REGNO_HPPA_R31 = 31,
  REGNO_HPPA_FR4L = 32,
  REGNO_HPPA_FR31H = 87,
  REGNO_HPPA_SIGRETURN = 89,
};

class Registers_HPPA {
public:
  enum {
    LAST_REGISTER = REGNO_HPPA_FR31H,
    LAST_RESTORE_REG = REGNO_HPPA_SIGRETURN,
    RETURN_OFFSET = 0,
    RETURN_MASK = 3,
  };

  __dso_hidden Registers_HPPA();

  static int dwarf2regno(int num) {
    if (num >= DWARF_HPPA_R1 && num <= DWARF_HPPA_R31)
      return REGNO_HPPA_R1 + (num - DWARF_HPPA_R1);
    if (num >= DWARF_HPPA_FR4L && num <= DWARF_HPPA_FR31H)
      return REGNO_HPPA_FR4L + (num - DWARF_HPPA_FR31H);
    if (num == DWARF_HPPA_SIGRETURN)
      return REGNO_HPPA_SIGRETURN;
    return LAST_REGISTER + 1;
  }

  bool validRegister(int num) const {
    return num >= REGNO_HPPA_PC && num <= REGNO_HPPA_R31) ||
       num == REGNO_HPPA_SIGRETURN;
  }

  uint64_t getRegister(int num) const {
    assert(validRegister(num));
    if (num == REGNO_HPPA_SIGRETURN)
      return sigreturn_reg;
    else
      return reg[num];
  }

  void setRegister(int num, uint64_t value) {
    assert(validRegister(num));
    if (num == REGNO_HPPA_SIGRETURN)
      sigreturn_reg = value;
    else
      reg[num] = value;
  }

  uint64_t getIP() const { return reg[REGNO_HPPA_PC]; }

  void setIP(uint64_t value) { reg[REGNO_HPPA_PC] = value; }

  uint64_t getSP() const { return reg[REGNO_HPPA_R30]; }

  void setSP(uint64_t value) { reg[REGNO_HPPA_R30] = value; }

  bool validFloatVectorRegister(int num) const {
    return num >= REGNO_HPPA_FR4L && num <= REGNO_HPPA_FR31H;
  }

  void copyFloatVectorRegister(int num, uint64_t addr_) {
    assert(validFloatVectorRegister(num));
    const void *addr = reinterpret_cast<const void *>(addr_);
    memcpy(fpreg + (num - REGNO_HPPA_FR4L), addr, sizeof(fpreg[0]));
  }

  __dso_hidden void jumpto() const __dead;

private:
  uint32_t reg[REGNO_HPPA_R31 + 1];
  uint32_t fpreg[56];
  uint32_t sigreturn_reg;
};

enum {
  DWARF_MIPS_R1 = 0,
  DWARF_MIPS_R31 = 31,
  DWARF_MIPS_F0 = 32,
  DWARF_MIPS_F31 = 63,
  // DWARF Pseudo-registers used by GCC on MIPS for MD{HI,LO} and
  // signal handler return address.
  DWARF_MIPS_MDHI = 64,
  DWARF_MIPS_MDLO = 65,
  DWARF_MIPS_SIGRETURN = 66,

  REGNO_MIPS_PC = 0,
  REGNO_MIPS_R1 = 0,
  REGNO_MIPS_R29 = 29,
  REGNO_MIPS_R31 = 31,
  REGNO_MIPS_F0 = 33,
  REGNO_MIPS_F31 = 64,
  // these live in other_reg[]
  REGNO_MIPS_MDHI = 65,
  REGNO_MIPS_MDLO = 66,
  REGNO_MIPS_SIGRETURN = 67
};

class Registers_MIPS {
public:
  enum {
    LAST_REGISTER = REGNO_MIPS_SIGRETURN,
    LAST_RESTORE_REG = REGNO_MIPS_SIGRETURN,
    RETURN_OFFSET = 0,
    RETURN_MASK = 0,
  };

  __dso_hidden Registers_MIPS();

  static int dwarf2regno(int num) {
    if (num >= DWARF_MIPS_R1 && num <= DWARF_MIPS_R31)
      return REGNO_MIPS_R1 + (num - DWARF_MIPS_R1);
    if (num >= DWARF_MIPS_F0 && num <= DWARF_MIPS_F31)
      return REGNO_MIPS_F0 + (num - DWARF_MIPS_F0);
    if (num >= DWARF_MIPS_MDHI && num <= DWARF_MIPS_SIGRETURN)
      return REGNO_MIPS_MDHI + (num - DWARF_MIPS_MDHI);
    return LAST_REGISTER + 1;
  }

  bool validRegister(int num) const {
    return (num >= REGNO_MIPS_PC && num <= REGNO_MIPS_R31) ||
      (num >= REGNO_MIPS_MDHI && num <= REGNO_MIPS_SIGRETURN);
  }

  uint64_t getRegister(int num) const {
    assert(validRegister(num));
    if (num >= REGNO_MIPS_MDHI && num <= REGNO_MIPS_SIGRETURN)
      return other_reg[num - REGNO_MIPS_MDHI];
    return reg[num];
  }

  void setRegister(int num, uint64_t value) {
    assert(validRegister(num));
    if (num >= REGNO_MIPS_MDHI && num <= REGNO_MIPS_SIGRETURN)
      other_reg[num - REGNO_MIPS_MDHI] = value;
    else
      reg[num] = value;
  }

  uint64_t getIP() const { return reg[REGNO_MIPS_PC]; }

  void setIP(uint64_t value) { reg[REGNO_MIPS_PC] = value; }

  uint64_t getSP() const { return reg[REGNO_MIPS_R29]; }

  void setSP(uint64_t value) { reg[REGNO_MIPS_R29] = value; }

  bool validFloatVectorRegister(int num) const {
    return num >= REGNO_MIPS_F0 && num <= REGNO_MIPS_F31;
  }

  void copyFloatVectorRegister(int num, uint64_t addr_) {
    assert(validFloatVectorRegister(num));
    const void *addr = reinterpret_cast<const void *>(addr_);
    memcpy(fpreg + (num - REGNO_MIPS_F0), addr, sizeof(fpreg[0]));
  }

  __dso_hidden void jumpto() const __dead;

private:
  uint32_t reg[REGNO_MIPS_R31 + 1];
  uint64_t fpreg[32];
  uint32_t other_reg[3];
};

enum {
  DWARF_MIPS64_R1 = 0,
  DWARF_MIPS64_R31 = 31,
  DWARF_MIPS64_F0 = 32,
  DWARF_MIPS64_F31 = 63,
  // DWARF Pseudo-registers used by GCC on MIPS for MD{HI,LO} and
  // signal handler return address.
  DWARF_MIPS64_MDHI = 64,
  DWARF_MIPS64_MDLO = 65,
  DWARF_MIPS64_SIGRETURN = 66,

  REGNO_MIPS64_PC = 0,
  REGNO_MIPS64_R1 = 0,
  REGNO_MIPS64_R29 = 29,
  REGNO_MIPS64_R31 = 31,
  REGNO_MIPS64_F0 = 33,
  REGNO_MIPS64_F31 = 64,
  // these live in other_reg[]
  REGNO_MIPS64_MDHI = 65,
  REGNO_MIPS64_MDLO = 66,
  REGNO_MIPS64_SIGRETURN = 67
};

class Registers_MIPS64 {
public:
  enum {
    LAST_REGISTER = REGNO_MIPS_SIGRETURN,
    LAST_RESTORE_REG = REGNO_MIPS_SIGRETURN,
    RETURN_OFFSET = 0,
    RETURN_MASK = 0,
  };

  __dso_hidden Registers_MIPS64();

  static int dwarf2regno(int num) {
    if (num >= DWARF_MIPS64_R1 && num <= DWARF_MIPS64_R31)
      return REGNO_MIPS64_R1 + (num - DWARF_MIPS64_R1);
    if (num >= DWARF_MIPS64_F0 && num <= DWARF_MIPS64_F31)
      return REGNO_MIPS64_F0 + (num - DWARF_MIPS64_F0);
    if (num >= DWARF_MIPS64_MDHI && num <= DWARF_MIPS64_SIGRETURN)
      return REGNO_MIPS64_MDHI + (num - DWARF_MIPS64_MDHI);
    return LAST_REGISTER + 1;
  }

  bool validRegister(int num) const {
    return (num >= REGNO_MIPS64_PC && num <= REGNO_MIPS64_R31) ||
        (num >= REGNO_MIPS64_MDHI && num <= REGNO_MIPS64_SIGRETURN);
  }

  uint64_t getRegister(int num) const {
    assert(validRegister(num));
    if (num >= REGNO_MIPS64_MDHI && num <= REGNO_MIPS64_SIGRETURN)
      return other_reg[num - REGNO_MIPS64_MDHI];
    return reg[num];
  }

  void setRegister(int num, uint64_t value) {
    assert(validRegister(num));
    if (num >= REGNO_MIPS64_MDHI && num <= REGNO_MIPS64_SIGRETURN)
      other_reg[num - REGNO_MIPS64_MDHI] = value;
    else
      reg[num] = value;
  }

  uint64_t getIP() const { return reg[REGNO_MIPS64_PC]; }

  void setIP(uint64_t value) { reg[REGNO_MIPS64_PC] = value; }

  uint64_t getSP() const { return reg[REGNO_MIPS64_R29]; }

  void setSP(uint64_t value) { reg[REGNO_MIPS64_R29] = value; }

  bool validFloatVectorRegister(int num) const {
    return num >= REGNO_MIPS64_F0 && num <= REGNO_MIPS64_F31;
  }

  void copyFloatVectorRegister(int num, uint64_t addr_) {
    assert(validFloatVectorRegister(num));
    const void *addr = reinterpret_cast<const void *>(addr_);
    memcpy(fpreg + (num - REGNO_MIPS64_F0), addr, sizeof(fpreg[0]));
  }

  __dso_hidden void jumpto() const __dead;

private:
  uint64_t reg[REGNO_MIPS64_R31 + 1];
  uint64_t fpreg[32];
  uint64_t other_reg[3];
};

enum {
  DWARF_OR1K_R0 = 0,
  DWARF_OR1K_SP = 1,
  DWARF_OR1K_LR = 9,
  DWARF_OR1K_R31 = 31,
  DWARF_OR1K_FPCSR = 32,

  REGNO_OR1K_R0 = 0,
  REGNO_OR1K_SP = 1,
  REGNO_OR1K_LR = 9,
  REGNO_OR1K_R31 = 31,
  REGNO_OR1K_FPCSR = 32,
};

class Registers_or1k {
public:
  enum {
    LAST_REGISTER = REGNO_OR1K_FPCSR,
    LAST_RESTORE_REG = REGNO_OR1K_FPCSR,
    RETURN_OFFSET = 0,
    RETURN_MASK = 0,
  };

  __dso_hidden Registers_or1k();

  static int dwarf2regno(int num) {
    if (num >= DWARF_OR1K_R0 && num <= DWARF_OR1K_R31)
      return REGNO_OR1K_R0 + (num - DWARF_OR1K_R0);
    if (num == DWARF_OR1K_FPCSR)
      return REGNO_OR1K_FPCSR;
    return LAST_REGISTER + 1;
  }

  bool validRegister(int num) const {
    return num >= 0 && num <= LAST_RESTORE_REG;
  }

  uint64_t getRegister(int num) const {
    assert(validRegister(num));
    return reg[num];
  }

  void setRegister(int num, uint64_t value) {
    assert(validRegister(num));
    reg[num] = value;
  }

  uint64_t getIP() const { return reg[REGNO_OR1K_LR]; }

  void setIP(uint64_t value) { reg[REGNO_OR1K_LR] = value; }

  uint64_t getSP() const { return reg[REGNO_OR1K_SP]; }

  void setSP(uint64_t value) { reg[REGNO_OR1K_SP] = value; }

  bool validFloatVectorRegister(int num) const {
    return false;
  }

  void copyFloatVectorRegister(int num, uint64_t addr_) {
  }

  __dso_hidden void jumpto() const __dead;

private:
  uint32_t reg[REGNO_OR1K_FPCSR + 1];
};

#if __i386__
typedef Registers_x86 NativeUnwindRegisters;
#elif __x86_64__
typedef Registers_x86_64 NativeUnwindRegisters;
#elif __powerpc__
typedef Registers_ppc32 NativeUnwindRegisters;
#elif __aarch64__
typedef Registers_aarch64 NativeUnwindRegisters;
#elif __arm__
typedef Registers_arm32 NativeUnwindRegisters;
#elif __vax__
typedef Registers_vax NativeUnwindRegisters;
#elif __m68k__
typedef Registers_M68K NativeUnwindRegisters;
#elif __mips_n64 || __mips_n32
typedef Registers_MIPS64 NativeUnwindRegisters;
#elif __mips__
typedef Registers_MIPS NativeUnwindRegisters;
#elif __sh3__
typedef Registers_SH3 NativeUnwindRegisters;
#elif __sparc64__
typedef Registers_SPARC64 NativeUnwindRegisters;
#elif __sparc__
typedef Registers_SPARC NativeUnwindRegisters;
#elif __alpha__
typedef Registers_Alpha NativeUnwindRegisters;
#elif __hppa__
typedef Registers_HPPA NativeUnwindRegisters;
#elif __or1k__
typedef Registers_or1k NativeUnwindRegisters;
#endif
} // namespace _Unwind

#endif // __REGISTERS_HPP__
