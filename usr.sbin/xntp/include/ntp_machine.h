/*
 * Collect all machine dependent idiosyncrasies in one place.
 */

#ifndef __ntp_machine
#define __ntp_machine

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*

			 HEY!  CHECK THIS OUT!

  The first half of this file is obsolete, and is only there to help
  reconcile "what went before" with "current behavior".

  The per-system SYS_* #defins ARE NO LONGER USED, with the temporary
  exception of SYS_WINNT.

  If you find a hunk of code that is bracketed by a SYS_* macro and you
  *know* that it is still needed, please let us know.  In many cases the
  code fragment is now handled somewhere else by autoconf choices.

*/

/*

INFO ON NEW KERNEL PLL SYS CALLS

  NTP_SYSCALLS_STD  - use the "normal" ones
  NTP_SYSCALL_GET   - SYS_ntp_gettime id
  NTP_SYSCALL_ADJ   - SYS_ntp_adjtime id
  NTP_SYSCALLS_LIBC - ntp_adjtime() and ntp_gettime() are in libc.

HOW TO GET IP INTERFACE INFORMATION

  Some UNIX V.4 machines implement a sockets library on top of
  streams. For these systems, you must use send the SIOCGIFCONF down
  the stream in an I_STR ioctl. This ususally also implies
  USE_STREAMS_DEVICE FOR IF_CONFIG. Dell UNIX is a notable exception.

  STREAMS_TLI - use ioctl(I_STR) to implement ioctl(SIOCGIFCONF)

WHAT DOES IOCTL(SIOCGIFCONF) RETURN IN THE BUFFER

  UNIX V.4 machines implement a sockets library on top of streams.
  When requesting the IP interface configuration with an ioctl(2) calll,
  an array of ifreq structures are placed in the provided buffer.  Some
  implementations also place the length of the buffer information in
  the first integer position of the buffer.  
  
  SIZE_RETURNED_IN_BUFFER - size integer is in the buffer

WILL IOCTL(SIOCGIFCONF) WORK ON A SOCKET

  Some UNIX V.4 machines do not appear to support ioctl() requests for the
  IP interface configuration on a socket.  They appear to require the use
  of the streams device instead.

  USE_STREAMS_DEVICE_FOR_IF_CONFIG - use the /dev/ip device for configuration

MISC  

  USE_PROTOTYPES    - Prototype functions
  DOSYNCTODR        - Resync TODR clock  every hour.
  RETSIGTYPE        - Define signal function type.
  NO_SIGNED_CHAR_DECL - No "signed char" see include/ntp.h
  LOCK_PROCESS      - Have plock.
  UDP_WILDCARD_DELIVERY
		    - these systems deliver broadcast packets to the wildcard
		      port instead to a port bound to the interface bound
		      to the correct broadcast address - are these
		      implementations broken or did the spec change ?
*/
  
#if 0

/*
 * IRIX 4.X and IRIX 5.x
 */
#if defined(SYS_IRIX4)||defined(SYS_IRIX5)
# define ADJTIME_IS_ACCURATE
# define LOCK_PROCESS
#endif

/*
 * Ultrix
 * Note: posix version has NTP_POSIX_SOURCE and HAVE_SIGNALED_IO
 */
#if defined(SYS_ULTRIX)
# define S_CHAR_DEFINED
# define NTP_SYSCALLS_STD
# define HAVE_MODEM_CONTROL
#endif

/*
 * AUX
 */
#if defined(SYS_AUX2) || defined(SYS_AUX3)
# define NO_SIGNED_CHAR_DECL
# define LOCK_PROCESS
# define NTP_POSIX_SOURCE
/*
 * This requires that _POSIX_SOURCE be forced on the
 * compiler command flag. We can't do it here since this
 * file is included _after_ the system header files and we
 * need to let _them_ know we're POSIX. We do this in
 * compilers/aux3.gcc...
 */
# define LOG_NTP LOG_LOCAL1
#endif

/*
 * HPUX
 */
#if defined(SYS_HPUX)
# define getdtablesize() sysconf(_SC_OPEN_MAX)
# define setlinebuf(f) setvbuf(f, NULL, _IOLBF, 0)
# define NO_SIGNED_CHAR_DECL
# define LOCK_PROCESS
#endif

/*
 * BSD/OS 2.0 and above
 */
#if defined(SYS_BSDI)
# define USE_FSETOWNCTTY	/* this funny system demands a CTTY for FSETOWN */
#endif

/*
 * FreeBSD 2.0 and above
 */
#ifdef SYS_FREEBSD
# define KERNEL_PLL
#endif

/*
 * Linux
 */
#if defined(SYS_LINUX)
# define ntp_adjtime __adjtimex
#endif

/*
 * PTX
 */
#if defined(SYS_PTX)
# define LOCK_PROCESS
struct timezone { int __0; };   /* unused placebo */
/*
 * no comment !@!
 */
typedef unsigned int u_int;
# ifndef	_NETINET_IN_SYSTM_INCLUDED	/* i am about to comment... */
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned long u_long;
# endif
#endif

/* 
 * UNIX V.4 on and NCR 3000
 */
#if defined(SYS_SVR4)
# define STREAM
# define LOCK_PROCESS
# define SIZE_RETURNED_IN_BUFFER
#endif

/*
 * (Univel/Novell) Unixware1 SVR4 on intel x86 processor
 */
#if defined(SYS_UNIXWARE1)
/* #define _POSIX_SOURCE */
# define STREAM
# define STREAMS
# undef STEP_SLEW 		/* TWO step */
# define LOCK_PROCESS
# define SIZE_RETURNED_IN_BUFFER
# include <sys/sockio.h>
# include <sys/types.h>
# include <netinet/in_systm.h>
#endif

/*
 * DomainOS
 */
#if defined(SYS_DOMAINOS)
# define NTP_SYSCALLS_STD
/* older versions of domain/os don't have class D */
# ifndef IN_CLASSD
#  define IN_CLASSD(i)		(((long)(i) & 0xf0000000) == 0xe0000000)
#  define IN_CLASSD_NET		0xf0000000
#  define IN_CLASSD_NSHIFT	28
#  define IN_CLASSD_HOST	0xfffffff
#  define IN_MULTICAST(i)	IN_CLASSD(i)
# endif
#endif

/*
 * Fujitsu UXP/V
 */
#if defined(SYS_UXPV)
# define LOCK_PROCESS
# define SIZE_RETURNED_IN_BUFFER
#endif
#endif

/* 
 * Windows NT
 */
#if defined(SYS_WINNT)
# define REFCLOCK				/* from xntpd.mak */
# define LOCAL_CLOCK			/* from xntpd.mak */
# define DES					/* from libntp.mak */
# define MD5					/* from libntp.mak */
# define NTP_LITTLE_ENDIAN		/* from libntp.mak */
# define SYSLOG_FILE			/* from libntp.mak */
# define HAVE_PROTOTYPES		/* from ntpq.mak */
# define SIZEOF_INT 4			/* for ntp_types.h */
# define SYSV_TIMEOFDAY			/* for ntp_unixtime.h */
# define HAVE_NET_IF_H
# define QSORT_USES_VOID_P
# define HAVE_MEMMOVE
# define volatile
# define STDC_HEADERS
# define NEED_S_CHAR_TYPEDEF
# define SIZEOF_SIGNED_CHAR 1
# define HAVE_NO_NICE
# define NOKMEM
# define PRESET_TICK (every / 10)
# define PRESET_TICKADJ 50
# define RETSIGTYPE void
# define NTP_POSIX_SOURCE
# define HAVE_SETVBUF
# define HAVE_VSPRINTF
# ifndef STR_SYSTEM
#  define STR_SYSTEM "WINDOWS/NT"
# endif
/* winsock.h contains macros for class A,B,C only */
# define IN_CLASSD(i)		(((long)(i) & 0xf0000000) == 0xe0000000)
# define IN_CLASSD_NET		0xf0000000
# define IN_CLASSD_NSHIFT	28
# define IN_CLASSD_HOST		0xfffffff
# define IN_MULTICAST(i)	IN_CLASSD(i)
# define isascii __isascii
# define isatty _isatty
# define fileno _fileno
# define mktemp _mktemp
# define getpid GetCurrentProcessId
# include <winsock.h>
# include <windows.h>
# include <winbase.h>
# undef interface
typedef char *caddr_t;
#endif

/*
 * Here's where autoconfig starts to take over
 */

#ifdef HAVE_SYS_STROPTS_H
# define STREAM
#endif

#ifdef STREAM			/* STREAM implies TERMIOS */
# ifndef HAVE_TERMIOS
#  define HAVE_TERMIOS
# endif
#endif

#ifndef	RETSIGTYPE
# if defined(NTP_POSIX_SOURCE)
#  define	RETSIGTYPE	void
# else
#  define	RETSIGTYPE	int
# endif
#endif

#ifdef	NTP_SYSCALLS_STD
# ifndef	NTP_SYSCALL_GET
#  define	NTP_SYSCALL_GET	235
# endif
# ifndef	NTP_SYSCALL_ADJ
#  define	NTP_SYSCALL_ADJ	236
# endif
#endif	/* NTP_SYSCALLS_STD */

#ifdef HAVE_RTPRIO
# define HAVE_NO_NICE
#else
# ifdef HAVE_SETPRIORITY
#  define HAVE_BSD_NICE
# else
#  ifdef HAVE_NICE
#   define HAVE_ATT_NICE
#  endif
# endif
#endif

#if	!defined(HAVE_ATT_NICE) \
	&& !defined(HAVE_BSD_NICE) \
	&& !defined(HAVE_NO_NICE) \
	&& !defined(SYS_WINNT)
#include "ERROR: You must define one of the HAVE_xx_NICE defines!"
#endif

/*
 * use only one tty model - no use in initialising
 * a tty in three ways
 * HAVE_TERMIOS is preferred over HAVE_SYSV_TTYS over HAVE_BSD_TTYS
 */

#ifdef HAVE_TERMIOS_H
# define HAVE_TERMIOS
#else
# ifdef HAVE_TERMIO_H
#  define HAVE_SYSV_TTYS
# else
#  ifdef HAVE_SGTTY_H
#   define HAVE_BSD_TTYS
#  endif
# endif
#endif

#ifdef HAVE_TERMIOS
# undef HAVE_BSD_TTYS
# undef HAVE_SYSV_TTYS
#endif

#ifdef HAVE_SYSV_TTYS
# undef HAVE_BSD_TTYS
#endif

#if !defined(SYS_WINNT) && !defined(VMS)
# if	!defined(HAVE_SYSV_TTYS) \
	&& !defined(HAVE_BSD_TTYS) \
	&& !defined(HAVE_TERMIOS)
#include "ERROR: no tty type defined!"
# endif
#endif /* SYS_WINNT || VMS */

#ifdef	WORDS_BIGENDIAN
# define	XNTP_BIG_ENDIAN	1
#else
# define	XNTP_LITTLE_ENDIAN	1
#endif

/*
 * Byte order woes.  The DES code is sensitive to byte order.  This
 * used to be resolved by calling ntohl() and htonl() to swap things
 * around, but this turned out to be quite costly on Vaxes where those
 * things are actual functions.  The code now straightens out byte
 * order troubles on its own, with no performance penalty for little
 * end first machines, but at great expense to cleanliness.
 */
#if !defined(XNTP_BIG_ENDIAN) && !defined(XNTP_LITTLE_ENDIAN)
	/*
	 * Pick one or the other.
	 */
	BYTE_ORDER_NOT_DEFINED_FOR_AUTHENTICATION
#endif

#if defined(XNTP_BIG_ENDIAN) && defined(XNTP_LITTLE_ENDIAN)
	/*
	 * Pick one or the other.
	 */
	BYTE_ORDER_NOT_DEFINED_FOR_AUTHENTICATION
#endif

#ifdef HAVE_PROTOTYPES
#define P(x) x
#else
#define P(x) ()
#endif

#endif /* __ntp_machine */
