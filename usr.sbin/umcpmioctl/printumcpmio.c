/*	$NetBSD: printumcpmio.c,v 1.3 2025/03/22 06:09:48 rillig Exp $	*/

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

#include <sys/cdefs.h>
#ifdef __RCSID
__RCSID("$NetBSD: printumcpmio.c,v 1.3 2025/03/22 06:09:48 rillig Exp $");
#endif

/* Functions to print stuff */

#include <stdio.h>

#include <dev/usb/umcpmio_hid_reports.h>

#undef EXTERN
#define EXTERN
#include "printumcpmio.h"

/* This is all cheaply done */

void
print_status(struct mcp2221_status_res *r)
{
	uint8_t *br = (uint8_t *)r;

	const char *outputs[] = {
		"cmd:\t\t\t\t",
		"completion:\t\t\t",
		"cancel_transfer:\t\t",
		"set_i2c_speed:\t\t",
		"i2c_clock_divider:\t\t",
		NULL,
		NULL,
		NULL,
		"internal_i2c_state:\t\t",
		"lsb_i2c_req_len:\t\t",
		"msb_i2c_req_len:\t\t",
		"lsb_i2c_trans_len:\t\t",
		"msb_i2c_trans_len:\t\t",
		"internal_i2c_bcount:\t\t",
		"i2c_speed_divider:\t\t",
		"i2c_timeout_value:\t\t",
		"lsb_i2c_address:\t\t",
		"msb_i2c_address:\t\t",
		NULL,
		NULL,
		"internal_i2c_state20:\t",
		NULL,
		"scl_line_value:\t\t",
		"sda_line_value:\t\t",
		"interrupt_edge_state:\t",
		"i2c_read_pending:\t\t",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		"mcp2221_hardware_rev_major:\t",
		"mcp2221_hardware_rev_minor:\t",
		"mcp2221_firmware_rev_major:\t",
		"mcp2221_firmware_rev_minor:\t",
		"adc_channel0_lsb:\t\t",
		"adc_channel0_msb:\t\t",
		"adc_channel1_lsb:\t\t",
		"adc_channel1_msb:\t\t",
		"adc_channel2_lsb:\t\t",
		"adc_channel2_msb:\t\t",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	};

	for (int n = 0; n < MCP2221_RES_BUFFER_SIZE; n++) {
		if (outputs[n] != NULL)
			printf("%02d:%s%d (0x%02X)\n", n, outputs[n], br[n], br[n]);
	}
}

void
print_sram(struct mcp2221_get_sram_res *r)
{
	uint8_t *br = (uint8_t *)r;

	const char *outputs[] = {
		"cmd:\t\t\t\t",
		"completion:\t\t\t",
		"len_chip_setting:\t\t",
		"len_gpio_setting:\t\t",
		"sn_initial_ps_cs:\t\t",
		"clock_divider:\t\t",
		"dac_reference_voltage:\t",
		"irq_adc_reference_voltage:\t",
		"lsb_usb_vid:\t\t\t",
		"msb_usb_vid:\t\t\t",
		"lsb_usb_pid:\t\t\t",
		"msb_usb_pid:\t\t\t",
		"usb_power_attributes:\t",
		"usb_requested_ma:\t\t",
		"current_password_byte_1:\t",
		"current_password_byte_2:\t",
		"current_password_byte_3:\t",
		"current_password_byte_4:\t",
		"current_password_byte_5:\t",
		"current_password_byte_6:\t",
		"current_password_byte_7:\t",
		"current_password_byte_8:\t",
		"gp0_settings:\t\t",
		"gp1_settings:\t\t",
		"gp2_settings:\t\t",
		"gp3_settings:\t\t",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	};

	for (int n = 0; n < MCP2221_RES_BUFFER_SIZE; n++) {
		if (outputs[n] != NULL)
			printf("%02d:%s%d (0x%02X)\n", n, outputs[n], br[n], br[n]);
	}
}

void
print_gpio_cfg(struct mcp2221_get_gpio_cfg_res *r)
{
	uint8_t *br = (uint8_t *)r;

	const char *outputs[] = {
		"cmd:\t\t\t",
		"completion:\t\t",
		"gp0_pin_value:\t",
		"gp0_pin_dir:\t\t",
		"gp1_pin_value:\t",
		"gp1_pin_dir:\t\t",
		"gp2_pin_value:\t",
		"gp2_pin_dir:\t\t",
		"gp3_pin_value:\t",
		"gp3_pin_dir:\t\t",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	};

	for (int n = 0; n < MCP2221_RES_BUFFER_SIZE; n++) {
		if (outputs[n] != NULL)
			printf("%02d:%s%d (0x%02X)\n", n, outputs[n], br[n], br[n]);
	}
}

void
print_flash(struct mcp2221_get_flash_res *r, int subcode)
{
	uint8_t *br = (uint8_t *)r;

	const char *outputs1[] = {
		"cmd:\t\t\t\t",
		"completion:\t\t\t",
		"res_len:\t\t\t"
	};

	const char *outputs2[][64] = {
		{
			NULL,
			"uartenum_led_protection:\t",
			"clock_divider:\t\t",
			"dac_reference_voltage:\t",
			"irq_adc_reference_voltage:\t",
			"lsb_usb_vid:\t\t\t",
			"msb_usb_vid:\t\t\t",
			"lsb_usb_pid:\t\t\t",
			"msb_usb_pid:\t\t\t",
			"usb_power_attributes:\t",
			"usb_requested_ma:\t\t",
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL
		},
		{
			NULL,
			"gp0_settings:\t\t",
			"gp1_settings:\t\t",
			"gp2_settings:\t\t",
			"gp3_settings:\t\t",
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL
		},
		{
			"always0x03:\t\t\t\t",
			"unicode_man_descriptor"
		},
		{
			"always0x03:\t\t\t\t\t",
			"unicode_product_descriptor"
		},
		{
			"always0x03:\t\t\t\t",
			"unicode_serial_number"
		},
		{
			"always0x03:\t\t\t\t",
			"factory_serial_number"
		},
	};

	int n = 0;
	for (; n <= 2; n++) {
		if (outputs1[n] != NULL)
			printf("%02d:%s%d (0x%02X)\n", n, outputs1[n], br[n], br[n]);
	}

	if (subcode == 0 ||
	    subcode == 1) {
		for (; n < MCP2221_RES_BUFFER_SIZE; n++) {
			if (outputs2[subcode][n - 3] != NULL)
				printf("%02d:%s%d (0x%02X)\n", n, outputs2[subcode][n - 3], br[n], br[n]);
		}
	} else {
		int c = 1;
		int l = br[2];
		printf("%02d:%s%d (0x%02X)\n", n, outputs2[subcode][n - 3], br[n], br[n]);
		n++;
		for (c = 1; c <= l; c++) {
			printf("%02d:%s%02d:\t\t%d (0x%02X)\n", n, outputs2[subcode][1], c, br[n], br[n]);
			n++;
		}
	}
}
