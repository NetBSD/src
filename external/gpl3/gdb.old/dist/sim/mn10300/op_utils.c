#include "sim-main.h"
#include "sim-syscall.h"
#include "targ-vals.h"

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif
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


INLINE_SIM_MAIN (void)
genericAdd(unsigned32 source, unsigned32 destReg)
{
  int z, c, n, v;
  unsigned32 dest, sum;

  dest = State.regs[destReg];
  sum = source + dest;
  State.regs[destReg] = sum;

  z = (sum == 0);
  n = (sum & 0x80000000);
  c = (sum < source) || (sum < dest);
  v = ((dest & 0x80000000) == (source & 0x80000000)
       && (dest & 0x80000000) != (sum & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}




INLINE_SIM_MAIN (void)
genericSub(unsigned32 source, unsigned32 destReg)
{
  int z, c, n, v;
  unsigned32 dest, difference;

  dest = State.regs[destReg];
  difference = dest - source;
  State.regs[destReg] = difference;

  z = (difference == 0);
  n = (difference & 0x80000000);
  c = (source > dest);
  v = ((dest & 0x80000000) != (source & 0x80000000)
       && (dest & 0x80000000) != (difference & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

INLINE_SIM_MAIN (void)
genericCmp(unsigned32 leftOpnd, unsigned32 rightOpnd)
{
  int z, c, n, v;
  unsigned32 value;

  value = rightOpnd - leftOpnd;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (leftOpnd > rightOpnd);
  v = ((rightOpnd & 0x80000000) != (leftOpnd & 0x80000000)
       && (rightOpnd & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}


INLINE_SIM_MAIN (void)
genericOr(unsigned32 source, unsigned32 destReg)
{
  int n, z;

  State.regs[destReg] |= source;
  z = (State.regs[destReg] == 0);
  n = (State.regs[destReg] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}


INLINE_SIM_MAIN (void)
genericXor(unsigned32 source, unsigned32 destReg)
{
  int n, z;

  State.regs[destReg] ^= source;
  z = (State.regs[destReg] == 0);
  n = (State.regs[destReg] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}


INLINE_SIM_MAIN (void)
genericBtst(unsigned32 leftOpnd, unsigned32 rightOpnd)
{
  unsigned32 temp;
  int z, n;

  temp = rightOpnd;
  temp &= leftOpnd;
  n = (temp & 0x80000000) != 0;
  z = (temp == 0);
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= (z ? PSW_Z : 0) | (n ? PSW_N : 0);
}

/* syscall */
INLINE_SIM_MAIN (void)
do_syscall (void)
{
  /* Registers passed to trap 0.  */

  /* Function number.  */
  reg_t func = State.regs[0];
  /* Parameters.  */
  reg_t parm1 = State.regs[1];
  reg_t parm2 = load_word (State.regs[REG_SP] + 12);
  reg_t parm3 = load_word (State.regs[REG_SP] + 16);
  reg_t parm4 = load_word (State.regs[REG_SP] + 20);

  /* We use this for simulated system calls; we may need to change
     it to a reserved instruction if we conflict with uses at
     Matsushita.  */
  int save_errno = errno;	
  errno = 0;

  if (func == TARGET_SYS_exit)
    {
      /* EXIT - caller can look in parm1 to work out the reason */
      sim_engine_halt (simulator, STATE_CPU (simulator, 0), NULL, PC,
		       (parm1 == 0xdead ? SIM_SIGABRT : sim_exited), parm1);
    }
  else
    {
      long result, result2;
      int errcode;

      sim_syscall_multi (STATE_CPU (simulator, 0), func, parm1, parm2,
			 parm3, parm4, &result, &result2, &errcode);

      /* Registers set by trap 0.  */
      State.regs[0] = errcode;
      State.regs[1] = result;
    }

  errno = save_errno;
}
