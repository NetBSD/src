/*	$NetBSD: rndpseudo.c,v 1.7.2.4 2013/01/16 05:33:13 yamt Exp $	*/

/*-
 * Copyright (c) 1997-2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael Graff <explorer@flame.org> and Thor Lancelot Simon.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
__KERNEL_RCSID(0, "$NetBSD: rndpseudo.c,v 1.7.2.4 2013/01/16 05:33:13 yamt Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/kmem.h>
#include <sys/atomic.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/pool.h>
#include <sys/kauth.h>
#include <sys/cprng.h>
#include <sys/cpu.h>
#include <sys/stat.h>

#include <sys/rnd.h>
#ifdef COMPAT_50
#include <compat/sys/rnd.h>
#endif

#include <dev/rnd_private.h>

#if defined(__HAVE_CPU_COUNTER) && !defined(_RUMPKERNEL) /* XXX: bad pooka */
#include <machine/cpu_counter.h>
#endif

#ifdef RND_DEBUG
#define	DPRINTF(l,x)      if (rnd_debug & (l)) printf x
extern int rnd_debug;
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
 * The size of a temporary buffer, kmem_alloc()ed when needed, and used for
 * reading and writing data.
 */
#define	RND_TEMP_BUFFER_SIZE	512

static pool_cache_t rp_pc;
static pool_cache_t rp_cpc;

/*
 * The per-CPU RNGs used for short requests
 */
cprng_strong_t **rp_cpurngs;

/*
 * Our random pool.  This is defined here rather than using the general
 * purpose one defined in rndpool.c.
 *
 * Samples are collected and queued into a separate mutex-protected queue
 * (rnd_samples, see above), and processed in a timeout routine; therefore,
 * the mutex protecting the random pool is at IPL_SOFTCLOCK() as well.
 */
extern rndpool_t rnd_pool;
extern kmutex_t  rndpool_mtx;

void	rndattach(int);

dev_type_open(rndopen);

const struct cdevsw rnd_cdevsw = {
	rndopen, noclose, noread, nowrite, noioctl, nostop,
	notty, nopoll, nommap, nokqfilter, D_OTHER|D_MPSAFE
};

static int rnd_read(struct file *, off_t *, struct uio *, kauth_cred_t, int);
static int rnd_write(struct file *, off_t *, struct uio *, kauth_cred_t, int);
static int rnd_ioctl(struct file *, u_long, void *);
static int rnd_poll(struct file *, int);
static int rnd_stat(struct file *, struct stat *);
static int rnd_close(struct file *);
static int rnd_kqfilter(struct file *, struct knote *);

const struct fileops rnd_fileops = {
	.fo_read = rnd_read,
	.fo_write = rnd_write,
	.fo_ioctl = rnd_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = rnd_poll,
	.fo_stat = rnd_stat,
	.fo_close = rnd_close,
	.fo_kqfilter = rnd_kqfilter,
	.fo_restart = fnullop_restart
};

void			rnd_wakeup_readers(void);	/* XXX */
extern int		rnd_ready;		/* XXX */
extern rndsave_t	*boot_rsp;		/* XXX */
extern LIST_HEAD(, krndsource) rnd_sources;	/* XXX */

/*
 * Generate a 32-bit counter.  This should be more machine dependent,
 * using cycle counters and the like when possible.
 */
static inline u_int32_t
rndpseudo_counter(void)
{
	struct timeval tv;

#if defined(__HAVE_CPU_COUNTER) && !defined(_RUMPKERNEL) /* XXX: bad pooka */
	if (cpu_hascounter())
		return (cpu_counter32());
#endif
	microtime(&tv);
	return (tv.tv_sec * 1000000 + tv.tv_usec);
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

	rp_pc = pool_cache_init(RND_TEMP_BUFFER_SIZE, 0, 0, 0,
				"rndtemp", NULL, IPL_NONE,
				NULL, NULL, NULL);
	rp_cpc = pool_cache_init(sizeof(rp_ctx_t), 0, 0, 0,
				 "rndctx", NULL, IPL_NONE,
				 NULL, NULL, NULL);

	/* mix in another counter */
	c = rndpseudo_counter();
	mutex_spin_enter(&rndpool_mtx);
	rndpool_add_data(&rnd_pool, &c, sizeof(u_int32_t), 1);
	mutex_spin_exit(&rndpool_mtx);

	rp_cpurngs = kmem_zalloc(maxcpus * sizeof(cprng_strong_t *),
				 KM_SLEEP);
}

int
rndopen(dev_t dev, int flag, int ifmt,
    struct lwp *l)
{
	rp_ctx_t *ctx;
	file_t *fp;
	int fd, hard, error = 0;

	switch (minor(dev)) {
	    case RND_DEV_URANDOM:
		hard = 0;
		break;
	    case RND_DEV_RANDOM:
		hard = 1;
		break;
	    default:
		return ENXIO;
	}
	ctx = pool_cache_get(rp_cpc, PR_WAITOK);	
	if ((error = fd_allocfile(&fp, &fd)) != 0) {
	    pool_cache_put(rp_cpc, ctx);
	    return error;
	}
	ctx->cprng = NULL;
	ctx->hard = hard;
	mutex_init(&ctx->interlock, MUTEX_DEFAULT, IPL_NONE);

	return fd_clone(fp, fd, flag, &rnd_fileops, ctx);
}

static void
rnd_alloc_cprng(rp_ctx_t *ctx)
{
	char personalization_buf[64];
	struct lwp *l = curlwp;
	int cflags = ctx->hard ? CPRNG_USE_CV :
				 CPRNG_INIT_ANY|CPRNG_REKEY_ANY;

	mutex_enter(&ctx->interlock);
	if (__predict_true(ctx->cprng == NULL)) {
		snprintf(personalization_buf,
			 sizeof(personalization_buf),
	 		 "%d%llud%d", l->l_proc->p_pid,
	 		 (unsigned long long int)l->l_ncsw, l->l_cpticks);
		ctx->cprng = cprng_strong_create(personalization_buf,
						 IPL_NONE, cflags);
	}
	membar_sync();
	mutex_exit(&ctx->interlock);
}

static int
rnd_read(struct file * fp, off_t *offp, struct uio *uio,
	  kauth_cred_t cred, int flags)
{
	rp_ctx_t *ctx = fp->f_data;
	cprng_strong_t *cprng;
	u_int8_t *bf;
	int strength, ret;
	struct cpu_info *ci = curcpu();

	DPRINTF(RND_DEBUG_READ,
	    ("Random:  Read of %zu requested, flags 0x%08x\n",
	    uio->uio_resid, flags));

	if (uio->uio_resid == 0)
		return (0);

	if (ctx->hard || uio->uio_resid > NIST_BLOCK_KEYLEN_BYTES) {
		if (ctx->cprng == NULL) {
			rnd_alloc_cprng(ctx);
		}
		cprng = ctx->cprng;
	} else {
		int index = cpu_index(ci);

		if (__predict_false(rp_cpurngs[index] == NULL)) {
			char rngname[32];

			snprintf(rngname, sizeof(rngname),
				 "%s-short", cpu_name(ci));
			rp_cpurngs[index] =
			    cprng_strong_create(rngname, IPL_NONE,
						CPRNG_INIT_ANY |
						CPRNG_REKEY_ANY);
		}
		cprng = rp_cpurngs[index];
	}

	if (__predict_false(cprng == NULL)) {
		printf("NULL rng!\n");
		return EIO;
        }

	strength = cprng_strong_strength(cprng);
	ret = 0;
	bf = pool_cache_get(rp_pc, PR_WAITOK);
	while (uio->uio_resid > 0) {
		int n, nread, want;

		want = MIN(RND_TEMP_BUFFER_SIZE, uio->uio_resid);

		/* XXX is this _really_ what's wanted? */
		if (ctx->hard) {
			n = MIN(want, strength - ctx->bytesonkey);
			if (n < 1) {
			    cprng_strong_deplete(cprng);
			    n = MIN(want, strength);
			    ctx->bytesonkey = 0;
			    membar_producer();
			}
		} else {
			n = want;
		}

		nread = cprng_strong(cprng, bf, n,
				     (fp->f_flag & FNONBLOCK) ? FNONBLOCK : 0);

		if (ctx->hard && nread > 0) {
			atomic_add_int(&ctx->bytesonkey, nread);
		}
		if (nread < 1) {
			if (fp->f_flag & FNONBLOCK) {
				ret = EWOULDBLOCK;
			} else {
				ret = EINTR;
			}
			goto out;
		}

		ret = uiomove((void *)bf, nread, uio);
		if (ret != 0 || n < want) {
			goto out;
		}
	}
out:
	pool_cache_put(rp_pc, bf);
	return (ret);
}

static int
rnd_write(struct file *fp, off_t *offp, struct uio *uio,
	   kauth_cred_t cred, int flags)
{
	u_int8_t *bf;
	int n, ret = 0, estimate_ok = 0, estimate = 0, added = 0;

	ret = kauth_authorize_device(cred,
	    KAUTH_DEVICE_RND_ADDDATA, NULL, NULL, NULL, NULL);
	if (ret) {
		return (ret);
	}
	estimate_ok = !kauth_authorize_device(cred,
	    KAUTH_DEVICE_RND_ADDDATA_ESTIMATE, NULL, NULL, NULL, NULL);

	DPRINTF(RND_DEBUG_WRITE,
	    ("Random: Write of %zu requested\n", uio->uio_resid));

	if (uio->uio_resid == 0)
		return (0);
	ret = 0;
	bf = pool_cache_get(rp_pc, PR_WAITOK);
	while (uio->uio_resid > 0) {
		/*
		 * Don't flood the pool.
		 */
		if (added > RND_POOLWORDS * sizeof(int)) {
#ifdef RND_VERBOSE
			printf("rnd: added %d already, adding no more.\n",
			       added);
#endif
			break;
		}
		n = min(RND_TEMP_BUFFER_SIZE, uio->uio_resid);

		ret = uiomove((void *)bf, n, uio);
		if (ret != 0)
			break;

		if (estimate_ok) {
			/*
			 * Don't cause samples to be discarded by taking
			 * the pool's entropy estimate to the max.
			 */
			if (added > RND_POOLWORDS / 2)
				estimate = 0;
			else
				estimate = n * NBBY / 2;
#ifdef RND_VERBOSE
			printf("rnd: adding on write, %d bytes, estimate %d\n",
			       n, estimate);
#endif
		} else {
#ifdef RND_VERBOSE
			printf("rnd: kauth says no entropy.\n");
#endif
		}

		/*
		 * Mix in the bytes.
		 */
		mutex_spin_enter(&rndpool_mtx);
		rndpool_add_data(&rnd_pool, bf, n, estimate);
		mutex_spin_exit(&rndpool_mtx);

		added += n;
		DPRINTF(RND_DEBUG_WRITE, ("Random: Copied in %d bytes\n", n));
	}
	pool_cache_put(rp_pc, bf);
	return (ret);
}

static void
krndsource_to_rndsource(krndsource_t *kr, rndsource_t *r)
{
	memset(r, 0, sizeof(*r));
	strlcpy(r->name, kr->name, sizeof(r->name));
        r->total = kr->total;
        r->type = kr->type;
        r->flags = kr->flags;
}

int
rnd_ioctl(struct file *fp, u_long cmd, void *addr)
{
	krndsource_t *kr;
	rndstat_t *rst;
	rndstat_name_t *rstnm;
	rndctl_t *rctl;
	rnddata_t *rnddata;
	u_int32_t count, start;
	int ret = 0;
	int estimate_ok = 0, estimate = 0;

	switch (cmd) {
	case FIONBIO:
	case FIOASYNC:
	case RNDGETENTCNT:
		break;

	case RNDGETPOOLSTAT:
	case RNDGETSRCNUM:
	case RNDGETSRCNAME:
		ret = kauth_authorize_device(curlwp->l_cred,
		    KAUTH_DEVICE_RND_GETPRIV, NULL, NULL, NULL, NULL);
		if (ret)
			return (ret);
		break;

	case RNDCTL:
		ret = kauth_authorize_device(curlwp->l_cred,
		    KAUTH_DEVICE_RND_SETPRIV, NULL, NULL, NULL, NULL);
		if (ret)
			return (ret);
		break;

	case RNDADDDATA:
		ret = kauth_authorize_device(curlwp->l_cred,
		    KAUTH_DEVICE_RND_ADDDATA, NULL, NULL, NULL, NULL);
		if (ret)
			return (ret);
		estimate_ok = !kauth_authorize_device(curlwp->l_cred,
		    KAUTH_DEVICE_RND_ADDDATA_ESTIMATE, NULL, NULL, NULL, NULL);
		break;

	default:
#ifdef COMPAT_50
		return compat_50_rnd_ioctl(fp, cmd, addr);
#else
		return ENOTTY;
#endif
	}

	switch (cmd) {

	/*
	 * Handled in upper layer really, but we have to return zero
	 * for it to be accepted by the upper layer.
	 */
	case FIONBIO:
	case FIOASYNC:
		break;

	case RNDGETENTCNT:
		mutex_spin_enter(&rndpool_mtx);
		*(u_int32_t *)addr = rndpool_get_entropy_count(&rnd_pool);
		mutex_spin_exit(&rndpool_mtx);
		break;

	case RNDGETPOOLSTAT:
		mutex_spin_enter(&rndpool_mtx);
		rndpool_get_stats(&rnd_pool, addr, sizeof(rndpoolstat_t));
		mutex_spin_exit(&rndpool_mtx);
		break;

	case RNDGETSRCNUM:
		rst = (rndstat_t *)addr;

		if (rst->count == 0)
			break;

		if (rst->count > RND_MAXSTATCOUNT)
			return (EINVAL);

		mutex_spin_enter(&rndpool_mtx);
		/*
		 * Find the starting source by running through the
		 * list of sources.
		 */
		kr = rnd_sources.lh_first;
		start = rst->start;
		while (kr != NULL && start >= 1) {
			kr = kr->list.le_next;
			start--;
		}

		/*
		 * Return up to as many structures as the user asked
		 * for.  If we run out of sources, a count of zero
		 * will be returned, without an error.
		 */
		for (count = 0; count < rst->count && kr != NULL; count++) {
			krndsource_to_rndsource(kr, &rst->source[count]);
			kr = kr->list.le_next;
		}

		rst->count = count;

		mutex_spin_exit(&rndpool_mtx);
		break;

	case RNDGETSRCNAME:
		/*
		 * Scan through the list, trying to find the name.
		 */
		mutex_spin_enter(&rndpool_mtx);
		rstnm = (rndstat_name_t *)addr;
		kr = rnd_sources.lh_first;
		while (kr != NULL) {
			if (strncmp(kr->name, rstnm->name,
				    MIN(sizeof(kr->name),
					sizeof(*rstnm))) == 0) {
				krndsource_to_rndsource(kr, &rstnm->source);
				mutex_spin_exit(&rndpool_mtx);
				return (0);
			}
			kr = kr->list.le_next;
		}
		mutex_spin_exit(&rndpool_mtx);

		ret = ENOENT;		/* name not found */

		break;

	case RNDCTL:
		/*
		 * Set flags to enable/disable entropy counting and/or
		 * collection.
		 */
		mutex_spin_enter(&rndpool_mtx);
		rctl = (rndctl_t *)addr;
		kr = rnd_sources.lh_first;

		/*
		 * Flags set apply to all sources of this type.
		 */
		if (rctl->type != 0xff) {
			while (kr != NULL) {
				if (kr->type == rctl->type) {
					kr->flags &= ~rctl->mask;
					kr->flags |=
					    (rctl->flags & rctl->mask);
				}
				kr = kr->list.le_next;
			}
			mutex_spin_exit(&rndpool_mtx);
			return (0);
		}

		/*
		 * scan through the list, trying to find the name
		 */
		while (kr != NULL) {
			if (strncmp(kr->name, rctl->name,
				    MIN(sizeof(kr->name),
                                        sizeof(rctl->name))) == 0) {
				kr->flags &= ~rctl->mask;
				kr->flags |= (rctl->flags & rctl->mask);
				
				mutex_spin_exit(&rndpool_mtx);
				return (0);
			}
			kr = kr->list.le_next;
		}

		mutex_spin_exit(&rndpool_mtx);
		ret = ENOENT;		/* name not found */
		
		break;

	case RNDADDDATA:
		/*
		 * Don't seed twice if our bootloader has
		 * seed loading support.
		 */
		if (!boot_rsp) {
			rnddata = (rnddata_t *)addr;

			if (rnddata->len > sizeof(rnddata->data))
				return EINVAL;

			if (estimate_ok) {
				/*
				 * Do not accept absurd entropy estimates, and
				 * do not flood the pool with entropy such that
				 * new samples are discarded henceforth.
				 */
				estimate = MIN((rnddata->len * NBBY) / 2,
					       MIN(rnddata->entropy,
						   RND_POOLBITS / 2));
			} else {
				estimate = 0;
			}

			mutex_spin_enter(&rndpool_mtx);
			rndpool_add_data(&rnd_pool, rnddata->data,
					 rnddata->len, estimate);
			mutex_spin_exit(&rndpool_mtx);

			rnd_wakeup_readers();
		}
#ifdef RND_VERBOSE
		else {
			printf("rnd: already seeded by boot loader\n");
		}
#endif
		break;

	default:
		return ENOTTY;
	}

	return (ret);
}

static int
rnd_poll(struct file *fp, int events)
{
	int revents;
	rp_ctx_t *ctx = fp->f_data;

	/*
	 * We are always writable.
	 */
	revents = events & (POLLOUT | POLLWRNORM);

	/*
	 * Save some work if not checking for reads.
	 */
	if ((events & (POLLIN | POLLRDNORM)) == 0)
		return (revents);

	if (ctx->cprng == NULL) {
		rnd_alloc_cprng(ctx);
		if (__predict_false(ctx->cprng == NULL)) {
			return EIO;
		}       
	}

	if (cprng_strong_ready(ctx->cprng)) {
		revents |= events & (POLLIN | POLLRDNORM);
	} else {
		mutex_enter(&ctx->cprng->mtx);
		selrecord(curlwp, &ctx->cprng->selq);
		mutex_exit(&ctx->cprng->mtx);
	}

	return (revents);
}

static int
rnd_stat(struct file *fp, struct stat *st)
{
	rp_ctx_t *ctx = fp->f_data;

	/* XXX lock, if cprng allocated?  why? */
	memset(st, 0, sizeof(*st));
	st->st_dev = makedev(cdevsw_lookup_major(&rnd_cdevsw),
						 ctx->hard ? RND_DEV_RANDOM :
						 RND_DEV_URANDOM);
	/* XXX leave atimespect, mtimespec, ctimespec = 0? */

	st->st_uid = kauth_cred_geteuid(fp->f_cred);
	st->st_gid = kauth_cred_getegid(fp->f_cred);
	st->st_mode = S_IFCHR;
	return 0;
}

static int
rnd_close(struct file *fp)
{
	rp_ctx_t *ctx = fp->f_data;

	if (ctx->cprng) {
		cprng_strong_destroy(ctx->cprng);
	}
	fp->f_data = NULL;
	mutex_destroy(&ctx->interlock);
	pool_cache_put(rp_cpc, ctx);

	return 0;
}

static void
filt_rnddetach(struct knote *kn)
{
	cprng_strong_t *c = kn->kn_hook;

	mutex_enter(&c->mtx);
	SLIST_REMOVE(&c->selq.sel_klist, kn, knote, kn_selnext);
	mutex_exit(&c->mtx);
}

static int
filt_rndread(struct knote *kn, long hint)
{
	cprng_strong_t *c = kn->kn_hook;

	if (cprng_strong_ready(c)) {
		kn->kn_data = RND_TEMP_BUFFER_SIZE;
		return 1;
	}
	return 0;
}

static const struct filterops rnd_seltrue_filtops =
	{ 1, NULL, filt_rnddetach, filt_seltrue };

static const struct filterops rndread_filtops =
	{ 1, NULL, filt_rnddetach, filt_rndread };

static int
rnd_kqfilter(struct file *fp, struct knote *kn)
{
	rp_ctx_t *ctx = fp->f_data;
	struct klist *klist;

	if (ctx->cprng == NULL) {
		rnd_alloc_cprng(ctx);
		if (__predict_false(ctx->cprng == NULL)) {
			return EIO;
		}
	}

	mutex_enter(&ctx->cprng->mtx);
	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &ctx->cprng->selq.sel_klist;
		kn->kn_fop = &rndread_filtops;
		break;

	case EVFILT_WRITE:
		klist = &ctx->cprng->selq.sel_klist;
		kn->kn_fop = &rnd_seltrue_filtops;
		break;

	default:
		mutex_exit(&ctx->cprng->mtx);
		return EINVAL;
	}

	kn->kn_hook = ctx->cprng;

	SLIST_INSERT_HEAD(klist, kn, kn_selnext);

	mutex_exit(&ctx->cprng->mtx);
	return (0);
}
