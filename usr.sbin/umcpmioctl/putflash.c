/*	$NetBSD: putflash.c,v 1.2 2025/03/22 05:46:32 rillig Exp $	*/

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

#ifdef __RCSID
__RCSID("$NetBSD: putflash.c,v 1.2 2025/03/22 05:46:32 rillig Exp $");
#endif

/* Functions to parse stuff */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include <dev/usb/umcpmio_hid_reports.h>
#include <dev/usb/umcpmio_io.h>
#include <sys/ioctl.h>

#undef EXTERN
#define EXTERN
#include "putflash.h"


int
parse_flash_gp_req(int fd, struct mcp2221_put_flash_req *req, char *argv[], int start, int end, bool debug)
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

		uint8_t *bbuf = (uint8_t *)&current_flash.get_flash_res;
		if (debug) {
			fprintf(stderr, "CURRENT REQ:\n");
			for (int i = 0; i < MCP2221_RES_BUFFER_SIZE; i++) {
				fprintf(stderr, "%02x ", bbuf[i]);
			}
			fprintf(stderr, "\n");
		}

		/*
		 * When flash is put, you put ALL of a particular subcode, so
		 * you have to do a get + put
		 */

		req->u.gp.gp0_settings = current_flash.get_flash_res.u.gp.gp0_settings;
		req->u.gp.gp1_settings = current_flash.get_flash_res.u.gp.gp1_settings;
		req->u.gp.gp2_settings = current_flash.get_flash_res.u.gp.gp2_settings;
		req->u.gp.gp3_settings = current_flash.get_flash_res.u.gp.gp3_settings;

		if (debug)
			fprintf(stderr, "CURRENT FLASH: %02x %02x %02x %02x\n", current_flash.get_flash_res.u.gp.gp0_settings, current_flash.get_flash_res.u.gp.gp1_settings, current_flash.get_flash_res.u.gp.gp2_settings, current_flash.get_flash_res.u.gp.gp3_settings);

		while (argcount < end) {
			gp = NULL;
			if (strncmp(argv[argcount], "GP0", 4) == 0) {
				gp = (uint8_t *)&req->u.gp.gp0_settings;
			}
			if (strncmp(argv[argcount], "GP1", 4) == 0) {
				gp = (uint8_t *)&req->u.gp.gp1_settings;
			}
			if (strncmp(argv[argcount], "GP2", 4) == 0) {
				gp = (uint8_t *)&req->u.gp.gp2_settings;
			}
			if (strncmp(argv[argcount], "GP3", 4) == 0) {
				gp = (uint8_t *)&req->u.gp.gp3_settings;
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
