/* $NetBSD: ld_thunkbus.c,v 1.7 2011/09/03 15:00:28 jmcneill Exp $ */

/*-
 * Copyright (c) 2011 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: ld_thunkbus.c,v 1.7 2011/09/03 15:00:28 jmcneill Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/disk.h>
#include <sys/kmem.h>

#include <dev/ldvar.h>

#include <machine/mainbus.h>
#include <machine/thunk.h>

static int	ld_thunkbus_match(device_t, cfdata_t, void *);
static void	ld_thunkbus_attach(device_t, device_t, void *);

static int	ld_thunkbus_ldstart(struct ld_softc *, struct buf *);
static int	ld_thunkbus_lddump(struct ld_softc *, void *, int, int);
static int	ld_thunkbus_ldflush(struct ld_softc *, int);

static void	ld_thunkbus_sig(int, siginfo_t *, void *);
static void	ld_thunkbus_complete(void *);

struct ld_thunkbus_softc;

struct ld_thunkbus_transfer {
	struct ld_thunkbus_softc *tt_sc;
	struct aiocb	tt_aio;
	struct buf	*tt_bp;
};

struct ld_thunkbus_softc {
	struct ld_softc	sc_ld;

	int		sc_fd;
	void		*sc_ih;

	struct ld_thunkbus_transfer sc_tt;
};

CFATTACH_DECL_NEW(ld_thunkbus, sizeof(struct ld_thunkbus_softc),
    ld_thunkbus_match, ld_thunkbus_attach, NULL, NULL);

static int
ld_thunkbus_match(device_t parent, cfdata_t match, void *opaque)
{
	struct thunkbus_attach_args *taa = opaque;

	if (taa->taa_type != THUNKBUS_TYPE_DISKIMAGE)
		return 0;

	return 1;
}

static void
ld_thunkbus_attach(device_t parent, device_t self, void *opaque)
{
	struct ld_thunkbus_softc *sc = device_private(self);
	struct ld_softc *ld = &sc->sc_ld;
	struct thunkbus_attach_args *taa = opaque;
	const char *path = taa->u.diskimage.path;
	struct sigaction sa;
	ssize_t size, blksize;

	ld->sc_dv = self;

	sc->sc_fd = thunk_open(path, O_RDWR, 0);
	if (sc->sc_fd == -1) {
		aprint_error(": couldn't open %s: %d\n", path, thunk_geterrno());
		return;
	}
	if (thunk_fstat_getsize(sc->sc_fd, &size, &blksize) == -1) {
		aprint_error(": couldn't stat %s: %d\n", path, thunk_geterrno());
		return;
	}

	aprint_naive("\n");
	aprint_normal(": %s (%lld)\n", path, (long long)size);

	ld->sc_flags = LDF_ENABLED;
	ld->sc_maxxfer = blksize;
	ld->sc_secsize = 512;
	ld->sc_secperunit = size / ld->sc_secsize;
	ld->sc_maxqueuecnt = 1;
	ld->sc_start = ld_thunkbus_ldstart;
	ld->sc_dump = ld_thunkbus_lddump;
	ld->sc_flush = ld_thunkbus_ldflush;

	sc->sc_ih = softint_establish(SOFTINT_BIO,
	    ld_thunkbus_complete, sc);

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART|SA_SIGINFO;
	sa.sa_sigaction = ld_thunkbus_sig;
	if (thunk_sigaction(SIGIO, &sa, NULL) == -1)
		panic("couldn't register SIGIO handler: %d", thunk_geterrno());

	ldattach(ld);
}

static void
ld_thunkbus_sig(int sig, siginfo_t *info, void *ctx)
{
	struct ld_thunkbus_transfer *tt;
	struct ld_thunkbus_softc *sc;

	curcpu()->ci_idepth++;

	if (info->si_signo == SIGIO) {
		tt = info->si_value.sival_ptr;
		sc = tt->tt_sc;
		softint_schedule(sc->sc_ih);
	}

	curcpu()->ci_idepth--;
}

static void
ld_thunkbus_complete(void *arg)
{
	struct ld_thunkbus_softc *sc = arg;
	struct ld_thunkbus_transfer *tt = &sc->sc_tt;
	struct buf *bp = tt->tt_bp;

	if (thunk_aio_error(&tt->tt_aio) == 0 &&
	    thunk_aio_return(&tt->tt_aio) != -1) {
		bp->b_resid = 0;
	} else {
		bp->b_error = thunk_geterrno();
		bp->b_resid = bp->b_bcount;
	}

	if (bp->b_error)
		printf("error!\n");

	lddone(&sc->sc_ld, bp);
}

static int
ld_thunkbus_ldstart(struct ld_softc *ld, struct buf *bp)
{
	struct ld_thunkbus_softc *sc = (struct ld_thunkbus_softc *)ld;
	struct ld_thunkbus_transfer *tt = &sc->sc_tt;
	int error;

	tt->tt_sc = sc;
	tt->tt_bp = bp;
	memset(&tt->tt_aio, 0, sizeof(tt->tt_aio));
	tt->tt_aio.aio_fildes = sc->sc_fd;
	tt->tt_aio.aio_buf = bp->b_data;
	tt->tt_aio.aio_nbytes = bp->b_bcount;
	tt->tt_aio.aio_offset = bp->b_rawblkno * ld->sc_secsize;

	tt->tt_aio.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	tt->tt_aio.aio_sigevent.sigev_signo = SIGIO;
	tt->tt_aio.aio_sigevent.sigev_value.sival_ptr = tt;

#if 0
	device_printf(sc->sc_ld.sc_dv, "%s addr %p, off=%lld, count=%lld\n",
	    (bp->b_flags & B_READ) ? "rd" : "wr",
	    bp->b_data,
	    (long long)bp->b_rawblkno,
	    (long long)bp->b_bcount);
#endif

	if (bp->b_flags & B_READ)
		error = thunk_aio_read(&tt->tt_aio);
	else
		error = thunk_aio_write(&tt->tt_aio);

	return error == -1 ? thunk_geterrno() : 0;
}

static int
ld_thunkbus_lddump(struct ld_softc *ld, void *data, int blkno, int blkcnt)
{
	struct ld_thunkbus_softc *sc = (struct ld_thunkbus_softc *)ld;
	ssize_t len;

	len = thunk_pwrite(sc->sc_fd, data, blkcnt, blkno);
	if (len == -1)
		return thunk_geterrno();
	else if (len != blkcnt) {
		device_printf(ld->sc_dv, "%s failed (short xfer)\n", __func__);
		return EIO;
	}

	return 0;
}

static int
ld_thunkbus_ldflush(struct ld_softc *ld, int flags)
{
	struct ld_thunkbus_softc *sc = (struct ld_thunkbus_softc *)ld;

	if (thunk_fsync(sc->sc_fd) == -1)
		return thunk_geterrno();

	return 0;
}

