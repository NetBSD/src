#include "config.h"

#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "mn10300_sim.h"
#include "simops.h"
#include "sim-types.h"
#include "targ-vals.h"
#include "bfd.h"
#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/time.h>

#define REG0(X) ((X) & 0x3)
#define REG1(X) (((X) & 0xc) >> 2)
#define REG0_4(X) (((X) & 0x30) >> 4)
#define REG0_8(X) (((X) & 0x300) >> 8)
#define REG1_8(X) (((X) & 0xc00) >> 10)
#define REG0_16(X) (((X) & 0x30000) >> 16)
#define REG1_16(X) (((X) & 0xc0000) >> 18)

/* mov imm8, dn */
void OP_8000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_8 (insn)] = SEXT8 (insn & 0xff);
}

/* mov dm, dn */
void OP_80 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)] = State.regs[REG_D0 + REG1 (insn)];
}

/* mov dm, an */
void OP_F1E0 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0 (insn)] = State.regs[REG_D0 + REG1 (insn)];
}

/* mov am, dn */
void OP_F1D0 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)] = State.regs[REG_A0 + REG1 (insn)];
}

/* mov imm8, an */
void OP_9000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0_8 (insn)] = insn & 0xff;
}

/* mov am, an */
void OP_90 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0 (insn)] = State.regs[REG_A0 + REG1 (insn)];
}

/* mov sp, an */
void OP_3C (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0 (insn)] = State.regs[REG_SP];
}

/* mov am, sp */
void OP_F2F0 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_SP] = State.regs[REG_A0 + REG1 (insn)];
}

/* mov psw, dn */
void OP_F2E4 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)] = PSW;
}

/* mov dm, psw */
void OP_F2F3 (insn, extension)
     unsigned long insn, extension;
{
  PSW = State.regs[REG_D0 + REG1 (insn)];
}

/* mov mdr, dn */
void OP_F2E0 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)] = State.regs[REG_MDR];
}

/* mov dm, mdr */
void OP_F2F2 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_MDR] = State.regs[REG_D0 + REG1 (insn)];
}

/* mov (am), dn */
void OP_70 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG1 (insn)]
    = load_word (State.regs[REG_A0 + REG0 (insn)]);
}

/* mov (d8,am), dn */
void OP_F80000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG1_8 (insn)]
    = load_word ((State.regs[REG_A0 + REG0_8 (insn)] + SEXT8 (insn & 0xff)));
}

/* mov (d16,am), dn */
void OP_FA000000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG1_16 (insn)]
    = load_word ((State.regs[REG_A0 + REG0_16 (insn)]
		  + SEXT16 (insn & 0xffff)));
}

/* mov (d32,am), dn */
void OP_FC000000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG1_16 (insn)]
    = load_word ((State.regs[REG_A0 + REG0_16 (insn)]
		  + ((insn & 0xffff) << 16) + extension));
}

/* mov (d8,sp), dn */
void OP_5800 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_8 (insn)]
    = load_word (State.regs[REG_SP] + (insn & 0xff));
}

/* mov (d16,sp), dn */
void OP_FAB40000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = load_word (State.regs[REG_SP] + (insn & 0xffff));
}

/* mov (d32,sp), dn */
void OP_FCB40000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = load_word (State.regs[REG_SP] + (((insn & 0xffff) << 16) + extension));
}

/* mov (di,am), dn */
void OP_F300 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_4 (insn)]
    = load_word ((State.regs[REG_A0 + REG0 (insn)]
		  + State.regs[REG_D0 + REG1 (insn)]));
}

/* mov (abs16), dn */
void OP_300000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)] = load_word ((insn & 0xffff));
}

/* mov (abs32), dn */
void OP_FCA40000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = load_word ((((insn & 0xffff) << 16) + extension));
}

/* mov (am), an */
void OP_F000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG1 (insn)]
    = load_word (State.regs[REG_A0 + REG0 (insn)]);
}

/* mov (d8,am), an */
void OP_F82000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG1_8 (insn)]
    = load_word ((State.regs[REG_A0 + REG0_8 (insn)]
		  + SEXT8 (insn & 0xff)));
}

/* mov (d16,am), an */
void OP_FA200000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG1_16 (insn)]
    = load_word ((State.regs[REG_A0 + REG0_16 (insn)]
		  + SEXT16 (insn & 0xffff)));
}

/* mov (d32,am), an */
void OP_FC200000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG1_16 (insn)]
    = load_word ((State.regs[REG_A0 + REG0_16 (insn)]
		  + ((insn & 0xffff) << 16) + extension));
}

/* mov (d8,sp), an */
void OP_5C00 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0_8 (insn)]
    = load_word (State.regs[REG_SP] + (insn & 0xff));
}

/* mov (d16,sp), an */
void OP_FAB00000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0_16 (insn)]
    = load_word (State.regs[REG_SP] + (insn & 0xffff));
}

/* mov (d32,sp), an */
void OP_FCB00000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0_16 (insn)]
    = load_word (State.regs[REG_SP] + (((insn & 0xffff) << 16) + extension));
}

/* mov (di,am), an */
void OP_F380 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0_4 (insn)]
    = load_word ((State.regs[REG_A0 + REG0 (insn)]
		 + State.regs[REG_D0 + REG1 (insn)]));
}

/* mov (abs16), an */
void OP_FAA00000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0_16 (insn)] = load_word ((insn & 0xffff));
}

/* mov (abs32), an */
void OP_FCA00000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0_16 (insn)]
    = load_word ((((insn & 0xffff) << 16) + extension));
}

/* mov (d8,am), sp */
void OP_F8F000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_SP]
    = load_word ((State.regs[REG_A0 + REG0_8 (insn)]
		  + SEXT8 (insn & 0xff)));
}

/* mov dm, (an) */
void OP_60 (insn, extension)
     unsigned long insn, extension;
{
  store_word (State.regs[REG_A0 + REG0 (insn)],
	      State.regs[REG_D0 + REG1 (insn)]);
}

/* mov dm, (d8,an) */
void OP_F81000 (insn, extension)
     unsigned long insn, extension;
{
  store_word ((State.regs[REG_A0 + REG0_8 (insn)] + SEXT8 (insn & 0xff)),
	      State.regs[REG_D0 + REG1_8 (insn)]);
}

/* mov dm (d16,an) */
void OP_FA100000 (insn, extension)
     unsigned long insn, extension;
{
  store_word ((State.regs[REG_A0 + REG0_16 (insn)] + SEXT16 (insn & 0xffff)),
	      State.regs[REG_D0 + REG1_16 (insn)]);
}

/* mov dm (d32,an) */
void OP_FC100000 (insn, extension)
     unsigned long insn, extension;
{
  store_word ((State.regs[REG_A0 + REG0_16 (insn)]
	       + ((insn & 0xffff) << 16) + extension),
	      State.regs[REG_D0 + REG1_16 (insn)]);
}

/* mov dm, (d8,sp) */
void OP_4200 (insn, extension)
     unsigned long insn, extension;
{
  store_word (State.regs[REG_SP] + (insn & 0xff),
	      State.regs[REG_D0 + REG1_8 (insn)]);
}

/* mov dm, (d16,sp) */
void OP_FA910000 (insn, extension)
     unsigned long insn, extension;
{
  store_word (State.regs[REG_SP] + (insn & 0xffff),
	      State.regs[REG_D0 + REG1_16 (insn)]);
}

/* mov dm, (d32,sp) */
void OP_FC910000 (insn, extension)
     unsigned long insn, extension;
{
  store_word (State.regs[REG_SP] + (((insn & 0xffff) << 16) + extension),
	      State.regs[REG_D0 + REG1_16 (insn)]);
}

/* mov dm, (di,an) */
void OP_F340 (insn, extension)
     unsigned long insn, extension;
{
  store_word ((State.regs[REG_A0 + REG0 (insn)]
	       + State.regs[REG_D0 + REG1 (insn)]),
	      State.regs[REG_D0 + REG0_4 (insn)]);
}

/* mov dm, (abs16) */
void OP_10000 (insn, extension)
     unsigned long insn, extension;
{
  store_word ((insn & 0xffff), State.regs[REG_D0 + REG1_16 (insn)]);
}

/* mov dm, (abs32) */
void OP_FC810000 (insn, extension)
     unsigned long insn, extension;
{
  store_word ((((insn & 0xffff) << 16) + extension), 
	      State.regs[REG_D0 + REG1_16 (insn)]);
}

/* mov am, (an) */
void OP_F010 (insn, extension)
     unsigned long insn, extension;
{
  store_word (State.regs[REG_A0 + REG0 (insn)],
	      State.regs[REG_A0 + REG1 (insn)]);
}

/* mov am, (d8,an) */
void OP_F83000 (insn, extension)
     unsigned long insn, extension;
{
  store_word ((State.regs[REG_A0 + REG0_8 (insn)] + SEXT8 (insn & 0xff)),
	      State.regs[REG_A0 + REG1_8 (insn)]);
}

/* mov am, (d16,an) */
void OP_FA300000 (insn, extension)
     unsigned long insn, extension;
{
  store_word ((State.regs[REG_A0 + REG0_16 (insn)] + SEXT16 (insn & 0xffff)),
	      State.regs[REG_A0 + REG1_16 (insn)]);
}

/* mov am, (d32,an) */
void OP_FC300000 (insn, extension)
     unsigned long insn, extension;
{
  store_word ((State.regs[REG_A0 + REG0_16 (insn)]
	       + ((insn & 0xffff) << 16) + extension),
	      State.regs[REG_A0 + REG1_16 (insn)]);
}

/* mov am, (d8,sp) */
void OP_4300 (insn, extension)
     unsigned long insn, extension;
{
  store_word (State.regs[REG_SP] + (insn & 0xff),
	      State.regs[REG_A0 + REG1_8 (insn)]);
}

/* mov am, (d16,sp) */
void OP_FA900000 (insn, extension)
     unsigned long insn, extension;
{
  store_word (State.regs[REG_SP] + (insn & 0xffff),
	      State.regs[REG_A0 + REG1_16 (insn)]);
}

/* mov am, (d32,sp) */
void OP_FC900000 (insn, extension)
     unsigned long insn, extension;
{
  store_word (State.regs[REG_SP] + (((insn & 0xffff) << 16) + extension),
	      State.regs[REG_A0 + REG1_16 (insn)]);
}

/* mov am, (di,an) */
void OP_F3C0 (insn, extension)
     unsigned long insn, extension;
{
  store_word ((State.regs[REG_A0 + REG0 (insn)]
	       + State.regs[REG_D0 + REG1 (insn)]),
	      State.regs[REG_A0 + REG0_4 (insn)]);
}

/* mov am, (abs16) */
void OP_FA800000 (insn, extension)
     unsigned long insn, extension;
{
  store_word ((insn & 0xffff), State.regs[REG_A0 + REG1_16 (insn)]);
}

/* mov am, (abs32) */
void OP_FC800000 (insn, extension)
     unsigned long insn, extension;
{
  store_word ((((insn & 0xffff) << 16) + extension), State.regs[REG_A0 + REG1_16 (insn)]);
}

/* mov sp, (d8,an) */
void OP_F8F400 (insn, extension)
     unsigned long insn, extension;
{
  store_word (State.regs[REG_A0 + REG0_8 (insn)] + SEXT8 (insn & 0xff),
	      State.regs[REG_SP]);
}

/* mov imm16, dn */
void OP_2C0000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long value;

  value = SEXT16 (insn & 0xffff);
  State.regs[REG_D0 + REG0_16 (insn)] = value;
}

/* mov imm32,dn */
void OP_FCCC0000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long value;

  value = ((insn & 0xffff) << 16) + extension;
  State.regs[REG_D0 + REG0_16 (insn)] = value;
}

/* mov imm16, an */
void OP_240000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long value;

  value = insn & 0xffff;
  State.regs[REG_A0 + REG0_16 (insn)] = value;
}

/* mov imm32, an */
void OP_FCDC0000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long value;

  value = ((insn & 0xffff) << 16) + extension;
  State.regs[REG_A0 + REG0_16 (insn)] = value;
}

/* movbu (am), dn */
void OP_F040 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG1 (insn)]
    = load_byte (State.regs[REG_A0 + REG0 (insn)]);
}

/* movbu (d8,am), dn */
void OP_F84000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG1_8 (insn)]
    = load_byte ((State.regs[REG_A0 + REG0_8 (insn)]
		  + SEXT8 (insn & 0xff)));
}

/* movbu (d16,am), dn */
void OP_FA400000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG1_16 (insn)]
    = load_byte ((State.regs[REG_A0 + REG0_16 (insn)]
		  + SEXT16 (insn & 0xffff)));
}

/* movbu (d32,am), dn */
void OP_FC400000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG1_16 (insn)]
    = load_byte ((State.regs[REG_A0 + REG0_16 (insn)]
		  + ((insn & 0xffff) << 16) + extension));
}

/* movbu (d8,sp), dn */
void OP_F8B800 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_8 (insn)]
    = load_byte ((State.regs[REG_SP] + (insn & 0xff)));
}

/* movbu (d16,sp), dn */
void OP_FAB80000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = load_byte ((State.regs[REG_SP] + (insn & 0xffff)));
}

/* movbu (d32,sp), dn */
void OP_FCB80000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = load_byte (State.regs[REG_SP] + (((insn & 0xffff) << 16) + extension));
}

/* movbu (di,am), dn */
void OP_F400 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_4 (insn)]
    = load_byte ((State.regs[REG_A0 + REG0 (insn)]
		  + State.regs[REG_D0 + REG1 (insn)]));
}

/* movbu (abs16), dn */
void OP_340000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)] = load_byte ((insn & 0xffff));
}

/* movbu (abs32), dn */
void OP_FCA80000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = load_byte ((((insn & 0xffff) << 16) + extension));
}

/* movbu dm, (an) */
void OP_F050 (insn, extension)
     unsigned long insn, extension;
{
  store_byte (State.regs[REG_A0 + REG0 (insn)],
	      State.regs[REG_D0 + REG1 (insn)]);
}

/* movbu dm, (d8,an) */
void OP_F85000 (insn, extension)
     unsigned long insn, extension;
{
  store_byte ((State.regs[REG_A0 + REG0_8 (insn)] + SEXT8 (insn & 0xff)),
	      State.regs[REG_D0 + REG1_8 (insn)]);
}

/* movbu dm, (d16,an) */
void OP_FA500000 (insn, extension)
     unsigned long insn, extension;
{
  store_byte ((State.regs[REG_A0 + REG0_16 (insn)] + SEXT16 (insn & 0xffff)),
	      State.regs[REG_D0 + REG1_16 (insn)]);
}

/* movbu dm, (d32,an) */
void OP_FC500000 (insn, extension)
     unsigned long insn, extension;
{
  store_byte ((State.regs[REG_A0 + REG0_16 (insn)]
	       + ((insn & 0xffff) << 16) + extension),
	      State.regs[REG_D0 + REG1_16 (insn)]);
}

/* movbu dm, (d8,sp) */
void OP_F89200 (insn, extension)
     unsigned long insn, extension;
{
  store_byte (State.regs[REG_SP] + (insn & 0xff),
	      State.regs[REG_D0 + REG1_8 (insn)]);
}

/* movbu dm, (d16,sp) */
void OP_FA920000 (insn, extension)
     unsigned long insn, extension;
{
  store_byte (State.regs[REG_SP] + (insn & 0xffff),
	      State.regs[REG_D0 + REG1_16 (insn)]);
}

/* movbu dm (d32,sp) */
void OP_FC920000 (insn, extension)
     unsigned long insn, extension;
{
  store_byte (State.regs[REG_SP] + (((insn & 0xffff) << 16) + extension),
	      State.regs[REG_D0 + REG1_16 (insn)]);
}

/* movbu dm, (di,an) */
void OP_F440 (insn, extension)
     unsigned long insn, extension;
{
  store_byte ((State.regs[REG_A0 + REG0 (insn)]
	       + State.regs[REG_D0 + REG1 (insn)]),
	      State.regs[REG_D0 + REG0_4 (insn)]);
}

/* movbu dm, (abs16) */
void OP_20000 (insn, extension)
     unsigned long insn, extension;
{
  store_byte ((insn & 0xffff), State.regs[REG_D0 + REG1_16 (insn)]);
}

/* movbu dm, (abs32) */
void OP_FC820000 (insn, extension)
     unsigned long insn, extension;
{
  store_byte ((((insn & 0xffff) << 16) + extension), State.regs[REG_D0 + REG1_16 (insn)]);
}

/* movhu (am), dn */
void OP_F060 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG1 (insn)]
    = load_half (State.regs[REG_A0 + REG0 (insn)]);
}

/* movhu (d8,am), dn */
void OP_F86000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG1_8 (insn)]
    = load_half ((State.regs[REG_A0 + REG0_8 (insn)]
		  + SEXT8 (insn & 0xff)));
}

/* movhu (d16,am), dn */
void OP_FA600000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG1_16 (insn)]
    = load_half ((State.regs[REG_A0 + REG0_16 (insn)]
		  + SEXT16 (insn & 0xffff)));
}

/* movhu (d32,am), dn */
void OP_FC600000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG1_16 (insn)]
    = load_half ((State.regs[REG_A0 + REG0_16 (insn)]
		  + ((insn & 0xffff) << 16) + extension));
}

/* movhu (d8,sp) dn */
void OP_F8BC00 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_8 (insn)]
    = load_half ((State.regs[REG_SP] + (insn & 0xff)));
}

/* movhu (d16,sp), dn */
void OP_FABC0000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = load_half ((State.regs[REG_SP] + (insn & 0xffff)));
}

/* movhu (d32,sp), dn */
void OP_FCBC0000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = load_half (State.regs[REG_SP] + (((insn & 0xffff) << 16) + extension));
}

/* movhu (di,am), dn */
void OP_F480 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_4 (insn)]
    = load_half ((State.regs[REG_A0 + REG0 (insn)]
		  + State.regs[REG_D0 + REG1 (insn)]));
}

/* movhu (abs16), dn */
void OP_380000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)] = load_half ((insn & 0xffff));
}

/* movhu (abs32), dn */
void OP_FCAC0000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = load_half ((((insn & 0xffff) << 16) + extension));
}

/* movhu dm, (an) */
void OP_F070 (insn, extension)
     unsigned long insn, extension;
{
  store_half (State.regs[REG_A0 + REG0 (insn)],
	      State.regs[REG_D0 + REG1 (insn)]);
}

/* movhu dm, (d8,an) */
void OP_F87000 (insn, extension)
     unsigned long insn, extension;
{
  store_half ((State.regs[REG_A0 + REG0_8 (insn)] + SEXT8 (insn & 0xff)),
	      State.regs[REG_D0 + REG1_8 (insn)]);
}

/* movhu dm, (d16,an) */
void OP_FA700000 (insn, extension)
     unsigned long insn, extension;
{
  store_half ((State.regs[REG_A0 + REG0_16 (insn)] + SEXT16 (insn & 0xffff)),
	      State.regs[REG_D0 + REG1_16 (insn)]);
}

/* movhu dm, (d32,an) */
void OP_FC700000 (insn, extension)
     unsigned long insn, extension;
{
  store_half ((State.regs[REG_A0 + REG0_16 (insn)]
	       + ((insn & 0xffff) << 16) + extension),
	      State.regs[REG_D0 + REG1_16 (insn)]);
}

/* movhu dm,(d8,sp) */
void OP_F89300 (insn, extension)
     unsigned long insn, extension;
{
  store_half (State.regs[REG_SP] + (insn & 0xff),
	      State.regs[REG_D0 + REG1_8 (insn)]);
}

/* movhu dm,(d16,sp) */
void OP_FA930000 (insn, extension)
     unsigned long insn, extension;
{
  store_half (State.regs[REG_SP] + (insn & 0xffff),
	      State.regs[REG_D0 + REG1_16 (insn)]);
}

/* movhu dm,(d32,sp) */
void OP_FC930000 (insn, extension)
     unsigned long insn, extension;
{
  store_half (State.regs[REG_SP] + (((insn & 0xffff) << 16) + extension),
	      State.regs[REG_D0 + REG1_16 (insn)]);
}

/* movhu dm, (di,an) */
void OP_F4C0 (insn, extension)
     unsigned long insn, extension;
{
  store_half ((State.regs[REG_A0 + REG0 (insn)]
	       + State.regs[REG_D0 + REG1 (insn)]),
	      State.regs[REG_D0 + REG0_4 (insn)]);
}

/* movhu dm, (abs16) */
void OP_30000 (insn, extension)
     unsigned long insn, extension;
{
  store_half ((insn & 0xffff), State.regs[REG_D0 + REG1_16 (insn)]);
}

/* movhu dm, (abs32) */
void OP_FC830000 (insn, extension)
     unsigned long insn, extension;
{
  store_half ((((insn & 0xffff) << 16) + extension), State.regs[REG_D0 + REG1_16 (insn)]);
}

/* ext dn */
void OP_F2D0 (insn, extension)
     unsigned long insn, extension;
{
  if (State.regs[REG_D0 + REG0 (insn)] & 0x80000000)
    State.regs[REG_MDR] = -1;
  else
    State.regs[REG_MDR] = 0;
}

/* extb dn */
void OP_10 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)] = SEXT8 (State.regs[REG_D0 + REG0 (insn)]);
}

/* extbu dn */
void OP_14 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)] &= 0xff;
}

/* exth dn */
void OP_18 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)]
    = SEXT16 (State.regs[REG_D0 + REG0 (insn)]);
}

/* exthu dn */
void OP_1C (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)] &= 0xffff;
}

/* movm (sp), reg_list */
void OP_CE00 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long sp = State.regs[REG_SP];
  unsigned long mask;

  mask = insn & 0xff;

  if (mask & 0x8)
    {
      sp += 4;
      State.regs[REG_LAR] = load_word (sp);
      sp += 4;
      State.regs[REG_LIR] = load_word (sp);
      sp += 4;
      State.regs[REG_MDR] = load_word (sp);
      sp += 4;
      State.regs[REG_A0 + 1] = load_word (sp);
      sp += 4;
      State.regs[REG_A0] = load_word (sp);
      sp += 4;
      State.regs[REG_D0 + 1] = load_word (sp);
      sp += 4;
      State.regs[REG_D0] = load_word (sp);
      sp += 4;
    }

  if (mask & 0x10)
    {
      State.regs[REG_A0 + 3] = load_word (sp);
      sp += 4;
    }

  if (mask & 0x20)
    {
      State.regs[REG_A0 + 2] = load_word (sp);
      sp += 4;
    }

  if (mask & 0x40)
    {
      State.regs[REG_D0 + 3] = load_word (sp);
      sp += 4;
    }

  if (mask & 0x80)
    {
      State.regs[REG_D0 + 2] = load_word (sp);
      sp += 4;
    }

  /* And make sure to update the stack pointer.  */
  State.regs[REG_SP] = sp;
}

/* movm reg_list, (sp) */
void OP_CF00 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long sp = State.regs[REG_SP];
  unsigned long mask;

  mask = insn & 0xff;

  if (mask & 0x80)
    {
      sp -= 4;
      store_word (sp, State.regs[REG_D0 + 2]);
    }

  if (mask & 0x40)
    {
      sp -= 4;
      store_word (sp, State.regs[REG_D0 + 3]);
    }

  if (mask & 0x20)
    {
      sp -= 4;
      store_word (sp, State.regs[REG_A0 + 2]);
    }

  if (mask & 0x10)
    {
      sp -= 4;
      store_word (sp, State.regs[REG_A0 + 3]);
    }

  if (mask & 0x8)
    {
      sp -= 4;
      store_word (sp, State.regs[REG_D0]);
      sp -= 4;
      store_word (sp, State.regs[REG_D0 + 1]);
      sp -= 4;
      store_word (sp, State.regs[REG_A0]);
      sp -= 4;
      store_word (sp, State.regs[REG_A0 + 1]);
      sp -= 4;
      store_word (sp, State.regs[REG_MDR]);
      sp -= 4;
      store_word (sp, State.regs[REG_LIR]);
      sp -= 4;
      store_word (sp, State.regs[REG_LAR]);
      sp -= 4;
    }

  /* And make sure to update the stack pointer.  */
  State.regs[REG_SP] = sp;
}

/* clr dn */
void OP_0 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG1 (insn)] = 0;

  PSW |= PSW_Z;
  PSW &= ~(PSW_V | PSW_C | PSW_N);
}

/* add dm,dn */
void OP_E0 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, reg2, value;

  reg1 = State.regs[REG_D0 + REG1 (insn)];
  reg2 = State.regs[REG_D0 + REG0 (insn)];
  value = reg1 + reg2;
  State.regs[REG_D0 + REG0 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (value < reg1) || (value < reg2);
  v = ((reg2 & 0x80000000) == (reg1 & 0x80000000)
       && (reg2 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* add dm, an */
void OP_F160 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, reg2, value;

  reg1 = State.regs[REG_D0 + REG1 (insn)];
  reg2 = State.regs[REG_A0 + REG0 (insn)];
  value = reg1 + reg2;
  State.regs[REG_A0 + REG0 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (value < reg1) || (value < reg2);
  v = ((reg2 & 0x80000000) == (reg1 & 0x80000000)
       && (reg2 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* add am, dn */
void OP_F150 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, reg2, value;

  reg1 = State.regs[REG_A0 + REG1 (insn)];
  reg2 = State.regs[REG_D0 + REG0 (insn)];
  value = reg1 + reg2;
  State.regs[REG_D0 + REG0 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (value < reg1) || (value < reg2);
  v = ((reg2 & 0x80000000) == (reg1 & 0x80000000)
       && (reg2 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* add am,an */
void OP_F170 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, reg2, value;

  reg1 = State.regs[REG_A0 + REG1 (insn)];
  reg2 = State.regs[REG_A0 + REG0 (insn)];
  value = reg1 + reg2;
  State.regs[REG_A0 + REG0 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (value < reg1) || (value < reg2);
  v = ((reg2 & 0x80000000) == (reg1 & 0x80000000)
       && (reg2 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* add imm8, dn */
void OP_2800 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_D0 + REG0_8 (insn)];
  imm = SEXT8 (insn & 0xff);
  value = reg1 + imm;
  State.regs[REG_D0 + REG0_8 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (value < reg1) || (value < imm);
  v = ((reg1 & 0x80000000) == (imm & 0x80000000)
       && (reg1 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* add imm16, dn */
void OP_FAC00000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_D0 + REG0_16 (insn)];
  imm = SEXT16 (insn & 0xffff);
  value = reg1 + imm;
  State.regs[REG_D0 + REG0_16 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (value < reg1) || (value < imm);
  v = ((reg1 & 0x80000000) == (imm & 0x80000000)
       && (reg1 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* add imm32,dn */
void OP_FCC00000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_D0 + REG0_16 (insn)];
  imm = ((insn & 0xffff) << 16) + extension;
  value = reg1 + imm;
  State.regs[REG_D0 + REG0_16 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (value < reg1) || (value < imm);
  v = ((reg1 & 0x80000000) == (imm & 0x80000000)
       && (reg1 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* add imm8, an */
void OP_2000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_A0 + REG0_8 (insn)];
  imm = SEXT8 (insn & 0xff);
  value = reg1 + imm;
  State.regs[REG_A0 + REG0_8 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (value < reg1) || (value < imm);
  v = ((reg1 & 0x80000000) == (imm & 0x80000000)
       && (reg1 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* add imm16, an */
void OP_FAD00000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_A0 + REG0_16 (insn)];
  imm = SEXT16 (insn & 0xffff);
  value = reg1 + imm;
  State.regs[REG_A0 + REG0_16 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (value < reg1) || (value < imm);
  v = ((reg1 & 0x80000000) == (imm & 0x80000000)
       && (reg1 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* add imm32, an */
void OP_FCD00000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_A0 + REG0_16 (insn)];
  imm = ((insn & 0xffff) << 16) + extension;
  value = reg1 + imm;
  State.regs[REG_A0 + REG0_16 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (value < reg1) || (value < imm);
  v = ((reg1 & 0x80000000) == (imm & 0x80000000)
       && (reg1 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* add imm8, sp */
void OP_F8FE00 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_SP];
  imm = SEXT8 (insn & 0xff);
  value = reg1 + imm;
  State.regs[REG_SP] = value;
}

/* add imm16,sp */
void OP_FAFE0000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_SP];
  imm = SEXT16 (insn & 0xffff);
  value = reg1 + imm;
  State.regs[REG_SP] = value;
}

/* add imm32, sp */
void OP_FCFE0000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_SP];
  imm = ((insn & 0xffff) << 16) + extension;
  value = reg1 + imm;
  State.regs[REG_SP] = value;
}

/* addc dm,dn */
void OP_F140 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, reg2, value;

  reg1 = State.regs[REG_D0 + REG1 (insn)];
  reg2 = State.regs[REG_D0 + REG0 (insn)];
  value = reg1 + reg2 + ((PSW & PSW_C) != 0);
  State.regs[REG_D0 + REG0 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (value < reg1) || (value < reg2);
  v = ((reg2 & 0x80000000) == (reg1 & 0x80000000)
       && (reg2 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* sub dm, dn */
void OP_F100 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, reg2, value;

  reg1 = State.regs[REG_D0 + REG1 (insn)];
  reg2 = State.regs[REG_D0 + REG0 (insn)];
  value = reg2 - reg1;
  State.regs[REG_D0 + REG0 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 > reg2);
  v = ((reg2 & 0x80000000) != (reg1 & 0x80000000)
       && (reg2 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* sub dm, an */
void OP_F120 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, reg2, value;

  reg1 = State.regs[REG_D0 + REG1 (insn)];
  reg2 = State.regs[REG_A0 + REG0 (insn)];
  value = reg2 - reg1;
  State.regs[REG_A0 + REG0 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 > reg2);
  v = ((reg2 & 0x80000000) != (reg1 & 0x80000000)
       && (reg2 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* sub am, dn */
void OP_F110 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, reg2, value;

  reg1 = State.regs[REG_A0 + REG1 (insn)];
  reg2 = State.regs[REG_D0 + REG0 (insn)];
  value = reg2 - reg1;
  State.regs[REG_D0 + REG0 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 > reg2);
  v = ((reg2 & 0x80000000) != (reg1 & 0x80000000)
       && (reg2 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* sub am, an */
void OP_F130 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, reg2, value;

  reg1 = State.regs[REG_A0 + REG1 (insn)];
  reg2 = State.regs[REG_A0 + REG0 (insn)];
  value = reg2 - reg1;
  State.regs[REG_A0 + REG0 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 > reg2);
  v = ((reg2 & 0x80000000) != (reg1 & 0x80000000)
       && (reg2 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* sub imm32, dn */
void OP_FCC40000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_D0 + REG0_16 (insn)];
  imm = ((insn & 0xffff) << 16) + extension;
  value = reg1 - imm;
  State.regs[REG_D0 + REG0_16 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 < imm);
  v = ((reg1 & 0x80000000) != (imm & 0x80000000)
       && (reg1 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* sub imm32, an */
void OP_FCD40000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_A0 + REG0_16 (insn)];
  imm = ((insn & 0xffff) << 16) + extension;
  value = reg1 - imm;
  State.regs[REG_A0 + REG0_16 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 < imm);
  v = ((reg1 & 0x80000000) != (imm & 0x80000000)
       && (reg1 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* subc dm, dn */
void OP_F180 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, reg2, value;

  reg1 = State.regs[REG_D0 + REG1 (insn)];
  reg2 = State.regs[REG_D0 + REG0 (insn)];
  value = reg2 - reg1 - ((PSW & PSW_C) != 0);
  State.regs[REG_D0 + REG0 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 > reg2);
  v = ((reg2 & 0x80000000) != (reg1 & 0x80000000)
       && (reg2 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* mul dm, dn */
void OP_F240 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long long temp;
  int n, z;

  temp = ((signed64)(signed32)State.regs[REG_D0 + REG0 (insn)]
          *  (signed64)(signed32)State.regs[REG_D0 + REG1 (insn)]);
  State.regs[REG_D0 + REG0 (insn)] = temp & 0xffffffff;
  State.regs[REG_MDR] = (temp & 0xffffffff00000000LL) >> 32;;
  z = (State.regs[REG_D0 + REG0 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* mulu dm, dn */
void OP_F250 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long long temp;
  int n, z;

  temp = ((unsigned64)State.regs[REG_D0 + REG0 (insn)]
          * (unsigned64)State.regs[REG_D0 + REG1 (insn)]);
  State.regs[REG_D0 + REG0 (insn)] = temp & 0xffffffff;
  State.regs[REG_MDR] = (temp & 0xffffffff00000000LL) >> 32;
  z = (State.regs[REG_D0 + REG0 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* div dm, dn */
void OP_F260 (insn, extension)
     unsigned long insn, extension;
{
  long long temp;
  int n, z;

  temp = State.regs[REG_MDR];
  temp <<= 32;
  temp |= State.regs[REG_D0 + REG0 (insn)];
  State.regs[REG_MDR] = temp % (long)State.regs[REG_D0 + REG1 (insn)];
  temp /= (long)State.regs[REG_D0 + REG1 (insn)];
  State.regs[REG_D0 + REG0 (insn)] = temp & 0xffffffff;
  z = (State.regs[REG_D0 + REG0 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* divu dm, dn */
void OP_F270 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long long temp;
  int n, z;

  temp = State.regs[REG_MDR];
  temp <<= 32;
  temp |= State.regs[REG_D0 + REG0 (insn)];
  State.regs[REG_MDR] = temp % State.regs[REG_D0 + REG1 (insn)];
  temp /= State.regs[REG_D0 + REG1 (insn)];
  State.regs[REG_D0 + REG0 (insn)] = temp & 0xffffffff;
  z = (State.regs[REG_D0 + REG0 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* inc dn */
void OP_40 (insn, extension)
     unsigned long insn, extension;
{
  int z,n,c,v;
  unsigned int value, imm, reg1;

  reg1 = State.regs[REG_D0 + REG1 (insn)];
  imm = 1;
  value = reg1 + imm;
  State.regs[REG_D0 + REG1 (insn)] = value;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (value < imm);
  v = ((reg1 & 0x80000000) == (imm & 0x80000000)
       && (reg1 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* inc an */
void OP_41 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG1 (insn)] += 1;
}

/* inc4 an */
void OP_50 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0 (insn)] += 4;
}

/* cmp imm8, dn */
void OP_A000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_D0 + REG0_8 (insn)];
  imm = SEXT8 (insn & 0xff);
  value = reg1 - imm;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 < imm);
  v = ((reg1 & 0x80000000) != (imm & 0x80000000)
       && (reg1 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* cmp dm, dn */
void OP_A0 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, reg2, value;

  reg1 = State.regs[REG_D0 + REG1 (insn)];
  reg2 = State.regs[REG_D0 + REG0 (insn)];
  value = reg2 - reg1;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 > reg2);
  v = ((reg2 & 0x80000000) != (reg1 & 0x80000000)
       && (reg2 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* cmp dm, an */
void OP_F1A0 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, reg2, value;

  reg1 = State.regs[REG_D0 + REG1 (insn)];
  reg2 = State.regs[REG_A0 + REG0 (insn)];
  value = reg2 - reg1;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 > reg2);
  v = ((reg2 & 0x80000000) != (reg1 & 0x80000000)
       && (reg2 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* cmp am, dn */
void OP_F190 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, reg2, value;

  reg1 = State.regs[REG_A0 + REG1 (insn)];
  reg2 = State.regs[REG_D0 + REG0 (insn)];
  value = reg2 - reg1;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 > reg2);
  v = ((reg2 & 0x80000000) != (reg1 & 0x80000000)
       && (reg2 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* cmp imm8, an */
void OP_B000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_A0 + REG0_8 (insn)];
  imm = insn & 0xff;
  value = reg1 - imm;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 < imm);
  v = ((reg1 & 0x80000000) != (imm & 0x80000000)
       && (reg1 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* cmp am, an */
void OP_B0 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, reg2, value;

  reg1 = State.regs[REG_A0 + REG1 (insn)];
  reg2 = State.regs[REG_A0 + REG0 (insn)];
  value = reg2 - reg1;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 > reg2);
  v = ((reg2 & 0x80000000) != (reg1 & 0x80000000)
       && (reg2 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* cmp imm16, dn */
void OP_FAC80000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_D0 + REG0_16 (insn)];
  imm = SEXT16 (insn & 0xffff);
  value = reg1 - imm;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 < imm);
  v = ((reg1 & 0x80000000) != (imm & 0x80000000)
       && (reg1 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* cmp imm32, dn */
void OP_FCC80000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_D0 + REG0_16 (insn)];
  imm = ((insn & 0xffff) << 16) + extension;
  value = reg1 - imm;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 < imm);
  v = ((reg1 & 0x80000000) != (imm & 0x80000000)
       && (reg1 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* cmp imm16, an */
void OP_FAD80000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_A0 + REG0_16 (insn)];
  imm = insn & 0xffff;
  value = reg1 - imm;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 < imm);
  v = ((reg1 & 0x80000000) != (imm & 0x80000000)
       && (reg1 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* cmp imm32, an */
void OP_FCD80000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v;
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_A0 + REG0_16 (insn)];
  imm = ((insn & 0xffff) << 16) + extension;
  value = reg1 - imm;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (reg1 < imm);
  v = ((reg1 & 0x80000000) != (imm & 0x80000000)
       && (reg1 & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

/* and dm, dn */
void OP_F200 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;

  State.regs[REG_D0 + REG0 (insn)] &= State.regs[REG_D0 + REG1 (insn)];
  z = (State.regs[REG_D0 + REG0 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* and imm8, dn */
void OP_F8E000 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;

  State.regs[REG_D0 + REG0_8 (insn)] &= (insn & 0xff);
  z = (State.regs[REG_D0 + REG0_8 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_8 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* and imm16, dn */
void OP_FAE00000 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;

  State.regs[REG_D0 + REG0_16 (insn)] &= (insn & 0xffff);
  z = (State.regs[REG_D0 + REG0_16 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_16 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* and imm32, dn */
void OP_FCE00000 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;

  State.regs[REG_D0 + REG0_16 (insn)]
    &= ((insn & 0xffff) << 16) + extension;
  z = (State.regs[REG_D0 + REG0_16 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_16 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* and imm16, psw */
void OP_FAFC0000 (insn, extension)
     unsigned long insn, extension;
{
  PSW &= (insn & 0xffff);
}

/* or dm, dn*/
void OP_F210 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;

  State.regs[REG_D0 + REG0 (insn)] |= State.regs[REG_D0 + REG1 (insn)];
  z = (State.regs[REG_D0 + REG0 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* or imm8, dn */
void OP_F8E400 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;

  State.regs[REG_D0 + REG0_8 (insn)] |= insn & 0xff;
  z = (State.regs[REG_D0 + REG0_8 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_8 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* or imm16, dn*/
void OP_FAE40000 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;

  State.regs[REG_D0 + REG0_16 (insn)] |= insn & 0xffff;
  z = (State.regs[REG_D0 + REG0_16 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_16 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* or imm32, dn */
void OP_FCE40000 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;

  State.regs[REG_D0 + REG0_16 (insn)]
    |= ((insn & 0xffff) << 16) + extension;
  z = (State.regs[REG_D0 + REG0_16 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_16 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* or imm16,psw */
void OP_FAFD0000 (insn, extension)
     unsigned long insn, extension;
{
  PSW |= (insn & 0xffff);
}

/* xor dm, dn */
void OP_F220 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;

  State.regs[REG_D0 + REG0 (insn)] ^= State.regs[REG_D0 + REG1 (insn)];
  z = (State.regs[REG_D0 + REG0 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* xor imm16, dn */
void OP_FAE80000 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;

  State.regs[REG_D0 + REG0_16 (insn)] ^= insn & 0xffff;
  z = (State.regs[REG_D0 + REG0_16 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_16 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* xor imm32, dn */
void OP_FCE80000 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;

  State.regs[REG_D0 + REG0_16 (insn)]
    ^= ((insn & 0xffff) << 16) + extension;
  z = (State.regs[REG_D0 + REG0_16 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_16 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* not dn */
void OP_F230 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;

  State.regs[REG_D0 + REG0 (insn)] = ~State.regs[REG_D0 + REG0 (insn)];
  z = (State.regs[REG_D0 + REG0 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* btst imm8, dn */
void OP_F8EC00 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int z, n;

  temp = State.regs[REG_D0 + REG0_8 (insn)];
  temp &= (insn & 0xff);
  n = (temp & 0x80000000) != 0;
  z = (temp == 0);
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= (z ? PSW_Z : 0) | (n ? PSW_N : 0);
}

/* btst imm16, dn */
void OP_FAEC0000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int z, n;

  temp = State.regs[REG_D0 + REG0_16 (insn)];
  temp &= (insn & 0xffff);
  n = (temp & 0x80000000) != 0;
  z = (temp == 0);
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= (z ? PSW_Z : 0) | (n ? PSW_N : 0);
}

/* btst imm32, dn */
void OP_FCEC0000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int z, n;

  temp = State.regs[REG_D0 + REG0_16 (insn)];
  temp &= ((insn & 0xffff) << 16) + extension;
  n = (temp & 0x80000000) != 0;
  z = (temp == 0);
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= (z ? PSW_Z : 0) | (n ? PSW_N : 0);
}

/* btst imm8,(abs32) */
void OP_FE020000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int n, z;

  temp = load_byte (((insn & 0xffff) << 16) | (extension >> 8));
  temp &= (extension & 0xff);
  n = (temp & 0x80000000) != 0;
  z = (temp == 0);
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= (z ? PSW_Z : 0) | (n ? PSW_N : 0);
}

/* btst imm8,(d8,an) */
void OP_FAF80000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int n, z;

  temp = load_byte ((State.regs[REG_A0 + REG0_16 (insn)]
                     + SEXT8 ((insn & 0xff00) >> 8)));
  temp &= (insn & 0xff);
  n = (temp & 0x80000000) != 0;
  z = (temp == 0);
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= (z ? PSW_Z : 0) | (n ? PSW_N : 0);
}

/* bset dm, (an) */
void OP_F080 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int z;

  temp = load_byte (State.regs[REG_A0 + REG0 (insn)]);
  z = (temp & State.regs[REG_D0 + REG1 (insn)]) == 0;
  temp |= State.regs[REG_D0 + REG1 (insn)];
  store_byte (State.regs[REG_A0 + REG0 (insn)], temp);
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= (z ? PSW_Z : 0);
}

/* bset imm8, (abs32) */
void OP_FE000000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int z;

  temp = load_byte (((insn & 0xffff) << 16 | (extension >> 8)));
  z = (temp & (extension & 0xff)) == 0;
  temp |= (extension & 0xff);
  store_byte ((((insn & 0xffff) << 16) | (extension >> 8)), temp);
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= (z ? PSW_Z : 0);
}

/* bset imm8,(d8,an) */
void OP_FAF00000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int z;

  temp = load_byte ((State.regs[REG_A0 + REG0_16 (insn)]
                     + SEXT8 ((insn & 0xff00) >> 8)));
  z = (temp & (insn & 0xff)) == 0;
  temp |= (insn & 0xff);
  store_byte ((State.regs[REG_A0 + REG0_16 (insn)]
	       + SEXT8 ((insn & 0xff00) >> 8)), temp);
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= (z ? PSW_Z : 0);
}

/* bclr dm, (an) */
void OP_F090 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int z;

  temp = load_byte (State.regs[REG_A0 + REG0 (insn)]);
  z = (temp & State.regs[REG_D0 + REG1 (insn)]) == 0;
  temp = temp & ~State.regs[REG_D0 + REG1 (insn)];
  store_byte (State.regs[REG_A0 + REG0 (insn)], temp);
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= (z ? PSW_Z : 0);
}

/* bclr imm8, (abs32) */
void OP_FE010000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int z;

  temp = load_byte (((insn & 0xffff) << 16) | (extension >> 8));
  z = (temp & (extension & 0xff)) == 0;
  temp = temp & ~(extension & 0xff);
  store_byte (((insn & 0xffff) << 16) | (extension >> 8), temp);
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= (z ? PSW_Z : 0);
}

/* bclr imm8,(d8,an) */
void OP_FAF40000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int z;

  temp = load_byte ((State.regs[REG_A0 + REG0_16 (insn)]
                     + SEXT8 ((insn & 0xff00) >> 8)));
  z = (temp & (insn & 0xff)) == 0;
  temp = temp & ~(insn & 0xff);
  store_byte ((State.regs[REG_A0 + REG0_16 (insn)]
	       + SEXT8 ((insn & 0xff00) >> 8)), temp);
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= (z ? PSW_Z : 0);
}

/* asr dm, dn */
void OP_F2B0 (insn, extension)
     unsigned long insn, extension;
{
  long temp;
  int z, n, c;

  temp = State.regs[REG_D0 + REG0 (insn)];
  c = temp & 1;
  temp >>= State.regs[REG_D0 + REG1 (insn)];
  State.regs[REG_D0 + REG0 (insn)] = temp;
  z = (State.regs[REG_D0 + REG0 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0) | (c ? PSW_C : 0));
}

/* asr imm8, dn */
void OP_F8C800 (insn, extension)
     unsigned long insn, extension;
{
  long temp;
  int z, n, c;

  temp = State.regs[REG_D0 + REG0_8 (insn)];
  c = temp & 1;
  temp >>= (insn & 0xff);
  State.regs[REG_D0 + REG0_8 (insn)] = temp;
  z = (State.regs[REG_D0 + REG0_8 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_8 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0) | (c ? PSW_C : 0));
}

/* lsr dm, dn */
void OP_F2A0 (insn, extension)
     unsigned long insn, extension;
{
  int z, n, c;

  c = State.regs[REG_D0 + REG0 (insn)] & 1;
  State.regs[REG_D0 + REG0 (insn)]
    >>= State.regs[REG_D0 + REG1 (insn)];
  z = (State.regs[REG_D0 + REG0 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0) | (c ? PSW_C : 0));
}

/* lsr imm8, dn */
void OP_F8C400 (insn, extension)
     unsigned long insn, extension;
{
  int z, n, c;

  c = State.regs[REG_D0 + REG0_8 (insn)] & 1;
  State.regs[REG_D0 + REG0_8 (insn)] >>=  (insn & 0xff);
  z = (State.regs[REG_D0 + REG0_8 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_8 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0) | (c ? PSW_C : 0));
}

/* asl dm, dn */
void OP_F290 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;

  State.regs[REG_D0 + REG0 (insn)]
    <<= State.regs[REG_D0 + REG1 (insn)];
  z = (State.regs[REG_D0 + REG0 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* asl imm8, dn */
void OP_F8C000 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;

  State.regs[REG_D0 + REG0_8 (insn)] <<= (insn & 0xff);
  z = (State.regs[REG_D0 + REG0_8 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_8 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* asl2 dn */
void OP_54 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;

  State.regs[REG_D0 + REG0 (insn)] <<= 2;
  z = (State.regs[REG_D0 + REG0 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* ror dn */
void OP_F284 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long value;
  int c,n,z;

  value = State.regs[REG_D0 + REG0 (insn)];
  c = (value & 0x1);

  value >>= 1;
  value |= ((PSW & PSW_C) != 0) ? 0x80000000 : 0;
  State.regs[REG_D0 + REG0 (insn)] = value;
  z = (value == 0);
  n = (value & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0) | (c ? PSW_C : 0));
}

/* rol dn */
void OP_F280 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long value;
  int c,n,z;

  value = State.regs[REG_D0 + REG0 (insn)];
  c = (value & 0x80000000) ? 1 : 0;

  value <<= 1;
  value |= ((PSW & PSW_C) != 0);
  State.regs[REG_D0 + REG0 (insn)] = value;
  z = (value == 0);
  n = (value & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0) | (c ? PSW_C : 0));
}

/* beq label:8 */
void OP_C800 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (PSW & PSW_Z)
    State.regs[REG_PC] += SEXT8 (insn & 0xff) - 2;
}

/* bne label:8 */
void OP_C900 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (!(PSW & PSW_Z))
    State.regs[REG_PC] += SEXT8 (insn & 0xff) - 2;
}

/* bgt label:8 */
void OP_C100 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (!((PSW & PSW_Z)
        || (((PSW & PSW_N) != 0) ^ ((PSW & PSW_V) != 0))))
    State.regs[REG_PC] += SEXT8 (insn & 0xff) - 2;
}

/* bge label:8 */
void OP_C200 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (!(((PSW & PSW_N) != 0) ^ ((PSW & PSW_V) != 0)))
    State.regs[REG_PC] += SEXT8 (insn & 0xff) - 2;
}

/* ble label:8 */
void OP_C300 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if ((PSW & PSW_Z)
       || (((PSW & PSW_N) != 0) ^ ((PSW & PSW_V) != 0)))
    State.regs[REG_PC] += SEXT8 (insn & 0xff) - 2;
}

/* blt label:8 */
void OP_C000 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (((PSW & PSW_N) != 0) ^ ((PSW & PSW_V) != 0))
    State.regs[REG_PC] += SEXT8 (insn & 0xff) - 2;
}

/* bhi label:8 */
void OP_C500 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (!(((PSW & PSW_C) != 0) || (PSW & PSW_Z) != 0))
    State.regs[REG_PC] += SEXT8 (insn & 0xff) - 2;
}

/* bcc label:8 */
void OP_C600 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (!(PSW & PSW_C))
    State.regs[REG_PC] += SEXT8 (insn & 0xff) - 2;
}

/* bls label:8 */
void OP_C700 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (((PSW & PSW_C) != 0) || (PSW & PSW_Z) != 0)
    State.regs[REG_PC] += SEXT8 (insn & 0xff) - 2;
}

/* bcs label:8 */
void OP_C400 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (PSW & PSW_C)
    State.regs[REG_PC] += SEXT8 (insn & 0xff) - 2;
}

/* bvc label:8 */
void OP_F8E800 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (!(PSW & PSW_V))
    State.regs[REG_PC] += SEXT8 (insn & 0xff) - 3;
}

/* bvs label:8 */
void OP_F8E900 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (PSW & PSW_V)
    State.regs[REG_PC] += SEXT8 (insn & 0xff) - 3;
}

/* bnc label:8 */
void OP_F8EA00 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (!(PSW & PSW_N))
    State.regs[REG_PC] += SEXT8 (insn & 0xff) - 3;
}

/* bns label:8 */
void OP_F8EB00 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (PSW & PSW_N)
    State.regs[REG_PC] += SEXT8 (insn & 0xff) - 3;
}

/* bra label:8 */
void OP_CA00 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  State.regs[REG_PC] += SEXT8 (insn & 0xff) - 2;
}

/* leq */
void OP_D8 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 1 after we return, so
     we subtract one here to make things right.  */
  if (PSW & PSW_Z)
    State.regs[REG_PC] = State.regs[REG_LAR] - 4 - 1;
}

/* lne */
void OP_D9 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 1 after we return, so
     we subtract one here to make things right.  */
  if (!(PSW & PSW_Z))
    State.regs[REG_PC] = State.regs[REG_LAR] - 4 - 1;
}

/* lgt */
void OP_D1 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 1 after we return, so
     we subtract one here to make things right.  */
  if (!((PSW & PSW_Z)
        || (((PSW & PSW_N) != 0) ^ ((PSW & PSW_V) != 0))))
    State.regs[REG_PC] = State.regs[REG_LAR] - 4 - 1;
}

/* lge */
void OP_D2 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 1 after we return, so
     we subtract one here to make things right.  */
  if (!(((PSW & PSW_N) != 0) ^ ((PSW & PSW_V) != 0)))
    State.regs[REG_PC] = State.regs[REG_LAR] - 4 - 1;
}

/* lle */
void OP_D3 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 1 after we return, so
     we subtract one here to make things right.  */
  if ((PSW & PSW_Z)
       || (((PSW & PSW_N) != 0) ^ ((PSW & PSW_V) != 0)))
    State.regs[REG_PC] = State.regs[REG_LAR] - 4 - 1;
}

/* llt */
void OP_D0 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 1 after we return, so
     we subtract one here to make things right.  */
  if (((PSW & PSW_N) != 0) ^ ((PSW & PSW_V) != 0))
    State.regs[REG_PC] = State.regs[REG_LAR] - 4 - 1;
}

/* lhi */
void OP_D5 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 1 after we return, so
     we subtract one here to make things right.  */
  if (!(((PSW & PSW_C) != 0) || (PSW & PSW_Z) != 0))
    State.regs[REG_PC] = State.regs[REG_LAR] - 4 - 1;
}

/* lcc */
void OP_D6 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 1 after we return, so
     we subtract one here to make things right.  */
  if (!(PSW & PSW_C))
    State.regs[REG_PC] = State.regs[REG_LAR] - 4 - 1;
}

/* lls */
void OP_D7 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 1 after we return, so
     we subtract one here to make things right.  */
  if (((PSW & PSW_C) != 0) || (PSW & PSW_Z) != 0)
    State.regs[REG_PC] = State.regs[REG_LAR] - 4 - 1;
}

/* lcs */
void OP_D4 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 1 after we return, so
     we subtract one here to make things right.  */
  if (PSW & PSW_C)
    State.regs[REG_PC] = State.regs[REG_LAR] - 4 - 1;
}

/* lra */
void OP_DA (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_PC] = State.regs[REG_LAR] - 4 - 1;
}

/* setlb */
void OP_DB (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_LIR] = load_mem_big (State.regs[REG_PC] + 1, 4);
  State.regs[REG_LAR] = State.regs[REG_PC] + 5;
}

/* jmp (an) */
void OP_F0F4 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_PC] = State.regs[REG_A0 + REG0 (insn)] - 2;
}

/* jmp label:16 */
void OP_CC0000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_PC] += SEXT16 (insn & 0xffff) - 3;
}

/* jmp label:32 */
void OP_DC000000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_PC] += (((insn & 0xffffff) << 8) + extension) - 5;
}

/* call label:16,reg_list,imm8 */
void OP_CD000000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned int next_pc, sp;
  unsigned long mask;

  sp = State.regs[REG_SP];
  next_pc = State.regs[REG_PC] + 5;
  State.mem[sp] = next_pc & 0xff;
  State.mem[sp+1] = (next_pc & 0xff00) >> 8;
  State.mem[sp+2] = (next_pc & 0xff0000) >> 16;
  State.mem[sp+3] = (next_pc & 0xff000000) >> 24;

  mask = insn & 0xff;

  if (mask & 0x80)
    {
      sp -= 4;
      store_word (sp, State.regs[REG_D0 + 2]);
    }

  if (mask & 0x40)
    {
      sp -= 4;
      store_word (sp, State.regs[REG_D0 + 3]);
    }

  if (mask & 0x20)
    {
      sp -= 4;
      store_word (sp, State.regs[REG_A0 + 2]);
    }

  if (mask & 0x10)
    {
      sp -= 4;
      store_word (sp, State.regs[REG_A0 + 3]);
    }

  if (mask & 0x8)
    {
      sp -= 4;
      store_word (sp, State.regs[REG_D0]);
      sp -= 4;
      store_word (sp, State.regs[REG_D0 + 1]);
      sp -= 4;
      store_word (sp, State.regs[REG_A0]);
      sp -= 4;
      store_word (sp, State.regs[REG_A0 + 1]);
      sp -= 4;
      store_word (sp, State.regs[REG_MDR]);
      sp -= 4;
      store_word (sp, State.regs[REG_LIR]);
      sp -= 4;
      store_word (sp, State.regs[REG_LAR]);
      sp -= 4;
    }

  /* Update the stack pointer, note that the register saves to do not
     modify SP.  The SP adjustment is derived totally from the imm8
     field.  */
  State.regs[REG_SP] -= extension;
  State.regs[REG_MDR] = next_pc;
  State.regs[REG_PC] += SEXT16 ((insn & 0xffff00) >> 8) - 5;
}

/* call label:32,reg_list,imm8*/
void OP_DD000000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned int next_pc, sp, adjust;
  unsigned long mask;

  sp = State.regs[REG_SP];
  next_pc = State.regs[REG_PC] + 7;
  State.mem[sp] = next_pc & 0xff;
  State.mem[sp+1] = (next_pc & 0xff00) >> 8;
  State.mem[sp+2] = (next_pc & 0xff0000) >> 16;
  State.mem[sp+3] = (next_pc & 0xff000000) >> 24;

  mask = (extension & 0xff00) >> 8;

  if (mask & 0x80)
    {
      sp -= 4;
      store_word (sp, State.regs[REG_D0 + 2]);
    }

  if (mask & 0x40)
    {
      sp -= 4;
      store_word (sp, State.regs[REG_D0 + 3]);
    }

  if (mask & 0x20)
    {
      sp -= 4;
      store_word (sp, State.regs[REG_A0 + 2]);
    }

  if (mask & 0x10)
    {
      sp -= 4;
      store_word (sp, State.regs[REG_A0 + 3]);
    }

  if (mask & 0x8)
    {
      sp -= 4;
      store_word (sp, State.regs[REG_D0]);
      sp -= 4;
      store_word (sp, State.regs[REG_D0 + 1]);
      sp -= 4;
      store_word (sp, State.regs[REG_A0]);
      sp -= 4;
      store_word (sp, State.regs[REG_A0 + 1]);
      sp -= 4;
      store_word (sp, State.regs[REG_MDR]);
      sp -= 4;
      store_word (sp, State.regs[REG_LIR]);
      sp -= 4;
      store_word (sp, State.regs[REG_LAR]);
      sp -= 4;
    }

  /* Update the stack pointer, note that the register saves to do not
     modify SP.  The SP adjustment is derived totally from the imm8
     field.  */
  State.regs[REG_SP] -= (extension & 0xff);
  State.regs[REG_MDR] = next_pc;
  State.regs[REG_PC] += (((insn & 0xffffff) << 8) | ((extension & 0xff0000) >> 16)) - 7;
}

/* calls (an) */
void OP_F0F0 (insn, extension)
     unsigned long insn, extension;
{
  unsigned int next_pc, sp;

  sp = State.regs[REG_SP];
  next_pc = State.regs[REG_PC] + 2;
  State.mem[sp] = next_pc & 0xff;
  State.mem[sp+1] = (next_pc & 0xff00) >> 8;
  State.mem[sp+2] = (next_pc & 0xff0000) >> 16;
  State.mem[sp+3] = (next_pc & 0xff000000) >> 24;
  State.regs[REG_MDR] = next_pc;
  State.regs[REG_PC] = State.regs[REG_A0 + REG0 (insn)] - 2;
}

/* calls label:16 */
void OP_FAFF0000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned int next_pc, sp;

  sp = State.regs[REG_SP];
  next_pc = State.regs[REG_PC] + 4;
  State.mem[sp] = next_pc & 0xff;
  State.mem[sp+1] = (next_pc & 0xff00) >> 8;
  State.mem[sp+2] = (next_pc & 0xff0000) >> 16;
  State.mem[sp+3] = (next_pc & 0xff000000) >> 24;
  State.regs[REG_MDR] = next_pc;
  State.regs[REG_PC] += SEXT16 (insn & 0xffff) - 4;
}

/* calls label:32 */
void OP_FCFF0000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned int next_pc, sp;

  sp = State.regs[REG_SP];
  next_pc = State.regs[REG_PC] + 6;
  State.mem[sp] = next_pc & 0xff;
  State.mem[sp+1] = (next_pc & 0xff00) >> 8;
  State.mem[sp+2] = (next_pc & 0xff0000) >> 16;
  State.mem[sp+3] = (next_pc & 0xff000000) >> 24;
  State.regs[REG_MDR] = next_pc;
  State.regs[REG_PC] += (((insn & 0xffff) << 16) + extension) - 6;
}

/* ret reg_list, imm8 */
void OP_DF0000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned int sp, offset;
  unsigned long mask;

  State.regs[REG_SP] += insn & 0xff;
  sp = State.regs[REG_SP];

  offset = -4;
  mask = (insn & 0xff00) >> 8;

  if (mask & 0x80)
    {
      State.regs[REG_D0 + 2] = load_word (sp + offset);
      offset -= 4;
    }

  if (mask & 0x40)
    {
      State.regs[REG_D0 + 3] = load_word (sp + offset);
      offset -= 4;
    }

  if (mask & 0x20)
    {
      State.regs[REG_A0 + 2] = load_word (sp + offset);
      offset -= 4;
    }

  if (mask & 0x10)
    {
      State.regs[REG_A0 + 3] = load_word (sp + offset);
      offset -= 4;
    }

  if (mask & 0x8)
    {
      State.regs[REG_D0] = load_word (sp + offset);
      offset -= 4;
      State.regs[REG_D0 + 1] = load_word (sp + offset);
      offset -= 4;
      State.regs[REG_A0] = load_word (sp + offset);
      offset -= 4;
      State.regs[REG_A0 + 1] = load_word (sp + offset);
      offset -= 4;
      State.regs[REG_MDR] = load_word (sp + offset);
      offset -= 4;
      State.regs[REG_LIR] = load_word (sp + offset);
      offset -= 4;
      State.regs[REG_LAR] = load_word (sp + offset);
      offset -= 4;
    }

  /* Restore the PC value.  */
  State.regs[REG_PC] = (State.mem[sp] | (State.mem[sp+1] << 8)
	      | (State.mem[sp+2] << 16) | (State.mem[sp+3] << 24));
  State.regs[REG_PC] -= 3;
}

/* retf reg_list,imm8 */
void OP_DE0000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned int sp, offset;
  unsigned long mask;

  State.regs[REG_SP] += (insn & 0xff);
  sp = State.regs[REG_SP];
  State.regs[REG_PC] = State.regs[REG_MDR] - 3;

  offset = -4;
  mask = (insn & 0xff00) >> 8;

  if (mask & 0x80)
    {
      State.regs[REG_D0 + 2] = load_word (sp + offset);
      offset -= 4;
    }

  if (mask & 0x40)
    {
      State.regs[REG_D0 + 3] = load_word (sp + offset);
      offset -= 4;
    }

  if (mask & 0x20)
    {
      State.regs[REG_A0 + 2] = load_word (sp + offset);
      offset -= 4;
    }

  if (mask & 0x10)
    {
      State.regs[REG_A0 + 3] = load_word (sp + offset);
      offset -= 4;
    }

  if (mask & 0x8)
    {
      State.regs[REG_D0] = load_word (sp + offset);
      offset -= 4;
      State.regs[REG_D0 + 1] = load_word (sp + offset);
      offset -= 4;
      State.regs[REG_A0] = load_word (sp + offset);
      offset -= 4;
      State.regs[REG_A0 + 1] = load_word (sp + offset);
      offset -= 4;
      State.regs[REG_MDR] = load_word (sp + offset);
      offset -= 4;
      State.regs[REG_LIR] = load_word (sp + offset);
      offset -= 4;
      State.regs[REG_LAR] = load_word (sp + offset);
      offset -= 4;
    }
}

/* rets */
void OP_F0FC (insn, extension)
     unsigned long insn, extension;
{
  unsigned int sp;

  sp = State.regs[REG_SP];
  State.regs[REG_PC] = (State.mem[sp] | (State.mem[sp+1] << 8)
	      | (State.mem[sp+2] << 16) | (State.mem[sp+3] << 24));
  State.regs[REG_PC] -= 2;
}

/* rti */
void OP_F0FD (insn, extension)
     unsigned long insn, extension;
{
  unsigned int sp, next_pc;

  sp = State.regs[REG_SP];
  PSW = State.mem[sp] | (State.mem[sp + 1] << 8);
  State.regs[REG_PC] = (State.mem[sp+4] | (State.mem[sp+5] << 8)
	      | (State.mem[sp+6] << 16) | (State.mem[sp+7] << 24));
  State.regs[REG_SP] += 8;
}

/* trap */
void OP_F0FE (insn, extension)
     unsigned long insn, extension;
{
  unsigned int sp, next_pc;

  sp = State.regs[REG_SP];
  next_pc = State.regs[REG_PC] + 2;
  State.mem[sp] = next_pc & 0xff;
  State.mem[sp+1] = (next_pc & 0xff00) >> 8;
  State.mem[sp+2] = (next_pc & 0xff0000) >> 16;
  State.mem[sp+3] = (next_pc & 0xff000000) >> 24;
  State.regs[REG_PC] = 0x40000010 - 2;
}

/* syscall */
void OP_F0C0 (insn, extension)
     unsigned long insn, extension;
{
  /* We use this for simulated system calls; we may need to change
     it to a reserved instruction if we conflict with uses at
     Matsushita.  */
  int save_errno = errno;	
  errno = 0;

/* Registers passed to trap 0 */

/* Function number.  */
#define FUNC   (State.regs[0])

/* Parameters.  */
#define PARM1   (State.regs[1])
#define PARM2   (load_word (State.regs[REG_SP] + 12))
#define PARM3   (load_word (State.regs[REG_SP] + 16))

/* Registers set by trap 0 */

#define RETVAL State.regs[0]	/* return value */
#define RETERR State.regs[1]	/* return error code */

/* Turn a pointer in a register into a pointer into real memory. */

#define MEMPTR(x) (State.mem + x)

  switch (FUNC)
    {
#if !defined(__GO32__) && !defined(_WIN32)
#ifdef TARGET_SYS_fork
      case TARGET_SYS_fork:
      RETVAL = fork ();
      break;
#endif
#ifdef TARGET_SYS_execve
    case TARGET_SYS_execve:
      RETVAL = execve (MEMPTR (PARM1), (char **) MEMPTR (PARM2),
		       (char **)MEMPTR (PARM3));
      break;
#endif
#ifdef TARGET_SYS_execv
    case TARGET_SYS_execv:
      RETVAL = execve (MEMPTR (PARM1), (char **) MEMPTR (PARM2), NULL);
      break;
#endif
#endif	/* ! GO32 and ! WIN32 */

    case TARGET_SYS_read:
      RETVAL = mn10300_callback->read (mn10300_callback, PARM1,
				    MEMPTR (PARM2), PARM3);
      break;
    case TARGET_SYS_write:
      RETVAL = (int)mn10300_callback->write (mn10300_callback, PARM1,
					     MEMPTR (PARM2), PARM3);
      break;
    case TARGET_SYS_lseek:
      RETVAL = mn10300_callback->lseek (mn10300_callback, PARM1, PARM2, PARM3);
      break;
    case TARGET_SYS_close:
      RETVAL = mn10300_callback->close (mn10300_callback, PARM1);
      break;
    case TARGET_SYS_open:
      RETVAL = mn10300_callback->open (mn10300_callback, MEMPTR (PARM1), PARM2);
      break;
    case TARGET_SYS_exit:
      /* EXIT - caller can look in PARM1 to work out the 
	 reason */
      if (PARM1 == 0xdead)
	State.exception = SIGABRT;
      else
	State.exception = SIGQUIT;
      State.exited = 1;
      break;

    case TARGET_SYS_stat:	/* added at hmsi */
      /* stat system call */
      {
	struct stat host_stat;
	reg_t buf;

	RETVAL = stat (MEMPTR (PARM1), &host_stat);
	
	buf = PARM2;

	/* Just wild-assed guesses.  */
	store_half (buf, host_stat.st_dev);
	store_half (buf + 2, host_stat.st_ino);
	store_word (buf + 4, host_stat.st_mode);
	store_half (buf + 8, host_stat.st_nlink);
	store_half (buf + 10, host_stat.st_uid);
	store_half (buf + 12, host_stat.st_gid);
	store_half (buf + 14, host_stat.st_rdev);
	store_word (buf + 16, host_stat.st_size);
	store_word (buf + 20, host_stat.st_atime);
	store_word (buf + 28, host_stat.st_mtime);
	store_word (buf + 36, host_stat.st_ctime);
      }
      break;

#ifdef TARGET_SYS_chown
    case TARGET_SYS_chown:
      RETVAL = chown (MEMPTR (PARM1), PARM2, PARM3);
      break;
#endif
    case TARGET_SYS_chmod:
      RETVAL = chmod (MEMPTR (PARM1), PARM2);
      break;
#ifdef TARGET_SYS_time
    case TARGET_SYS_time:
      RETVAL = time ((void*) MEMPTR (PARM1));
      break;
#endif
#ifdef TARGET_SYS_times
    case TARGET_SYS_times:
      {
	struct tms tms;
	RETVAL = times (&tms);
	store_word (PARM1, tms.tms_utime);
	store_word (PARM1 + 4, tms.tms_stime);
	store_word (PARM1 + 8, tms.tms_cutime);
	store_word (PARM1 + 12, tms.tms_cstime);
	break;
      }
#endif
#ifdef TARGET_SYS_gettimeofday
    case TARGET_SYS_gettimeofday:
      {
	struct timeval t;
	struct timezone tz;
	RETVAL = gettimeofday (&t, &tz);
	store_word (PARM1, t.tv_sec);
	store_word (PARM1 + 4, t.tv_usec);
	store_word (PARM2, tz.tz_minuteswest);
	store_word (PARM2 + 4, tz.tz_dsttime);
	break;
      }
#endif
#ifdef TARGET_SYS_utime
    case TARGET_SYS_utime:
      /* Cast the second argument to void *, to avoid type mismatch
	 if a prototype is present.  */
      RETVAL = utime (MEMPTR (PARM1), (void *) MEMPTR (PARM2));
      break;
#endif
    default:
      abort ();
    }
  RETERR = errno;
  errno = save_errno;
}

/* rtm */
void OP_F0FF (insn, extension)
     unsigned long insn, extension;
{
 abort ();
}

/* nop */
void OP_CB (insn, extension)
     unsigned long insn, extension;
{
}

/* putx dm,dm */
void OP_F500 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_MDRQ] = State.regs[REG_D0 + REG0 (insn)];
}

/* getx dm,dm */
void OP_F6F0 (insn, extension)
     unsigned long insn, extension;
{
  int z, n;
  z = (State.regs[REG_MDRQ] == 0);
  n = ((State.regs[REG_MDRQ] & 0x80000000) != 0);
  State.regs[REG_D0 + REG0 (insn)] = State.regs[REG_MDRQ];

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= (z ? PSW_Z : 0) | (n ? PSW_N : 0);
}

/* mulq dm,dn */
void OP_F600 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long long temp;
  int n, z;

  temp = ((signed64)(signed32)State.regs[REG_D0 + REG0 (insn)]
          *  (signed64)(signed32)State.regs[REG_D0 + REG1 (insn)]);
  State.regs[REG_D0 + REG0 (insn)] = temp & 0xffffffff;
  State.regs[REG_MDRQ] = (temp & 0xffffffff00000000LL) >> 32;;
  z = (State.regs[REG_D0 + REG0 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* mulq imm8,dn */
void OP_F90000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long long temp;
  int n, z;

  temp = ((signed64)(signed32)State.regs[REG_D0 + REG0_8 (insn)]
          * (signed64)(signed32)SEXT8 (insn & 0xff));
  State.regs[REG_D0 + REG0_8 (insn)] = temp & 0xffffffff;
  State.regs[REG_MDRQ] = (temp & 0xffffffff00000000LL) >> 32;;
  z = (State.regs[REG_D0 + REG0_8 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_8 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* mulq imm16,dn */
void OP_FB000000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long long temp;
  int n, z;

  temp = ((signed64)(signed32)State.regs[REG_D0 + REG0_16 (insn)]
          * (signed64)(signed32)SEXT16 (insn & 0xffff));
  State.regs[REG_D0 + REG0_16 (insn)] = temp & 0xffffffff;
  State.regs[REG_MDRQ] = (temp & 0xffffffff00000000LL) >> 32;;
  z = (State.regs[REG_D0 + REG0_16 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_16 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* mulq imm32,dn */
void OP_FD000000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long long temp;
  int n, z;

  temp = ((signed64)(signed32)State.regs[REG_D0 + REG0_16 (insn)]
          * (signed64)(signed32)(((insn & 0xffff) << 16) + extension));
  State.regs[REG_D0 + REG0_16 (insn)] = temp & 0xffffffff;
  State.regs[REG_MDRQ] = (temp & 0xffffffff00000000LL) >> 32;;
  z = (State.regs[REG_D0 + REG0_16 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_16 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* mulqu dm,dn */
void OP_F610 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long long temp;
  int n, z;

  temp = ((unsigned64) State.regs[REG_D0 + REG0 (insn)]
	  * (unsigned64) State.regs[REG_D0 + REG1 (insn)]);
  State.regs[REG_D0 + REG0 (insn)] = temp & 0xffffffff;
  State.regs[REG_MDRQ] = (temp & 0xffffffff00000000LL) >> 32;;
  z = (State.regs[REG_D0 + REG0 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* mulqu imm8,dn */
void OP_F91400 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long long temp;
  int n, z;

  temp = ((unsigned64)State.regs[REG_D0 + REG0_8 (insn)]
	  * (unsigned64)SEXT8 (insn & 0xff));
  State.regs[REG_D0 + REG0_8 (insn)] = temp & 0xffffffff;
  State.regs[REG_MDRQ] = (temp & 0xffffffff00000000LL) >> 32;;
  z = (State.regs[REG_D0 + REG0_8 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_8 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* mulqu imm16,dn */
void OP_FB140000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long long temp;
  int n, z;

  temp = ((unsigned64)State.regs[REG_D0 + REG0_16 (insn)]
	  * (unsigned64) SEXT16 (insn & 0xffff));
  State.regs[REG_D0 + REG0_16 (insn)] = temp & 0xffffffff;
  State.regs[REG_MDRQ] = (temp & 0xffffffff00000000LL) >> 32;;
  z = (State.regs[REG_D0 + REG0_16 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_16 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* mulqu imm32,dn */
void OP_FD140000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long long temp;
  int n, z;

  temp = ((unsigned64)State.regs[REG_D0 + REG0_16 (insn)]
          * (unsigned64)(((insn & 0xffff) << 16) + extension));
  State.regs[REG_D0 + REG0_16 (insn)] = temp & 0xffffffff;
  State.regs[REG_MDRQ] = (temp & 0xffffffff00000000LL) >> 32;;
  z = (State.regs[REG_D0 + REG0_16 (insn)] == 0);
  n = (State.regs[REG_D0 + REG0_16 (insn)] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}

/* sat16 dm,dn */
void OP_F640 (insn, extension)
     unsigned long insn, extension;
{
  int temp;

  temp = State.regs[REG_D0 + REG1 (insn)];
  temp = (temp > 0x7fff ? 0x7fff : temp);
  temp = (temp < -0x8000 ? -0x8000 : temp);
  State.regs[REG_D0 + REG0 (insn)] = temp;
}

/* sat24 dm,dn */
void OP_F650 (insn, extension)
     unsigned long insn, extension;
{
  int temp;

  temp = State.regs[REG_D0 + REG1 (insn)];
  temp = (temp > 0x7fffff ? 0x7fffff : temp);
  temp = (temp < -0x800000 ? -0x800000 : temp);
  State.regs[REG_D0 + REG0 (insn)] = temp;
}

/* bsch dm,dn */
void OP_F670 (insn, extension)
     unsigned long insn, extension;
{
  int temp, c;

  temp = State.regs[REG_D0 + REG1 (insn)];
  temp <<= (State.regs[REG_D0 + REG0 (insn)] & 0x1f);
  c = (temp != 0 ? 1 : 0);
  PSW &= ~(PSW_C);
  PSW |= (c ? PSW_C : 0);
}

/* breakpoint */
void
OP_FF (insn, extension)
     unsigned long insn, extension;
{
  State.exception = SIGTRAP;
  PC -= 1;
}

