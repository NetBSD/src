/*	$NetBSD: umcpmio_hid_reports.h,v 1.3 2025/11/29 18:39:14 brad Exp $	*/

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

#define UMCPMIO_REQ_BUFFER_SIZE 64
#define UMCPMIO_RES_BUFFER_SIZE 64

#define MCP2210_REQ_BUFFER_SIZE UMCPMIO_REQ_BUFFER_SIZE
#define MCP2210_RES_BUFFER_SIZE UMCPMIO_RES_BUFFER_SIZE

/* MCP-2210 stuff */

#define MCP2210_CMD_STATUS		0x10
#define MCP2210_CMD_SPI_CANCEL		0x11
#define MCP2210_CMD_GET_GP6_EVENTS	0x12
#define MCP2210_CMD_GET_GPIO_SRAM	0x20
#define MCP2210_CMD_SET_GPIO_SRAM	0x21
#define MCP2210_CMD_SET_GPIO_VAL_SRAM	0x30
#define MCP2210_CMD_GET_GPIO_VAL_SRAM	0x31
#define MCP2210_CMD_SET_GPIO_DIR_SRAM	0x32
#define MCP2210_CMD_GET_GPIO_DIR_SRAM	0x33
#define MCP2210_CMD_SET_SPI_SRAM	0x40
#define MCP2210_CMD_GET_SPI_SRAM	0x41
#define MCP2210_CMD_SPI_TRANSFER	0x42
#define MCP2210_CMD_READ_EEPROM		0x50
#define MCP2210_CMD_WRITE_EEPROM	0x51
#define MCP2210_CMD_SET_NVRAM		0x60
#define MCP2210_CMD_GET_NVRAM		0x61
#define MCP2210_CMD_SEND_PASSWORD	0x70
#define MCP2210_CMD_SPI_BUS_RELEASE	0x80

#define UMCPMIO_CMD_COMPLETE_OK		0x00

#define MCP2210_CMD_COMPLETE_OK		0x00
#define MCP2210_CMD_SPI_BUS_UNAVAIL	0xF7
#define MCP2210_CMD_USB_TRANSFER_IP	0xF8
#define MCP2210_CMD_SPI_TRANSFER_IP	MCP2210_CMD_USB_TRANSFER_IP
#define MCP2210_CMD_UNKNOWN		0xF9
#define MCP2210_CMD_EEPROM_FAIL		0xFA
#define MCP2210_CMD_EEPROM_LOCKED	0xFB
#define MCP2210_CMD_ACCESS_REJECTED	0xFC
#define MCP2210_CMD_ACCESS_DENIED	0xFD
#define MCP2210_CMD_BLOCKED_ACCESS	MCP2210_CMD_EEPROM_LOCKED

struct mcp2210_status_req {
	uint8_t		cmd; /* MCP2210_CMD_STATUS */
	uint8_t		reserved[63];
};

#define MCP2221_I2C_SPEED_SET		0x20
#define MCP2221_I2C_SPEED_BUSY		0x21
#define MCP2221_ENGINE_T1_MASK_NACK	0x40

struct mcp2210_status_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		spi_bus_release;
	uint8_t		spi_bus_owner;
	uint8_t		attempted_password_tries;
	uint8_t		password_guessed;
	uint8_t		dontcare[58];
};

struct mcp2210_cancel_spi_req {
	uint8_t		cmd; /* MCP2210_CMD_SPI_CANCEL */
	uint8_t		reserved[63];
};

struct mcp2210_cancel_spi_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		spi_bus_release;
	uint8_t		spi_bus_owner;
	uint8_t		attempted_password_tries;
	uint8_t		password_guessed;
	uint8_t		dontcare[58];
};

#define MCP2210_COUNTER_RESET 0x00
#define MCP2210_COUNTER_RETAIN 0xff

struct mcp2210_get_gp6_events_req {
	uint8_t		cmd; /* MCP2210_CMD_GET_GP6_EVENTS */
	uint8_t		reset_counter;
	uint8_t		reserved[62];
};

struct mcp2210_get_gp6_events_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		dontcare[2];
	uint8_t		counter_lsb;
	uint8_t		counter_msb;
	uint8_t		dontcare2[58];
};

struct mcp2210_get_gpio_sram_req {
	uint8_t		cmd; /* MCP2210_CMD_GET_GPIO_SRAM */
	uint8_t		reserved[63];
};

#define MCP2210_PIN_IS_GPIO	0x00
#define MCP2210_PIN_IS_ALT0	0x01
#define MCP2210_PIN_IS_DED	0x02

#define MCP2210_COUNTER_OFF		0x00
#define MCP2210_COUNTER_FALLING_EDGE	0x01
#define MCP2210_COUNTER_RISING_EDGE	0x02
#define MCP2210_COUNTER_LOW_PULSE	0x03
#define MCP2210_COUNTER_HIGH_PULSE	0x04

struct mcp2210_get_gpio_sram_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		dontcare[2];
#define MCP2210_GPIO_SRAM_GP0 4
	uint8_t		gp0_designation;
	uint8_t		gp1_designation;
	uint8_t		gp2_designation;
	uint8_t		gp3_designation;
	uint8_t		gp4_designation;
	uint8_t		gp5_designation;
	uint8_t		gp6_designation;
	uint8_t		gp7_designation;
	uint8_t		gp8_designation;
	uint8_t		default_output_lsb;
	uint8_t		default_output_msb;
	uint8_t		default_direction_lsb;
	uint8_t		default_direction_msb;
	uint8_t		other_settings;
	uint8_t		nvram_protection;
	uint8_t		dontcare2[45];
};

struct mcp2210_set_gpio_sram_req {
	uint8_t		cmd; /* MCP2210_CMD_SET_GPIO_SRAM */
	uint8_t		reserved[3];
	uint8_t		gp0_designation;
	uint8_t		gp1_designation;
	uint8_t		gp2_designation;
	uint8_t		gp3_designation;
	uint8_t		gp4_designation;
	uint8_t		gp5_designation;
	uint8_t		gp6_designation;
	uint8_t		gp7_designation;
	uint8_t		gp8_designation;
	uint8_t		default_output_lsb;
	uint8_t		default_output_msb;
	uint8_t		default_direction_lsb;
	uint8_t		default_direction_msb;
	uint8_t		other_settings;
	uint8_t		reserved2[46];
};

struct mcp2210_set_gpio_sram_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		dontcare[62];
};

struct mcp2210_set_spi_sram_req {
	uint8_t		cmd; /* MCP2210_CMD_SET_SPI_SRAM */
	uint8_t		reserved[3];
	uint8_t		bit_rate_byte_3; /* lsb */
	uint8_t		bit_rate_byte_2;
	uint8_t		bit_rate_byte_1;
	uint8_t		bit_rate_byte_0; /* msb */
	uint8_t		idle_cs_value_lsb;
	uint8_t		idle_cs_value_msb;
	uint8_t		active_cs_value_lsb;
	uint8_t		active_cs_value_msb;
	uint8_t		cs_to_data_delay_lsb;
	uint8_t		cs_to_data_delay_msb;
	uint8_t		lb_to_cs_deassert_delay_lsb;
	uint8_t		lb_to_cs_deassert_delay_msb;
	uint8_t		delay_between_bytes_lsb;
	uint8_t		delay_between_bytes_msb;
	uint8_t		bytes_per_spi_transaction_lsb;
	uint8_t		bytes_per_spi_transaction_msb;
	uint8_t		spi_mode;
	uint8_t		dontcare[43];
};

struct mcp2210_set_spi_sram_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		dontcare[2];
	uint8_t		bit_rate_byte_3; /* lsb */
	uint8_t		bit_rate_byte_2;
	uint8_t		bit_rate_byte_1;
	uint8_t		bit_rate_byte_0; /* msb */
	uint8_t		idle_cs_value_lsb;
	uint8_t		idle_cs_value_msb;
	uint8_t		active_cs_value_lsb;
	uint8_t		active_cs_value_msb;
	uint8_t		cs_to_data_delay_lsb;
	uint8_t		cs_to_data_delay_msb;
	uint8_t		lb_to_cs_deassert_delay_lsb;
	uint8_t		lb_to_cs_deassert_delay_msb;
	uint8_t		delay_between_bytes_lsb;
	uint8_t		delay_between_bytes_msb;
	uint8_t		bytes_per_spi_transaction_lsb;
	uint8_t		bytes_per_spi_transaction_msb;
	uint8_t		spi_mode;
	uint8_t		dontcare2[43];
};

struct mcp2210_get_spi_sram_req {
	uint8_t		cmd; /* MCP2210_CMD_GET_SPI_SRAM */
	uint8_t		reserved[63];
};

#define MCP2210_SPI_MODE_0	0x00
#define MCP2210_SPI_MODE_1	0x01
#define MCP2210_SPI_MODE_2	0x02
#define MCP2210_SPI_MODE_3	0x03

struct mcp2210_get_spi_sram_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		size_spi_res;
	uint8_t		dontcare;
	uint8_t		bit_rate_byte_3; /* lsb */
	uint8_t		bit_rate_byte_2;
	uint8_t		bit_rate_byte_1;
	uint8_t		bit_rate_byte_0; /* msb */
	uint8_t		idle_cs_value_lsb;
	uint8_t		idle_cs_value_msb;
	uint8_t		active_cs_value_lsb;
	uint8_t		active_cs_value_msb;
	uint8_t		cs_to_data_delay_lsb;
	uint8_t		cs_to_data_delay_msb;
	uint8_t		lb_to_cs_deassert_delay_lsb;
	uint8_t		lb_to_cs_deassert_delay_msb;
	uint8_t		delay_between_bytes_lsb;
	uint8_t		delay_between_bytes_msb;
	uint8_t		bytes_per_spi_transaction_lsb;
	uint8_t		bytes_per_spi_transaction_msb;
	uint8_t		spi_mode;
	uint8_t		dontcare2[43];
};

struct mcp2210_spi_transfer_req {
	uint8_t		cmd; /* MCP2210_CMD_SPI_TRANSFER */
	uint8_t		num_send_bytes;
	uint8_t		reserved[2];
	uint8_t		send_bytes[60];
};

#define MCP2210_SPI_STATUS_DATA_DONE 0x10
#define MCP2210_SPI_STATUS_NO_DATA_YET 0x20
#define MCP2210_SPI_STATUS_DATA 0x30

struct mcp2210_spi_transfer_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		num_receive_bytes;
	uint8_t		spi_engine_status;
	uint8_t		receive_bytes[60];
};

struct mcp2210_set_gpio_value_req {
	uint8_t		cmd; /* MCP2210_CMD_SET_GPIO_VAL_SRAM */
	uint8_t		reserved[3];
	uint8_t		pin_value_lsb;
	uint8_t		pin_value_msb;
	uint8_t		reserved2[58];
};

struct mcp2210_set_gpio_value_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		dontcare[2];
	uint8_t		pin_value_lsb;
	uint8_t		pin_value_msb;
	uint8_t		dontcare2[58];
};

struct mcp2210_get_gpio_value_req {
	uint8_t		cmd; /* MCP2210_CMD_GET_GPIO_VAL_SRAM */
	uint8_t		reserved[63];
};

struct mcp2210_get_gpio_value_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		dontcare[2];
	uint8_t		pin_value_lsb;
	uint8_t		pin_value_msb;
	uint8_t		dontcare2[58];
};

struct mcp2210_set_gpio_dir_req {
	uint8_t		cmd; /* MCP2210_CMD_SET_GPIO_DIR_SRAM */
	uint8_t		reserved[3];
	uint8_t		pin_dir_lsb;
	uint8_t		pin_dir_msb;
	uint8_t		reserved2[58];
};

struct mcp2210_set_gpio_dir_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		dontcare[62];
};


struct mcp2210_get_gpio_dir_req {
	uint8_t		cmd; /* MCP2210_CMD_GET_GPIO_DIR_SRAM */
	uint8_t		reserved[63];
};

struct mcp2210_get_gpio_dir_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		dontcare[2];
	uint8_t		pin_dir_lsb;
	uint8_t		pin_dir_msb;
	uint8_t		dontcare2[58];
};

struct mcp2210_read_eeprom_req {
	uint8_t		cmd; /* MCP2210_CMD_READ_EEPROM */
	uint8_t		addr;
	uint8_t		dontcare[62];
};

struct mcp2210_read_eeprom_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		addr;
	uint8_t		value;
	uint8_t		dontcare[60];
};

struct mcp2210_write_eeprom_req {
	uint8_t		cmd; /* MCP2210_CMD_WRITE_EEPROM */
	uint8_t		addr;
	uint8_t		value;
	uint8_t		dontcare[61];
};

struct mcp2210_write_eeprom_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		dontcare[62];
};

#define MCP2210_NVRAM_SUBCODE_SPI		0x10
#define MCP2210_NVRAM_SUBCODE_CS		0x20
#define MCP2210_NVRAM_SUBCODE_USBKEYPARAMS	0x30
#define MCP2210_NVRAM_SUBCODE_USBPROD		0x40
#define MCP2210_NVRAM_SUBCODE_USBMAN		0x50

#define MCP2210_USBPOWER_SELF		0x40
#define MCP2210_USBPOWER_BUSS		0x80

struct mcp2210_set_nvram_req {
	uint8_t		cmd; /* MCP2210_CMD_SET_NVRAM */
	uint8_t		subcode;
	uint8_t		reserved[2];
	union {
		struct {
			uint8_t		bit_rate_byte_3; /* lsb */
			uint8_t		bit_rate_byte_2;
			uint8_t		bit_rate_byte_1;
			uint8_t		bit_rate_byte_0; /* msb */
			uint8_t		idle_cs_value_lsb;
			uint8_t		idle_cs_value_msb;
			uint8_t		active_cs_value_lsb;
			uint8_t		active_cs_value_msb;
			uint8_t		cs_to_data_delay_lsb;
			uint8_t		cs_to_data_delay_msb;
			uint8_t		lb_to_cs_deassert_delay_lsb;
			uint8_t		lb_to_cs_deassert_delay_msb;
			uint8_t		delay_between_bytes_lsb;
			uint8_t		delay_between_bytes_msb;
			uint8_t		bytes_per_spi_transaction_lsb;
			uint8_t		bytes_per_spi_transaction_msb;
			uint8_t		spi_mode;
			uint8_t		dontcare[43];
		} spi;
		struct {
			uint8_t		gp0_designation;
			uint8_t		gp1_designation;
			uint8_t		gp2_designation;
			uint8_t		gp3_designation;
			uint8_t		gp4_designation;
			uint8_t		gp5_designation;
			uint8_t		gp6_designation;
			uint8_t		gp7_designation;
			uint8_t		gp8_designation;
			uint8_t		default_output_lsb;
			uint8_t		default_output_msb;
			uint8_t		default_direction_lsb;
			uint8_t		default_direction_msb;
			uint8_t		other_settings;
			uint8_t		nvram_protection;
			uint8_t		password_byte_1;
			uint8_t		password_byte_2;
			uint8_t		password_byte_3;
			uint8_t		password_byte_4;
			uint8_t		password_byte_5;
			uint8_t		password_byte_6;
			uint8_t		password_byte_7;
			uint8_t		password_byte_8;
			uint8_t		dontcare[37];
		} cs;
		struct {
			uint8_t		lsb_usb_vid;
			uint8_t		msb_usb_vid;
			uint8_t		lsb_usb_pid;
			uint8_t		msb_usb_pid;
			uint8_t		usb_power_attributes;
			uint8_t		usb_requested_ma;
			uint8_t		dontcare3[54];
		} usbkeyparams;
		struct {
			uint8_t		total_length;
			uint8_t		always0x03;
			uint8_t		unicode_product_descriptor[58];
		} usbprod;
		struct {
			uint8_t		total_length;
			uint8_t		always0x03;
			uint8_t		unicode_man_descriptor[58];
		} usbman;
	} u;
};

struct mcp2210_set_nvram_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		subcode;
	uint8_t		dontcare[61];
};

struct mcp2210_get_nvram_req {
	uint8_t		cmd; /* MCP2210_CMD_GET_NVRAM */
	uint8_t		subcode;
	uint8_t		reserved[62];
};

struct mcp2210_get_nvram_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		subcode;
	uint8_t		dontcare;
	union {
		struct {
			uint8_t		bit_rate_byte_3; /* lsb */
			uint8_t		bit_rate_byte_2;
			uint8_t		bit_rate_byte_1;
			uint8_t		bit_rate_byte_0; /* msb */
			uint8_t		idle_cs_value_lsb;
			uint8_t		idle_cs_value_msb;
			uint8_t		active_cs_value_lsb;
			uint8_t		active_cs_value_msb;
			uint8_t		cs_to_data_delay_lsb;
			uint8_t		cs_to_data_delay_msb;
			uint8_t		lb_to_cs_deassert_delay_lsb;
			uint8_t		lb_to_cs_deassert_delay_msb;
			uint8_t		delay_between_bytes_lsb;
			uint8_t		delay_between_bytes_msb;
			uint8_t		bytes_per_spi_transaction_lsb;
			uint8_t		bytes_per_spi_transaction_msb;
			uint8_t		spi_mode;
			uint8_t		dontcare[43];
		} spi;
		struct {
			uint8_t		gp0_designation;
			uint8_t		gp1_designation;
			uint8_t		gp2_designation;
			uint8_t		gp3_designation;
			uint8_t		gp4_designation;
			uint8_t		gp5_designation;
			uint8_t		gp6_designation;
			uint8_t		gp7_designation;
			uint8_t		gp8_designation;
			uint8_t		default_output_lsb;
			uint8_t		default_output_msb;
			uint8_t		default_direction_lsb;
			uint8_t		default_direction_msb;
			uint8_t		other_settings;
			uint8_t		nvram_protection;
			uint8_t		dontcare[45];
		} cs;
		struct {
			uint8_t		dontcare[8];
			uint8_t		lsb_usb_vid;
			uint8_t		msb_usb_vid;
			uint8_t		lsb_usb_pid;
			uint8_t		msb_usb_pid;
			uint8_t		dontcare2[13];
			uint8_t		usb_power_attributes;
			uint8_t		usb_requested_ma;
			uint8_t		dontcare3[33];
		} usbkeyparams;
		struct {
			uint8_t		total_length;
			uint8_t		always0x03;
			uint8_t		unicode_product_descriptor[58];
		} usbprod;
		struct {
			uint8_t		total_length;
			uint8_t		always0x03;
			uint8_t		unicode_man_descriptor[58];
		} usbman;
	} u;
};

struct mcp2210_release_spi_bus_req {
	uint8_t		cmd; /* MCP2210_CMD_SPI_BUS_RELEASE */
	uint8_t		reserved[63];
};

struct mcp2210_release_spi_bus_res {
	uint8_t		cmd;
	uint8_t		completion;
	uint8_t		dontcare[62];
};

/* MCP-2221 / MCP-2221A stuff */

#define MCP2221_REQ_BUFFER_SIZE UMCPMIO_REQ_BUFFER_SIZE
#define MCP2221_RES_BUFFER_SIZE UMCPMIO_RES_BUFFER_SIZE

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
