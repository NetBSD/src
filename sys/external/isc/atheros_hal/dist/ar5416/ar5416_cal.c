/*
 * Copyright (c) 2002-2008 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2008 Atheros Communications, Inc.
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
 * $Id: ar5416_cal.c,v 1.1.1.1.18.1 2011/06/06 09:09:19 jruoho Exp $
 */
#include "opt_ah.h"

#include "ah.h"
#include "ah_internal.h"
#include "ah_devid.h"

#include "ah_eeprom_v14.h"

#include "ar5212/ar5212.h"	/* for NF cal related declarations */

#include "ar5416/ar5416.h"
#include "ar5416/ar5416reg.h"
#include "ar5416/ar5416phy.h"

/* Owl specific stuff */
#define NUM_NOISEFLOOR_READINGS 6       /* 3 chains * (ctl + ext) */

static void ar5416StartNFCal(struct ath_hal *ah);
static void ar5416LoadNF(struct ath_hal *ah, HAL_CHANNEL_INTERNAL *);
static int16_t ar5416GetNf(struct ath_hal *, HAL_CHANNEL_INTERNAL *);

/*
 * Determine if calibration is supported by device and channel flags
 */
static OS_INLINE HAL_BOOL
ar5416IsCalSupp(struct ath_hal *ah, HAL_CHANNEL *chan, HAL_CAL_TYPE calType) 
{
	struct ar5416PerCal *cal = &AH5416(ah)->ah_cal;

	switch (calType & cal->suppCals) {
	case IQ_MISMATCH_CAL:
		/* Run IQ Mismatch for non-CCK only */
		return !IS_CHAN_B(chan);
	case ADC_GAIN_CAL:
	case ADC_DC_CAL:
		/* Run ADC Gain Cal for non-CCK & non 2GHz-HT20 only */
		return !IS_CHAN_B(chan) &&
		    !(IS_CHAN_2GHZ(chan) && IS_CHAN_HT20(chan));
	}
	return AH_FALSE;
}

/*
 * Setup HW to collect samples used for current cal
 */
static void
ar5416SetupMeasurement(struct ath_hal *ah, HAL_CAL_LIST *currCal)
{
	/* Start calibration w/ 2^(INIT_IQCAL_LOG_COUNT_MAX+1) samples */
	OS_REG_RMW_FIELD(ah, AR_PHY_TIMING_CTRL4,
	    AR_PHY_TIMING_CTRL4_IQCAL_LOG_COUNT_MAX,
	    currCal->calData->calCountMax);

	/* Select calibration to run */
	switch (currCal->calData->calType) {
	case IQ_MISMATCH_CAL:
		OS_REG_WRITE(ah, AR_PHY_CALMODE, AR_PHY_CALMODE_IQ);
		HALDEBUG(ah, HAL_DEBUG_PERCAL,
		    "%s: start IQ Mismatch calibration\n", __func__);
		break;
	case ADC_GAIN_CAL:
		OS_REG_WRITE(ah, AR_PHY_CALMODE, AR_PHY_CALMODE_ADC_GAIN);
		HALDEBUG(ah, HAL_DEBUG_PERCAL,
		    "%s: start ADC Gain calibration\n", __func__);
		break;
	case ADC_DC_CAL:
		OS_REG_WRITE(ah, AR_PHY_CALMODE, AR_PHY_CALMODE_ADC_DC_PER);
		HALDEBUG(ah, HAL_DEBUG_PERCAL,
		    "%s: start ADC DC calibration\n", __func__);
		break;
	case ADC_DC_INIT_CAL:
		OS_REG_WRITE(ah, AR_PHY_CALMODE, AR_PHY_CALMODE_ADC_DC_INIT);
		HALDEBUG(ah, HAL_DEBUG_PERCAL,
		    "%s: start Init ADC DC calibration\n", __func__);
		break;
	}
	/* Kick-off cal */
	OS_REG_SET_BIT(ah, AR_PHY_TIMING_CTRL4, AR_PHY_TIMING_CTRL4_DO_CAL);
}

/*
 * Initialize shared data structures and prepare a cal to be run.
 */
static void
ar5416ResetMeasurement(struct ath_hal *ah, HAL_CAL_LIST *currCal)
{
	struct ar5416PerCal *cal = &AH5416(ah)->ah_cal;

	/* Reset data structures shared between different calibrations */
	OS_MEMZERO(cal->caldata, sizeof(cal->caldata));
	cal->calSamples = 0;

	/* Setup HW for new calibration */
	ar5416SetupMeasurement(ah, currCal);

	/* Change SW state to RUNNING for this calibration */
	currCal->calState = CAL_RUNNING;
}

#if 0
/*
 * Run non-periodic calibrations.
 */
static HAL_BOOL
ar5416RunInitCals(struct ath_hal *ah, int init_cal_count)
{
	struct ath_hal_5416 *ahp = AH5416(ah);
	struct ar5416PerCal *cal = &AH5416(ah)->ah_cal;
	HAL_CHANNEL_INTERNAL ichan;	/* XXX bogus */
	HAL_CAL_LIST *curCal = ahp->ah_cal_curr;
	HAL_BOOL isCalDone;
	int i;

	if (curCal == AH_NULL)
		return AH_FALSE;

	ichan.calValid = 0;
	for (i = 0; i < init_cal_count; i++) {
		/* Reset this Cal */
		ar5416ResetMeasurement(ah, curCal);
		/* Poll for offset calibration complete */
		if (!ath_hal_wait(ah, AR_PHY_TIMING_CTRL4, AR_PHY_TIMING_CTRL4_DO_CAL, 0)) {
			HALDEBUG(ah, HAL_DEBUG_ANY,
			    "%s: Cal %d failed to finish in 100ms.\n",
			    __func__, curCal->calData->calType);
			/* Re-initialize list pointers for periodic cals */
			cal->cal_list = cal->cal_last = cal->cal_curr = AH_NULL;
			return AH_FALSE;
		}
		/* Run this cal */
		ar5416DoCalibration(ah, &ichan, ahp->ah_rxchainmask,
		    curCal, &isCalDone);
		if (!isCalDone)
			HALDEBUG(ah, HAL_DEBUG_ANY,
			    "%s: init cal %d did not complete.\n",
			    __func__, curCal->calData->calType);
		if (curCal->calNext != AH_NULL)
			curCal = curCal->calNext;
	}

	/* Re-initialize list pointers for periodic cals */
	cal->cal_list = cal->cal_last = cal->cal_curr = AH_NULL;
	return AH_TRUE;
}
#endif

/*
 * Initialize Calibration infrastructure.
 */
HAL_BOOL
ar5416InitCal(struct ath_hal *ah, HAL_CHANNEL *chan)
{
	struct ar5416PerCal *cal = &AH5416(ah)->ah_cal;
	HAL_CHANNEL_INTERNAL *ichan;

	ichan = ath_hal_checkchannel(ah, chan);
	HALASSERT(ichan != AH_NULL);

	if (AR_SREV_MERLIN_10_OR_LATER(ah)) {
		/* Enable Rx Filter Cal */
		OS_REG_CLR_BIT(ah, AR_PHY_ADC_CTL, AR_PHY_ADC_CTL_OFF_PWDADC);
		OS_REG_SET_BIT(ah, AR_PHY_AGC_CONTROL,
		    AR_PHY_AGC_CONTROL_FLTR_CAL);

		/* Clear the carrier leak cal bit */
		OS_REG_CLR_BIT(ah, AR_PHY_CL_CAL_CTL, AR_PHY_CL_CAL_ENABLE);

		/* kick off the cal */
		OS_REG_SET_BIT(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_CAL);

		/* Poll for offset calibration complete */
		if (!ath_hal_wait(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_CAL, 0)) {
			HALDEBUG(ah, HAL_DEBUG_ANY,
			    "%s: offset calibration failed to complete in 1ms; "
			    "noisy environment?\n", __func__);
			return AH_FALSE;
		}

		/* Set the cl cal bit and rerun the cal a 2nd time */
		/* Enable Rx Filter Cal */
		OS_REG_CLR_BIT(ah, AR_PHY_ADC_CTL, AR_PHY_ADC_CTL_OFF_PWDADC);
		OS_REG_SET_BIT(ah, AR_PHY_AGC_CONTROL,
		    AR_PHY_AGC_CONTROL_FLTR_CAL);

		OS_REG_SET_BIT(ah, AR_PHY_CL_CAL_CTL, AR_PHY_CL_CAL_ENABLE);
	} 	

	/* Calibrate the AGC */
	OS_REG_SET_BIT(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_CAL);

	/* Poll for offset calibration complete */
	if (!ath_hal_wait(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_CAL, 0)) {
		HALDEBUG(ah, HAL_DEBUG_ANY,
		    "%s: offset calibration did not complete in 1ms; "
		    "noisy environment?\n", __func__);
		return AH_FALSE;
	}

	/* 
	 * Do NF calibration after DC offset and other CALs.
	 * Per system engineers, noise floor value can sometimes be 20 dB
	 * higher than normal value if DC offset and noise floor cal are
	 * triggered at the same time.
	 */
	/* XXX this actually kicks off a NF calibration -adrian */
	OS_REG_SET_BIT(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_NF);
	/*
	 * Try to make sure the above NF cal completes, just so
	 * it doesn't clash with subsequent percals - adrian
	 */
	if (!ar5212WaitNFCalComplete(ah, 10000)) {
		HALDEBUG(ah, HAL_DEBUG_ANY, "%s: initial NF calibration did "
		    "not complete in time; noisy environment?\n", __func__);
		return AH_FALSE;
	}

	/* Initialize list pointers */
	cal->cal_list = cal->cal_last = cal->cal_curr = AH_NULL;

	/*
	 * Enable IQ, ADC Gain, ADC DC Offset Cals
	 */
	if (AR_SREV_SOWL_10_OR_LATER(ah)) {
		/* Setup all non-periodic, init time only calibrations */
		/* XXX: Init DC Offset not working yet */
#if 0
		if (ar5416IsCalSupp(ah, chan, ADC_DC_INIT_CAL)) {
			INIT_CAL(&cal->adcDcCalInitData);
			INSERT_CAL(cal, &cal->adcDcCalInitData);
		}
		/* Initialize current pointer to first element in list */
		cal->cal_curr = cal->cal_list;

		if (cal->ah_cal_curr != AH_NULL && !ar5416RunInitCals(ah, 0))
			return AH_FALSE;
#endif
	}

	/* If Cals are supported, add them to list via INIT/INSERT_CAL */
	if (ar5416IsCalSupp(ah, chan, ADC_GAIN_CAL)) {
		INIT_CAL(&cal->adcGainCalData);
		INSERT_CAL(cal, &cal->adcGainCalData);
		HALDEBUG(ah, HAL_DEBUG_PERCAL,
		    "%s: enable ADC Gain Calibration.\n", __func__);
	}
	if (ar5416IsCalSupp(ah, chan, ADC_DC_CAL)) {
		INIT_CAL(&cal->adcDcCalData);
		INSERT_CAL(cal, &cal->adcDcCalData);
		HALDEBUG(ah, HAL_DEBUG_PERCAL,
		    "%s: enable ADC DC Calibration.\n", __func__);
	}
	if (ar5416IsCalSupp(ah, chan, IQ_MISMATCH_CAL)) {
		INIT_CAL(&cal->iqCalData);
		INSERT_CAL(cal, &cal->iqCalData);
		HALDEBUG(ah, HAL_DEBUG_PERCAL,
		    "%s: enable IQ Calibration.\n", __func__);
	}
	/* Initialize current pointer to first element in list */
	cal->cal_curr = cal->cal_list;

	/* Kick off measurements for the first cal */
	if (cal->cal_curr != AH_NULL)
		ar5416ResetMeasurement(ah, cal->cal_curr);

	/* Mark all calibrations on this channel as being invalid */
	ichan->calValid = 0;

	return AH_TRUE;
}

/*
 * Entry point for upper layers to restart current cal.
 * Reset the calibration valid bit in channel.
 */
HAL_BOOL
ar5416ResetCalValid(struct ath_hal *ah, HAL_CHANNEL *chan)
{
	struct ar5416PerCal *cal = &AH5416(ah)->ah_cal;
	HAL_CHANNEL_INTERNAL *ichan = ath_hal_checkchannel(ah, chan);
	HAL_CAL_LIST *currCal = cal->cal_curr;

	if (!AR_SREV_SOWL_10_OR_LATER(ah))
		return AH_FALSE;
	if (currCal == AH_NULL)
		return AH_FALSE;
	if (ichan == AH_NULL) {
		HALDEBUG(ah, HAL_DEBUG_ANY,
		    "%s: invalid channel %u/0x%x; no mapping\n",
		    __func__, chan->channel, chan->channelFlags);
		return AH_FALSE;
	}
	/*
	 * Expected that this calibration has run before, post-reset.
	 * Current state should be done
	 */
	if (currCal->calState != CAL_DONE) {
		HALDEBUG(ah, HAL_DEBUG_ANY,
		    "%s: Calibration state incorrect, %d\n",
		    __func__, currCal->calState);
		return AH_FALSE;
	}

	/* Verify Cal is supported on this channel */
	if (!ar5416IsCalSupp(ah, chan, currCal->calData->calType))
		return AH_FALSE;

	HALDEBUG(ah, HAL_DEBUG_PERCAL,
	    "%s: Resetting Cal %d state for channel %u/0x%x\n",
	    __func__, currCal->calData->calType, chan->channel,
	    chan->channelFlags);

	/* Disable cal validity in channel */
	ichan->calValid &= ~currCal->calData->calType;
	currCal->calState = CAL_WAITING;

	return AH_TRUE;
}

/*
 * Recalibrate the lower PHY chips to account for temperature/environment
 * changes.
 */
static void
ar5416DoCalibration(struct ath_hal *ah,  HAL_CHANNEL_INTERNAL *ichan,
	uint8_t rxchainmask, HAL_CAL_LIST *currCal, HAL_BOOL *isCalDone)
{
	struct ar5416PerCal *cal = &AH5416(ah)->ah_cal;

	/* Cal is assumed not done until explicitly set below */
	*isCalDone = AH_FALSE;

	HALDEBUG(ah, HAL_DEBUG_PERCAL,
	    "%s: %s Calibration, state %d, calValid 0x%x\n",
	    __func__, currCal->calData->calName, currCal->calState,
	    ichan->calValid);

	/* Calibration in progress. */
	if (currCal->calState == CAL_RUNNING) {
		/* Check to see if it has finished. */
		if (!(OS_REG_READ(ah, AR_PHY_TIMING_CTRL4) & AR_PHY_TIMING_CTRL4_DO_CAL)) {
			HALDEBUG(ah, HAL_DEBUG_PERCAL,
			    "%s: sample %d of %d finished\n",
			    __func__, cal->calSamples,
			    currCal->calData->calNumSamples);
			/* 
			 * Collect measurements for active chains.
			 */
			currCal->calData->calCollect(ah);
			if (++cal->calSamples >= currCal->calData->calNumSamples) {
				int i, numChains = 0;
				for (i = 0; i < AR5416_MAX_CHAINS; i++) {
					if (rxchainmask & (1 << i))
						numChains++;
				}
				/* 
				 * Process accumulated data
				 */
				currCal->calData->calPostProc(ah, numChains);

				/* Calibration has finished. */
				ichan->calValid |= currCal->calData->calType;
				currCal->calState = CAL_DONE;
				*isCalDone = AH_TRUE;
			} else {
				/*
				 * Set-up to collect of another sub-sample.
				 */
				ar5416SetupMeasurement(ah, currCal);
			}
		}
	} else if (!(ichan->calValid & currCal->calData->calType)) {
		/* If current cal is marked invalid in channel, kick it off */
		ar5416ResetMeasurement(ah, currCal);
	}
}

/*
 * Internal interface to schedule periodic calibration work.
 */
HAL_BOOL
ar5416PerCalibrationN(struct ath_hal *ah,  HAL_CHANNEL *chan,
	u_int rxchainmask, HAL_BOOL longcal, HAL_BOOL *isCalDone)
{
	struct ar5416PerCal *cal = &AH5416(ah)->ah_cal;
	HAL_CAL_LIST *currCal = cal->cal_curr;
	HAL_CHANNEL_INTERNAL *ichan;

	OS_MARK(ah, AH_MARK_PERCAL, chan->channel);

	*isCalDone = AH_TRUE;

	/*
	 * Since ath_hal calls the PerCal method with rxchainmask=0x1;
	 * override it with the current chainmask. The upper levels currently
	 * doesn't know about the chainmask.
	 */
	rxchainmask = AH5416(ah)->ah_rx_chainmask;

	/* Invalid channel check */
	ichan = ath_hal_checkchannel(ah, chan);
	if (ichan == AH_NULL) {
		HALDEBUG(ah, HAL_DEBUG_ANY,
		    "%s: invalid channel %u/0x%x; no mapping\n",
		    __func__, chan->channel, chan->channelFlags);
		return AH_FALSE;
	}

	/*
	 * For given calibration:
	 * 1. Call generic cal routine
	 * 2. When this cal is done (isCalDone) if we have more cals waiting
	 *    (eg after reset), mask this to upper layers by not propagating
	 *    isCalDone if it is set to TRUE.
	 *    Instead, change isCalDone to FALSE and setup the waiting cal(s)
	 *    to be run.
	 */
	if (currCal != AH_NULL &&
	    (currCal->calState == CAL_RUNNING ||
	     currCal->calState == CAL_WAITING)) {
		ar5416DoCalibration(ah, ichan, rxchainmask, currCal, isCalDone);
		if (*isCalDone == AH_TRUE) {
			cal->cal_curr = currCal = currCal->calNext;
			if (currCal->calState == CAL_WAITING) {
				*isCalDone = AH_FALSE;
				ar5416ResetMeasurement(ah, currCal);
			}
		}
	}

	/* Do NF cal only at longer intervals */
	if (longcal) {
		/*
		 * Get the value from the previous NF cal
		 * and update the history buffer.
		 */
		ar5416GetNf(ah, ichan);

		/* 
		 * Load the NF from history buffer of the current channel.
		 * NF is slow time-variant, so it is OK to use a
		 * historical value.
		 */
		ar5416LoadNF(ah, AH_PRIVATE(ah)->ah_curchan);

		/* start NF calibration, without updating BB NF register*/
		ar5416StartNFCal(ah);
	}
	return AH_TRUE;
}

/*
 * Recalibrate the lower PHY chips to account for temperature/environment
 * changes.
 */
HAL_BOOL
ar5416PerCalibration(struct ath_hal *ah,  HAL_CHANNEL *chan, HAL_BOOL *isIQdone)
{
	struct ath_hal_5416 *ahp = AH5416(ah);
	struct ar5416PerCal *cal = &AH5416(ah)->ah_cal;
	HAL_CAL_LIST *curCal = cal->cal_curr;

	if (curCal != AH_NULL && curCal->calData->calType == IQ_MISMATCH_CAL) {
		return ar5416PerCalibrationN(ah, chan, ahp->ah_rx_chainmask,
		    AH_TRUE, isIQdone);
	} else {
		HAL_BOOL isCalDone;

		*isIQdone = AH_FALSE;
		return ar5416PerCalibrationN(ah, chan, ahp->ah_rx_chainmask,
		    AH_TRUE, &isCalDone);
	}
}

static HAL_BOOL
ar5416GetEepromNoiseFloorThresh(struct ath_hal *ah,
	const HAL_CHANNEL_INTERNAL *chan, int16_t *nft)
{
	switch (chan->channelFlags & CHANNEL_ALL_NOTURBO) {
	case CHANNEL_A:
	case CHANNEL_A_HT20:
	case CHANNEL_A_HT40PLUS:
	case CHANNEL_A_HT40MINUS:
		ath_hal_eepromGet(ah, AR_EEP_NFTHRESH_5, nft);
		break;
	case CHANNEL_B:
	case CHANNEL_G:
	case CHANNEL_G_HT20:
	case CHANNEL_G_HT40PLUS:
	case CHANNEL_G_HT40MINUS:
		ath_hal_eepromGet(ah, AR_EEP_NFTHRESH_2, nft);
		break;
	default:
		HALDEBUG(ah, HAL_DEBUG_ANY,
		    "%s: invalid channel flags 0x%x\n",
		    __func__, chan->channelFlags);
		return AH_FALSE;
	}
	return AH_TRUE;
}

static void
ar5416StartNFCal(struct ath_hal *ah)
{
	OS_REG_SET_BIT(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_ENABLE_NF);
	OS_REG_SET_BIT(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_NO_UPDATE_NF);
	OS_REG_SET_BIT(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_NF);
}

static void
ar5416LoadNF(struct ath_hal *ah, HAL_CHANNEL_INTERNAL *chan)
{
	static const uint32_t ar5416_cca_regs[] = {
		AR_PHY_CCA,
		AR_PHY_CH1_CCA,
		AR_PHY_CH2_CCA,
		AR_PHY_EXT_CCA,
		AR_PHY_CH1_EXT_CCA,
		AR_PHY_CH2_EXT_CCA
	};
	struct ar5212NfCalHist *h;
	int i;
	int32_t val;
	uint8_t chainmask;

	/*
	 * Force NF calibration for all chains.
	 */
	if (AR_SREV_KITE(ah)) {
		/* Kite has only one chain */
		chainmask = 0x9;
	} else if (AR_SREV_MERLIN(ah)) {
		/* Merlin has only two chains */
		chainmask = 0x1B;
	} else {
		chainmask = 0x3F;
	}

	/*
	 * Write filtered NF values into maxCCApwr register parameter
	 * so we can load below.
	 */
	h = AH5416(ah)->ah_cal.nfCalHist;
	for (i = 0; i < AR5416_NUM_NF_READINGS; i ++)
		if (chainmask & (1 << i)) { 
			val = OS_REG_READ(ah, ar5416_cca_regs[i]);
			val &= 0xFFFFFE00;
			val |= (((uint32_t)(h[i].privNF) << 1) & 0x1ff);
			OS_REG_WRITE(ah, ar5416_cca_regs[i], val);
		}

	/* Load software filtered NF value into baseband internal minCCApwr variable. */
	OS_REG_CLR_BIT(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_ENABLE_NF);
	OS_REG_CLR_BIT(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_NO_UPDATE_NF);
	OS_REG_SET_BIT(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_NF);

	/* Wait for load to complete, should be fast, a few 10s of us. */
	if (! ar5212WaitNFCalComplete(ah, 10000)) {
		/*
		 * We timed out waiting for the noisefloor to load, probably due
		 * to an in-progress rx. Simply return here and allow the load
		 * plenty of time to complete before the next calibration
		 * interval. We need to avoid trying to load -50 (which happens
		 * below) while the previous load is still in progress as this
		 * can cause rx deafness. Instead by returning here, the
		 * baseband nf cal will just be capped by our present
		 * noisefloor until the next calibration timer.
		 */
		HALDEBUG(ah, HAL_DEBUG_ANY, "Timeout while waiting for nf "
		    "to load: AR_PHY_AGC_CONTROL=0x%x\n",
		    OS_REG_READ(ah, AR_PHY_AGC_CONTROL));
		return;
	}

	/*
	 * Restore maxCCAPower register parameter again so that we're not capped
	 * by the median we just loaded.  This will be initial (and max) value
	 * of next noise floor calibration the baseband does.  
	 */
	for (i = 0; i < AR5416_NUM_NF_READINGS; i ++)
		if (chainmask & (1 << i)) {	
			val = OS_REG_READ(ah, ar5416_cca_regs[i]);
			val &= 0xFFFFFE00;
			val |= (((uint32_t)(-50) << 1) & 0x1ff);
			OS_REG_WRITE(ah, ar5416_cca_regs[i], val);
		}
}

void
ar5416InitNfHistBuff(struct ar5212NfCalHist *h)
{
	int i, j;

	for (i = 0; i < AR5416_NUM_NF_READINGS; i ++) {
		h[i].currIndex = 0;
		h[i].privNF = AR5416_CCA_MAX_GOOD_VALUE;
		h[i].invalidNFcount = AR512_NF_CAL_HIST_MAX;
		for (j = 0; j < AR512_NF_CAL_HIST_MAX; j ++)
			h[i].nfCalBuffer[j] = AR5416_CCA_MAX_GOOD_VALUE;
	}
}

/*
 * Update the noise floor buffer as a ring buffer
 */
static void
ar5416UpdateNFHistBuff(struct ar5212NfCalHist *h, int16_t *nfarray)
{
	int i;

	for (i = 0; i < AR5416_NUM_NF_READINGS; i ++) {
		h[i].nfCalBuffer[h[i].currIndex] = nfarray[i];

		if (++h[i].currIndex >= AR512_NF_CAL_HIST_MAX)
			h[i].currIndex = 0;
		if (h[i].invalidNFcount > 0) {
			if (nfarray[i] < AR5416_CCA_MIN_BAD_VALUE ||
			    nfarray[i] > AR5416_CCA_MAX_HIGH_VALUE) {
				h[i].invalidNFcount = AR512_NF_CAL_HIST_MAX;
			} else {
				h[i].invalidNFcount--;
				h[i].privNF = nfarray[i];
			}
		} else {
			h[i].privNF = ar5212GetNfHistMid(h[i].nfCalBuffer);
		}
	}
}   

/*
 * Read the NF and check it against the noise floor threshhold
 */
static int16_t
ar5416GetNf(struct ath_hal *ah, HAL_CHANNEL_INTERNAL *chan)
{
	int16_t nf, nfThresh;

	if (OS_REG_READ(ah, AR_PHY_AGC_CONTROL) & AR_PHY_AGC_CONTROL_NF) {
		HALDEBUG(ah, HAL_DEBUG_ANY,
		    "%s: NF didn't complete in calibration window\n", __func__);
		nf = 0;
	} else {
		/* Finished NF cal, check against threshold */
		int16_t nfarray[NUM_NOISEFLOOR_READINGS] = { 0 };
			
		/* TODO - enhance for multiple chains and ext ch */
		ath_hal_getNoiseFloor(ah, nfarray);
		nf = nfarray[0];
		if (ar5416GetEepromNoiseFloorThresh(ah, chan, &nfThresh)) {
			if (nf > nfThresh) {
				HALDEBUG(ah, HAL_DEBUG_ANY,
				    "%s: noise floor failed detected; "
				    "detected %d, threshold %d\n", __func__,
				    nf, nfThresh);
				/*
				 * NB: Don't discriminate 2.4 vs 5Ghz, if this
				 *     happens it indicates a problem regardless
				 *     of the band.
				 */
				chan->channelFlags |= CHANNEL_CW_INT;
				nf = 0;
			}
		} else {
			nf = 0;
		}
		ar5416UpdateNFHistBuff(AH5416(ah)->ah_cal.nfCalHist, nfarray);
		chan->rawNoiseFloor = nf;
	}
	return nf;
}
