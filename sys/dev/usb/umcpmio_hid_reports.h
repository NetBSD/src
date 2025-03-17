/*	$NetBSD: umcpmio_hid_reports.h,v 1.2 2025/03/17 18:24:08 riastradh Exp $	*/

/*
 * Copyright (c) 2024 Brad Spencer <brad@anduin.eldar.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
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
 */

#ifndef	_UMCPMIO_HID_REPORTS_H_
#define	_UMCPMIO_HID_REPORTS_H_

#include <sys/types.h>

/*
 * It is nice that all HID reports want a 64 byte request and return a
 * 64 byte response.
 */

#define MCP2221_REQ_BUFFER_SIZE 64
#define MCP2221_RES_BUFFER_SIZE 64

#define	MCP2221_CMD_STATUS		0x10

#define MCP2221_CMD_I2C_FETCH_READ_DATA	0x40

#define MCP2221_CMD_SET_GPIO_CFG	0x50
#define MCP2221_CMD_GET_GPIO_CFG	0x51

#define MCP2221_CMD_SET_SRAM		0x60
#define MCP2221_CMD_GET_SRAM		0x61

#define MCP2221_I2C_WRITE_DATA		0x90
#define MCP2221_I2C_READ_DATA		0x91
#define MCP2221_I2C_WRITE_DATA_RS	0x92
#define MCP2221_I2C_READ_DATA_RS	0x93
#define MCP2221_I2C_WRITE_DATA_NS	0x94

#define MCP2221_CMD_GET_FLASH		0xb0
#define MCP2221_CMD_SET_FLASH		0xb1
#define MCP2221_CMD_SEND_FLASH_PASSWORD	0xb2

#define MCP2221_CMD_COMPLETE_OK		0x00
#define MCP2221_CMD_COMPLETE_NO_SUPPORT	0x02
#define MCP2221_CMD_COMPLETE_EPERM	0x03

#define MCP2221_I2C_DO_CANCEL		0x10
#define MCP2221_INTERNAL_CLOCK		12000000
#define MCP2221_DEFAULT_I2C_SPEED	100000
#define MCP2221_I2C_SET_SPEED		0x20

/* The request and response structures are, perhaps, over literal. */

struct mcp2221_status_req {
	uint8_t		cmd; /* MCP2221_CMD_STATUS */
	uint8_t		dontcare1;
	uint8_t		cancel_transfer;
	uint8_t		set_i2c_speed;
	uint8_t		i2c_clock_divider;
	uint8_t		dontcare2[59];
};

#define MCP2221_I2C_SPEED_SET		0x20
#define MCP2221_I2C_SPEED_BUSY		0x21
#define MCP2221_ENGINE_T1_MASK_NACK	0x40

struct mcp2221_status_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		cancel_transfer;
	uint8_t		set_i2c_speed;
	uint8_t		i2c_clock_divider;
	uint8_t		dontcare2[3];
	uint8_t		internal_i2c_state;
	uint8_t		lsb_i2c_req_len;
	uint8_t		msb_i2c_req_len;
	uint8_t		lsb_i2c_trans_len;
	uint8_t		msb_i2c_trans_len;
	uint8_t		internal_i2c_bcount;
	uint8_t		i2c_speed_divider;
	uint8_t		i2c_timeout_value;
	uint8_t		lsb_i2c_address;
	uint8_t		msb_i2c_address;
	uint8_t		dontcare3a[2];
	uint8_t		internal_i2c_state20;
	uint8_t		dontcare3b;
	uint8_t		scl_line_value;
	uint8_t		sda_line_value;
	uint8_t		interrupt_edge_state;
	uint8_t		i2c_read_pending;
	uint8_t		dontcare4[20];
	uint8_t		mcp2221_hardware_rev_major;
	uint8_t		mcp2221_hardware_rev_minor;
	uint8_t		mcp2221_firmware_rev_major;
	uint8_t		mcp2221_firmware_rev_minor;
	uint8_t		adc_channel0_lsb;
	uint8_t		adc_channel0_msb;
	uint8_t		adc_channel1_lsb;
	uint8_t		adc_channel1_msb;
	uint8_t		adc_channel2_lsb;
	uint8_t		adc_channel2_msb;
	uint8_t		dontcare5[8];
};

#define MCP2221_GPIO_CFG_ALTER		0xff

struct mcp2221_set_gpio_cfg_req {
	uint8_t		cmd; /* MCP2221_CMD_SET_GPIO_CFG */
	uint8_t		dontcare1;

	uint8_t		alter_gp0_value;
	uint8_t		new_gp0_value;
	uint8_t		alter_gp0_dir;
	uint8_t		new_gp0_dir;

	uint8_t		alter_gp1_value;
	uint8_t		new_gp1_value;
	uint8_t		alter_gp1_dir;
	uint8_t		new_gp1_dir;

	uint8_t		alter_gp2_value;
	uint8_t		new_gp2_value;
	uint8_t		alter_gp2_dir;
	uint8_t		new_gp2_dir;

	uint8_t		alter_gp3_value;
	uint8_t		new_gp3_value;
	uint8_t		alter_gp3_dir;
	uint8_t		new_gp3_dir;

	uint8_t		reserved[46];
};

struct mcp2221_set_gpio_cfg_res {
	uint8_t		cmd;
	uint8_t		completion;

	uint8_t		alter_gp0_value;
	uint8_t		new_gp0_value;
	uint8_t		alter_gp0_dir;
	uint8_t		new_gp0_dir;

	uint8_t		alter_gp1_value;
	uint8_t		new_gp1_value;
	uint8_t		alter_gp1_dir;
	uint8_t		new_gp1_dir;

	uint8_t		alter_gp2_value;
	uint8_t		new_gp2_value;
	uint8_t		alter_gp2_dir;
	uint8_t		new_gp2_dir;

	uint8_t		alter_gp3_value;
	uint8_t		new_gp3_value;
	uint8_t		alter_gp3_dir;
	uint8_t		new_gp3_dir;

	uint8_t		dontcare[46];
};

struct mcp2221_get_gpio_cfg_req {
	uint8_t		cmd; /* MCP2221_CMD_GET_GPIO_CFG */
	uint8_t		dontcare[63];
};

#define MCP2221_GPIO_CFG_VALUE_NOT_GPIO	0xEE
#define MCP2221_GPIO_CFG_DIR_NOT_GPIO	0xEF
#define MCP2221_GPIO_CFG_DIR_INPUT	0x01
#define MCP2221_GPIO_CFG_DIR_OUTPUT	0x00

struct mcp2221_get_gpio_cfg_res {
	uint8_t		cmd;
	uint8_t		completion;

	uint8_t		gp0_pin_value;
	uint8_t		gp0_pin_dir;

	uint8_t		gp1_pin_value;
	uint8_t		gp1_pin_dir;

	uint8_t		gp2_pin_value;
	uint8_t		gp2_pin_dir;

	uint8_t		gp3_pin_value;
	uint8_t		gp3_pin_dir;

	uint8_t		dontcare[54];
};

#define MCP2221_SRAM_GPIO_CHANGE_DCCD	0x80

#define MCP2221_SRAM_GPIO_CLOCK_DC_MASK	0x18
#define MCP2221_SRAM_GPIO_CLOCK_DC_75	0x18
#define MCP2221_SRAM_GPIO_CLOCK_DC_50	0x10
#define MCP2221_SRAM_GPIO_CLOCK_DC_25	0x08
#define MCP2221_SRAM_GPIO_CLOCK_DC_0	0x00

#define MCP2221_SRAM_GPIO_CLOCK_CD_MASK	0x07
#define MCP2221_SRAM_GPIO_CLOCK_CD_375KHZ 0x07
#define MCP2221_SRAM_GPIO_CLOCK_CD_750KHZ 0x06
#define MCP2221_SRAM_GPIO_CLOCK_CD_1P5MHZ 0x05
#define MCP2221_SRAM_GPIO_CLOCK_CD_3MHZ 0x04
#define MCP2221_SRAM_GPIO_CLOCK_CD_6MHZ 0x03
#define MCP2221_SRAM_GPIO_CLOCK_CD_12MHZ 0x02
#define MCP2221_SRAM_GPIO_CLOCK_CD_24MHZ 0x01

#define MCP2221_SRAM_CHANGE_DAC_VREF	0x80
#define MCP2221_SRAM_DAC_IS_VRM		0x20
#define MCP2221_SRAM_DAC_VRM_MASK	0xC0
#define MCP2221_SRAM_DAC_VRM_4096V	0xC0
#define MCP2221_SRAM_DAC_VRM_2048V	0x80
#define MCP2221_SRAM_DAC_VRM_1024V	0x40
#define MCP2221_SRAM_DAC_VRM_OFF	0x00
#define MCP2221_SRAM_CHANGE_DAC_VALUE	0x80
#define MCP2221_SRAM_DAC_VALUE_MASK	0x1F

#define MCP2221_SRAM_CHANGE_ADC_VREF	0x80
#define MCP2221_SRAM_ADC_IS_VRM		0x04
#define MCP2221_SRAM_ADC_VRM_MASK	0x18
#define MCP2221_SRAM_ADC_VRM_4096V	0x18
#define MCP2221_SRAM_ADC_VRM_2048V	0x10
#define MCP2221_SRAM_ADC_VRM_1024V	0x08
#define MCP2221_SRAM_ADC_VRM_OFF	0x00

#define MCP2221_SRAM_ALTER_IRQ		0x80
#define MCP2221_SRAM_ALTER_POS_EDGE	0x10
#define MCP2221_SRAM_ENABLE_POS_EDGE	0x08
#define MCP2221_SRAM_ALTER_NEG_EDGE	0x04
#define MCP2221_SRAM_ENABLE_NEG_EDGE	0x02
#define MCP2221_SRAM_CLEAR_IRQ		0x01

#define MCP2221_SRAM_ALTER_GPIO		0xff
#define MCP2221_SRAM_GPIO_HIGH		0x0f
#define MCP2221_SRAM_GPIO_OUTPUT_HIGH	0x10
#define MCP2221_SRAM_GPIO_TYPE_MASK	0x08
#define MCP2221_SRAM_GPIO_INPUT		0x08
#define MCP2221_SRAM_PIN_TYPE_MASK	0x07
#define MCP2221_SRAM_PIN_IS_GPIO	0x00
#define MCP2221_SRAM_PIN_IS_DED		0x01
#define MCP2221_SRAM_PIN_IS_ALT0	0x02
#define MCP2221_SRAM_PIN_IS_ALT1	0x03
#define MCP2221_SRAM_PIN_IS_ALT2	0x04

struct mcp2221_set_sram_req {
	uint8_t		cmd; /* MCP2221_CMD_SET_SRAM */
	uint8_t		dontcare1;

	uint8_t		clock_output_divider;
	uint8_t		dac_voltage_reference;
	uint8_t		set_dac_output_value;
	uint8_t		adc_voltage_reference;
	uint8_t		irq_config;

	uint8_t		alter_gpio_config;
	uint8_t		gp0_settings;
	uint8_t		gp1_settings;
	uint8_t		gp2_settings;
	uint8_t		gp3_settings;

	uint8_t		reserved[52];
};

struct mcp2221_set_sram_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		dontcare[62];
};

struct mcp2221_get_sram_req {
	uint8_t		cmd; /* MCP2221_CMD_GET_SRAM */
	uint8_t		dontcare[63];
};

struct mcp2221_get_sram_res {
	uint8_t		cmd;
	uint8_t		completion;

	uint8_t		len_chip_setting;
	uint8_t		len_gpio_setting;

	uint8_t		sn_initial_ps_cs;
	uint8_t		clock_divider;
	uint8_t		dac_reference_voltage;
	uint8_t		irq_adc_reference_voltage;

	uint8_t		lsb_usb_vid;
	uint8_t		msb_usb_vid;
	uint8_t		lsb_usb_pid;
	uint8_t		msb_usb_pid;

	uint8_t		usb_power_attributes;
	uint8_t		usb_requested_ma;

	uint8_t		current_password_byte_1;
	uint8_t		current_password_byte_2;
	uint8_t		current_password_byte_3;
	uint8_t		current_password_byte_4;
	uint8_t		current_password_byte_5;
	uint8_t		current_password_byte_6;
	uint8_t		current_password_byte_7;
	uint8_t		current_password_byte_8;

	uint8_t		gp0_settings;
	uint8_t		gp1_settings;
	uint8_t		gp2_settings;
	uint8_t		gp3_settings;

	uint8_t		dontcare[38];
};

#define MCP2221_I2C_ENGINE_BUSY		0x01
#define MCP2221_ENGINE_STARTTIMEOUT	0x12
#define MCP2221_ENGINE_REPSTARTTIMEOUT	0x17
#define MCP2221_ENGINE_STOPTIMEOUT	0x62
#define MCP2221_ENGINE_ADDRSEND		0x21
#define MCP2221_ENGINE_ADDRTIMEOUT	0x23
#define MCP2221_ENGINE_PARTIALDATA	0x41
#define MCP2221_ENGINE_READMORE		0x43
#define MCP2221_ENGINE_WRITETIMEOUT	0x44
#define MCP2221_ENGINE_READTIMEOUT	0x52
#define MCP2221_ENGINE_READPARTIAL	0x54
#define MCP2221_ENGINE_READCOMPLETE	0x55
#define MCP2221_ENGINE_ADDRNACK		0x25
#define MCP2221_ENGINE_WRITINGNOSTOP	0x45

struct mcp2221_i2c_req {
	uint8_t		cmd; /* MCP2221_I2C_WRITE_DATA
			      * MCP2221_I2C_READ_DATA
			      * MCP2221_I2C_WRITE_DATA_RS
			      * MCP2221_I2C_READ_DATA_RS
			      * MCP2221_I2C_WRITE_DATA_NS
			      */
	uint8_t		lsblen;
	uint8_t		msblen;
	uint8_t		slaveaddr;
	uint8_t		data[60];
};

struct mcp2221_i2c_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		internal_i2c_state;
	uint8_t		dontcare[61];
};

#define MCP2221_FETCH_READ_PARTIALDATA	0x41
#define MCP2221_FETCH_READERROR		0x7F

struct mcp2221_i2c_fetch_req {
	uint8_t		cmd; /* MCP2221_CMD_I2C_FETCH_READ_DATA */
	uint8_t		dontcare[63];
};

struct mcp2221_i2c_fetch_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		internal_i2c_state;
	uint8_t		fetchlen;
	uint8_t		data[60];
};

#define MCP2221_FLASH_SUBCODE_CS	0x00
#define MCP2221_FLASH_SUBCODE_GP	0x01
#define MCP2221_FLASH_SUBCODE_USBMAN	0x02
#define MCP2221_FLASH_SUBCODE_USBPROD	0x03
#define MCP2221_FLASH_SUBCODE_USBSN	0x04
#define MCP2221_FLASH_SUBCODE_CHIPSN	0x05

struct mcp2221_get_flash_req {
	uint8_t		cmd; /* MCP2221_CMD_GET_FLASH */
	uint8_t		subcode;
	uint8_t		reserved[62];
};

struct mcp2221_get_flash_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		res_len;
	union {
		struct {
			uint8_t		dontcare;
			uint8_t		uartenum_led_protection;
			uint8_t		clock_divider;
			uint8_t		dac_reference_voltage;
			uint8_t		irq_adc_reference_voltage;
			uint8_t		lsb_usb_vid;
			uint8_t		msb_usb_vid;
			uint8_t		lsb_usb_pid;
			uint8_t		msb_usb_pid;
			uint8_t		usb_power_attributes;
			uint8_t		usb_requested_ma;
			uint8_t		dontcare2[50];
		} cs;
		struct {
			uint8_t		dontcare;
			uint8_t		gp0_settings;
			uint8_t		gp1_settings;
			uint8_t		gp2_settings;
			uint8_t		gp3_settings;
			uint8_t		dontcare2[56];
		} gp;
		struct {
			uint8_t		always0x03;
			uint8_t		unicode_man_descriptor[60];
		} usbman;
		struct {
			uint8_t		always0x03;
			uint8_t		unicode_product_descriptor[60];
		} usbprod;
		struct usbsn {
			uint8_t		always0x03;
			uint8_t		unicode_serial_number[60];
		} usbsn;
		struct {
			uint8_t		dontcare;
			uint8_t		factory_serial_number[60];
		} chipsn;
	} u;
};

#define MCP2221_FLASH_GPIO_HIGH		0x0f
#define MCP2221_FLASH_GPIO_VALUE_MASK	0x10
#define MCP2221_FLASH_GPIO_TYPE_MASK	0x08
#define MCP2221_FLASH_GPIO_INPUT	0x08
#define MCP2221_FLASH_PIN_TYPE_MASK	0x07
#define MCP2221_FLASH_PIN_IS_GPIO	0x00
#define MCP2221_FLASH_PIN_IS_DED	0x01
#define MCP2221_FLASH_PIN_IS_ALT0	0x02
#define MCP2221_FLASH_PIN_IS_ALT1	0x03
#define MCP2221_FLASH_PIN_IS_ALT2	0x04

struct mcp2221_put_flash_req {
	uint8_t		cmd; /* MCP2221_CMD_SET_FLASH */
	uint8_t		subcode;
	union {
		struct {
			uint8_t		uartenum_led_protection;
			uint8_t		clock_divider;
			uint8_t		dac_reference_voltage;
			uint8_t		irq_adc_reference_voltage;
			uint8_t		lsb_usb_vid;
			uint8_t		msb_usb_vid;
			uint8_t		lsb_usb_pid;
			uint8_t		msb_usb_pid;
			uint8_t		usb_power_attributes;
			uint8_t		usb_requested_ma;
			uint8_t		password_byte_1;
			uint8_t		password_byte_2;
			uint8_t		password_byte_3;
			uint8_t		password_byte_4;
			uint8_t		password_byte_5;
			uint8_t		password_byte_6;
			uint8_t		password_byte_7;
			uint8_t		password_byte_8;
			uint8_t		dontcare[44];
		} cs;
		struct {
			uint8_t		gp0_settings;
			uint8_t		gp1_settings;
			uint8_t		gp2_settings;
			uint8_t		gp3_settings;
			uint8_t		dontcare[58];
		} gp;
		struct {
			uint8_t		len;
			uint8_t		always0x03;
			uint8_t		unicode_man_descriptor[60];
		} usbman;
		struct {
			uint8_t		len;
			uint8_t		always0x03;
			uint8_t		unicode_product_descriptor[60];
		} usbprod;
		struct {
			uint8_t		len;
			uint8_t		always0x03;
			uint8_t		unicode_serial_number[60];
		} usbsn;
	} u;
};

struct mcp2221_put_flash_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		dontcare[62];
};

/* XXX - missing is the submit password call to unlock the chip */

#endif	/* _UMCPMIO_HID_REPORTS_H_ */
