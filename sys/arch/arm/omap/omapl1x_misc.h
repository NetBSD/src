/* $NetBSD: omapl1x_misc.h,v 1.1.10.2 2014/08/20 00:02:47 tls Exp $ */
/*
 * Copyright (c) 2013 Sughosh Ganu
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_OMAP_OMAPL1X_MISC_H_
#define _ARM_OMAP_OMAPL1X_MISC_H_


#ifndef STATHZ
# define STATHZ	64
#endif

#define USB0REF_FREQ		0xf
#define USB0REF_FREQ_24M	0x2

#define USB0VBDTCTEN		__BIT(4)
#define USB0SESNDEN		__BIT(5)
#define USB0PHY_PLLON		__BIT(6)
#define USB1SUSPENDM		__BIT(7)
#define USB0OTGPWDN		__BIT(9)
#define USB0PHYPWDN		__BIT(10)
#define USB0PHYCLKMUX		__BIT(11)
#define USB1PHYCLKMUX		__BIT(12)
#define USB0OTGMODE		__BITS(13,14)
#define RESET			__BIT(15)
#define USB0PHYCLKGD		__BIT(17)

#define SYSCFG_KICK0R_KEY	0x83E70B13
#define SYSCFG_KICK1R_KEY	0x95A4F1E0

#define PSC1_USB20_MODULE	0x1
#define PSC1_USB11_MODULE	0x2

struct omapl1x_syscfg {
	bus_space_tag_t		syscfg_iot;	/* Bus tag */
	bus_addr_t		syscfg_addr;	/* Address */
	bus_space_handle_t	syscfg_ioh;
	bus_size_t		syscfg_size;
};

void	omapl1x_reset(void);
uint8_t omapl1x_lpsc_enable(uint32_t module);
uint8_t omapl1x_usbclk_enable(struct omapl1x_syscfg *syscfg);
uint64_t omapl1x_get_tc_freq(void);

#endif /* ARM_OMAP__OMAPL1X_MISC_H_ */
