/* machines.c - provide special support for peculiar architectures
 *
 * Real bummers unite !
 *
 */

#include "ntp_stdlib.h"


#ifndef SYS_WINNT

#ifdef SYS_PTX			/* Does PTX still need this? */
/*#include <sys/types.h>	*/
#include <sys/procstats.h>

int
gettimeofday(tvp)
  struct timeval *tvp;
{
  /*
   * hi, this is Sequents sneak path to get to a clock
   * this is also the most logical syscall for such a function
   */
  return (get_process_stats(tvp, PS_SELF, (struct procstats *) 0,
			    (struct procstats *) 0));
}
#endif /* SYS_PTX */

#ifdef HAVE_SETTIMEOFDAY
char *set_tod_using = "settimeofday";
#else /* not HAVE_SETTIMEOFDAY */
# ifdef HAVE_CLOCK_SETTIME
char *set_tod_using = "clock_settime";

/*#include <time.h>	*/

int
settimeofday(tvp, tzp)
  struct timeval *tvp;
  void *tzp;
{
  struct timespec ts;

  /* Convert timeval to timespec */
  ts.tv_sec = tvp->tv_sec;
  ts.tv_nsec = 1000 *  tvp->tv_usec;

  return clock_settime(CLOCK_REALTIME, &ts);
}

# else /* not (HAVE_SETTIMEOFDAY || HAVE_CLOCK_SETTIME) */
#  ifdef HAVE_STIME
char *set_tod_using = "stime";

int
settimeofday(tvp, tzp)
  struct timeval *tvp;
  void *tzp;
{
  return (stime(&tvp->tv_sec));	/* lie as bad as SysVR4 */
}

#  endif /* HAVE_STIME */
# endif /* not (HAVE_SETTIMEOFDAY || HAVE_CLOCK_SETTIME) */
#endif /* not HAVE_SETTIME */


#else /* SYS_WINNT */


#include <time.h>
#include <sys\timeb.h>
#include <conio.h>
#include "ntp_syslog.h"

char *	set_tod_using = "SetSystemTime";

/* Windows NT versions of gettimeofday and settimeofday */

int
gettimeofday(tv)
     struct timeval *tv;
{
  struct _timeb timebuffer;

  _ftime(&timebuffer);
  tv->tv_sec = (long) timebuffer.time;
  tv->tv_usec = (long) (1000 * (timebuffer.millitm));
  return 0;
}


int
settimeofday(tv)
     struct timeval *tv;
{
  SYSTEMTIME st;
  struct tm *gmtm;
  long x = tv->tv_sec;
  long y = tv->tv_usec;
  
  gmtm = gmtime((const time_t *) &x);
  st.wSecond		= (WORD) gmtm->tm_sec;
  st.wMinute		= (WORD) gmtm->tm_min;
  st.wHour			= (WORD) gmtm->tm_hour;
  st.wDay			= (WORD) gmtm->tm_mday;
  st.wMonth			= (WORD) (gmtm->tm_mon  + 1);
  st.wYear			= (WORD) (gmtm->tm_year + 1900);
  st.wDayOfWeek		= (WORD) gmtm->tm_wday;
  st.wMilliseconds	= (WORD) (y / 1000);

  if (!SetSystemTime(&st)) { 
    msyslog(LOG_ERR, "SetSystemTime failed: %m\n");
    return -1;
  }
  return 0;
}


/* getpass is used in ntpq.c and ntpdc.c */

char *
getpass(const char * prompt)
{
	int c, i;
	static char password[32];

	fprintf(stderr, "%s", prompt); 
	fflush(stderr);
	for (i=0; i<sizeof(password)-1 && ((c=_getch())!='\n'); i++) {
		password[i] = c;
	}
	password[i] = '\0';

	return password;
}

#endif /* SYS_WINNT */

#if !defined(HAVE_MEMSET) || defined(NTP_NEED_BOPS)
void
ntp_memset(a, x, c)
	char *a;
	int x, c;
{
	while (c-- > 0)
		*a++ = x;
}
#endif /*POSIX*/
