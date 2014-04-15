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
    RETURN_REG = REGNO_X86_EIP,
    RETURN_OFFSET = 0,
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
    RETURN_REG = REGNO_X86_64_RIP,
    RETURN_OFFSET = 0,
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
  DWARF_PPC32_CR = 70,
  DWARF_PPC32_V0 = 77,
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
};

class Registers_ppc32 {
public:
  enum {
    LAST_REGISTER = REGNO_PPC32_V31,
    LAST_RESTORE_REG = REGNO_PPC32_V31,
    RETURN_REG = REGNO_PPC32_LR,
    RETURN_OFFSET = 0,
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
    default:
      return LAST_REGISTER + 1;
    }
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
};

enum {
  DWARF_ARM32_R0 = 0,
  DWARF_ARM32_R15 = 15,
  DWARF_ARM32_SPSR = 128,
  DWARF_ARM32_D0 = 256,		// VFP-v3/Neon
  DWARF_ARM32_D31 = 287,
  REGNO_ARM32_R0 = 0,
  REGNO_ARM32_SP = 13,
  REGNO_ARM32_R15 = 15,
  REGNO_ARM32_SPSR = 16,
  REGNO_ARM32_D0 = 0,
  REGNO_ARM32_D31 = 31,
};

class Registers_arm32 {
public:
  enum {
    LAST_REGISTER = REGNO_ARM32_D31,
    LAST_RESTORE_REG = REGNO_ARM32_SPSR,
    RETURN_REG = REGNO_ARM32_SPSR,
    RETURN_OFFSET = 0,
  };

  __dso_hidden Registers_arm32();

  static int dwarf2regno(int num) {
    if (num >= DWARF_ARM32_R0 && num <= DWARF_ARM32_R15)
      return REGNO_ARM32_R0 + (num - DWARF_ARM32_R0);
    if (num >= DWARF_ARM32_D0 && num <= DWARF_ARM32_D31)
      return REGNO_ARM32_D0 + (num - DWARF_ARM32_D0);
    if (num == DWARF_ARM32_SPSR)
      return REGNO_ARM32_SPSR;
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

  uint64_t getIP() const { return reg[REGNO_ARM32_R15]; }

  void setIP(uint64_t value) { reg[REGNO_ARM32_R15] = value; }

  uint64_t getSP() const { return reg[REGNO_ARM32_SP]; }

  void setSP(uint64_t value) { reg[REGNO_ARM32_SP] = value; }

  bool validFloatVectorRegister(int num) const {
    return (num >= REGNO_ARM32_D0 && num <= REGNO_ARM32_D31);
  }

  void copyFloatVectorRegister(int num, uint64_t addr_) {
    const void *addr = reinterpret_cast<const void *>(addr_);
    memcpy(fpreg + (num - REGNO_ARM32_D0), addr, sizeof(fpreg[0]));
  }

  __dso_hidden void jumpto() const __dead;

private:
  uint32_t reg[REGNO_ARM32_SPSR + 1];
  uint64_t fpreg[32];
};

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
    RETURN_REG = REGNO_VAX_R15,
    RETURN_OFFSET = 0,
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
    RETURN_REG = REGNO_M68K_PC,
    RETURN_OFFSET = 0,
  };

  __dso_hidden Registers_M68K();

  static int dwarf2regno(int num) {
    if (num >= DWARF_M68K_A0 && num <= DWARF_M68K_A7)
      return REGNO_M68K_A0 + (num - DWARF_M68K_A0);
    if (num >= DWARF_M68K_D0 && num <= DWARF_M68K_D7)
      return REGNO_M68K_D0 + (num - DWARF_M68K_D0);
    if (num >= DWARF_M68K_FP0 && num <= DWARF_M68K_FP7)
      return REGNO_M68K_FP0 + (num - DWARF_M68K_FP0);
    if (num == DWARF_M68K_PC)
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

  REGNO_SH3_R0 = 0,
  REGNO_SH3_R15 = 15,
  REGNO_SH3_PC = 16,
  REGNO_SH3_PR = 17,
};

class Registers_SH3 {
public:
  enum {
    LAST_REGISTER = REGNO_SH3_PR,
    LAST_RESTORE_REG = REGNO_SH3_PR,
    RETURN_REG = REGNO_SH3_PR,
    RETURN_OFFSET = 0,
  };

  __dso_hidden Registers_SH3();

  static int dwarf2regno(int num) {
    if (num >= DWARF_SH3_R0 && num <= DWARF_SH3_R15)
      return REGNO_SH3_R0 + (num - DWARF_SH3_R0);
    if (num == DWARF_SH3_PC)
      return REGNO_SH3_PC;
    if (num == DWARF_SH3_PR)
      return REGNO_SH3_PR;
    return LAST_REGISTER + 1;
  }

  bool validRegister(int num) const {
    return num >= 0 && num <= REGNO_SH3_PR;
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
  uint32_t reg[REGNO_SH3_PR + 1];
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
    RETURN_REG = REGNO_SPARC64_R15,
    RETURN_OFFSET = 8,
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
    RETURN_REG = REGNO_SPARC_R15,
    RETURN_OFFSET = 8,
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

#if __i386__
typedef Registers_x86 NativeUnwindRegisters;
#elif __x86_64__
typedef Registers_x86_64 NativeUnwindRegisters;
#elif __powerpc__
typedef Registers_ppc32 NativeUnwindRegisters;
#elif __arm__ && !defined(__ARM_EABI__)
typedef Registers_arm32 NativeUnwindRegisters;
#elif __vax__
typedef Registers_vax NativeUnwindRegisters;
#elif __m68k__
typedef Registers_M68K NativeUnwindRegisters;
#elif __sh3__
typedef Registers_SH3 NativeUnwindRegisters;
#elif __sparc64__
typedef Registers_SPARC64 NativeUnwindRegisters;
#elif __sparc__
typedef Registers_SPARC NativeUnwindRegisters;
#endif
} // namespace _Unwind

#endif // __REGISTERS_HPP__
