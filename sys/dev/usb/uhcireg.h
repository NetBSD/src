/*	$NetBSD: uhcireg.h,v 1.19.46.1 2017/12/03 11:37:34 jdolecek Exp $	*/
/*	$FreeBSD: src/sys/dev/usb/uhcireg.h,v 1.12 1999/11/17 22:33:42 n_hibma Exp $ */

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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

#ifndef _DEV_USB_UHCIREG_H_
#define _DEV_USB_UHCIREG_H_

/*** PCI config registers ***/

#define PCI_USBREV		0x60	/* USB protocol revision */
#define  PCI_USBREV_MASK	0xff
#define  PCI_USBREV_PRE_1_0	0x00
#define  PCI_USBREV_1_0		0x10
#define  PCI_USBREV_1_1		0x11

#define PCI_LEGSUP		0xc0	/* Legacy Support register */
#define  PCI_LEGSUP_A20PTS	__BIT(15)	/* End of A20GATE passthru status */
#define  PCI_LEGSUP_USBPIRQDEN	__BIT(13)	/* USB PIRQ D Enable */
#define  PCI_LEGSUP_USBIRQS	__BIT(12)	/* USB IRQ status */
#define  PCI_LEGSUP_TBY64W	__BIT(11)	/* Trap by 64h write status */
#define  PCI_LEGSUP_TBY64R	__BIT(10)	/* Trap by 64h read status */
#define  PCI_LEGSUP_TBY60W	__BIT(9)	/* Trap by 60h write status */
#define  PCI_LEGSUP_TBY60R	__BIT(8)	/* Trap by 60h read status */
#define  PCI_LEGSUP_SMIEPTE	__BIT(7)	/* SMI at end of passthru enable */
#define  PCI_LEGSUP_PSS		__BIT(6)	/* Passthru status */
#define  PCI_LEGSUP_A20PTEN	__BIT(5)	/* A20GATE passthru enable */
#define  PCI_LEGSUP_USBSMIEN	__BIT(4)	/* Enable SMI# generation */

#define PCI_CBIO		0x20	/* configuration base IO */

#define PCI_INTERFACE_UHCI	0x00

/*** UHCI registers ***/

#define UHCI_CMD		0x00
#define  UHCI_CMD_RS		__BIT(0)
#define  UHCI_CMD_HCRESET	__BIT(1)
#define  UHCI_CMD_GRESET	__BIT(2)
#define  UHCI_CMD_EGSM		__BIT(3)
#define  UHCI_CMD_FGR		__BIT(4)
#define  UHCI_CMD_SWDBG		__BIT(5)
#define  UHCI_CMD_CF		__BIT(6)
#define  UHCI_CMD_MAXP		__BIT(7)

#define UHCI_STS		0x02
#define  UHCI_STS_USBINT	__BIT(0)
#define  UHCI_STS_USBEI		__BIT(1)
#define  UHCI_STS_RD		__BIT(2)
#define  UHCI_STS_HSE		__BIT(3)
#define  UHCI_STS_HCPE		__BIT(4)
#define  UHCI_STS_HCH		__BIT(5)
#define  UHCI_STS_ALLINTRS	__BITS(5,0)

#define UHCI_INTR		0x04
#define  UHCI_INTR_TOCRCIE	__BIT(0)
#define  UHCI_INTR_RIE		__BIT(1)
#define  UHCI_INTR_IOCE		__BIT(2)
#define  UHCI_INTR_SPIE		__BIT(3)

#define UHCI_FRNUM		0x06
#define  UHCI_FRNUM_MASK	__BITS(9,0)


#define UHCI_FLBASEADDR		0x08

#define UHCI_SOF		0x0c
#define  UHCI_SOF_MASK		__BITS(6,0)

#define UHCI_PORTSC1		0x010
#define UHCI_PORTSC2		0x012
#define UHCI_PORTSC_CCS		__BIT(0)
#define UHCI_PORTSC_CSC		__BIT(1)
#define UHCI_PORTSC_PE		__BIT(2)
#define UHCI_PORTSC_POEDC	__BIT(3)
#define UHCI_PORTSC_LS_MASK	__BITS(5,4)
#define UHCI_PORTSC_GET_LS(p)	__SHIFTOUT((p), UHCI_PORTSC_LS_MASK)
#define UHCI_PORTSC_RD		__BIT(6)
#define UHCI_PORTSC_LSDA	__BIT(8)
#define UHCI_PORTSC_PR		__BIT(9)
#define UHCI_PORTSC_OCI		__BIT(10)
#define UHCI_PORTSC_OCIC	__BIT(11)
#define UHCI_PORTSC_SUSP	__BIT(12)

#define URWMASK(x) \
  ((x) & (UHCI_PORTSC_SUSP | UHCI_PORTSC_PR | UHCI_PORTSC_RD | UHCI_PORTSC_PE))

#define UHCI_FRAMELIST_COUNT	1024
#define UHCI_FRAMELIST_ALIGN	4096

#define UHCI_TD_ALIGN		16
#define UHCI_QH_ALIGN		16

typedef uint32_t uhci_physaddr_t;
#define UHCI_PTR_T		__BIT(0)
#define UHCI_PTR_TD		0x00000000
#define UHCI_PTR_QH		__BIT(1)
#define UHCI_PTR_VF		__BIT(2)

/*
 * Wait this long after a QH has been removed.  This gives that HC a
 * chance to stop looking at it before it's recycled.
 */
#define UHCI_QH_REMOVE_DELAY	5

/*
 * The Queue Heads and Transfer Descriptors are accessed
 * by both the CPU and the USB controller which run
 * concurrently.  This means that they have to be accessed
 * with great care.  As long as the data structures are
 * not linked into the controller's frame list they cannot
 * be accessed by it and anything goes.  As soon as a
 * TD is accessible by the controller it "owns" the td_status
 * field; it will not be written by the CPU.  Similarly
 * the controller "owns" the qh_elink field.
 */

typedef struct {
	volatile uhci_physaddr_t td_link;
	volatile uint32_t td_status;
#define UHCI_TD_ACTLEN_MASK	__BITS(10,0)
#define UHCI_TD_GET_ACTLEN(s)	\
    ((__SHIFTOUT((s), UHCI_TD_ACTLEN_MASK) + 1) & __SHIFTOUT_MASK(UHCI_TD_ACTLEN_MASK))
#define UHCI_TD_ZERO_ACTLEN(t)	((t) | 0x3ff)
#define UHCI_TD_BITSTUFF	__BIT(17)
#define UHCI_TD_CRCTO		__BIT(18)
#define UHCI_TD_NAK		__BIT(19)
#define UHCI_TD_BABBLE		__BIT(20)
#define UHCI_TD_DBUFFER		__BIT(21)
#define UHCI_TD_STALLED		__BIT(22)
#define UHCI_TD_ACTIVE		__BIT(23)
#define UHCI_TD_STATUS_MASK	__BITS(16,23)
#define UHCI_TD_IOC		__BIT(24)
#define UHCI_TD_IOS		__BIT(25)
#define UHCI_TD_LS		__BIT(26)
#define UHCI_TD_ERRCNT_MASK	__BITS(28,27)
#define UHCI_TD_GET_ERRCNT(s)	__SHIFTOUT((s), UHCI_TD_ERRCNT_MASK)
#define UHCI_TD_SET_ERRCNT(n)	__SHIFTIN((n), UHCI_TD_ERRCNT_MASK)
#define UHCI_TD_SPD		__BIT(29)
	volatile uint32_t td_token;
#define UHCI_TD_PID_IN		0x69
#define UHCI_TD_PID_OUT		0xe1
#define UHCI_TD_PID_SETUP	0x2d
#define UHCI_TD_PID_MASK	__BITS(7,0)
#define UHCI_TD_SET_PID(p)	__SHIFTIN((p), UHCI_TD_PID_MASK)
#define UHCI_TD_GET_PID(s)	__SHIFTOUT((s), UHCI_TD_PID_MASK)
#define UHCI_TD_DEVADDR_MASK	__BITS(14,8)
#define UHCI_TD_SET_DEVADDR(a)	__SHIFTIN((a), UHCI_TD_DEVADDR_MASK)
#define UHCI_TD_GET_DEVADDR(s)	__SHIFTOUT((s), UHCI_TD_DEVADDR_MASK)
#define UHCI_TD_ENDPT_MASK	__BITS(18,15)
#define UHCI_TD_SET_ENDPT(e)	__SHIFTIN((e), UHCI_TD_ENDPT_MASK)
#define UHCI_TD_GET_ENDPT(s)	__SHIFTOUT((s), UHCI_TD_ENDPT_MASK)
#define UHCI_TD_DT_MASK		__BIT(19)
#define UHCI_TD_SET_DT(t)	__SHIFTIN((t), UHCI_TD_DT_MASK)
#define UHCI_TD_GET_DT(s)	__SHIFTOUT((s), UHCI_TD_DT_MASK)
#define UHCI_TD_MAXLEN_MASK	__BITS(31,21)
#define UHCI_TD_SET_MAXLEN(l)	\
    __SHIFTIN((((l)-1) & __SHIFTOUT_MASK(UHCI_TD_MAXLEN_MASK)), UHCI_TD_MAXLEN_MASK)
#define UHCI_TD_GET_MAXLEN(s)	\
    (__SHIFTOUT((s), UHCI_TD_MAXLEN_MASK) + 1)
	volatile uint32_t td_buffer;
} uhci_td_t;

#define UHCI_TD_ERROR \
    (UHCI_TD_BITSTUFF|UHCI_TD_CRCTO|UHCI_TD_BABBLE|UHCI_TD_DBUFFER|UHCI_TD_STALLED)

#define UHCI_TD_SETUP(len, endp, dev) \
    (UHCI_TD_SET_MAXLEN(len) | UHCI_TD_SET_ENDPT(endp) | \
    UHCI_TD_SET_DEVADDR(dev) | UHCI_TD_PID_SETUP)
#define UHCI_TD_OUT(len, endp, dev, dt) \
    (UHCI_TD_SET_MAXLEN(len) | UHCI_TD_SET_ENDPT(endp) | \
    UHCI_TD_SET_DEVADDR(dev) | UHCI_TD_PID_OUT | UHCI_TD_SET_DT(dt))
#define UHCI_TD_IN(len, endp, dev, dt) \
    (UHCI_TD_SET_MAXLEN(len) | UHCI_TD_SET_ENDPT(endp) | \
    UHCI_TD_SET_DEVADDR(dev) | UHCI_TD_PID_IN | UHCI_TD_SET_DT(dt))

typedef struct {
	volatile uhci_physaddr_t qh_hlink;
	volatile uhci_physaddr_t qh_elink;
} uhci_qh_t;

#endif /* _DEV_USB_UHCIREG_H_ */
