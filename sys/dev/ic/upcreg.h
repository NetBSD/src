/* $NetBSD: upcreg.h,v 1.1 2000/08/08 22:14:53 bjh21 Exp $ */

/*
 * Ben Harris, 2000
 *
 * This file is in the public domain.
 */

/*
 * upcreg.h - CHIPS and Technologies Universal Peripheral Controllers
 */

/*
 * This file contains register details for:
 * CHIPS 82C710 Universal Peripheral Controller
 * CHIPS 82C711 Universal Peripheral Controller II
 * CHIPS 82C721 Universal Peripheral Controller III
 */

/* Fixed port addresses */

#define UPC_PORT_CFGADDR	0x3f0 /* Configuration register address */
#define UPC_PORT_CFGDATA	0x3f1 /* Configuration register value */
#define UPC_PORT_IDECMDBASE    	0x1f0 /* IDE primary base */
#define UPC_PORT_IDECTLBASE    	0x3f6 /* IDE secondary base */
#define UPC_PORT_FDCBASE	0x3f4 /* FDC base address (82C721 only) */
#define UPC_PORT_GAME		0x201 /* -GAMECS active */

/* Configuration magic sequences */

#define UPC_CFGMAGIC_ENTER	0x55 /* Write twice to enter config mode. */
#define UPC_CFGMAGIC_EXIT	0xaa /* Write once to exit config mode. */

/* Configuration registers */
#define UPC_CFGADDR_CR0		0x00 /* Configuration Register 0 */
#define UPC_CFGADDR_CR1		0x01 /* Configuration Register 1 */
#define UPC_CFGADDR_CR2		0x02 /* Configuration Register 2 */
#define UPC_CFGADDR_CR3		0x03 /* Configuration Register 3 */
#define UPC_CFGADDR_CR4		0x04 /* Configuration Register 4 */

/* Configuration register 0 */
#define UPC_CR0_VALID		0x80 /* Device has been configured */
#define UPC_CR0_OSC_MASK	0x60 /* Oscillator control */
#define UPC_CR0_OSC_ON		0x00 /* Oscillator always on */
#define UPC_CR0_OSC_PWRGD	0x20 /* Oscillator on when PWRGD */
#define UPC_CR0_OSC_OFF		0x60 /* Oscillator always off */
#define UPC_CR0_FDC_ENABLE	0x10 /* FDC enabled */
#define UPC_CR0_FDC_ON		0x08 /* FDC powered */
#define UPC_CR0_IDE_AT		0x02 /* IDE controller is AT type */
#define UPC_CR0_IDE_ENABLE	0x01 /* IDE controller enabled */

/* Configuration register 1 */
#define UPC_CR1_READ_ENABLE	0x80 /* Enable reading of config regs */
#define UPC_CR1_COM34_MASK	0x60 /* COM3/COM4 addresses */
#define UPC_CR1_COM34_338_238	0x00 /* COM3 = 0x338; COM4 = 0x238 */
#define UPC_CR1_COM34_3E8_2E8	0x20 /* COM3 = 0x3E8; COM4 = 0x2E8 */
#define UPC_CR1_COM34_2E8_2E0	0x40 /* COM3 = 0x2E8; COM4 = 0x2E0 */
#define UPC_CR1_COM34_220_228	0x60 /* COM3 = 0x220; COM4 = 0x228 */
#define UPC_CR1_IRQ_ACTHIGH	0x10 /* IRQ is active-high */
#define UPC_CR1_LPT_BORING	0x08 /* Parallel port is not EPP */
#define UPC_CR1_LPT_ON		0x04 /* Parallel port is powered */
#define UPC_CR1_LPT_MASK	0x03 /* Parallel port address */
#define UPC_CR1_LPT_DISABLE	0x00 /* Parallel port disabled */
#define UPC_CR1_LPT_3BC		0x01 /* Parallel port at 0x3BC */
#define UPC_CR1_LPT_378		0x02 /* Parallel port at 0x378 */
#define UPC_CR1_LPT_278		0x03 /* Parallel port at 0x278 */

/* Configuration register 2 */
/* I believe 2ndary serial is absent on 82C710 */
#define UPC_CR2_UART2_ON	0x80 /* 2ndary serial powered */
#define UPC_CR2_UART2_ENABLE	0x40 /* 2ndary serial enabled */
#define UPC_CR2_UART2_MASK	0x30 /* 2ndary serial address */
#define UPC_CR2_UART2_3F8	0x00 /* 2ndary serial at 0x3F8 */
#define UPC_CR2_UART2_2F8	0x10 /* 2ndary serial at 0x2F8 */
#define UPC_CR2_UART2_COM3	0x20 /* 2ndary serial at COM3 (see CR1) */
#define UPC_CR2_UART2_COM4	0x30 /* 2ndary serial at COM4 (see CR1) */
#define UPC_CR2_UART1_ON	0x80 /* primary serial powered */
#define UPC_CR2_UART1_ENABLE	0x40 /* primary serial enabled */
#define UPC_CR2_UART1_MASK	0x30 /* primary serial address */
#define UPC_CR2_UART1_3F8	0x00 /* primary serial at 0x3F8 */
#define UPC_CR2_UART1_2F8	0x10 /* primary serial at 0x2F8 */
#define UPC_CR2_UART1_COM3	0x20 /* primary serial at COM3 (see CR1) */
#define UPC_CR2_UART1_COM4	0x30 /* primary serial at COM4 (see CR1) */

/* Configuration register 3 */
#define UPC_CR3_UART2_TEST	0x80 /* 2ndary serial test mode */
#define UPC_CR3_UART1_TEST	0x40 /* primary serial test mode */
#define UPC_CR3_FDC_TEST_MASK	0x30 /* FDC test modes */
#define UPC_CR3_FDC_TEST_NORMAL	0x00 /* FDC normal mode */

/* Configuration register 4 (82C721 only) */
#define UPC_CR4_UART2_DIV13	0x01 /* Use normal (cf MIDI) clock for UART2 */
