/*	$NetBSD: rnd.c,v 1.12 1999/01/27 10:41:00 mrg Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael Graff <explorer@flame.org>.  This code uses ideas and
 * algorithms from the Linux driver written by Ted Ts'o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/malloc.h>
#include <sys/md5.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/rnd.h>
#include <sys/vnode.h>

#ifdef RND_DEBUG
#define DPRINTF(l,x)      if (rnd_debug & (l)) printf x
int     rnd_debug = 0;
#else
#define DPRINTF(l,x)
#endif

#define RND_DEBUG_WRITE		0x0001
#define RND_DEBUG_READ		0x0002
#define RND_DEBUG_IOCTL		0x0004
#define RND_DEBUG_SNOOZE	0x0008

/*
 * list devices attached
 */
/* #define RND_VERBOSE */

/*
 * Use the extraction time as a somewhat-random source
 */
#ifndef RND_USE_EXTRACT_TIME
#define RND_USE_EXTRACT_TIME 1
#endif

/*
 * The size of a temporary buffer, malloc()ed when needed, and used for
 * reading and writing data.
 */
#define RND_TEMP_BUFFER_SIZE	128

typedef struct _rnd_event_t {
	rndsource_t    *source;
	u_int32_t	val;
	u_int32_t	timestamp;
} rnd_event_t;

/*
 * the event queue.  Fields are altered at an interrupt level.
 */
volatile int		rnd_head;
volatile int		rnd_tail;
volatile int		rnd_timeout_pending;
volatile rnd_event_t	rnd_events[RND_EVENTQSIZE];

/*
 * our select/poll queue
 */
struct selinfo rnd_selq;

/*
 * Set when there are readers blocking on data from us
 */
#define RND_READWAITING 0x00000001
volatile u_int32_t  rnd_status;

/*
 * our random pool.  This is defined here rather than using the general
 * purpose one defined in rndpool.c
 */
rndpool_t   rnd_pool;

/*
 * This is used for devices that pass a NULL source pointer into the
 * rnd_add_*() functions.  The user never sees this source, and cannot
 * modify it.
 */
static rndsource_t rnd_source_no_estimate = {
	{ 'U', 'n', 'k', 'n', 'o', 'w', 'n', 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	0, 0, 0, 0,
	(RND_FLAG_NO_ESTIMATE | RND_TYPE_UNKNOWN)
};

/*
 * This source is used to easily "remove" queue entries when the source
 * which actually generated the events is going away.
 */
static rndsource_t rnd_source_no_collect = {
	{ 'U', 'n', 'k', 'n', 'o', 'w', 'n', 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	0, 0, 0, 0,
	(RND_FLAG_NO_COLLECT | RND_FLAG_NO_ESTIMATE | RND_TYPE_UNKNOWN)
};

void	rndattach __P((int));
int	rndopen __P((dev_t, int, int, struct proc *));
int	rndclose __P((dev_t, int, int, struct proc *));
int	rndread __P((dev_t, struct uio *, int));
int	rndwrite __P((dev_t, struct uio *, int));
int	rndioctl __P((dev_t, u_long, caddr_t, int, struct proc *));
int	rndpoll __P((dev_t, int, struct proc *));

static inline void	rnd_wakeup_readers(void);
static inline u_int32_t rnd_estimate_entropy(rndsource_t *, u_int32_t);
static inline u_int32_t rnd_timestamp(void);
static 	      void	rnd_timeout(void *);

/*
 * Generate a 32-bit timestamp.  This should be more machine dependant,
 * using cycle counters and the like when possible.
 */
static inline u_int32_t
rnd_timestamp()
{
	struct timeval	tv;
	u_int32_t	t;

	microtime(&tv);

	t = tv.tv_sec * 1000000 + tv.tv_usec;

	return t;
}

/*
 * Check to see if there are readers waiting on us.  If so, kick them.
 */
static inline void
rnd_wakeup_readers()
{
	/*
	 * If we have added new bits, and now have enough to do something,
	 * wake up sleeping readers.
	 */
	if (rndpool_get_entropy_count(&rnd_pool) > RND_ENTROPY_THRESHOLD * 8) {
		if (rnd_status & RND_READWAITING) {
			DPRINTF(RND_DEBUG_SNOOZE,
				("waking up pending readers.\n"));
			rnd_status &= ~RND_READWAITING;
			wakeup(&rnd_selq);
		}
		selwakeup(&rnd_selq);
	}
}

/*
 * Use the timing of the event to estimate the entropy gathered.
 * Note that right now we will return either one or two, depending on
 * if all the differentials (first, second, and third) are non-zero.
 */
static inline u_int32_t
rnd_estimate_entropy(rs, t)
	rndsource_t *rs;
	u_int32_t t;
{
	int32_t		delta;
	int32_t		delta2;
	int32_t		delta3;

	/*
	 * If the time counter has overflowed, calculate the real difference.
	 * If it has not, it is simplier.
	 */
	if (t < rs->last_time)
		delta = UINT_MAX - rs->last_time + t;
	else
		delta = rs->last_time - t;

	if (delta < 0)
		delta = -delta;

	/*
	 * Calculate the second and third order differentials
	 */
	delta2 = rs->last_delta - delta;
	if (delta2 < 0)
		delta2 = -delta2;

	delta3 = rs->last_delta2 - delta2;
	if (delta3 < 0)
		delta3 = -delta3;

	rs->last_time = t;
	rs->last_delta = delta;
	rs->last_delta2 = delta2;

	/*
	 * If any delta is 0, we got no entropy.  If all are non-zero, we
	 * got one bit.
	 *
	 * XXX This is probably too conservative, but better that than
	 * too liberal.
	 */
	if (delta == 0 || delta2 == 0 || delta3 == 0)
		return 0;

	return 1;
}

static int		rnd_ready;

LIST_HEAD(, __rndsource_element)	rnd_sources;

/*
 * attach the random device, and initialize the global random pool
 * for our use.
 */
void
rndattach(num)
	int num;
{

	rnd_init();
}

int
rndopen(dev, flags, ifmt, p)
	dev_t dev;
	int flags, ifmt;
	struct proc *p;
{

	if (rnd_ready == 0)
		return (ENXIO);

	if (minor(dev) == RND_DEV_RANDOM || minor(dev) == RND_DEV_URANDOM)
		return 0;

	return (ENXIO);
}

int
rndclose(dev, flags, ifmt, p)
	dev_t dev;
	int flags, ifmt;
	struct proc *p;
{

	return (0);
}

int
rndread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	int		ret;
	u_int32_t       nread;
	int		n;
	int		s;
	u_int8_t	*buf;
	u_int32_t	mode;
	u_int32_t	entcnt;

	DPRINTF(RND_DEBUG_READ,
		("Random:  Read of %d requested, flags 0x%08x\n",
		 uio->uio_resid, ioflag));

	if (uio->uio_resid == 0)
		return (0);

	switch (minor(dev)) {
	case RND_DEV_RANDOM:
		mode = RND_EXTRACT_GOOD;
		break;
	case RND_DEV_URANDOM:
		mode = RND_EXTRACT_ANY;
		break;
	default:
		/* Can't happen, but this is cheap */
		return (ENXIO);
	}

	ret = 0;

	buf = malloc(RND_TEMP_BUFFER_SIZE, M_TEMP, M_WAITOK);

	while (uio->uio_resid > 0) {
		n = min(RND_TEMP_BUFFER_SIZE, uio->uio_resid);

		/*
		 * Make certain there is data available.  If there
		 * is, do the I/O even if it is partial.  If not,
		 * sleep unless the user has requested non-blocking
		 * I/O.
		 */
		for (;;) {
			/*
			 * If not requesting strong randomness, we
			 * can always read.
			 */
			if (mode == RND_EXTRACT_ANY)
				break;

			/*
			 * How much entropy do we have?  If it is enough for
			 * one hash, we can read.
			 */
			s = splsoftclock();
			entcnt = rndpool_get_entropy_count(&rnd_pool);
			splx(s);
			if (entcnt >= RND_ENTROPY_THRESHOLD * 8)
				break;

			/*
			 * Data is not available.
			 */
			if (ioflag & IO_NDELAY) {
				ret = EWOULDBLOCK;
				goto out;
			}

			rnd_status |= RND_READWAITING;
			ret = tsleep(&rnd_selq, PRIBIO|PCATCH,
				     "rndread", 0);

			if (ret)
				goto out;
		}

		nread = rnd_extract_data(buf, n, mode);

		/*
		 * copy (possibly partial) data to the user.
		 * If an error occurs, or this is a partial
		 * read, bail out.
		 */
		ret = uiomove((caddr_t)buf, nread, uio);
		if (ret != 0 || nread != n)
			goto out;
	}

out:
	free(buf, M_TEMP);
	return (ret);
}

int
rndwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	u_int8_t	*buf;
	int		ret;
	int		n;
	int		s;

	DPRINTF(RND_DEBUG_WRITE,
		("Random:  Write of %d requested\n", uio->uio_resid));

	if (uio->uio_resid == 0)
		return (0);

	ret = 0;

	buf = malloc(RND_TEMP_BUFFER_SIZE, M_TEMP, M_WAITOK);

	while (uio->uio_resid > 0) {
		n = min(RND_TEMP_BUFFER_SIZE, uio->uio_resid);

		ret = uiomove((caddr_t)buf, n, uio);
		if (ret != 0)
			break;

		/*
		 * Mix in the bytes.
		 */
		s = splsoftclock();
		rndpool_add_data(&rnd_pool, buf, n, 0);
		splx(s);

		DPRINTF(RND_DEBUG_WRITE, ("Random:  Copied in %d bytes\n", n));
	}

	free(buf, M_TEMP);
	return (ret);
}

int
rndioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	int			 ret;
	rndsource_element_t	*rse;
	rndstat_t		*rst;
	rndstat_name_t		*rstnm;
	rndctl_t		*rctl;
	rnddata_t		*rnddata;
	u_int32_t		 count;
	u_int32_t		 start;
	int			 s;
	
	ret = 0;

	switch (cmd) {

	  /*
	   * Handled in upper layer really, but we have to return zero
	   * for it to be accepted by the upper layer.
	   */
	case FIONBIO:
	case FIOASYNC:
		break;

	case RNDGETENTCNT:
		s = splsoftclock();
		*(u_int32_t *)addr = rndpool_get_entropy_count(&rnd_pool);
		splx(s);

		break;

	case RNDGETPOOL:
		if ((ret = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (ret);

		s = splsoftclock();
		ret = copyout(rndpool_get_pool(&rnd_pool),
			      addr, rndpool_get_poolsize());
		splx(s);

		break;

	case RNDADDTOENTCNT:
		if ((ret = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (ret);

		s = splsoftclock();
		rndpool_increment_entropy_count(&rnd_pool, *(u_int32_t *)addr);
		rnd_wakeup_readers();
		splx(s);

		break;

	case RNDSETENTCNT:
		if ((ret = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (ret);

		s = splsoftclock();
		rndpool_set_entropy_count(&rnd_pool, *(u_int32_t *)addr);
		rnd_wakeup_readers();
		splx(s);

		break;

	case RNDGETSRCNUM:
		if ((ret = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (ret);

		rst = (rndstat_t *)addr;

		if (rst->count == 0)
			break;

		if (rst->count > RND_MAXSTATCOUNT)
			return (EINVAL);

		/*
		 * find the starting source by running through the
		 * list of sources.
		 */
		rse = rnd_sources.lh_first;
		start = rst->start;
		while (rse != NULL && start >= 1) {
			rse = rse->list.le_next;
			start--;
		}

		/*
		 * Return up to as many structures as the user asked
		 * for.  If we run out of sources, a count of zero
		 * will be returned, without an error.
		 */
		for (count = 0 ; count < rst->count && rse != NULL ; count++) {
			bcopy(&rse->data, &rst->source[count],
			      sizeof(rndsource_t));
			rse = rse->list.le_next;
		}

		rst->count = count;

		break;

	case RNDGETSRCNAME:
		if ((ret = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (ret);

		/*
		 * scan through the list, trying to find the name
		 */
		rstnm = (rndstat_name_t *)addr;
		rse = rnd_sources.lh_first;
		while (rse != NULL) {
			if (strncmp(rse->data.name, rstnm->name, 16) == 0) {
				bcopy(&rse->data, &rstnm->source,
				      sizeof(rndsource_t));

				return 0;
			}
			rse = rse->list.le_next;
		}

		ret = ENOENT;  /* name not found */

		break;

	case RNDCTL:
		if ((ret = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (ret);

		/*
		 * set flags to enable/disable entropy counting and/or
		 * collection
		 */
		rctl = (rndctl_t *)addr;
		rse = rnd_sources.lh_first;

		/*
		 * flags set apply to all sources of this type
		 */
		if (rctl->type != 0xff) {
			while (rse != NULL) {
				if ((rse->data.tyfl & 0xff) == rctl->type) {
					rse->data.tyfl &= ~rctl->mask;
					rse->data.tyfl |= (rctl->flags
							   & rctl->mask);
				}
				rse = rse->list.le_next;
			}

			return 0;
		}
			
		/*
		 * scan through the list, trying to find the name
		 */
		while (rse != NULL) {
			if (strncmp(rse->data.name, rctl->name, 16) == 0) {
				rse->data.tyfl &= ~rctl->mask;
				rse->data.tyfl |= (rctl->flags & rctl->mask);

				return 0;
			}
			rse = rse->list.le_next;
		}

		ret = ENOENT;  /* name not found */

		break;

	case RNDADDDATA:
		if ((ret = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (ret);

		rnddata = (rnddata_t *)addr;

		s = splsoftclock();
		rndpool_add_data(&rnd_pool, rnddata->data, rnddata->len,
				 rnddata->entropy);

		rnd_wakeup_readers();
		splx(s);

		break;

	default:
		return (EINVAL);
	}

	return (ret);
}

int
rndpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	int revents;
	int s;
	u_int32_t entcnt;

	/*
	 * we are always writable
	 */
	revents = events & (POLLOUT | POLLWRNORM);

	/*
	 * Save some work if not checking for reads
	 */
	if ((events & (POLLIN | POLLRDNORM)) == 0)
		return (revents);

	/*
	 * If the minor device is not /dev/random, we are always readable.
	 */
	if (minor(dev) != RND_DEV_RANDOM) {
		revents |= events & (POLLIN | POLLRDNORM);
		return (revents);
	}

	/*
	 * make certain we have enough entropy to be readable
	 */
	s = splsoftclock();
	entcnt = rndpool_get_entropy_count(&rnd_pool);
	splx(s);

	if (entcnt >= RND_ENTROPY_THRESHOLD * 8)
		revents |= events & (POLLIN | POLLRDNORM);
	else
		selrecord(p, &rnd_selq);

	return (revents);
}

/*
 * initialize the data for the random generator.
 */
void
rnd_init(void)
{
	if (rnd_ready)
		return;

	LIST_INIT(&rnd_sources);

	rnd_head = 0;
	rnd_tail = 0;

	rndpool_init(&rnd_pool);

	rnd_ready = 1;

#ifdef RND_VERBOSE
	printf("Random device ready\n");
#endif
}

/*
 * add a source to our list of sources
 */
void
rnd_attach_source(rs, name, tyfl)
	rndsource_element_t *rs;
	char *name;
	u_int32_t tyfl;
{
	strcpy(rs->data.name, name);

	/*
	 * force network devices to not collect any entropy by
	 * default
	 */
	if ((tyfl & 0x00ff) == RND_TYPE_NET)
		tyfl |= RND_FLAG_NO_ESTIMATE;

	rs->data.tyfl = tyfl;

	LIST_INSERT_HEAD(&rnd_sources, rs, list);

#ifdef RND_VERBOSE
	printf("%s: attached as an entropy source\n", rs->data.name);
#endif
}

/*
 * remove a source from our list of sources
 */
void
rnd_detach_source(rs)
	rndsource_element_t *rs;
{
	int	     elem;
	volatile rnd_event_t *ev;
	int          s;

	s = splhigh();

	LIST_REMOVE(rs, list);

	/*
	 * If there are events queued up, "remove" them from the event queue
	 */
	elem = rnd_tail;

	while (elem != rnd_head) {
		ev = &rnd_events[elem];
		if (ev->source == &rs->data)
			ev->source = &rnd_source_no_collect;
		elem = (elem + 1) & (RND_EVENTQSIZE - 1);
	}

	splx(s);
}
	
/*
 * Add a value to the entropy pool.  If rs is NULL no entropy estimation
 * will be performed, otherwise it should point to the source-specific
 * source structure.
 */
void
rnd_add_uint32(rs, val)
	rndsource_element_t *rs;
	u_int32_t val;
{
	rndsource_t    *rst;
	volatile rnd_event_t    *ev;
	int		s;
	int		nexthead;

	s = splhigh();

	/*
	 * check for full ring.  If the queue is full and we have not
	 * already scheduled a timeout, do so here.
	 */
	nexthead = (rnd_head + 1) & (RND_EVENTQSIZE - 1);
	if (nexthead == rnd_tail) {
		if (rnd_timeout_pending == 0) {
			rnd_timeout_pending = 1;
			timeout(rnd_timeout, NULL, 1);
		}
		splx(s);
		return;
	}

	/*
	 * If the source is null, we don't want to estimate, but we will
	 * collect.  Point to our internal source definition for this.
	 */
	if (rs == NULL)
		rst = &rnd_source_no_estimate;
	else
		rst = &rs->data;

	/*
	 * If we are not collecting any data at all, just return.
	 */
	if (rst->tyfl & RND_FLAG_NO_COLLECT) {
		splx(s);
		return;
	}

	/*
	 * Otherwise, queue it up for later addition, and schedule a
	 * timeout to process it.  Since we are at splhigh, this is
	 * an atomic operation...
	 */
	ev = &rnd_events[rnd_head];
	ev->source = rst;
	ev->val = val;
	ev->timestamp = rnd_timestamp();
	rnd_head = nexthead;

	if (rnd_timeout_pending == 0) {
		rnd_timeout_pending = 1;
		timeout(rnd_timeout, NULL, 1);
	}

	splx(s);
}

/*
 * timeout, run to process the events in the ring buffer.  Only one of these
 * can possibly be running at a time, and we are run at splsoftclock().
 */
static void
rnd_timeout(arg)
	void *arg;
{
	u_int32_t	entropy;
	volatile rnd_event_t    *ev;

	rnd_timeout_pending = 0;

	/*
	 * check for empty queue
	 */
	if (rnd_head == rnd_tail)
		return;

	/*
	 * Run through the event queue, processing events.
	 */
	while (rnd_head != rnd_tail) {
		ev = &rnd_events[rnd_tail];

		/*
		 * We repeat this check here, since it is possible the source
		 * was disabled before we were called, but after the entry
		 * was queued.
		 */
		if ((ev->source->tyfl & RND_FLAG_NO_COLLECT) == 0) {
			rndpool_add_uint32(&rnd_pool, ev->val, 0);

			/*
			 * If we are not estimating entropy from this source,
			 * assume zero.  We still add the timestamp, just
			 * don't bother calculating the estimation.
			 */
			if (ev->source->tyfl & RND_FLAG_NO_ESTIMATE)
				entropy = 0;
			else
				entropy = rnd_estimate_entropy(ev->source,
							       ev->timestamp);

			rndpool_add_uint32(&rnd_pool, ev->timestamp, entropy);
			ev->source->total += entropy;
		}
		rnd_tail = (rnd_tail + 1) & (RND_EVENTQSIZE - 1);
	}

	/*
	 * wake up any potential readers waiting.
	 */
	rnd_wakeup_readers();
}

void
rnd_add_data(rs, p, len, entropy)
	rndsource_element_t *rs;
	void *p;
	u_int32_t len;
	u_int32_t entropy;
{
	rndsource_t    *rst;
	int		s;
	u_int32_t	t;

	/*
	 * if the caller is trying to add more entropy than can possibly
	 * be in the buffer we are passed, ignore the whole thing.
	 */
	if (entropy > len * 8)
		return;

	s = splsoftclock();

	/*
	 * If the source is null, we don't want to estimate, but we will
	 * collect.
	 */
	if (rs == NULL)
		rst = &rnd_source_no_estimate;
	else
		rst = &rs->data;

	/*
	 * If we are not collecting any data at all, just return.
	 */
	if (rst->tyfl & RND_FLAG_NO_COLLECT) {
		splx(s);
		return;
	}

	rndpool_add_data(&rnd_pool, p, len, entropy);

	/*
	 * If we are not estimating timing entropy from this source, add
	 * the timestamp and assume zero entropy from timing info.
	 */
	t = rnd_timestamp();
	if (rst->tyfl & RND_FLAG_NO_ESTIMATE)
		entropy = 0;
	else
		entropy = rnd_estimate_entropy(rst, t);

	rndpool_add_uint32(&rnd_pool, t, entropy);
	rst->total += entropy;

	rnd_wakeup_readers();

	splx(s);
}

int
rnd_extract_data(p, len, flags)
	void *p;
	u_int32_t len;
	u_int32_t flags;
{
	int s;
	int retval;

	s = splsoftclock();

#if RND_USE_EXTRACT_TIME
	rndpool_add_uint32(&rnd_pool, rnd_timestamp(), 0);
#endif

	retval = rndpool_extract_data(&rnd_pool, p, len, flags);

	splx(s);

	return retval;
}
