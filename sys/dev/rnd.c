/*	$NetBSD: rnd.c,v 1.43 2004/04/25 16:42:40 simonb Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rnd.c,v 1.43 2004/04/25 16:42:40 simonb Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/rnd.h>
#include <sys/vnode.h>
#include <sys/pool.h>

#ifdef __HAVE_CPU_COUNTER
#include <machine/cpu_counter.h>
#endif

#ifdef RND_DEBUG
#define	DPRINTF(l,x)      if (rnd_debug & (l)) printf x
int	rnd_debug = 0;
#else
#define	DPRINTF(l,x)
#endif

#define	RND_DEBUG_WRITE		0x0001
#define	RND_DEBUG_READ		0x0002
#define	RND_DEBUG_IOCTL		0x0004
#define	RND_DEBUG_SNOOZE	0x0008

/*
 * list devices attached
 */
#if 0
#define	RND_VERBOSE
#endif

/*
 * Use the extraction time as a somewhat-random source
 */
#ifndef RND_USE_EXTRACT_TIME
#define	RND_USE_EXTRACT_TIME 1
#endif

/*
 * The size of a temporary buffer, malloc()ed when needed, and used for
 * reading and writing data.
 */
#define	RND_TEMP_BUFFER_SIZE	128

/*
 * This is a little bit of state information attached to each device that we
 * collect entropy from.  This is simply a collection buffer, and when it
 * is full it will be "detached" from the source and added to the entropy
 * pool after entropy is distilled as much as possible.
 */
#define	RND_SAMPLE_COUNT	64	/* collect N samples, then compress */
typedef struct _rnd_sample_t {
	SIMPLEQ_ENTRY(_rnd_sample_t) next;
	rndsource_t	*source;
	int		cursor;
	int		entropy;
	u_int32_t	ts[RND_SAMPLE_COUNT];
	u_int32_t	values[RND_SAMPLE_COUNT];
} rnd_sample_t;

/*
 * The event queue.  Fields are altered at an interrupt level.
 * All accesses must be protected at splhigh().
 */
volatile int			rnd_timeout_pending;
SIMPLEQ_HEAD(, _rnd_sample_t)	rnd_samples;

/*
 * our select/poll queue
 */
struct selinfo rnd_selq;

/*
 * Set when there are readers blocking on data from us
 */
#define	RND_READWAITING 0x00000001
volatile u_int32_t rnd_status;

/*
 * Memory pool; accessed only at splhigh().
 */
POOL_INIT(rnd_mempool, sizeof(rnd_sample_t), 0, 0, 0, "rndsample", NULL);

/*
 * Our random pool.  This is defined here rather than using the general
 * purpose one defined in rndpool.c.
 *
 * Samples are collected and queued at splhigh() into a separate queue
 * (rnd_samples, see above), and processed in a timeout routine; therefore,
 * all other accesses to the random pool must be at splsoftclock() as well.
 */
rndpool_t rnd_pool;

/*
 * This source is used to easily "remove" queue entries when the source
 * which actually generated the events is going away.
 */
static rndsource_t rnd_source_no_collect = {
	{ 'N', 'o', 'C', 'o', 'l', 'l', 'e', 'c', 't', 0, 0, 0, 0, 0, 0, 0 },
	0, 0, 0, 0,
	RND_TYPE_UNKNOWN,
	(RND_FLAG_NO_COLLECT | RND_FLAG_NO_ESTIMATE | RND_TYPE_UNKNOWN),
	NULL
};

struct callout rnd_callout = CALLOUT_INITIALIZER;

void	rndattach __P((int));

dev_type_open(rndopen);
dev_type_read(rndread);
dev_type_write(rndwrite);
dev_type_ioctl(rndioctl);
dev_type_poll(rndpoll);
dev_type_kqfilter(rndkqfilter);

const struct cdevsw rnd_cdevsw = {
	rndopen, nullclose, rndread, rndwrite, rndioctl,
	nostop, notty, rndpoll, nommap, rndkqfilter,
};

static inline void	rnd_wakeup_readers(void);
static inline u_int32_t rnd_estimate_entropy(rndsource_t *, u_int32_t);
static inline u_int32_t rnd_counter(void);
static        void	rnd_timeout(void *);

static int		rnd_ready = 0;
static int		rnd_have_entropy = 0;

LIST_HEAD(, __rndsource_element)	rnd_sources;

/*
 * Generate a 32-bit counter.  This should be more machine dependant,
 * using cycle counters and the like when possible.
 */
static inline u_int32_t
rnd_counter(void)
{
	struct timeval tv;

#ifdef __HAVE_CPU_COUNTER
	if (cpu_hascounter())
		return (cpu_counter32());
#endif
	if (rnd_ready) {
		microtime(&tv);
		return (tv.tv_sec * 1000000 + tv.tv_usec);
	} 
	/* when called from rnd_init, its too early to call microtime safely */
	return (0);
}

/*
 * Check to see if there are readers waiting on us.  If so, kick them.
 *
 * Must be called at splsoftclock().
 */
static inline void
rnd_wakeup_readers(void)
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
		selnotify(&rnd_selq, 0);

#ifdef RND_VERBOSE
		if (!rnd_have_entropy)
			printf("rnd: have initial entropy (%u)\n",
			       rndpool_get_entropy_count(&rnd_pool));
#endif
		/*
		 * Allow open of /dev/random now, too.
		 */
		rnd_have_entropy = 1;
	}
}

/*
 * Use the timing of the event to estimate the entropy gathered.
 * If all the differentials (first, second, and third) are non-zero, return
 * non-zero.  If any of these are zero, return zero.
 */
static inline u_int32_t
rnd_estimate_entropy(rndsource_t *rs, u_int32_t t)
{
	int32_t delta, delta2, delta3;

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
	 * might have something.
	 */
	if (delta == 0 || delta2 == 0 || delta3 == 0)
		return (0);

	return (1);
}

/*
 * "Attach" the random device. This is an (almost) empty stub, since
 * pseudo-devices don't get attached until after config, after the
 * entropy sources will attach. We just use the timing of this event
 * as another potential source of initial entropy.
 */
void
rndattach(int num)
{
	u_int32_t c;

	/* Trap unwary players who don't call rnd_init() early */
	KASSERT(rnd_ready);

	/* mix in another counter */
	c = rnd_counter();
	rndpool_add_data(&rnd_pool, &c, sizeof(u_int32_t), 1);
}

/*
 * initialize the global random pool for our use.
 * rnd_init() must be called very early on in the boot process, so
 * the pool is ready for other devices to attach as sources.
 */
void
rnd_init(void)
{
	u_int32_t c;

	if (rnd_ready)
		return;

	/* 
	 * take a counter early, hoping that there's some variance in
	 * the following operations 
	 */
	c = rnd_counter();

	LIST_INIT(&rnd_sources);
	SIMPLEQ_INIT(&rnd_samples);

	rndpool_init(&rnd_pool);

	/* Mix *something*, *anything* into the pool to help it get started. 
	 * However, it's not safe for rnd_counter() to call microtime() yet,
	 * so on some platforms we might just end up with zeros anyway.
	 * XXX more things to add would be nice.
	 */ 
	if (c) {
		rndpool_add_data(&rnd_pool, &c, sizeof(u_int32_t), 1);
		c = rnd_counter();
		rndpool_add_data(&rnd_pool, &c, sizeof(u_int32_t), 1);
	}

	rnd_ready = 1;

#ifdef RND_VERBOSE
	printf("rnd: initialised (%u)%s", RND_POOLBITS,
	       c ? " with counter\n" : "\n");
#endif
}

int
rndopen(dev_t dev, int flags, int ifmt, struct proc *p)
{

	if (rnd_ready == 0)
		return (ENXIO);

	if (minor(dev) == RND_DEV_URANDOM)
		return (0);

	/*
	 * If this is the strong random device and we have never collected
	 * entropy (or have not yet) don't allow it to be opened.  This will
	 * prevent waiting forever for something that just will not appear.
	 */
	if (minor(dev) == RND_DEV_RANDOM) {
		if (rnd_have_entropy == 0)
			return (ENXIO);
		else
			return (0);
	}

	return (ENXIO);
}

int
rndread(dev_t dev, struct uio *uio, int ioflag)
{
	u_int8_t *buf;
	u_int32_t entcnt, mode, n, nread;
	int ret, s;

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
		 * Copy (possibly partial) data to the user.
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
rndwrite(dev_t dev, struct uio *uio, int ioflag)
{
	u_int8_t *buf;
	int n, ret, s;

	DPRINTF(RND_DEBUG_WRITE,
	    ("Random: Write of %d requested\n", uio->uio_resid));

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

		DPRINTF(RND_DEBUG_WRITE, ("Random: Copied in %d bytes\n", n));
	}

	free(buf, M_TEMP);
	return (ret);
}

int
rndioctl(dev_t dev, u_long cmd, caddr_t addr, int flag, struct proc *p)
{
	rndsource_element_t *rse;
	rndstat_t *rst;
	rndstat_name_t *rstnm;
	rndctl_t *rctl;
	rnddata_t *rnddata;
	u_int32_t count, start;
	int ret, s;

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

	case RNDGETPOOLSTAT:
		if ((ret = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (ret);

		s = splsoftclock();
		rndpool_get_stats(&rnd_pool, addr, sizeof(rndpoolstat_t));
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
		 * Find the starting source by running through the
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
		for (count = 0; count < rst->count && rse != NULL; count++) {
			memcpy(&rst->source[count], &rse->data,
			    sizeof(rndsource_t));
			/* Zero out information which may leak */
			rst->source[count].last_time = 0;
			rst->source[count].last_delta = 0;
			rst->source[count].last_delta2 = 0;
			rst->source[count].state = 0;
			rse = rse->list.le_next;
		}

		rst->count = count;

		break;

	case RNDGETSRCNAME:
		if ((ret = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (ret);

		/*
		 * Scan through the list, trying to find the name.
		 */
		rstnm = (rndstat_name_t *)addr;
		rse = rnd_sources.lh_first;
		while (rse != NULL) {
			if (strncmp(rse->data.name, rstnm->name, 16) == 0) {
				memcpy(&rstnm->source, &rse->data,
				    sizeof(rndsource_t));

				return (0);
			}
			rse = rse->list.le_next;
		}

		ret = ENOENT;		/* name not found */

		break;

	case RNDCTL:
		if ((ret = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (ret);

		/*
		 * Set flags to enable/disable entropy counting and/or
		 * collection.
		 */
		rctl = (rndctl_t *)addr;
		rse = rnd_sources.lh_first;

		/*
		 * Flags set apply to all sources of this type.
		 */
		if (rctl->type != 0xff) {
			while (rse != NULL) {
				if (rse->data.type == rctl->type) {
					rse->data.flags &= ~rctl->mask;
					rse->data.flags |=
					    (rctl->flags & rctl->mask);
				}
				rse = rse->list.le_next;
			}

			return (0);
		}

		/*
		 * scan through the list, trying to find the name
		 */
		while (rse != NULL) {
			if (strncmp(rse->data.name, rctl->name, 16) == 0) {
				rse->data.flags &= ~rctl->mask;
				rse->data.flags |= (rctl->flags & rctl->mask);

				return (0);
			}
			rse = rse->list.le_next;
		}

		ret = ENOENT;		/* name not found */

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
rndpoll(dev_t dev, int events, struct proc *p)
{
	u_int32_t entcnt;
	int revents, s;

	/*
	 * We are always writable.
	 */
	revents = events & (POLLOUT | POLLWRNORM);

	/*
	 * Save some work if not checking for reads.
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
	 * Make certain we have enough entropy to be readable.
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

static void
filt_rnddetach(struct knote *kn)
{
	int s;

	s = splsoftclock();
	SLIST_REMOVE(&rnd_selq.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_rndread(struct knote *kn, long hint)
{
	uint32_t entcnt;

	entcnt = rndpool_get_entropy_count(&rnd_pool);
	if (entcnt >= RND_ENTROPY_THRESHOLD * 8) {
		kn->kn_data = RND_TEMP_BUFFER_SIZE;
		return (1);
	}
	return (0);
}

static const struct filterops rnd_seltrue_filtops =
	{ 1, NULL, filt_rnddetach, filt_seltrue };

static const struct filterops rndread_filtops =
	{ 1, NULL, filt_rnddetach, filt_rndread };

int
rndkqfilter(dev_t dev, struct knote *kn)
{
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &rnd_selq.sel_klist;
		if (minor(dev) == RND_DEV_URANDOM)
			kn->kn_fop = &rnd_seltrue_filtops;
		else
			kn->kn_fop = &rndread_filtops;
		break;

	case EVFILT_WRITE:
		klist = &rnd_selq.sel_klist;
		kn->kn_fop = &rnd_seltrue_filtops;
		break;

	default:
		return (1);
	}

	kn->kn_hook = NULL;

	s = splsoftclock();
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}

static rnd_sample_t *
rnd_sample_allocate(rndsource_t *source)
{
	rnd_sample_t *c;
	int s;

	s = splhigh();
	c = pool_get(&rnd_mempool, PR_WAITOK);
	splx(s);
	if (c == NULL)
		return (NULL);

	c->source = source;
	c->cursor = 0;
	c->entropy = 0;

	return (c);
}

/*
 * Don't wait on allocation.  To be used in an interrupt context.
 */
static rnd_sample_t *
rnd_sample_allocate_isr(rndsource_t *source)
{
	rnd_sample_t *c;
	int s;

	s = splhigh();
	c = pool_get(&rnd_mempool, 0);
	splx(s);
	if (c == NULL)
		return (NULL);

	c->source = source;
	c->cursor = 0;
	c->entropy = 0;

	return (c);
}

static void
rnd_sample_free(rnd_sample_t *c)
{
	int s;

	memset(c, 0, sizeof(rnd_sample_t));
	s = splhigh();
	pool_put(&rnd_mempool, c);
	splx(s);
}

/*
 * Add a source to our list of sources.
 */
void
rnd_attach_source(rndsource_element_t *rs, char *name, u_int32_t type,
    u_int32_t flags)
{
	u_int32_t ts;

	ts = rnd_counter();

	strlcpy(rs->data.name, name, sizeof(rs->data.name));
	rs->data.last_time = ts;
	rs->data.last_delta = 0;
	rs->data.last_delta2 = 0;
	rs->data.total = 0;

	/*
	 * Force network devices to not collect any entropy by
	 * default.
	 */
	if (type == RND_TYPE_NET)
		flags |= (RND_FLAG_NO_COLLECT | RND_FLAG_NO_ESTIMATE);

	rs->data.type = type;
	rs->data.flags = flags;

	rs->data.state = rnd_sample_allocate(&rs->data);

	LIST_INSERT_HEAD(&rnd_sources, rs, list);

#ifdef RND_VERBOSE
	printf("rnd: %s attached as an entropy source (", rs->data.name);
	if (!(flags & RND_FLAG_NO_COLLECT)) {
		printf("collecting");
		if (flags & RND_FLAG_NO_ESTIMATE)
			printf(" without estimation");
	}
	else
		printf("off");
	printf(")\n");
#endif

	/* 
	 * Again, put some more initial junk in the pool.
	 * XXX Bogus, but harder to guess than zeros.
	 */
	rndpool_add_data(&rnd_pool, &ts, sizeof(u_int32_t), 1);
}

/*
 * Remove a source from our list of sources.
 */
void
rnd_detach_source(rndsource_element_t *rs)
{
	rnd_sample_t *sample;
	rndsource_t *source;
	int s;

	s = splhigh();

	LIST_REMOVE(rs, list);

	source = &rs->data;

	if (source->state) {
		rnd_sample_free(source->state);
		source->state = NULL;
	}

	/*
	 * If there are samples queued up "remove" them from the sample queue
	 * by setting the source to the no-collect pseudosource.
	 */
	sample = SIMPLEQ_FIRST(&rnd_samples);
	while (sample != NULL) {
		if (sample->source == source)
			sample->source = &rnd_source_no_collect;

		sample = SIMPLEQ_NEXT(sample, next);
	}

	splx(s);
#ifdef RND_VERBOSE
	printf("rnd: %s detached as an entropy source\n", rs->data.name);
#endif
}

/*
 * Add a value to the entropy pool.  If rs is NULL no entropy estimation
 * will be performed, otherwise it should point to the source-specific
 * source structure.
 */
void
rnd_add_uint32(rndsource_element_t *rs, u_int32_t val)
{
	rndsource_t *rst;
	rnd_sample_t *state;
	u_int32_t ts;
	int s;

	/*
	 * If we are not collecting any data at all, just return.
	 */
	if (rs == NULL)
		return;

	rst = &rs->data;

	if (rst->flags & RND_FLAG_NO_COLLECT)
		return;

	/*
	 * Sample the counter as soon as possible to avoid
	 * entropy overestimation.
	 */
	ts = rnd_counter();

	/*
	 * If the sample buffer is NULL, try to allocate one here.  If this
	 * fails, drop this sample.
	 */
	state = rst->state;
	if (state == NULL) {
		state = rnd_sample_allocate_isr(rst);
		if (state == NULL)
			return;
		rst->state = state;
	}

	/*
	 * If we are estimating entropy on this source,
	 * calculate differentials.
	 */

	if ((rst->flags & RND_FLAG_NO_ESTIMATE) == 0)
		state->entropy += rnd_estimate_entropy(rst, ts);

	state->ts[state->cursor] = ts;
	state->values[state->cursor] = val;
	state->cursor++;

	/*
	 * If the state arrays are not full, we're done.
	 */
	if (state->cursor < RND_SAMPLE_COUNT)
		return;

	/*
	 * State arrays are full.  Queue this chunk on the processing queue.
	 */
	s = splhigh();
	SIMPLEQ_INSERT_HEAD(&rnd_samples, state, next);
	rst->state = NULL;

	/*
	 * If the timeout isn't pending, have it run in the near future.
	 */
	if (rnd_timeout_pending == 0) {
		rnd_timeout_pending = 1;
		callout_reset(&rnd_callout, 1, rnd_timeout, NULL);
	}
	splx(s);

	/*
	 * To get here we have to have queued the state up, and therefore
	 * we need a new state buffer.  If we can, allocate one now;
	 * if we don't get it, it doesn't matter; we'll try again on
	 * the next random event.
	 */
	rst->state = rnd_sample_allocate_isr(rst);
}

void
rnd_add_data(rndsource_element_t *rs, void *data, u_int32_t len,
    u_int32_t entropy)
{
	rndsource_t *rst;

	/* Mix in the random data directly into the pool. */
	rndpool_add_data(&rnd_pool, data, len, entropy);

	if (rs != NULL) {
		rst = &rs->data;
		rst->total += entropy;

		if ((rst->flags & RND_FLAG_NO_ESTIMATE) == 0)
			/* Estimate entropy using timing information */
			rnd_add_uint32(rs, *(u_int8_t *)data);
	}

	/* Wake up any potential readers since we've just added some data. */
	rnd_wakeup_readers();
}

/*
 * Timeout, run to process the events in the ring buffer.  Only one of these
 * can possibly be running at a time, run at splsoftclock().
 */
static void
rnd_timeout(void *arg)
{
	rnd_sample_t *sample;
	rndsource_t *source;
	u_int32_t entropy;
	int s;

	/*
	 * Sample queue is protected at splhigh(); go there briefly to dequeue.
	 */
	s = splhigh();
	rnd_timeout_pending = 0;

	sample = SIMPLEQ_FIRST(&rnd_samples);
	while (sample != NULL) {
		SIMPLEQ_REMOVE_HEAD(&rnd_samples, next);
		splx(s);

		source = sample->source;

		/*
		 * We repeat this check here, since it is possible the source
		 * was disabled before we were called, but after the entry
		 * was queued.
		 */
		if ((source->flags & RND_FLAG_NO_COLLECT) == 0) {
			rndpool_add_data(&rnd_pool, sample->values,
			    RND_SAMPLE_COUNT * 4, 0);

			entropy = sample->entropy;
			if (source->flags & RND_FLAG_NO_ESTIMATE)
				entropy = 0;

			rndpool_add_data(&rnd_pool, sample->ts,
			    RND_SAMPLE_COUNT * 4,
			    entropy);

			source->total += sample->entropy;
		}

		rnd_sample_free(sample);

		/* Go back to splhigh to dequeue the next one.. */
		s = splhigh();
		sample = SIMPLEQ_FIRST(&rnd_samples);
	}
	splx(s);

	/*
	 * Wake up any potential readers waiting.
	 */
	rnd_wakeup_readers();
}

u_int32_t
rnd_extract_data(void *p, u_int32_t len, u_int32_t flags)
{
	int retval, s;
	u_int32_t c;

	s = splsoftclock();
	if (!rnd_have_entropy) {
#ifdef RND_VERBOSE
		printf("rnd: WARNING! initial entropy low (%u).\n",
		       rndpool_get_entropy_count(&rnd_pool));
#endif
		/* Try once again to put something in the pool */
		c = rnd_counter();
		rndpool_add_data(&rnd_pool, &c, sizeof(u_int32_t), 1);
	}
	retval = rndpool_extract_data(&rnd_pool, p, len, flags);
	splx(s);

	return (retval);
}
