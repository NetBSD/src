/* $NetBSD: spi.c,v 1.21 2022/01/19 09:30:11 martin Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: spi.c,v 1.21 2022/01/19 09:30:11 martin Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/errno.h>

#include <dev/spi/spivar.h>
#include <dev/spi/spi_io.h>

#include "ioconf.h"
#include "locators.h"

struct spi_softc {
	struct spi_controller	sc_controller;
	int			sc_mode;
	int			sc_speed;
	int			sc_slave;
	int			sc_nslaves;
	struct spi_handle	*sc_slaves;
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
	struct spi_softc	*sh_sc;
	struct spi_controller	*sh_controller;
	int			sh_slave;
	int			sh_mode;
	int			sh_speed;
	int			sh_flags;
#define SPIH_ATTACHED		1
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
spi_print(void *aux, const char *pnp)
{
	struct spi_attach_args *sa = aux;

	if (sa->sa_handle->sh_slave != -1)
		aprint_normal(" slave %d", sa->sa_handle->sh_slave);

	return (UNCONF);
}

static int
spi_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct spi_softc *sc = device_private(parent);
	struct spi_attach_args sa;
	int addr;

	addr = cf->cf_loc[SPICF_SLAVE];
	if ((addr < 0) || (addr >= sc->sc_controller.sct_nslaves)) {
		return -1;
	}

	memset(&sa, 0, sizeof sa);
	sa.sa_handle = &sc->sc_slaves[addr];
	if (ISSET(sa.sa_handle->sh_flags, SPIH_ATTACHED))
		return -1;

	if (config_probe(parent, cf, &sa)) {
		SET(sa.sa_handle->sh_flags, SPIH_ATTACHED);
		config_attach(parent, cf, &sa, spi_print, CFARGS_NONE);
	}

	return 0;
}

/*
 * XXX this is the same as i2c_fill_compat. It could be refactored into a
 * common fill_compat function with pointers to compat & ncompat instead
 * of attach_args as the first parameter.
 */
static void
spi_fill_compat(struct spi_attach_args *sa, const char *compat, size_t len,
	char **buffer)
{
	int count, i;
	const char *c, *start, **ptr;

	*buffer = NULL;
	for (i = count = 0, c = compat; i < len; i++, c++)
		if (*c == 0)
			count++;
	count += 2;
	ptr = malloc(sizeof(char*)*count, M_TEMP, M_WAITOK);
	if (!ptr)
		return;

	for (i = count = 0, start = c = compat; i < len; i++, c++) {
		if (*c == 0) {
			ptr[count++] = start;
			start = c + 1;
		}
	}
	if (start < compat + len) {
		/* last string not 0 terminated */
		size_t l = c - start;
		*buffer = malloc(l + 1, M_TEMP, M_WAITOK);
		memcpy(*buffer, start, l);
		(*buffer)[l] = 0;
		ptr[count++] = *buffer;
	}
	ptr[count] = NULL;

	sa->sa_compat = ptr;
	sa->sa_ncompat = count;
}

static void
spi_direct_attach_child_devices(device_t parent, struct spi_softc *sc,
    prop_array_t child_devices)
{
	unsigned int count;
	prop_dictionary_t child;
	prop_data_t cdata;
	uint32_t slave;
	uint64_t cookie;
	struct spi_attach_args sa;
	int loc[SPICF_NLOCS];
	char *buf;
	int i;

	memset(loc, 0, sizeof loc);
	count = prop_array_count(child_devices);
	for (i = 0; i < count; i++) {
		child = prop_array_get(child_devices, i);
		if (!child)
			continue;
		if (!prop_dictionary_get_uint32(child, "slave", &slave))
			continue;
		if(slave >= sc->sc_controller.sct_nslaves)
			continue;
		if (!prop_dictionary_get_uint64(child, "cookie", &cookie))
			continue;
		if (!(cdata = prop_dictionary_get(child, "compatible")))
			continue;
		loc[SPICF_SLAVE] = slave;

		memset(&sa, 0, sizeof sa);
		sa.sa_handle = &sc->sc_slaves[i];
		sa.sa_prop = child;
		sa.sa_cookie = cookie;
		if (ISSET(sa.sa_handle->sh_flags, SPIH_ATTACHED))
			continue;
		SET(sa.sa_handle->sh_flags, SPIH_ATTACHED);

		buf = NULL;
		spi_fill_compat(&sa,
				prop_data_value(cdata),
				prop_data_size(cdata), &buf);
		config_found(parent, &sa, spi_print,
		    CFARGS(.locators = loc));

		if (sa.sa_compat)
			free(sa.sa_compat, M_TEMP);
		if (buf)
			free(buf, M_TEMP);
	}
}

int
spi_compatible_match(const struct spi_attach_args *sa, const cfdata_t cf,
		     const struct device_compatible_entry *compats)
{
	if (sa->sa_ncompat > 0)
		return device_compatible_match(sa->sa_compat, sa->sa_ncompat,
					       compats);

	return 1;
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
	cv_init(&sc->sc_cv, "spictl");

	sc->sc_controller = *sba->sba_controller;
	sc->sc_nslaves = sba->sba_controller->sct_nslaves;
	/* allocate slave structures */
	sc->sc_slaves = malloc(sizeof (struct spi_handle) * sc->sc_nslaves,
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
		sc->sc_slaves[i].sh_controller = &sc->sc_controller;
	}

	/* First attach devices known to be present via fdt */
	if (sba->sba_child_devices) {
		spi_direct_attach_child_devices(self, sc, sba->sba_child_devices);
	}
	/* Then do any other devices the user may have manually wired */
	config_search(self, NULL,
	    CFARGS(.search = spi_search));
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
	device_t self = device_lookup(&spi_cd, minor(dev));
	struct spi_handle *sh;
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
		error = spi_configure(self, sh, sic->sic_mode, sic->sic_speed);
		break;
	case SPI_IOCTL_TRANSFER:
		sit = (spi_ioctl_transfer_t *)data;
		if (sit->sit_addr < 0 || sit->sit_addr >= sc->sc_nslaves) {
			error = EINVAL;
			break;
		}
		if ((sit->sit_send && sit->sit_sendlen == 0)
		    || (sit->sit_recv && sit->sit_recv == 0)) {
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
    spi_match, spi_attach, NULL, NULL);

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
spi_configure(device_t dev __unused, struct spi_handle *sh, int mode, int speed)
{

	sh->sh_mode = mode;
	sh->sh_speed = speed;

	/* No need to report errors; no failures. */

	return 0;
}

/*
 * Acquire controller
 */
static void
spi_acquire(struct spi_handle *sh)
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
spi_release(struct spi_handle *sh)
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
spi_transfer(struct spi_handle *sh, struct spi_transfer *st)
{
	struct spi_softc	*sc = sh->sh_sc;
	struct spi_controller	*tag = sh->sh_controller;
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
	struct spi_handle *sh = st->st_spiprivate;

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
 * not full duplex.  If you wnat full duplex, you can't use these convenience
 * wrappers.
 */
int
spi_recv(struct spi_handle *sh, int cnt, uint8_t *data)
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
spi_send(struct spi_handle *sh, int cnt, const uint8_t *data)
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
spi_send_recv(struct spi_handle *sh, int scnt, const uint8_t *snd,
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

