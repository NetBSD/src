/*	$NetBSD: ite8181var.h,v 1.6 2001/09/16 05:32:18 uch Exp $	*/

/*-
 * Copyright (c) 2000 SATO Kazumi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
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

struct ite8181_softc {
	struct device		sc_dev;
	bus_addr_t		sc_baseaddr;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	int			sc_mba;	/* Memory base offset */
	int			sc_gba;	/* GUI base offset */
	int			sc_sba;	/* Graphics base offset */
	void			*sc_powerhook;	/* power management hook */
	config_hook_tag		sc_hardpowerhook;
 	int			sc_powerstate; /* power state related by LCD */
#define PWRSTAT_SUSPEND         (1<<0)
#define PWRSTAT_VIDEOOFF        (1<<1)
#define PWRSTAT_LCD             (1<<2)
#define PWRSTAT_BACKLIGHT       (1<<3)
#define PWRSTAT_ALL           (0xffffffff)
	int			sc_lcd_inited;
#define	BACKLIGHT_INITED	(1<<0)
#define	BRIGHTNESS_INITED	(1<<1)
#define	CONTRAST_INITED		(1<<2)
	int			sc_brightness;
	int			sc_brightness_save;
	int			sc_max_brightness;
	int			sc_contrast;
	int			sc_max_contrast;

	struct hpcfb_fbconf	sc_fbconf;
	struct hpcfb_dspconf	sc_dspconf;
	int			sc_lcd; /* lcd on/off */
};

int	ite8181_probe(bus_space_tag_t, bus_space_handle_t);
void	ite8181_attach(struct ite8181_softc *);
