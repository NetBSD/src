/*	$NetBSD: rnd.c,v 1.5 1997/10/13 18:35:15 explorer Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael Graff <explorer@flame.org>.  This code is derived from the
 * random driver written by Ted Ts'o.
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

/*
 * This is the minimum amount of random data we can have and be
 * considered "readable"
 */
#define RND_ENTROPY_THRESHOLD	64

/*
 * our select/poll queue
 */
struct selinfo rnd_selq;

/*
 * Set when there are readers blocking on data from us
 */
#define RND_READWAITING 0x00000001
u_int32_t  rnd_status;

void	rndattach __P((int));
int	rndopen __P((dev_t, int, int, struct proc *));
int	rndclose __P((dev_t, int, int, struct proc *));
int	rndread __P((dev_t, struct uio *, int));
int	rndwrite __P((dev_t, struct uio *, int));
int	rndioctl __P((dev_t, u_long, caddr_t, int, struct proc *));
int	rndpoll __P((dev_t, int, struct proc *));

static inline u_int32_t rnd_estimate_entropy(rndsource_t *, u_int32_t *);

/*
 * Use the timing of the event to estimate the entropy gathered.
 * Note that right now we will return either one or two, depending on
 * if all the differentials (first, second, and third) are non-zero.
 */
static inline u_int32_t
rnd_estimate_entropy(rs, t)
	rndsource_t *rs;
	u_int32_t *t;
{
	struct timeval	tv;
	int32_t		delta;
	int32_t		delta2;
	int32_t		delta3;

	microtime(&tv);

	*t = tv.tv_sec * 1000000 + tv.tv_usec;

	if (*t < rs->last_time)
		delta = UINT_MAX - rs->last_time + *t;
	else
		delta = rs->last_time - *t;

	if (delta < 0)
		delta = -delta;

	delta2 = rs->last_delta - delta;
	if (delta2 < 0)
		delta2 = -delta2;

	delta3 = rs->last_delta2 - delta2;
	if (delta3 < 0)
		delta3 = -delta3;

	rs->last_time = *t;
	rs->last_delta = delta;
	rs->last_delta2 = delta2;

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

			s = splhigh();
			entcnt = rndpool_get_entropy_count(NULL);
			splx(s);

			if (entcnt >= RND_ENTROPY_THRESHOLD)
				break;

			/*
			 * Data is not available.
			 */
			if (ioflag & IO_NDELAY) {
				ret = EWOULDBLOCK;
				goto out;
			}

			rnd_status |= RND_READWAITING;
			ret = tsleep(&rnd_status, PRIBIO|PCATCH,
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
		rnd_add_data(NULL, buf, n, 0);

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
		*(u_int32_t *)addr = rndpool_get_entropy_count(NULL);

		break;

	case RNDGETPOOL:
		if ((ret = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (ret);

		bcopy(rndpool_get_pool(NULL), addr, rndpool_get_poolsize());

		break;

	case RNDADDTOENTCNT:
		if ((ret = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (ret);

		rndpool_increment_entropy_count(NULL, *(u_int32_t *)addr);

		break;

	case RNDSETENTCNT:
		if ((ret = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (ret);

		rndpool_set_entropy_count(NULL, *(u_int32_t *)addr);

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
		rnd_add_data(NULL, rnddata->data, rnddata->len,
			     rnddata->entropy);

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
	 *  we are always writable
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
	s = splhigh();
	entcnt = rndpool_get_entropy_count(NULL);
	splx(s);

	if (entcnt >= RND_ENTROPY_THRESHOLD)
		revents |= events & (POLLIN | POLLRDNORM);
	else
		selrecord(p, &rnd_selq);

	return (revents);
}

/*
 * always called at splhigh() or something else safe
 */
void
rnd_init(void)
{
	if (rnd_ready)
		return;

	LIST_INIT(&rnd_sources);

	rndpool_init_global();

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
 * Add a value to the entropy pool.  If rs is NULL no entropy estimation
 * will be performed, otherwise it should point to the source-specific
 * source structure.
 */
void
rnd_add_uint32(rs, val)
	rndsource_element_t *rs;
	u_int32_t val;
{
	int		s;
	u_int32_t	entropy;
	u_int32_t	t;

	s = splhigh();

	/*
	 * if the source is NULL, add the byte given and return.
	 */
	if (rs == NULL) {
		rndpool_add_uint32(NULL, val, 0);
		splx(s);
		return;
	}

	/*
	 * If we are not collecting any data at all, just return.
	 */
	if (rs->data.tyfl & RND_FLAG_NO_COLLECT) {
		splx(s);
		return;
	}

	rndpool_add_uint32(NULL, val, 0);

	/*
	 * If we are not estimating entropy from this source, we are done.
	 */
	if (rs->data.tyfl & RND_FLAG_NO_ESTIMATE) {
		splx(s);
		return;
	}

	entropy = rnd_estimate_entropy(&rs->data, &t);

	if (entropy) {
		rndpool_add_uint32(NULL, t, entropy);
		rs->data.total += entropy;

		/*
		 * if we have enough bits to do something useful,
		 * wake up sleeping readers.
		 */
		if (rndpool_get_entropy_count(NULL) > RND_ENTROPY_THRESHOLD) {
			if (rnd_status & RND_READWAITING) {

				DPRINTF(RND_DEBUG_SNOOZE,
					("waking up pending readers.\n"));

				rnd_status &= ~RND_READWAITING;
				wakeup(&rnd_status);
			}
			selwakeup(&rnd_selq);
		}
	}

	splx(s);
}

void
rnd_add_data(rs, p, len, entropy)
	rndsource_element_t *rs;
	void *p;
	u_int32_t len;
	u_int32_t entropy;
{
	int		s;
	u_int32_t	t;

	/*
	 * if the caller is trying to add more entropy than can possibly
	 * be in the buffer we are passed, ignore the whole thing.
	 */
	if (entropy > len * 8)
		return;

	s = splhigh();

	/*
	 * if the source is NULL, add the data and return.
	 */
	if (rs == NULL) {
		rndpool_add_data(NULL, p, len, entropy);
		splx(s);
		return;
	}

	/*
	 * If we are not collecting any data at all, just return.
	 */
	if (rs->data.tyfl & RND_FLAG_NO_COLLECT) {
		splx(s);
		return;
	}

	rndpool_add_data(NULL, p, len, entropy);

	/*
	 * If we are not estimating entropy from this source, we are done.
	 */
	if (rs->data.tyfl & RND_FLAG_NO_ESTIMATE) {
		splx(s);
		return;
	}

	entropy = rnd_estimate_entropy(&rs->data, &t);

	if (entropy) {
		rndpool_add_uint32(NULL, t, entropy);
		rs->data.total += entropy;

		/*
		 * if we have enough bits to do something useful,
		 * wake up sleeping readers.
		 */
		if (rndpool_get_entropy_count(NULL) > RND_ENTROPY_THRESHOLD) {
			if (rnd_status & RND_READWAITING) {

				DPRINTF(RND_DEBUG_SNOOZE,
					("waking up pending readers.\n"));

				rnd_status &= ~RND_READWAITING;
				wakeup(&rnd_status);
			}
			selwakeup(&rnd_selq);
		}
	}

	splx(s);
}

int
rnd_extract_data(p, len, flags)
	void *p;
	u_int32_t len;
	u_int32_t flags;
{
	struct timeval tv;
	int s;
	int retval;

	s = splhigh();

#if RND_USE_EXTRACT_TIME
	microtime(&tv);
	rndpool_add_uint32(NULL, tv.tv_usec, 0);
#endif

	retval = rndpool_extract_data(NULL, p, len, flags);

	splx(s);

	return retval;
}
