/* $NetBSD: ssdfbvar.h,v 1.2 2019/03/17 04:03:17 tnn Exp $ */

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tobias Nygren.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * cfdata attachment flags
 */
#define SSDFB_ATTACH_FLAG_PRODUCT_MASK		0x000000ff
#define SSDFB_ATTACH_FLAG_UPSIDEDOWN		0x00000100
#define SSDFB_ATTACH_FLAG_INVERSE		0x00000200
#define SSDFB_ATTACH_FLAG_CONSOLE		0x00000400

/*
 * Fundamental commands
 * SSD1306 Rev 1.1 p.28
 * SH1106 Rev 0.1 p.19,20,22
 */
#define SSDFB_CMD_SET_CONTRAST_CONTROL		0x81
#define SSDFB_CMD_ENTIRE_DISPLAY_OFF		0xa4
#define SSDFB_CMD_ENTIRE_DISPLAY_ON		0xa5
#define SSDFB_CMD_SET_NORMAL_DISPLAY		0xa6
#define SSDFB_CMD_SET_INVERSE_DISPLAY		0xa7
#define SSDFB_CMD_SET_DISPLAY_OFF		0xae
#define SSDFB_CMD_SET_DISPLAY_ON		0xaf

/*
 * Scrolling commands; SSD1306 Rev 1.1 p. 28
 */
#define SSDFB_CMD_VERTICAL_AND_RIGHT_SCROLL	0x29
#define SSDFB_CMD_VERTICAL_AND_LEFT_SCROLL	0x2a
#define SSDFB_CMD_DEACTIVATE_SCROLL		0x2e
#define SSDFB_CMD_ACTIVATE_SCROLL		0x2f
#define SSDFB_CMD_SET_VERTICAL_SCROLL_AREA	0xa3

/*
 * Addressing commands
 * SSD1306 Rev 1.1 p.30
 * SH1106 Rev 0.1 p.18,22
 */
#define SSDFB_CMD_SET_LOWER_COLUMN_START_ADDRESS_BASE	0x00
#define SSDFB_CMD_SET_LOWER_COLUMN_START_ADDRESS_MAX	0x0f
#define SSDFB_CMD_SET_HIGHER_COLUMN_START_ADDRESS_BASE	0x10
#define SSDFB_CMD_SET_HIGHER_COLUMN_START_ADDRESS_MAX	0x1f
#define SSD1306_CMD_SET_MEMORY_ADDRESSING_MODE		0x20
	#define SSD1306_MEMORY_ADDRESSING_MODE_HORIZONTAL 0x00
	#define SSD1306_MEMORY_ADDRESSING_MODE_VERTICAL	0x01
	#define SSD1306_MEMORY_ADDRESSING_MODE_PAGE	0x02
#define SSD1306_CMD_SET_COLUMN_ADDRESS			0x21
#define SSD1306_CMD_SET_PAGE_ADDRESS			0x22
#define SSDFB_CMD_SET_PAGE_START_ADDRESS_BASE		0xb0
#define SSDFB_CMD_SET_PAGE_START_ADDRESS_MAX		0xb7

/*
 * Resolution & hardware layout commands
 * SSD1306 Rev 1.1 p.31
 * SH1106 Rev 0.1 p.19,20,21,23
 */
#define SSDFB_CMD_SET_DISPLAY_START_LINE_BASE		0x40
#define SSDFB_CMD_SET_DISPLAY_START_LINE_MAX		0x7f
#define SSDFB_CMD_SET_SEGMENT_REMAP_NORMAL		0xa0
#define SSDFB_CMD_SET_SEGMENT_REMAP_REVERSE		0xa1
#define SSDFB_CMD_SET_MULTIPLEX_RATIO			0xa8
#define SSDFB_CMD_SET_COM_OUTPUT_DIRECTION_NORMAL	0xc0
#define SSDFB_CMD_SET_COM_OUTPUT_DIRECTION_REMAP	0xc8
#define SSDFB_CMD_SET_DISPLAY_OFFSET			0xd3
#define SSDFB_CMD_SET_COM_PINS_HARDWARE_CFG		0xda
	#define SSDFB_COM_PINS_A1_MASK			0x02
	#define SSDFB_COM_PINS_ALTERNATIVE_MASK		0x10
	#define SSDFB_COM_PINS_REMAP_MASK		0x20

/*
 * Timing & driving commands
 * SSD1306 Rev 1.1 p.32
 * SH1106 Rev 0.1 p.24,25,26
 */
#define SSDFB_CMD_SET_DISPLAY_CLOCK_RATIO		0xd5
	#define SSDFB_DISPLAY_CLOCK_DIVIDER_MASK	0x0f
	#define SSDFB_DISPLAY_CLOCK_DIVIDER_SHIFT	0
	#define SSDFB_DISPLAY_CLOCK_OSCILLATOR_MASK	0xf0
	#define SSDFB_DISPLAY_CLOCK_OSCILLATOR_SHIFT	4
#define SSDFB_CMD_SET_PRECHARGE_PERIOD			0xd9
	#define SSDFB_PRECHARGE_MASK			0x0f
	#define SSDFB_PRECHARGE_SHIFT			0
	#define SSDFB_DISCHARGE_MASK			0xf0
	#define SSDFB_DISCHARGE_SHIFT			4
#define SSDFB_CMD_SET_VCOMH_DESELECT_LEVEL		0xdb
	#define SSD1306_VCOMH_DESELECT_LEVEL_0_65_VCC	0x00
	#define SSD1306_VCOMH_DESELECT_LEVEL_0_77_VCC	0x20
	#define SSD1306_VCOMH_DESELECT_LEVEL_0_83_VCC	0x30
	#define SH1106_VCOMH_DESELECT_LEVEL_DEFAULT	0x35

/*
 * Misc commands
 * SSD1306 Rev 1.1 p.32
 * SH1106 Rev 0.1 p.27,28
 */
#define SSDFB_CMD_NOP					0xe3
#define SH1106_CMD_READ_MODIFY_WRITE			0xe0
#define SH1106_CMD_READ_MODIFY_WRITE_CANCEL		0xee

/*
 * Charge pump commands
 * SSD1306 App Note Rev 0.4 p.3
 * SH1106 V0.1 p.18
 */
#define SSD1306_CMD_SET_CHARGE_PUMP			0x8d
	#define SSD1306_CHARGE_PUMP_ENABLE		0x14
	#define SSD1306_CHARGE_PUMP_DISABE		0x10
#define SH1106_CMD_SET_CHARGE_PUMP_7V4			0x30
#define SH1106_CMD_SET_CHARGE_PUMP_8V0			0x31
#define SH1106_CMD_SET_CHARGE_PUMP_8V4			0x32
#define SH1106_CMD_SET_CHARGE_PUMP_9V0			0x33

/*
 * DC-DC commands
 * SH1106 V0.1 p.18
 */
#define SH1106_CMD_SET_DC_DC				0xad
	#define SH1106_DC_DC_OFF			0x8a
	#define SH1106_DC_DC_ON				0x8b

typedef enum {
	SSDFB_CONTROLLER_UNKNOWN=0,
	SSDFB_CONTROLLER_SSD1306=1,
	SSDFB_CONTROLLER_SH1106=2,
} ssdfb_controller_id_t;

typedef enum {
	SSDFB_PRODUCT_UNKNOWN=0,
	SSDFB_PRODUCT_SSD1306_GENERIC=1,
	SSDFB_PRODUCT_SH1106_GENERIC=2,
	SSDFB_PRODUCT_ADAFRUIT_931=3,
	SSDFB_PRODUCT_ADAFRUIT_938=4,
} ssdfb_product_id_t;

#define SSDFB_I2C_DEFAULT_ADDR		0x3c
#define SSDFB_I2C_ALTERNATIVE_ADDR	0x3d

/* Co bit has different behaviour in SSD1306 and SH1106 */
#define SSDFB_I2C_CTRL_BYTE_CONTINUATION_MASK	__BIT(7)
#define SSDFB_I2C_CTRL_BYTE_DATA_MASK		__BIT(6)

union ssdfb_block {
	uint8_t		col[8];
	uint64_t	raw;
};

struct ssdfb_product {
	ssdfb_product_id_t		p_product_id;
	ssdfb_controller_id_t		p_controller_id;
	const char			*p_name;
	int				p_width;
	int				p_height;
	int				p_panel_shift;
	uint8_t				p_fosc;
	uint8_t				p_fosc_div;
	uint8_t				p_precharge;
	uint8_t				p_discharge;
	uint8_t				p_compin_cfg;
	uint8_t				p_vcomh_deselect_level;
	uint8_t				p_default_contrast;
	uint8_t				p_multiplex_ratio;
	uint8_t				p_chargepump_cmd;
	uint8_t				p_chargepump_arg;
};

struct ssdfb_softc {
	device_t			sc_dev;
	const struct ssdfb_product	*sc_p;

	/* wscons & rasops state */
	u_int				sc_mode;
	int				sc_fontcookie;
	struct wsdisplay_font		*sc_font;
	struct wsscreen_descr		sc_screen_descr;
	const struct wsscreen_descr	*sc_screens[1];
	struct wsscreen_list		sc_screenlist;
	struct rasops_info		sc_ri;
	size_t				sc_ri_bits_len;
	struct wsdisplay_emulops	sc_orig_riops;
	int				sc_nscreens;
	device_t			sc_wsdisplay;
	bool				sc_is_console;
	bool				sc_usepoll;

	/* hardware state */
	bool				sc_upsidedown;
	bool				sc_inverse;
	uint8_t				sc_contrast;
	bool				sc_display_on;
	union ssdfb_block		*sc_gddram;
	size_t				sc_gddram_len;

	/* damage tracking */
	lwp_t				*sc_thread;
	kcondvar_t			sc_cond;
	kmutex_t			sc_cond_mtx;
	bool				sc_detaching;
	int				sc_backoff;
	bool				sc_modified;
	struct uvm_object		*sc_uobj;

	/* reference to bus-specific code */
	void	*sc_cookie;
	int	(*sc_cmd)(void *, uint8_t *, size_t, bool);
	int	(*sc_transfer_rect)(void *, uint8_t, uint8_t, uint8_t, uint8_t,
				    uint8_t *, size_t, bool);
};

void	ssdfb_attach(struct ssdfb_softc *, int flags);
int	ssdfb_detach(struct ssdfb_softc *);
