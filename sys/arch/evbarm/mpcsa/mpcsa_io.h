/* $Id: mpcsa_io.h,v 1.1.22.1 2008/07/03 18:37:52 simonb Exp $ */

#ifndef	_mpcsa_io_h_
#define	_mpcsa_io_h_	1

/* port A pins (bit numbers): */
#define	PA_RTSD		29U
#define	PA_CTSD		28U
#define	PA_GSMOFF	27U
#define	PA_SCL		26U
#define	PA_SDA		25U
#define	PA_GSMON	24U
#define	PA_EXP1		9U
#define	PA_EXP0		8U
#define	PA_TXD4		5U
#define	PA_SPICS1	4U
#define	PA_SPICS0	3U
#define	PA_SPCK		2U
#define	PA_MOSI		1U
#define	PA_MISO		0U

/* port B pins: */
#define	PB_CTS4		29U
#define	PB_CTS3		28U
#define	PB_CTS2		27U
#define	PB_CTS1		26U
#define	PB_RTS4		25U
#define	PB_RTS3		24U
#define	PB_RTS2		23U
#define	PB_RTS1		22U
#define	PB_DIN4		14U
#define	PB_DIN3		13U
#define	PB_DIN2		12U
#define	PB_DIN1		11U
#define	PB_RXD6		10U
#define	PB_TXE6		9U
#define	PB_TXE5		7U
#define	PB_RXD5		6U
#define	PB_S_RF		5U
#define	PB_S_RK		4U
#define	PB_S_RD		3U
#define	PB_S_TD		2U
#define	PB_S_TK		1U
#define	PB_S_TF		0U

/* port C pins: */
#define	PC_CFRESET	5U
#define	PC_CFCD		4U
#define	PC_CFIRQ	3U
#define	PC_DSRD		1U
#define	PC_DTRD		0U

/* port D pins: */
#define	PD_DSR4		27U
#define	PD_DSR3		26U
#define	PD_DSR2		25U
#define	PD_DSR1		24U
#define	PD_DTR4		23U
#define	PD_DTR3		22U
#define	PD_DTR2		21U
#define	PD_DTR1		20U
#define	PD_SPICS2	19U
#define	PD_DCD4		18U
#define	PD_K702		17U
#define	PD_K701		16U
#define	PD_SW1		15U
#define	PD_SW2		14U
#define	PD_SW3		13U
#define	PD_SW4		12U
#define	PD_RESET_OUT	6U

/* Leds behind SPI: */
#define	PSPI_ELED43	11U
#define	PSPI_ELED33	10U
#define	PSPI_ELED23	9U
#define	PSPI_ELED13	8U
#define	PSPI_RLED1	7U
#define	PSPI_GLED2	6U
#define	PSPI_GLED1	5U
#define	PSPI_SLED5	4U
#define	PSPI_SLED4	3U
#define	PSPI_SLED3	2U
#define	PSPI_SLED2	1U
#define	PSPI_SLED1	0U


/* led numbers: */
enum {
  LED_SER1 = 1,
  LED_SER2,
  LED_SER3,
  LED_SER4,
  LED_SER5,
  LED_SER_MAX = LED_SER5,
  LED_GSM,
  LED_GSM_LINK,
  LED_HB,
  LED_ETH1,
  LED_ETH2,
  LED_ETH3,
  LED_ETH4,
  LED_MAX
};

#define	NUM_ETH_PORTS	4	// amount of ethernet ports

#endif /* _mpcsa_io_h_ */
