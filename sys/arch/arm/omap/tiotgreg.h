/* $NetBSD: tiotgreg.h,v 1.1.2.2 2014/08/10 06:53:52 tls Exp $ */
/*
 * Copyright (c) 2013 Manuel Bouyer.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define TI_OTG_NPORTS 2
/* USBSS registers */
#define TIOTG_USBSS_OFFSET 0
#define TIOTG_USBSS_READ4(sc, reg) \
	bus_space_read_4(sc->sc_iot, sc->sc_ioh, (reg) + TIOTG_USBSS_OFFSET)
#define TIOTG_USBSS_WRITE4(sc, reg, val) \
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, (reg) + TIOTG_USBSS_OFFSET, (val))

#define USBSS_REVREG		0x00  
#define USBSS_SYSCONFIG		0x10
#define	USBSS_SYSCONFIG_USB0_OCP_EN_N	0x800
#define	USBSS_SYSCONFIG_PHY0_UTMI_EN_N	0x400
#define	USBSS_SYSCONFIG_USB1_OCP_EN_N	0x200
#define	USBSS_SYSCONFIG_PHY1_UTMI_EN_N	0x100
#define	USBSS_SYSCONFIG_STBYMODE_SHIFT	4
#define	USBSS_SYSCONFIG_IDLEMODE_SHIFT	2
#define	USBSS_SYSCONFIG_FREEEMU		0x002
#define	USBSS_SYSCONFIG_SRESET		0x001

/* USB control registers */
#define USB_CTRL_OFFSET(port)	(0x1000 + (0x800 * (port)))
#define USB_PORT_SIZE	0x800 /* size of CTRL+PHY+CORE */
#define TIOTG_USBC_READ4(sc, reg) \
	bus_space_read_4(sc->sc_ctrliot, sc->sc_ctrlioh, (reg))
#define TIOTG_USBC_WRITE4(sc, reg, val) \
	bus_space_write_4(sc->sc_ctrliot, sc->sc_ctrlioh, (reg), (val))

#define USBCTRL_REV		0x00
#define USBCTRL_CTRL		0x14
#define USBCTRL_STAT		0x18
#define USBCTRL_IRQ_STAT0	0x30
#define	USBCTRL_IRQ_STAT0_RXSHIFT	16
#define	USBCTRL_IRQ_STAT0_TXSHIFT	0
#define USBCTRL_IRQ_STAT1		0x34
#define	USBCTRL_IRQ_STAT1_DRVVBUS	(1 << 8)
#define USBCTRL_INTEN_SET0	0x38
#define USBCTRL_INTEN_SET1	0x3C  
#define	USBCTRL_INTEN_USB_ALL   	0x1ff
#define	USBCTRL_INTEN_USB_SOF   	(1 << 3)
#define USBCTRL_INTEN_CLR0	0x40  
#define USBCTRL_INTEN_CLR1	0x44  
#define USBCTRL_UTMI	0xE0  
#define	USBCTRL_UTMI_FSDATAEXT		(1 << 1)
#define USBCTRL_MODE	0xE8  
#define	USBCTRL_MODE_IDDIG		(1 << 8)
#define	USBCTRL_MODE_IDDIGMUX		(1 << 7)

#define USB_CORE_OFFSET		0x400
#define USB_CORE_SIZE		0x400
