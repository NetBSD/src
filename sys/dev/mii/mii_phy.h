/*  $NetBSD: mii_phy.h,v 1.1 1997/10/17 17:34:00 bouyer Exp $   */
 
/*
 * Copyright (c) 1997 Manuel Bouyer.  All rights reserved.
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
 *  This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/* mii/phy interface */
typedef struct _mii_phy {
	u_int32_t phy_id; 
	u_int32_t adapter_id;
	int dev;
	void *mii_softc;
	void *phy_softc;
	int  (*phy_media_set) __P((int, void *));
	int  (*phy_status) __P((int, void*));
	void (*phy_pdown) __P((void*));
	u_int32_t phy_media;
#define PHY_AUI         0x01
#define PHY_BNC         0x02
#define PHY_10baseT     0x04
#define PHY_10baseTfd   0x08
#define PHY_100baseTx   0x10 
#define PHY_100baseTxfd 0x20
#define PHY_100baseT4   0x40
} mii_phy_t;

int mii_readreg __P((void *, u_int16_t, u_int16_t));
void mii_writereg __P((void *, u_int16_t, u_int16_t, u_int16_t));
