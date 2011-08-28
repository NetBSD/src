#	$NetBSD: Makefile,v 1.4 2011/08/28 15:48:19 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/isc/atheros_hal/dist
.PATH:	${S}/external/isc/atheros_hal/dist/ar5210
.PATH:	${S}/external/isc/atheros_hal/dist/ar5211
.PATH:	${S}/external/isc/atheros_hal/dist/ar5212
.PATH:	${S}/external/isc/atheros_hal/dist/ar5312
.PATH:	${S}/external/isc/atheros_hal/dist/ar5312
.PATH:	${S}/external/isc/atheros_hal/dist/ar5416
.PATH:	${S}/external/isc/atheros_hal/ic

KMOD=	ath_hal

SRCS=	ah.c \
	ah_eeprom_v1.c \
	ah_eeprom_v3.c \
	ah_eeprom_v14.c \
	ah_eeprom_v4k.c \
	ah_regdomain.c
SRCS+=	ah_osdep.c
SRCS+=	ar5210_attach.c \
	ar5210_beacon.c \
	ar5210_interrupts.c \
	ar5210_keycache.c \
	ar5210_misc.c \
	ar5210_phy.c \
	ar5210_power.c \
	ar5210_recv.c \
	ar5210_reset.c \
	ar5210_xmit.c
SRCS+=	ar5211_attach.c \
	ar5211_beacon.c \
	ar5211_interrupts.c \
	ar5211_keycache.c \
	ar5211_misc.c \
	ar5211_phy.c \
	ar5211_power.c \
	ar5211_recv.c \
	ar5211_reset.c \
	ar5211_xmit.c
SRCS+=	ar2316.c \
	ar2317.c \
	ar2413.c \
	ar2425.c \
	ar5111.c \
	ar5112.c \
	ar5413.c
SRCS+=	ar5212_ani.c \
	ar5212_attach.c \
	ar5212_beacon.c \
	ar5212_eeprom.c \
	ar5212_gpio.c \
	ar5212_interrupts.c \
	ar5212_keycache.c \
	ar5212_misc.c \
	ar5212_phy.c \
	ar5212_power.c \
	ar5212_recv.c \
	ar5212_reset.c \
	ar5212_rfgain.c \
	ar5212_xmit.c
SRCS+=	ar5312_attach.c \
	ar5312_eeprom.c \
	ar5312_gpio.c \
	ar5312_interrupts.c \
	ar5312_misc.c \
	ar5312_power.c \
	ar5312_reset.c \
	ar5315_gpio.c
SRCS+=	ar2133.c \
	ar5416_ani.c \
	ar5416_attach.c \
	ar5416_beacon.c \
	ar5416_cal.c \
	ar5416_cal_adcdc.c \
	ar5416_cal_adcgain.c \
	ar5416_cal_iq.c \
	ar5416_eeprom.c \
	ar5416_gpio.c \
	ar5416_interrupts.c \
	ar5416_keycache.c \
	ar5416_misc.c \
	ar5416_phy.c \
	ar5416_power.c \
	ar5416_recv.c \
	ar5416_reset.c \
	ar5416_xmit.c \
	ar9160_attach.c \
	ar9280.c \
	ar9280_attach.c \
	ar9285.c \
	ar9285_attach.c \
	ar9285_reset.c

.include "Makefile.inc"

.include <bsd.kmodule.mk>
KMODSCRIPT=	${S}/modules/ath_hal/ath_hal.ldscript
