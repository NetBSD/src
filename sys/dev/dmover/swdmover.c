/*	$NetBSD: swdmover.c,v 1.4 2003/03/06 21:10:45 thorpej Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * swdmover.c: Software back-end providing the dmover functions
 * mentioned in dmover(9).
 *
 * This module provides a fallback for cases where no hardware
 * data movers are present in a system, and also serves an an
 * example of how to write a dmover back-end.
 *
 * Note that even through the software dmover doesn't require
 * interrupts to be blocked, we block them anyway to demonstrate
 * the locking protocol.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: swdmover.c,v 1.4 2003/03/06 21:10:45 thorpej Exp $");

#include <sys/param.h>
#include <sys/lock.h>
#include <sys/kthread.h>
#include <sys/systm.h>
#include <sys/uio.h>

#include <dev/dmover/dmovervar.h>

struct swdmover_function {
	void	(*sdf_process)(struct dmover_request *);
};

static struct dmover_backend swdmover_backend;
static struct proc *swdmover_proc;
static int swdmover_cv;

void	swdmoverattach(int);

/*
 * swdmover_process:
 *
 *	Dmover back-end entry point.
 */
static void
swdmover_process(struct dmover_backend *dmb)
{
	int s;

	/*
	 * Just wake up the processing thread.  This will allow
	 * requests to linger on the middle-end's queue so that
	 * they can be cancelled, if need-be.
	 */
	s = splbio();
	/* XXXLOCK */
	if (TAILQ_EMPTY(&dmb->dmb_pendreqs) == 0)
		wakeup(&swdmover_cv);
	/* XXXUNLOCK */
	splx(s);
}

/*
 * swdmover_thread:
 *
 *	Request processing thread.
 */
static void
swdmover_thread(void *arg)
{
	struct dmover_backend *dmb = arg;
	struct dmover_request *dreq;
	struct swdmover_function *sdf;
	int s;

	s = splbio();
	/* XXXLOCK */

	for (;;) {
		dreq = TAILQ_FIRST(&dmb->dmb_pendreqs);
		if (dreq == NULL) {
			/* XXXUNLOCK */
			(void) tsleep(&swdmover_cv, PRIBIO, "swdmvr", 0);
			continue;
		}

		dmover_backend_remque(dmb, dreq);
		dreq->dreq_flags |= DMOVER_REQ_RUNNING;

		/* XXXUNLOCK */
		splx(s);

		sdf = dreq->dreq_assignment->das_algdesc->dad_data;
		(*sdf->sdf_process)(dreq);

		s = splbio();
		/* XXXLOCK */
	}
}

/*
 * swdmover_func_zero_process:
 *
 *	Processing routine for the "zero" function.
 */
static void
swdmover_func_zero_process(struct dmover_request *dreq)
{

	switch (dreq->dreq_outbuf_type) {
	case DMOVER_BUF_LINEAR:
		memset(dreq->dreq_outbuf.dmbuf_linear.l_addr, 0,
		    dreq->dreq_outbuf.dmbuf_linear.l_len);
		break;

	case DMOVER_BUF_UIO:
	    {
		struct uio *uio = dreq->dreq_outbuf.dmbuf_uio;
		char *cp;
		size_t count, buflen;
		int error;

		if (uio->uio_rw != UIO_READ) {
			/* XXXLOCK */
			dreq->dreq_error = EINVAL;
			dreq->dreq_flags |= DMOVER_REQ_ERROR;
			/* XXXUNLOCK */
			break;
		}

		buflen = uio->uio_resid;
		if (buflen > 1024)
			buflen = 1024;
		cp = alloca(buflen);
		memset(cp, 0, buflen);

		while ((count = uio->uio_resid) != 0) {
			if (count > buflen)
				count = buflen;
			error = uiomove(cp, count, uio);
			if (error) {
				/* XXXLOCK */
				dreq->dreq_error = error;
				dreq->dreq_flags |= DMOVER_REQ_ERROR;
				/* XXXUNLOCK */
				break;
			}
		}
		break;
	    }

	default:
		/* XXXLOCK */
		dreq->dreq_error = EINVAL;
		dreq->dreq_flags |= DMOVER_REQ_ERROR;
		/* XXXUNLOCK */
	}

	dmover_done(dreq);
}

/*
 * swdmover_func_fill8_process:
 *
 *	Processing routine for the "fill8" function.
 */
static void
swdmover_func_fill8_process(struct dmover_request *dreq)
{

	switch (dreq->dreq_outbuf_type) {
	case DMOVER_BUF_LINEAR:
		memset(dreq->dreq_outbuf.dmbuf_linear.l_addr,
		    dreq->dreq_immediate[0],
		    dreq->dreq_outbuf.dmbuf_linear.l_len);
		break;

	case DMOVER_BUF_UIO:
	    {
		struct uio *uio = dreq->dreq_outbuf.dmbuf_uio;
		char *cp;
		size_t count, buflen;
		int error;

		if (uio->uio_rw != UIO_READ) {
			/* XXXLOCK */
			dreq->dreq_error = EINVAL;
			dreq->dreq_flags |= DMOVER_REQ_ERROR;
			/* XXXUNLOCK */
			break;
		}

		buflen = uio->uio_resid;
		if (buflen > 1024)
			buflen = 1024;
		cp = alloca(buflen);
		memset(cp, dreq->dreq_immediate[0], buflen);

		while ((count = uio->uio_resid) != 0) {
			if (count > buflen)
				count = buflen;
			error = uiomove(cp, count, uio);
			if (error) {
				/* XXXLOCK */
				dreq->dreq_error = error;
				dreq->dreq_flags |= DMOVER_REQ_ERROR;
				/* XXXUNLOCK */
				break;
			}
		}
		break;
	    }

	default:
		/* XXXLOCK */
		dreq->dreq_error = EINVAL;
		dreq->dreq_flags |= DMOVER_REQ_ERROR;
		/* XXXUNLOCK */
	}

	dmover_done(dreq);
}

/*
 * swdmover_func_copy_process:
 *
 *	Processing routine for the "copy" function.
 */
static void
swdmover_func_copy_process(struct dmover_request *dreq)
{

	/* XXX Currently, both buffers must be of same type. */
	if (dreq->dreq_inbuf_type != dreq->dreq_outbuf_type) {
		/* XXXLOCK */
		dreq->dreq_error = EINVAL;
		dreq->dreq_flags |= DMOVER_REQ_ERROR;
		/* XXXUNLOCK */
		goto done;
	}

	switch (dreq->dreq_outbuf_type) {
	case DMOVER_BUF_LINEAR:
		if (dreq->dreq_outbuf.dmbuf_linear.l_len !=
		    dreq->dreq_inbuf[0].dmbuf_linear.l_len) {
			/* XXXLOCK */
			dreq->dreq_error = EINVAL;
			dreq->dreq_flags |= DMOVER_REQ_ERROR;
			/* XXXUNLOCK */
			break;
		}
		memcpy(dreq->dreq_outbuf.dmbuf_linear.l_addr,
		    dreq->dreq_inbuf[0].dmbuf_linear.l_addr,
		    dreq->dreq_outbuf.dmbuf_linear.l_len);
		break;

	case DMOVER_BUF_UIO:
	    {
		struct uio *uio_out = dreq->dreq_outbuf.dmbuf_uio;
		struct uio *uio_in = dreq->dreq_inbuf[0].dmbuf_uio;
		char *cp;
		size_t count, buflen;
		int error;

		if (uio_in->uio_rw != UIO_WRITE ||
		    uio_out->uio_rw != UIO_READ ||
		    uio_in->uio_resid != uio_out->uio_resid) {
			/* XXXLOCK */
			dreq->dreq_error = EINVAL;
			dreq->dreq_flags |= DMOVER_REQ_ERROR;
			/* XXXUNLOCK */
			break;
		}

		buflen = uio_in->uio_resid;
		if (buflen > 1024)
			buflen = 1024;
		cp = alloca(buflen);

		while ((count = uio_in->uio_resid) != 0) {
			if (count > buflen)
				count = buflen;
			error = uiomove(cp, count, uio_in);
			if (error == 0)
				error = uiomove(cp, count, uio_out);
			if (error) {
				/* XXXLOCK */
				dreq->dreq_error = error;
				dreq->dreq_flags |= DMOVER_REQ_ERROR;
				/* XXXUNLOCK */
				break;
			}
		}
		break;
	    }

	default:
		/* XXXLOCK */
		dreq->dreq_error = EINVAL;
		dreq->dreq_flags |= DMOVER_REQ_ERROR;
		/* XXXUNLOCK */
	}

 done:
	dmover_done(dreq);
}

static struct swdmover_function swdmover_func_zero = {
	swdmover_func_zero_process
};

static struct swdmover_function swdmover_func_fill8 = {
	swdmover_func_fill8_process
};

struct swdmover_function swdmover_func_copy = {
	swdmover_func_copy_process
};

const struct dmover_algdesc swdmover_algdescs[] = {
	{
	  DMOVER_FUNC_ZERO,
	  &swdmover_func_zero,
	  0
	},
	{
	  DMOVER_FUNC_FILL8,
	  &swdmover_func_fill8,
	  0
	},
	{
	  DMOVER_FUNC_COPY,
	  &swdmover_func_copy,
	  1
	},
};
#define	SWDMOVER_ALGDESC_COUNT \
	(sizeof(swdmover_algdescs) / sizeof(swdmover_algdescs[0]))

/*
 * swdmover_create_thread:
 *
 *	Actually create the swdmover processing thread.
 */
static void
swdmover_create_thread(void *arg)
{
	int error;

	error = kthread_create1(swdmover_thread, arg, &swdmover_proc,
	    "swdmover");
	if (error)
		printf("WARNING: unable to create swdmover thread, "
		    "error = %d\n", error);
}

/*
 * swdmoverattach:
 *
 *	Pesudo-device attach routine.
 */
void
swdmoverattach(int count)
{

	swdmover_backend.dmb_name = "swdmover";
	swdmover_backend.dmb_speed = 1;		/* XXX */
	swdmover_backend.dmb_cookie = NULL;
	swdmover_backend.dmb_algdescs = swdmover_algdescs;
	swdmover_backend.dmb_nalgdescs = SWDMOVER_ALGDESC_COUNT;
	swdmover_backend.dmb_process = swdmover_process;

	kthread_create(swdmover_create_thread, &swdmover_backend);

	/* XXX Should only register this when kthread creation succeeds. */
	dmover_backend_register(&swdmover_backend);
}
