/*	$NetBSD: memory.c,v 1.3 2010/11/14 03:57:17 uebayasi Exp $	*/
/*	$OpenBSD: mem.c,v 1.15 2007/10/14 17:29:04 kettenis Exp $	*/

/*-
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)mem.c	8.3 (Berkeley) 1/12/94
 */

/* 
 *	The EEPROMs for Serial Presence Detect don't show up in the
 * 	OpenFirmware tree, but their contents are available through the
 * 	"dimm-info" property of the "/memory" node.  To make the
 * 	information available, we fake up an I2C bus with EEPROMs
 * 	containing the appropriate slices of the "dimm-info" property.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: memory.c,v 1.3 2010/11/14 03:57:17 uebayasi Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <machine/autoconf.h>

#include <dev/i2c/i2cvar.h>
#include <dev/ofw/openfirm.h>

struct memory_softc {
	struct device	 sc_dev;

	u_char		*sc_buf;
	int		 sc_len;
};

/* Size of a single SPD entry in "dimm-info" property. */
#define SPD_SIZE	128

int	memory_match(struct device *, struct cfdata *, void *);
void	memory_attach(struct device *, struct device *, void *);

CFATTACH_DECL(memory, sizeof(struct memory_softc), memory_match, memory_attach,
              NULL, NULL);

int	memory_i2c_acquire_bus(void *, int);
void	memory_i2c_release_bus(void *, int);
int	memory_i2c_exec(void *, i2c_op_t, i2c_addr_t,
   	                const void *, size_t, void *, size_t, int);

int
memory_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "memory") == 0)
		return (1);
	return (0);
}

void
memory_attach(struct device *parent, struct device *self, void *aux)
{
	struct memory_softc *sc = (struct memory_softc *)self;
	struct confargs *ca = aux;
	struct i2c_controller ic;
	struct i2c_attach_args ia;

	sc->sc_len = OF_getproplen(ca->ca_node, "dimm-info");
	if (sc->sc_len > 0) {
		sc->sc_buf = malloc(sc->sc_len, M_DEVBUF, M_NOWAIT);
		if (sc->sc_buf == NULL) {
			printf(": can't allocate memory\n");
			return;
		}
		printf(": len=%d", sc->sc_len);
	}

	printf("\n");

	if (sc->sc_len > 0) {
		int	addr;

		OF_getprop(ca->ca_node, "dimm-info", sc->sc_buf, sc->sc_len);

		memset(&ic, 0, sizeof ic);
		ic.ic_cookie = sc;
		ic.ic_acquire_bus = memory_i2c_acquire_bus;
		ic.ic_release_bus = memory_i2c_release_bus;
		ic.ic_exec = memory_i2c_exec;

		memset(&ia, 0, sizeof ia);
		ia.ia_tag = &ic;
#ifdef notyet
		ia.ia_name = "spd";
#endif		
		addr = 0;
		while ((addr * SPD_SIZE) < sc->sc_len) {
			/* Skip entries that have not been filled in. */
			if (sc->sc_buf[addr * SPD_SIZE] != 0) {
				ia.ia_addr = 0x50 + addr;
				config_found(self, &ia, NULL);
			}
			addr++;
		}

		/* No need to keep the "dimm-info" contents around. */
		free(sc->sc_buf, M_DEVBUF);
		sc->sc_len = -1;
	}
}

int
memory_i2c_acquire_bus(void *cookie, int flags)
{
	return (0);
}

void
memory_i2c_release_bus(void *cookie, int flags)
{
}

int
memory_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr,
                const void *cmdbuf, size_t cmdlen, 
		void *buf, size_t len, int flags)
{
	struct memory_softc *sc = cookie;
	size_t off;

	if (op != I2C_OP_READ_WITH_STOP || cmdlen != 1)
		return (EINVAL);

	off = (addr - 0x50) * SPD_SIZE + *(const u_char *)cmdbuf;
	if ((off + len) > sc->sc_len)
		return (EIO);

	memcpy(buf, &sc->sc_buf[off], len);
	return (0);
}


