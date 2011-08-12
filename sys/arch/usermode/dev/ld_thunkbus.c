/* $NetBSD: ld_thunkbus.c,v 1.1 2011/08/12 12:59:13 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ld_thunkbus.c,v 1.1 2011/08/12 12:59:13 jmcneill Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/disk.h>

#include <dev/ldvar.h>

#include <machine/mainbus.h>
#include <machine/thunk.h>

static int	ld_thunkbus_match(device_t, cfdata_t, void *);
static void	ld_thunkbus_attach(device_t, device_t, void *);

static int	ld_thunkbus_ldstart(struct ld_softc *, struct buf *);
static int	ld_thunkbus_lddump(struct ld_softc *, void *, int, int);
static int	ld_thunkbus_ldflush(struct ld_softc *, int);

struct ld_thunkbus_softc {
	struct ld_softc	sc_ld;

	int		sc_fd;
	struct stat	sc_st;
};

CFATTACH_DECL_NEW(ld_thunkbus, sizeof(struct ld_thunkbus_softc),
    ld_thunkbus_match, ld_thunkbus_attach, NULL, NULL);

extern int errno;

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

	ld->sc_dv = self;

	sc->sc_fd = thunk_open(path, O_RDWR, 0);
	if (sc->sc_fd == -1) {
		aprint_error(": couldn't open %s: %d\n", path, errno);
		return;
	}
	if (thunk_fstat(sc->sc_fd, &sc->sc_st) == -1) {
		aprint_error(": couldn't stat %s: %d\n", path, errno);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": %s (%lld)\n", path, (long long)sc->sc_st.st_size);

	ld->sc_flags = LDF_ENABLED;
	ld->sc_maxxfer = sc->sc_st.st_blksize;
	ld->sc_secsize = 1;
	ld->sc_secperunit = sc->sc_st.st_size;
	ld->sc_maxqueuecnt = 1;
	ld->sc_start = ld_thunkbus_ldstart;
	ld->sc_dump = ld_thunkbus_lddump;
	ld->sc_flush = ld_thunkbus_ldflush;

	ldattach(ld);
}

static int
ld_thunkbus_ldstart(struct ld_softc *ld, struct buf *bp)
{
	struct ld_thunkbus_softc *sc = (struct ld_thunkbus_softc *)ld;
	ssize_t len;

	if (bp->b_flags & B_READ)
		len = thunk_pread(sc->sc_fd, bp->b_data, bp->b_bcount,
		    bp->b_rawblkno);
	else
		len = thunk_pwrite(sc->sc_fd, bp->b_data, bp->b_bcount,
		    bp->b_rawblkno);

	if (len == -1)
		return errno;
	else if (len != bp->b_bcount)
		panic("%s: short xfer", __func__);
	return 0;
}

static int
ld_thunkbus_lddump(struct ld_softc *ld, void *data, int blkno, int blkcnt)
{
	struct ld_thunkbus_softc *sc = (struct ld_thunkbus_softc *)ld;
	ssize_t len;

	len = thunk_pwrite(sc->sc_fd, data, blkcnt, blkno);
	if (len == -1)
		return errno;
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
		return errno;

	return 0;
}

