/*	$NetBSD: ld_nvme.c,v 1.1 2016/05/01 10:21:02 nonaka Exp $	*/

/*-
 * Copyright (C) 2016 NONAKA Kimihiro <nonaka@netbsd.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ld_nvme.c,v 1.1 2016/05/01 10:21:02 nonaka Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/disk.h>
#include <sys/kmem.h>

#include <dev/ldvar.h>
#include <dev/ic/nvmereg.h>
#include <dev/ic/nvmevar.h>

struct ld_nvme_softc {
	struct ld_softc		sc_ld;
	struct nvme_softc	*sc_nvme;

	uint16_t		sc_nsid;
};

static int	ld_nvme_match(device_t, cfdata_t, void *);
static void	ld_nvme_attach(device_t, device_t, void *);
static int	ld_nvme_detach(device_t, int);

CFATTACH_DECL_NEW(ld_nvme, sizeof(struct ld_nvme_softc),
    ld_nvme_match, ld_nvme_attach, ld_nvme_detach, NULL);

static int	ld_nvme_start(struct ld_softc *, struct buf *);
static int	ld_nvme_dump(struct ld_softc *, void *, int, int);
static int	ld_nvme_flush(struct ld_softc *, int);

static int	ld_nvme_dobio(struct ld_nvme_softc *, void *, int, daddr_t,
		    int, struct buf *);
static void	ld_nvme_biodone(struct nvme_ns_context *);
static void	ld_nvme_syncdone(struct nvme_ns_context *);


static int
ld_nvme_match(device_t parent, cfdata_t match, void *aux)
{
	struct nvme_attach_args *naa = aux;

	if (naa->naa_nsid == 0)
		return 0;

	return 1;
}

static void
ld_nvme_attach(device_t parent, device_t self, void *aux)
{
	struct ld_nvme_softc *sc = device_private(self);
	struct ld_softc *ld = &sc->sc_ld;
	struct nvme_softc *nsc = device_private(parent);
	struct nvme_attach_args *naa = aux;
	struct nvme_namespace *ns;
	struct nvm_namespace_format *f;
	uint64_t nsze;
	int error;

	ld->sc_dv = self;
	sc->sc_nvme = nsc;
	sc->sc_nsid = naa->naa_nsid;

	aprint_naive("\n");
	aprint_normal("\n");

	error = nvme_ns_identify(sc->sc_nvme, sc->sc_nsid);
	if (error) {
		aprint_error_dev(self, "couldn't identify namespace\n");
		return;
	}

	ns = nvme_ns_get(sc->sc_nvme, sc->sc_nsid);
	KASSERT(ns);
	nsze = lemtoh64(&ns->ident->nsze);
	f = &ns->ident->lbaf[NVME_ID_NS_FLBAS(ns->ident->flbas)];

	ld->sc_secsize = 1 << f->lbads;
	ld->sc_secperunit = nsze;
	ld->sc_maxxfer = MAXPHYS;
	ld->sc_maxqueuecnt = naa->naa_qentries;
	ld->sc_start = ld_nvme_start;
	ld->sc_dump = ld_nvme_dump;
	ld->sc_flush = ld_nvme_flush;
	ld->sc_flags = LDF_ENABLED;
	ldattach(ld);
}

static int
ld_nvme_detach(device_t self, int flags)
{
	struct ld_nvme_softc *sc = device_private(self);
	struct ld_softc *ld = &sc->sc_ld;
	int rv;

	if ((rv = ldbegindetach(ld, flags)) != 0)
		return rv;
	ldenddetach(ld);

	nvme_ns_free(sc->sc_nvme, sc->sc_nsid);

	return 0;
}

static int
ld_nvme_start(struct ld_softc *ld, struct buf *bp)
{
	struct ld_nvme_softc *sc = device_private(ld->sc_dv);

	return ld_nvme_dobio(sc, bp->b_data, bp->b_bcount, bp->b_rawblkno,
	    BUF_ISWRITE(bp), bp);
}

static int
ld_nvme_dump(struct ld_softc *ld, void *data, int blkno, int blkcnt)
{
	struct ld_nvme_softc *sc = device_private(ld->sc_dv);

	return ld_nvme_dobio(sc, data, blkcnt * ld->sc_secsize, blkno, 1, NULL);
}

static int
ld_nvme_dobio(struct ld_nvme_softc *sc, void *data, int datasize, daddr_t blkno,
    int dowrite, struct buf *bp)
{
	struct nvme_ns_context *ctx;
	int error;
	int s;

	ctx = nvme_ns_get_ctx(bp != NULL ? PR_WAITOK : PR_NOWAIT);
	ctx->nnc_cookie = sc;
	ctx->nnc_nsid = sc->sc_nsid;
	ctx->nnc_done = ld_nvme_biodone;
	ctx->nnc_buf = bp;
	ctx->nnc_data = data;
	ctx->nnc_datasize = datasize;
	ctx->nnc_secsize = sc->sc_ld.sc_secsize;
	ctx->nnc_blkno = blkno;
	ctx->nnc_flags = dowrite ? 0 : NVME_NS_CTX_F_READ;
	if (bp == NULL) {
		SET(ctx->nnc_flags, NVME_NS_CTX_F_POLL);
		s = splbio();
	}
	error = nvme_ns_dobio(sc->sc_nvme, ctx);
	if (bp == NULL) {
		splx(s);
	}

	return error;
}

static void
ld_nvme_biodone(struct nvme_ns_context *ctx)
{
	struct ld_nvme_softc *sc = ctx->nnc_cookie;
	struct buf *bp = ctx->nnc_buf;
	int status = NVME_CQE_SC(ctx->nnc_status);

	if (bp != NULL) {
		if (status != NVME_CQE_SC_SUCCESS) {
			bp->b_error = EIO;
			bp->b_resid = bp->b_bcount;
			aprint_error_dev(sc->sc_ld.sc_dv, "I/O error\n");
		} else {
			bp->b_resid = 0;
		}
		lddone(&sc->sc_ld, bp);
	} else {
		if (status != NVME_CQE_SC_SUCCESS) {
			aprint_error_dev(sc->sc_ld.sc_dv, "I/O error\n");
		}
	}
	nvme_ns_put_ctx(ctx);
}

static int
ld_nvme_flush(struct ld_softc *ld, int flags)
{
	struct ld_nvme_softc *sc = device_private(ld->sc_dv);
	struct nvme_ns_context *ctx;
	int error;
	int s;

	ctx = nvme_ns_get_ctx((flags & LDFL_POLL) ? PR_NOWAIT : PR_WAITOK);
	ctx->nnc_cookie = sc;
	ctx->nnc_nsid = sc->sc_nsid;
	ctx->nnc_done = ld_nvme_syncdone;
	ctx->nnc_flags = 0;
	if (flags & LDFL_POLL) {
		SET(ctx->nnc_flags, NVME_NS_CTX_F_POLL);
		s = splbio();
	}
	error = nvme_ns_sync(sc->sc_nvme, ctx);
	if (flags & LDFL_POLL) {
		splx(s);
	}

	return error;
}

static void
ld_nvme_syncdone(struct nvme_ns_context *ctx)
{

	nvme_ns_put_ctx(ctx);
}
