/*	$NetBSD: tod.h,v 1.2 2001/06/11 21:33:47 fredette Exp $	*/

/*
 * Structures and macros des accessing the National Semiconductor
 * MM58167 time-of-day chip as wired in the Sun2.  See
 * http://www.national.com/ds/MM/MM58167B.pdf for data sheets,
 * and sys/dev/ic/mm58167var.h.
 */

#if __for_reference_only__
struct mm58167regs {
  
  /* the various timers.  all of these values are in BCD: */

  /* the most significant digit of this value is the milliseconds
     unit; least significant digit of this value is undefined: */
  u_int8_t mm58167_msec_xxx;
  u_int8_t mm58167_unused0;
  /* both digits of this value make up centiseconds: */
  u_int8_t mm58167_csec;
  u_int8_t mm58167_unused1;
  u_int8_t mm58167_sec;
  u_int8_t mm58167_unused2;
  u_int8_t mm58167_min;
  u_int8_t mm58167_unused3;
  u_int8_t mm58167_hour;
  u_int8_t mm58167_unused4;
  u_int8_t mm58167_wday;
  u_int8_t mm58167_unused5;
  u_int8_t mm58167_day;
  u_int8_t mm58167_unused6;
  u_int8_t mm58167_mon;
  u_int8_t mm58167_unused7;

  /* the compare latches.  these line up with the timers above, but
     since we don't use them, we don't go through the trouble of
     defining real members for them: */
  struct {
    u_int8_t _mm58167_latch_val;
    u_int8_t _mm58167_latch_unused;
  } _mm58167_latches[8];

  u_int8_t mm58167_isr;		/* interrupt status - not used */
  u_int8_t _mm58167_unused8;
  u_int8_t mm58167_icr;		/* interrupt control - not used */
  u_int8_t _mm58167_unused9;
  u_int8_t mm58167_creset;	/* counter reset mask */
  u_int8_t _mm58167_unused10;
  u_int8_t mm58167_lreset;	/* latch reset mask */
  u_int8_t _mm58167_unused11;
  u_int8_t mm58167_status;	/* bad counter read status */
  u_int8_t _mm58167_unused12;
  u_int8_t mm58167_go;		/* GO - start at integral seconds */
  u_int8_t _mm58167_unused13;
  u_int8_t mm58167_stby;	/* standby mode - not used */
  u_int8_t _mm58167_unused14;
  u_int8_t mm58167_test;	/* test mode - ??? */
  u_int8_t _mm58167_unused15;
};
#endif

/*
 * Register offsets.
 */
#define	MM58167REG_MSEC_XXX	0
#define	MM58167REG_CSEC		2
#define	MM58167REG_SEC		4
#define	MM58167REG_MIN		6
#define	MM58167REG_HOUR		8
#define	MM58167REG_WDAY		10
#define	MM58167REG_DAY		12
#define	MM58167REG_MON		14
#define	MM58167REG_STATUS	40
#define	MM58167REG_GO		42
#define	MM58167REG_BANK_SZ	48
