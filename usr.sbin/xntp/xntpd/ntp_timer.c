/*
 * ntp_event.c - event timer support routines
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/signal.h>

#include "ntpd.h"
#include "ntp_stdlib.h"


/*
 * These routines provide support for the event timer.  The timer is
 * implemented by an interrupt routine which sets a flag once every
 * 2**EVENT_TIMEOUT seconds (currently 4), and a timer routine which
 * is called when the mainline code gets around to seeing the flag.
 * The timer routine dispatches the clock adjustment code if its time
 * has come, then searches the timer queue for expiries which are
 * dispatched to the transmit procedure.  Finally, we call the hourly
 * procedure to do cleanup and print a message.
 */


/*
 * Alarm flag.  The mainline code imports this.
 */
volatile int alarm_flag;

/*
 * adjust and hourly counters
 */
static	u_long adjust_timer;
static	u_long hourly_timer;

/*
 * Imported from the leap module.  The leap timer.
 */
extern u_long leap_timer;

/*
 * Statistics counter for the interested.
 */
volatile u_long alarm_overflow;

#define	HOUR	(60*60)

/*
 * Current_time holds the number of seconds since we started, in
 * increments of 2**EVENT_TIMEOUT seconds.  The timer queue is the
 * hash into which we sort timer entries.
 */
u_long current_time;
struct event timerqueue[TIMER_NSLOTS];

/*
 * Stats.  Number of overflows and number of calls to transmit().
 */
u_long timer_timereset;
u_long timer_overflows;
u_long timer_xmtcalls;

#ifndef SYS_WINNT
static	RETSIGTYPE alarming	P((int));
#else
void PASCAL alarming P((UINT,UINT,DWORD,DWORD,DWORD));
#endif /* SYS_WINNT */

#if defined(VMS)
static int vmstimer[2];    	/* time for next timer AST */
static int vmsinc[2];		/* timer increment */
#endif /* VMS */

/*
 * init_timer - initialize the timer data structures
 */
void
init_timer()
{
  register int i;
#if !defined(VMS)
# ifndef SYS_WINNT
  struct itimerval itimer;
# else /* SYS_WINNT */
  TIMECAPS tc;
  HANDLE hToken;
  TOKEN_PRIVILEGES tkp;
  UINT wTimerRes, wTimerID;
  extern HANDLE hMutex;
# endif /* SYS_WINNT */
#endif /* VMS */

  /*
   * Initialize...
   */
  alarm_flag = 0;
  alarm_overflow = 0;
  adjust_timer = 1;
  hourly_timer = HOUR;
  current_time = 0;
  timer_overflows = 0;
  timer_xmtcalls = 0;
  timer_timereset = 0;

  for (i = 0; i < TIMER_NSLOTS; i++) {
    /*
     * Queue pointers should point at themselves.  Event
     * times must be set to 0 since this is used to
     * detect the queue end.
     */
    timerqueue[i].next = &timerqueue[i];
    timerqueue[i].prev = &timerqueue[i];
    timerqueue[i].event_time = 0;
  }

#ifndef SYS_WINNT
  /*
   * Set up the alarm interrupt.  The first comes 2**EVENT_TIMEOUT
   * seconds from now and they continue on every 2**EVENT_TIMEOUT
   * seconds.
   */
# if !defined(VMS)
  (void) signal_no_reset(SIGALRM, alarming);
  itimer.it_interval.tv_sec = itimer.it_value.tv_sec = (1<<EVENT_TIMEOUT);
  itimer.it_interval.tv_usec = itimer.it_value.tv_usec = 0;
  setitimer(ITIMER_REAL, &itimer, (struct itimerval *)0);
# else /* VMS */
  vmsinc[0] = 10000000;		/* 1 sec */
  vmsinc[1] = 0;
  lib$emul(&(1<<EVENT_TIMEOUT), &vmsinc, &0, &vmsinc);

  sys$gettim(&vmstimer);	/* that's "now" as abstime */

  lib$addx(&vmsinc, &vmstimer, &vmstimer);
  sys$setimr(0, &vmstimer, alarming, alarming, 0);
# endif /* VMS */
#else /* SYS_WINNT */
  _tzset();

  /*
   * Get privileges needed for fiddling with the clock
   */

  /* get the current process token handle */
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
    msyslog(LOG_ERR, "OpenProcessToken failed: %m");
    exit(1);
  }
  /* get the LUID for system-time privilege. */
  LookupPrivilegeValue(NULL, SE_SYSTEMTIME_NAME, &tkp.Privileges[0].Luid);
  tkp.PrivilegeCount = 1;  /* one privilege to set */
  tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  /* get set-time privilege for this process. */
  AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0);
  /* cannot test return value of AdjustTokenPrivileges. */
  if (GetLastError() != ERROR_SUCCESS)
    msyslog(LOG_ERR, "AdjustTokenPrivileges failed: %m");

  /*
   * Set up timer interrupts for every 2**EVENT_TIMEOUT seconds
   * Under Win/NT, expiry of timer interval leads to invocation
   * of a callback function (on a different thread) rather than
   * generating an alarm signal
   */

  /* determine max and min resolution supported */
  if(timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
    msyslog(LOG_ERR, "timeGetDevCaps failed: %m");
    exit(1);
  }
  wTimerRes = min(max(tc.wPeriodMin, TARGET_RESOLUTION), tc.wPeriodMax);
  /* establish the minimum timer resolution that we'll use */
  timeBeginPeriod(wTimerRes);

  hMutex = CreateMutex(
		       NULL,           	/* no security attributes */
		       FALSE,          	/* initially not owned */
		       "MutexForNTP"); 	/* name of mutex */
  if (hMutex == NULL) {
    msyslog(LOG_ERR, "cannot create a mutex: %m\n");
    exit(1);
  }

  /* start the timer event */
  wTimerID = timeSetEvent(
			  (1<<EVENT_TIMEOUT) * 1000,   /* Delay in ms */
			  wTimerRes,                   /* Resolution */
			  (LPTIMECALLBACK) alarming,   /* Callback function */
			  (DWORD) 0,                   /* User data */
			  TIME_PERIODIC);           /* Event type (periodic) */
  if (wTimerID == 0) {
    msyslog(LOG_ERR, "timeSetEvent failed: %m");
    exit(1);
  }
#endif /* SYS_WINNT */
}


/*
 * timer - dispatch anyone who needs to be
 */
void
timer()
{
  register struct event *ev;
  register struct event *tq;
#ifdef SYS_WINNT
  extern HANDLE hMutex;
#endif

  current_time += (1<<EVENT_TIMEOUT);

  /*
   * Adjustment timeout first
   */
  if (adjust_timer <= current_time) {
    adjust_timer += 1;
    adj_host_clock();
  }

#ifdef SYS_WINNT
  if (!ReleaseMutex(hMutex)) {
    msyslog(LOG_ERR, "alarming cannot release mutex: %m\n");
    exit(1);
  }
#endif /* SYS_WINNT */

  /*
   * Leap timer next.
   */
  if (leap_timer != 0 && leap_timer <= current_time)
    leap_process();

  /*
   * Now dispatch any peers whose event timer has expired.
   */

  /* Added mutex to prevent race condition among threads under Windows NT */
#ifdef SYS_WINNT
  WaitForSingleObject(m_hListMutex,INFINITE); 
#endif /* SYS_WINNT */

#ifdef TIMERQUEUE_DEBUG
  {
    int i;
    int j;

    for (i = 0; i < TIMER_NSLOTS; ++i)
      {
	struct event *qh;

	qh = ev = &timerqueue[i];
	if (qh->event_time != 0)
	  msyslog(LOG_ERR, "timerqueue[%d].event_time is %d instead of 0!",
		  i, timerqueue[i].event_time);
	j = 0;
	do
	  {
	    if (ev->prev->next != ev)
	      msyslog(LOG_ERR, "timerqueue[%d]: #%d: ev->prev->next != ev",
		      i, j);
	    if (ev->next->prev != ev)
	      msyslog(LOG_ERR, "timerqueue[%d]: #%d: ev->next->prev != ev",
		      i, j);
	    ++j;
	    ev = ev->next;
	  }
	while (qh != ev);
      }
  }
#endif /* TIMERQUEUE_DEBUG */

  tq = &timerqueue[TIMER_SLOT(current_time)];
  if (tq) {
    ev = tq->next;
    while (ev
	   && ev->event_time != 0
	   && ev->event_time < (current_time + (1<<EVENT_TIMEOUT))) {
      tq->next = ev->next;
      tq->next->prev = tq;
      ev->prev = ev->next = 0;
      timer_xmtcalls++;
      ev->event_handler(ev->peer);
      ev = tq->next;
    }
    if (!ev)
      msyslog(LOG_ERR, "timer: ev was NIL!");
  } else {
    msyslog(LOG_ERR, "timer: tq was NIL!");
  }

  /* Added mutex to prevent race condition among threads under Windows NT */
#ifdef SYS_WINNT
  ReleaseMutex(m_hListMutex);
#endif /* SYS_WINNT */

  /*
   * Finally, call the hourly routine
   */
  if (hourly_timer <= current_time) {
    hourly_timer += HOUR;
    hourly_stats();
  }
}


#ifndef SYS_WINNT
/*
 * alarming - tell the world we've been alarmed
 */
static RETSIGTYPE
alarming(sig)
     int sig;
{
  extern int initializing;	/* from main line code */

#if !defined(VMS)
  if (initializing)
    return;
  if (alarm_flag)
    alarm_overflow++;
  else
    alarm_flag++;
#else /* VMS AST routine */
  if (!initializing) {
    if (alarm_flag) alarm_overflow++;
    else alarm_flag = 1;	/* increment is no good */
  }
  lib$addx(&vmsinc,&vmstimer,&vmstimer);
  sys$setimr(0,&vmstimer,alarming,alarming,0);
#endif /* VMS */
}
#else /* SYS_WINNT */
/*
 * alarming for WinNT - invoke the timer() routine after grabbing the mutex
 */
void PASCAL alarming (UINT wTimerID, UINT msg,
		      DWORD dwUser, DWORD dw1, DWORD dw2)
{
  extern int debug;
  static int initializing2 = 1;
  extern HANDLE hMutex;
  DWORD dwWaitResult;
  extern HANDLE TimerThreadHandle;
  static DWORD threadID;
#ifdef DEBUG
  SYSTEMTIME st;
#endif

  /*
   * set the priority for timer() thread to be higher than the main thread
   */
  if (initializing2) {
    TimerThreadHandle = GetCurrentThread();
    if (!SetThreadPriority(TimerThreadHandle, (DWORD) THREAD_PRIORITY_HIGHEST))
      msyslog(LOG_ERR, "SetThreadPriority failed: %m");
    threadID = GetCurrentThreadId();
    initializing2 = 0;
  }

#ifdef DEBUG
  if (debug > 9) {
    GetSystemTime(&st);
    printf("thread %u (timer callback): time %02u:%02u:%02u:%03u\n",
	   threadID, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    fflush(stdout);
  }
#endif

  dwWaitResult = WaitForSingleObject(
				     hMutex, 	/* handle of mutex */
				     5000L);	/* five-second time-out interval */

  switch (dwWaitResult) {
  case WAIT_OBJECT_0:
    /* The thread got mutex ownership. */
    /* the mutex is released in the timer() routine */
    timer();
    break;
  default:
    /* Cannot get mutex ownership due to time-out. */
    msyslog(LOG_ERR, "alarming error with mutex: %m\n");
    exit(1);
  }

  UNREFERENCED_PARAMETER(dw1);
  UNREFERENCED_PARAMETER(dw2);
  UNREFERENCED_PARAMETER(dwUser);
  UNREFERENCED_PARAMETER(msg);
  UNREFERENCED_PARAMETER(wTimerID);
}
#endif /* SYS_WINNT */


/*
 * timer_clr_stats - clear timer module stat counters
 */
void
timer_clr_stats()
{
  timer_overflows = 0;
  timer_xmtcalls = 0;
  timer_timereset = current_time;
}
