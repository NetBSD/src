/* Simulator for the Hitachi SH architecture.

   Written by Steve Chamberlain of Cygnus Support.
   sac@cygnus.com

   This file is part of SH sim


		THIS SOFTWARE IS NOT COPYRIGHTED

   Cygnus offers the following for use in the public domain.  Cygnus
   makes no warranty with regard to the software or it's performance
   and the user accepts the software "AS IS" with all faults.

   CYGNUS DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO
   THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

*/

#include "config.h"

#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "sysdep.h"
#include "bfd.h"
#include "callback.h"
#include "remote-sim.h"

/* This file is local - if newlib changes, then so should this.  */
#include "syscall.h"

#include <math.h>

#ifdef _WIN32
#include <float.h>		/* Needed for _isnan() */
#define isnan _isnan
#endif

#ifndef SIGBUS
#define SIGBUS SIGSEGV
#endif

#ifndef SIGQUIT
#define SIGQUIT SIGTERM
#endif

#ifndef SIGTRAP
#define SIGTRAP 5
#endif

#define O_RECOMPILE 85
#define DEFINE_TABLE
#define DISASSEMBLER_TABLE

/* Define the rate at which the simulator should poll the host
   for a quit. */
#define POLL_QUIT_INTERVAL 0x60000

typedef union
{

  struct
  {
    /* On targets like sparc-sun-solaris, fregs will be aligned on a 64 bit
       boundary (because of the d member).  To avoid padding between
       registers - which whould make the job of sim_fetch_register harder,
       we add padding at the start.  */
    int pad_dummy;
    int regs[16];
    int pc;
    int pr;

    int gbr;
    int vbr;
    int mach;
    int macl;

    int sr;

    int fpul;

    int fpscr;

    /* sh3e */
    union fregs_u
      {
	float f[16];
	double d[8];
	int i[16];
      }
    fregs[2];

    int ssr;
    int spc;
    /* sh3 */
    int bank[2][8];

    int ticks;
    int stalls;
    int memstalls;
    int cycles;
    int insts;

    int prevlock;
    int thislock;
    int exception;

    int end_of_registers;

    int msize;
#define PROFILE_FREQ 1
#define PROFILE_SHIFT 2
    int profile;
    unsigned short *profile_hist;
    unsigned char *memory;
  }
  asregs;
  int asints[40];
} saved_state_type;

saved_state_type saved_state;


/* These variables are at file scope so that functions other than
   sim_resume can use the fetch/store macros */

static int target_little_endian;
static int host_little_endian;

#if 1
static int maskl = ~0;
static int maskw = ~0;
#endif

static SIM_OPEN_KIND sim_kind;
static char *myname;


/* Short hand definitions of the registers */

#define SBIT(x) ((x)&sbit)
#define R0 	saved_state.asregs.regs[0]
#define Rn 	saved_state.asregs.regs[n]
#define Rm 	saved_state.asregs.regs[m]
#define UR0 	(unsigned int)(saved_state.asregs.regs[0])
#define UR 	(unsigned int)R
#define UR 	(unsigned int)R
#define SR0 	saved_state.asregs.regs[0]
#define GBR 	saved_state.asregs.gbr
#define VBR 	saved_state.asregs.vbr
#define SSR	saved_state.asregs.ssr
#define SPC	saved_state.asregs.spc
#define MACH 	saved_state.asregs.mach
#define MACL 	saved_state.asregs.macl
#define FPUL	saved_state.asregs.fpul

#define PC pc



/* Alternate bank of registers r0-r6 */

/* Note: code controling SR handles flips between BANK0 and BANK1 */
#define Rn_BANK(n) (saved_state.asregs.bank[!SR_RB][(n)])
#define SET_Rn_BANK(n, EXP) do { saved_state.asregs.bank[!SR_RB][(n)] = (EXP); } while (0)


/* Manipulate SR */

#define SR_MASK_M (1 << 9)
#define SR_MASK_Q (1 << 8)
#define SR_MASK_I (0xf << 4)
#define SR_MASK_S (1 << 1)
#define SR_MASK_T (1 << 0)

#define SR_MASK_BL (1 << 28)
#define SR_MASK_RB (1 << 29)
#define SR_MASK_MD (1 << 30)

#define M 	((saved_state.asregs.sr & SR_MASK_M) != 0)
#define Q 	((saved_state.asregs.sr & SR_MASK_Q) != 0)
#define S 	((saved_state.asregs.sr & SR_MASK_S) != 0)
#define T 	((saved_state.asregs.sr & SR_MASK_T) != 0)

#define SR_BL ((saved_state.asregs.sr & SR_MASK_BL) != 0)
#define SR_RB ((saved_state.asregs.sr & SR_MASK_RB) != 0)
#define SR_MD ((saved_state.asregs.sr & SR_MASK_MD) != 0)

/* Note: don't use this for privileged bits */
#define SET_SR_BIT(EXP, BIT) \
do { \
  if ((EXP) & 1) \
    saved_state.asregs.sr |= (BIT); \
  else \
    saved_state.asregs.sr &= ~(BIT); \
} while (0)

#define SET_SR_M(EXP) SET_SR_BIT ((EXP), SR_MASK_M)
#define SET_SR_Q(EXP) SET_SR_BIT ((EXP), SR_MASK_Q)
#define SET_SR_S(EXP) SET_SR_BIT ((EXP), SR_MASK_S)
#define SET_SR_T(EXP) SET_SR_BIT ((EXP), SR_MASK_T)

#define GET_SR() (saved_state.asregs.sr - 0)
#define SET_SR(x) set_sr (x)
static void
set_sr (new_sr)
     int new_sr;
{
  /* do we need to swap banks */
  int old_gpr = (SR_MD ? !SR_RB : 0);
  int new_gpr = ((new_sr & SR_MASK_MD)
		 ? (new_sr & SR_MASK_RB) == 0
		 : 0);
  if (old_gpr != new_gpr)
    {
      int i;
      for (i = 0; i < 8; i++)
	{
	  saved_state.asregs.bank[old_gpr][i] = saved_state.asregs.regs[i];
	  saved_state.asregs.regs[i] = saved_state.asregs.bank[new_gpr][i];
	}
    }
}


/* Manipulate FPSCR */

#define FPSCR_MASK_FR (1 << 21)
#define FPSCR_MASK_SZ (1 << 20)
#define FPSCR_MASK_PR (1 << 19)

#define FPSCR_FR  ((GET_FPSCR() & FPSCR_MASK_FR) != 0)
#define FPSCR_SZ  ((GET_FPSCR() & FPSCR_MASK_SZ) != 0)
#define FPSCR_PR  ((GET_FPSCR() & FPSCR_MASK_PR) != 0)

static void
set_fpscr1 (x)
	int x;
{
  int old = saved_state.asregs.fpscr;
  saved_state.asregs.fpscr = (x);
  /* swap the floating point register banks */
  if ((saved_state.asregs.fpscr ^ old) & FPSCR_MASK_FR)
    {
      union fregs_u tmpf = saved_state.asregs.fregs[0];
      saved_state.asregs.fregs[0] = saved_state.asregs.fregs[1];
      saved_state.asregs.fregs[1] = tmpf;
    }
}

#define GET_FPSCR()  (saved_state.asregs.fpscr)
#define SET_FPSCR(x) \
do { \
  set_fpscr1 (x); \
} while (0)


int 
fail ()
{
  abort ();
}

int
special_address (addr, bits_written, data)
     void *addr;
     int bits_written, data;
{
  if ((unsigned) addr >> 24 == 0xf0 && bits_written == 32 && (data & 1) == 0)
    /* This invalidates (if not associative) or might invalidate
       (if associative) an instruction cache line.  This is used for
       trampolines.  Since we don't simulate the cache, this is a no-op
       as far as the simulator is concerned.  */
    return 1;
  /* We can't do anything useful with the other stuff, so fail.  */
  return 0;
}

/* This function exists solely for the purpose of setting a breakpoint to
   catch simulated bus errors when running the simulator under GDB.  */

void
bp_holder ()
{
}

/* FIXME: sim_resume should be renamed to sim_engine_run.  sim_resume
   being implemented by ../common/sim_resume.c and the below should
   make a call to sim_engine_halt */

#define BUSERROR(addr, mask, bits_written, data) \
  if (addr & ~mask) \
    { \
      if (special_address (addr, bits_written, data)) \
	return; \
      saved_state.asregs.exception = SIGBUS; \
      bp_holder (); \
    }

/* Define this to enable register lifetime checking.
   The compiler generates "add #0,rn" insns to mark registers as invalid,
   the simulator uses this info to call fail if it finds a ref to an invalid
   register before a def

   #define PARANOID
*/

#ifdef PARANOID
int valid[16];
#define CREF(x)  if(!valid[x]) fail();
#define CDEF(x)  valid[x] = 1;
#define UNDEF(x) valid[x] = 0;
#else
#define CREF(x)
#define CDEF(x)
#define UNDEF(x)
#endif

static void parse_and_set_memory_size PARAMS ((char *str));

static int IOMEM PARAMS ((int addr, int write, int value));

static host_callback *callback;



/* Floating point registers */

#define DR(n) (get_dr (n))
static double
get_dr (n)
     int n;
{
  n = (n & ~1);
  if (host_little_endian)
    {
      union
      {
	int i[2];
	double d;
      } dr;
      dr.i[1] = saved_state.asregs.fregs[0].i[n + 0];
      dr.i[0] = saved_state.asregs.fregs[0].i[n + 1];
      return dr.d;
    }
  else
    return (saved_state.asregs.fregs[0].d[n >> 1]);
}

#define SET_DR(n, EXP) set_dr ((n), (EXP))
static void
set_dr (n, exp)
     int n;
     double exp;
{
  n = (n & ~1);
  if (host_little_endian)
    {
      union
      {
	int i[2];
	double d;
      } dr;
      dr.d = exp;
      saved_state.asregs.fregs[0].i[n + 0] = dr.i[1];
      saved_state.asregs.fregs[0].i[n + 1] = dr.i[0];
    }
  else
    saved_state.asregs.fregs[0].d[n >> 1] = exp;
}

#define SET_FI(n,EXP) (saved_state.asregs.fregs[0].i[(n)] = (EXP))
#define FI(n) (saved_state.asregs.fregs[0].i[(n)])

#define FR(n) (saved_state.asregs.fregs[0].f[(n)])
#define SET_FR(n,EXP) (saved_state.asregs.fregs[0].f[(n)] = (EXP))

#define XD_TO_XF(n) ((((n) & 1) << 5) | ((n) & 0x1e))
#define XF(n) (saved_state.asregs.fregs[(n) >> 5].i[(n) & 0x1f])
#define SET_XF(n,EXP) (saved_state.asregs.fregs[(n) >> 5].i[(n) & 0x1f] = (EXP))


#define FP_OP(n, OP, m) \
{ \
  if (FPSCR_PR) \
    { \
      if (((n) & 1) || ((m) & 1)) \
	saved_state.asregs.exception = SIGILL; \
      else \
	SET_DR(n, (DR(n) OP DR(m))); \
    } \
  else \
    SET_FR(n, (FR(n) OP FR(m))); \
} while (0)

#define FP_UNARY(n, OP) \
{ \
  if (FPSCR_PR) \
    { \
      if ((n) & 1) \
	saved_state.asregs.exception = SIGILL; \
      else \
	SET_DR(n, (OP (DR(n)))); \
    } \
  else \
    SET_FR(n, (OP (FR(n)))); \
} while (0)

#define FP_CMP(n, OP, m) \
{ \
  if (FPSCR_PR) \
    { \
      if (((n) & 1) || ((m) & 1)) \
	saved_state.asregs.exception = SIGILL; \
      else \
	SET_SR_T (DR(n) OP DR(m)); \
    } \
  else \
    SET_SR_T (FR(n) OP FR(m)); \
} while (0)



static void INLINE 
wlat_little (memory, x, value, maskl)
     unsigned char *memory;
{
  int v = value;
  unsigned char *p = memory + ((x) & maskl);
  BUSERROR(x, maskl, 32, v);
  p[3] = v >> 24;
  p[2] = v >> 16;
  p[1] = v >> 8;
  p[0] = v;
}

static void INLINE 
wwat_little (memory, x, value, maskw)
     unsigned char *memory;
{
  int v = value;
  unsigned char *p = memory + ((x) & maskw);
  BUSERROR(x, maskw, 16, v);

  p[1] = v >> 8;
  p[0] = v;
}

static void INLINE 
wbat_any (memory, x, value, maskb)
     unsigned char *memory;
{
  unsigned char *p = memory + (x & maskb);
  if (x > 0x5000000)
    IOMEM (x, 1, value);
  BUSERROR(x, maskb, 8, value);

  p[0] = value;
}

static void INLINE 
wlat_big (memory, x, value, maskl)
     unsigned char *memory;
{
  int v = value;
  unsigned char *p = memory + ((x) & maskl);
  BUSERROR(x, maskl, 32, v);

  p[0] = v >> 24;
  p[1] = v >> 16;
  p[2] = v >> 8;
  p[3] = v;
}

static void INLINE 
wwat_big (memory, x, value, maskw)
     unsigned char *memory;
{
  int v = value;
  unsigned char *p = memory + ((x) & maskw);
  BUSERROR(x, maskw, 16, v);

  p[0] = v >> 8;
  p[1] = v;
}

static void INLINE 
wbat_big (memory, x, value, maskb)
     unsigned char *memory;
{
  unsigned char *p = memory + (x & maskb);
  BUSERROR(x, maskb, 8, value);

  if (x > 0x5000000)
    IOMEM (x, 1, value);
  p[0] = value;
}

/* Read functions */

static int INLINE 
rlat_little (memory, x, maskl)
     unsigned char *memory;
{
  unsigned char *p = memory + ((x) & maskl);
  BUSERROR(x, maskl, -32, -1);

  return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}

static int INLINE 
rwat_little (memory, x, maskw)
     unsigned char *memory;
{
  unsigned char *p = memory + ((x) & maskw);
  BUSERROR(x, maskw, -16, -1);

  return (p[1] << 8) | p[0];
}

static int INLINE 
rbat_any (memory, x, maskb)
     unsigned char *memory;
{
  unsigned char *p = memory + ((x) & maskb);
  BUSERROR(x, maskb, -8, -1);

  return p[0];
}

static int INLINE 
rlat_big (memory, x, maskl)
     unsigned char *memory;
{
  unsigned char *p = memory + ((x) & maskl);
  BUSERROR(x, maskl, -32, -1);

  return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static int INLINE 
rwat_big (memory, x, maskw)
     unsigned char *memory;
{
  unsigned char *p = memory + ((x) & maskw);
  BUSERROR(x, maskw, -16, -1);

  return (p[0] << 8) | p[1];
}

#define RWAT(x) 	(little_endian ? rwat_little(memory, x, maskw): rwat_big(memory, x, maskw))
#define RLAT(x) 	(little_endian ? rlat_little(memory, x, maskl): rlat_big(memory, x, maskl))
#define RBAT(x)         (rbat_any (memory, x, maskb))
#define WWAT(x,v) 	(little_endian ? wwat_little(memory, x, v, maskw): wwat_big(memory, x, v, maskw))
#define WLAT(x,v) 	(little_endian ? wlat_little(memory, x, v, maskl): wlat_big(memory, x, v, maskl))
#define WBAT(x,v)       (wbat_any (memory, x, v, maskb))

#define RUWAT(x)  (RWAT(x) & 0xffff)
#define RSWAT(x)  ((short)(RWAT(x)))
#define RSBAT(x)  (SEXT(RBAT(x)))

#define RDAT(x, n) (do_rdat (memory, (x), (n), (little_endian)))
static int
do_rdat (memory, x, n, little_endian)
     char *memory;
     int x;
     int n;
     int little_endian;
{
  int f0;
  int f1;
  int i = (n & 1);
  int j = (n & ~1);
  if (little_endian)
    {
      f0 = rlat_little (memory, x + 0, maskl);
      f1 = rlat_little (memory, x + 4, maskl);
    }
  else
    {
      f0 = rlat_big (memory, x + 0, maskl);
      f1 = rlat_big (memory, x + 4, maskl);
    }
  saved_state.asregs.fregs[i].i[(j + 0)] = f0;
  saved_state.asregs.fregs[i].i[(j + 1)] = f1;
  return 0;
}

#define WDAT(x, n) (do_wdat (memory, (x), (n), (little_endian)))
static int
do_wdat (memory, x, n, little_endian)
     char *memory;
     int x;
     int n;
     int little_endian;
{
  int f0;
  int f1;
  int i = (n & 1);
  int j = (n & ~1);
  f0 = saved_state.asregs.fregs[i].i[(j + 0)];
  f1 = saved_state.asregs.fregs[i].i[(j + 1)];
  if (little_endian)
    {
      wlat_little (memory, (x + 0), f0, maskl);
      wlat_little (memory, (x + 4), f1, maskl);
    }
  else
    {
      wlat_big (memory, (x + 0), f0, maskl);
      wlat_big (memory, (x + 4), f1, maskl);
    }
  return 0;
}


#define MA(n) do { memstalls += (((pc & 3) != 0) ? (n) : ((n) - 1)); } while (0)

#define SEXT(x)     	(((x &  0xff) ^ (~0x7f))+0x80)
#define SEXT12(x)	(((x & 0xfff) ^ 0x800) - 0x800)
#define SEXTW(y)    	((int)((short)y))

#define Delay_Slot(TEMPPC)  	iword = RUWAT(TEMPPC); goto top;

int empty[16];

#define L(x)   thislock = x;
#define TL(x)  if ((x) == prevlock) stalls++;
#define TB(x,y)  if ((x) == prevlock || (y)==prevlock) stalls++;

#if defined(__GO32__) || defined(_WIN32)
int sim_memory_size = 19;
#else
int sim_memory_size = 24;
#endif

static int sim_profile_size = 17;
static int nsamples;

#undef TB
#define TB(x,y)

#define SMR1 (0x05FFFEC8)	/* Channel 1  serial mode register */
#define BRR1 (0x05FFFEC9)	/* Channel 1  bit rate register */
#define SCR1 (0x05FFFECA)	/* Channel 1  serial control register */
#define TDR1 (0x05FFFECB)	/* Channel 1  transmit data register */
#define SSR1 (0x05FFFECC)	/* Channel 1  serial status register */
#define RDR1 (0x05FFFECD)	/* Channel 1  receive data register */

#define SCI_RDRF  	 0x40	/* Recieve data register full */
#define SCI_TDRE	0x80	/* Transmit data register empty */

static int
IOMEM (addr, write, value)
     int addr;
     int write;
     int value;
{
  if (write)
    {
      switch (addr)
	{
	case TDR1:
	  if (value != '\r')
	    {
	      putchar (value);
	      fflush (stdout);
	    }
	  break;
	}
    }
  else
    {
      switch (addr)
	{
	case RDR1:
	  return getchar ();
	}
    }
  return 0;
}

static int
get_now ()
{
  return time ((long *) 0);
}

static int
now_persec ()
{
  return 1;
}

static FILE *profile_file;

static void
swap (memory, n)
     unsigned char *memory;
     int n;
{
  int little_endian = target_little_endian;
  WLAT (0, n);
}

static void
swap16 (memory, n)
     unsigned char *memory;
     int n;
{
  int little_endian = target_little_endian;
  WWAT (0, n);
}

static void
swapout (n)
     int n;
{
  if (profile_file)
    {
      char b[4];
      swap (b, n);
      fwrite (b, 4, 1, profile_file);
    }
}

static void
swapout16 (n)
     int n;
{
  char b[4];
  swap16 (b, n);
  fwrite (b, 2, 1, profile_file);
}

/* Turn a pointer in a register into a pointer into real memory. */

static char *
ptr (x)
     int x;
{
  return (char *) (x + saved_state.asregs.memory);
}

/* Simulate a monitor trap, put the result into r0 and errno into r1 */

static void
trap (i, regs, memory, maskl, maskw, little_endian)
     int i;
     int *regs;
     unsigned char *memory;
{
  switch (i)
    {
    case 1:
      printf ("%c", regs[0]);
      break;
    case 2:
      saved_state.asregs.exception = SIGQUIT;
      break;
    case 3:			/* FIXME: for backwards compat, should be removed */
    case 34:
      {
	extern int errno;
	int perrno = errno;
	errno = 0;

	switch (regs[4])
	  {

#if !defined(__GO32__) && !defined(_WIN32)
	  case SYS_fork:
	    regs[0] = fork ();
	    break;
	  case SYS_execve:
	    regs[0] = execve (ptr (regs[5]), (char **)ptr (regs[6]), (char **)ptr (regs[7]));
	    break;
	  case SYS_execv:
	    regs[0] = execve (ptr (regs[5]),(char **) ptr (regs[6]), 0);
	    break;
	  case SYS_pipe:
	    {
	      char *buf;
	      int host_fd[2];

	      buf = ptr (regs[5]);

	      regs[0] = pipe (host_fd);

	      WLAT (buf, host_fd[0]);
	      buf += 4;
	      WLAT (buf, host_fd[1]);
	    }
	    break;

	  case SYS_wait:
	    regs[0] = wait (ptr (regs[5]));
	    break;
#endif

	  case SYS_read:
	    regs[0] = callback->read (callback, regs[5], ptr (regs[6]), regs[7]);
	    break;
	  case SYS_write:
	    if (regs[5] == 1)
	      regs[0] = (int)callback->write_stdout (callback, ptr(regs[6]), regs[7]);
	    else
	      regs[0] = (int)callback->write (callback, regs[5], ptr (regs[6]), regs[7]);
	    break;
	  case SYS_lseek:
	    regs[0] = callback->lseek (callback,regs[5], regs[6], regs[7]);
	    break;
	  case SYS_close:
	    regs[0] = callback->close (callback,regs[5]);
	    break;
	  case SYS_open:
	    regs[0] = callback->open (callback,ptr (regs[5]), regs[6]);
	    break;
	  case SYS_exit:
	    /* EXIT - caller can look in r5 to work out the reason */
	    saved_state.asregs.exception = SIGQUIT;
	    regs[0] = regs[5];
	    break;

	  case SYS_stat:	/* added at hmsi */
	    /* stat system call */
	    {
	      struct stat host_stat;
	      char *buf;

	      regs[0] = stat (ptr (regs[5]), &host_stat);

	      buf = ptr (regs[6]);

	      WWAT (buf, host_stat.st_dev);
	      buf += 2;
	      WWAT (buf, host_stat.st_ino);
	      buf += 2;
	      WLAT (buf, host_stat.st_mode);
	      buf += 4;
	      WWAT (buf, host_stat.st_nlink);
	      buf += 2;
	      WWAT (buf, host_stat.st_uid);
	      buf += 2;
	      WWAT (buf, host_stat.st_gid);
	      buf += 2;
	      WWAT (buf, host_stat.st_rdev);
	      buf += 2;
	      WLAT (buf, host_stat.st_size);
	      buf += 4;
	      WLAT (buf, host_stat.st_atime);
	      buf += 4;
	      WLAT (buf, 0);
	      buf += 4;
	      WLAT (buf, host_stat.st_mtime);
	      buf += 4;
	      WLAT (buf, 0);
	      buf += 4;
	      WLAT (buf, host_stat.st_ctime);
	      buf += 4;
	      WLAT (buf, 0);
	      buf += 4;
	      WLAT (buf, 0);
	      buf += 4;
	      WLAT (buf, 0);
	      buf += 4;
	    }
	    break;

#ifndef _WIN32
	  case SYS_chown:
	    regs[0] = chown (ptr (regs[5]), regs[6], regs[7]);
	    break;
#endif /* _WIN32 */
	  case SYS_chmod:
	    regs[0] = chmod (ptr (regs[5]), regs[6]);
	    break;
	  case SYS_utime:
	    /* Cast the second argument to void *, to avoid type mismatch
	       if a prototype is present.  */
	    regs[0] = utime (ptr (regs[5]), (void *) ptr (regs[6]));
	    break;
	  default:
	    abort ();
	  }
	regs[1] = callback->get_errno (callback);
	errno = perrno;
      }
      break;

    case 0xc3:
    case 255:
      saved_state.asregs.exception = SIGTRAP;
      break;
    }

}

void
control_c (sig, code, scp, addr)
     int sig;
     int code;
     char *scp;
     char *addr;
{
  saved_state.asregs.exception = SIGINT;
}

static int
div1 (R, iRn2, iRn1/*, T*/)
     int *R;
     int iRn1;
     int iRn2;
     /* int T;*/
{
  unsigned long tmp0;
  unsigned char old_q, tmp1;

  old_q = Q;
  SET_SR_Q ((unsigned char) ((0x80000000 & R[iRn1]) != 0));
  R[iRn1] <<= 1;
  R[iRn1] |= (unsigned long) T;

  switch (old_q)
    {
    case 0:
      switch (M)
	{
	case 0:
	  tmp0 = R[iRn1];
	  R[iRn1] -= R[iRn2];
	  tmp1 = (R[iRn1] > tmp0);
	  switch (Q)
	    {
	    case 0:
	      SET_SR_Q (tmp1);
	      break;
	    case 1:
	      SET_SR_Q ((unsigned char) (tmp1 == 0));
	      break;
	    }
	  break;
	case 1:
	  tmp0 = R[iRn1];
	  R[iRn1] += R[iRn2];
	  tmp1 = (R[iRn1] < tmp0);
	  switch (Q)
	    {
	    case 0:
	      SET_SR_Q ((unsigned char) (tmp1 == 0));
	      break;
	    case 1:
	      SET_SR_Q (tmp1);
	      break;
	    }
	  break;
	}
      break;
    case 1:
      switch (M)
	{
	case 0:
	  tmp0 = R[iRn1];
	  R[iRn1] += R[iRn2];
	  tmp1 = (R[iRn1] < tmp0);
	  switch (Q)
	    {
	    case 0:
	      SET_SR_Q (tmp1);
	      break;
	    case 1:
	      SET_SR_Q ((unsigned char) (tmp1 == 0));
	      break;
	    }
	  break;
	case 1:
	  tmp0 = R[iRn1];
	  R[iRn1] -= R[iRn2];
	  tmp1 = (R[iRn1] > tmp0);
	  switch (Q)
	    {
	    case 0:
	      SET_SR_Q ((unsigned char) (tmp1 == 0));
	      break;
	    case 1:
	      SET_SR_Q (tmp1);
	      break;
	    }
	  break;
	}
      break;
    }
  /*T = (Q == M);*/
  SET_SR_T (Q == M);
  /*return T;*/
}

static void
dmul (sign, rm, rn)
     int sign;
     unsigned int rm;
     unsigned int rn;
{
  unsigned long RnL, RnH;
  unsigned long RmL, RmH;
  unsigned long temp0, temp1, temp2, temp3;
  unsigned long Res2, Res1, Res0;

  RnL = rn & 0xffff;
  RnH = (rn >> 16) & 0xffff;
  RmL = rm & 0xffff;
  RmH = (rm >> 16) & 0xffff;
  temp0 = RmL * RnL;
  temp1 = RmH * RnL;
  temp2 = RmL * RnH;
  temp3 = RmH * RnH;
  Res2 = 0;
  Res1 = temp1 + temp2;
  if (Res1 < temp1)
    Res2 += 0x00010000;
  temp1 = (Res1 << 16) & 0xffff0000;
  Res0 = temp0 + temp1;
  if (Res0 < temp0)
    Res2 += 1;
  Res2 += ((Res1 >> 16) & 0xffff) + temp3;
  
  if (sign)
    {
      if (rn & 0x80000000)
	Res2 -= rm;
      if (rm & 0x80000000)
	Res2 -= rn;
    }

  MACH = Res2;
  MACL = Res0;
}

static void
macw (regs, memory, n, m)
     int *regs;
     unsigned char *memory;
     int m, n;
{
  int little_endian = target_little_endian;
  long tempm, tempn;
  long prod, macl, sum;

  tempm=RSWAT(regs[m]); regs[m]+=2;
  tempn=RSWAT(regs[n]); regs[n]+=2;

  macl = MACL;
  prod = (long)(short) tempm * (long)(short) tempn;
  sum = prod + macl;
  if (S)
    {
      if ((~(prod ^ macl) & (sum ^ prod)) < 0)
	{
	  /* MACH's lsb is a sticky overflow bit.  */
	  MACH |= 1;
	  /* Store the smallest negative number in MACL if prod is
	     negative, and the largest positive number otherwise.  */
	  sum = 0x7fffffff + (prod < 0);
	}
    }
  else
    {
      long mach;
      /* Add to MACH the sign extended product, and carry from low sum.  */
      mach = MACH + (-(prod < 0)) + ((unsigned long) sum < prod);
      /* Sign extend at 10:th bit in MACH.  */
      MACH = (mach & 0x1ff) | -(mach & 0x200);
    }
  MACL = sum;
}

/* Set the memory size to the power of two provided. */

void
sim_size (power)
     int power;

{
  saved_state.asregs.msize = 1 << power;

  sim_memory_size = power;

  if (saved_state.asregs.memory)
    {
      free (saved_state.asregs.memory);
    }

  saved_state.asregs.memory =
    (unsigned char *) calloc (64, saved_state.asregs.msize / 64);

  if (!saved_state.asregs.memory)
    {
      fprintf (stderr,
	       "Not enough VM for simulation of %d bytes of RAM\n",
	       saved_state.asregs.msize);

      saved_state.asregs.msize = 1;
      saved_state.asregs.memory = (unsigned char *) calloc (1, 1);
    }
}

static void
init_pointers ()
{
  host_little_endian = 0;
  *(char*)&host_little_endian = 1;
  host_little_endian &= 1;

  if (saved_state.asregs.msize != 1 << sim_memory_size)
    {
      sim_size (sim_memory_size);
    }

  if (saved_state.asregs.profile && !profile_file)
    {
      profile_file = fopen ("gmon.out", "wb");
      /* Seek to where to put the call arc data */
      nsamples = (1 << sim_profile_size);

      fseek (profile_file, nsamples * 2 + 12, 0);

      if (!profile_file)
	{
	  fprintf (stderr, "Can't open gmon.out\n");
	}
      else
	{
	  saved_state.asregs.profile_hist =
	    (unsigned short *) calloc (64, (nsamples * sizeof (short) / 64));
	}
    }
}

static void
dump_profile ()
{
  unsigned int minpc;
  unsigned int maxpc;
  unsigned short *p;
  int i;

  p = saved_state.asregs.profile_hist;
  minpc = 0;
  maxpc = (1 << sim_profile_size);

  fseek (profile_file, 0L, 0);
  swapout (minpc << PROFILE_SHIFT);
  swapout (maxpc << PROFILE_SHIFT);
  swapout (nsamples * 2 + 12);
  for (i = 0; i < nsamples; i++)
    swapout16 (saved_state.asregs.profile_hist[i]);

}

static void
gotcall (from, to)
     int from;
     int to;
{
  swapout (from);
  swapout (to);
  swapout (1);
}

#define MMASKB ((saved_state.asregs.msize -1) & ~0)

int
sim_stop (sd)
     SIM_DESC sd;
{
  saved_state.asregs.exception = SIGINT;
  return 1;
}

void
sim_resume (sd, step, siggnal)
     SIM_DESC sd;
     int step, siggnal;
{
  register unsigned int pc;
  register int cycles = 0;
  register int stalls = 0;
  register int memstalls = 0;
  register int insts = 0;
  register int prevlock;
  register int thislock;
  register unsigned int doprofile;
  register int pollcount = 0;
  register int little_endian = target_little_endian;

  int tick_start = get_now ();
  void (*prev) ();
  void (*prev_fpe) ();
  extern unsigned char sh_jump_table0[];

  register unsigned char *jump_table = sh_jump_table0;

  register int *R = &(saved_state.asregs.regs[0]);
  /*register int T;*/
  register int PR;

  register int maskb = ((saved_state.asregs.msize - 1) & ~0);
  register int maskw = ((saved_state.asregs.msize - 1) & ~1);
  register int maskl = ((saved_state.asregs.msize - 1) & ~3);
  register unsigned char *memory;
  register unsigned int sbit = ((unsigned int) 1 << 31);

  prev = signal (SIGINT, control_c);
  prev_fpe = signal (SIGFPE, SIG_IGN);

  init_pointers ();

  memory = saved_state.asregs.memory;

  if (step)
    {
      saved_state.asregs.exception = SIGTRAP;
    }
  else
    {
      saved_state.asregs.exception = 0;
    }

  pc = saved_state.asregs.pc;
  PR = saved_state.asregs.pr;
  /*T = GET_SR () & SR_MASK_T;*/
  prevlock = saved_state.asregs.prevlock;
  thislock = saved_state.asregs.thislock;
  doprofile = saved_state.asregs.profile;

  /* If profiling not enabled, disable it by asking for
     profiles infrequently. */
  if (doprofile == 0)
    doprofile = ~0;

  do
    {
      register unsigned int iword = RUWAT (pc);
      register unsigned int ult;
      register unsigned int nia = pc + 2;
#ifndef ACE_FAST
      insts++;
#endif
    top:

#include "code.c"


      pc = nia;

      if (--pollcount < 0)
	{
	  pollcount = POLL_QUIT_INTERVAL;
	  if ((*callback->poll_quit) != NULL
	      && (*callback->poll_quit) (callback))
	    {
	      sim_stop (sd);
	    }	    
	}

#ifndef ACE_FAST
      prevlock = thislock;
      thislock = 30;
      cycles++;

      if (cycles >= doprofile)
	{

	  saved_state.asregs.cycles += doprofile;
	  cycles -= doprofile;
	  if (saved_state.asregs.profile_hist)
	    {
	      int n = pc >> PROFILE_SHIFT;
	      if (n < nsamples)
		{
		  int i = saved_state.asregs.profile_hist[n];
		  if (i < 65000)
		    saved_state.asregs.profile_hist[n] = i + 1;
		}

	    }
	}
#endif
    }
  while (!saved_state.asregs.exception);

  if (saved_state.asregs.exception == SIGILL
      || saved_state.asregs.exception == SIGBUS)
    {
      pc -= 2;
    }

  saved_state.asregs.ticks += get_now () - tick_start;
  saved_state.asregs.cycles += cycles;
  saved_state.asregs.stalls += stalls;
  saved_state.asregs.memstalls += memstalls;
  saved_state.asregs.insts += insts;
  saved_state.asregs.pc = pc;
  /* restore the T and other cached SR bits */
  SET_SR (GET_SR());
  saved_state.asregs.pr = PR;

  saved_state.asregs.prevlock = prevlock;
  saved_state.asregs.thislock = thislock;

  if (profile_file)
    {
      dump_profile ();
    }

  signal (SIGFPE, prev_fpe);
  signal (SIGINT, prev);
}

int
sim_write (sd, addr, buffer, size)
     SIM_DESC sd;
     SIM_ADDR addr;
     unsigned char *buffer;
     int size;
{
  int i;

  init_pointers ();

  for (i = 0; i < size; i++)
    {
      saved_state.asregs.memory[MMASKB & (addr + i)] = buffer[i];
    }
  return size;
}

int
sim_read (sd, addr, buffer, size)
     SIM_DESC sd;
     SIM_ADDR addr;
     unsigned char *buffer;
     int size;
{
  int i;

  init_pointers ();

  for (i = 0; i < size; i++)
    {
      buffer[i] = saved_state.asregs.memory[MMASKB & (addr + i)];
    }
  return size;
}

/* We have to add one to RN as an index into asints because of the padding
   added at the start of asregs.  */
int
sim_store_register (sd, rn, memory, length)
     SIM_DESC sd;
     int rn;
     unsigned char *memory;
     int length;
{
  int little_endian;
  init_pointers ();
  little_endian = target_little_endian;
  if (&saved_state.asints[rn+1]
      == &saved_state.asregs.fpscr)
    set_fpscr1 (RLAT(0));
  else
    saved_state.asints[rn+1] = RLAT(0);
  return -1;
}

int
sim_fetch_register (sd, rn, memory, length)
     SIM_DESC sd;
     int rn;
     unsigned char *memory;
     int length;
{
  int little_endian;
  init_pointers ();
  little_endian = target_little_endian;
  WLAT (0, saved_state.asints[rn+1]);
  return -1;
}

int
sim_trace (sd)
     SIM_DESC sd;
{
  return 0;
}

void
sim_stop_reason (sd, reason, sigrc)
     SIM_DESC sd;
     enum sim_stop *reason;
     int *sigrc;
{
  /* The SH simulator uses SIGQUIT to indicate that the program has
     exited, so we must check for it here and translate it to exit.  */
  if (saved_state.asregs.exception == SIGQUIT)
    {
      *reason = sim_exited;
      *sigrc = saved_state.asregs.regs[5];
    }
  else
    {
      *reason = sim_stopped;
      *sigrc = saved_state.asregs.exception;
    }
}

void
sim_info (sd, verbose)
     SIM_DESC sd;
     int verbose;
{
  double timetaken = (double) saved_state.asregs.ticks / (double) now_persec ();
  double virttime = saved_state.asregs.cycles / 36.0e6;

  callback->printf_filtered (callback, "\n\n# instructions executed  %10d\n", 
			     saved_state.asregs.insts);
  callback->printf_filtered (callback, "# cycles                 %10d\n",
			     saved_state.asregs.cycles);
  callback->printf_filtered (callback, "# pipeline stalls        %10d\n",
			     saved_state.asregs.stalls);
  callback->printf_filtered (callback, "# misaligned load/store  %10d\n",
			     saved_state.asregs.memstalls);
  callback->printf_filtered (callback, "# real time taken        %10.4f\n",
			     timetaken);
  callback->printf_filtered (callback, "# virtual time taken     %10.4f\n",
			     virttime);
  callback->printf_filtered (callback, "# profiling size         %10d\n",
			     sim_profile_size);
  callback->printf_filtered (callback, "# profiling frequency    %10d\n",
			     saved_state.asregs.profile);
  callback->printf_filtered (callback, "# profile maxpc          %10x\n",
			     (1 << sim_profile_size) << PROFILE_SHIFT);

  if (timetaken != 0)
    {
      callback->printf_filtered (callback, "# cycles/second          %10d\n", 
				 (int) (saved_state.asregs.cycles / timetaken));
      callback->printf_filtered (callback, "# simulation ratio       %10.4f\n", 
				 virttime / timetaken);
    }
}

void
sim_set_profile (n)
     int n;
{
  saved_state.asregs.profile = n;
}

void
sim_set_profile_size (n)
     int n;
{
  sim_profile_size = n;
}

SIM_DESC
sim_open (kind, cb, abfd, argv)
     SIM_OPEN_KIND kind;
     host_callback *cb;
     struct _bfd *abfd;
     char **argv;
{
  char **p;
  int endian_set = 0;

  sim_kind = kind;
  myname = argv[0];
  callback = cb;

  for (p = argv + 1; *p != NULL; ++p)
    {
      if (strcmp (*p, "-E") == 0)
	{
	  ++p;
	  if (*p == NULL)
	    {
	      /* FIXME: This doesn't use stderr, but then the rest of the
		 file doesn't either.  */
	      callback->printf_filtered (callback, "Missing argument to `-E'.\n");
	      return 0;
	    }
	  target_little_endian = strcmp (*p, "big") != 0;
          endian_set = 1;
	}
      else if (isdigit (**p))
	parse_and_set_memory_size (*p);
    }

  if (abfd != NULL && ! endian_set)
      target_little_endian = ! bfd_big_endian (abfd);

  /* fudge our descriptor for now */
  return (SIM_DESC) 1;
}

static void
parse_and_set_memory_size (str)
     char *str;
{
  int n;

  n = strtol (str, NULL, 10);
  if (n > 0 && n <= 24)
    sim_memory_size = n;
  else
    callback->printf_filtered (callback, "Bad memory size %d; must be 1 to 24, inclusive\n", n);
}

void
sim_close (sd, quitting)
     SIM_DESC sd;
     int quitting;
{
  /* nothing to do */
}

SIM_RC
sim_load (sd, prog, abfd, from_tty)
     SIM_DESC sd;
     char *prog;
     bfd *abfd;
     int from_tty;
{
  extern bfd *sim_load_file (); /* ??? Don't know where this should live.  */
  bfd *prog_bfd;

  prog_bfd = sim_load_file (sd, myname, callback, prog, abfd,
			    sim_kind == SIM_OPEN_DEBUG,
			    0, sim_write);
  if (prog_bfd == NULL)
    return SIM_RC_FAIL;
  if (abfd == NULL)
    bfd_close (prog_bfd);
  return SIM_RC_OK;
}

SIM_RC
sim_create_inferior (sd, prog_bfd, argv, env)
     SIM_DESC sd;
     struct _bfd *prog_bfd;
     char **argv;
     char **env;
{
  /* clear the registers */
  memset (&saved_state, 0,
	  (char*)&saved_state.asregs.end_of_registers - (char*)&saved_state);
  /* set the PC */
  if (prog_bfd != NULL)
    saved_state.asregs.pc = bfd_get_start_address (prog_bfd);
  return SIM_RC_OK;
}

void
sim_do_command (sd, cmd)
     SIM_DESC sd;
     char *cmd;
{
  char *sms_cmd = "set-memory-size";
  int cmdsize;

  if (cmd == NULL || *cmd == '\0')
    {
      cmd = "help";
    }

  cmdsize = strlen (sms_cmd);
  if (strncmp (cmd, sms_cmd, cmdsize) == 0 && strchr (" \t", cmd[cmdsize]) != NULL)
    {
      parse_and_set_memory_size (cmd + cmdsize + 1);
    }
  else if (strcmp (cmd, "help") == 0)
    {
      (callback->printf_filtered) (callback, "List of SH simulator commands:\n\n");
      (callback->printf_filtered) (callback, "set-memory-size <n> -- Set the number of address bits to use\n");
      (callback->printf_filtered) (callback, "\n");
    }
  else
    {
      (callback->printf_filtered) (callback, "Error: \"%s\" is not a valid SH simulator command.\n", cmd);
    }
}

void
sim_set_callbacks (p)
     host_callback *p;
{
  callback = p;
}
