/*	$NetBSD: tod.h,v 1.4 2005/12/11 12:19:16 christos Exp $	*/

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
	uint8_t mm58167_msec_xxx;
	uint8_t mm58167_unused0;
	/* both digits of this value make up centiseconds: */
	uint8_t mm58167_csec;
	uint8_t mm58167_unused1;
	uint8_t mm58167_sec;
	uint8_t mm58167_unused2;
	uint8_t mm58167_min;
	uint8_t mm58167_unused3;
	uint8_t mm58167_hour;
	uint8_t mm58167_unused4;
	uint8_t mm58167_wday;
	uint8_t mm58167_unused5;
	uint8_t mm58167_day;
	uint8_t mm58167_unused6;
	uint8_t mm58167_mon;
	uint8_t mm58167_unused7;

	/* the compare latches.  these line up with the timers above, but
	   since we don't use them, we don't go through the trouble of
	   defining real members for them: */
	struct {
		uint8_t _mm58167_latch_val;
		uint8_t _mm58167_latch_unused;
	} _mm58167_latches[8];

	uint8_t mm58167_isr;		/* interrupt status - not used */
	uint8_t _mm58167_unused8;
	uint8_t mm58167_icr;		/* interrupt control - not used */
	uint8_t _mm58167_unused9;
	uint8_t mm58167_creset;		/* counter reset mask */
	uint8_t _mm58167_unused10;
	uint8_t mm58167_lreset;		/* latch reset mask */
	uint8_t _mm58167_unused11;
	uint8_t mm58167_status;		/* bad counter read status */
	uint8_t _mm58167_unused12;
	uint8_t mm58167_go;		/* GO - start at integral seconds */
	uint8_t _mm58167_unused13;
	uint8_t mm58167_stby;		/* standby mode - not used */
	uint8_t _mm58167_unused14;
	uint8_t mm58167_test;		/* test mode - ??? */
	uint8_t _mm58167_unused15;
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
