/*	$NetBSD: mq200var.h,v 1.1.4.3 2001/03/12 13:28:38 bouyer Exp $	*/

/*-
 * Copyright (c) 2000 Takemura Shin
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

#include <machine/bus.h>
#include <machine/config_hook.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <dev/hpc/hpcfbvar.h>
#include <dev/hpc/hpcfbio.h>

struct mq200_softc {
	struct device		sc_dev;
	bus_addr_t		sc_baseaddr;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	void			*sc_powerhook;	/* power management hook */
	config_hook_tag		sc_hardpowerhook;
	int			sc_powerstate;
#define	PWRSTAT_SUSPEND		(1<<0)
#define	PWRSTAT_VIDEOOFF	(1<<1)
#define	PWRSTAT_LCD		(1<<2)
#define	PWRSTAT_BACKLIGHT	(1<<3)
#define PWRSTAT_ALL		(0xffffffff)
	int			sc_brightness;
	int			sc_brightness_save;
	int			sc_max_brightness;
	int			sc_contrast;
	int			sc_max_contrast;
	int			sc_mq200pwstate; /* mq200 power state */
	struct hpcfb_fbconf	sc_fbconf;
	struct hpcfb_dspconf	sc_dspconf;
};

int	mq200_probe	__P((bus_space_tag_t, bus_space_handle_t));
void	mq200_attach	__P((struct mq200_softc *));
