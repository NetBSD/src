/*	$NetBSD: mq200var.h,v 1.7.4.2 2001/09/16 05:32:19 uch Exp $	*/

/*-
 * Copyright (c) 2000, 2001 TAKEMURA Shin
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
	int			sc_powerstate; /* power state related by LCD */
#define	PWRSTAT_SUSPEND		(1<<0)
#define	PWRSTAT_VIDEOOFF	(1<<1)
#define	PWRSTAT_LCD		(1<<2)
#define	PWRSTAT_BACKLIGHT	(1<<3)
#define PWRSTAT_ALL		(0xffffffff)
	int			sc_lcd_inited;
#define BACKLIGHT_INITED	(1<<0)
#define	BRIGHTNESS_INITED	(1<<1)
#define	CONTRAST_INITED		(1<<2)
	int			sc_brightness;
	int			sc_brightness_save;
	int			sc_max_brightness;
	int			sc_contrast;
	int			sc_max_contrast;
	int			sc_mq200pwstate; /* mq200 power state */
	struct hpcfb_fbconf	sc_fbconf;
	struct hpcfb_dspconf	sc_dspconf;

	int			sc_baseclock;
#define MQ200_SC_GC1_ENABLE	(1<<0)	/* XXX, You can't change this	*/
#define MQ200_SC_GC2_ENABLE	(1<<1)	/* XXX, You can't change this	*/
#define MQ200_SC_GC_MASK	(MQ200_SC_GC1_ENABLE|MQ200_SC_GC2_ENABLE)
	char			sc_flags;
	int			sc_width[2], sc_height[2];
	const struct mq200_md_param	*sc_md;
	const struct mq200_crt_param	*sc_crt;

#define MQ200_I_DCMISC	0
#define MQ200_I_PLL(n)	(MQ200_I_DCMISC + (n) - 1)	/* 1, 2	*/
#define MQ200_I_PMC	3
#define MQ200_I_MM01	4
#define MQ200_I_GCC(n)	(5 + (n))			/* 5, 6	*/
#define MQ200_I_MAX	7
	struct mq200_regctx {
		int		offset;
		u_int32_t	val;
	} sc_regctxs[MQ200_I_MAX];
};

int	mq200_probe(bus_space_tag_t, bus_space_handle_t);
void	mq200_attach(struct mq200_softc *);
