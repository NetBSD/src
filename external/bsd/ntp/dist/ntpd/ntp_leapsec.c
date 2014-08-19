/*	$NetBSD: ntp_leapsec.c,v 1.1.1.1.8.2 2014/08/19 23:51:42 tls Exp $	*/

/*
 * ntp_leapsec.c - leap second processing for NTPD
 *
 * Written by Juergen Perlinger (perlinger@ntp.org) for the NTP project.
 * The contents of 'html/copyright.html' apply.
 * ----------------------------------------------------------------------
 * This is an attempt to get the leap second handling into a dedicated
 * module to make the somewhat convoluted logic testable.
 */

#include <config.h>
#include <sys/types.h>
#include <ctype.h>

#include "ntp_types.h"
#include "ntp_fp.h"
#include "ntp_stdlib.h"
#include "ntp_calendar.h"
#include "ntp_leapsec.h"
#include "ntp.h"

/* ---------------------------------------------------------------------
 * GCC is rather sticky with its 'const' attribute. We have to do it more
 * explicit than with a cast if we want to get rid of a CONST qualifier.
 * Greetings from the PASCAL world, where casting was only possible via
 * untagged unions...
 */
static void* noconst(const void* ptr)
{
	union {
		const void * cp;
		void *       vp;
	} tmp;
	tmp.cp = ptr;
	return tmp.vp;
}

/* ---------------------------------------------------------------------
 * Things to put into libntp...
 */

vint64
strtouv64(
	const char * begp,
	char **      endp,
	int          base)
{
	vint64  res;
	u_char  digit;
	int     sig, num;
	const u_char *src;
	
	num = sig = 0;
	src = (const u_char*)begp;
	while (isspace(*src))
		src++;

	if (*src == '-') {
		src++;
		sig = 1;
	} else  if (*src == '+') {
		src++;
	}

	if (base == 0) {
		base = 10;
		if (*src == '0') {
			base = 8;
			if (toupper(*++src) == 'X') {
				src++;
				base = 16;
			}
		}
	} else if (base == 16) { /* remove optional leading '0x' or '0X' */
		if (src[0] == '0' && toupper(src[1]) == 'X')
			src += 2;
	} else if (base <= 2 || base > 36) {
		memset(&res, 0xFF, sizeof(res));
		errno = ERANGE;
		return res;
	}
	
	memset(&res, 0, sizeof(res));
	while (*src) {
		if (isdigit(*src))
			digit = *src - '0';
		else if (isupper(*src))
			digit = *src - 'A' + 10;
		else if (islower(*src))
			digit = *src - 'a' + 10;
		else
			break;
		if (digit >= base)
			break;
		num = 1;
#if defined(HAVE_INT64)
		res.Q_s = res.Q_s * base + digit;
#else
		/* res *= base, using 16x16->32 bit
		 * multiplication. Slow but portable.
		 */ 
		{
			uint32_t accu;
			accu       = (uint32_t)res.W_s.ll * base;
			res.W_s.ll = (uint16_t)accu;
			accu       = (accu >> 16)
			           + (uint32_t)res.W_s.lh * base;
			res.W_s.lh = (uint16_t)accu;
			/* the upper bits can be done in one step: */
			res.D_s.hi = res.D_s.hi * base + (accu >> 16);
		}
		M_ADD(res.D_s.hi, res.D_s.lo, 0, digit);
#endif
		src++;
	}
	if (!num)
		errno = EINVAL;
	if (endp)
		*endp = (char*)noconst(src);
	if (sig)
		M_NEG(res.D_s.hi, res.D_s.lo);
	return res;
}

int icmpv64(
	const vint64 * lhs,
	const vint64 * rhs)
{
	int res;

#if defined(HAVE_INT64)
	res = (lhs->q_s > rhs->q_s)
	    - (lhs->q_s < rhs->q_s);
#else	
	res = (lhs->d_s.hi > rhs->d_s.hi)
	    - (lhs->d_s.hi < rhs->d_s.hi);
	if ( ! res )
		res = (lhs->D_s.lo > rhs->D_s.lo)
		    - (lhs->D_s.lo < rhs->D_s.lo);
#endif

	return res;
}


int ucmpv64(
	const vint64 * lhs,
	const vint64 * rhs)
{
	int res;
	
#if defined(HAVE_INT64)
	res = (lhs->Q_s > rhs->Q_s)
	    - (lhs->Q_s < rhs->Q_s);
#else	
	res = (lhs->D_s.hi > rhs->D_s.hi)
	    - (lhs->D_s.hi < rhs->D_s.hi);
	if ( ! res )
		res = (lhs->D_s.lo > rhs->D_s.lo)
		    - (lhs->D_s.lo < rhs->D_s.lo);
#endif
	return res;
}

#if 0
static vint64
addv64(
    const vint64 *lhs,
    const vint64 *rhs)
{
	vint64 res;

#if defined(HAVE_INT64)
	res.Q_s = lhs->Q_s + rhs->Q_s;
#else
	res = *lhs;
	M_ADD(res.D_s.hi, res.D_s.lo, rhs->D_s.hi, rhs->D_s.lo);
#endif
	return res;
}
#endif

static vint64
subv64(
    const vint64 *lhs,
    const vint64 *rhs)
{
	vint64 res;

#if defined(HAVE_INT64)
	res.Q_s = lhs->Q_s - rhs->Q_s;
#else
	res = *lhs;
	M_SUB(res.D_s.hi, res.D_s.lo, rhs->D_s.hi, rhs->D_s.lo);
#endif
	return res;
}

static vint64
addv64i32(
	const vint64 * lhs,
	int32_t        rhs)
{
	vint64 res;

	res = *lhs;
#if defined(HAVE_INT64)
	res.q_s += rhs;
#else
	M_ADD(res.D_s.hi, res.D_s.lo,  -(rhs < 0), rhs);
#endif
	return res;
}

#if 0
static vint64
subv64i32(
	const vint64 * lhs,
	int32_t        rhs)
{
	vint64 res;

	res = *lhs;
#if defined(HAVE_INT64)
	res.q_s -= rhs;
#else
	M_SUB(res.D_s.hi, res.D_s.lo,  -(rhs < 0), rhs);
#endif
	return res;
}
#endif

#if 0
static vint64
addv64u32(
	const vint64 * lhs,
	uint32_t       rhs)
{
	vint64 res;

	res = *lhs;
#if defined(HAVE_INT64)
	res.Q_s += rhs;
#else
	M_ADD(res.D_s.hi, res.D_s.lo, 0, rhs);
#endif
	return res;
}
#endif

static vint64
subv64u32(
	const vint64 * lhs,
	uint32_t       rhs)
{
	vint64 res;

	res = *lhs;
#if defined(HAVE_INT64)
	res.Q_s -= rhs;
#else
	M_SUB(res.D_s.hi, res.D_s.lo, 0, rhs);
#endif
	return res;
}

/* ---------------------------------------------------------------------
 * Things to put into ntp_calendar... (and consequently into libntp...)
 */

/* ------------------------------------------------------------------ */
static int
ntpcal_ntp64_to_date(
	struct calendar *jd,
	const vint64    *ntp)
{
	ntpcal_split ds;
	
	ds = ntpcal_daysplit(ntp);
	ds.hi += ntpcal_daysec_to_date(jd, ds.lo);

	return ntpcal_rd_to_date(jd, ds.hi + DAY_NTP_STARTS);
}

/* ------------------------------------------------------------------ */
static vint64
ntpcal_date_to_ntp64(
	const struct calendar *jd)
{
	return ntpcal_dayjoin(ntpcal_date_to_rd(jd) - DAY_NTP_STARTS,
			      ntpcal_date_to_daysec(jd));
}


/* ---------------------------------------------------------------------
 * Our internal data structure
 */
#define MAX_HIST 10	/* history of leap seconds */

struct leap_info {
	vint64   ttime;	/* transition time (after the step, ntp scale) */
	uint32_t stime;	/* schedule limit (a month before transition)  */
	int16_t  taiof;	/* TAI offset on and after the transition      */
	uint8_t  dynls; /* dynamic: inserted on peer/clock request     */
};
typedef struct leap_info leap_info_t;

struct leap_head {
	vint64   update; /* time of information update                 */
	vint64   expire; /* table expiration time                      */
	uint16_t size;	 /* number of infos in table	               */
	int16_t  base_tai;	/* total leaps before first entry      */
	int16_t  this_tai;	/* current TAI offset	               */
	int16_t  next_tai;	/* TAI offset after 'when'             */
	vint64   dtime;	 /* due time (current era end)                 */
	vint64   ttime;	 /* nominal transition time (next era start)   */
	vint64   stime;	 /* schedule time (when we take notice)        */
	vint64   ebase;	 /* base time of this leap era                 */
	uint8_t  dynls;	 /* next leap is dynamic (by peer request)     */
};
typedef struct leap_head leap_head_t;

struct leap_table {
	leap_signature_t lsig;
	leap_head_t	 head;
	leap_info_t  	 info[MAX_HIST];
};

/* Where we store our tables */
static leap_table_t _ltab[2], *_lptr;
static int/*BOOL*/  _electric;

/* Forward decls of local helpers */
static int    add_range(leap_table_t*, const leap_info_t*);
static char * get_line(leapsec_reader, void*, char*, size_t);
static char * skipws(const char*);
static int    parsefail(const char * cp, const char * ep);
static void   reload_limits(leap_table_t*, const vint64*);
static int    betweenu32(uint32_t, uint32_t, uint32_t);
static void   reset_times(leap_table_t*);
static int    leapsec_add(leap_table_t*, const vint64*, int);
static int    leapsec_raw(leap_table_t*, const vint64 *, int, int);

/* =====================================================================
 * Get & Set the current leap table
 */

/* ------------------------------------------------------------------ */
leap_table_t *
leapsec_get_table(
	int alternate)
{
	leap_table_t *p1, *p2;

	p1 = _lptr;
	p1 = &_ltab[p1 == &_ltab[1]];
	p2 = &_ltab[p1 == &_ltab[0]];
	if (alternate) {
		memcpy(p2, p1, sizeof(leap_table_t));
		p1 = p2;
	}

	return p1;
}

/* ------------------------------------------------------------------ */
int/*BOOL*/
leapsec_set_table(
	leap_table_t * pt)
{
	if (pt == &_ltab[0] || pt == &_ltab[1])
		_lptr = pt;
	return _lptr == pt;
}

/* ------------------------------------------------------------------ */
int/*BOOL*/
leapsec_electric(
	int/*BOOL*/ on)
{
	int res = _electric;
	if (on < 0)
		return res;

	_electric = (on != 0);
	if (_electric == res)
		return res;

	if (_lptr == &_ltab[0] || _lptr == &_ltab[1])
		reset_times(_lptr);

	return res;
}

/* =====================================================================
 * API functions that operate on tables
 */

/* ---------------------------------------------------------------------
 * Clear all leap second data. Use it for init & cleanup
 */
void
leapsec_clear(
	leap_table_t * pt)
{
	memset(&pt->lsig, 0, sizeof(pt->lsig));
	memset(&pt->head, 0, sizeof(pt->head));
	reset_times(pt);
}

/* ---------------------------------------------------------------------
 * Load a leap second file and check expiration on the go
 */
int/*BOOL*/
leapsec_load(
	leap_table_t * pt  ,
	leapsec_reader func,
	void *         farg,
	int            use_build_limit)
{
	char   *cp, *ep, linebuf[50];
	vint64 ttime, limit;
	long   taiof;
	struct calendar build;

	leapsec_clear(pt);
	if (use_build_limit && ntpcal_get_build_date(&build))
		limit = ntpcal_date_to_ntp64(&build);
	else
		memset(&limit, 0, sizeof(limit));

	while (get_line(func, farg, linebuf, sizeof(linebuf))) {
		cp = linebuf;
		if (*cp == '#') {
			cp++;
			if (*cp == '@') {
				cp = skipws(cp+1);
				pt->head.expire = strtouv64(cp, &ep, 10);
				if (parsefail(cp, ep))
					goto fail_read;
				pt->lsig.etime = pt->head.expire.D_s.lo;
			} else if (*cp == '$') {
				cp = skipws(cp+1);
				pt->head.update = strtouv64(cp, &ep, 10);
				if (parsefail(cp, ep))
					goto fail_read;
			}		    
		} else if (isdigit((u_char)*cp)) {
			ttime = strtouv64(cp, &ep, 10);
			if (parsefail(cp, ep))
				goto fail_read;
			cp = skipws(ep);
			taiof = strtol(cp, &ep, 10);
			if (   parsefail(cp, ep)
			    || taiof > SHRT_MAX || taiof < SHRT_MIN)
				goto fail_read;
			if (ucmpv64(&ttime, &limit) >= 0) {
				if (!leapsec_raw(pt, &ttime,
						 taiof, FALSE))
					goto fail_insn;
			} else {
				pt->head.base_tai = (int16_t)taiof;
			}
			pt->lsig.ttime = ttime.D_s.lo;
			pt->lsig.taiof = (int16_t)taiof;
		}
	}
	return TRUE;

fail_read:
	errno = EILSEQ;
fail_insn:
	leapsec_clear(pt);
	return FALSE;
}

/* ---------------------------------------------------------------------
 * Dump a table in human-readable format. Use 'fprintf' and a FILE
 * pointer if you want to get it printed into a stream.
 */
void
leapsec_dump(
	const leap_table_t * pt  ,
	leapsec_dumper       func,
	void *               farg)
{
	int             idx;
	vint64          ts;
	struct calendar atb, ttb;

	ntpcal_ntp64_to_date(&ttb, &pt->head.expire);
	(*func)(farg, "leap table (%u entries) expires at %04u-%02u-%02u:\n",
		pt->head.size,
		ttb.year, ttb.month, ttb.monthday);
	idx = pt->head.size;
	while (idx-- != 0) {
		ts = pt->info[idx].ttime;
		ntpcal_ntp64_to_date(&ttb, &ts);
		ts = subv64u32(&ts, pt->info[idx].stime);
		ntpcal_ntp64_to_date(&atb, &ts);

		(*func)(farg, "%04u-%02u-%02u [%c] (%04u-%02u-%02u) - %d\n",
			ttb.year, ttb.month, ttb.monthday,
			"-*"[pt->info[idx].dynls != 0],
			atb.year, atb.month, atb.monthday,
			pt->info[idx].taiof);
	}
}

/* =====================================================================
 * usecase driven API functions
 */

int/*BOOL*/
leapsec_query(
	leap_result_t * qr   ,
	uint32_t        ts32 ,
	const time_t *  pivot)
{
	leap_table_t *   pt;
	vint64           ts64, last, next;
	uint32_t         due32;
	int              fired;

	/* preset things we use later on... */
	fired = FALSE;
	ts64  = ntpcal_ntp_to_ntp(ts32, pivot);
	pt    = leapsec_get_table(FALSE);
	memset(qr, 0, sizeof(leap_result_t));

	if (ucmpv64(&ts64, &pt->head.ebase) < 0) {
		/* Most likely after leap frame reset. Could also be a
		 * backstep of the system clock. Anyway, get the new
		 * leap era frame.
		 */
		reload_limits(pt, &ts64);
	} else if (ucmpv64(&ts64, &pt->head.dtime) >= 0)	{
		/* Boundary crossed in forward direction. This might
		 * indicate a leap transition, so we prepare for that
		 * case.
		 *
		 * Some operations below are actually NOPs in electric
		 * mode, but having only one code path that works for
		 * both modes is easier to maintain.
		 */
		last = pt->head.ttime;
		qr->warped = (int16_t)(last.D_s.lo -
				       pt->head.dtime.D_s.lo);
		next = addv64i32(&ts64, qr->warped);
		reload_limits(pt, &next);
		fired = ucmpv64(&pt->head.ebase, &last) == 0;
		if (fired) {
			ts64 = next;
			ts32 = next.D_s.lo;
		} else {
			qr->warped = 0;
		}
	}

	qr->tai_offs = pt->head.this_tai;

	/* If before the next scheduling alert, we're done. */
	if (ucmpv64(&ts64, &pt->head.stime) < 0)
		return fired;

	/* now start to collect the remaing data */
	due32 = pt->head.dtime.D_s.lo;

	qr->tai_diff  = pt->head.next_tai - pt->head.this_tai;
	qr->ttime     = pt->head.ttime;
	qr->ddist     = due32 - ts32;
	qr->dynamic   = pt->head.dynls;
	qr->proximity = LSPROX_SCHEDULE;

	/* if not in the last day before transition, we're done. */
	if (!betweenu32(due32 - SECSPERDAY, ts32, due32))
		return fired;
	
	qr->proximity = LSPROX_ANNOUNCE;
	if (!betweenu32(due32 - 10, ts32, due32))
		return fired;

	/* The last 10s before the transition. Prepare for action! */
	qr->proximity = LSPROX_ALERT;
	return fired;
}

/* ------------------------------------------------------------------ */
int/*BOOL*/
leapsec_frame(
        leap_result_t *qr)
{
	const leap_table_t * pt;

        memset(qr, 0, sizeof(leap_result_t));
	pt = leapsec_get_table(FALSE);
	if (ucmpv64(&pt->head.ttime, &pt->head.stime) <= 0)
                return FALSE;

	qr->tai_offs = pt->head.this_tai;
	qr->tai_diff = pt->head.next_tai - pt->head.this_tai;
	qr->ttime    = pt->head.ttime;
	qr->dynamic  = pt->head.dynls;

        return TRUE;
}

/* ------------------------------------------------------------------ */
/* Reset the current leap frame */
void
leapsec_reset_frame(void)
{
	reset_times(leapsec_get_table(FALSE));
}

/* ------------------------------------------------------------------ */
int/*BOOL*/
leapsec_load_file(
	FILE * ifp   ,
	int    blimit)
{
	leap_table_t * pt;

	pt = leapsec_get_table(TRUE);
	return leapsec_load(pt, (leapsec_reader)getc, ifp, blimit)
	    && leapsec_set_table(pt);
}

/* ------------------------------------------------------------------ */
void
leapsec_getsig(
	leap_signature_t * psig)
{
	const leap_table_t * pt;

	pt = leapsec_get_table(FALSE);
	memcpy(psig, &pt->lsig, sizeof(leap_signature_t));
}

/* ------------------------------------------------------------------ */
int/*BOOL*/
leapsec_expired(
	uint32_t       when,
	const time_t * tpiv)
{
	const leap_table_t * pt;
	vint64 limit;

	pt = leapsec_get_table(FALSE);
	limit = ntpcal_ntp_to_ntp(when, tpiv);
	return ucmpv64(&limit, &pt->head.expire) >= 0;
}

/* ------------------------------------------------------------------ */
int32_t
leapsec_daystolive(
	uint32_t       when,
	const time_t * tpiv)
{
	const leap_table_t * pt;
	vint64 limit;

	pt = leapsec_get_table(FALSE);
	limit = ntpcal_ntp_to_ntp(when, tpiv);
	limit = subv64(&pt->head.expire, &limit);
	return ntpcal_daysplit(&limit).hi;
}

/* ------------------------------------------------------------------ */
int/*BOOL*/
leapsec_add_fix(
	int            total,
	uint32_t       ttime,
	uint32_t       etime,
	const time_t * pivot)
{
	time_t         tpiv;
	leap_table_t * pt;
	vint64         tt64, et64;

	if (pivot == NULL) {
		time(&tpiv);
		pivot = &tpiv;
	}
	
	et64 = ntpcal_ntp_to_ntp(etime, pivot);
	tt64 = ntpcal_ntp_to_ntp(ttime, pivot);
	pt   = leapsec_get_table(TRUE);

	if (   ucmpv64(&et64, &pt->head.expire) <= 0
	   || !leapsec_raw(pt, &tt64, total, FALSE) )
		return FALSE;

	pt->lsig.etime = etime;
	pt->lsig.ttime = ttime;
	pt->lsig.taiof = (int16_t)total;

	pt->head.expire = et64;

	return leapsec_set_table(pt);
}

/* ------------------------------------------------------------------ */
int/*BOOL*/
leapsec_add_dyn(
	int            insert,
	uint32_t       ntpnow,
	const time_t * pivot )
{
	leap_table_t * pt;
	vint64         now64;

	pt = leapsec_get_table(TRUE);
	now64 = ntpcal_ntp_to_ntp(ntpnow, pivot);
	return leapsec_add(pt, &now64, (insert != 0))
	    && leapsec_set_table(pt);
}

/* =====================================================================
 * internal helpers
 */

/* [internal] Reset / init the time window in the leap processor to
 * force reload on next query. Since a leap transition cannot take place
 * at an odd second, the value chosen avoids spurious leap transition
 * triggers. Making all three times equal forces a reload. Using the
 * maximum value for unsigned 64 bits makes finding the next leap frame
 * a bit easier.
 */
static void
reset_times(
	leap_table_t * pt)
{
	memset(&pt->head.ebase, 0xFF, sizeof(vint64));
	pt->head.stime = pt->head.ebase;
	pt->head.ttime = pt->head.ebase;
	pt->head.dtime = pt->head.ebase;
}

/* [internal] Add raw data to the table, removing old entries on the
 * fly. This cannot fail currently.
 */
static int/*BOOL*/
add_range(
	leap_table_t *      pt,
	const leap_info_t * pi)
{
	/* If the table is full, make room by throwing out the oldest
	 * entry. But remember the accumulated leap seconds!
	 */
	if (pt->head.size >= MAX_HIST) {
		pt->head.size     = MAX_HIST - 1;
		pt->head.base_tai = pt->info[pt->head.size].taiof;
	}

	/* make room in lower end and insert item */
	memmove(pt->info+1, pt->info, pt->head.size*sizeof(*pt->info)); 
	pt->info[0] = *pi;
	pt->head.size++;

	/* invalidate the cached limit data -- we might have news ;-)
	 *
	 * This blocks a spurious transition detection. OTOH, if you add
	 * a value after the last query before a leap transition was
	 * expected to occur, this transition trigger is lost. But we
	 * can probably live with that.
	 */
	reset_times(pt);
	return TRUE;
}

/* [internal] given a reader function, read characters into a buffer
 * until either EOL or EOF is reached. Makes sure that the buffer is
 * always NUL terminated, but silently truncates excessive data. The
 * EOL-marker ('\n') is *not* stored in the buffer.
 *
 * Returns the pointer to the buffer, unless EOF was reached when trying
 * to read the first character of a line.
 */
static char *
get_line(
	leapsec_reader func,
	void *         farg,
	char *         buff,
	size_t         size)
{
	int   ch;
	char *ptr;
	
	/* if we cannot even store the delimiter, declare failure */
	if (buff == NULL || size == 0)
		return NULL;

	ptr = buff;
	while (EOF != (ch = (*func)(farg)) && '\n' != ch)
		if (size > 1) {
			size--;
			*ptr++ = (char)ch;
		}
	/* discard trailing whitespace */
	while (ptr != buff && isspace((u_char)ptr[-1]))
		ptr--;
	*ptr = '\0';
	return (ptr == buff && ch == EOF) ? NULL : buff;
}

/* [internal] skips whitespace characters from a character buffer. */
static char *
skipws(
	const char *ptr)
{
	while (isspace((u_char)*ptr))
		ptr++;
	return (char*)noconst(ptr);
}

/* [internal] check if a strtoXYZ ended at EOL or whistespace and
 * converted something at all. Return TRUE if something went wrong.
 */
static int/*BOOL*/
parsefail(
	const char * cp,
	const char * ep)
{
	return (cp == ep)
	    || (*ep && *ep != '#' && !isspace((u_char)*ep));
}

/* [internal] reload the table limits around the given time stamp. This
 * is where the real work is done when it comes to table lookup and
 * evaluation. Some care has been taken to have correct code for dealing
 * with boundary conditions and empty tables.
 *
 * In electric mode, transition and trip time are the same. In dumb
 * mode, the difference of the TAI offsets must be taken into account
 * and trip time and transition time become different. The difference
 * becomes the warping distance when the trip time is reached.
 */
static void
reload_limits(
	leap_table_t * pt,
	const vint64 * ts)
{
	int idx;

	/* Get full time and search the true lower bound. Use a
	 * simple loop here, since the number of entries does
	 * not warrant a binary search. This also works for an empty
	 * table, so there is no shortcut for that case.
	 */
	for (idx = 0; idx != pt->head.size; idx++)
		if (ucmpv64(ts, &pt->info[idx].ttime) >= 0)
			break;

	/* get time limits with proper bound conditions. Note that the
	 * bounds of the table will be observed even if the table is
	 * empty -- no undefined condition must arise from this code.
	 */
	if (idx >= pt->head.size) {
		memset(&pt->head.ebase, 0x00, sizeof(vint64));
		pt->head.this_tai = pt->head.base_tai;
	} else {
		pt->head.ebase    = pt->info[idx].ttime;
		pt->head.this_tai = pt->info[idx].taiof;
	}
	if (--idx >= 0) {
		pt->head.next_tai = pt->info[idx].taiof;
		pt->head.dynls    = pt->info[idx].dynls;
		pt->head.ttime    = pt->info[idx].ttime;

		if (_electric)
			pt->head.dtime = pt->head.ttime;
                else
			pt->head.dtime = addv64i32(
				&pt->head.ttime,
				pt->head.next_tai - pt->head.this_tai);
		
		pt->head.stime = subv64u32(
			&pt->head.ttime, pt->info[idx].stime);

	} else {
		memset(&pt->head.ttime, 0xFF, sizeof(vint64));
		pt->head.stime    = pt->head.ttime;
		pt->head.dtime    = pt->head.ttime;
		pt->head.next_tai = pt->head.this_tai;
		pt->head.dynls    = 0;
	}
}

/* [internal] Take a time stamp and create a leap second frame for
 * it. This will schedule a leap second for the beginning of the next
 * month, midnight UTC. The 'insert' argument tells if a leap second is
 * added (!=0) or removed (==0). We do not handle multiple inserts
 * (yet?)
 *
 * Returns 1 if the insert worked, 0 otherwise. (It's not possible to
 * insert a leap second into the current history -- only appending
 * towards the future is allowed!)
 */
static int/*BOOL*/
leapsec_add(
	leap_table_t*  pt    ,
	const vint64 * now64 ,
	int            insert)
{
	vint64		ttime, stime;
	struct calendar	fts;
	leap_info_t	li;

	/* Check against the table expiration and the lates available
	 * leap entry. Do not permit inserts, only appends, and only if
	 * the extend the table beyond the expiration!
	 */
	if (   ucmpv64(now64, &pt->head.expire) < 0
	   || (pt->head.size && ucmpv64(now64, &pt->info[0].ttime) <= 0)) {
		errno = ERANGE;
		return FALSE;
	}

	ntpcal_ntp64_to_date(&fts, now64);
	/* To guard against dangling leap flags: do not accept leap
	 * second request on the 1st hour of the 1st day of the month.
	 */
	if (fts.monthday == 1 && fts.hour == 0) {
		errno = EINVAL;
		return FALSE;
	}

	/* Ok, do the remaining calculations */
	fts.monthday = 1;
	fts.hour     = 0;
	fts.minute   = 0;
	fts.second   = 0;
	stime = ntpcal_date_to_ntp64(&fts);
	fts.month++;
	ttime = ntpcal_date_to_ntp64(&fts);

	li.ttime = ttime;
	li.stime = ttime.D_s.lo - stime.D_s.lo;
	li.taiof = (pt->head.size ? pt->info[0].taiof : pt->head.base_tai)
	         + (insert ? 1 : -1);
	li.dynls = 1;
	return add_range(pt, &li);
}

/* [internal] Given a time stamp for a leap insertion (the exact begin
 * of the new leap era), create new leap frame and put it into the
 * table. This is the work horse for reading a leap file and getting a
 * leap second update via authenticated network packet.
 */
int/*BOOL*/
leapsec_raw(
	leap_table_t * pt,
	const vint64 * ttime,
	int            taiof,
	int            dynls)
{
	vint64		stime;
	struct calendar	fts;
	leap_info_t	li;

	/* Check that we only extend the table. Paranoia rulez! */
	if (pt->head.size && ucmpv64(ttime, &pt->info[0].ttime) <= 0) {
		errno = ERANGE;
		return FALSE;
	}

	ntpcal_ntp64_to_date(&fts, ttime);
	/* If this does not match the exact month start, bail out. */
	if (fts.monthday != 1 || fts.hour || fts.minute || fts.second) {
		errno = EINVAL;
		return FALSE;
	}
	fts.month--; /* was in range 1..12, no overflow here! */
	stime    = ntpcal_date_to_ntp64(&fts);
	li.ttime = *ttime;
	li.stime = ttime->D_s.lo - stime.D_s.lo;
	li.taiof = (int16_t)taiof;
	li.dynls = (dynls != 0);
	return add_range(pt, &li);
}

/* [internal] Do a wrap-around save range inclusion check.
 * Returns TRUE if x in [lo,hi[ (intervall open on right side) with full
 * handling of an overflow / wrap-around.
 */
static int/*BOOL*/
betweenu32(
	uint32_t lo,
	uint32_t x,
	uint32_t hi)
{
	int rc;
	if (lo <= hi)
		rc = (lo <= x) && (x < hi);
	else
		rc = (lo <= x) || (x < hi);
	return rc;
}

/* -*- that's all folks! -*- */
