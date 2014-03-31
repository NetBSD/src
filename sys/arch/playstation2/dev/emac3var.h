/*	$NetBSD: emac3var.h,v 1.5 2014/03/31 11:25:49 martin Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

struct emac3_softc {
	struct device dev;
	struct mii_data mii;
	u_int8_t eaddr[ETHER_ADDR_LEN];
	u_int32_t mode1_reg;
};

int emac3_init(struct emac3_softc *);
int emac3_reset(struct emac3_softc *);
void emac3_exit(struct emac3_softc *);

void emac3_enable(void);
void emac3_disable(void);

void emac3_intr_enable(void);
void emac3_intr_disable(void);
void emac3_intr_clear(void);
int emac3_intr(void *);

void emac3_tx_kick(void);
int emac3_tx_done(void);

void emac3_setmulti(struct emac3_softc *, struct ethercom *);

int emac3_phy_readreg(struct device *, int, int);
void emac3_phy_writereg(struct device *, int, int, int);
void emac3_phy_statchg(struct device *);
