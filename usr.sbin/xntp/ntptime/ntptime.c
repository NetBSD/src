/*
 * NTP test program
 *
 * This program tests to see if the NTP user interface routines
 * ntp_gettime() and ntp_adjtime() have been implemented in the kernel.
 * If so, each of these routines is called to display current timekeeping
 * data.
 *
 * For more information, see the README.kern file in the doc directory
 * of the xntp3 distribution.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef KERNEL_PLL
#include <stdio.h>
#include <ctype.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include "ntp_fp.h"
#include "ntp_unixtime.h"
#include "ntp_stdlib.h"

#ifndef	SYS_DECOSF1
# define BADCALL -1		/* this is supposed to be a bad syscall */
#endif /* SYS_DECOSF1 */

#ifdef KERNEL_PLL
# include <sys/timex.h>
# ifdef NTP_SYSCALLS_STD
#  define ntp_gettime(t)  syscall(SYS_ntp_gettime, (t))
#  define ntp_adjtime(t)  syscall(SYS_ntp_adjtime, (t))
# else /* NOT NTP_SYSCALLS_STD */
#  ifdef HAVE___NTP_GETTIME
#   define ntp_gettime(t)  __ntp_gettime((t))
#  endif
#  ifdef HAVE___ADJTIMEX
#   define ntp_adjtime(t)  __adjtimex((t))
#  endif
# endif /* NOT NTP_SYSCALLS_STD */
#endif /* KERNEL_PLL */

#ifdef NTP_SYSCALLS_STD
# ifdef DECL_SYSCALL
extern int syscall      P((int, void *, ...));
# endif	/* DECL_SYSCALL */
#endif /* NTP_SYSCALLS_STD */

#define TIMEX_MOD_BITS \
"\20\1OFFSET\2FREQUENCY\3MAXERROR\4ESTERROR\5STATUS\6TIMECONST\
\17CLKB\20CLKA"
 
#define TIMEX_STA_BITS \
"\20\1PLL\2PPSFREQ\3PPSTIME\4FLL\5INS\6DEL\7UNSYNC\10FREQHOLD\
\11PPSSIGNAL\12PPSJITTER\13PPSWANDER\14PPSERROR\15CLOCKERR"

/*
 * Function prototypes
 */
#ifndef NTP_SYSCALLS_LIBC
# ifdef DECL_SYSCALL
extern int syscall	P((int, void *, ...));
# endif /* DECL_SYSCALL */
#endif /* NTP_SYSCALLS_LIBC */
char *sprintb		P((u_int, char *));
char *timex_state	P((int));

#ifdef SIGSYS
void pll_trap		P((int));

static struct sigaction newsigsys;	/* new sigaction status */
static struct sigaction sigsys;	/* current sigaction status */
static sigjmp_buf env;		/* environment var. for pll_trap() */
#endif

static volatile int pll_control; /* (0) daemon, (1) kernel loop */

static char* progname;
static char optargs[] = "ce:f:hm:o:rs:t:";

void
main(argc, argv)
     int argc;
     char *argv[];
{
  extern int ntp_optind;
  extern char *ntp_optarg;
  int status;
  struct ntptimeval ntv;
  struct timex ntx, _ntx;
  int	times[20];
  double ftemp, gtemp;
  l_fp ts;
  int c;
  int errflg	= 0;
  int cost	= 0;
  int rawtime	= 0;

  memset((char *)&ntx, 0, sizeof(ntx));
  progname = argv[0];
  while ((c = ntp_getopt(argc, argv, optargs)) != EOF) switch (c) {
  case 'c':
    cost++;
    break;
  case 'e':
    ntx.modes |= MOD_ESTERROR;
    ntx.esterror = atoi(ntp_optarg);
    break;
  case 'f':
    ntx.modes |= MOD_FREQUENCY;
    ntx.freq = (int) (atof(ntp_optarg) *
		      (1 << SHIFT_USEC));
    if (ntx.freq < (-100 << SHIFT_USEC)
	||  ntx.freq > ( 100 << SHIFT_USEC)) errflg++;
    break;
  case 'm':
    ntx.modes |= MOD_MAXERROR;
    ntx.maxerror = atoi(ntp_optarg);
    break;
  case 'o':
    ntx.modes |= MOD_OFFSET;
    ntx.offset = atoi(ntp_optarg);
    break;
  case 'r':
    rawtime++;
    break;
  case 's':
    ntx.modes |= MOD_STATUS;
    ntx.status = atoi(ntp_optarg);
    if (ntx.status < 0 || ntx.status > 4) errflg++;
    break;
  case 't':
    ntx.modes |= MOD_TIMECONST;
    ntx.constant = atoi(ntp_optarg);
    if (ntx.constant < 0 || ntx.constant > MAXTC)
      errflg++;
    break;
  default:
    errflg++;
  }
  if (errflg || (ntp_optind != argc)) {
    (void) fprintf(stderr,
		   "usage: %s [-%s]\n\n\
-c		display the time taken to call ntp_gettime (us)\n\
-e esterror	estimate of the error (us)\n\
-f frequency	Frequency error (-100 .. 100) (ppm)\n\
-h		display this help info\n\
-m maxerror	max possible error (us)\n\
-o offset	current offset (ms)\n\
-r		print the unix and NTP time raw\n\
-l leap		Set the leap bits\n\
-t timeconstant	log2 of PLL time constant (0 .. %d)\n",
		   progname, optargs, MAXTC);
    exit(2);
  }

#ifdef SIGSYS
  /*
   * Test to make sure the sigaction() works in case of invalid
   * syscall codes.
   */
  newsigsys.sa_handler = pll_trap;
  newsigsys.sa_flags = 0;
  if (sigaction(SIGSYS, &newsigsys, &sigsys)) {
    perror("sigaction() fails to save SIGSYS trap");
    exit(1);
  }
#endif /* SIGSYS */

#ifdef	BADCALL
  /*
   * Make sure the trapcatcher works.
   */
  pll_control = 1;
#ifdef SIGSYS
  if (sigsetjmp(env, 1) == 0)
    {
#endif
      status = syscall(BADCALL, &ntv); /* dummy parameter */
      if ((status < 0) && (errno == ENOSYS))
	{
	  --pll_control;
	}
#ifdef SIGSYS
    }
#endif
  if (pll_control)
    printf("sigaction() failed to catch an invalid syscall\n");
#endif /* BADCALL */

  if (cost) {
    for (c = 0; c < sizeof times / sizeof times[0]; c++)
      {
#ifdef SIGSYS
	if (sigsetjmp(env, 1) == 0)
	  {
#endif
	    status = ntp_gettime(&ntv);
	    if ((status < 0) && (errno == ENOSYS))
	      {
		--pll_control;
	      }
#ifdef SIGSYS
	  }
#endif
	if (pll_control < 0)
	  break;
	times[c] = ntv.time.tv_usec;
      }
    if (pll_control >= 0) {
      printf("[ us %06d:", times[0]);
      for (c = 1; c < sizeof times / sizeof times[0]; c++)
	printf(" %d", times[c] - times[c - 1]);
      printf(" ]\n");
    }
  }
#ifdef SIGSYS
  if (sigsetjmp(env, 1) == 0)
    {
#endif
      status = ntp_gettime(&ntv);
      if ((status < 0) && (errno == ENOSYS))
	{
	  --pll_control;
	}
#ifdef SIGSYS
    }
#endif
  _ntx.modes = 0;				/* Ensure nothing is set */
#ifdef SIGSYS
  if (sigsetjmp(env, 1) == 0)
    {
#endif
      status = ntp_adjtime(&_ntx);
      if ((status < 0) && (errno == ENOSYS))
	{
	  --pll_control;
	}
#ifdef SIGSYS
    }
#endif
  if (pll_control < 0) {
    printf("NTP user interface routines are not configured in this kernel.\n");
    goto lexit;
  }

  /*
   * Fetch timekeeping data and display.
   */
  status = ntp_gettime(&ntv);
  if (status < 0)
    perror("ntp_gettime() call fails");
  else {
    printf("ntp_gettime() returns code %d (%s)\n",
	   status, timex_state(status));
    TVTOTS(&ntv.time, &ts);
    ts.l_uf += TS_ROUNDBIT;		/* guaranteed not to overflow */
    ts.l_ui += JAN_1970;
    ts.l_uf &= TS_MASK;
    printf("  time %s, (.%06d),\n",
	   prettydate(&ts), (int) ntv.time.tv_usec);
    printf("  maximum error %ld us, estimated error %ld us.\n",
	   ntv.maxerror, ntv.esterror);
    if (rawtime) printf("  ntptime=%x.%x unixtime=%x.%06d %s",
			(unsigned int) ts.l_ui, (unsigned int) ts.l_uf,
			(int) ntv.time.tv_sec, (int) ntv.time.tv_usec,
			ctime((const time_t *) &ntv.time.tv_sec));
  }
  status = ntp_adjtime(&ntx);
  if (status < 0)
    perror((errno == EPERM) ? 
	   "Must be root to set kernel values\nntp_adjtime() call fails" :
	   "ntp_adjtime() call fails");
  else {
    printf("ntp_adjtime() returns code %d (%s)\n",
	   status, timex_state(status));
    ftemp = ntx.freq;
    ftemp /= (1 << SHIFT_USEC);
    printf("  modes %s,\n", sprintb(ntx.modes, TIMEX_MOD_BITS));
    printf("  offset %ld us, frequency %.3f ppm, interval %d s,\n",
	   ntx.offset, ftemp, 1 << ntx.shift);
    printf("  maximum error %ld us, estimated error %ld us,\n",
	   ntx.maxerror, ntx.esterror);
    ftemp = ntx.tolerance;
    ftemp /= (1 << SHIFT_USEC);
    printf("  status %s,\n", sprintb(ntx.status, TIMEX_STA_BITS));
    printf("  time constant %ld, precision %ld us, tolerance %.0f ppm,\n",
	   ntx.constant, ntx.precision, ftemp);
    if (ntx.shift == 0)
      return;
    ftemp = ntx.ppsfreq;
    ftemp /= (1 << SHIFT_USEC);
    gtemp = ntx.stabil;
    gtemp /= (1 << SHIFT_USEC);
    printf("  pps frequency %.3f ppm, stability %.3f ppm, jitter %ld us,\n",
	   ftemp, gtemp, ntx.jitter);
    printf("  intervals %ld, jitter exceeded %ld, stability exceeded %ld, errors %ld.\n",
	   ntx.calcnt, ntx.jitcnt, ntx.stbcnt, ntx.errcnt);
  }

  /*
   * Put things back together the way we found them.
   */
 lexit:
#ifdef SIGSYS
  if (sigaction(SIGSYS, &sigsys, (struct sigaction *)NULL)) {
    perror("sigaction() fails to restore SIGSYS trap");
    exit(1);
  }
#endif
  exit(0);
}

#ifdef SIGSYS
/*
 * pll_trap - trap processor for undefined syscalls
 */
void
pll_trap(arg)
     int arg;
{
  pll_control--;
  siglongjmp(env, 1);
}
#endif

/*
 * Print a value a la the %b format of the kernel's printf
 */
char *
sprintb(v, bits)
     register u_int v;
     register char *bits;
{
  register char *cp;
  register int i, any = 0;
  register char c;
  static char buf[132];

  if (bits && *bits == 8)
    (void)sprintf(buf, "0%o", v);
  else
    (void)sprintf(buf, "0x%x", v);
  cp = buf + strlen(buf);
  bits++;
  if (bits) {
    *cp++ = ' ';
    *cp++ = '(';
    while ((i = *bits++) != 0) {
      if (v & (1 << (i-1))) {
	if (any)
	  *cp++ = ',';
	any = 1;
	for (; (c = *bits) > 32; bits++)
	  *cp++ = c;
      } else
	for (; *bits > 32; bits++)
	  continue;
    }
    *cp++ = ')';
  }
  *cp = '\0';
  return (buf);
}

char *timex_states[] = {
  "OK", "INS", "DEL", "OOP", "WAIT", "ERROR"
};

char *
timex_state(s)
     register int s;
{
  static char buf[32];

  if (s >= 0 && s <= sizeof(timex_states) / sizeof(timex_states[0]))
    return (timex_states[s]);
  sprintf(buf, "TIME-#%d", s);
  return (buf);
}
#endif /* KERNEL_PLL */
