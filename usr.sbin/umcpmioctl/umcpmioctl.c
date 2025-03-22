/*	$NetBSD: umcpmioctl.c,v 1.3 2025/03/22 05:46:32 rillig Exp $	*/

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
__RCSID("$NetBSD: umcpmioctl.c,v 1.3 2025/03/22 05:46:32 rillig Exp $");
#endif

/* Main userland program that can pull the SRAM and FLASH content from a MCP2221
 * / MCP2221A chip using umcpmio(4).
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

#include <dev/usb/umcpmio_hid_reports.h>
#include <dev/usb/umcpmio_io.h>

#define EXTERN extern
#include "umcpmioctl.h"
#include "umcpmioctlconst.h"
#include "printumcpmio.h"
#include "putflash.h"

static void
usage(void)
{
	const char *p = getprogname();

	fprintf(stderr, "Usage: %s [-dh] device cmd args\n\n",
	    p);

	for (long unsigned int i = 0; i < __arraycount(umcpmioctlcmds); i++) {
		switch (umcpmioctlcmds[i].id) {
		case UMCPMIO_GET:
			for (long unsigned int j = 0; j < __arraycount(getsubcmds); j++) {
				fprintf(stderr, "%s [-dh] device %s %s %s\n",
				    p, umcpmioctlcmds[i].cmd, getsubcmds[j].cmd, getsubcmds[j].helpargs);
			}
			break;
		case UMCPMIO_PUT:
			for (long unsigned int j = 0; j < __arraycount(putsubcmds); j++) {
				fprintf(stderr, "%s [-dh] device %s %s %s\n",
				    p, umcpmioctlcmds[i].cmd, putsubcmds[j].cmd, putsubcmds[j].helpargs);
			}
			break;
		default:
			fprintf(stderr, "%s [-dh] device %s %s\n",
			    p, umcpmioctlcmds[i].cmd, umcpmioctlcmds[i].helpargs);
			break;
		};
	}

	fprintf(stderr, "\n");
	fprintf(stderr, "sram - The SRAM on the chip\n");
	fprintf(stderr, "gp - The GPIO pin state and function\n");
	fprintf(stderr, "cs - Chip Settings\n");
	fprintf(stderr, "usbman - USB Manufacturer Descriptor\n");
	fprintf(stderr, "usbprod - USB Product Descriptor\n");
	fprintf(stderr, "usbsn - USB Serial Number\n");
	fprintf(stderr, "chipsn - Chip Serial Number\n");
}

static int
valid_cmd(const struct umcpmioctlcmd c[], long unsigned int csize, char *cmdtocheck)
{
	int r = -1;

	for (long unsigned int i = 0; i < csize; i++) {
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

	while ((c = getopt(argc, argv, "dh")) != -1) {
		switch (c) {
		case 'd':
			debug = true;
			break;
		case 'h':
		default:
			usage();
			exit(0);
		}
	}

	argc -= optind;
	argv += optind;

	if (debug) {
		fprintf(stderr, "ARGC: %d\n", argc);
		fprintf(stderr, "ARGV[0]: %s ; ARGV[1]: %s ; ARGV[2]: %s ; ARGV[3]: %s; ARGV[4]: %s; ARGV[5]: %s\n",
		    argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
	}

	if (argc <= 1) {
		usage();
		exit(0);
	}

	fd = open(argv[0], O_RDWR, 0);
	if (fd == -1) {
		err(EXIT_FAILURE, "open %s", argv[0]);
	}

	/* Parse out the command line into what the requested action is */

	valid = valid_cmd(umcpmioctlcmds, __arraycount(umcpmioctlcmds), argv[1]);

	if (valid != -1) {
		uint8_t *buf;
		struct mcp2221_status_res status_res;
		struct mcp2221_get_sram_res get_sram_res;
		struct mcp2221_get_gpio_cfg_res get_gpio_cfg_res;
		struct umcpmio_ioctl_get_flash ioctl_get_flash;
		struct umcpmio_ioctl_put_flash ioctl_put_flash;

		switch (umcpmioctlcmds[valid].id) {
		case UMCPMIO_GET:
			if (argc >= 3) {
				validsub = valid_cmd(getsubcmds, __arraycount(getsubcmds), argv[2]);
				if (validsub != -1) {
					switch (getsubcmds[validsub].id) {
					case UMCPMIO_IOCTL_GET_SRAM:
						error = ioctl(fd, UMCPMIO_GET_SRAM, &get_sram_res);
						break;
					case UMCPMIO_IOCTL_GET_GP_CFG:
						error = ioctl(fd, UMCPMIO_GET_GP_CFG, &get_gpio_cfg_res);
						break;
					case UMCPMIO_IOCTL_GET_FLASH:
						if (argc == 4) {
							validsubsub = valid_cmd(getflashsubcmds, __arraycount(getflashsubcmds), argv[3]);
							if (validsubsub != -1) {
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
									usage();
									exit(1);
								}
								error = ioctl(fd, UMCPMIO_GET_FLASH, &ioctl_get_flash);
							} else {
								fprintf(stderr, "Unknown subcommand to get flash: %s %d\n\n", argv[3], validsubsub);
								usage();
								exit(1);
							}
						} else {
							fprintf(stderr, "Missing arguments to get flash command\n\n");
							usage();
							exit(1);
						}
						break;
					default:
						fprintf(stderr, "Unhandled subcommand to get: %s %d\n\n", argv[2], validsub);
						usage();
						exit(1);
					}
				} else {
					fprintf(stderr, "Unknown subcommand to get: %s\n\n", argv[2]);
					usage();
					exit(1);
				}
			} else {
				fprintf(stderr, "Missing arguments to get command\n\n");
				usage();
				exit(1);
			}
			break;
		case UMCPMIO_PUT:
			if (argc > 3) {
				validsub = valid_cmd(putsubcmds, __arraycount(putsubcmds), argv[2]);
				if (validsub != -1) {
					switch (putsubcmds[validsub].id) {
					case UMCPMIO_IOCTL_PUT_FLASH:
						if (argc >= 4) {
							validsubsub = valid_cmd(putflashsubcmds, __arraycount(putflashsubcmds), argv[3]);
							if (validsubsub != -1) {
								switch (putflashsubcmds[validsubsub].id) {
								case UMCPMIO_IOCTL_PUT_FLASH_GP:
									memset(&ioctl_put_flash, 0, sizeof(ioctl_put_flash));
									ioctl_put_flash.subcode = MCP2221_FLASH_SUBCODE_GP;
									error = parse_flash_gp_req(fd, &ioctl_put_flash.put_flash_req, argv, 4, argc, debug);

									if (debug) {
										fprintf(stderr, "REQ FOR FLASH GP PUT: error=%d:\n", error);
										buf = (uint8_t *)&ioctl_put_flash.put_flash_req.cmd;
										for (int i = 0; i < MCP2221_REQ_BUFFER_SIZE; i++) {
											fprintf(stderr, " %02x", buf[i]);
										}
										fprintf(stderr, "\n----\n");
									}

									if (!error)
										error = ioctl(fd, UMCPMIO_PUT_FLASH, &ioctl_put_flash);

									break;
								default:
									fprintf(stderr, "Unhandled subcommand to get flash: %s %d\n\n", argv[3], validsubsub);
									usage();
									exit(1);
								};
							} else {
								fprintf(stderr, "Unknown subcommand to put flash: %s %d\n\n", argv[3], validsubsub);
								usage();
								exit(1);
							}
						} else {
							fprintf(stderr, "Missing arguments to put flash command\n\n");
							usage();
							exit(1);
						}
						break;
					default:
						fprintf(stderr, "Unhandled subcommand to put: %s %d\n\n", argv[2], validsub);
						usage();
						exit(1);
					};
				} else {
					fprintf(stderr, "Unknown subcommand to put: %s\n\n", argv[2]);
					usage();
					exit(1);
				}
			} else {
				fprintf(stderr, "Missing arguments to put command\n\n");
				usage();
				exit(1);
			}
			break;
		case UMCPMIO_STATUS:
			if (debug)
				fprintf(stderr, "Doing status\n");
			error = ioctl(fd, UMCPMIO_GET_STATUS, &status_res);
			if (debug)
				fprintf(stderr, "UMCPMIO_GET_STATUS: error=%d, \n", error);
			break;
		default:
			fprintf(stderr, "Unknown handling of command: %d\n", valid);
			exit(2);
		}
		if (!error) {
			switch (umcpmioctlcmds[valid].id) {
			case UMCPMIO_GET:
				if (debug) {
					switch (getsubcmds[validsub].id) {
					case UMCPMIO_IOCTL_GET_SRAM:
						buf = (uint8_t *)&get_sram_res;
						break;
					case UMCPMIO_IOCTL_GET_GP_CFG:
						buf = (uint8_t *)&get_gpio_cfg_res;
						break;
					case UMCPMIO_IOCTL_GET_FLASH:
						buf = (uint8_t *)&ioctl_get_flash.get_flash_res.cmd;
						break;
					default:
						fprintf(stderr, "Unhandled subcommand in print for get (debug): %s %d\n\n", argv[2], validsub);
						usage();
						exit(1);
					}
					for (int i = 0; i < MCP2221_RES_BUFFER_SIZE; i++) {
						printf(" %02x", buf[i]);
					}
					printf("\n");
				}

				switch (getsubcmds[validsub].id) {
				case UMCPMIO_IOCTL_GET_SRAM:
					print_sram(&get_sram_res);
					break;
				case UMCPMIO_IOCTL_GET_GP_CFG:
					print_gpio_cfg(&get_gpio_cfg_res);
					break;
				case UMCPMIO_IOCTL_GET_FLASH:
					print_flash(&ioctl_get_flash.get_flash_res, getflashsubcmds[validsubsub].id);
					break;
				default:
					fprintf(stderr, "Unhandled subcommand in print for get: %s %d\n\n", argv[2], validsub);
					usage();
					exit(1);
				}

				break;
			case UMCPMIO_PUT:
				if (debug) {
					switch (putsubcmds[validsub].id) {
					case UMCPMIO_IOCTL_PUT_FLASH:
						buf = (uint8_t *)&ioctl_put_flash.put_flash_res.cmd;
						break;
					default:
						fprintf(stderr, "Unhandled subcommand in print for put (debug): %s %d\n\n", argv[2], validsub);
						usage();
						exit(1);
					}
					for (int i = 0; i < MCP2221_RES_BUFFER_SIZE; i++) {
						printf(" %02x", buf[i]);
					}
					printf("\n");
				}

				if (putsubcmds[validsub].id == UMCPMIO_IOCTL_PUT_FLASH &&
				    putflashsubcmds[validsubsub].id == UMCPMIO_IOCTL_PUT_FLASH_GP) {
					switch (ioctl_put_flash.put_flash_res.completion) {
					case MCP2221_CMD_COMPLETE_NO_SUPPORT:
						printf("Command not supported\n");
						exit(2);
					case MCP2221_CMD_COMPLETE_EPERM:
						printf("Permission denied\n");
						exit(2);
					case MCP2221_CMD_COMPLETE_OK:
					default:
						break;
					}
				} else {
					fprintf(stderr, "Unhandled subcommand in print for put: %s %d %s %d\n\n", argv[2], validsub, argv[3], validsubsub);
					usage();
					exit(1);
				}
				break;
			case UMCPMIO_STATUS:
				if (debug) {
					buf = &status_res.cmd;
					for (int i = 0; i < MCP2221_RES_BUFFER_SIZE; i++) {
						fprintf(stderr, " %02x", buf[i]);
					}
					fprintf(stderr, "\n");
				}
				print_status(&status_res);
				break;
			default:
				fprintf(stderr, "Unknown printing of command: %d\n", valid);
				exit(2);
			}
		} else {
			fprintf(stderr, "Error: %d\n", error);
			exit(1);
		}
	} else {
		fprintf(stderr, "Unknown command: %s\n\n", argv[1]);
		usage();
		exit(1);
	}

	close(fd);
	exit(0);
}
