/*	$NetBSD: if_uaxreg.h,v 1.1 2003/02/16 17:18:47 augustss Exp $	*/

/*
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net).
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#define UAX_CONFIG_NO		1
#define UAX_IFACE_IDX		0

#define UAX_MAX_PHYS		2

#define UAX_TX_TIMEOUT		2000 /* ms */
#define UAX_BUFSZ		(ETHER_MAX_LEN - ETHER_CRC_LEN)

/* Requests */
#define UAX_READ_SRAM		0x02
#define UAX_WRITE_RX_SRAM	0x03
#define UAX_WRITE_TX_SRAM	0x04
#define UAX_SOFTWARE_MII	0x06
#define UAX_READ_MII_REG	0x07
#define UAX_WRITE_MII_REG	0x08
#define UAX_READ_MII_OPMODE	0x09
#define UAX_HARDWARE_MII	0x0a
#define UAX_READ_SROM		0x0b
#define UAX_WRITE_SROM		0x0c
#define UAX_WRITE_SROM_ENABLE	0x0d
#define UAX_WRITE_SROM_DISABLE	0x0e
#define UAX_READ_RX_CTRL	0x0f
#define UAX_WRITE_RX_CTRL	0x10
#define  UAX_RX_PROMISCUOUS		0x01
#define  UAX_RX_ALL_MULTICAST		0x02
#define  UAX_RX_DIRECTED		0x04
#define  UAX_RX_BROADCAST		0x08
#define  UAX_RX_MULTICAST		0x10
#define  UAX_RX_ALTERNATE		0x80
#define UAX_READ_IPGS		0x11
#define UAX_WRITE_IPG		0x12
#define UAX_WRITE_IPG1		0x13
#define UAX_WRITE_IPG2		0x14
#define UAX_READ_MULTI_FILTER	0x15
#define  UAX_MULTI_FILTER_SIZE		8
#define UAX_WRITE_MULTI_FILTER	0x16
#define UAX_READ_NODEID		0x17
#define UAX_READ_PHYID		0x19
#define  UAX_GET_PHY(r)			((r) & 0x1f)
#define  UAX_GET_PHY_TYPE(r)		(((r) >> 5) & 0x07)
#define UAX_READ_MEDIUM_STATUS	0x1a
#define UAX_WRITE_MEDIUM_STATUS	0x1b
#define UAX_GET_MONITOR_MODE	0x1c
#define UAX_SET_MONITOR_MODE	0x1d
#define UAX_READ_GPIOS		0x1e
#define UAX_WRITE_GPIOS		0x1f

#define UAX_INTR_PKTLEN 8
#define UAX_INTR_INTERVAL 100	/* XXX */
struct uax_intrpkt {
	u_int8_t		uax_intrdata[UAX_INTR_PKTLEN];
};

