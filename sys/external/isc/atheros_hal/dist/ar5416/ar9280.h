/*
 * Copyright (c) 2008-2009 Sam Leffler, Errno Consulting
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
 * $FreeBSD$
 */
#ifndef _ATH_AR9280_H_
#define _ATH_AR9280_H_

#include "ar5416/ar5416.h"

struct ath_hal_9280 {
	struct ath_hal_5416 ah_5416;

	HAL_INI_ARRAY	ah_ini_xmodes;
	HAL_INI_ARRAY	ah_ini_rxgain;
	HAL_INI_ARRAY	ah_ini_txgain;
};
#define	AH9280(_ah)	((struct ath_hal_9280 *)(_ah))

#define	AR9280_DEFAULT_RXCHAINMASK	3
#define	AR9280_DEFAULT_TXCHAINMASK	1

HAL_BOOL ar9280RfAttach(struct ath_hal *, HAL_STATUS *);

struct ath_hal;

HAL_BOOL	ar9280SetAntennaSwitch(struct ath_hal *, HAL_ANT_SETTING);
void		ar9280SpurMitigate(struct ath_hal *ah,
	        	HAL_CHANNEL_INTERNAL *chan);

#endif	/* _ATH_AR9280_H_ */
