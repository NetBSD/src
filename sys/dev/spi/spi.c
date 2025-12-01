/* $NetBSD: spi.c,v 1.40 2025/12/01 14:05:07 thorpej Exp $ */

/*-
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * Portions of this code were written by Garrett D'Amore for the
 * Champaign-Urbana Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_fdt.h"		/* XXX */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: spi.c,v 1.40 2025/12/01 14:05:07 thorpej Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/errno.h>
#include <sys/uio.h>

#include <dev/spi/spivar.h>
#include <dev/spi/spi_io.h>
#include <dev/spi/spi_calls.h>

#ifdef FDT
#include <dev/fdt/fdt_spi.h>	/* XXX */
#include <dev/ofw/openfirm.h>	/* XXX */
#endif

#include "ioconf.h"
#include "locators.h"

struct spi_softc {
	device_t		sc_dev;
	const struct spi_controller *sc_controller;
	int			sc_mode;
	int			sc_speed;
	int			sc_slave;
	int			sc_nslaves;
	spi_handle_t		sc_slaves;
	kmutex_t		sc_slave_state_lock;
	kmutex_t		sc_lock;
	kcondvar_t		sc_cv;
	kmutex_t		sc_dev_lock;
	int			sc_flags;
#define SPIC_BUSY		1
};

static dev_type_open(spi_open);
static dev_type_close(spi_close);
static dev_type_ioctl(spi_ioctl);

const struct cdevsw spi_cdevsw = {
	.d_open = spi_open,
	.d_close = spi_close,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = spi_ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};

/*
 * SPI slave device.  We have one of these per slave.
 */
struct spi_handle {
	struct spi_softc	*sh_sc;		    /* static */
	const struct spi_controller *sh_controller; /* static */
	int			sh_slave;	    /* static */
	int			sh_mode;	/* locked by owning child */
	int			sh_speed;	/* locked by owning child */
	int			sh_flags;	/* vv slave_state_lock vv */
#define SPIH_ATTACHED		__BIT(0)
#define	SPIH_DIRECT		__BIT(1)
	device_t		sh_dev;		/* ^^ slave_state_lock ^^ */
};

#define SPI_MAXDATA 4096

/*
 * API for bus drivers.
 */

int
spibus_print(void *aux, const char *pnp)
{

	if (pnp != NULL)
		aprint_normal("spi at %s", pnp);

	return (UNCONF);
}


static int
spi_match(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

static int
spi_print_direct(void *aux, const char *pnp)
{
	struct spi_attach_args *sa = aux;

	if (pnp != NULL) {
		aprint_normal("%s%s%s%s at %s slave %d",
		    sa->sa_name ? sa->sa_name : "(unknown)",
		    sa->sa_clist ? " (" : "",
		    sa->sa_clist ? sa->sa_clist : "",
		    sa->sa_clist ? ")" : "",
		    pnp, sa->sa_handle->sh_slave);
	} else {
		aprint_normal(" slave %d", sa->sa_handle->sh_slave);
	}

	return UNCONF;
}

static int
spi_print(void *aux, const char *pnp)
{
	struct spi_attach_args *sa = aux;

	aprint_normal(" slave %d", sa->sa_handle->sh_slave);

	return UNCONF;
}

static void
spi_attach_child(struct spi_softc *sc, struct spi_attach_args *sa,
    int chip_select, cfdata_t cf)
{
	spi_handle_t sh;
	device_t newdev = NULL;
	bool is_direct = cf == NULL;
	const int skip_flags = is_direct ? SPIH_ATTACHED
					 : (SPIH_ATTACHED | SPIH_DIRECT);
	const int claim_flags = skip_flags ^ SPIH_DIRECT;
	int locs[SPICF_NLOCS] = { 0 };

	if (chip_select < 0 ||
	    chip_select >= sc->sc_controller->sct_nslaves) {
		return;
	}

	sh = &sc->sc_slaves[chip_select];

	mutex_enter(&sc->sc_slave_state_lock);
	if (ISSET(sh->sh_flags, skip_flags)) {
		mutex_exit(&sc->sc_slave_state_lock);
		return;
	}

	/* Keep others off of this chip select. */
	SET(sh->sh_flags, claim_flags);
	mutex_exit(&sc->sc_slave_state_lock);

	locs[SPICF_SLAVE] = chip_select;
	sa->sa_handle = sh;

	if (is_direct) {
		newdev = config_found(sc->sc_dev, sa, spi_print_direct,
		    CFARGS(.submatch = config_stdsubmatch,
			   .locators = locs,
			   .devhandle = sa->sa_devhandle));
	} else {
		if (config_probe(sc->sc_dev, cf, sa)) {
			newdev = config_attach(sc->sc_dev, cf, sa, spi_print,
			    CFARGS(.locators = locs));
		}
	}

	mutex_enter(&sc->sc_slave_state_lock);
	if (newdev == NULL) {
		/*
		 * Clear our claim on this chip select (yes, just
		 * the ATTACHED flag; we want to keep indirects off
		 * of chip selects for which there is a device tree
		 * node).
		 */
		CLR(sh->sh_flags, SPIH_ATTACHED);
	} else {
		/* Record the child for posterity. */
		sh->sh_dev = newdev;
	}
	mutex_exit(&sc->sc_slave_state_lock);
}

static int
spi_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct spi_softc *sc = device_private(parent);

	if (cf->cf_loc[SPICF_SLAVE] == SPICF_SLAVE_DEFAULT) {
		/* No wildcards for indirect on SPI. */
		return 0;
	}

	struct spi_attach_args sa = { 0 };
	spi_attach_child(sc, &sa, cf->cf_loc[SPICF_SLAVE], cf);

	return 0;
}

static bool
spi_enumerate_devices_callback(device_t self,
    struct spi_enumerate_devices_args *args)
{
	struct spi_softc *sc = device_private(self);

	spi_attach_child(sc, args->sa, args->chip_select, NULL);

	return true;				/* keep enumerating */
}

int
spi_compatible_match(const struct spi_attach_args *sa,
		     const struct device_compatible_entry *compats)
{
	return device_compatible_match_strlist(sa->sa_clist,
	    sa->sa_clist_size, compats);
}

const struct device_compatible_entry *
spi_compatible_lookup(const struct spi_attach_args *sa,
		      const struct device_compatible_entry *compats)
{
	return device_compatible_lookup_strlist(sa->sa_clist,
	    sa->sa_clist_size, compats);
}

bool
spi_use_direct_match(const struct spi_attach_args *sa,
		     const struct device_compatible_entry *compats,
		     int *match_resultp)
{
	KASSERT(match_resultp != NULL);

	if (sa->sa_clist != NULL && sa->sa_clist_size != 0) {
		*match_resultp = spi_compatible_match(sa, compats);
		return true;
	}

	return false;
}

/*
 * API for device drivers.
 *
 * We provide wrapper routines to decouple the ABI for the SPI
 * device drivers from the ABI for the SPI bus drivers.
 */
static void
spi_attach(device_t parent, device_t self, void *aux)
{
	struct spi_softc *sc = device_private(self);
	struct spibus_attach_args *sba = aux;
	int i;

	aprint_naive(": SPI bus\n");
	aprint_normal(": SPI bus\n");

	mutex_init(&sc->sc_dev_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	mutex_init(&sc->sc_slave_state_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_cv, "spictl");

	sc->sc_dev = self;
	sc->sc_controller = sba->sba_controller;
	sc->sc_nslaves = sba->sba_controller->sct_nslaves;
	/* allocate slave structures */
	sc->sc_slaves = malloc(sizeof(*sc->sc_slaves) * sc->sc_nslaves,
	    M_DEVBUF, M_WAITOK | M_ZERO);

	sc->sc_speed = 0;
	sc->sc_mode = -1;
	sc->sc_slave = -1;

	/*
	 * Initialize slave handles
	 */
	for (i = 0; i < sc->sc_nslaves; i++) {
		sc->sc_slaves[i].sh_slave = i;
		sc->sc_slaves[i].sh_sc = sc;
		sc->sc_slaves[i].sh_controller = sc->sc_controller;
	}

	/* XXX Need a better way for this. */
	switch (devhandle_type(device_handle(sc->sc_dev))) {
#ifdef FDT
	case DEVHANDLE_TYPE_OF:
#if 0
		/*
		 * XXX The addition of a USB SPI controller has triggered
		 * XXX an unfortunate situation, whereby it is attaching
		 * XXX a SPI controller on an otherwise FDT platform (RISC-V)
		 * XXX that does not happen to currently have any platform
		 * XXX SoC SPI controller drivers that carry the fdt_spi
		 * XXX config attribute that would pull in the function
		 * XXX being called here.
		 * XXX
		 * XXX As it happens we can fairly safely elide this call
		 * XXX because, at the moment (1 Dec 2025), there are no
		 * XXX consumers of the registration if performs.  However,
		 * XXX this points to a larger problem if needed a way to
		 * XXX resolve these situations at runtime with some sort
		 * XXX of "platform" abstraction rather than at kernel build
		 * XXX time.
		 */
		fdtbus_register_spi_controller(self, sc->sc_controller);
#endif
		break;
#endif /* FDT */
	default:
		break;
	}

	/*
	 * Attempt to enumerate the devices on the bus using the
	 * platform device tree.
	 */
	struct spi_attach_args sa = { 0 };
	struct spi_enumerate_devices_args enumargs = {
		.sa = &sa,
		.callback = spi_enumerate_devices_callback,
	};
	device_call(self, SPI_ENUMERATE_DEVICES(&enumargs));

	/* Then do any other devices the user may have manually wired */
	config_search(self, NULL,
	    CFARGS(.search = spi_search));
}

static int
spi_detach(device_t self, int flags)
{
	int error;

	error = config_detach_children(self, flags);
	if (error)
		return error;

	return 0;
}

static int
spi_open(dev_t dev, int flag, int fmt, lwp_t *l)
{
	struct spi_softc *sc = device_lookup_private(&spi_cd, minor(dev));

	if (sc == NULL)
		return ENXIO;

	return 0;
}

static int
spi_close(dev_t dev, int flag, int fmt, lwp_t *l)
{

	return 0;
}

static int
spi_ioctl(dev_t dev, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct spi_softc *sc = device_lookup_private(&spi_cd, minor(dev));
	spi_handle_t sh;
	spi_ioctl_configure_t *sic;
	spi_ioctl_transfer_t *sit;
	uint8_t *sbuf, *rbuf;
	int error;

	if (sc == NULL)
		return ENXIO;

	mutex_enter(&sc->sc_dev_lock);

	switch (cmd) {
	case SPI_IOCTL_CONFIGURE:
		sic = (spi_ioctl_configure_t *)data;
		if (sic->sic_addr < 0 || sic->sic_addr >= sc->sc_nslaves) {
			error = EINVAL;
			break;
		}
		sh = &sc->sc_slaves[sic->sic_addr];
		error = spi_configure(sc->sc_dev, sh, sic->sic_mode,
		    sic->sic_speed);
		break;
	case SPI_IOCTL_TRANSFER:
		sit = (spi_ioctl_transfer_t *)data;
		if (sit->sit_addr < 0 || sit->sit_addr >= sc->sc_nslaves) {
			error = EINVAL;
			break;
		}
		if ((sit->sit_send && sit->sit_sendlen == 0)
		    || (sit->sit_recv && sit->sit_recvlen == 0)) {
			error = EINVAL;
			break;
		}
		sh = &sc->sc_slaves[sit->sit_addr];
		sbuf = rbuf = NULL;
		error = 0;
		if (sit->sit_send && sit->sit_sendlen <= SPI_MAXDATA) {
			sbuf = malloc(sit->sit_sendlen, M_DEVBUF, M_WAITOK);
			error = copyin(sit->sit_send, sbuf, sit->sit_sendlen);
		}
		if (sit->sit_recv && sit->sit_recvlen <= SPI_MAXDATA) {
			rbuf = malloc(sit->sit_recvlen, M_DEVBUF, M_WAITOK);
		}
		if (error == 0) {
			if (sbuf && rbuf)
				error = spi_send_recv(sh,
					sit->sit_sendlen, sbuf,
					sit->sit_recvlen, rbuf);
			else if (sbuf)
				error = spi_send(sh,
					sit->sit_sendlen, sbuf);
			else if (rbuf)
				error = spi_recv(sh,
					sit->sit_recvlen, rbuf);
		}
		if (rbuf) {
			if (error == 0)
				error = copyout(rbuf, sit->sit_recv,
						sit->sit_recvlen);
			free(rbuf, M_DEVBUF);
		}
		if (sbuf) {
			free(sbuf, M_DEVBUF);
		}
		break;
	default:
		error = ENODEV;
		break;
	}

	mutex_exit(&sc->sc_dev_lock);

	return error;
}

CFATTACH_DECL_NEW(spi, sizeof(struct spi_softc),
    spi_match, spi_attach, spi_detach, NULL);

/*
 * Configure.  This should be the first thing that the SPI driver
 * should do, to configure which mode (e.g. SPI_MODE_0, which is the
 * same as Philips Microwire mode), and speed.  If the bus driver
 * cannot run fast enough, then it should just configure the fastest
 * mode that it can support.  If the bus driver cannot run slow
 * enough, then the device is incompatible and an error should be
 * returned.
 */
int
spi_configure(device_t dev, spi_handle_t sh, int mode, int speed)
{
	struct spi_get_transfer_mode_args args = { 0 };
	int error;

	/*
	 * Get transfer mode information from the platform device tree, if
	 * it exists.
	 */
	error = device_call(dev, SPI_GET_TRANSFER_MODE(&args));
	if (error) {
		if (error != ENOTSUP) {
			/*
			 * This error is fatal.  Error message has already
			 * been displayed.
			 */
			return error;
		}
	} else {
		/*
		 * If the device tree specifies clock phase shift or
		 * polarity inversion, override whatever the caller
		 * specified.
		 */
		if (args.mode != 0) {
			aprint_debug_dev(dev,
			    "using SPI mode %u from device tree\n",
			    args.mode);
			mode = args.mode;
		}

		/*
		 * If the device tree specifies the max clock frequency,
		 * override whatever the caller specified.
		 */
		if (args.max_frequency != 0) {
			aprint_debug_dev(dev,
			    "using max-frequency %u Hz from device tree\n",
			    args.max_frequency);
			speed = args.max_frequency;
		}

		/* XXX Handle the other transfer properties. */
	}

	sh->sh_mode = mode;
	sh->sh_speed = speed;

	return 0;
}

/*
 * Acquire controller
 */
static void
spi_acquire(spi_handle_t sh)
{
	struct spi_softc *sc = sh->sh_sc;

	mutex_enter(&sc->sc_lock);
	while ((sc->sc_flags & SPIC_BUSY) != 0)
		cv_wait(&sc->sc_cv, &sc->sc_lock);
	sc->sc_flags |= SPIC_BUSY;
	mutex_exit(&sc->sc_lock);
}

/*
 * Release controller
 */
static void
spi_release(spi_handle_t sh)
{
	struct spi_softc *sc = sh->sh_sc;

	mutex_enter(&sc->sc_lock);
	sc->sc_flags &= ~SPIC_BUSY;
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);
}

void
spi_transfer_init(struct spi_transfer *st)
{

	mutex_init(&st->st_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&st->st_cv, "spixfr");

	st->st_flags = 0;
	st->st_errno = 0;
	st->st_done = NULL;
	st->st_chunks = NULL;
	st->st_private = NULL;
	st->st_slave = -1;
}

void
spi_chunk_init(struct spi_chunk *chunk, int cnt, const uint8_t *wptr,
    uint8_t *rptr)
{

	chunk->chunk_write = chunk->chunk_wptr = wptr;
	chunk->chunk_read = chunk->chunk_rptr = rptr;
	chunk->chunk_rresid = chunk->chunk_wresid = chunk->chunk_count = cnt;
	chunk->chunk_next = NULL;
}

void
spi_transfer_add(struct spi_transfer *st, struct spi_chunk *chunk)
{
	struct spi_chunk **cpp;

	/* this is an O(n) insert -- perhaps we should use a simpleq? */
	for (cpp = &st->st_chunks; *cpp; cpp = &(*cpp)->chunk_next);
	*cpp = chunk;
}

int
spi_transfer(spi_handle_t sh, struct spi_transfer *st)
{
	struct spi_softc	*sc = sh->sh_sc;
	const struct spi_controller *tag = sh->sh_controller;
	struct spi_chunk	*chunk;
	int error;

	/*
	 * Initialize "resid" counters and pointers, so that callers
	 * and bus drivers don't have to.
	 */
	for (chunk = st->st_chunks; chunk; chunk = chunk->chunk_next) {
		chunk->chunk_wresid = chunk->chunk_rresid = chunk->chunk_count;
		chunk->chunk_wptr = chunk->chunk_write;
		chunk->chunk_rptr = chunk->chunk_read;
	}

	/*
	 * Match slave and parameters to handle
	 */
	st->st_slave = sh->sh_slave;

	/*
	 * Reserve controller during transaction
 	 */
	spi_acquire(sh);

	st->st_spiprivate = (void *)sh;

	/*
	 * Reconfigure controller
	 *
	 * XXX backends don't configure per-slave parameters
	 * Whenever we switch slaves or change mode or speed, we
	 * need to tell the backend.
	 */
	if (sc->sc_slave != sh->sh_slave
	    || sc->sc_mode != sh->sh_mode
	    || sc->sc_speed != sh->sh_speed) {
		error = (*tag->sct_configure)(tag->sct_cookie,
				sh->sh_slave, sh->sh_mode, sh->sh_speed);
		if (error)
			return error;
	}
	sc->sc_mode = sh->sh_mode;
	sc->sc_speed = sh->sh_speed;
	sc->sc_slave = sh->sh_slave;

	error = (*tag->sct_transfer)(tag->sct_cookie, st);

	return error;
}

void
spi_wait(struct spi_transfer *st)
{
	spi_handle_t sh = st->st_spiprivate;

	mutex_enter(&st->st_lock);
	while (!(st->st_flags & SPI_F_DONE)) {
		cv_wait(&st->st_cv, &st->st_lock);
	}
	mutex_exit(&st->st_lock);
	cv_destroy(&st->st_cv);
	mutex_destroy(&st->st_lock);

	/*
	 * End transaction
	 */
	spi_release(sh);
}

void
spi_done(struct spi_transfer *st, int err)
{

	mutex_enter(&st->st_lock);
	if ((st->st_errno = err) != 0) {
		st->st_flags |= SPI_F_ERROR;
	}
	st->st_flags |= SPI_F_DONE;
	if (st->st_done != NULL) {
		(*st->st_done)(st);
	} else {
		cv_broadcast(&st->st_cv);
	}
	mutex_exit(&st->st_lock);
}

/*
 * Some convenience routines.  These routines block until the work
 * is done.
 *
 * spi_recv - receives data from the bus
 *
 * spi_send - sends data to the bus
 *
 * spi_send_recv - sends data to the bus, and then receives.  Note that this is
 * done synchronously, i.e. send a command and get the response.  This is
 * not full duplex.  If you want full duplex, you can't use these convenience
 * wrappers.
 *
 * spi_sendv - scatter send data to the bus
 */
int
spi_recv(spi_handle_t sh, int cnt, uint8_t *data)
{
	struct spi_transfer	trans;
	struct spi_chunk	chunk;

	spi_transfer_init(&trans);
	spi_chunk_init(&chunk, cnt, NULL, data);
	spi_transfer_add(&trans, &chunk);

	/* enqueue it and wait for it to complete */
	spi_transfer(sh, &trans);
	spi_wait(&trans);

	if (trans.st_flags & SPI_F_ERROR)
		return trans.st_errno;

	return 0;
}

int
spi_send(spi_handle_t sh, int cnt, const uint8_t *data)
{
	struct spi_transfer	trans;
	struct spi_chunk	chunk;

	spi_transfer_init(&trans);
	spi_chunk_init(&chunk, cnt, data, NULL);
	spi_transfer_add(&trans, &chunk);

	/* enqueue it and wait for it to complete */
	spi_transfer(sh, &trans);
	spi_wait(&trans);

	if (trans.st_flags & SPI_F_ERROR)
		return trans.st_errno;

	return 0;
}

int
spi_send_recv(spi_handle_t sh, int scnt, const uint8_t *snd,
    int rcnt, uint8_t *rcv)
{
	struct spi_transfer	trans;
	struct spi_chunk	chunk1, chunk2;

	spi_transfer_init(&trans);
	spi_chunk_init(&chunk1, scnt, snd, NULL);
	spi_chunk_init(&chunk2, rcnt, NULL, rcv);
	spi_transfer_add(&trans, &chunk1);
	spi_transfer_add(&trans, &chunk2);

	/* enqueue it and wait for it to complete */
	spi_transfer(sh, &trans);
	spi_wait(&trans);

	if (trans.st_flags & SPI_F_ERROR)
		return trans.st_errno;

	return 0;
}

int
spi_sendv(spi_handle_t sh, const struct iovec *iov,
    int iovcnt)
{
	struct spi_transfer	trans;
	SIMPLEQ_HEAD(,spi_chunk_q) ck_q;
	struct spi_chunk_q	*ce;

	SIMPLEQ_INIT(&ck_q);

	spi_transfer_init(&trans);
	for(int c = 0; c < iovcnt;c++) {
		ce = kmem_alloc(sizeof(struct spi_chunk_q),KM_NOSLEEP);
		if (ce == NULL)
			return ENOMEM;
		spi_chunk_init(&ce->chunk, iov[c].iov_len, iov[c].iov_base, NULL);
		spi_transfer_add(&trans, &ce->chunk);
		SIMPLEQ_INSERT_HEAD(&ck_q, ce, chunk_q);
	}

	/* enqueue it and wait for it to complete */
	spi_transfer(sh, &trans);
	spi_wait(&trans);

        while ((ce = SIMPLEQ_FIRST(&ck_q)) != NULL) {
                SIMPLEQ_REMOVE_HEAD(&ck_q, chunk_q);
		kmem_free(ce, sizeof(struct spi_chunk_q));
        }

	if (trans.st_flags & SPI_F_ERROR)
		return trans.st_errno;

	return 0;
}
