/*	$NetBSD: ralink_gpio.c,v 1.3.12.3 2017/12/03 11:36:28 jdolecek Exp $	*/
/*-
 * Copyright (c) 2011 CradlePoint Technology, Inc.
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY CRADLEPOINT TECHNOLOGY, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* ra_gpio.c -- Ralink 3052 gpio driver */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ralink_gpio.c,v 1.3.12.3 2017/12/03 11:36:28 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/event.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <dev/cons.h>

#include <mips/cpuregs.h>
#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>

#include <mips/ralink/ralink_reg.h>
#include <mips/ralink/ralink_var.h>

#if !defined(MT7628)
#define SLICKROCK
#endif
#include <mips/ralink/ralink_gpio.h>

#if 0
#define ENABLE_RALINK_DEBUG_ERROR 1
#define ENABLE_RALINK_DEBUG_MISC  1
#define ENABLE_RALINK_DEBUG_INFO  1
#define ENABLE_RALINK_DEBUG_FORCE 1
#define ENABLE_RALINK_DEBUG_FUNC 1
#endif

#include <mips/ralink/ralink_debug.h>

/*
 * From the RT3052 datasheet, the GPIO pins are
 * shared with a number of other functions.  Not
 * all will be available for GPIO.  To make sure
 * pins are in GPIO mode, the GPIOMODE purpose
 * select register must be set (reset defaults all pins
 * as GPIO except for JTAG)
 *
 * Note, GPIO0 is not shared, it is the single dedicated
 * GPIO pin.
 *
 * 52 pins:
 *
 *	40 - 51 RGMII  (GE0_* pins)
 *	24 - 39 SDRAM  (MD31:MD16 pins)
 *	22 - 23 MDIO   (MDC/MDIO pins)
 *	17 - 21 JTAG   (JTAG_* pins)
 *	15 - 16 UART Light  (RXD2/TXD2 pins)
 *	7  - 14 UART Full   (8 pins)
 *	(7 - 14)  UARTF_7 (3'b111 in Share mode reg)
 *	(11 - 14) UARTF_5 (3'b110 in Share mode reg, same with 6)
 *	(7 - 9)   UARTF_4 (3'b100 in Share mode reg)
 *	3  -  6 SPI    (SPI_* pins)
 *	1  -  2 I2C    (I2C_SCLK/I2C_SD pins)
 */

#ifdef MT7628
#define GPIO_PINS 96
#define SPECIAL_COMMANDS	0
#else
#if defined(SLICKROCK)
#define GPIO_PINS 96
#else
#define GPIO_PINS 52
#endif

/*
 * Special commands are pseudo pin numbers used to pass data to bootloader,
 */
#define BOOT_COUNT		GPIO_PINS
#define UPGRADE			(BOOT_COUNT + 1)
#define SPECIAL_COMMANDS	(UPGRADE + 1 - GPIO_PINS)
#endif


/*
 * The pin_share array maps to the highest pin used for each of the 10
 * GPIO mode bit settings.
 */
#if defined(MT7628)
const static struct {
	int pin_start;
	int pin_end;
	uint32_t sysreg;
	uint32_t regmask;
	uint32_t mode;
} gpio_mux_map[] = {
	{ 0,	3,	RA_SYSCTL_GPIO1MODE,	GPIO1MODE_I2S,		1 },
	{ 4,	5,	RA_SYSCTL_GPIO1MODE,	GPIO1MODE_I2C,		1 },
	{ 6,	6,	RA_SYSCTL_GPIO1MODE,	GPIO1MODE_SPI_CS1,	1 },
	{ 7,	10,	RA_SYSCTL_GPIO1MODE,	GPIO1MODE_SPI,		1 },
	{ 11,	11,	RA_SYSCTL_GPIO1MODE,	GPIO1MODE_GPIO,		1 },
	{ 12,	13,	RA_SYSCTL_GPIO1MODE,	GPIO1MODE_UART0,	1 },
	{ 14,	17,	RA_SYSCTL_GPIO1MODE,	GPIO1MODE_SPIS,		1 },
	{ 18,	18,	RA_SYSCTL_GPIO1MODE,	GPIO1MODE_PWM0,		1 },
	{ 19,	19,	RA_SYSCTL_GPIO1MODE,	GPIO1MODE_PWM1,		1 },
	{ 20,	21,	RA_SYSCTL_GPIO1MODE,	GPIO1MODE_UART2,	1 },
	{ 22,	29,	RA_SYSCTL_GPIO1MODE,	GPIO1MODE_SD,		1 },
	{ 30,	30,	RA_SYSCTL_GPIO2MODE,	GPIO2MODE_P4_LED_KN,	1 },
	{ 31,	31,	RA_SYSCTL_GPIO2MODE,	GPIO2MODE_P3_LED_KN,	1 },
	{ 32,	32,	RA_SYSCTL_GPIO2MODE,	GPIO2MODE_P2_LED_KN,	1 },
	{ 33,	33,	RA_SYSCTL_GPIO2MODE,	GPIO2MODE_P1_LED_KN,	1 },
	{ 34,	34,	RA_SYSCTL_GPIO2MODE,	GPIO2MODE_P0_LED_KN,	1 },
	{ 35,	35,	RA_SYSCTL_GPIO2MODE,	GPIO2MODE_WLED_KN,	1 },
	{ 36,	36,	RA_SYSCTL_GPIO1MODE,	GPIO1MODE_PERST,	1 },
	{ 37,	37,	RA_SYSCTL_GPIO1MODE,	GPIO1MODE_REFCLK,	1 },
	{ 38,	38,	RA_SYSCTL_GPIO1MODE,	GPIO1MODE_WDT,		1 },
	{ 39,	39,	RA_SYSCTL_GPIO2MODE,	GPIO2MODE_P4_LED_AN,	1 },
	{ 40,	40,	RA_SYSCTL_GPIO2MODE,	GPIO2MODE_P3_LED_AN,	1 },
	{ 41,	41,	RA_SYSCTL_GPIO2MODE,	GPIO2MODE_P2_LED_AN,	1 },
	{ 42,	42,	RA_SYSCTL_GPIO2MODE,	GPIO2MODE_P1_LED_AN,	1 },
	{ 43,	43,	RA_SYSCTL_GPIO2MODE,	GPIO2MODE_P0_LED_AN,	1 },
	{ 44,	44,	RA_SYSCTL_GPIO2MODE,	GPIO2MODE_WLED_AN,	1 },
	{ 45,	45,	RA_SYSCTL_GPIO1MODE,	GPIO1MODE_UART1,	1 },
	{ 46,	46,	RA_SYSCTL_GPIO1MODE,	GPIO1MODE_UART1,	1 },
};
#else
#if defined(SLICKROCK)
#define SR_GPIO_MODE 0xc1c1f
#else
const static u_int8_t pin_share[] = {
	2, 6, 9, 14, 14, 16, 21, 23, 39, 51
};
#endif
#endif /* MT7628 */

#define DEBOUNCE_TIME 150  /* Milliseconds */

#if defined(TABLEROCK)  || defined(SPOT2) || defined(PUCK) || defined(MOAB)

static const u_int32_t debounce_pin[DEBOUNCED_PINS] = {
	WPS_BUTTON,
	POWER_OFF_BUTTON,
	DOCK_SENSE,
	IN_5V,
	LAN_WAN_SW
};
static struct timeval debounce_time[DEBOUNCED_PINS];

/* LED defines for display during boot */
static const char led_array1[] = {
	LED_BATT_7,
	LED_BATT_8,
	LED_BATT_9,
	LED_BATT_10,
	LED_SS_11,
	LED_SS_12,
	LED_SS_13,
	LED_SS_14
};

#define SET_SS_LED_REG RA_PIO_24_39_SET_BIT
#define CLEAR_SS_LED_REG RA_PIO_24_39_CLR_BIT
#define SS_OFFSET 24
#define BOOT_LED_TIMING 3000

#endif	/* TABLEROCK || SPOT2 || PUCK || MOAB */

#if defined(PEBBLES500) || defined (PEBBLES35)

static const u_int32_t debounce_pin[DEBOUNCED_PINS] = {
	WPS_BUTTON,
	SOFT_RST_IN_BUTTON,
	CURRENT_LIMIT_FLAG1_3_3v,
	CURRENT_LIMIT_FLAG_USB1,
	CURRENT_LIMIT_FLAG1_1_5v,
	EXCARD_ATTACH
};
static struct timeval debounce_time[DEBOUNCED_PINS];

/* LED defines for display during boot */
#if defined(PEBBLES500)
static const char led_array1[] = {
	LED_SS_13,
	LED_SS_12,
	LED_SS_11,
	LED_SS_10
};
#else
static const char led_array1[] = {};
#endif

#define SET_SS_LED_REG RA_PIO_40_51_SET_BIT
#define CLEAR_SS_LED_REG RA_PIO_40_51_CLR_BIT
#define SS_OFFSET 40
#define BOOT_LED_TIMING 5500

#endif	/* PEBBLES500 || PEBBLES35 */


#if defined(SLICKROCK)

static const u_int32_t debounce_pin[DEBOUNCED_PINS] = {
	WPS_BUTTON,
	SOFT_RST_IN_BUTTON,
	SS_BUTTON,
	CURRENT_LIMIT_FLAG_USB1,
	CURRENT_LIMIT_FLAG_USB2,
	CURRENT_LIMIT_FLAG_USB3,
	CURRENT_LIMIT_FLAG_EX1,
	CURRENT_LIMIT_FLAG_EX2,
	WIFI_ENABLE
};
static struct timeval debounce_time[DEBOUNCED_PINS];

/* LED defines for display during boot */
static const char led_array1[] = {
	LED_SS_13,
	LED_SS_12,
	LED_SS_11,
	LED_SS_10
};

#define SET_SS_LED_REG RA_PIO_40_51_SET_BIT
#define CLEAR_SS_LED_REG RA_PIO_40_51_CLR_BIT
#define SS_OFFSET 40
#define BOOT_LED_TIMING 3250

#endif	/* SLICKROCK */

#ifndef DEBOUNCED_PINS
static const u_int32_t debounce_pin[] = {};
static struct timeval debounce_time[] = {};
#endif
#ifndef BOOT_LED_TIMING
static const char led_array1[] = {};
#endif

#define RA_GPIO_PIN_INIT(sc, var, pin, ptp, regname)			\
	do {								\
		const u_int _reg_bit = 1 << (pin - ptp->pin_reg_base);	\
		const u_int _mask_bit = 1 << (pin - ptp->pin_mask_base);\
		var = gp_read(sc, ptp->regname.reg);			\
		if ((ptp->regname.mask & _mask_bit) != 0) {		\
			var |= _reg_bit;				\
		} else {						\
			var &= ~_reg_bit;				\
		}							\
		gp_write(sc, ptp->regname.reg, var);			\
	} while (0)

#define RA_GPIO_PIN_INIT_DIR(sc, var, pin, ptp)				\
	do {								\
		const u_int _reg_bit = 1 << (pin - ptp->pin_reg_base);	\
		const u_int _mask_bit = 1 << (pin - ptp->pin_mask_base);\
		var = gp_read(sc, ptp->pin_dir.reg);			\
		if ((ptp->pin_dir.mask & _mask_bit) != 0) {		\
			var |= _reg_bit;				\
			sc->sc_pins[pin].pin_flags = GPIO_PIN_OUTPUT;	\
		} else {						\
			var &= ~_reg_bit;				\
			sc->sc_pins[pin].pin_flags = GPIO_PIN_INPUT;	\
		}							\
		gp_write(sc, ptp->pin_dir.reg, var);			\
	} while (0)



typedef struct ra_gpio_softc {
	device_t sc_dev;
	struct gpio_chipset_tag	sc_gc;
	gpio_pin_t sc_pins[GPIO_PINS + SPECIAL_COMMANDS];

	bus_space_tag_t	sc_memt;	/* bus space tag */

	bus_space_handle_t sc_sy_memh;	/* Sysctl bus space handle */
	int sc_sy_size;			/* size of Sysctl register space */

	bus_space_handle_t sc_gp_memh;	/* PIO bus space handle */
	int sc_gp_size;			/* size of PIO register space */

	void *sc_ih;			/* interrupt handle */
	void *sc_si;			/* softintr handle  */

	struct callout sc_tick_callout;	/* For debouncing inputs */

	/*
	 * These track gpio pins that have interrupted
	 */
#if defined(MT7628)
	uint32_t sc_intr_status00_31;
	uint32_t sc_intr_status32_63;
	uint32_t sc_intr_status64_95;
#else
	uint32_t sc_intr_status00_23;
	uint32_t sc_intr_status24_39;
	uint32_t sc_intr_status40_51;
	uint32_t sc_intr_status72_95;
#endif

} ra_gpio_softc_t;

static int ra_gpio_match(device_t, cfdata_t , void *);
static void ra_gpio_attach(device_t, device_t, void *);
#ifdef NOTYET
static int ra_gpio_open(void *, device_t);
static int ra_gpio_close(void *, device_t);
#endif
static void ra_gpio_pin_init(ra_gpio_softc_t *, int);
static void ra_gpio_pin_ctl(void *, int, int);

static int  ra_gpio_intr(void *);
static void ra_gpio_softintr(void *);
static void ra_gpio_debounce_process(void *);
static void ra_gpio_debounce_setup(ra_gpio_softc_t *);

static void disable_gpio_interrupt(ra_gpio_softc_t *, int);
static void enable_gpio_interrupt(ra_gpio_softc_t *, int);

static void gpio_reset_registers(ra_gpio_softc_t *);
#if 0
static void gpio_register_dump(ra_gpio_softc_t *);
#endif

typedef struct pin_reg {
	u_int mask;
	u_int reg;
} pin_reg_t;

typedef struct pin_tab {
	int pin_reg_base;
	int pin_reg_limit;
	int pin_mask_base;
	u_int pin_enabled;
	u_int pin_input;
	u_int pin_output_clr;
	u_int pin_output_set;
	pin_reg_t pin_dir;
	pin_reg_t pin_rise;
	pin_reg_t pin_fall;
	pin_reg_t pin_pol;
} pin_tab_t;

/*
 * use pin_tab[] in conjunction with pin_tab_index[]
 * to look up register offsets, masks, etc. for a given GPIO pin,
 * instead of lots of if/then/else test & branching
 */
static const pin_tab_t pin_tab[] = {
#if defined(MT7628)
    {
	0, 31, 0, 0xffffffff,
	RA_PIO_00_31_DATA,
	RA_PIO_00_31_CLR_BIT,
	RA_PIO_00_31_SET_BIT,
	{ 0xffffffff,		RA_PIO_00_31_DIR		},
	{ 0xffffffff,		RA_PIO_00_31_INT_RISE_EN	},
	{ 0xffffffff,		RA_PIO_00_31_INT_FALL_EN	},
	{ 0xffffffff,		RA_PIO_00_31_POLARITY		}
    },
    {
	32, 63, 32, 0xffffffff,
	RA_PIO_32_63_DATA,
	RA_PIO_32_63_CLR_BIT,
	RA_PIO_32_63_SET_BIT,
	{ 0xffffffff,		RA_PIO_32_63_DIR		},
	{ 0xffffffff,		RA_PIO_32_63_INT_RISE_EN	},
	{ 0xffffffff,		RA_PIO_32_63_INT_FALL_EN	},
	{ 0xffffffff,		RA_PIO_32_63_POLARITY		}
    },
    {
	64, 95, 64, 0xffffffff,
	RA_PIO_64_95_DATA,
	RA_PIO_64_95_CLR_BIT,
	RA_PIO_64_95_SET_BIT,
	{ 0xffffffff,		RA_PIO_64_95_DIR		},
	{ 0xffffffff,		RA_PIO_64_95_INT_RISE_EN	},
	{ 0xffffffff,		RA_PIO_64_95_INT_FALL_EN	},
	{ 0xffffffff,		RA_PIO_64_95_POLARITY		}
    }
#else
    {
	0, 24, 0, GPIO_PIN_MASK,
	RA_PIO_00_23_DATA,
	RA_PIO_00_23_CLR_BIT,
	RA_PIO_00_23_SET_BIT,
	{ GPIO_OUTPUT_PIN_MASK,          RA_PIO_00_23_DIR,         },
	{ GPIO_INT_PIN_MASK,             RA_PIO_00_23_INT_RISE_EN, },
	{ GPIO_INT_FEDGE_PIN_MASK,       RA_PIO_00_23_INT_RISE_EN, },
	{ GPIO_POL_MASK,                 RA_PIO_00_23_POLARITY,    },
    },
    {
	24, 40, 24, GPIO_PIN_MASK_24_51,
	RA_PIO_24_39_DATA,
	RA_PIO_24_39_CLR_BIT,
	RA_PIO_24_39_SET_BIT,
	{ GPIO_OUTPUT_PIN_MASK_24_51,    RA_PIO_24_39_DIR,         },
	{ GPIO_INT_PIN_MASK_24_51,       RA_PIO_24_39_INT_RISE_EN, },
	{ GPIO_INT_FEDGE_PIN_MASK_24_51, RA_PIO_24_39_INT_FALL_EN, },
	{ GPIO_POL_MASK_24_51,           RA_PIO_24_39_POLARITY,    },
    },
    {
	40, 52, 24, GPIO_PIN_MASK_24_51,
	RA_PIO_40_51_DATA,
	RA_PIO_40_51_CLR_BIT,
	RA_PIO_40_51_SET_BIT,
	{ GPIO_OUTPUT_PIN_MASK_24_51,    RA_PIO_40_51_DIR,         },
	{ GPIO_INT_PIN_MASK_24_51,       RA_PIO_40_51_INT_RISE_EN, },
	{ GPIO_INT_FEDGE_PIN_MASK_24_51, RA_PIO_40_51_INT_FALL_EN, },
	{ GPIO_POL_MASK_24_51,           RA_PIO_40_51_POLARITY,    },
    },
#if defined(SLICKROCK)
    {
	72, 96, 72, GPIO_PIN_MASK_72_95,
	RA_PIO_72_95_DATA,
	RA_PIO_72_95_CLR_BIT,
	RA_PIO_72_95_SET_BIT,
	{ GPIO_OUTPUT_PIN_MASK_72_95,    RA_PIO_72_95_DIR,         },
	{ GPIO_INT_PIN_MASK_72_95,       RA_PIO_72_95_INT_RISE_EN, },
	{ GPIO_INT_FEDGE_PIN_MASK_72_95, RA_PIO_72_95_INT_FALL_EN, },
	{ GPIO_POL_MASK_72_95,           RA_PIO_72_95_POLARITY,    },
    },
#endif
#endif
};

/*
 * use pin_tab_index[] to determine index in pin_tab[]
 * for a given pin.  -1 means there is no pin there.
 */
static const int pin_tab_index[GPIO_PINS] = {
#if defined(MT7628)
/*		 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 */
/*  0 */	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 16 */	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 32 */	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 48 */	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 64 */	 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
/* 80 */	 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
#else /* !MT7628 */
/*		 0   1   2   3   4   5   6   7   8   9	*/
/*  0 */	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 10 */	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 20 */	 0,  0,  0,  0,  1,  1,  1,  1,  1,  1,
/* 30 */	 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/* 40 */	 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
/* 50 */	 2,  2,
#if defined(SLICKROCK)
/* 50 */	        -1, -1, -1, -1, -1, -1, -1, -1,
/* 60 */	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
/* 70 */	-1, -1,
/* 72 */	         3,  3,  3,  3,  3,  3,  3,  3,
/* 80 */	 3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
/* 90 */	 3,  3,  3,  3,  3,  3
#endif
#endif /* !MT7628 */
};

CFATTACH_DECL_NEW(rgpio, sizeof(struct ra_gpio_softc), ra_gpio_match,
	ra_gpio_attach, NULL, NULL);

/*
 * Event handler calls and structures
 */
static int  gpio_event_app_user_attach(struct knote *);
static void gpio_event_app_user_detach(struct knote *);
static int  gpio_event_app_user_event(struct knote *, long);

static struct klist knotes;
static int app_filter_id;
static struct filterops app_fops = {
	0,
	gpio_event_app_user_attach,
	gpio_event_app_user_detach,
	gpio_event_app_user_event
};
static struct callout led_tick_callout;
static int gpio_driver_blink_leds = 1;

static inline uint32_t
sy_read(ra_gpio_softc_t *sc, bus_size_t off)
{
	KASSERTMSG((off & 3) == 0, "%s: unaligned off=%#" PRIxBUSSIZE "\n",
	    __func__, off);
	return bus_space_read_4(sc->sc_memt, sc->sc_sy_memh, off);
}

static inline void
sy_write(ra_gpio_softc_t *sc, bus_size_t off, uint32_t val)
{
	KASSERTMSG((off & 3) == 0, "%s: unaligned off=%#" PRIxBUSSIZE "\n",
	    __func__, off);
	bus_space_write_4(sc->sc_memt, sc->sc_sy_memh, off, val);
}

static inline uint32_t
gp_read(ra_gpio_softc_t *sc, bus_size_t off)
{
	KASSERTMSG((off & 3) == 0, "%s: unaligned off=%#" PRIxBUSSIZE "\n",
	    __func__, off);
	return bus_space_read_4(sc->sc_memt, sc->sc_gp_memh, off);
}

static inline void
gp_write(ra_gpio_softc_t *sc, bus_size_t off, uint32_t val)
{
	KASSERTMSG((off & 3) == 0, "%s: unaligned off=%#" PRIxBUSSIZE "\n",
	    __func__, off);
	bus_space_write_4(sc->sc_memt, sc->sc_gp_memh, off, val);
}

/*
 * Basic debug function, dump all PIO registers
 */
#if 0
static void
gpio_register_dump(ra_gpio_softc_t *sc)
{
	for (int i=0; i < 0xb0; i+=4)
		RALINK_DEBUG(RALINK_DEBUG_INFO, "Reg: 0x%x, Value: 0x%x\n",
			(RA_PIO_BASE + i), gp_read(sc, i));
}
#endif

static void
gpio_reset_registers(ra_gpio_softc_t *sc)
{
#if 0
	for (int i=0; i < 0x88; i+=4)
		gp_write(sc, i, 0);
#endif
}


static int
ra_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	RALINK_DEBUG_FUNC_ENTRY();

	return 1;
}

static void
ra_gpio_attach(device_t parent, device_t self, void *aux)
{

	struct gpiobus_attach_args gba;
	ra_gpio_softc_t * const sc = device_private(self);
	const struct mainbus_attach_args *ma = aux;
	int error;

	aprint_naive(": Ralink GPIO controller\n");
	aprint_normal(": Ralink GPIO controller\n");

	sc->sc_dev = self;
	sc->sc_memt = ma->ma_memt;
	sc->sc_sy_size = 0x10000;
	sc->sc_gp_size = 0x1000;

	/*
	 * map the registers
	 *
	 * we map the Sysctl, and PIO registers seperately so we can use the
	 * defined register offsets sanely; just use the correct corresponding
	 * bus_space_handle
	 */

	if ((error = bus_space_map(sc->sc_memt, RA_SYSCTL_BASE,
	    sc->sc_sy_size, 0, &sc->sc_sy_memh)) != 0) {
		aprint_error_dev(sc->sc_dev,
			"unable to map Sysctl registers, error=%d\n", error);
		goto fail_0;
	}

	if ((error = bus_space_map(sc->sc_memt, RA_PIO_BASE,
	    sc->sc_gp_size, 0, &sc->sc_gp_memh)) != 0) {
		aprint_error_dev(sc->sc_dev,
			"unable to map PIO registers, error=%d\n", error);
		goto fail_1;
	}

	/* Reset some registers */
#if defined(MT7628)
	gp_write(sc, RA_PIO_00_31_INT_RISE_EN, 0xffffffff);
	gp_write(sc, RA_PIO_00_31_INT_FALL_EN, 0xffffffff);
	gp_write(sc, RA_PIO_00_31_INT_HIGH_EN, 0);
	gp_write(sc, RA_PIO_00_31_INT_LOW_EN, 0);

	gp_write(sc, RA_PIO_32_63_INT_RISE_EN, 0xffffffff);
	gp_write(sc, RA_PIO_32_63_INT_FALL_EN, 0xffffffff);
	gp_write(sc, RA_PIO_32_63_INT_HIGH_EN, 0);
	gp_write(sc, RA_PIO_32_63_INT_LOW_EN, 0);

	gp_write(sc, RA_PIO_64_95_INT_RISE_EN, 0xffffffff);
	gp_write(sc, RA_PIO_64_95_INT_FALL_EN, 0xffffffff);
	gp_write(sc, RA_PIO_64_95_INT_HIGH_EN, 0);
	gp_write(sc, RA_PIO_64_95_INT_LOW_EN, 0);

	gp_write(sc, RA_PIO_00_31_POLARITY, 0);
	gp_write(sc, RA_PIO_32_63_POLARITY, 0);
	gp_write(sc, RA_PIO_64_95_POLARITY, 0);

#else
	gp_write(sc, RA_PIO_00_23_INT, 0xffffff);
	gp_write(sc, RA_PIO_00_23_EDGE_INT, 0xffffff);
	gp_write(sc, RA_PIO_24_39_INT, 0xffff);
	gp_write(sc, RA_PIO_24_39_EDGE_INT, 0xffff);
	gp_write(sc, RA_PIO_40_51_INT, 0xfff);
	gp_write(sc, RA_PIO_40_51_EDGE_INT, 0xfff);
	gp_write(sc, RA_PIO_00_23_POLARITY, 0);
#if defined(SLICKROCK)
	gp_write(sc, RA_PIO_72_95_INT, 0xffffff);
	gp_write(sc, RA_PIO_72_95_EDGE_INT, 0xffffff);
#endif
#endif

	/* Set up for interrupt handling, low priority interrupt queue */
	sc->sc_ih = ra_intr_establish(RA_IRQ_PIO,
		ra_gpio_intr, sc, 0);
	if (sc->sc_ih == NULL) {
		RALINK_DEBUG(RALINK_DEBUG_ERROR,
			"%s: unable to establish interrupt\n",
			device_xname(sc->sc_dev));
		goto fail_2;
	}

	/* Soft int setup */
	sc->sc_si = softint_establish(SOFTINT_BIO, ra_gpio_softintr, sc);
	if (sc->sc_si == NULL) {
		ra_intr_disestablish(sc->sc_ih);
		RALINK_DEBUG(RALINK_DEBUG_ERROR,
			"%s: unable to establish soft interrupt\n",
			device_xname(sc->sc_dev));
		goto fail_3;
	}

	SLIST_INIT(&knotes);
	if (kfilter_register("CP_GPIO_EVENT", &app_fops, &app_filter_id) != 0) {
		RALINK_DEBUG(RALINK_DEBUG_ERROR,
			"%s: kfilter_register for CP_GPIO_EVENT failed\n",
			__func__);
		goto fail_4;
	}

	sc->sc_gc.gp_cookie = sc;
	sc->sc_gc.gp_pin_read = ra_gpio_pin_read;
	sc->sc_gc.gp_pin_write = ra_gpio_pin_write;
	sc->sc_gc.gp_pin_ctl = ra_gpio_pin_ctl;

#if 0
	gpio_register_dump(sc);
#endif
	gpio_reset_registers(sc);

	/* Initialize the GPIO pins */
	for (int pin = 0; pin < GPIO_PINS; pin++)
		ra_gpio_pin_init(sc, pin);

#if 0
	/* debug check */
	KASSERT((sy_read(sc, RA_SYSCTL_GPIOMODE) == 0x31c) != 0);
	RALINK_DEBUG(RALINK_DEBUG_INFO, "SYSCTL_GPIOMODE = 0x%x\n",
		sy_read(sc, RA_SYSCTL_GPIOMODE));
#endif

	/*
	 * Some simple board setup actions:
	 * Check if we're attached to the dock. If so, enable dock power.
	 *  BIG NOTE: Dock power is dependent on USB5V_EN!
	 */
#if defined(PEBBLES500) || defined (PEBBLES35)
	RALINK_DEBUG(RALINK_DEBUG_INFO, "Enabling USB power\n");
	ra_gpio_pin_write(sc, VBUS_EN, 1);
	ra_gpio_pin_write(sc, POWER_EN_USB, 1);
#if defined(PEBBLES500)
	/*
	 * Is an express card attached? Enable power if it is
	 *  or it isn't
	 */
#if 0
	if (ra_gpio_pin_read(sc, EXCARD_ATTACH) == 0) {
#endif
		ra_gpio_pin_write(sc, POWER_EN_EXCARD1_3_3v, 1);
		ra_gpio_pin_write(sc, POWER_EN_EXCARD1_1_5v, 1);
#if 0
	}
#endif	/* 0 */
#endif	/* PEBBLES500 */
#endif	/* PEBBLES500 || PEBBLES35 */

#if defined(TABLEROCK) || defined(SPOT2) || defined(PUCK) || defined(MOAB)
	/* CHARGER_OFF pin matches the IN_5V */
	if (ra_gpio_pin_read(sc, IN_5V) == 0) {
		ra_gpio_pin_write(sc, CHARGER_OFF, 0);
	} else {
		ra_gpio_pin_write(sc, CHARGER_OFF, 1);
	}
#endif

#if defined(SLICKROCK)
	/* Enable all modem slots */
	ra_gpio_pin_write(sc, POWER_EN_USB1, 1);
	ra_gpio_pin_write(sc, POWER_EN_USB2, 1);
	ra_gpio_pin_write(sc, POWER_EN_USB3, 1);
	ra_gpio_pin_write(sc, POWER_EN_EX1, 1);
	ra_gpio_pin_write(sc, EX1_CPUSB_RST, 0);
	ra_gpio_pin_write(sc, POWER_EN_EX2, 1);
	ra_gpio_pin_write(sc, EX2_CPUSB_RST, 0);

	/* Wake up with an overcurrent on EX1. Try to shut it down. */
	gp_write(sc, RA_PIO_72_95_INT, 0xffffff);
	gp_write(sc, RA_PIO_72_95_EDGE_INT, 0xffffff);
#endif

#ifdef BOOT_COUNT
	sc->sc_pins[BOOT_COUNT].pin_flags = GPIO_PIN_OUTPUT;
	sc->sc_pins[BOOT_COUNT].pin_mapped = 0;
#endif
#ifdef UPGRADE
	sc->sc_pins[UPGRADE].pin_flags = GPIO_PIN_OUTPUT;
	sc->sc_pins[UPGRADE].pin_mapped = 0;
#endif
	gba.gba_gc = &sc->sc_gc;
	gba.gba_pins = sc->sc_pins;

	/* Note, > 52nd pin isn't a gpio, it is a special command */
	gba.gba_npins = (GPIO_PINS + SPECIAL_COMMANDS);

	config_found_ia(sc->sc_dev, "gpiobus", &gba, gpiobus_print);

#if 0
	gpio_register_dump(sc);
#endif

	/* init our gpio debounce */
	callout_init(&sc->sc_tick_callout, 0);
	callout_setfunc(&sc->sc_tick_callout, ra_gpio_debounce_process, sc);

	/* LED blinking during boot */
	callout_init(&led_tick_callout, 0);

	ra_gpio_toggle_LED(sc);
	return;

 fail_4:
	softint_disestablish(sc->sc_si);
 fail_3:
	ra_intr_disestablish(sc->sc_ih);
 fail_2:
	bus_space_unmap(sc->sc_memt, sc->sc_gp_memh, sc->sc_sy_size);
 fail_1:
	bus_space_unmap(sc->sc_memt, sc->sc_sy_memh, sc->sc_sy_size);
 fail_0:
	return;
}

/*
 * ra_gpio_pin_init - initialize the given gpio pin
 */
static void
ra_gpio_pin_init(ra_gpio_softc_t *sc, int pin)
{
	uint32_t r;

	sc->sc_pins[pin].pin_caps = 0;
	sc->sc_pins[pin].pin_flags = 0;
	sc->sc_pins[pin].pin_state = 0;
	sc->sc_pins[pin].pin_mapped = 0;

	/* ensure pin number is in range */
	KASSERT(pin < GPIO_PINS);
	if (pin >= GPIO_PINS)
		return;

	/* if pin number is in a gap in the range, just return */
	const int index = pin_tab_index[pin];
	if (index == -1)
		return;

	/* if pin is not enabled, just return */
	const pin_tab_t * const ptp = &pin_tab[index];
	const u_int mask_bit = 1 << (pin - ptp->pin_mask_base);
	if ((ptp->pin_enabled & mask_bit) == 0)
		return;

	sc->sc_pins[pin].pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
	    GPIO_PIN_INVIN | GPIO_PIN_INVOUT;
	sc->sc_pins[pin].pin_state = GPIO_PIN_INPUT;

#if defined(MT7628)
	/*
	 * Set the SYSCTL_GPIO{1,2}MODE register
	 * for the PIO block of any mapped GPIO
	 */
	for (int i = 0; i < __arraycount(gpio_mux_map); i++) {
		if ((pin >= gpio_mux_map[i].pin_start) &&
		    (pin >= gpio_mux_map[i].pin_end)) {
			r = sy_read(sc, gpio_mux_map[i].sysreg);
			r &= ~gpio_mux_map[i].regmask;
			r |= __SHIFTIN(gpio_mux_map[i].mode,
			    gpio_mux_map[i].regmask);
			sy_write(sc, gpio_mux_map[i].sysreg, r);
			break;
		}
	}
#else
#if defined(SLICKROCK)
	r = sy_read(sc, RA_SYSCTL_GPIOMODE);
	r |= SR_GPIO_MODE;
	sy_write(sc, RA_SYSCTL_GPIOMODE, r);
#else
	/*
	 * Set the SYSCTL_GPIOMODE register to 1 for
	 * the PIO block of any mapped GPIO
	 * (most have reset defaults of 1 already).
	 * GPIO0 doesn't have an associated MODE register.
	 */
	if (pin != 0) {
		u_int gpio_mode;

		for (gpio_mode = 0; gpio_mode < __arraycount(pin_share);
		    gpio_mode++) {
			if (pin <= pin_share[gpio_mode]) {
				r = sy_read(sc, RA_SYSCTL_GPIOMODE);
				if (10 == pin) {
					/*
					 * Special case:
					 *   GPIO 10 requires GPIOMODE_UARTF0-2
					 */
					r |= GPIOMODE_UARTF_0_2;
				} else {
					/* standard case */
					r |= (1 << gpio_mode);
				}
				sy_write(sc, RA_SYSCTL_GPIOMODE, r);
				break;
			}
		}
	}
#endif /* SLICKROCK */
#endif /* !MT7628 */

	/* set direction */
	RA_GPIO_PIN_INIT_DIR(sc, r, pin, ptp);

	/* rising edge interrupt */
	RA_GPIO_PIN_INIT(sc, r, pin, ptp, pin_rise);

	/* falling edge interrupt */
	RA_GPIO_PIN_INIT(sc, r, pin, ptp, pin_fall);

	/* polarirty */
	RA_GPIO_PIN_INIT(sc, r, pin, ptp, pin_pol);
}

/*
 * Note: This has special hacks in it. If pseudo-pin BOOT_COUNT
 * is requested, it is a signal check the memo register for a special key
 * that means run Wi-Fi in no security mode.
 */
int
ra_gpio_pin_read(void *arg, int pin)
{
	RALINK_DEBUG_FUNC_ENTRY();
	const ra_gpio_softc_t * const sc = arg;
	int rv;

	KASSERT(sc != NULL);

	if (pin < GPIO_PINS) {
		/*
		 * normal case: a regular GPIO pin
		 * if pin number is in a gap in the range,
		 * then warn and return 0
		 */
		const int index = pin_tab_index[pin];
		KASSERTMSG(index != -1, "%s: non-existant pin=%d\n",
			__func__, pin);
		if (index == -1) {
			rv = 0;
		} else {
			const pin_tab_t * const ptp = &pin_tab[index];
			const uint32_t reg_bit = 1 << (pin - ptp->pin_reg_base);
			const bus_size_t off = ptp->pin_input;
			uint32_t r;

			r = bus_space_read_4(sc->sc_memt, sc->sc_gp_memh, off);
			rv = ((r & (1 << reg_bit)) != 0);
		}
	} else {
		/*
		 * Special hack: a pseudo-pin used for signaling
		 */
		rv = 0;
		switch (pin) {
#ifdef BOOT_COUNT
		case BOOT_COUNT:
			if (1 == ra_check_memo_reg(NO_SECURITY))
				rv = 1;
			break;
#endif
		default:
#ifdef DIAGNOSTIC
			aprint_normal_dev(sc->sc_dev, "%s: bad pin=%d\n",
				__func__, pin);
#endif
			break;
		}
	}

	RALINK_DEBUG_0(RALINK_DEBUG_INFO, "pin %d, value %x\n", pin, rv);
	return rv;
}

/*
 * There are three ways to change the value of a pin.
 *   You can write to the DATA register, which will set
 *   or reset a pin.  But you need to store it locally and
 *   read/mask/set it, which is potentially racy without locks.
 *   There are also SET and RESET registers, which allow you to write
 *   a value to a single pin and not affect any other pins
 *   by accident.
 *
 * NOTE:  This has special hacks in it.  If pin 52 (which does not exist)
 * is written, it is a signal to clear the boot count register.  If pin
 * 53 is written, it is a upgrade signal to the bootloader.
 *
 */
#define MAGIC		0x27051956
#define UPGRADE_MAGIC	0x27051957
void
ra_gpio_pin_write(void *arg, int pin, int value)
{
	RALINK_DEBUG_FUNC_ENTRY();
	ra_gpio_softc_t * const sc = arg;
#if defined(BOOT_COUNT) || defined(UPGRADE)
	uint32_t r;
#endif

	KASSERT(sc != NULL);
	RALINK_DEBUG(RALINK_DEBUG_INFO, "pin %d, val %d\n", pin, value);

	if (pin >= GPIO_PINS) {
		/*
		 * Special hack: a pseudo-pin used for signaling
		 */
		switch(pin) {
#ifdef BOOT_COUNT
		case BOOT_COUNT:
			/* Reset boot count */
			r = sy_read(sc, RA_SYSCTL_MEMO0);
			if (r == MAGIC)
				sy_write(sc, RA_SYSCTL_MEMO1, 0);
			break;
#endif
#ifdef UPGRADE
		case UPGRADE:
			/* Set upgrade flag */
			sy_write(sc, RA_SYSCTL_MEMO0, UPGRADE_MAGIC);
			sy_write(sc, RA_SYSCTL_MEMO1, UPGRADE_MAGIC);
			break;
#endif
		default:
#ifdef DIAGNOSTIC
			aprint_normal_dev(sc->sc_dev, "%s: bad pin=%d\n",
				__func__, pin);
#endif
		}
		return;
	}

	/*
	 * normal case: a regular GPIO pin
	 * if pin number is in a gap in the range,
	 * then warn and return
	 */
	const int index = pin_tab_index[pin];
	KASSERTMSG(index != -1, "%s: non-existant pin=%d\n", __func__, pin);
	if (index == -1)
		return;

	const pin_tab_t * const ptp = &pin_tab[index];
	const u_int mask_bit = 1 << (pin - ptp->pin_mask_base);
	const uint32_t reg_bit = 1 << (pin - ptp->pin_reg_base);
	const bus_size_t off = (value == 0) ?
		ptp->pin_output_clr : ptp->pin_output_set;

	if ((ptp->pin_dir.mask & mask_bit) == 0) {
#ifdef DIAGNOSTIC
		aprint_normal_dev(sc->sc_dev,
			"%s: Writing non-output pin: %d\n", __func__, pin);
#endif
		return;
	}

	bus_space_write_4(sc->sc_memt, sc->sc_gp_memh, off, reg_bit);
}

static void
ra_gpio_pin_ctl(void *arg, int pin, int flags)
{
	RALINK_DEBUG_FUNC_ENTRY();

	/*
	 * For now, this lets us know that user-space is using the GPIOs
	 *  and the kernel shouldn't blink LEDs any more
	 */
	gpio_driver_blink_leds = 0;
}

/*
 *  Check the three interrupt registers and ack them
 *   immediately.  If a button is pushed, use the
 *   handle_key_press call to debounce it.  Otherwise,
 *   call the softint handler to send any necessary
 *   events.
 */
static int
ra_gpio_intr(void *arg)
{
	RALINK_DEBUG_FUNC_ENTRY();
	ra_gpio_softc_t * const sc = arg;

#if 0
	/* Read the 3 interrupt registers */
#if defined(MT7628)
	if (sc->sc_intr_status00_31 || sc->sc_intr_status32_63 ||
	    sc->sc_intr_status64_95) {
		printf("\n0-31 %x, 32-63 %x, 64_95 %x\n",
		    sc->sc_intr_status00_31,
		    sc->sc_intr_status32_63,
		    sc->sc_intr_status64_95);
	}
#else
	if (sc->sc_intr_status00_23 || sc->sc_intr_status24_39 ||
	    sc->sc_intr_status40_51) {
		printf("\n0-23 %x, 24-39 %x, 40_51 %x\n",
		    sc->sc_intr_status00_23,
		    sc->sc_intr_status24_39,
		    sc->sc_intr_status40_51);
	}
#endif
#endif

#if defined(MT7628)
	sc->sc_intr_status00_31 |= gp_read(sc, RA_PIO_00_31_INT_STAT);
	sc->sc_intr_status32_63 |= gp_read(sc, RA_PIO_32_63_INT_STAT);
	sc->sc_intr_status64_95 |= gp_read(sc, RA_PIO_64_95_INT_STAT);
#else
	sc->sc_intr_status00_23 |= gp_read(sc, RA_PIO_00_23_INT);
	sc->sc_intr_status24_39 |= gp_read(sc, RA_PIO_24_39_INT);
	sc->sc_intr_status40_51 |= gp_read(sc, RA_PIO_40_51_INT);
#if defined(SLICKROCK)
	sc->sc_intr_status72_95 |= gp_read(sc, RA_PIO_72_95_INT);
#endif
#endif

#if 0
	/* Trivial error checking, some interrupt had to have fired */
#if defined(MT7628)
	KASSERT((sc->sc_intr_status00_31 | sc->sc_intr_status32_64 |
	    sc->sc_intr_status64_95) != 0);
#else
	KASSERT((sc->sc_intr_status00_23 | sc->sc_intr_status24_39 |
	    sc->sc_intr_status40_51) != 0);
#endif
#endif

	/* Debounce interrupt */
	ra_gpio_debounce_setup(sc);

	/*
	 * and ACK the interrupt.
	 *  OR the values in case the softint handler hasn't
	 *  been scheduled and handled any previous int
	 *  I don't know if resetting the EDGE register is
	 *  necessary, but the Ralink Linux driver does it.
	 */
#if defined(MT7628)
	gp_write(sc, RA_PIO_00_31_INT_STAT, sc->sc_intr_status00_31);
	gp_write(sc, RA_PIO_00_31_INT_STAT_EDGE, sc->sc_intr_status00_31);
	gp_write(sc, RA_PIO_32_63_INT_STAT, sc->sc_intr_status32_63);
	gp_write(sc, RA_PIO_32_63_INT_STAT_EDGE, sc->sc_intr_status32_63);
	gp_write(sc, RA_PIO_64_95_INT_STAT, sc->sc_intr_status64_95);
	gp_write(sc, RA_PIO_64_95_INT_STAT_EDGE, sc->sc_intr_status64_95);

	/* Reset until next time */
	sc->sc_intr_status00_31 = 0;
	sc->sc_intr_status32_63 = 0;
	sc->sc_intr_status64_95 = 0;
#else
	gp_write(sc, RA_PIO_00_23_INT, sc->sc_intr_status00_23);
	gp_write(sc, RA_PIO_00_23_EDGE_INT, sc->sc_intr_status00_23);
	gp_write(sc, RA_PIO_24_39_INT, sc->sc_intr_status24_39);
	gp_write(sc, RA_PIO_24_39_EDGE_INT, sc->sc_intr_status24_39);
	gp_write(sc, RA_PIO_40_51_INT, sc->sc_intr_status40_51);
	gp_write(sc, RA_PIO_40_51_EDGE_INT, sc->sc_intr_status40_51);
#if defined(SLICKROCK)
	gp_write(sc, RA_PIO_72_95_INT, sc->sc_intr_status72_95);
	gp_write(sc, RA_PIO_72_95_EDGE_INT, sc->sc_intr_status72_95);
#endif

	/* Reset until next time */
	sc->sc_intr_status00_23 = 0;
	sc->sc_intr_status24_39 = 0;
	sc->sc_intr_status40_51 = 0;
	sc->sc_intr_status72_95 = 0;
#endif /* MT7628 */

	return 1;
}

/*
 *  Handle key debouncing for the given pin
 */
static bool
ra_gpio_debounce_pin(ra_gpio_softc_t *sc, struct timeval *tv, u_int pin)
{
	RALINK_DEBUG_FUNC_ENTRY();

	/*
	 * If a pin has a time set, it is waiting for
	 *  a debounce period.  Check if it is ready
	 *  to send its event and clean up.  Otherwise,
	 *  reschedule 10mSec and try again later.
	 */
	if (0 != debounce_time[pin].tv_sec) {
		if (timercmp(tv, &debounce_time[pin], <)) {
			/*
			 * Haven't hit debounce time,
			 * need to reschedule
			 */
			return true;
		}
#if defined(SLICKROCK)
		switch (debounce_pin[pin]) {
		case SOFT_RST_IN_BUTTON:
			KNOTE(&knotes, RESET_BUTTON_EVT);
			break;

		case SS_BUTTON:
			KNOTE(&knotes, SS_BUTTON_EVT);
			break;

		case WPS_BUTTON:
			KNOTE(&knotes, WPS_BUTTON_EVT);
			break;

		case WIFI_ENABLE:
			KNOTE(&knotes, WIFI_ENABLE_EVT);
			break;

		/*
		 * These events are in case of overcurrent
		 * on USB/ExpressCard devices.
		 * If we receive an overcurrent signal,
		 * turn off power to the device and
		 * let the USB driver know.
		 */
		case CURRENT_LIMIT_FLAG_USB1:
			ra_gpio_pin_write(sc, POWER_EN_USB1, 0);
			KNOTE(&knotes, CURRENT_LIMIT_EVT);
#if 0
			cpusb_overcurrent_occurred(CURRENT_LIMIT_FLAG_USB1);
#endif
			printf("\nUSB1 current limit received!\n");
			break;

		case CURRENT_LIMIT_FLAG_USB2:
			ra_gpio_pin_write(sc, POWER_EN_USB2, 0);
			KNOTE(&knotes, CURRENT_LIMIT_EVT);
#if 0
			cpusb_overcurrent_occurred(CURRENT_LIMIT_FLAG_USB2);
#endif
			printf("\nUSB2 current limit received!\n");
			break;

		case CURRENT_LIMIT_FLAG_USB3:
			ra_gpio_pin_write(sc, POWER_EN_USB3, 0);
			KNOTE(&knotes, CURRENT_LIMIT_EVT);
#if 0
			cpusb_overcurrent_occurred(CURRENT_LIMIT_FLAG_USB3);
#endif
			printf("\nUSB3 current limit received!\n");
			break;

		case CURRENT_LIMIT_FLAG_EX1:
			ra_gpio_pin_write(sc, POWER_EN_EX1, 0);
			KNOTE(&knotes, CURRENT_LIMIT_EVT);
#if 0
			cpusb_overcurrent_occurred(debounce_pin[pin]);
#endif
			printf("\nExpressCard1 current limit received!\n");
			break;

		case CURRENT_LIMIT_FLAG_EX2:
			ra_gpio_pin_write(sc, POWER_EN_EX2, 0);
			KNOTE(&knotes, CURRENT_LIMIT_EVT);
#if 0
			cpusb_overcurrent_occurred(debounce_pin[pin]);
#endif
			printf("\nExpressCard2 current limit received!\n");
			break;

		default:
			printf("\nUnknown debounce pin %d received.\n",
			    debounce_pin[pin]);
		}
#endif/* SLICKROCK */
#if defined(PEBBLES500) || defined(PEBBLES35)
		switch (debounce_pin[pin]) {
		case SOFT_RST_IN_BUTTON:
			KNOTE(&knotes, RESET_BUTTON_EVT);
			break;

		case WPS_BUTTON:
			KNOTE(&knotes, WPS_BUTTON_EVT);
			break;

		case EXCARD_ATTACH:
			KNOTE(&knotes, EXCARD_ATTACH_EVT);
			break;

		/*
		 * These events are in case of overcurrent
		 * on USB/ExpressCard devices.
		 * If we receive an overcurrent signal,
		 * turn off power to the device and
		 * let the USB driver know.
		 */
		case CURRENT_LIMIT_FLAG_USB1:
			ra_gpio_pin_write(sc, POWER_EN_USB, 0);
			KNOTE(&knotes, CURRENT_LIMIT_EVT);
			cpusb_overcurrent_occurred(CURRENT_LIMIT_FLAG_USB1);
			printf("\nUSB current limit received!\n");
			break;

		/*
		 * If either voltage is over current,
		 * turn off all ExpressCard power
		 */
		case CURRENT_LIMIT_FLAG1_3_3v:
		case CURRENT_LIMIT_FLAG1_1_5v:
			ra_gpio_pin_write(sc, POWER_EN_EXCARD1_3_3v, 0);
			ra_gpio_pin_write(sc, POWER_EN_EXCARD1_1_5v, 0);
			KNOTE(&knotes, CURRENT_LIMIT_EVT);
			cpusb_overcurrent_occurred(debounce_pin[pin]);
			printf("\nExpressCard current limit received!\n");
			break;

		default:
			printf("\nUnknown debounce pin received.\n");
		}
#endif/* PEBBLES500 || PEBBLES35 */
#if defined(TABLEROCK) || defined(SPOT2) || defined(PUCK) || defined(MOAB)
		if (POWER_OFF_BUTTON == debounce_pin[pin]) {
			KNOTE(&knotes, POWER_BUTTON_EVT);
		}
		if (LAN_WAN_SW == debounce_pin[pin]) {
			KNOTE(&knotes, LAN_WAN_SW_EVT);
		}
		if (DOCK_SENSE == debounce_pin[pin]) {
			KNOTE(&knotes, DOCK_SENSE_EVT);
		}
		if (WPS_BUTTON == debounce_pin[pin]) {
			KNOTE(&knotes, WPS_BUTTON_EVT);
		}
		if (IN_5V == debounce_pin[pin]) {
			/* Set the charger to match the in 5V line */
			if (ra_gpio_pin_read(sc, IN_5V) == 0) {
				ra_gpio_pin_write(sc, CHARGER_OFF, 0);
			} else {
				ra_gpio_pin_write(sc, CHARGER_OFF, 1);
			}
			KNOTE(&knotes, IN_5V_EVT);
		}
#endif/* TABLEROCK || SPOT2 || PUCK || MOAB */
		/* Re-enable interrupt and reset time */
		enable_gpio_interrupt(sc, debounce_pin[pin]);
		debounce_time[pin].tv_sec = 0;

	}
	return false;
}

/*
 *  Handle key debouncing.
 *  Re-enable the key interrupt after DEBOUNCE_TIME
 */
static void
ra_gpio_debounce_process(void *arg)
{
	RALINK_DEBUG_FUNC_ENTRY();
	ra_gpio_softc_t * const sc = arg;
	bool reschedule = false;
	struct timeval now;

	microtime(&now);

	for (u_int pin=0; pin < __arraycount(debounce_pin); pin++)
		if (ra_gpio_debounce_pin(sc, &now, pin))
			reschedule = true;

	if (reschedule)
		callout_schedule(&sc->sc_tick_callout, MS_TO_HZ(10));
}

/*
 * Handle key and other interrupt debouncing.
 */
static void
ra_gpio_debounce_setup(ra_gpio_softc_t *sc)
{
	RALINK_DEBUG_FUNC_ENTRY();
	u_int32_t pin;
	struct timeval now;
	struct timeval wait = {
		.tv_sec  = 0,
		.tv_usec = (DEBOUNCE_TIME * 1000)
	};

	/*
	 * The 371/372 series are a bit more complex.  They have
	 *  interrupt sources across all three interrupt
	 *  registers.
	 */
	for (int i = 0; i < __arraycount(debounce_pin); i++) {
		u_int32_t *intr_status;
		int offset;
#if defined(MT7628)
		if (debounce_pin[i] < 32) {
			intr_status = &sc->sc_intr_status00_31;
			offset = 0;
		} else if (debounce_pin[i] < 64) {
			intr_status = &sc->sc_intr_status32_63;
			offset = 32;
		} else {
			intr_status = &sc->sc_intr_status64_95;
			offset = 64;
		}
#else /* !MT7628 */
		if (debounce_pin[i] < 24) {
			intr_status = &sc->sc_intr_status00_23;
			offset = 0;
		} else if (debounce_pin[i] < 40) {
			intr_status = &sc->sc_intr_status24_39;
			offset = 24;
		} else if (debounce_pin[i] < 71) {
			intr_status = &sc->sc_intr_status40_51;
			offset = 40;
		} else {
			intr_status = &sc->sc_intr_status72_95;
			offset = 72;
		}
#endif /* !MT7628 */
		if (*intr_status & (1 << (debounce_pin[i] - offset))) {
			pin = debounce_pin[i];

#ifdef ENABLE_RALINK_DEBUG_INFO
			if (ra_gpio_pin_read(sc, pin)) {
				RALINK_DEBUG(RALINK_DEBUG_INFO,
				    "%s() button 0x%x, pin %d released\n",
				    __func__, *intr_status, pin);
			} else {
				RALINK_DEBUG(RALINK_DEBUG_INFO,
				    "%s() button 0x%x, pin %d pressed\n",
				    __func__, *intr_status, pin);
			}
#endif

#if 0
			/* Mask pin that is being handled */
			*intr_status &= ~((1 << (debounce_pin[i] - offset)));
#endif

			/* Handle debouncing. */
			disable_gpio_interrupt(sc, pin);
			microtime(&now);

#if defined(TABLEROCK)
			/*
			 * The dock's LAN_WAN switch can cause a fire of
			 * interrupts if it sticks in the half-way position.
			 * Ignore it for longer than other buttons.
			 */
			if (pin == LAN_WAN_SW) {
				wait.tv_usec *= 2;
			}
#endif

			timeradd(&now, &wait, &debounce_time[i]);
		}
	}

	/* If the debounce callout hasn't been scheduled, start it up. */
	if (FALSE == callout_pending(&sc->sc_tick_callout)) {
		callout_schedule(&sc->sc_tick_callout, MS_TO_HZ(DEBOUNCE_TIME));
	}
}

/*
 * Disable both rising and falling
 */
static void
disable_gpio_interrupt(ra_gpio_softc_t *sc, int pin)
{
	RALINK_DEBUG_FUNC_ENTRY();
	const int index = pin_tab_index[pin];
	KASSERTMSG(index != -1, "%s: non-existant pin=%d\n", __func__, pin);
	if (index == -1)
		return;

	const pin_tab_t * const ptp = &pin_tab[index];
	const uint32_t reg_bit = 1 << (pin - ptp->pin_reg_base);
	uint32_t r;

	r = gp_read(sc, ptp->pin_rise.reg);
	r &= ~reg_bit;
	gp_write(sc, ptp->pin_rise.reg, r);

	r = gp_read(sc, ptp->pin_fall.reg);
	r &= ~reg_bit;
	gp_write(sc, ptp->pin_fall.reg, r);
}

/*
 * Restore GPIO interrupt setting
 */
static void
enable_gpio_interrupt(ra_gpio_softc_t *sc, int pin)
{
	const int index = pin_tab_index[pin];
	KASSERTMSG(index != -1, "%s: non-existant pin=%d\n", __func__, pin);
	if (index == -1)
		return;

	const pin_tab_t * const ptp = &pin_tab[index];
	const uint32_t mask_bit = 1 << (pin - ptp->pin_mask_base);
	const uint32_t reg_bit = 1 << (pin - ptp->pin_reg_base);
	uint32_t r;

	if (ptp->pin_rise.mask & mask_bit) {
		r = gp_read(sc, ptp->pin_rise.reg);
		r |= reg_bit;
		gp_write(sc, ptp->pin_rise.reg, r);
	}

	if (ptp->pin_fall.mask & mask_bit) {
		r = gp_read(sc, ptp->pin_fall.reg);
		r |= reg_bit;
		gp_write(sc, ptp->pin_fall.reg, r);
	}
}

/*
 * XXX this function is obsolete/unused? XXX
 *
 *  Go through each of the interrupts and send the appropriate
 *   event.
 */
static void
ra_gpio_softintr(void *arg)
{
#if defined(MT7628)
	RALINK_DEBUG(RALINK_DEBUG_INFO,
	    "gpio softintr called with 0x%x, 0x%x, 0x%x\n",
	    sc->sc_intr_status00_31, sc->sc_intr_status32_63,
	    sc->sc_intr_status64_95);
#else
	RALINK_DEBUG(RALINK_DEBUG_INFO,
	    "gpio softintr called with 0x%x, 0x%x, 0x%x, 0x%x\n",
	    sc->sc_intr_status00_23, sc->sc_intr_status24_39,
	    sc->sc_intr_status40_51, sc->sc_intr_status72_95);
#endif
}

/*
 * gpio_event_app_user_attach - called when knote is ADDed
 */
static int
gpio_event_app_user_attach(struct knote *kn)
{
	RALINK_DEBUG_0(RALINK_DEBUG_INFO, "%s() %p\n", __func__, kn);

	if (NULL == kn) {
		RALINK_DEBUG(RALINK_DEBUG_ERROR, "Null kn found\n");
		return 0;
	}

	kn->kn_flags |= EV_CLEAR;	/* automatically set */
	SLIST_INSERT_HEAD(&knotes, kn, kn_selnext);

	return 0;
}

/*
 * gpio_event_app_user_detach - called when knote is DELETEd
 */
static void
gpio_event_app_user_detach(struct knote *kn)
{
	RALINK_DEBUG_0(RALINK_DEBUG_INFO, "%s() %p\n", __func__, kn);
	if (NULL == kn) {
		RALINK_DEBUG(RALINK_DEBUG_ERROR, "Null kn found\n");
		return;
	}
	SLIST_REMOVE(&knotes, kn, knote, kn_selnext);
}

/*
 * gpio_event_app_user_event - called when event is triggered
 */
static int
gpio_event_app_user_event(struct knote *kn, long hint)
{
	RALINK_DEBUG_0(RALINK_DEBUG_INFO, "%s() %p hint: %ld\n",
	    __func__, kn, hint);

	if (NULL == kn) {
		RALINK_DEBUG(RALINK_DEBUG_ERROR, "Null kn found\n");
		return 0;
	}

	if (hint != 0) {
		kn->kn_data = kn->kn_sdata;
		kn->kn_fflags |= hint;
	}

	return 1;
}

/*
 *  Blinky light control during boot.
 *  This marches through the SS/BATT LEDS
 *  to give an indication something is going on.
 */
void
ra_gpio_toggle_LED(void *arg)
{
	RALINK_DEBUG_FUNC_ENTRY();
	ra_gpio_softc_t * const sc = arg;
	static int led_index = 0;
	static int led_timing_hack = 1;

	if ((led_timing_hack >= 6) ||
		(0 == gpio_driver_blink_leds)) {
		/* We're out of boot timing, don't blink LEDs any more */
		return;
	}

#if 0
	/* Disable lit LED */
	gp_write(sc, SET_SS_LED_REG,
	    (1 << (led_array1[led_index++] - SS_OFFSET)));
#endif

	if (led_index == (sizeof(led_array1))) {
		led_index = 0;
		for (int i = 0; i < sizeof(led_array1); i++) {
			ra_gpio_pin_write(sc, led_array1[i], 1);
		}
	}

	/* Light next LED */
	ra_gpio_pin_write(sc, led_array1[led_index++], 0);

#if 0
	if (led_index == 4) {
		led_timing_hack = 1;
	}
#endif

#ifdef BOOT_LED_TIMING
	/* Setting 3.5 second per LED */
	if ((led_timing_hack) &&
		(led_timing_hack < 6)) {
		led_timing_hack++;
		callout_reset(&led_tick_callout, MS_TO_HZ(BOOT_LED_TIMING),
		    ra_gpio_toggle_LED, sc);
	}
#endif
}
