/*	$NetBSD: putflash.c,v 1.4 2025/11/29 18:39:15 brad Exp $	*/

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
__RCSID("$NetBSD: putflash.c,v 1.4 2025/11/29 18:39:15 brad Exp $");
#endif

/* Functions to parse stuff */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#include <dev/usb/umcpmio_hid_reports.h>
#include <dev/usb/umcpmio_io.h>
#include <sys/ioctl.h>

#undef EXTERN
#define EXTERN
#include "putflash.h"


int
mcp2221_parse_flash_gp_req(int fd, struct mcp2221_put_flash_req *req, char *argv[], int start, int end, bool debug)
{
	int error = 0;
	struct umcpmio_ioctl_get_flash current_flash;
	int arggood = false;

	current_flash.subcode = MCP2221_FLASH_SUBCODE_GP;
	error = ioctl(fd, UMCPMIO_GET_FLASH, &current_flash);

	if (debug)
		fprintf(stderr, "CURRENT FLASH: error=%d\n", error);

	if (!error) {
		int argcount = start;
		uint8_t *gp;

		uint8_t *bbuf = &current_flash.res.get_blob[0];
		if (debug) {
			fprintf(stderr, "CURRENT REQ:\n");
			for (int i = 0; i < MCP2221_RES_BUFFER_SIZE; i++){
				fprintf(stderr, "%02x ", bbuf[i]);
			}
			fprintf(stderr, "\n");
		}

		/* When flash is put, you put ALL of a particular subcode, so
		 * you have to do a get + put */
		req->u.gp.gp0_settings = current_flash.res.mcp2221_get_flash_res.u.gp.gp0_settings;
		req->u.gp.gp1_settings = current_flash.res.mcp2221_get_flash_res.u.gp.gp1_settings;
		req->u.gp.gp2_settings = current_flash.res.mcp2221_get_flash_res.u.gp.gp2_settings;
		req->u.gp.gp3_settings = current_flash.res.mcp2221_get_flash_res.u.gp.gp3_settings;

		if (debug)
			fprintf(stderr, "CURRENT FLASH: %02x %02x %02x %02x\n", current_flash.res.mcp2221_get_flash_res.u.gp.gp0_settings, current_flash.res.mcp2221_get_flash_res.u.gp.gp1_settings, current_flash.res.mcp2221_get_flash_res.u.gp.gp2_settings, current_flash.res.mcp2221_get_flash_res.u.gp.gp3_settings);

		while (argcount < end) {
			gp = NULL;
			if (strncmp(argv[argcount], "GP0", 4) == 0) {
				gp = (uint8_t *) & req->u.gp.gp0_settings;
			}
			if (strncmp(argv[argcount], "GP1", 4) == 0) {
				gp = (uint8_t *) & req->u.gp.gp1_settings;
			}
			if (strncmp(argv[argcount], "GP2", 4) == 0) {
				gp = (uint8_t *) & req->u.gp.gp2_settings;
			}
			if (strncmp(argv[argcount], "GP3", 4) == 0) {
				gp = (uint8_t *) & req->u.gp.gp3_settings;
			}
			if (gp == NULL) {
				if (debug)
					fprintf(stderr, "NOT GPn: %d %s\n", argcount, argv[argcount]);
				error = EINVAL;
				break;
			}
			argcount++;
			if (argcount < end) {
				arggood = false;
				if (strncmp(argv[argcount], "GPIO_PIN_INPUT", 15) == 0) {
					*gp &= MCP2221_FLASH_GPIO_VALUE_MASK;
					*gp |= MCP2221_FLASH_GPIO_INPUT;
					arggood = true;
				}
				if (strncmp(argv[argcount], "GPIO_PIN_OUTPUT", 16) == 0) {
					*gp &= MCP2221_FLASH_GPIO_VALUE_MASK;
					arggood = true;
				}
				if (strncmp(argv[argcount], "GPIO_PIN_ALT0", 14) == 0) {
					*gp &= (MCP2221_FLASH_GPIO_VALUE_MASK | MCP2221_FLASH_GPIO_INPUT);
					*gp &= ~MCP2221_FLASH_PIN_TYPE_MASK;
					*gp |= MCP2221_FLASH_PIN_IS_ALT0;
					arggood = true;
				}
				if (strncmp(argv[argcount], "GPIO_PIN_ALT1", 14) == 0) {
					*gp &= (MCP2221_FLASH_GPIO_VALUE_MASK | MCP2221_FLASH_GPIO_INPUT);
					*gp &= ~MCP2221_FLASH_PIN_TYPE_MASK;
					*gp |= MCP2221_FLASH_PIN_IS_ALT1;
					arggood = true;
				}
				if (strncmp(argv[argcount], "GPIO_PIN_ALT2", 14) == 0) {
					*gp &= (MCP2221_FLASH_GPIO_VALUE_MASK | MCP2221_FLASH_GPIO_INPUT);
					*gp &= ~MCP2221_FLASH_PIN_TYPE_MASK;
					*gp |= MCP2221_FLASH_PIN_IS_ALT2;
					arggood = true;
				}
				if (strncmp(argv[argcount], "GPIO_PIN_ALT3", 14) == 0) {
					*gp &= (MCP2221_FLASH_GPIO_VALUE_MASK | MCP2221_FLASH_GPIO_INPUT);
					*gp &= ~MCP2221_FLASH_PIN_TYPE_MASK;
					*gp |= MCP2221_FLASH_PIN_IS_DED;
					arggood = true;
				}
				if (strncmp(argv[argcount], "DEFAULT_OUTPUT_ZERO", 20) == 0) {
					*gp &= ~MCP2221_FLASH_GPIO_VALUE_MASK;
					arggood = true;
				}
				if (strncmp(argv[argcount], "DEFAULT_OUTPUT_ONE", 19) == 0) {
					*gp |= MCP2221_FLASH_GPIO_VALUE_MASK;
					arggood = true;
				}
				if (!arggood) {
					if (debug)
						fprintf(stderr, "BAD ARGUMENT: %d %s\n", argcount, argv[argcount]);
					error = EINVAL;
					break;
				}
			} else {
				error = EINVAL;
			}

			argcount++;
		}
	}
	return (error);
}

int
mcp2210_parse_flash_gp_req(int fd, struct mcp2210_set_nvram_req *req, char *argv[], int start, int end, bool debug)
{
	int error = 0;
	struct umcpmio_ioctl_get_flash current_flash;
	int arggood = false;

	current_flash.subcode = MCP2210_NVRAM_SUBCODE_CS;
	error = ioctl(fd, UMCPMIO_GET_FLASH, &current_flash);

	if (debug)
		fprintf(stderr, "CURRENT FLASH: error=%d\n", error);

	if (!error) {
		int argcount = start;
		uint8_t *gp;

		uint8_t *bbuf = &current_flash.res.get_blob[0];
		if (debug) {
			fprintf(stderr, "CURRENT REQ:\n");
			for (int i = 0; i < MCP2221_RES_BUFFER_SIZE; i++){
				fprintf(stderr, "%02x ", bbuf[i]);
			}
			fprintf(stderr, "\n");
		}

		/* This needs to be a read / modify / write sort of thing */
		memset(req, 0, MCP2210_REQ_BUFFER_SIZE);
		memcpy(&req->u.cs.gp0_designation,
		    &current_flash.res.mcp2210_get_nvram_res.u.cs.gp0_designation,
		    &current_flash.res.mcp2210_get_nvram_res.u.cs.nvram_protection -
		    &current_flash.res.mcp2210_get_nvram_res.u.cs.gp0_designation + 1);

		if (debug)
			fprintf(stderr, "CURRENT FLASH: DESIGNATION: %02x %02x %02x %02x %02x %02x %02x %02x %02x"
			    "- DIRECTION: %02x %02x - VALUE: %02x %02x - OTHER_SETTINGS: %02x\n",
			    current_flash.res.mcp2210_get_nvram_res.u.cs.gp0_designation,
			    current_flash.res.mcp2210_get_nvram_res.u.cs.gp1_designation,
			    current_flash.res.mcp2210_get_nvram_res.u.cs.gp2_designation,
			    current_flash.res.mcp2210_get_nvram_res.u.cs.gp3_designation,
			    current_flash.res.mcp2210_get_nvram_res.u.cs.gp4_designation,
			    current_flash.res.mcp2210_get_nvram_res.u.cs.gp5_designation,
			    current_flash.res.mcp2210_get_nvram_res.u.cs.gp6_designation,
			    current_flash.res.mcp2210_get_nvram_res.u.cs.gp7_designation,
			    current_flash.res.mcp2210_get_nvram_res.u.cs.gp8_designation,
			    current_flash.res.mcp2210_get_nvram_res.u.cs.default_direction_lsb,
			    current_flash.res.mcp2210_get_nvram_res.u.cs.default_direction_msb,
			    current_flash.res.mcp2210_get_nvram_res.u.cs.default_output_lsb,
			    current_flash.res.mcp2210_get_nvram_res.u.cs.default_output_msb,
			    current_flash.res.mcp2210_get_nvram_res.u.cs.other_settings);

		int pin;
		while (argcount < end) {
			gp = NULL;
			pin = -1;
			if (strncmp(argv[argcount], "GP0", 4) == 0) {
				gp = (uint8_t *) & req->u.cs.gp0_designation;
				pin = 0;
			}
			if (strncmp(argv[argcount], "GP1", 4) == 0) {
				gp = (uint8_t *) & req->u.cs.gp1_designation;
				pin = 1;
			}
			if (strncmp(argv[argcount], "GP2", 4) == 0) {
				gp = (uint8_t *) & req->u.cs.gp2_designation;
				pin = 2;
			}
			if (strncmp(argv[argcount], "GP3", 4) == 0) {
				gp = (uint8_t *) & req->u.cs.gp3_designation;
				pin = 3;
			}
			if (strncmp(argv[argcount], "GP4", 4) == 0) {
				gp = (uint8_t *) & req->u.cs.gp4_designation;
				pin = 4;
			}
			if (strncmp(argv[argcount], "GP5", 4) == 0) {
				gp = (uint8_t *) & req->u.cs.gp5_designation;
				pin = 5;
			}
			if (strncmp(argv[argcount], "GP6", 4) == 0) {
				gp = (uint8_t *) & req->u.cs.gp6_designation;
				pin = 6;
			}
			if (strncmp(argv[argcount], "GP7", 4) == 0) {
				gp = (uint8_t *) & req->u.cs.gp7_designation;
				pin = 7;
			}
			if (strncmp(argv[argcount], "GP8", 4) == 0) {
				gp = (uint8_t *) & req->u.cs.gp8_designation;
				pin = 8;
			}
			if (gp == NULL) {
				if (debug)
					fprintf(stderr, "NOT GPn: %d %s\n", argcount, argv[argcount]);
				error = EINVAL;
				break;
			}
			argcount++;
			if (argcount < end) {
				arggood = false;
				if (strncmp(argv[argcount], "GPIO_PIN_INPUT", 15) == 0) {
					*gp = MCP2210_PIN_IS_GPIO;
					if (pin < 8) {
						req->u.cs.default_direction_lsb |= (1 << pin);
					}
					arggood = true;
				}
				if (strncmp(argv[argcount], "GPIO_PIN_OUTPUT", 16) == 0) {
					*gp = MCP2210_PIN_IS_GPIO;
					if (pin < 8) {
						req->u.cs.default_direction_lsb &= ~(1 << pin);
					}
					arggood = true;
				}
				if (strncmp(argv[argcount], "GPIO_PIN_ALT0", 14) == 0) {
					*gp = MCP2210_PIN_IS_ALT0;
					arggood = true;
				}
				if (strncmp(argv[argcount], "GPIO_PIN_ALT3", 14) == 0) {
					*gp = MCP2210_PIN_IS_DED;
					req->u.cs.other_settings &= 0xf1;
					req->u.cs.other_settings |= MCP2210_COUNTER_FALLING_EDGE << 1;
					arggood = true;
				}
				if (pin == 6) {
					if (strncmp(argv[argcount], "GPIO_PIN_ALT4", 14) == 0) {
						*gp = MCP2210_PIN_IS_DED;
						req->u.cs.other_settings &= 0xf1;
						req->u.cs.other_settings |= MCP2210_COUNTER_RISING_EDGE << 1;
						arggood = true;
					}
					if (strncmp(argv[argcount], "GPIO_PIN_ALT5", 14) == 0) {
						*gp = MCP2210_PIN_IS_DED;
						req->u.cs.other_settings &= 0xf1;
						req->u.cs.other_settings |= MCP2210_COUNTER_LOW_PULSE << 1;
						arggood = true;
					}
					if (strncmp(argv[argcount], "GPIO_PIN_ALT6", 14) == 0) {
						*gp = MCP2210_PIN_IS_DED;
						req->u.cs.other_settings &= 0xf1;
						req->u.cs.other_settings |= MCP2210_COUNTER_HIGH_PULSE << 1;
						arggood = true;
					}
				}
				if (strncmp(argv[argcount], "DEFAULT_OUTPUT_ZERO", 20) == 0) {
					if (pin < 8) {
						req->u.cs.default_output_lsb &= ~(1 << pin);
					}
					arggood = true;
				}
				if (strncmp(argv[argcount], "DEFAULT_OUTPUT_ONE", 19) == 0) {
					if (pin < 8) {
						req->u.cs.default_output_lsb |= (1 << pin);
					}
					arggood = true;
				}
				if (!arggood) {
					if (debug)
						fprintf(stderr, "BAD ARGUMENT: %d %s\n", argcount, argv[argcount]);
					error = EINVAL;
					break;
				}
			} else {
				error = EINVAL;
			}

			argcount++;
		}
	}
	return (error);
}

int
mcp2210_parse_flash_spi_req(int fd, struct mcp2210_set_nvram_req *req, char *argv[], int start, int end, bool debug)
{
	int error = 0;
	struct umcpmio_ioctl_get_flash current_flash;

	current_flash.subcode = MCP2210_NVRAM_SUBCODE_SPI;
	error = ioctl(fd, UMCPMIO_GET_FLASH, &current_flash);

	if (debug)
		fprintf(stderr, "CURRENT FLASH: error=%d\n", error);

	if (!error) {
		int argcount = start;
		uint8_t *p_lsb;
		uint8_t *p_msb;

		uint8_t *bbuf = &current_flash.res.get_blob[0];
		if (debug) {
			fprintf(stderr, "CURRENT REQ:\n");
			for (int i = 0; i < MCP2221_RES_BUFFER_SIZE; i++){
				fprintf(stderr, "%02x ", bbuf[i]);
			}
			fprintf(stderr, "\n");
		}

		/* This needs to be a read / modify / write sort of thing */
		memcpy(req, &current_flash.res, MCP2210_REQ_BUFFER_SIZE);

		while (argcount < end) {
			p_lsb = p_msb = NULL;
			if (strncmp(argv[argcount], "CS_TO_DATA_DELAY", 17) == 0) {
				p_lsb = (uint8_t *) & req->u.spi.cs_to_data_delay_lsb;
				p_msb = (uint8_t *) & req->u.spi.cs_to_data_delay_msb;
			}
			if (strncmp(argv[argcount], "LAST_BYTE_TO_CS_DELAY", 22) == 0) {
				p_lsb = (uint8_t *) & req->u.spi.lb_to_cs_deassert_delay_lsb;
				p_msb = (uint8_t *) & req->u.spi.lb_to_cs_deassert_delay_msb;
			}
			if (strncmp(argv[argcount], "DELAY_BETWEEN_BYTES", 20) == 0) {
				p_lsb = (uint8_t *) & req->u.spi.delay_between_bytes_lsb;
				p_msb = (uint8_t *) & req->u.spi.delay_between_bytes_msb;
			}
			if (p_lsb == NULL) {
				if (debug)
					fprintf(stderr, "BAD ARGUMENT: %d %s\n", argcount, argv[argcount]);
				error = EINVAL;
				break;
			}
			argcount++;
			if (argcount < end) {
				uint16_t v;
				v = (uint16_t) strtoi(argv[argcount], NULL, 0, 0, 0xffff, &error);
				if (error) {
					if (debug)
						fprintf(stderr, "BAD CONVERSION OF uint16_t: %d %s\n", argcount, argv[argcount]);
					error = EINVAL;
					break;
				}
				*p_lsb = (uint8_t)(v & 0x00ff);
				*p_msb = (uint8_t)((v >> 8) & 0x00ff);
			} else {
				error = EINVAL;
			}

			argcount++;
		}
	}
	return error;
}

int
mcp2210_parse_flash_usbkeyparams_req(int fd, struct mcp2210_set_nvram_req *req, char *argv[], int start, int end, bool debug)
{
	int error = 0;
	struct umcpmio_ioctl_get_flash current_flash;

	current_flash.subcode = MCP2210_NVRAM_SUBCODE_USBKEYPARAMS;
	error = ioctl(fd, UMCPMIO_GET_FLASH, &current_flash);

	if (debug)
		fprintf(stderr, "CURRENT FLASH: error=%d\n", error);

	if (!error) {
		int argcount = start;

		uint8_t *bbuf = &current_flash.res.get_blob[0];
		if (debug) {
			fprintf(stderr, "CURRENT REQ:\n");
			for (int i = 0; i < MCP2221_RES_BUFFER_SIZE; i++){
				fprintf(stderr, "%02x ", bbuf[i]);
			}
			fprintf(stderr, "\n");
		}

		/* This needs to be a read / modify / write sort of thing */
		memset(req, 0, MCP2210_REQ_BUFFER_SIZE);
		req->u.usbkeyparams.usb_power_attributes =
		    current_flash.res.mcp2210_get_nvram_res.u.usbkeyparams.usb_power_attributes;
		req->u.usbkeyparams.usb_requested_ma =
		    current_flash.res.mcp2210_get_nvram_res.u.usbkeyparams.usb_requested_ma;

		bbuf = &req->cmd;
		if (debug) {
			fprintf(stderr, "COPY REQ:\n");
			for (int i = 0; i < MCP2221_RES_BUFFER_SIZE; i++){
				fprintf(stderr, "%02x ", bbuf[i]);
			}
			fprintf(stderr, "\n");
		}
		bool powered = false;
		bool ma = false;
		while (argcount < end) {
			powered = ma = false;
			if (strncmp(argv[argcount], "POWERED", 8) == 0) {
				powered = true;
			}
			if (strncmp(argv[argcount], "MA", 3) == 0) {
				ma = true;
			}
			if (powered == false &&
			    ma == false) {
				if (debug)
					fprintf(stderr, "BAD ARGUMENT: %d %s\n", argcount, argv[argcount]);
				error = EINVAL;
				break;
			}
			argcount++;
			if (argcount < end) {
				if (powered) {
					uint8_t power_type = 0;
					if (strncmp(argv[argcount], "SELF", 5) == 0) {
						power_type = MCP2210_USBPOWER_SELF;
					}
					if (strncmp(argv[argcount], "BUS", 4) == 0) {
						power_type = MCP2210_USBPOWER_BUSS;
					}
					if (power_type == 0) {
						if (debug)
							fprintf(stderr, "BAD ARGUMENT: %s\n", argv[argcount]);
						error = EINVAL;
						break;
					}
					if (debug)
						fprintf(stderr, "REQUESTED power_type=%02x\n", power_type);
					req->u.usbkeyparams.usb_power_attributes &=
					    ~(MCP2210_USBPOWER_SELF | MCP2210_USBPOWER_BUSS);
					req->u.usbkeyparams.usb_power_attributes |= power_type;
				}
				if (ma) {
					uint16_t power_ma = 0;
					uint8_t pm;

					power_ma = (uint16_t) strtoi(argv[argcount], NULL, 0, 1, 511, &error);
					if (error) {
						if (debug)
							fprintf(stderr, "BAD CONVERSION OF MA 1 - 511: %d %s\n", argcount, argv[argcount]);
						error = EINVAL;
						break;
					}
					if (debug)
						fprintf(stderr, "REQUESTED power_ma=%d\n", power_ma);
					pm = (uint8_t)(power_ma / 2);
					if (pm == 0)
						pm = 1;
					req->u.usbkeyparams.usb_requested_ma = pm;
				}
			} else {
				error = EINVAL;
			}

			argcount++;
		}
	}
	return error;
}
