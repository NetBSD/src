/*	$NetBSD: mii_adapter.h,v 1.2 1997/11/17 08:56:08 thorpej Exp $	*/
 
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
 *	This product includes software developed by Manuel Bouyer.
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

/* The mii bus data definitions */

/*
 * Services provided by or to the adapter :
 * set/clear/read bit, where bit may be MII_DATA, MII_CLOCK, MII_TXEN.
 */

typedef struct _mii_data {
	void *adapter_softc;
	u_int32_t adapter_id; /* adapter ID, see mii_adapters_id.h */

	/*
	 * Services provided by the adapter :
	 * set/clear/read bit, where bit may be MII_DATA, MII_CLOCK, MII_TXEN
	 * or read/write PHYs registers.
	 */
	void (*mii_setbit)  __P((void *adapter_softc, u_int8_t bit));
	void (*mii_clrbit)  __P((void *adapter_softc, u_int8_t bit));
	int  (*mii_readbit) __P((void *adapter_softc, u_int8_t bit));
	int  (*mii_readreg)  __P((void *, u_int16_t, u_int16_t));
	void (*mii_writereg) __P((void *, u_int16_t, u_int16_t, u_int16_t));

	/*
	 * services provided to the adapter:
	 */
	int mii_media_status;
	int mii_media_active;

	/*
	 * mii's private data
	 */
	struct mii_softc *mii_sc;
} mii_data_t;

/* bit types */
#define	MII_DATA	0x00
#define	MII_CLOCK	0x01
#define	MII_TXEN	0x02

void	mii_media_add __P((struct ifmedia *,  mii_data_t*));
int	mii_mediachg __P((mii_data_t*));
void	mii_pollstat __P((mii_data_t*));
