/* $NetBSD: rtw.c,v 1.1.2.2 2004/10/19 15:56:56 skrll Exp $ */
/*-
 * Copyright (c) 2004, 2005 David Young.  All rights reserved.
 *
 * Programmed for NetBSD by David Young.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of David Young may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY David Young ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL David
 * Young BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/*
 * Device driver for the Realtek RTL8180 802.11 MAC/BBP.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtw.c,v 1.1.2.2 2004/10/19 15:56:56 skrll Exp $");

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h> 
#include <sys/callout.h>
#include <sys/mbuf.h>   
#include <sys/malloc.h>
#include <sys/kernel.h>
#if 0
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>
#endif
#include <sys/time.h>
#include <sys/types.h>

#include <machine/endian.h>
#include <machine/bus.h>
#include <machine/intr.h>	/* splnet */

#include <uvm/uvm_extern.h>
 
#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_compat.h>
#include <net80211/ieee80211_radiotap.h>

#if NBPFILTER > 0 
#include <net/bpf.h>
#endif 

#include <dev/ic/rtwreg.h>
#include <dev/ic/rtwvar.h>
#include <dev/ic/rtwphyio.h>
#include <dev/ic/rtwphy.h>

#include <dev/ic/smc93cx6var.h>

#define	KASSERT2(__cond, __msg)		\
	do {				\
		if (!(__cond))		\
			panic __msg ;	\
	} while (0)

#ifdef RTW_DEBUG
int rtw_debug = 2;
#endif /* RTW_DEBUG */

#define NEXT_ATTACH_STATE(sc, state) do {				\
	DPRINTF(sc, ("%s: attach state %s\n", __func__, #state));	\
	sc->sc_attach_state = state;					\
} while (0)

int rtw_dwelltime = 1000;	/* milliseconds */

#ifdef RTW_DEBUG
static void
rtw_print_regs(struct rtw_regs *regs, const char *dvname, const char *where)
{
#define PRINTREG32(sc, reg) \
	RTW_DPRINTF2(("%s: reg[ " #reg " / %03x ] = %08x\n", \
	    dvname, reg, RTW_READ(regs, reg)))

#define PRINTREG16(sc, reg) \
	RTW_DPRINTF2(("%s: reg[ " #reg " / %03x ] = %04x\n", \
	    dvname, reg, RTW_READ16(regs, reg)))

#define PRINTREG8(sc, reg) \
	RTW_DPRINTF2(("%s: reg[ " #reg " / %03x ] = %02x\n", \
	    dvname, reg, RTW_READ8(regs, reg)))

	RTW_DPRINTF2(("%s: %s\n", dvname, where));

	PRINTREG32(regs, RTW_IDR0);
	PRINTREG32(regs, RTW_IDR1);
	PRINTREG32(regs, RTW_MAR0);
	PRINTREG32(regs, RTW_MAR1);
	PRINTREG32(regs, RTW_TSFTRL);
	PRINTREG32(regs, RTW_TSFTRH);
	PRINTREG32(regs, RTW_TLPDA);
	PRINTREG32(regs, RTW_TNPDA);
	PRINTREG32(regs, RTW_THPDA);
	PRINTREG32(regs, RTW_TCR);
	PRINTREG32(regs, RTW_RCR);
	PRINTREG32(regs, RTW_TINT);
	PRINTREG32(regs, RTW_TBDA);
	PRINTREG32(regs, RTW_ANAPARM);
	PRINTREG32(regs, RTW_BB);
	PRINTREG32(regs, RTW_PHYCFG);
	PRINTREG32(regs, RTW_WAKEUP0L);
	PRINTREG32(regs, RTW_WAKEUP0H);
	PRINTREG32(regs, RTW_WAKEUP1L);
	PRINTREG32(regs, RTW_WAKEUP1H);
	PRINTREG32(regs, RTW_WAKEUP2LL);
	PRINTREG32(regs, RTW_WAKEUP2LH);
	PRINTREG32(regs, RTW_WAKEUP2HL);
	PRINTREG32(regs, RTW_WAKEUP2HH);
	PRINTREG32(regs, RTW_WAKEUP3LL);
	PRINTREG32(regs, RTW_WAKEUP3LH);
	PRINTREG32(regs, RTW_WAKEUP3HL);
	PRINTREG32(regs, RTW_WAKEUP3HH);
	PRINTREG32(regs, RTW_WAKEUP4LL);
	PRINTREG32(regs, RTW_WAKEUP4LH);
	PRINTREG32(regs, RTW_WAKEUP4HL);
	PRINTREG32(regs, RTW_WAKEUP4HH);
	PRINTREG32(regs, RTW_DK0);
	PRINTREG32(regs, RTW_DK1);
	PRINTREG32(regs, RTW_DK2);
	PRINTREG32(regs, RTW_DK3);
	PRINTREG32(regs, RTW_RETRYCTR);
	PRINTREG32(regs, RTW_RDSAR);
	PRINTREG32(regs, RTW_FER);
	PRINTREG32(regs, RTW_FEMR);
	PRINTREG32(regs, RTW_FPSR);
	PRINTREG32(regs, RTW_FFER);

	/* 16-bit registers */
	PRINTREG16(regs, RTW_BRSR);
	PRINTREG16(regs, RTW_IMR);
	PRINTREG16(regs, RTW_ISR);
	PRINTREG16(regs, RTW_BCNITV);
	PRINTREG16(regs, RTW_ATIMWND);
	PRINTREG16(regs, RTW_BINTRITV);
	PRINTREG16(regs, RTW_ATIMTRITV);
	PRINTREG16(regs, RTW_CRC16ERR);
	PRINTREG16(regs, RTW_CRC0);
	PRINTREG16(regs, RTW_CRC1);
	PRINTREG16(regs, RTW_CRC2);
	PRINTREG16(regs, RTW_CRC3);
	PRINTREG16(regs, RTW_CRC4);
	PRINTREG16(regs, RTW_CWR);

	/* 8-bit registers */
	PRINTREG8(regs, RTW_CR);
	PRINTREG8(regs, RTW_9346CR);
	PRINTREG8(regs, RTW_CONFIG0);
	PRINTREG8(regs, RTW_CONFIG1);
	PRINTREG8(regs, RTW_CONFIG2);
	PRINTREG8(regs, RTW_MSR);
	PRINTREG8(regs, RTW_CONFIG3);
	PRINTREG8(regs, RTW_CONFIG4);
	PRINTREG8(regs, RTW_TESTR);
	PRINTREG8(regs, RTW_PSR);
	PRINTREG8(regs, RTW_SCR);
	PRINTREG8(regs, RTW_PHYDELAY);
	PRINTREG8(regs, RTW_CRCOUNT);
	PRINTREG8(regs, RTW_PHYADDR);
	PRINTREG8(regs, RTW_PHYDATAW);
	PRINTREG8(regs, RTW_PHYDATAR);
	PRINTREG8(regs, RTW_CONFIG5);
	PRINTREG8(regs, RTW_TPPOLL);

	PRINTREG16(regs, RTW_BSSID16);
	PRINTREG32(regs, RTW_BSSID32);
#undef PRINTREG32
#undef PRINTREG16
#undef PRINTREG8
}
#endif /* RTW_DEBUG */

void
rtw_continuous_tx_enable(struct rtw_regs *regs, int enable)
{
	u_int32_t tcr;
	tcr = RTW_READ(regs, RTW_TCR);
	tcr &= ~RTW_TCR_LBK_MASK;
	if (enable)
		tcr |= RTW_TCR_LBK_CONT;
	else
		tcr |= RTW_TCR_LBK_NORMAL;
	RTW_WRITE(regs, RTW_TCR, tcr);
	RTW_SYNC(regs, RTW_TCR, RTW_TCR);
	if (enable) {
		rtw_config0123_enable(regs, 1);
		rtw_anaparm_enable(regs, 1);
		rtw_txdac_enable(regs, 0);
	} else {
		rtw_txdac_enable(regs, 1);
		rtw_anaparm_enable(regs, 0);
		rtw_config0123_enable(regs, 0);
	}
}

/*
 * Enable registers, switch register banks.
 */
void
rtw_config0123_enable(struct rtw_regs *regs, int enable)
{
	u_int8_t ecr;
	ecr = RTW_READ8(regs, RTW_9346CR);
	ecr &= ~(RTW_9346CR_EEM_MASK | RTW_9346CR_EECS | RTW_9346CR_EESK);
	if (enable)
		ecr |= RTW_9346CR_EEM_CONFIG;
	else
		ecr |= RTW_9346CR_EEM_NORMAL;
	RTW_WRITE8(regs, RTW_9346CR, ecr);
	RTW_SYNC(regs, RTW_9346CR, RTW_9346CR);
}

/* requires rtw_config0123_enable(, 1) */
void
rtw_anaparm_enable(struct rtw_regs *regs, int enable)
{
	u_int8_t cfg3;

	cfg3 = RTW_READ8(regs, RTW_CONFIG3);
	cfg3 |= RTW_CONFIG3_PARMEN | RTW_CONFIG3_CLKRUNEN;
	if (!enable)
		cfg3 &= ~RTW_CONFIG3_PARMEN;
	RTW_WRITE8(regs, RTW_CONFIG3, cfg3);
	RTW_SYNC(regs, RTW_CONFIG3, RTW_CONFIG3);
}

/* requires rtw_anaparm_enable(, 1) */
void
rtw_txdac_enable(struct rtw_regs *regs, int enable)
{
	u_int32_t anaparm;

	anaparm = RTW_READ(regs, RTW_ANAPARM);
	if (enable)
		anaparm &= ~RTW_ANAPARM_TXDACOFF;
	else
		anaparm |= RTW_ANAPARM_TXDACOFF;
	RTW_WRITE(regs, RTW_ANAPARM, anaparm);
	RTW_SYNC(regs, RTW_ANAPARM, RTW_ANAPARM);
}

static __inline int
rtw_chip_reset(struct rtw_regs *regs, char (*dvname)[IFNAMSIZ])
{
	int i;
	u_int8_t cr;
	uint32_t tcr;

	/* from Linux driver */
	tcr = RTW_TCR_CWMIN | RTW_TCR_MXDMA_2048 |
	      LSHIFT(7, RTW_TCR_SRL_MASK) | LSHIFT(7, RTW_TCR_LRL_MASK);

	RTW_WRITE(regs, RTW_TCR, tcr);

	RTW_WBW(regs, RTW_CR, RTW_TCR);

	RTW_WRITE8(regs, RTW_CR, RTW_CR_RST);

	RTW_WBR(regs, RTW_CR, RTW_CR);

	for (i = 0; i < 10000; i++) {
		if ((cr = RTW_READ8(regs, RTW_CR) & RTW_CR_RST) == 0) {
			RTW_DPRINTF(("%s: reset in %dus\n", *dvname, i));
			return 0;
		}
		RTW_RBR(regs, RTW_CR, RTW_CR);
		DELAY(1); /* 1us */
	}

	printf("%s: reset failed\n", *dvname);
	return ETIMEDOUT;
}

static __inline int
rtw_recall_eeprom(struct rtw_regs *regs, char (*dvname)[IFNAMSIZ])
{
	int i;
	u_int8_t ecr;

	ecr = RTW_READ8(regs, RTW_9346CR);
	ecr = (ecr & ~RTW_9346CR_EEM_MASK) | RTW_9346CR_EEM_AUTOLOAD;
	RTW_WRITE8(regs, RTW_9346CR, ecr);

	RTW_WBR(regs, RTW_9346CR, RTW_9346CR);

	/* wait 2.5ms for completion */
	for (i = 0; i < 25; i++) {
		ecr = RTW_READ8(regs, RTW_9346CR);
		if ((ecr & RTW_9346CR_EEM_MASK) == RTW_9346CR_EEM_NORMAL) {
			RTW_DPRINTF(("%s: recall EEPROM in %dus\n", *dvname,
			    i * 100));
			return 0;
		}
		RTW_RBR(regs, RTW_9346CR, RTW_9346CR);
		DELAY(100);
	}
	printf("%s: recall EEPROM failed\n", *dvname);
	return ETIMEDOUT;
}

static __inline int
rtw_reset(struct rtw_softc *sc)
{
	int rc;
	uint32_t config1;

	if ((rc = rtw_chip_reset(&sc->sc_regs, &sc->sc_dev.dv_xname)) != 0)
		return rc;

	if ((rc = rtw_recall_eeprom(&sc->sc_regs, &sc->sc_dev.dv_xname)) != 0)
		;

	config1 = RTW_READ(&sc->sc_regs, RTW_CONFIG1);
	RTW_WRITE(&sc->sc_regs, RTW_CONFIG1, config1 & ~RTW_CONFIG1_PMEN);
	/* TBD turn off maximum power saving? */

	return 0;
}

static __inline int
rtw_txdesc_dmamaps_create(bus_dma_tag_t dmat, struct rtw_txctl *descs,
    u_int ndescs)
{
	int i, rc = 0;
	for (i = 0; i < ndescs; i++) {
		rc = bus_dmamap_create(dmat, MCLBYTES, RTW_MAXPKTSEGS, MCLBYTES,
		    0, 0, &descs[i].stx_dmamap);
		if (rc != 0)
			break;
	}
	return rc;
}

static __inline int
rtw_rxdesc_dmamaps_create(bus_dma_tag_t dmat, struct rtw_rxctl *descs,
    u_int ndescs)
{
	int i, rc = 0;
	for (i = 0; i < ndescs; i++) {
		rc = bus_dmamap_create(dmat, MCLBYTES, 1, MCLBYTES, 0, 0,
		    &descs[i].srx_dmamap);
		if (rc != 0)
			break;
	}
	return rc;
}

static __inline void
rtw_rxctls_setup(struct rtw_rxctl (*descs)[RTW_RXQLEN])
{
	int i;
	for (i = 0; i < RTW_RXQLEN; i++)
		(*descs)[i].srx_mbuf = NULL;
}

static __inline void
rtw_rxdesc_dmamaps_destroy(bus_dma_tag_t dmat, struct rtw_rxctl *descs,
    u_int ndescs)
{
	int i;
	for (i = 0; i < ndescs; i++) {
		if (descs[i].srx_dmamap != NULL)
			bus_dmamap_destroy(dmat, descs[i].srx_dmamap);
	}
}

static __inline void
rtw_txdesc_dmamaps_destroy(bus_dma_tag_t dmat, struct rtw_txctl *descs,
    u_int ndescs)
{
	int i;
	for (i = 0; i < ndescs; i++) {
		if (descs[i].stx_dmamap != NULL)
			bus_dmamap_destroy(dmat, descs[i].stx_dmamap);
	}
}

static __inline void
rtw_srom_free(struct rtw_srom *sr)
{
	sr->sr_size = 0;
	if (sr->sr_content == NULL)
		return;
	free(sr->sr_content, M_DEVBUF);
	sr->sr_content = NULL;
}

static void
rtw_srom_defaults(struct rtw_srom *sr, u_int32_t *flags, u_int8_t *cs_threshold,
    enum rtw_rfchipid *rfchipid, u_int32_t *rcr, char (*dvname)[IFNAMSIZ])
{
	*flags |= (RTW_F_DIGPHY|RTW_F_ANTDIV);
	*cs_threshold = RTW_SR_ENERGYDETTHR_DEFAULT;
	*rcr |= RTW_RCR_ENCS1;
	*rfchipid = RTW_RFCHIPID_PHILIPS;
}

static int
rtw_srom_parse(struct rtw_srom *sr, u_int32_t *flags, u_int8_t *cs_threshold,
    enum rtw_rfchipid *rfchipid, u_int32_t *rcr, enum rtw_locale *locale,
    char (*dvname)[IFNAMSIZ])
{
	int i;
	const char *rfname, *paname;
	char scratch[sizeof("unknown 0xXX")];
	u_int16_t version;
	u_int8_t mac[IEEE80211_ADDR_LEN];

	*flags &= ~(RTW_F_DIGPHY|RTW_F_DFLANTB|RTW_F_ANTDIV);
	*rcr &= ~(RTW_RCR_ENCS1 | RTW_RCR_ENCS2);

	version = RTW_SR_GET16(sr, RTW_SR_VERSION);
	printf("%s: SROM version %d.%d", *dvname, version >> 8, version & 0xff);

	if (version <= 0x0101) {
		printf(" is not understood, limping along with defaults\n");
		rtw_srom_defaults(sr, flags, cs_threshold, rfchipid, rcr,
		    dvname);
		return 0;
	}
	printf("\n");

	for (i = 0; i < IEEE80211_ADDR_LEN; i++)
		mac[i] = RTW_SR_GET(sr, RTW_SR_MAC + i);

	RTW_DPRINTF(("%s: EEPROM MAC %s\n", *dvname, ether_sprintf(mac)));

	*cs_threshold = RTW_SR_GET(sr, RTW_SR_ENERGYDETTHR);

	if ((RTW_SR_GET(sr, RTW_SR_CONFIG2) & RTW_CONFIG2_ANT) != 0)
		*flags |= RTW_F_ANTDIV;

	if ((RTW_SR_GET(sr, RTW_SR_RFPARM) & RTW_SR_RFPARM_DIGPHY) != 0)
		*flags |= RTW_F_DIGPHY;
	if ((RTW_SR_GET(sr, RTW_SR_RFPARM) & RTW_SR_RFPARM_DFLANTB) != 0)
		*flags |= RTW_F_DFLANTB;

	*rcr |= LSHIFT(MASK_AND_RSHIFT(RTW_SR_GET(sr, RTW_SR_RFPARM),
	    RTW_SR_RFPARM_CS_MASK), RTW_RCR_ENCS1);

	*rfchipid = RTW_SR_GET(sr, RTW_SR_RFCHIPID);
	switch (*rfchipid) {
	case RTW_RFCHIPID_GCT:		/* this combo seen in the wild */
		rfname = "GCT GRF5101";
		paname = "Winspring WS9901";
		break;
	case RTW_RFCHIPID_MAXIM:
		rfname = "Maxim MAX2820";	/* guess */
		paname = "Maxim MAX2422";	/* guess */
		break;
	case RTW_RFCHIPID_INTERSIL:
		rfname = "Intersil HFA3873";	/* guess */
		paname = "Intersil <unknown>";
		break;
	case RTW_RFCHIPID_PHILIPS:	/* this combo seen in the wild */
		rfname = "Philips SA2400A";
		paname = "Philips SA2411";
		break;
	case RTW_RFCHIPID_RFMD:
		/* this is the same front-end as an atw(4)! */
		rfname = "RFMD RF2948B, "	/* mentioned in Realtek docs */
			 "LNA: RFMD RF2494, "	/* mentioned in Realtek docs */
			 "SYN: Silicon Labs Si4126";	/* inferred from
			 				 * reference driver
							 */
		paname = "RFMD RF2189";		/* mentioned in Realtek docs */
		break;
	case RTW_RFCHIPID_RESERVED:
		rfname = paname = "reserved";
		break;
	default:
		snprintf(scratch, sizeof(scratch), "unknown 0x%02x", *rfchipid);
		rfname = paname = scratch;
	}
	printf("%s: RF: %s, PA: %s\n", *dvname, rfname, paname);

	switch (RTW_SR_GET(sr, RTW_SR_CONFIG0) & RTW_CONFIG0_GL_MASK) {
	case RTW_CONFIG0_GL_USA:
		*locale = RTW_LOCALE_USA;
		break;
	case RTW_CONFIG0_GL_EUROPE:
		*locale = RTW_LOCALE_EUROPE;
		break;
	case RTW_CONFIG0_GL_JAPAN:
		*locale = RTW_LOCALE_JAPAN;
		break;
	default:
		*locale = RTW_LOCALE_UNKNOWN;
		break;
	}
	return 0;
}

/* Returns -1 on failure. */
static int
rtw_srom_read(struct rtw_regs *regs, u_int32_t flags, struct rtw_srom *sr,
    char (*dvname)[IFNAMSIZ])
{
	int rc;
	struct seeprom_descriptor sd;
	u_int8_t ecr;

	(void)memset(&sd, 0, sizeof(sd));

	ecr = RTW_READ8(regs, RTW_9346CR);

	if ((flags & RTW_F_9356SROM) != 0) {
		RTW_DPRINTF(("%s: 93c56 SROM\n", *dvname));
		sr->sr_size = 256;
		sd.sd_chip = C56_66;
	} else {
		RTW_DPRINTF(("%s: 93c46 SROM\n", *dvname));
		sr->sr_size = 128;
		sd.sd_chip = C46;
	}

	ecr &= ~(RTW_9346CR_EEDI | RTW_9346CR_EEDO | RTW_9346CR_EESK |
	    RTW_9346CR_EEM_MASK);
	ecr |= RTW_9346CR_EEM_PROGRAM;

	RTW_WRITE8(regs, RTW_9346CR, ecr);

	sr->sr_content = malloc(sr->sr_size, M_DEVBUF, M_NOWAIT);

	if (sr->sr_content == NULL) {
		printf("%s: unable to allocate SROM buffer\n", *dvname);
		return ENOMEM;
	}

	(void)memset(sr->sr_content, 0, sr->sr_size);

	/* RTL8180 has a single 8-bit register for controlling the
	 * 93cx6 SROM.  There is no "ready" bit. The RTL8180
	 * input/output sense is the reverse of read_seeprom's.
	 */
	sd.sd_tag = regs->r_bt;
	sd.sd_bsh = regs->r_bh;
	sd.sd_regsize = 1;
	sd.sd_control_offset = RTW_9346CR;
	sd.sd_status_offset = RTW_9346CR;
	sd.sd_dataout_offset = RTW_9346CR;
	sd.sd_CK = RTW_9346CR_EESK;
	sd.sd_CS = RTW_9346CR_EECS;
	sd.sd_DI = RTW_9346CR_EEDO;
	sd.sd_DO = RTW_9346CR_EEDI;
	/* make read_seeprom enter EEPROM read/write mode */ 
	sd.sd_MS = ecr;
	sd.sd_RDY = 0;
#if 0
	sd.sd_clkdelay = 50;
#endif

	if (!read_seeprom(&sd, sr->sr_content, 0, sr->sr_size/2)) {
		printf("%s: could not read SROM\n", *dvname);
		free(sr->sr_content, M_DEVBUF);
		sr->sr_content = NULL;
		return -1;	/* XXX */
	}

	/* end EEPROM read/write mode */ 
	RTW_WRITE8(regs, RTW_9346CR,
	    (ecr & ~RTW_9346CR_EEM_MASK) | RTW_9346CR_EEM_NORMAL);
	RTW_WBRW(regs, RTW_9346CR, RTW_9346CR);

	if ((rc = rtw_recall_eeprom(regs, dvname)) != 0)
		return rc;

#ifdef RTW_DEBUG
	{
		int i;
		RTW_DPRINTF(("\n%s: serial ROM:\n\t", *dvname));
		for (i = 0; i < sr->sr_size/2; i++) {
			if (((i % 8) == 0) && (i != 0))
				RTW_DPRINTF(("\n\t"));
			RTW_DPRINTF((" %04x", sr->sr_content[i]));
		}
		RTW_DPRINTF(("\n"));
	}
#endif /* RTW_DEBUG */
	return 0;
}

#if 0
static __inline int
rtw_identify_rf(struct rtw_regs *regs, enum rtw_rftype *rftype,
    char (*dvname)[IFNAMSIZ])
{
	u_int8_t cfg4;
	const char *name;

	cfg4 = RTW_READ8(regs, RTW_CONFIG4);

	switch (cfg4 & RTW_CONFIG4_RFTYPE_MASK) {
	case RTW_CONFIG4_RFTYPE_PHILIPS:
		*rftype = RTW_RFTYPE_PHILIPS;
		name = "Philips";
		break;
	case RTW_CONFIG4_RFTYPE_INTERSIL:
		*rftype = RTW_RFTYPE_INTERSIL;
		name = "Intersil";
		break;
	case RTW_CONFIG4_RFTYPE_RFMD:
		*rftype = RTW_RFTYPE_RFMD;
		name = "RFMD";
		break;
	default:
		name = "<unknown>";
		return ENXIO;
	}

	printf("%s: RF prog type %s\n", *dvname, name);
	return 0;
}
#endif

static __inline void
rtw_init_channels(enum rtw_locale locale,
    struct ieee80211_channel (*chans)[IEEE80211_CHAN_MAX+1],
    char (*dvname)[IFNAMSIZ])
{
	int i;
	const char *name = NULL;
#define ADD_CHANNEL(_chans, _chan) do {			\
	(*_chans)[_chan].ic_flags = IEEE80211_CHAN_B;		\
	(*_chans)[_chan].ic_freq =				\
	    ieee80211_ieee2mhz(_chan, (*_chans)[_chan].ic_flags);\
} while (0)

	switch (locale) {
	case RTW_LOCALE_USA:	/* 1-11 */
		name = "USA";
		for (i = 1; i <= 11; i++)
			ADD_CHANNEL(chans, i);
		break;
	case RTW_LOCALE_JAPAN:	/* 1-14 */
		name = "Japan";
		ADD_CHANNEL(chans, 14);
		for (i = 1; i <= 14; i++)
			ADD_CHANNEL(chans, i);
		break;
	case RTW_LOCALE_EUROPE:	/* 1-13 */
		name = "Europe";
		for (i = 1; i <= 13; i++)
			ADD_CHANNEL(chans, i);
		break;
	default:			/* 10-11 allowed by most countries */
		name = "<unknown>";
		for (i = 10; i <= 11; i++)
			ADD_CHANNEL(chans, i);
		break;
	}
	printf("%s: Geographic Location %s\n", *dvname, name);
#undef ADD_CHANNEL
}

static __inline void
rtw_identify_country(struct rtw_regs *regs, enum rtw_locale *locale,
    char (*dvname)[IFNAMSIZ])
{
	u_int8_t cfg0 = RTW_READ8(regs, RTW_CONFIG0);

	switch (cfg0 & RTW_CONFIG0_GL_MASK) {
	case RTW_CONFIG0_GL_USA:
		*locale = RTW_LOCALE_USA;
		break;
	case RTW_CONFIG0_GL_JAPAN:
		*locale = RTW_LOCALE_JAPAN;
		break;
	case RTW_CONFIG0_GL_EUROPE:
		*locale = RTW_LOCALE_EUROPE;
		break;
	default:
		*locale = RTW_LOCALE_UNKNOWN;
		break;
	}
}

static __inline int
rtw_identify_sta(struct rtw_regs *regs, u_int8_t (*addr)[IEEE80211_ADDR_LEN],
    char (*dvname)[IFNAMSIZ])
{
	static const u_int8_t empty_macaddr[IEEE80211_ADDR_LEN] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	u_int32_t idr0 = RTW_READ(regs, RTW_IDR0),
	          idr1 = RTW_READ(regs, RTW_IDR1);

	(*addr)[0] = MASK_AND_RSHIFT(idr0, BITS(0,  7));
	(*addr)[1] = MASK_AND_RSHIFT(idr0, BITS(8,  15));
	(*addr)[2] = MASK_AND_RSHIFT(idr0, BITS(16, 23));
	(*addr)[3] = MASK_AND_RSHIFT(idr0, BITS(24 ,31));

	(*addr)[4] = MASK_AND_RSHIFT(idr1, BITS(0,  7));
	(*addr)[5] = MASK_AND_RSHIFT(idr1, BITS(8, 15));

	if (IEEE80211_ADDR_EQ(addr, empty_macaddr)) {
		printf("%s: could not get mac address, attach failed\n",
		    *dvname);
		return ENXIO;
	}

	printf("%s: 802.11 address %s\n", *dvname, ether_sprintf(*addr));

	return 0;
}

static u_int8_t
rtw_chan2txpower(struct rtw_srom *sr, struct ieee80211com *ic,
    struct ieee80211_channel *chan)
{
	u_int idx = RTW_SR_TXPOWER1 + ieee80211_chan2ieee(ic, chan) - 1;
	KASSERT2(idx >= RTW_SR_TXPOWER1 && idx <= RTW_SR_TXPOWER14,
	    ("%s: channel %d out of range", __func__,
	     idx - RTW_SR_TXPOWER1 + 1));
	return RTW_SR_GET(sr, idx);
}

static void
rtw_txdesc_blk_init_all(struct rtw_txdesc_blk (*htcs)[RTW_NTXPRI])
{
	int pri;
	u_int ndesc[RTW_NTXPRI] =
	    {RTW_NTXDESCLO, RTW_NTXDESCMD, RTW_NTXDESCHI, RTW_NTXDESCBCN};

	for (pri = 0; pri < RTW_NTXPRI; pri++) {
		(*htcs)[pri].htc_nfree = ndesc[pri];
		(*htcs)[pri].htc_next = 0;
	}
}

static int
rtw_txctl_blk_init(struct rtw_txctl_blk *stc)
{
	int i;
	struct rtw_txctl *stx;

	SIMPLEQ_INIT(&stc->stc_dirtyq);
	SIMPLEQ_INIT(&stc->stc_freeq);
	for (i = 0; i < stc->stc_ndesc; i++) {
		stx = &stc->stc_desc[i];
		stx->stx_mbuf = NULL;
		SIMPLEQ_INSERT_TAIL(&stc->stc_freeq, stx, stx_q);
	}
	return 0;
}

static void
rtw_txctl_blk_init_all(struct rtw_txctl_blk (*stcs)[RTW_NTXPRI])
{
	int pri;
	for (pri = 0; pri < RTW_NTXPRI; pri++) {
		rtw_txctl_blk_init(&(*stcs)[pri]);
	}
}

static __inline void
rtw_rxdescs_sync(bus_dma_tag_t dmat, bus_dmamap_t dmap, u_int desc0, u_int
    nsync, int ops)
{
	/* sync to end of ring */
	if (desc0 + nsync > RTW_NRXDESC) {
		bus_dmamap_sync(dmat, dmap,
		    offsetof(struct rtw_descs, hd_rx[desc0]),
		    sizeof(struct rtw_rxdesc) * (RTW_NRXDESC - desc0), ops);
		nsync -= (RTW_NRXDESC - desc0);
		desc0 = 0;
	}

	/* sync what remains */
	bus_dmamap_sync(dmat, dmap,
	    offsetof(struct rtw_descs, hd_rx[desc0]),
	    sizeof(struct rtw_rxdesc) * nsync, ops);
}

static void
rtw_txdescs_sync(bus_dma_tag_t dmat, bus_dmamap_t dmap,
    struct rtw_txdesc_blk *htc, u_int desc0, u_int nsync, int ops)
{
	/* sync to end of ring */
	if (desc0 + nsync > htc->htc_ndesc) {
		bus_dmamap_sync(dmat, dmap,
		    htc->htc_ofs + sizeof(struct rtw_txdesc) * desc0,
		    sizeof(struct rtw_txdesc) * (htc->htc_ndesc - desc0),
		    ops);
		nsync -= (htc->htc_ndesc - desc0);
		desc0 = 0;
	}

	/* sync what remains */
	bus_dmamap_sync(dmat, dmap,
	    htc->htc_ofs + sizeof(struct rtw_txdesc) * desc0,
	    sizeof(struct rtw_txdesc) * nsync, ops);
}

static void
rtw_txdescs_sync_all(bus_dma_tag_t dmat, bus_dmamap_t dmap,
    struct rtw_txdesc_blk (*htcs)[RTW_NTXPRI])
{
	int pri;
	for (pri = 0; pri < RTW_NTXPRI; pri++) {
		rtw_txdescs_sync(dmat, dmap,
		    &(*htcs)[pri], 0, (*htcs)[pri].htc_ndesc,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	}
}

static void
rtw_rxbufs_release(bus_dma_tag_t dmat, struct rtw_rxctl *desc)
{
	int i;
	struct rtw_rxctl *srx;

	for (i = 0; i < RTW_NRXDESC; i++) {
		srx = &desc[i];
		bus_dmamap_unload(dmat, srx->srx_dmamap);
		m_freem(srx->srx_mbuf);
		srx->srx_mbuf = NULL;
	}
}

static __inline int
rtw_rxbuf_alloc(bus_dma_tag_t dmat, struct rtw_rxctl *srx)
{
	int rc;
	struct mbuf *m;

	MGETHDR(m, M_DONTWAIT, MT_DATA); 
	if (m == NULL)
		return ENOMEM;

	MCLGET(m, M_DONTWAIT); 
	if (m == NULL)
		return ENOMEM;

	m->m_pkthdr.len = m->m_len = m->m_ext.ext_size;

	rc = bus_dmamap_load_mbuf(dmat, srx->srx_dmamap, m, BUS_DMA_NOWAIT);
	if (rc != 0)
		return rc;

	srx->srx_mbuf = m;

	return 0;
}

static int
rtw_rxctl_init_all(bus_dma_tag_t dmat, struct rtw_rxctl *desc,
    u_int *next, char (*dvname)[IFNAMSIZ])
{
	int i, rc;
	struct rtw_rxctl *srx;

	for (i = 0; i < RTW_NRXDESC; i++) {
		srx = &desc[i];
		if ((rc = rtw_rxbuf_alloc(dmat, srx)) == 0)
			continue;
		printf("%s: failed rtw_rxbuf_alloc after %d buffers, rc = %d\n",
		    *dvname, i, rc);
		if (i == 0) {
			rtw_rxbufs_release(dmat, desc);
			return rc;
		}
	}
	*next = 0;
	return 0;
}

static __inline void
rtw_rxdesc_init(bus_dma_tag_t dmat, bus_dmamap_t dmam,
    struct rtw_rxdesc *hrx, struct rtw_rxctl *srx, int idx)
{
	int is_last = (idx == RTW_NRXDESC - 1);
	uint32_t ctl;

	hrx->hrx_buf = htole32(srx->srx_dmamap->dm_segs[0].ds_addr);

	ctl = LSHIFT(srx->srx_mbuf->m_len, RTW_RXCTL_LENGTH_MASK) |
	    RTW_RXCTL_OWN | RTW_RXCTL_FS | RTW_RXCTL_LS;

	if (is_last)
		ctl |= RTW_RXCTL_EOR;

	hrx->hrx_ctl = htole32(ctl);

	/* sync the mbuf */
	bus_dmamap_sync(dmat, srx->srx_dmamap, 0, srx->srx_dmamap->dm_mapsize,
	    BUS_DMASYNC_PREREAD);

	/* sync the descriptor */
	bus_dmamap_sync(dmat, dmam, RTW_DESC_OFFSET(hd_rx, idx),
	    sizeof(struct rtw_rxdesc),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
}

static void
rtw_rxdesc_init_all(bus_dma_tag_t dmat, bus_dmamap_t dmam,
    struct rtw_rxdesc *desc, struct rtw_rxctl *ctl)
{
	int i;
	struct rtw_rxdesc *hrx;
	struct rtw_rxctl *srx;

	for (i = 0; i < RTW_NRXDESC; i++) {
		hrx = &desc[i];
		srx = &ctl[i];
		rtw_rxdesc_init(dmat, dmam, hrx, srx, i);
	}
}

static void
rtw_io_enable(struct rtw_regs *regs, u_int8_t flags, int enable)
{
	u_int8_t cr;

	RTW_DPRINTF(("%s: %s 0x%02x\n", __func__,
	    enable ? "enable" : "disable", flags));

	cr = RTW_READ8(regs, RTW_CR);

	/* XXX reference source does not enable MULRW */
#if 0
	/* enable PCI Read/Write Multiple */
	cr |= RTW_CR_MULRW;
#endif

	RTW_RBW(regs, RTW_CR, RTW_CR);	/* XXX paranoia? */
	if (enable)
		cr |= flags;
	else
		cr &= ~flags;
	RTW_WRITE8(regs, RTW_CR, cr);
	RTW_SYNC(regs, RTW_CR, RTW_CR);
}

static void
rtw_intr_rx(struct rtw_softc *sc, u_int16_t isr)
{
	u_int next;
	int rate, rssi;
	u_int32_t hrssi, hstat, htsfth, htsftl;
	struct rtw_rxdesc *hrx;
	struct rtw_rxctl *srx;
	struct mbuf *m;

	struct ieee80211_node *ni;
	struct ieee80211_frame *wh;

	for (next = sc->sc_rxnext; ; next = (next + 1) % RTW_RXQLEN) {
		rtw_rxdescs_sync(sc->sc_dmat, sc->sc_desc_dmamap,
		    next, 1, BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		hrx = &sc->sc_rxdesc[next];
		srx = &sc->sc_rxctl[next];

		hstat = le32toh(hrx->hrx_stat);
		hrssi = le32toh(hrx->hrx_rssi);
		htsfth = le32toh(hrx->hrx_tsfth);
		htsftl = le32toh(hrx->hrx_tsftl);

		RTW_DPRINTF2(("%s: rxdesc[%d] hstat %#08x hrssi %#08x "
		    "htsft %#08x%08x\n", __func__, next,
		    hstat, hrssi, htsfth, htsftl));

		if ((hstat & RTW_RXSTAT_OWN) != 0) /* belongs to NIC */
			break;

		if ((hstat & RTW_RXSTAT_IOERROR) != 0) {
			printf("%s: DMA error/FIFO overflow %08x, "
			    "rx descriptor %d\n", sc->sc_dev.dv_xname,
			    hstat & RTW_RXSTAT_IOERROR, next);
			goto next;
		}

		switch (hstat & RTW_RXSTAT_RATE_MASK) {
		case RTW_RXSTAT_RATE_1MBPS:
			rate = 10;
			break;
		case RTW_RXSTAT_RATE_2MBPS:
			rate = 20;
			break;
		case RTW_RXSTAT_RATE_5MBPS:
			rate = 55;
			break;
		default:
#ifdef RTW_DEBUG
			if (rtw_debug > 1)
				printf("%s: interpreting rate #%d as 11 MB/s\n",
				    sc->sc_dev.dv_xname,
				    MASK_AND_RSHIFT(hstat,
				        RTW_RXSTAT_RATE_MASK));
#endif /* RTW_DEBUG */
			/*FALLTHROUGH*/
		case RTW_RXSTAT_RATE_11MBPS:
			rate = 110;
			break;
		}

		RTW_DPRINTF2(("%s: rate %d\n", __func__, rate));

#ifdef RTW_DEBUG
#define PRINTSTAT(flag) do { \
	if ((hstat & flag) != 0) { \
		printf("%s" #flag, delim); \
		delim = ","; \
	} \
} while (0)
		if (rtw_debug > 1) {
			const char *delim = "<";
			printf("%s: ", sc->sc_dev.dv_xname);
			if ((hstat & RTW_RXSTAT_DEBUG) != 0) {
				printf("status %08x<", hstat);
				PRINTSTAT(RTW_RXSTAT_SPLCP);
				PRINTSTAT(RTW_RXSTAT_MAR);
				PRINTSTAT(RTW_RXSTAT_PAR);
				PRINTSTAT(RTW_RXSTAT_BAR);
				PRINTSTAT(RTW_RXSTAT_PWRMGT);
				PRINTSTAT(RTW_RXSTAT_CRC32);
				PRINTSTAT(RTW_RXSTAT_ICV);
				printf(">, ");
			}
			printf("rate %d.%d Mb/s, time %08x%08x\n",
			    rate / 10, rate % 10, htsfth, htsftl);
		}
#endif /* RTW_DEBUG */

		if ((hstat & RTW_RXSTAT_RES) != 0 &&
		    sc->sc_ic.ic_opmode != IEEE80211_M_MONITOR)
			goto next;

		/* if bad flags, skip descriptor */
		if ((hstat & RTW_RXSTAT_ONESEG) != RTW_RXSTAT_ONESEG) {
			printf("%s: too many rx segments\n",
			    sc->sc_dev.dv_xname);
			goto next;
		}

		m = srx->srx_mbuf;

		/* if temporarily out of memory, re-use mbuf */
		if (rtw_rxbuf_alloc(sc->sc_dmat, srx) != 0) {
			printf("%s: rtw_rxbuf_alloc(, %d) failed, "
			    "dropping this packet\n", sc->sc_dev.dv_xname,
			    next);
			goto next;
		}

		if (sc->sc_rfchipid == RTW_RFCHIPID_PHILIPS)
			rssi = MASK_AND_RSHIFT(hrssi, RTW_RXRSSI_RSSI);
		else {
			rssi = MASK_AND_RSHIFT(hrssi, RTW_RXRSSI_IMR_RSSI);
			/* TBD find out each front-end's LNA gain in the
			 * front-end's units
			 */
			if ((hrssi & RTW_RXRSSI_IMR_LNA) == 0)
				rssi |= 0x80;
		}

		m->m_pkthdr.len = m->m_len =
		    MASK_AND_RSHIFT(hstat, RTW_RXSTAT_LENGTH_MASK);
		m->m_flags |= M_HASFCS;

		wh = mtod(m, struct ieee80211_frame *);
		/* TBD use _MAR, _BAR, _PAR flags as hints to _find_rxnode? */
		ni = ieee80211_find_rxnode(&sc->sc_ic, wh);

		sc->sc_tsfth = htsfth;

		ieee80211_input(&sc->sc_if, m, ni, rssi, htsftl);
		ieee80211_release_node(&sc->sc_ic, ni);
next:
		rtw_rxdesc_init(sc->sc_dmat, sc->sc_desc_dmamap,
		    hrx, srx, next);
	}
	sc->sc_rxnext = next;
	return;
}

static void
rtw_intr_tx(struct rtw_softc *sc, u_int16_t isr)
{
	/* TBD */
	return;
}

static void
rtw_intr_beacon(struct rtw_softc *sc, u_int16_t isr)
{
	/* TBD */
	return;
}

static void
rtw_intr_atim(struct rtw_softc *sc)
{
	/* TBD */
	return;
}

static void
rtw_intr_ioerror(struct rtw_softc *sc, u_int16_t isr)
{
	if ((isr & (RTW_INTR_RDU|RTW_INTR_RXFOVW)) != 0) {
#if 0
		rtw_rxctl_init_all(sc->sc_dmat, sc->sc_rxctl, &sc->sc_rxnext,
		    &sc->sc_dev.dv_xname);
		rtw_rxdesc_init_all(sc->sc_dmat, sc->sc_desc_dmamap,
		    sc->sc_rxdesc, sc->sc_rxctl);
#endif
		rtw_io_enable(&sc->sc_regs, RTW_CR_RE, 1);
	}
	if ((isr & RTW_INTR_TXFOVW) != 0)
		;	/* TBD restart transmit engine */
	return;
}

static __inline void
rtw_suspend_ticks(struct rtw_softc *sc)
{
	printf("%s: suspending ticks\n", sc->sc_dev.dv_xname);
	sc->sc_do_tick = 0;
}

static __inline void
rtw_resume_ticks(struct rtw_softc *sc)
{
	int s;
	struct timeval tv;
	u_int32_t tsftrl0, tsftrl1, next_tick;

	tsftrl0 = RTW_READ(&sc->sc_regs, RTW_TSFTRL);

	s = splclock();
	timersub(&mono_time, &sc->sc_tick0, &tv);
	splx(s);

	tsftrl1 = RTW_READ(&sc->sc_regs, RTW_TSFTRL);
	next_tick = tsftrl1 + 1000000 - tv.tv_usec;
	RTW_WRITE(&sc->sc_regs, RTW_TINT, next_tick);

	sc->sc_do_tick = 1;

	printf("%s: resume ticks delta %#08x now %#08x next %#08x\n",
	    sc->sc_dev.dv_xname, tsftrl1 - tsftrl0, tsftrl1, next_tick);
}

static void
rtw_intr_timeout(struct rtw_softc *sc)
{
	printf("%s: timeout\n", sc->sc_dev.dv_xname);
	if (sc->sc_do_tick)
		rtw_resume_ticks(sc);
	return;
}

int
rtw_intr(void *arg)
{
	struct rtw_softc *sc = arg;
	struct rtw_regs *regs = &sc->sc_regs;
	u_int16_t isr;

#ifdef RTW_DEBUG
	if ((sc->sc_flags & RTW_F_ENABLED) == 0)
		panic("%s: rtw_intr: not enabled", sc->sc_dev.dv_xname);
#endif /* RTW_DEBUG */

	/*
	 * If the interface isn't running, the interrupt couldn't
	 * possibly have come from us.
	 */
	if ((sc->sc_if.if_flags & IFF_RUNNING) == 0 ||
	    (sc->sc_dev.dv_flags & DVF_ACTIVE) == 0) {
		RTW_DPRINTF2(("%s: stray interrupt\n", sc->sc_dev.dv_xname));
		return (0);
	}

	for (;;) {
		isr = RTW_READ16(regs, RTW_ISR);

		RTW_WRITE16(regs, RTW_ISR, isr);

		if (sc->sc_intr_ack != NULL)
			(*sc->sc_intr_ack)(regs);

		if (isr == 0)
			break;

#ifdef RTW_DEBUG
#define PRINTINTR(flag) do { \
	if ((isr & flag) != 0) { \
		printf("%s" #flag, delim); \
		delim = ","; \
	} \
} while (0)

		if (rtw_debug > 1 && isr != 0) {
			const char *delim = "<";

			printf("%s: reg[ISR] = %x", sc->sc_dev.dv_xname, isr);

			PRINTINTR(RTW_INTR_TXFOVW);
			PRINTINTR(RTW_INTR_TIMEOUT);
			PRINTINTR(RTW_INTR_BCNINT);
			PRINTINTR(RTW_INTR_ATIMINT);
			PRINTINTR(RTW_INTR_TBDER);
			PRINTINTR(RTW_INTR_TBDOK);
			PRINTINTR(RTW_INTR_THPDER);
			PRINTINTR(RTW_INTR_THPDOK);
			PRINTINTR(RTW_INTR_TNPDER);
			PRINTINTR(RTW_INTR_TNPDOK);
			PRINTINTR(RTW_INTR_RXFOVW);
			PRINTINTR(RTW_INTR_RDU);
			PRINTINTR(RTW_INTR_TLPDER);
			PRINTINTR(RTW_INTR_TLPDOK);
			PRINTINTR(RTW_INTR_RER);
			PRINTINTR(RTW_INTR_ROK);

			printf(">\n");
		}
#undef PRINTINTR
#endif /* RTW_DEBUG */

		if ((isr & RTW_INTR_RX) != 0)
			rtw_intr_rx(sc, isr & RTW_INTR_RX);
		if ((isr & RTW_INTR_TX) != 0)
			rtw_intr_tx(sc, isr & RTW_INTR_TX);
		if ((isr & RTW_INTR_BEACON) != 0)
			rtw_intr_beacon(sc, isr & RTW_INTR_BEACON);
		if ((isr & RTW_INTR_ATIMINT) != 0)
			rtw_intr_atim(sc);
		if ((isr & RTW_INTR_IOERROR) != 0)
			rtw_intr_ioerror(sc, isr & RTW_INTR_IOERROR);
		if ((isr & RTW_INTR_TIMEOUT) != 0)
			rtw_intr_timeout(sc);
	}

	return 1;
}

static void
rtw_stop(struct ifnet *ifp, int disable)
{
	struct rtw_softc *sc = (struct rtw_softc *)ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct rtw_regs *regs = &sc->sc_regs;

	rtw_suspend_ticks(sc);

	ieee80211_new_state(ic, IEEE80211_S_INIT, -1);

	/* Disable interrupts. */
	RTW_WRITE16(regs, RTW_IMR, 0);

	/* Stop the transmit and receive processes. First stop DMA,
	 * then disable receiver and transmitter.
	 */
	RTW_WRITE8(regs, RTW_TPPOLL,
	    RTW_TPPOLL_SBQ|RTW_TPPOLL_SHPQ|RTW_TPPOLL_SNPQ|RTW_TPPOLL_SLPQ);

	rtw_io_enable(&sc->sc_regs, RTW_CR_RE|RTW_CR_TE, 0);

	/* TBD Release transmit buffers. */

	if (disable) {
		rtw_disable(sc);
		rtw_rxbufs_release(sc->sc_dmat, &sc->sc_rxctl[0]);
	}

	/* Mark the interface as not running.  Cancel the watchdog timer. */
	ifp->if_flags &= ~IFF_RUNNING;
	ifp->if_timer = 0;
	return;
}

const char *
rtw_pwrstate_string(enum rtw_pwrstate power)
{
	switch (power) {
	case RTW_ON:
		return "on";
	case RTW_SLEEP:
		return "sleep";
	case RTW_OFF:
		return "off";
	default:
		return "unknown";
	}
}

/* XXX I am using the RFMD settings gleaned from the reference
 * driver.
 */
static void
rtw_maxim_pwrstate(struct rtw_regs *regs, enum rtw_pwrstate power,
    int before_rf)
{
	u_int32_t anaparm;

	RTW_DPRINTF(("%s: power state %s, %s RF\n", __func__,
	    rtw_pwrstate_string(power), (before_rf) ? "before" : "after"));

	anaparm = RTW_READ(regs, RTW_ANAPARM);
	anaparm &= ~(RTW_ANAPARM_RFPOW0_MASK|RTW_ANAPARM_RFPOW1_MASK);
	anaparm &= ~RTW_ANAPARM_TXDACOFF;

	switch (power) {
	case RTW_OFF:
		if (before_rf)
			return;
		anaparm |= RTW_ANAPARM_RFPOW0_RFMD_OFF;
		anaparm |= RTW_ANAPARM_RFPOW1_RFMD_OFF;
		anaparm |= RTW_ANAPARM_TXDACOFF;
		break;
	case RTW_SLEEP:
		if (!before_rf)
			return;
		anaparm |= RTW_ANAPARM_RFPOW0_RFMD_SLEEP;
		anaparm |= RTW_ANAPARM_RFPOW1_RFMD_SLEEP;
		anaparm |= RTW_ANAPARM_TXDACOFF;
		break;
	case RTW_ON:
		if (!before_rf)
			return;
		anaparm |= RTW_ANAPARM_RFPOW0_RFMD_ON;
		anaparm |= RTW_ANAPARM_RFPOW1_RFMD_ON;
		break;
	}
	RTW_WRITE(regs, RTW_ANAPARM, anaparm);
	RTW_SYNC(regs, RTW_ANAPARM, RTW_ANAPARM);
}

static void
rtw_philips_pwrstate(struct rtw_regs *regs, enum rtw_pwrstate power,
    int before_rf)
{
	u_int32_t anaparm;

	RTW_DPRINTF(("%s: power state %s, %s RF\n", __func__,
	    rtw_pwrstate_string(power), (before_rf) ? "before" : "after"));

	anaparm = RTW_READ(regs, RTW_ANAPARM);
	anaparm &= ~(RTW_ANAPARM_RFPOW0_MASK|RTW_ANAPARM_RFPOW1_MASK);
	anaparm &= ~RTW_ANAPARM_TXDACOFF;

	switch (power) {
	case RTW_OFF:
		if (before_rf)
			return;
		anaparm |= RTW_ANAPARM_RFPOW0_PHILIPS_OFF;
		anaparm |= RTW_ANAPARM_RFPOW1_PHILIPS_OFF;
		anaparm |= RTW_ANAPARM_TXDACOFF;
		break;
	case RTW_SLEEP:
		if (!before_rf)
			return;
		anaparm |= RTW_ANAPARM_RFPOW0_PHILIPS_SLEEP;
		anaparm |= RTW_ANAPARM_RFPOW1_PHILIPS_SLEEP;
		anaparm |= RTW_ANAPARM_TXDACOFF;
		break;
	case RTW_ON:
		if (!before_rf)
			return;
		anaparm |= RTW_ANAPARM_RFPOW0_PHILIPS_ON;
		anaparm |= RTW_ANAPARM_RFPOW1_PHILIPS_ON;
		break;
	}
	RTW_WRITE(regs, RTW_ANAPARM, anaparm);
	RTW_SYNC(regs, RTW_ANAPARM, RTW_ANAPARM);
}

static void
rtw_pwrstate0(struct rtw_softc *sc, enum rtw_pwrstate power, int before_rf)
{
	struct rtw_regs *regs = &sc->sc_regs;

	rtw_config0123_enable(regs, 1);
	rtw_anaparm_enable(regs, 1);

	(*sc->sc_pwrstate_cb)(regs, power, before_rf);

	rtw_anaparm_enable(regs, 0);
	rtw_config0123_enable(regs, 0);

	return;
}

static int
rtw_pwrstate(struct rtw_softc *sc, enum rtw_pwrstate power)
{
	int rc;

	RTW_DPRINTF2(("%s: %s->%s\n", __func__,
	    rtw_pwrstate_string(sc->sc_pwrstate), rtw_pwrstate_string(power)));

	if (sc->sc_pwrstate == power)
		return 0;

	rtw_pwrstate0(sc, power, 1);
	rc = rtw_rf_pwrstate(sc->sc_rf, power);
	rtw_pwrstate0(sc, power, 0);

	switch (power) {
	case RTW_ON:
		/* TBD */
		break;
	case RTW_SLEEP:
		/* TBD */
		break;
	case RTW_OFF:
		/* TBD */
		break;
	}
	if (rc == 0)
		sc->sc_pwrstate = power;
	else
		sc->sc_pwrstate = RTW_OFF;
	return rc;
}

static int
rtw_tune(struct rtw_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	u_int chan;
	int rc;
	int antdiv = sc->sc_flags & RTW_F_ANTDIV,
	    dflantb = sc->sc_flags & RTW_F_DFLANTB;

	KASSERT(ic->ic_bss->ni_chan != NULL);

	chan = ieee80211_chan2ieee(ic, ic->ic_bss->ni_chan);
	if (chan == IEEE80211_CHAN_ANY)
		panic("%s: chan == IEEE80211_CHAN_ANY\n", __func__);

	if (chan == sc->sc_cur_chan) {
		RTW_DPRINTF(("%s: already tuned chan #%d\n", __func__, chan)); 
		return 0;
	}

	rtw_suspend_ticks(sc);

	rtw_io_enable(&sc->sc_regs, RTW_CR_RE | RTW_CR_TE, 0);

	/* TBD wait for Tx to complete */

	KASSERT((sc->sc_flags & RTW_F_ENABLED) != 0);

	if ((rc = rtw_phy_init(&sc->sc_regs, sc->sc_rf,
	    rtw_chan2txpower(&sc->sc_srom, ic, ic->ic_bss->ni_chan),
	    sc->sc_csthr, ic->ic_bss->ni_chan->ic_freq, antdiv,
	    dflantb, RTW_ON)) != 0) {
		/* XXX condition on powersaving */
		printf("%s: phy init failed\n", sc->sc_dev.dv_xname);
	}

	sc->sc_cur_chan = chan;

	rtw_io_enable(&sc->sc_regs, RTW_CR_RE | RTW_CR_TE, 1);

	rtw_resume_ticks(sc);

	return rc;
}

void
rtw_disable(struct rtw_softc *sc)
{
	int rc;

	if ((sc->sc_flags & RTW_F_ENABLED) == 0)
		return;

	/* turn off PHY */
	if ((rc = rtw_pwrstate(sc, RTW_OFF)) != 0)
		printf("%s: failed to turn off PHY (%d)\n",
		    sc->sc_dev.dv_xname, rc);

	if (sc->sc_disable != NULL)
		(*sc->sc_disable)(sc);

	sc->sc_flags &= ~RTW_F_ENABLED;
}

int
rtw_enable(struct rtw_softc *sc)
{
	if ((sc->sc_flags & RTW_F_ENABLED) == 0) {
		if (sc->sc_enable != NULL && (*sc->sc_enable)(sc) != 0) {
			printf("%s: device enable failed\n",
			    sc->sc_dev.dv_xname);
			return (EIO);
		}
		sc->sc_flags |= RTW_F_ENABLED;
	}
	return (0);
}

static void
rtw_transmit_config(struct rtw_regs *regs)
{
	u_int32_t tcr;

	tcr = RTW_READ(regs, RTW_TCR);

	tcr |= RTW_TCR_SAT;		/* send ACK as fast as possible */
	tcr &= ~RTW_TCR_LBK_MASK;
	tcr |= RTW_TCR_LBK_NORMAL;	/* normal operating mode */

	/* set short/long retry limits */
	tcr &= ~(RTW_TCR_SRL_MASK|RTW_TCR_LRL_MASK);
	tcr |= LSHIFT(7, RTW_TCR_SRL_MASK) | LSHIFT(7, RTW_TCR_LRL_MASK);

	tcr |= RTW_TCR_CRC;	/* NIC appends CRC32 */

	RTW_WRITE(regs, RTW_TCR, tcr);
}

static __inline void
rtw_enable_interrupts(struct rtw_softc *sc)
{
	struct rtw_regs *regs = &sc->sc_regs;

	sc->sc_inten = RTW_INTR_RX|RTW_INTR_TX|RTW_INTR_BEACON|RTW_INTR_ATIMINT;
	sc->sc_inten |= RTW_INTR_IOERROR|RTW_INTR_TIMEOUT;

	RTW_WRITE16(regs, RTW_IMR, sc->sc_inten);
	RTW_WRITE16(regs, RTW_ISR, 0xffff);

	/* XXX necessary? */
	if (sc->sc_intr_ack != NULL)
		(*sc->sc_intr_ack)(regs);
}

/* XXX is the endianness correct? test. */
#define	rtw_calchash(addr) \
	(ether_crc32_le((addr), IEEE80211_ADDR_LEN) & BITS(5, 0))

static void
rtw_pktfilt_load(struct rtw_softc *sc)
{
	struct rtw_regs *regs = &sc->sc_regs;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ethercom *ec = &ic->ic_ec;
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	int hash;
	u_int32_t hashes[2] = { 0, 0 };
	struct ether_multi *enm;
	struct ether_multistep step;

	/* XXX might be necessary to stop Rx/Tx engines while setting filters */

#define RTW_RCR_MONITOR (RTW_RCR_ACRC32|RTW_RCR_APM|RTW_RCR_AAP|RTW_RCR_AB)

	if (ic->ic_opmode == IEEE80211_M_MONITOR)
		sc->sc_rcr |= RTW_RCR_MONITOR;
	else
		sc->sc_rcr &= ~RTW_RCR_MONITOR;

	/* XXX reference sources BEGIN */
	sc->sc_rcr |= RTW_RCR_ENMARP | RTW_RCR_AICV | RTW_RCR_ACRC32;
	sc->sc_rcr |= RTW_RCR_AB | RTW_RCR_AM | RTW_RCR_APM;
#if 0
	/* receive broadcasts in our BSS */
	sc->sc_rcr |= RTW_RCR_ADD3;
#endif
	/* XXX reference sources END */

	/* receive pwrmgmt frames. */
	sc->sc_rcr |= RTW_RCR_APWRMGT;
	/* receive mgmt/ctrl/data frames. */
	sc->sc_rcr |= RTW_RCR_AMF | RTW_RCR_ACF | RTW_RCR_ADF;
	/* initialize Rx DMA threshold, Tx DMA burst size */
	sc->sc_rcr |= RTW_RCR_RXFTH_WHOLE | RTW_RCR_MXDMA_1024;

	ifp->if_flags &= ~IFF_ALLMULTI;

	if (ifp->if_flags & IFF_PROMISC) {
		sc->sc_rcr |= RTW_RCR_AB;	/* accept all broadcast */
allmulti:
		ifp->if_flags |= IFF_ALLMULTI;
		goto setit;
	}

	/*
	 * Program the 64-bit multicast hash filter.
	 */
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		/* XXX */
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
		    ETHER_ADDR_LEN) != 0)
			goto allmulti;

		hash = rtw_calchash(enm->enm_addrlo);
		hashes[hash >> 5] |= 1 << (hash & 0x1f);
		ETHER_NEXT_MULTI(step, enm);
	}

	if (ifp->if_flags & IFF_BROADCAST) {
		hash = rtw_calchash(etherbroadcastaddr);
		hashes[hash >> 5] |= 1 << (hash & 0x1f);
	}

	/* all bits set => hash is useless */
	if (~(hashes[0] & hashes[1]) == 0)
		goto allmulti;

 setit:
	if (ifp->if_flags & IFF_ALLMULTI)
		sc->sc_rcr |= RTW_RCR_AM;	/* accept all multicast */

	if (ic->ic_state == IEEE80211_S_SCAN)
		sc->sc_rcr |= RTW_RCR_AB;	/* accept all broadcast */

	hashes[0] = hashes[1] = 0xffffffff;

	RTW_WRITE(regs, RTW_MAR0, hashes[0]);
	RTW_WRITE(regs, RTW_MAR1, hashes[1]);
	RTW_WRITE(regs, RTW_RCR, sc->sc_rcr);
	RTW_SYNC(regs, RTW_MAR0, RTW_RCR); /* RTW_MAR0 < RTW_MAR1 < RTW_RCR */

	DPRINTF(sc, ("%s: RTW_MAR0 %08x RTW_MAR1 %08x RTW_RCR %08x\n",
	    sc->sc_dev.dv_xname, RTW_READ(regs, RTW_MAR0),
	    RTW_READ(regs, RTW_MAR1), RTW_READ(regs, RTW_RCR)));

	return;
}

static int
rtw_init(struct ifnet *ifp)
{
	struct rtw_softc *sc = (struct rtw_softc *)ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct rtw_regs *regs = &sc->sc_regs;
	int rc = 0, s;

	if ((rc = rtw_enable(sc)) != 0)
		goto out;

	/* Cancel pending I/O and reset. */
	rtw_stop(ifp, 0);

	ic->ic_bss->ni_chan = ic->ic_ibss_chan;
	DPRINTF(sc, ("%s: channel %d freq %d flags 0x%04x\n",
	    __func__, ieee80211_chan2ieee(ic, ic->ic_bss->ni_chan),
	    ic->ic_bss->ni_chan->ic_freq, ic->ic_bss->ni_chan->ic_flags));

	if ((rc = rtw_pwrstate(sc, RTW_OFF)) != 0)
		goto out;

	rtw_txdesc_blk_init_all(&sc->sc_txdesc_blk);

	rtw_txctl_blk_init_all(&sc->sc_txctl_blk);

	rtw_rxctl_init_all(sc->sc_dmat, sc->sc_rxctl, &sc->sc_rxnext,
	    &sc->sc_dev.dv_xname);
	rtw_rxdesc_init_all(sc->sc_dmat, sc->sc_desc_dmamap,
	    sc->sc_rxdesc, sc->sc_rxctl);

	rtw_txdescs_sync_all(sc->sc_dmat, sc->sc_desc_dmamap,
	    &sc->sc_txdesc_blk);
#if 0	/* redundant with rtw_rxdesc_init_all */
	rtw_rxdescs_sync(sc->sc_dmat, sc->sc_desc_dmamap,
	    0, RTW_NRXDESC, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
#endif

	RTW_WRITE(regs, RTW_RDSAR, RTW_RING_BASE(sc, hd_rx));

	rtw_transmit_config(regs);

	rtw_config0123_enable(regs, 1);

	RTW_WRITE(regs, RTW_MSR, 0x0);	/* no link */

	RTW_WRITE(regs, RTW_BRSR, 0x0);	/* long PLCP header, 1Mbps basic rate */

	rtw_anaparm_enable(regs, 0);

	rtw_config0123_enable(regs, 0);

#if 0
	RTW_WRITE(regs, RTW_FEMR, RTW_FEMR_GWAKE|RTW_FEMR_WKUP|RTW_FEMR_INTR);
#endif
	/* XXX from reference sources */
	RTW_WRITE(regs, RTW_FEMR, 0xffff);

	RTW_WRITE(regs, RTW_PHYDELAY, sc->sc_phydelay);
	/* from Linux driver */
	RTW_WRITE(regs, RTW_CRCOUNT, RTW_CRCOUNT_MAGIC);

	rtw_enable_interrupts(sc);

	rtw_pktfilt_load(sc);

	RTW_WRITE(regs, RTW_TLPDA, RTW_RING_BASE(sc, hd_txlo));
	RTW_WRITE(regs, RTW_TNPDA, RTW_RING_BASE(sc, hd_txmd));
	RTW_WRITE(regs, RTW_THPDA, RTW_RING_BASE(sc, hd_txhi));
	RTW_WRITE(regs, RTW_TBDA, RTW_RING_BASE(sc, hd_bcn));

	rtw_io_enable(regs, RTW_CR_RE|RTW_CR_TE, 1);

	ifp->if_flags |= IFF_RUNNING;
	ic->ic_state = IEEE80211_S_INIT;

	RTW_WRITE16(regs, RTW_BSSID16, 0x0);
	RTW_WRITE(regs, RTW_BSSID32, 0x0);

	s = splclock();
	sc->sc_tick0 = mono_time;
	splx(s);

	rtw_resume_ticks(sc);

	switch (ic->ic_opmode) {
	case IEEE80211_M_AHDEMO:
	case IEEE80211_M_IBSS:
		RTW_WRITE8(regs, RTW_MSR, RTW_MSR_NETYPE_ADHOC_OK);
		break;
	case IEEE80211_M_HOSTAP:
		RTW_WRITE8(regs, RTW_MSR, RTW_MSR_NETYPE_AP_OK);
	case IEEE80211_M_MONITOR:
		/* XXX */
		RTW_WRITE8(regs, RTW_MSR, RTW_MSR_NETYPE_NOLINK);
		return ieee80211_new_state(ic, IEEE80211_S_RUN, -1);
	case IEEE80211_M_STA:
		RTW_WRITE8(regs, RTW_MSR, RTW_MSR_NETYPE_INFRA_OK);
		break;
	}
	return ieee80211_new_state(ic, IEEE80211_S_SCAN, -1);
out:
	return rc;
}

static int
rtw_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	int rc = 0;
	struct rtw_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;

	switch (cmd) {
	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) != 0) {
			if ((sc->sc_flags & RTW_F_ENABLED) != 0) {
				rtw_pktfilt_load(sc);
			} else
				rc = rtw_init(ifp);
#ifdef RTW_DEBUG
			rtw_print_regs(&sc->sc_regs, ifp->if_xname, __func__);
#endif /* RTW_DEBUG */
		} else if ((sc->sc_flags & RTW_F_ENABLED) != 0) {
#ifdef RTW_DEBUG
			rtw_print_regs(&sc->sc_regs, ifp->if_xname, __func__);
#endif /* RTW_DEBUG */
			rtw_stop(ifp, 1);
		}
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if (cmd == SIOCADDMULTI)
			rc = ether_addmulti(ifr, &sc->sc_ic.ic_ec);
		else
			rc = ether_delmulti(ifr, &sc->sc_ic.ic_ec);
		if (rc == ENETRESET) {
			if ((sc->sc_flags & RTW_F_ENABLED) != 0)
				rtw_pktfilt_load(sc);
			rc = 0;
		}
		break;
	default:
		if ((rc = ieee80211_ioctl(ifp, cmd, data)) == ENETRESET) {
			if ((sc->sc_flags & RTW_F_ENABLED) != 0)
				rc = rtw_init(ifp);
			else
				rc = 0;
		}
		break;
	}
	return rc;
}

/* Point *mp at the next 802.11 frame to transmit.  Point *stcp
 * at the driver's selection of transmit control block for the packet.
 */
static __inline int
rtw_dequeue(struct ifnet *ifp, struct rtw_txctl_blk **stcp, struct mbuf **mp,
    struct ieee80211_node **nip)
{
	struct mbuf *m0;
	struct rtw_softc *sc;
	struct ieee80211com *ic;

	sc = (struct rtw_softc *)ifp->if_softc;
	ic = &sc->sc_ic;

	*mp = NULL;

	if (!IF_IS_EMPTY(&ic->ic_mgtq)) {
		IF_DEQUEUE(&ic->ic_mgtq, m0);
		*nip = (struct ieee80211_node *)m0->m_pkthdr.rcvif;
		m0->m_pkthdr.rcvif = NULL;
	} else if (ic->ic_state != IEEE80211_S_RUN)
		return 0;
	else if (!IF_IS_EMPTY(&ic->ic_pwrsaveq)) {
		IF_DEQUEUE(&ic->ic_pwrsaveq, m0);
		*nip = (struct ieee80211_node *)m0->m_pkthdr.rcvif;
		m0->m_pkthdr.rcvif = NULL;
	} else {
		IFQ_POLL(&ifp->if_snd, m0);
		if (m0 == NULL)
			return 0;
		IFQ_DEQUEUE(&ifp->if_snd, m0);
		ifp->if_opackets++;
#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m0);
#endif
		if ((m0 = ieee80211_encap(ifp, m0, nip)) == NULL) {
			ifp->if_oerrors++;
			return -1;
		}
	}
	*stcp = &sc->sc_txctl_blk[RTW_TXPRIMD];
	*mp = m0;
	return 0;
}

static void
rtw_start(struct ifnet *ifp)
{
	struct mbuf *m0;
	struct rtw_softc *sc;
	struct rtw_txctl_blk *stc;
	struct ieee80211_node *ni;

	sc = (struct rtw_softc *)ifp->if_softc;

#if 0
	struct ifqueue		ic_mgtq;
	struct ifqueue		ic_pwrsaveq;
struct rtw_txctl_blk {
	/* dirty/free s/w descriptors */
	struct rtw_txq		stc_dirtyq;
	struct rtw_txq		stc_freeq;
	u_int			stc_ndesc;
	struct rtw_txctl	*stc_desc;
};
#endif
	while (!SIMPLEQ_EMPTY(&stc->stc_freeq)) {
		if (rtw_dequeue(ifp, &stc, &m0, &ni) == -1)
			continue;
		if (m0 == NULL)
			break;
		ieee80211_release_node(&sc->sc_ic, ni);
	}
	return;
}

static void
rtw_watchdog(struct ifnet *ifp)
{
	/* TBD */
	return;
}

static void
rtw_start_beacon(struct rtw_softc *sc, int enable)
{
	/* TBD */
	return;
}

static void
rtw_next_scan(void *arg)
{
	struct ieee80211com *ic = arg;
	int s;

	/* don't call rtw_start w/o network interrupts blocked */
	s = splnet();
	if (ic->ic_state == IEEE80211_S_SCAN)
		ieee80211_next_scan(ic);
	splx(s);
}

/* Synchronize the hardware state with the software state. */
static int
rtw_newstate(struct ieee80211com *ic, enum ieee80211_state nstate, int arg)
{
	struct ifnet *ifp = &ic->ic_if;
	struct rtw_softc *sc = ifp->if_softc;
	enum ieee80211_state ostate;
	int error;

	ostate = ic->ic_state;

	if (nstate == IEEE80211_S_INIT) {
		callout_stop(&sc->sc_scan_ch);
		sc->sc_cur_chan = IEEE80211_CHAN_ANY;
		rtw_start_beacon(sc, 0);
		return (*sc->sc_mtbl.mt_newstate)(ic, nstate, arg);
	}

	if (ostate == IEEE80211_S_INIT && nstate != IEEE80211_S_INIT)
		rtw_pwrstate(sc, RTW_ON);

	if ((error = rtw_tune(sc)) != 0)
		return error;

	switch (nstate) {
	case IEEE80211_S_ASSOC:
		break;
	case IEEE80211_S_INIT:
		panic("%s: unexpected state IEEE80211_S_INIT\n", __func__);
		break;
	case IEEE80211_S_SCAN:
#if 0
		memset(sc->sc_bssid, 0, IEEE80211_ADDR_LEN);
		rtw_write_bssid(sc);
#endif

		callout_reset(&sc->sc_scan_ch, rtw_dwelltime * hz / 1000,
		    rtw_next_scan, ic);

		break;
	case IEEE80211_S_RUN:
		if (ic->ic_opmode == IEEE80211_M_STA)
			break;
		/*FALLTHROUGH*/
	case IEEE80211_S_AUTH:
#if 0
		rtw_write_bssid(sc);
		rtw_write_bcn_thresh(sc);
		rtw_write_ssid(sc);
		rtw_write_sup_rates(sc);
#endif
		if (ic->ic_opmode == IEEE80211_M_AHDEMO ||
		    ic->ic_opmode == IEEE80211_M_MONITOR)
			break;

		/* TBD set listen interval, beacon interval */

#if 0
		rtw_tsf(sc);
#endif
		break;
	}

	if (nstate != IEEE80211_S_SCAN)
		callout_stop(&sc->sc_scan_ch);

	if (nstate == IEEE80211_S_RUN &&
	    (ic->ic_opmode == IEEE80211_M_HOSTAP ||
	     ic->ic_opmode == IEEE80211_M_IBSS))
		rtw_start_beacon(sc, 1);
	else
		rtw_start_beacon(sc, 0);

	return (*sc->sc_mtbl.mt_newstate)(ic, nstate, arg);
}

static void
rtw_recv_beacon(struct ieee80211com *ic, struct mbuf *m0,
    struct ieee80211_node *ni, int subtype, int rssi, u_int32_t rstamp)
{
	/* TBD */
	return;
}

static void
rtw_recv_mgmt(struct ieee80211com *ic, struct mbuf *m,
    struct ieee80211_node *ni, int subtype, int rssi, u_int32_t rstamp)
{
	struct rtw_softc *sc = (struct rtw_softc*)ic->ic_softc;

	switch (subtype) {
	case IEEE80211_FC0_SUBTYPE_PROBE_REQ:
		/* do nothing: hardware answers probe request XXX */
		break;
	case IEEE80211_FC0_SUBTYPE_PROBE_RESP:
	case IEEE80211_FC0_SUBTYPE_BEACON:
		rtw_recv_beacon(ic, m, ni, subtype, rssi, rstamp);
		break;
	default:
		(*sc->sc_mtbl.mt_recv_mgmt)(ic, m, ni, subtype, rssi, rstamp);
		break;
	}
	return;
}

static struct ieee80211_node *
rtw_node_alloc(struct ieee80211com *ic)
{
	struct rtw_softc *sc = (struct rtw_softc *)ic->ic_if.if_softc;
	struct ieee80211_node *ni = (*sc->sc_mtbl.mt_node_alloc)(ic);

	DPRINTF(sc, ("%s: alloc node %p\n", sc->sc_dev.dv_xname, ni));
	return ni;
}

static void
rtw_node_free(struct ieee80211com *ic, struct ieee80211_node *ni)
{
	struct rtw_softc *sc = (struct rtw_softc *)ic->ic_if.if_softc;

	DPRINTF(sc, ("%s: freeing node %p %s\n", sc->sc_dev.dv_xname, ni,
	    ether_sprintf(ni->ni_bssid)));
	(*sc->sc_mtbl.mt_node_free)(ic, ni);
}

static int
rtw_media_change(struct ifnet *ifp)
{
	int error;

	error = ieee80211_media_change(ifp);
	if (error == ENETRESET) {
		if ((ifp->if_flags & (IFF_RUNNING|IFF_UP)) ==
		    (IFF_RUNNING|IFF_UP))
			rtw_init(ifp);		/* XXX lose error */
		error = 0;
	}
	return error;
}

static void
rtw_media_status(struct ifnet *ifp, struct ifmediareq *imr)
{
	struct rtw_softc *sc = ifp->if_softc;

	if ((sc->sc_flags & RTW_F_ENABLED) == 0) {
		imr->ifm_active = IFM_IEEE80211 | IFM_NONE;
		imr->ifm_status = 0;
		return;
	}
	ieee80211_media_status(ifp, imr);
}

void
rtw_power(int why, void *arg)
{
	struct rtw_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	int s;

	DPRINTF(sc, ("%s: rtw_power(%d,)\n", sc->sc_dev.dv_xname, why));

	s = splnet();
	switch (why) {
	case PWR_STANDBY:
		/* XXX do nothing. */
		break;
	case PWR_SUSPEND:
		rtw_stop(ifp, 0);
		if (sc->sc_power != NULL)
			(*sc->sc_power)(sc, why);
		break;
	case PWR_RESUME:
		if (ifp->if_flags & IFF_UP) {
			if (sc->sc_power != NULL)
				(*sc->sc_power)(sc, why);
			rtw_init(ifp);
		}
		break;
	case PWR_SOFTSUSPEND:
	case PWR_SOFTSTANDBY:
	case PWR_SOFTRESUME:
		break;
	}
	splx(s);
}

/* rtw_shutdown: make sure the interface is stopped at reboot time. */
void
rtw_shutdown(void *arg)
{
	struct rtw_softc *sc = arg;

	rtw_stop(&sc->sc_ic.ic_if, 1);
}

static __inline void
rtw_setifprops(struct ifnet *ifp, char (*dvname)[IFNAMSIZ], void *softc)
{
	(void)memcpy(ifp->if_xname, *dvname, IFNAMSIZ);
	ifp->if_softc = softc;
	ifp->if_flags = IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST |
	    IFF_NOTRAILERS;
	ifp->if_ioctl = rtw_ioctl;
	ifp->if_start = rtw_start;
	ifp->if_watchdog = rtw_watchdog;
	ifp->if_init = rtw_init;
	ifp->if_stop = rtw_stop;
}

static __inline void
rtw_set80211props(struct ieee80211com *ic)
{
	int nrate;
	ic->ic_phytype = IEEE80211_T_DS;
	ic->ic_opmode = IEEE80211_M_STA;
	ic->ic_caps = IEEE80211_C_PMGT | IEEE80211_C_IBSS |
	    IEEE80211_C_HOSTAP | IEEE80211_C_MONITOR | IEEE80211_C_WEP;

	nrate = 0;
	ic->ic_sup_rates[IEEE80211_MODE_11B].rs_rates[nrate++] = 2;
	ic->ic_sup_rates[IEEE80211_MODE_11B].rs_rates[nrate++] = 4;
	ic->ic_sup_rates[IEEE80211_MODE_11B].rs_rates[nrate++] = 11;
	ic->ic_sup_rates[IEEE80211_MODE_11B].rs_rates[nrate++] = 22;
	ic->ic_sup_rates[IEEE80211_MODE_11B].rs_nrates = nrate;
}

static __inline void
rtw_set80211methods(struct rtw_mtbl *mtbl, struct ieee80211com *ic)
{
	mtbl->mt_newstate = ic->ic_newstate;
	ic->ic_newstate = rtw_newstate;

	mtbl->mt_recv_mgmt = ic->ic_recv_mgmt;
	ic->ic_recv_mgmt = rtw_recv_mgmt;

	mtbl->mt_node_free = ic->ic_node_free;
	ic->ic_node_free = rtw_node_free;

	mtbl->mt_node_alloc = ic->ic_node_alloc;
	ic->ic_node_alloc = rtw_node_alloc;
}

static __inline void
rtw_establish_hooks(struct rtw_hooks *hooks, char (*dvname)[IFNAMSIZ],
    void *arg)
{
	/*
	 * Make sure the interface is shutdown during reboot.
	 */
	hooks->rh_shutdown = shutdownhook_establish(rtw_shutdown, arg);
	if (hooks->rh_shutdown == NULL)
		printf("%s: WARNING: unable to establish shutdown hook\n",
		    *dvname);

	/*
	 * Add a suspend hook to make sure we come back up after a
	 * resume.
	 */
	hooks->rh_power = powerhook_establish(rtw_power, arg);
	if (hooks->rh_power == NULL)
		printf("%s: WARNING: unable to establish power hook\n",
		    *dvname);
}

static __inline void
rtw_disestablish_hooks(struct rtw_hooks *hooks, char (*dvname)[IFNAMSIZ],
    void *arg)
{
	if (hooks->rh_shutdown != NULL)
		shutdownhook_disestablish(hooks->rh_shutdown);

	if (hooks->rh_power != NULL)
		powerhook_disestablish(hooks->rh_power);
}

static __inline void
rtw_init_radiotap(struct rtw_softc *sc)
{
	memset(&sc->sc_rxtapu, 0, sizeof(sc->sc_rxtapu));
	sc->sc_rxtap.rr_ihdr.it_len = sizeof(sc->sc_rxtapu);
	sc->sc_rxtap.rr_ihdr.it_present = RTW_RX_RADIOTAP_PRESENT;

	memset(&sc->sc_txtapu, 0, sizeof(sc->sc_txtapu));
	sc->sc_txtap.rt_ihdr.it_len = sizeof(sc->sc_txtapu);
	sc->sc_txtap.rt_ihdr.it_present = RTW_TX_RADIOTAP_PRESENT;
}

static int
rtw_txctl_blk_setup(struct rtw_txctl_blk *stc, u_int qlen)
{
	SIMPLEQ_INIT(&stc->stc_dirtyq);
	SIMPLEQ_INIT(&stc->stc_freeq);
	stc->stc_ndesc = qlen;
	stc->stc_desc = malloc(qlen * sizeof(*stc->stc_desc), M_DEVBUF,
	    M_NOWAIT);
	if (stc->stc_desc == NULL)
		return ENOMEM;
	return 0;
}

static void
rtw_txctl_blk_cleanup_all(struct rtw_softc *sc)
{
	struct rtw_txctl_blk *stc;
	int qlen[RTW_NTXPRI] =
	     {RTW_TXQLENLO, RTW_TXQLENMD, RTW_TXQLENHI, RTW_TXQLENBCN};
	int pri;

	for (pri = 0; pri < sizeof(qlen)/sizeof(qlen[0]); pri++) {
		stc = &sc->sc_txctl_blk[pri];
		free(stc->stc_desc, M_DEVBUF);
		stc->stc_desc = NULL;
	}
}

static int
rtw_txctl_blk_setup_all(struct rtw_softc *sc)
{
	int pri, rc = 0;
	int qlen[RTW_NTXPRI] =
	     {RTW_TXQLENLO, RTW_TXQLENMD, RTW_TXQLENHI, RTW_TXQLENBCN};

	for (pri = 0; pri < sizeof(qlen)/sizeof(qlen[0]); pri++) {
		rc = rtw_txctl_blk_setup(&sc->sc_txctl_blk[pri], qlen[pri]);
		if (rc != 0)
			break;
	}
	return rc;
}

static void
rtw_txdesc_blk_setup(struct rtw_txdesc_blk *htc, struct rtw_txdesc *desc,
    u_int ndesc, bus_addr_t ofs, bus_addr_t physbase)
{
	int i;

	htc->htc_ndesc = ndesc;
	htc->htc_desc = desc;
	htc->htc_physbase = physbase;
	htc->htc_ofs = ofs;

	(void)memset(htc->htc_desc, 0,
	    sizeof(htc->htc_desc[0]) * htc->htc_ndesc);

	for (i = 0; i < htc->htc_ndesc; i++) {
		htc->htc_desc[i].htx_next = htole32(RTW_NEXT_DESC(htc, i));
	}
}

static void
rtw_txdesc_blk_setup_all(struct rtw_softc *sc)
{
	rtw_txdesc_blk_setup(&sc->sc_txdesc_blk[RTW_TXPRILO],
	    &sc->sc_descs->hd_txlo[0], RTW_NTXDESCLO,
	    RTW_RING_OFFSET(hd_txlo), RTW_RING_BASE(sc, hd_txlo));

	rtw_txdesc_blk_setup(&sc->sc_txdesc_blk[RTW_TXPRIMD],
	    &sc->sc_descs->hd_txmd[0], RTW_NTXDESCMD,
	    RTW_RING_OFFSET(hd_txmd), RTW_RING_BASE(sc, hd_txmd));

	rtw_txdesc_blk_setup(&sc->sc_txdesc_blk[RTW_TXPRIHI],
	    &sc->sc_descs->hd_txhi[0], RTW_NTXDESCHI,
	    RTW_RING_OFFSET(hd_txhi), RTW_RING_BASE(sc, hd_txhi));

	rtw_txdesc_blk_setup(&sc->sc_txdesc_blk[RTW_TXPRIBCN],
	    &sc->sc_descs->hd_bcn[0], RTW_NTXDESCBCN,
	    RTW_RING_OFFSET(hd_bcn), RTW_RING_BASE(sc, hd_bcn));
}

static struct rtw_rf *
rtw_rf_attach(struct rtw_softc *sc, enum rtw_rfchipid rfchipid,
    rtw_rf_write_t rf_write, int digphy)
{
	struct rtw_rf *rf;

	switch (rfchipid) {
	case RTW_RFCHIPID_MAXIM:
		rf = rtw_max2820_create(&sc->sc_regs, rf_write, 0);
		sc->sc_pwrstate_cb = rtw_maxim_pwrstate;
		break;
	case RTW_RFCHIPID_PHILIPS:
		rf = rtw_sa2400_create(&sc->sc_regs, rf_write, digphy);
		sc->sc_pwrstate_cb = rtw_philips_pwrstate;
		break;
	default:
		return NULL;
	}
	rf->rf_continuous_tx_cb =
	    (rtw_continuous_tx_cb_t)rtw_continuous_tx_enable;
	rf->rf_continuous_tx_arg = (void *)sc;
	return rf;
}

/* Revision C and later use a different PHY delay setting than
 * revisions A and B.
 */
static u_int8_t
rtw_check_phydelay(struct rtw_regs *regs, u_int32_t rcr0)
{
#define REVAB (RTW_RCR_MXDMA_UNLIMITED | RTW_RCR_AICV)
#define REVC (REVAB | RTW_RCR_RXFTH_WHOLE)

	u_int8_t phydelay = LSHIFT(0x6, RTW_PHYDELAY_PHYDELAY);

	RTW_WRITE(regs, RTW_RCR, REVAB);
	RTW_WRITE(regs, RTW_RCR, REVC);

	RTW_WBR(regs, RTW_RCR, RTW_RCR);
	if ((RTW_READ(regs, RTW_RCR) & REVC) == REVC)
		phydelay |= RTW_PHYDELAY_REVC_MAGIC;

	RTW_WRITE(regs, RTW_RCR, rcr0);	/* restore RCR */

	return phydelay;
#undef REVC
}

void
rtw_attach(struct rtw_softc *sc)
{
	rtw_rf_write_t rf_write;
	struct rtw_txctl_blk *stc;
	int pri, rc, vers;

#if 0
	CASSERT(RTW_DESC_ALIGNMENT % sizeof(struct rtw_txdesc) == 0,
	    "RTW_DESC_ALIGNMENT is not a multiple of "
	    "sizeof(struct rtw_txdesc)");

	CASSERT(RTW_DESC_ALIGNMENT % sizeof(struct rtw_rxdesc) == 0,
	    "RTW_DESC_ALIGNMENT is not a multiple of "
	    "sizeof(struct rtw_rxdesc)");

	CASSERT(RTW_DESC_ALIGNMENT % RTW_MAXPKTSEGS == 0,
	    "RTW_DESC_ALIGNMENT is not a multiple of RTW_MAXPKTSEGS");
#endif

	NEXT_ATTACH_STATE(sc, DETACHED);

	switch (RTW_READ(&sc->sc_regs, RTW_TCR) & RTW_TCR_HWVERID_MASK) {
	case RTW_TCR_HWVERID_F:
		vers = 'F';
		rf_write = rtw_rf_hostwrite;
		break;
	case RTW_TCR_HWVERID_D:
		vers = 'D';
		rf_write = rtw_rf_macwrite;
		break;
	default:
		vers = '?';
		rf_write = rtw_rf_macwrite;
		break;
	}
	printf("%s: hardware version %c\n", sc->sc_dev.dv_xname, vers);

	rc = bus_dmamem_alloc(sc->sc_dmat, sizeof(struct rtw_descs),
	    RTW_DESC_ALIGNMENT, 0, &sc->sc_desc_segs, 1, &sc->sc_desc_nsegs,
	    0);

	if (rc != 0) {
		printf("%s: could not allocate hw descriptors, error %d\n",
		     sc->sc_dev.dv_xname, rc);
		goto err;
	}

	NEXT_ATTACH_STATE(sc, FINISH_DESC_ALLOC);

	rc = bus_dmamem_map(sc->sc_dmat, &sc->sc_desc_segs,
	    sc->sc_desc_nsegs, sizeof(struct rtw_descs),
	    (caddr_t*)&sc->sc_descs, BUS_DMA_COHERENT);

	if (rc != 0) {
		printf("%s: could not map hw descriptors, error %d\n",
		    sc->sc_dev.dv_xname, rc);
		goto err;
	}
	NEXT_ATTACH_STATE(sc, FINISH_DESC_MAP);

	rc = bus_dmamap_create(sc->sc_dmat, sizeof(struct rtw_descs), 1,
	    sizeof(struct rtw_descs), 0, 0, &sc->sc_desc_dmamap);

	if (rc != 0) {
		printf("%s: could not create DMA map for hw descriptors, "
		    "error %d\n", sc->sc_dev.dv_xname, rc);
		goto err;
	}
	NEXT_ATTACH_STATE(sc, FINISH_DESCMAP_CREATE);

	rc = bus_dmamap_load(sc->sc_dmat, sc->sc_desc_dmamap, sc->sc_descs,
	    sizeof(struct rtw_descs), NULL, 0);

	if (rc != 0) {
		printf("%s: could not load DMA map for hw descriptors, "
		    "error %d\n", sc->sc_dev.dv_xname, rc);
		goto err;
	}
	NEXT_ATTACH_STATE(sc, FINISH_DESCMAP_LOAD);

	if (rtw_txctl_blk_setup_all(sc) != 0)
		goto err;
	NEXT_ATTACH_STATE(sc, FINISH_TXCTLBLK_SETUP);

	rtw_txdesc_blk_setup_all(sc);

	NEXT_ATTACH_STATE(sc, FINISH_TXDESCBLK_SETUP);

	sc->sc_rxdesc = &sc->sc_descs->hd_rx[0];

	rtw_rxctls_setup(&sc->sc_rxctl);

	for (pri = 0; pri < RTW_NTXPRI; pri++) {
		stc = &sc->sc_txctl_blk[pri];

		if ((rc = rtw_txdesc_dmamaps_create(sc->sc_dmat,
		    &stc->stc_desc[0], stc->stc_ndesc)) != 0) {
			printf("%s: could not load DMA map for "
			    "hw tx descriptors, error %d\n",
			    sc->sc_dev.dv_xname, rc);
			goto err;
		}
	}

	NEXT_ATTACH_STATE(sc, FINISH_TXMAPS_CREATE);
	if ((rc = rtw_rxdesc_dmamaps_create(sc->sc_dmat, &sc->sc_rxctl[0],
	                                    RTW_RXQLEN)) != 0) {
		printf("%s: could not load DMA map for hw rx descriptors, "
		    "error %d\n", sc->sc_dev.dv_xname, rc);
		goto err;
	}
	NEXT_ATTACH_STATE(sc, FINISH_RXMAPS_CREATE);

	/* Reset the chip to a known state. */
	if (rtw_reset(sc) != 0)
		goto err;
	NEXT_ATTACH_STATE(sc, FINISH_RESET);

	sc->sc_rcr = RTW_READ(&sc->sc_regs, RTW_RCR);

	if ((sc->sc_rcr & RTW_RCR_9356SEL) != 0)
		sc->sc_flags |= RTW_F_9356SROM;

	if (rtw_srom_read(&sc->sc_regs, sc->sc_flags, &sc->sc_srom,
	    &sc->sc_dev.dv_xname) != 0)
		goto err;

	NEXT_ATTACH_STATE(sc, FINISH_READ_SROM);

	if (rtw_srom_parse(&sc->sc_srom, &sc->sc_flags, &sc->sc_csthr,
	    &sc->sc_rfchipid, &sc->sc_rcr, &sc->sc_locale,
	    &sc->sc_dev.dv_xname) != 0) {
		printf("%s: attach failed, malformed serial ROM\n",
		    sc->sc_dev.dv_xname);
		goto err;
	}

	RTW_DPRINTF(("%s: CS threshold %u\n", sc->sc_dev.dv_xname,
	    sc->sc_csthr));

	NEXT_ATTACH_STATE(sc, FINISH_PARSE_SROM);

	sc->sc_rf = rtw_rf_attach(sc, sc->sc_rfchipid, rf_write,
	    sc->sc_flags & RTW_F_DIGPHY);

	if (sc->sc_rf == NULL) {
		printf("%s: attach failed, could not attach RF\n",
		    sc->sc_dev.dv_xname);
		goto err;
	}

#if 0
	if (rtw_identify_rf(&sc->sc_regs, &sc->sc_rftype,
	    &sc->sc_dev.dv_xname) != 0) {
		printf("%s: attach failed, unknown RF unidentified\n",
		    sc->sc_dev.dv_xname);
		goto err;
	}
#endif

	NEXT_ATTACH_STATE(sc, FINISH_RF_ATTACH);

	sc->sc_phydelay = rtw_check_phydelay(&sc->sc_regs, sc->sc_rcr);

	RTW_DPRINTF(("%s: PHY delay %d\n", sc->sc_dev.dv_xname,
	    sc->sc_phydelay));

	if (sc->sc_locale == RTW_LOCALE_UNKNOWN)
		rtw_identify_country(&sc->sc_regs, &sc->sc_locale,
		    &sc->sc_dev.dv_xname);

	rtw_init_channels(sc->sc_locale, &sc->sc_ic.ic_channels,
	    &sc->sc_dev.dv_xname);

	if (rtw_identify_sta(&sc->sc_regs, &sc->sc_ic.ic_myaddr,
	    &sc->sc_dev.dv_xname) != 0)
		goto err;
	NEXT_ATTACH_STATE(sc, FINISH_ID_STA);

	rtw_setifprops(&sc->sc_if, &sc->sc_dev.dv_xname, (void*)sc);

	IFQ_SET_READY(&sc->sc_if.if_snd);

	rtw_set80211props(&sc->sc_ic);

	/*
	 * Call MI attach routines.
	 */
	if_attach(&sc->sc_if);
	ieee80211_ifattach(&sc->sc_if);

	rtw_set80211methods(&sc->sc_mtbl, &sc->sc_ic);

	/* possibly we should fill in our own sc_send_prresp, since
	 * the RTL8180 is probably sending probe responses in ad hoc
	 * mode.
	 */

	/* complete initialization */
	ieee80211_media_init(&sc->sc_if, rtw_media_change, rtw_media_status);
	callout_init(&sc->sc_scan_ch);

#if NBPFILTER > 0
	bpfattach2(&sc->sc_if, DLT_IEEE802_11_RADIO,
	    sizeof(struct ieee80211_frame) + 64, &sc->sc_radiobpf);
#endif

	rtw_establish_hooks(&sc->sc_hooks, &sc->sc_dev.dv_xname, (void*)sc);

	rtw_init_radiotap(sc);

	NEXT_ATTACH_STATE(sc, FINISHED);

	return;
err:
	rtw_detach(sc);
	return;
}

int
rtw_detach(struct rtw_softc *sc)
{
	int pri;

	switch (sc->sc_attach_state) {
	case FINISHED:
		rtw_disestablish_hooks(&sc->sc_hooks, &sc->sc_dev.dv_xname,
		    (void*)sc);
		callout_stop(&sc->sc_scan_ch);
		ieee80211_ifdetach(&sc->sc_if);
		if_detach(&sc->sc_if);
		break;
	case FINISH_ID_STA:
	case FINISH_RF_ATTACH:
		rtw_rf_destroy(sc->sc_rf);
		sc->sc_rf = NULL;
		/*FALLTHROUGH*/
	case FINISH_PARSE_SROM:
	case FINISH_READ_SROM:
		rtw_srom_free(&sc->sc_srom);
		/*FALLTHROUGH*/
	case FINISH_RESET:
	case FINISH_RXMAPS_CREATE:
		rtw_rxdesc_dmamaps_destroy(sc->sc_dmat, &sc->sc_rxctl[0],
		    RTW_RXQLEN);
		/*FALLTHROUGH*/
	case FINISH_TXMAPS_CREATE:
		for (pri = 0; pri < RTW_NTXPRI; pri++) {
			rtw_txdesc_dmamaps_destroy(sc->sc_dmat,
			    sc->sc_txctl_blk[pri].stc_desc,
			    sc->sc_txctl_blk[pri].stc_ndesc);
		}
		/*FALLTHROUGH*/
	case FINISH_TXDESCBLK_SETUP:
	case FINISH_TXCTLBLK_SETUP:
		rtw_txctl_blk_cleanup_all(sc);
		/*FALLTHROUGH*/
	case FINISH_DESCMAP_LOAD:
		bus_dmamap_unload(sc->sc_dmat, sc->sc_desc_dmamap);
		/*FALLTHROUGH*/
	case FINISH_DESCMAP_CREATE:
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_desc_dmamap);
		/*FALLTHROUGH*/
	case FINISH_DESC_MAP:
		bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_descs,
		    sizeof(struct rtw_descs));
		/*FALLTHROUGH*/
	case FINISH_DESC_ALLOC:
		bus_dmamem_free(sc->sc_dmat, &sc->sc_desc_segs,
		    sc->sc_desc_nsegs);
		/*FALLTHROUGH*/
	case DETACHED:
		NEXT_ATTACH_STATE(sc, DETACHED);
		break;
	}
	return 0;
}

int
rtw_activate(struct device *self, enum devact act)
{
	struct rtw_softc *sc = (struct rtw_softc *)self;
	int rc = 0, s;

	s = splnet();
	switch (act) {
	case DVACT_ACTIVATE:
		rc = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		if_deactivate(&sc->sc_ic.ic_if);
		break;
	}
	splx(s);
	return rc;
}
