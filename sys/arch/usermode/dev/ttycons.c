/* $NetBSD: ttycons.c,v 1.1.6.2 2008/01/21 09:39:45 yamt Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Jared D. McNeill.
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
__KERNEL_RCSID(0, "$NetBSD: ttycons.c,v 1.1.6.2 2008/01/21 09:39:45 yamt Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/cons.h>

#include <machine/mainbus.h>

static int	ttycons_match(device_t, cfdata_t, void *);
static void	ttycons_attach(device_t, device_t, void *);

void		ttycons_consinit(void);

typedef struct ttycons_softc {
	device_t	sc_dev;
} ttycons_softc_t;

dev_type_cngetc(ttycons_cngetc);
dev_type_cnputc(ttycons_cnputc);
dev_type_cnpollc(ttycons_cnpollc);

static struct cnm_state ttycons_cnm_state;
struct consdev ttycons_consdev = {
	.cn_getc = ttycons_cngetc,
	.cn_putc = ttycons_cnputc,
	.cn_pollc = ttycons_cnpollc,
};

CFATTACH_DECL_NEW(ttycons, sizeof(ttycons_softc_t),
    ttycons_match, ttycons_attach, NULL, NULL);

static int
ttycons_match(device_t parent, cfdata_t match, void *opaque)
{
	struct thunkbus_attach_args *taa = opaque;

	if (taa->taa_type != THUNKBUS_TYPE_TTYCONS)
		return 0;

	return 1;
}

static void
ttycons_attach(device_t parent, device_t self, void *opaque)
{
	ttycons_softc_t *sc = device_private(self);

	aprint_naive("\n");
	aprint_normal(": console\n");

	sc->sc_dev = self;
}

void
ttycons_consinit(void)
{
	cn_tab = &ttycons_consdev;
	cn_init_magic(&ttycons_cnm_state);
	cn_set_magic("\047\001");
}

int
ttycons_cngetc(dev_t dev)
{
	extern int getchar(void);
	return getchar();
}

void
ttycons_cnputc(dev_t dev, int c)
{
	extern void putchar(int);
	putchar(c);
}

void
ttycons_cnpollc(dev_t dev, int on)
{
}
