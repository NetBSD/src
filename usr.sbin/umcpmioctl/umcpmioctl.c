/*	$NetBSD: umcpmioctl.c,v 1.7 2025/11/29 18:39:15 brad Exp $	*/

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
__RCSID("$NetBSD: umcpmioctl.c,v 1.7 2025/11/29 18:39:15 brad Exp $");
#endif

/* Main userland program that can pull the SRAM and FLASH content from a
 * MCP-2210 or MCP-2221 / MCP-2221A chip using umcpmio(4).
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <dev/usb/umcpmio_hid_reports.h>
#include <dev/usb/umcpmio_io.h>

#define EXTERN extern
#include "umcpmioctl.h"
#include "umcpmioctlconst.h"
#include "printumcpmio.h"
#include "putflash.h"

__dead static void
usage(int status)
{
	const char *p = getprogname();

	fprintf(stderr, "Usage: %s [-dh] device cmd args\n\n",
	    p);

	for (long unsigned int i = 0; i < __arraycount(umcpmioctlcmds); i++){
		switch (umcpmioctlcmds[i].id) {
		case UMCPMIO_GET:
			for (long unsigned int j = 0; j < __arraycount(getsubcmds); j++){
				fprintf(stderr, "%s [-dh] device %s %s %s\n",
				    p, umcpmioctlcmds[i].cmd, getsubcmds[j].cmd, getsubcmds[j].helpargs);
				if (getsubcmds[j].id == UMCPMIO_IOCTL_GET_SRAM) {
					for (long unsigned int k = 0; k < __arraycount(getsubcmds); k++){
						fprintf(stderr, "%s [-dh] device %s %s %s %s\n",
						    p, umcpmioctlcmds[i].cmd, getsubcmds[j].cmd, getsubsubcmds[k].cmd, getsubsubcmds[k].helpargs);
					}
				}
			}
			break;
		case UMCPMIO_PUT:
			for (long unsigned int j = 0; j < __arraycount(putflashsubcmds); j++){
				fprintf(stderr, "%s [-dh] device %s flash %s %s\n",
				    p, umcpmioctlcmds[i].cmd, putflashsubcmds[j].cmd, putflashsubcmds[j].helpargs);
			}
			break;
		default:
			fprintf(stderr, "%s [-dh] device %s %s\n",
			    p, umcpmioctlcmds[i].cmd, umcpmioctlcmds[i].helpargs);
			break;
		};
	}

	fprintf(stderr, "\n");
	fprintf(stderr, "sram - The SRAM on the chip (MCP2221)\n");
	fprintf(stderr, "sram gpio - The GPIO pins functions (MCP2210)\n");
	fprintf(stderr, "sram gpio_values - The GPIO pins current values (MCP2210)\n");
	fprintf(stderr, "sram gpio_direction - The GPIO pins current direction (MCP2210)\n");
	fprintf(stderr, "sram spi - The current SPI parameters (MCP2210)\n");
	fprintf(stderr, "gp - The GPIO pin state and function (MCP2221)\n");
	fprintf(stderr, "cs - Chip Settings\n");
	fprintf(stderr, "usbkeyparams - USB parameters around power\n");
	fprintf(stderr, "usbman - USB Manufacturer Descriptor\n");
	fprintf(stderr, "usbprod - USB Product Descriptor\n");
	fprintf(stderr, "usbsn - USB Serial Number (MCP2221)\n");
	fprintf(stderr, "chipsn - Chip Serial Number (MCP2221)\n");
	exit(status);
}

static int
valid_cmd(const struct umcpmioctlcmd c[], long unsigned int csize, char *cmdtocheck)
{
	int r = -1;

	for (long unsigned int i = 0; i < csize; i++){
		if (strncmp(cmdtocheck, c[i].cmd, 16) == 0) {
			r = (int)i;
			break;
		}
	}

	return r;
}

int
main(int argc, char *argv[])
{
	int c;
	bool debug = false;
	int fd = -1, error = 0, valid, validsub = -1, validsubsub = -1;
	uint8_t chip_type;

	while ((c = getopt(argc, argv, "dh")) != -1) {
		switch (c) {
		case 'd':
			debug = true;
			break;
		case 'h':
			usage(0);
		default:
			usage(1);
		}
	}

	argc -= optind;
	argv += optind;

	if (debug) {
		fprintf(stderr, "ARGC: %d\n", argc);
		fprintf(stderr, "ARGV[0]: %s ; ARGV[1]: %s ; ARGV[2]: %s ; ARGV[3]: %s; ARGV[4]: %s; ARGV[5]: %s\n",
		    argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
	}
	if (argc <= 1)
		usage(0);

	fd = open(argv[0], O_RDWR, 0);
	if (fd == -1) {
		err(EXIT_FAILURE, "open %s", argv[0]);
	}
	error = ioctl(fd, UMCPMIO_CHIP_TYPE, &chip_type);
	if (error) {
		fprintf(stderr, "Could not get chip type\n");
		exit(1);
	}

	/* Parse out the command line into what the requested action is */
	valid = valid_cmd(umcpmioctlcmds, __arraycount(umcpmioctlcmds), argv[1]);
	if (valid == -1) {
		fprintf(stderr, "Unknown command: %s\n\n", argv[1]);
		usage(1);
	}
	uint8_t *buf;
	union umcpmio_ioctl_get_status status_res;
	struct mcp2221_get_sram_res get_sram_res;
	struct mcp2221_get_gpio_cfg_res get_gpio_cfg_res;
	struct umcpmio_ioctl_get_flash ioctl_get_flash;
	struct umcpmio_ioctl_put_flash ioctl_put_flash;
	struct mcp2210_ioctl_get_sram mcp2210_get_sram;
	struct mcp2210_cancel_spi_res cancel_res;

	switch (umcpmioctlcmds[valid].id) {
	case UMCPMIO_GET:
		if (argc < 3) {
			fprintf(stderr, "Missing arguments to get command\n\n");
			usage(1);
		}
		validsub = valid_cmd(getsubcmds, __arraycount(getsubcmds), argv[2]);
		if (validsub == -1) {
			fprintf(stderr, "Unknown subcommand to get: %s\n\n", argv[2]);
			usage(1);
		}
		switch (getsubcmds[validsub].id) {
		case UMCPMIO_IOCTL_GET_SRAM:
			if (chip_type == UMCPMIO_CHIP_TYPE_2210) {
				if (argc != 4) {
					fprintf(stderr, "Missing arguments to get sram command\n\n");
					usage(1);
				}
				validsubsub = valid_cmd(getsubsubcmds, __arraycount(getsubsubcmds), argv[3]);
				if (validsubsub == -1) {
					fprintf(stderr, "Unknown subcommand to get sram: %s %d\n\n", argv[3], validsubsub);
					usage(1);
				}
				switch (getsubsubcmds[validsubsub].id) {
				case MCP2210_IOCTL_GET_SRAM_GPIO:
					mcp2210_get_sram.cmd = MCP2210_CMD_GET_GPIO_SRAM;
					break;
				case MCP2210_IOCTL_GET_SRAM_GPIO_VAL:
					mcp2210_get_sram.cmd = MCP2210_CMD_GET_GPIO_VAL_SRAM;
					break;
				case MCP2210_IOCTL_GET_SRAM_GPIO_DIR:
					mcp2210_get_sram.cmd = MCP2210_CMD_GET_GPIO_DIR_SRAM;
					break;
				case MCP2210_IOCTL_GET_SRAM_SPI:
					mcp2210_get_sram.cmd = MCP2210_CMD_GET_SPI_SRAM;
					break;
				default:
					fprintf(stderr, "Unhandled subcommand to get sram: %s %d\n\n", argv[3], validsubsub);
					usage(1);
				}
				error = ioctl(fd, MCP2210_GET_SRAM, &mcp2210_get_sram);
			}
			if (chip_type == UMCPMIO_CHIP_TYPE_2221) {
				error = ioctl(fd, MCP2221_GET_SRAM, &get_sram_res);
			}
			break;
		case MCP2221_IOCTL_GET_GP_CFG:
			error = ioctl(fd, MCP2221_GET_GP_CFG, &get_gpio_cfg_res);
			break;
		case UMCPMIO_IOCTL_GET_FLASH:
			if (argc != 4) {
				fprintf(stderr, "Missing arguments to get flash command\n\n");
				usage(1);
			}
			validsubsub = valid_cmd(getflashsubcmds, __arraycount(getflashsubcmds), argv[3]);
			if (validsubsub == -1) {
				fprintf(stderr, "Unknown subcommand to get flash: %s %d\n\n", argv[3], validsubsub);
				usage(1);
			}
			if (chip_type == UMCPMIO_CHIP_TYPE_2210) {
				switch (getflashsubcmds[validsubsub].id) {
				case UMCPMIO_IOCTL_GET_NVRAM_SPI:
					ioctl_get_flash.subcode = MCP2210_NVRAM_SUBCODE_SPI;
					break;
				case UMCPMIO_IOCTL_GET_FLASH_GP:
				case UMCPMIO_IOCTL_GET_FLASH_CS:
					ioctl_get_flash.subcode = MCP2210_NVRAM_SUBCODE_CS;
					break;
				case UMCPMIO_IOCTL_GET_NVRAM_USBKEYPARAMS:
					ioctl_get_flash.subcode = MCP2210_NVRAM_SUBCODE_USBKEYPARAMS;
					break;
				case UMCPMIO_IOCTL_GET_FLASH_USBPROD:
					ioctl_get_flash.subcode = MCP2210_NVRAM_SUBCODE_USBPROD;
					break;
				case UMCPMIO_IOCTL_GET_FLASH_USBMAN:
					ioctl_get_flash.subcode = MCP2210_NVRAM_SUBCODE_USBMAN;
					break;
				default:
					fprintf(stderr, "Unhandled subcommand to get flash: %s %d\n\n", argv[3], validsubsub);
					usage(1);
				}
			}
			if (chip_type == UMCPMIO_CHIP_TYPE_2221) {
				switch (getflashsubcmds[validsubsub].id) {
				case UMCPMIO_IOCTL_GET_FLASH_CS:
					ioctl_get_flash.subcode = MCP2221_FLASH_SUBCODE_CS;
					break;
				case UMCPMIO_IOCTL_GET_FLASH_GP:
					ioctl_get_flash.subcode = MCP2221_FLASH_SUBCODE_GP;
					break;
				case UMCPMIO_IOCTL_GET_FLASH_USBMAN:
					ioctl_get_flash.subcode = MCP2221_FLASH_SUBCODE_USBMAN;
					break;
				case UMCPMIO_IOCTL_GET_FLASH_USBPROD:
					ioctl_get_flash.subcode = MCP2221_FLASH_SUBCODE_USBPROD;
					break;
				case UMCPMIO_IOCTL_GET_FLASH_USBSN:
					ioctl_get_flash.subcode = MCP2221_FLASH_SUBCODE_USBSN;
					break;
				case UMCPMIO_IOCTL_GET_FLASH_CHIPSN:
					ioctl_get_flash.subcode = MCP2221_FLASH_SUBCODE_CHIPSN;
					break;
				default:
					fprintf(stderr, "Unhandled subcommand to get flash: %s %d\n\n", argv[3], validsubsub);
					usage(1);
				}
			}
			error = ioctl(fd, UMCPMIO_GET_FLASH, &ioctl_get_flash);
			break;
		case UMCPMIO_IOCTL_GET_TYPE:
			break;
		default:
			fprintf(stderr, "Unhandled subcommand to get: %s %d\n\n", argv[2], validsub);
			usage(1);
		}
		break;
	case UMCPMIO_PUT:
		if (argc <= 3) {
			fprintf(stderr, "Missing arguments to put command\n\n");
			usage(1);
		}
		validsub = valid_cmd(putsubcmds, __arraycount(putsubcmds), argv[2]);
		if (validsub == -1) {
			fprintf(stderr, "Unknown subcommand to put: %s\n\n", argv[2]);
			usage(1);
		}
		switch (putsubcmds[validsub].id) {
		case UMCPMIO_IOCTL_PUT_FLASH:
			if (argc < 4) {
				fprintf(stderr, "Missing arguments to put flash command\n\n");
				usage(1);
			}
			validsubsub = valid_cmd(putflashsubcmds, __arraycount(putflashsubcmds), argv[3]);
			if (validsubsub == -1) {
				fprintf(stderr, "Unknown subcommand to put flash: %s %d\n\n", argv[3], validsubsub);
				usage(1);
			}
			memset(&ioctl_put_flash, 0, sizeof(ioctl_put_flash));

			if (chip_type == UMCPMIO_CHIP_TYPE_2210) {
				switch (putflashsubcmds[validsubsub].id) {
				case UMCPMIO_IOCTL_PUT_FLASH_GP:
					ioctl_put_flash.subcode = MCP2210_NVRAM_SUBCODE_CS;
					error = mcp2210_parse_flash_gp_req(fd, &ioctl_put_flash.req.mcp2210_set_req, argv, 4, argc, debug);

					if (debug) {
						fprintf(stderr, "REQ FOR FLASH GP PUT: error=%d:\n", error);
						buf = &ioctl_put_flash.req.put_req_blob[0];
						for (int i = 0; i < MCP2210_REQ_BUFFER_SIZE; i++){
							fprintf(stderr, " %02x", buf[i]);
						}
						fprintf(stderr, "\n----\n");
					}
					break;
				case MCP2210_IOCTL_PUT_NVRAM_SPI:
					ioctl_put_flash.subcode = MCP2210_NVRAM_SUBCODE_SPI;
					error = mcp2210_parse_flash_spi_req(fd, &ioctl_put_flash.req.mcp2210_set_req, argv, 4, argc, debug);

					if (debug) {
						fprintf(stderr, "REQ FOR NVRAM SPI PUT: error=%d:\n", error);
						buf = &ioctl_put_flash.req.put_req_blob[0];
						for (int i = 0; i < MCP2210_REQ_BUFFER_SIZE; i++){
							fprintf(stderr, " %02x", buf[i]);
						}
						fprintf(stderr, "\n----\n");
					}
					break;
				case MCP2210_IOCTL_PUT_NVRAM_USBKEYPARAMS:
					ioctl_put_flash.subcode = MCP2210_NVRAM_SUBCODE_USBKEYPARAMS;
					error = mcp2210_parse_flash_usbkeyparams_req(fd, &ioctl_put_flash.req.mcp2210_set_req, argv, 4, argc, debug);

					if (debug) {
						fprintf(stderr, "REQ FOR NVRAM USBKEYPARAMS PUT: error=%d:\n", error);
						buf = &ioctl_put_flash.req.put_req_blob[0];
						for (int i = 0; i < MCP2210_REQ_BUFFER_SIZE; i++){
							fprintf(stderr, " %02x", buf[i]);
						}
						fprintf(stderr, "\n----\n");
					}
					break;
				default:
					fprintf(stderr, "Unhandled subcommand to put flash: %s %d\n\n", argv[3], validsubsub);
					usage(1);
				}
			}
			if (chip_type == UMCPMIO_CHIP_TYPE_2221) {
				switch (putflashsubcmds[validsubsub].id) {
				case UMCPMIO_IOCTL_PUT_FLASH_GP:
					ioctl_put_flash.subcode = MCP2221_FLASH_SUBCODE_GP;
					error = mcp2221_parse_flash_gp_req(fd, &ioctl_put_flash.req.mcp2221_put_req, argv, 4, argc, debug);

					if (debug) {
						fprintf(stderr, "REQ FOR FLASH GP PUT: error=%d:\n", error);
						buf = &ioctl_put_flash.req.put_req_blob[0];
						for (int i = 0; i < MCP2221_REQ_BUFFER_SIZE; i++){
							fprintf(stderr, " %02x", buf[i]);
						}
						fprintf(stderr, "\n----\n");
					}
					break;
				default:
					fprintf(stderr, "Unhandled subcommand to put flash: %s %d\n\n", argv[3], validsubsub);
					usage(1);
				}
			}
			if (!error)
				error = ioctl(fd, UMCPMIO_PUT_FLASH, &ioctl_put_flash);
			break;
		default:
			fprintf(stderr, "Unhandled subcommand to put: %s %d\n\n", argv[2], validsub);
			usage(1);
		}
		break;
	case UMCPMIO_STATUS:
		if (debug)
			fprintf(stderr, "Doing status\n");
		error = ioctl(fd, UMCPMIO_GET_STATUS, &status_res);
		if (debug)
			fprintf(stderr, "UMCPMIO_GET_STATUS: error=%d, \n", error);
		break;
	case UMCPMIO_SEND:
		if (argc < 3) {
			fprintf(stderr, "Missing arguments to send command\n\n");
			usage(1);
		}
		validsub = valid_cmd(sendsubcmds, __arraycount(sendsubcmds), argv[2]);
		if (validsub == -1) {
			fprintf(stderr, "Unknown subcommand to send: %s\n\n", argv[2]);
			usage(1);
		}
		if (chip_type == UMCPMIO_CHIP_TYPE_2210) {
			switch (sendsubcmds[validsub].id) {
			case MCP2210_IOCTL_SEND_CANCEL:
				error = ioctl(fd, MCP2210_CANCEL_SPI, &cancel_res);
				break;
			default:
				fprintf(stderr, "Unhandled subcommand to send: %s %d\n\n", argv[2], validsub);
				usage(1);
			}
		} else {
			error = EINVAL;
		}

		break;
	default:
		fprintf(stderr, "Unknown handling of command: %d\n", valid);
		exit(2);
	}
	if (error) {
		fprintf(stderr, "Error: %d\n", error);
		exit(1);
	}
	switch (umcpmioctlcmds[valid].id) {
	case UMCPMIO_GET:
		if (debug) {
			switch (getsubcmds[validsub].id) {
			case UMCPMIO_IOCTL_GET_SRAM:
				if (chip_type == UMCPMIO_CHIP_TYPE_2210)
					buf = &mcp2210_get_sram.res[0];
				if (chip_type == UMCPMIO_CHIP_TYPE_2221)
					buf = (uint8_t *) & get_sram_res;
				break;
			case MCP2221_IOCTL_GET_GP_CFG:
				buf = (uint8_t *) & get_gpio_cfg_res;
				break;
			case UMCPMIO_IOCTL_GET_FLASH:
				buf = &ioctl_get_flash.res.get_blob[0];
				break;
			default:
				fprintf(stderr, "Unhandled subcommand in print for get (debug): %s %d\n\n", argv[2], validsub);
				usage(1);
			}
			for (int i = 0; i < UMCPMIO_RES_BUFFER_SIZE; i++){
				printf(" %02x", buf[i]);
			}
			printf("\n");
		}
		switch (getsubcmds[validsub].id) {
		case UMCPMIO_IOCTL_GET_SRAM:
			if (chip_type == UMCPMIO_CHIP_TYPE_2210)
				print_sram(&mcp2210_get_sram.res[0], getsubsubcmds[validsubsub].id, chip_type);
			if (chip_type == UMCPMIO_CHIP_TYPE_2221)
				print_sram((uint8_t *) & get_sram_res.cmd, -1, chip_type);
			break;
		case MCP2221_IOCTL_GET_GP_CFG:
			print_gpio_cfg(&get_gpio_cfg_res);
			break;
		case UMCPMIO_IOCTL_GET_FLASH:
			print_flash(&ioctl_get_flash.res.get_blob[0], getflashsubcmds[validsubsub].id, chip_type);
			break;
		case UMCPMIO_IOCTL_GET_TYPE:
			switch (chip_type) {
			case UMCPMIO_CHIP_TYPE_2210:
				printf("MCP-2210\n");
				break;
			case UMCPMIO_CHIP_TYPE_2221:
				printf("MCP-2221 / MCP-2221A\n");
				break;
			default:
				fprintf(stderr, "Unknown chip type: %d\n", chip_type);
			}
			break;
		default:
			fprintf(stderr, "Unhandled subcommand in print for get: %s %d\n\n", argv[2], validsub);
			usage(1);
		}

		break;
	case UMCPMIO_PUT:
		if (debug) {
			switch (putsubcmds[validsub].id) {
			case UMCPMIO_IOCTL_PUT_FLASH:
				buf = &ioctl_put_flash.res.put_res_blob[0];
				break;
			default:
				fprintf(stderr, "Unhandled subcommand in print for put (debug): %s %d\n\n", argv[2], validsub);
				usage(1);
			}
			for (int i = 0; i < MCP2221_RES_BUFFER_SIZE; i++){
				printf(" %02x", buf[i]);
			}
			printf("\n");
		}
		switch (ioctl_put_flash.res.put_res_blob[1]) {
		case MCP2210_CMD_USB_TRANSFER_IP:
			printf("USB transfer in progress\n");
			exit(3);
		case MCP2210_CMD_UNKNOWN:
		case MCP2221_CMD_COMPLETE_NO_SUPPORT:
			printf("Command not supported or unknown\n");
			exit(2);
		case MCP2210_CMD_BLOCKED_ACCESS:
		case MCP2210_CMD_ACCESS_REJECTED:
		case MCP2210_CMD_ACCESS_DENIED:
		case MCP2221_CMD_COMPLETE_EPERM:
			printf("Permission denied\n");
			exit(2);
			break;
		case UMCPMIO_CMD_COMPLETE_OK:
		default:
			break;
		}

		break;
	case UMCPMIO_STATUS:
		if (debug) {
			buf = &status_res.status_blob[0];
			for (int i = 0; i < UMCPMIO_RES_BUFFER_SIZE; i++){
				fprintf(stderr, " %02x", buf[i]);
			}
			fprintf(stderr, "\n");
		}
		print_status(&status_res.status_blob[0], chip_type);
		break;
	case UMCPMIO_SEND:
		if (debug) {
			buf = &cancel_res.cmd;
			for (int i = 0; i < MCP2210_RES_BUFFER_SIZE; i++){
				fprintf(stderr, " %02x", buf[i]);
			}
			fprintf(stderr, "\n");
		}

		break;
	default:
		fprintf(stderr, "Unknown printing of command: %d\n", valid);
		exit(2);
	}

	(void)close(fd);
	exit(0);
}
