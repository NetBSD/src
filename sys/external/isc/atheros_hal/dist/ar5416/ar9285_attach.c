/*
 * Copyright (c) 2008-2009 Sam Leffler, Errno Consulting
 * Copyright (c) 2008 Atheros Communications, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $FreeBSD: src/sys/dev/ath/ath_hal/ar5416/ar9285_attach.c,v 1.5 2010/06/01 15:33:10 rpaulo Exp $
 */
#include "opt_ah.h"

#include "ah.h"
#include "ah_internal.h"
#include "ah_devid.h"

#include "ah_eeprom_v4k.h"		/* XXX for tx/rx gain */

#include "ar5416/ar9280.h"
#include "ar5416/ar9285.h"
#include "ar5416/ar5416reg.h"
#include "ar5416/ar5416phy.h"

#include "ar5416/ar9285.ini"
#include "ar5416/ar9285v2.ini"
#include "ar5416/ar9280v2.ini"		/* XXX ini for tx/rx gain */

static const HAL_PERCAL_DATA ar9280_iq_cal = {		/* single sample */
	.calName = "IQ", .calType = IQ_MISMATCH_CAL,
	.calNumSamples	= MIN_CAL_SAMPLES,
	.calCountMax	= PER_MAX_LOG_COUNT,
	.calCollect	= ar5416IQCalCollect,
	.calPostProc	= ar5416IQCalibration
};
static const HAL_PERCAL_DATA ar9280_adc_gain_cal = {	/* single sample */
	.calName = "ADC Gain", .calType = ADC_GAIN_CAL,
	.calNumSamples	= MIN_CAL_SAMPLES,
	.calCountMax	= PER_MIN_LOG_COUNT,
	.calCollect	= ar5416AdcGainCalCollect,
	.calPostProc	= ar5416AdcGainCalibration
};
static const HAL_PERCAL_DATA ar9280_adc_dc_cal = {	/* single sample */
	.calName = "ADC DC", .calType = ADC_DC_CAL,
	.calNumSamples	= MIN_CAL_SAMPLES,
	.calCountMax	= PER_MIN_LOG_COUNT,
	.calCollect	= ar5416AdcDcCalCollect,
	.calPostProc	= ar5416AdcDcCalibration
};
static const HAL_PERCAL_DATA ar9280_adc_init_dc_cal = {
	.calName = "ADC Init DC", .calType = ADC_DC_INIT_CAL,
	.calNumSamples	= MIN_CAL_SAMPLES,
	.calCountMax	= INIT_LOG_COUNT,
	.calCollect	= ar5416AdcDcCalCollect,
	.calPostProc	= ar5416AdcDcCalibration
};

static void ar9285ConfigPCIE(struct ath_hal *ah, HAL_BOOL restore);
static HAL_BOOL ar9285FillCapabilityInfo(struct ath_hal *ah);
static void ar9285WriteIni(struct ath_hal *ah,
	HAL_CHANNEL_INTERNAL *chan);

static void
ar9285AniSetup(struct ath_hal *ah)
{
	/* NB: disable ANI for reliable RIFS rx */
	ar5212AniAttach(ah, AH_NULL, AH_NULL, AH_FALSE);
}

/*
 * Attach for an AR9285 part.
 */
static struct ath_hal *
ar9285Attach(uint16_t devid, HAL_SOFTC sc,
	HAL_BUS_TAG st, HAL_BUS_HANDLE sh, HAL_STATUS *status)
{
	struct ath_hal_9285 *ahp9285;
	struct ath_hal_5212 *ahp;
	struct ath_hal *ah;
	uint32_t val;
	HAL_STATUS ecode;
	HAL_BOOL rfStatus;

	HALDEBUG(AH_NULL, HAL_DEBUG_ATTACH, "%s: sc %p st %p sh %p\n",
	    __func__, sc, (void*) st, (void*) sh);

	/* NB: memory is returned zero'd */
	ahp9285 = ath_hal_malloc(sizeof (struct ath_hal_9285));
	if (ahp9285 == AH_NULL) {
		HALDEBUG(AH_NULL, HAL_DEBUG_ANY,
		    "%s: cannot allocate memory for state block\n", __func__);
		*status = HAL_ENOMEM;
		return AH_NULL;
	}
	ahp = AH5212(ahp9285);
	ah = &ahp->ah_priv.h;

	ar5416InitState(AH5416(ah), devid, sc, st, sh, status);

	/* XXX override with 9285 specific state */
	/* override 5416 methods for our needs */
	ah->ah_setAntennaSwitch		= ar9285SetAntennaSwitch;
	ah->ah_configPCIE		= ar9285ConfigPCIE;
	ah->ah_setTxPower		= ar9285SetTransmitPower;
	ah->ah_setBoardValues		= ar9285SetBoardValues;

	AH5416(ah)->ah_cal.iqCalData.calData = &ar9280_iq_cal;
	AH5416(ah)->ah_cal.adcGainCalData.calData = &ar9280_adc_gain_cal;
	AH5416(ah)->ah_cal.adcDcCalData.calData = &ar9280_adc_dc_cal;
	AH5416(ah)->ah_cal.adcDcCalInitData.calData = &ar9280_adc_init_dc_cal;
	AH5416(ah)->ah_cal.suppCals = ADC_GAIN_CAL | ADC_DC_CAL | IQ_MISMATCH_CAL;

	AH5416(ah)->ah_spurMitigate	= ar9280SpurMitigate;
	AH5416(ah)->ah_writeIni		= ar9285WriteIni;
	AH5416(ah)->ah_rx_chainmask	= AR9285_DEFAULT_RXCHAINMASK;
	AH5416(ah)->ah_tx_chainmask	= AR9285_DEFAULT_TXCHAINMASK;

	ahp->ah_maxTxTrigLev		= MAX_TX_FIFO_THRESHOLD >> 1;

	if (!ar5416SetResetReg(ah, HAL_RESET_POWER_ON)) {
		/* reset chip */
		HALDEBUG(ah, HAL_DEBUG_ANY, "%s: couldn't reset chip\n",
		    __func__);
		ecode = HAL_EIO;
		goto bad;
	}

	if (!ar5416SetPowerMode(ah, HAL_PM_AWAKE, AH_TRUE)) {
		HALDEBUG(ah, HAL_DEBUG_ANY, "%s: couldn't wakeup chip\n",
		    __func__);
		ecode = HAL_EIO;
		goto bad;
	}
	/* Read Revisions from Chips before taking out of reset */
	val = OS_REG_READ(ah, AR_SREV);
	HALDEBUG(ah, HAL_DEBUG_ATTACH,
	    "%s: ID 0x%x VERSION 0x%x TYPE 0x%x REVISION 0x%x\n",
	    __func__, MS(val, AR_XSREV_ID), MS(val, AR_XSREV_VERSION),
	    MS(val, AR_XSREV_TYPE), MS(val, AR_XSREV_REVISION));
	/* NB: include chip type to differentiate from pre-Sowl versions */
	AH_PRIVATE(ah)->ah_macVersion =
	    (val & AR_XSREV_VERSION) >> AR_XSREV_TYPE_S;
	AH_PRIVATE(ah)->ah_macRev = MS(val, AR_XSREV_REVISION);
	AH_PRIVATE(ah)->ah_ispcie = (val & AR_XSREV_TYPE_HOST_MODE) == 0;

	/* setup common ini data; rf backends handle remainder */
	if (AR_SREV_KITE_12_OR_LATER(ah)) {
		HAL_INI_INIT(&ahp->ah_ini_modes, ar9285Modes_v2, 6);
		HAL_INI_INIT(&ahp->ah_ini_common, ar9285Common_v2, 2);
		HAL_INI_INIT(&AH5416(ah)->ah_ini_pcieserdes,
		    ar9285PciePhy_clkreq_always_on_L1_v2, 2);
	} else {
		HAL_INI_INIT(&ahp->ah_ini_modes, ar9285Modes, 6);
		HAL_INI_INIT(&ahp->ah_ini_common, ar9285Common, 2);
		HAL_INI_INIT(&AH5416(ah)->ah_ini_pcieserdes,
		    ar9285PciePhy_clkreq_always_on_L1, 2);
	}
	ar5416AttachPCIE(ah);

	ecode = ath_hal_v4kEepromAttach(ah);
	if (ecode != HAL_OK)
		goto bad;

	if (!ar5416ChipReset(ah, AH_NULL)) {	/* reset chip */
		HALDEBUG(ah, HAL_DEBUG_ANY, "%s: chip reset failed\n",
		    __func__);
		ecode = HAL_EIO;
		goto bad;
	}

	AH_PRIVATE(ah)->ah_phyRev = OS_REG_READ(ah, AR_PHY_CHIP_ID);

	if (!ar5212ChipTest(ah)) {
		HALDEBUG(ah, HAL_DEBUG_ANY, "%s: hardware self-test failed\n",
		    __func__);
		ecode = HAL_ESELFTEST;
		goto bad;
	}

	/*
	 * Set correct Baseband to analog shift
	 * setting to access analog chips.
	 */
	OS_REG_WRITE(ah, AR_PHY(0), 0x00000007);

	/* Read Radio Chip Rev Extract */
	AH_PRIVATE(ah)->ah_analog5GhzRev = ar5416GetRadioRev(ah);
	switch (AH_PRIVATE(ah)->ah_analog5GhzRev & AR_RADIO_SREV_MAJOR) {
        case AR_RAD2133_SREV_MAJOR:	/* Sowl: 2G/3x3 */
	case AR_RAD5133_SREV_MAJOR:	/* Sowl: 2+5G/3x3 */
		break;
	default:
		if (AH_PRIVATE(ah)->ah_analog5GhzRev == 0) {
			AH_PRIVATE(ah)->ah_analog5GhzRev =
				AR_RAD5133_SREV_MAJOR;
			break;
		}
#ifdef AH_DEBUG
		HALDEBUG(ah, HAL_DEBUG_ANY,
		    "%s: 5G Radio Chip Rev 0x%02X is not supported by "
		    "this driver\n", __func__,
		    AH_PRIVATE(ah)->ah_analog5GhzRev);
		ecode = HAL_ENOTSUPP;
		goto bad;
#endif
	}
	rfStatus = ar9285RfAttach(ah, &ecode);
	if (!rfStatus) {
		HALDEBUG(ah, HAL_DEBUG_ANY, "%s: RF setup failed, status %u\n",
		    __func__, ecode);
		goto bad;
	}

	HAL_INI_INIT(&ahp9285->ah_ini_rxgain, ar9280Modes_original_rxgain_v2,
	    6);
	/* setup txgain table */
	switch (ath_hal_eepromGet(ah, AR_EEP_TXGAIN_TYPE, AH_NULL)) {
	case AR5416_EEP_TXGAIN_HIGH_POWER:
		HAL_INI_INIT(&ahp9285->ah_ini_txgain,
		    ar9285Modes_high_power_tx_gain_v2, 6);
		break;
	case AR5416_EEP_TXGAIN_ORIG:
		HAL_INI_INIT(&ahp9285->ah_ini_txgain,
		    ar9285Modes_original_tx_gain_v2, 6);
		break;
	default:
		HALASSERT(AH_FALSE);
		goto bad;		/* XXX ? try to continue */
	}

	/*
	 * Got everything we need now to setup the capabilities.
	 */
	if (!ar9285FillCapabilityInfo(ah)) {
		ecode = HAL_EEREAD;
		goto bad;
	}

	ecode = ath_hal_eepromGet(ah, AR_EEP_MACADDR, ahp->ah_macaddr);
	if (ecode != HAL_OK) {
		HALDEBUG(ah, HAL_DEBUG_ANY,
		    "%s: error getting mac address from EEPROM\n", __func__);
		goto bad;
        }
	/* XXX How about the serial number ? */
	/* Read Reg Domain */
	AH_PRIVATE(ah)->ah_currentRD =
	    ath_hal_eepromGet(ah, AR_EEP_REGDMN_0, AH_NULL);

	/*
	 * ah_miscMode is populated by ar5416FillCapabilityInfo()
	 * starting from griffin. Set here to make sure that
	 * AR_MISC_MODE_MIC_NEW_LOC_ENABLE is set before a GTK is
	 * placed into hardware.
	 */
	if (ahp->ah_miscMode != 0)
		OS_REG_WRITE(ah, AR_MISC_MODE, ahp->ah_miscMode);

	ar9285AniSetup(ah);			/* Anti Noise Immunity */
	ar5416InitNfHistBuff(AH5416(ah)->ah_cal.nfCalHist);

	HALDEBUG(ah, HAL_DEBUG_ATTACH, "%s: return\n", __func__);

	return ah;
bad:
	if (ah != AH_NULL)
		ah->ah_detach(ah);
	if (status)
		*status = ecode;
	return AH_NULL;
}

static void
ar9285ConfigPCIE(struct ath_hal *ah, HAL_BOOL restore)
{
	if (AH_PRIVATE(ah)->ah_ispcie && !restore) {
		ath_hal_ini_write(ah, &AH5416(ah)->ah_ini_pcieserdes, 1, 0);
		OS_DELAY(1000);
		OS_REG_SET_BIT(ah, AR_PCIE_PM_CTRL, AR_PCIE_PM_CTRL_ENA);
		OS_REG_WRITE(ah, AR_WA, AR9285_WA_DEFAULT);
	}
}

static void
ar9285WriteIni(struct ath_hal *ah, HAL_CHANNEL_INTERNAL *chan)
{
	u_int modesIndex;
	int regWrites = 0;

	/* Setup the indices for the next set of register array writes */
	/* XXX Ignore 11n dynamic mode on the AR5416 for the moment */
	if (IS_CHAN_HT40(chan))
		modesIndex = 3;
	else if (IS_CHAN_108G(chan))
		modesIndex = 5;
	else
		modesIndex = 4;

	/* Set correct Baseband to analog shift setting to access analog chips. */
	OS_REG_WRITE(ah, AR_PHY(0), 0x00000007);
	OS_REG_WRITE(ah, AR_PHY_ADC_SERIAL_CTL, AR_PHY_SEL_INTERNAL_ADDAC);
	regWrites = ath_hal_ini_write(ah, &AH5212(ah)->ah_ini_modes,
	    modesIndex, regWrites);
	if (AR_SREV_KITE_12_OR_LATER(ah)) {
		regWrites = ath_hal_ini_write(ah, &AH9285(ah)->ah_ini_txgain,
		    modesIndex, regWrites);
	}
	regWrites = ath_hal_ini_write(ah, &AH5212(ah)->ah_ini_common,
	    1, regWrites);

      	OS_REG_SET_BIT(ah, AR_DIAG_SW, (AR_DIAG_RX_DIS | AR_DIAG_RX_ABORT));

	if (AR_SREV_MERLIN_10_OR_LATER(ah)) {
		uint32_t val;
		val = OS_REG_READ(ah, AR_PCU_MISC_MODE2) &
			(~AR_PCU_MISC_MODE2_HWWAR1);
		OS_REG_WRITE(ah, AR_PCU_MISC_MODE2, val);
		OS_REG_WRITE(ah, 0x9800 + (651 << 2), 0x11);
	}

}

/*
 * Fill all software cached or static hardware state information.
 * Return failure if capabilities are to come from EEPROM and
 * cannot be read.
 */
static HAL_BOOL
ar9285FillCapabilityInfo(struct ath_hal *ah)
{
	HAL_CAPABILITIES *pCap = &AH_PRIVATE(ah)->ah_caps;

	if (!ar5416FillCapabilityInfo(ah))
		return AH_FALSE;
	pCap->halNumGpioPins = 12;
	pCap->halWowSupport = AH_TRUE;
	pCap->halWowMatchPatternExact = AH_TRUE;
#if 0
	pCap->halWowMatchPatternDword = AH_TRUE;
#endif
	pCap->halCSTSupport = AH_TRUE;
	pCap->halRifsRxSupport = AH_TRUE;
	pCap->halRifsTxSupport = AH_TRUE;
	pCap->halRtsAggrLimit = 64*1024;	/* 802.11n max */
	pCap->halExtChanDfsSupport = AH_TRUE;
#if 0
	/* XXX bluetooth */
	pCap->halBtCoexSupport = AH_TRUE;
#endif
	pCap->halAutoSleepSupport = AH_FALSE;	/* XXX? */
#if 0
	pCap->hal4kbSplitTransSupport = AH_FALSE;
#endif
	pCap->halRxStbcSupport = 1;
	pCap->halTxStbcSupport = 1;

	return AH_TRUE;
}

HAL_BOOL
ar9285SetAntennaSwitch(struct ath_hal *ah, HAL_ANT_SETTING settings)
{
#define ANTENNA0_CHAINMASK    0x1
#define ANTENNA1_CHAINMASK    0x2
	struct ath_hal_5416 *ahp = AH5416(ah);

	/* Antenna selection is done by setting the tx/rx chainmasks approp. */
	switch (settings) {
	case HAL_ANT_FIXED_A:
		/* Enable first antenna only */
		ahp->ah_tx_chainmask = ANTENNA0_CHAINMASK;
		ahp->ah_rx_chainmask = ANTENNA0_CHAINMASK;
		break;
	case HAL_ANT_FIXED_B:
		/* Enable second antenna only, after checking capability */
		if (AH_PRIVATE(ah)->ah_caps.halTxChainMask > ANTENNA1_CHAINMASK)
			ahp->ah_tx_chainmask = ANTENNA1_CHAINMASK;
		ahp->ah_rx_chainmask = ANTENNA1_CHAINMASK;
		break;
	case HAL_ANT_VARIABLE:
		/* Restore original chainmask settings */
		/* XXX */
		ahp->ah_tx_chainmask = AR9285_DEFAULT_TXCHAINMASK;
		ahp->ah_rx_chainmask = AR9285_DEFAULT_RXCHAINMASK;
		break;
	}
	return AH_TRUE;
#undef ANTENNA0_CHAINMASK
#undef ANTENNA1_CHAINMASK
}

static const char*
ar9285Probe(uint16_t vendorid, uint16_t devid)
{
	if (vendorid == ATHEROS_VENDOR_ID && devid == AR9285_DEVID_PCIE)
		return "Atheros 9285";
	return AH_NULL;
}
AH_CHIP(AR9285, ar9285Probe, ar9285Attach);
