/*	$NetBSD: emcfanctl.c,v 1.1 2025/03/11 13:56:48 brad Exp $	*/

/*
 * Copyright (c) 2025 Brad Spencer <brad@anduin.eldar.org>
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
__RCSID("$NetBSD: emcfanctl.c,v 1.1 2025/03/11 13:56:48 brad Exp $");
#endif

/* Main userland program that can interfact with a EMC-210x and EMC-230x
 * fan controller using emcfan(4).
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <dev/i2c/emcfanreg.h>
#include <dev/i2c/emcfaninfo.h>

#define EXTERN extern
#include "emcfanctl.h"
#include "emcfanctlconst.h"
#include "emcfanctlutil.h"
#include "emcfanctloutputs.h"

int	valid_cmd(const struct emcfanctlcmd[], long unsigned int, char *);

static void
usage(void)
{
	const char *p = getprogname();

	fprintf(stderr, "Usage: %s [-jdh] device cmd args\n\n",
	    p);

	for(long unsigned int i = 0;i < __arraycount(emcfanctlcmds);i++) {
		switch (emcfanctlcmds[i].id) {
		case EMCFANCTL_REGISTER:
			for(long unsigned int j = 0;j < __arraycount(emcfanctlregistercmds);j++) {
				fprintf(stderr,"%s [-jdh] device %s %s\n",
				    p,
				    emcfanctlcmds[i].cmd,
				    emcfanctlregistercmds[j].helpargs);
			}
			break;
		case EMCFANCTL_FAN:
			for(long unsigned int j = 0;j < __arraycount(emcfanctlfancmds);j++) {
				switch (emcfanctlfancmds[j].id) {
				case EMCFANCTL_FAN_DRIVE:
				case EMCFANCTL_FAN_DIVIDER:
					for(long unsigned int k = 0;k < __arraycount(emcfanctlddcmds);k++) {
						fprintf(stderr,"%s [-jdh] device %s <n> %s %s\n",
						    p,
						    emcfanctlcmds[i].cmd,
						    emcfanctlfancmds[j].cmd,
						    emcfanctlddcmds[k].helpargs);
					}
					break;
				case EMCFANCTL_FAN_MINEXPECTED_RPM:
					for(long unsigned int k = 0;k < __arraycount(emcfanctlddcmds);k++) {
						if (emcfanctlddcmds[k].id != EMCFANCTL_FAN_DD_WRITE) {
							fprintf(stderr,"%s [-jdh] device %s <n> %s %s\n",
							    p,
							    emcfanctlcmds[i].cmd,
							    emcfanctlfancmds[j].cmd,
							    emcfanctlddcmds[k].helpargs);
						} else {
							fprintf(stderr,"%s [-jdh] device %s <n> %s %s ",
							    p,
							    emcfanctlcmds[i].cmd,
							    emcfanctlfancmds[j].cmd,
							    emcfanctlddcmds[k].helpargs);
							for(long unsigned int q = 0;q < __arraycount(fan_minexpectedrpm);q++) {
								fprintf(stderr,"%d|",fan_minexpectedrpm[q].human_int);
							}
							fprintf(stderr,"\n");
						}
					}
					break;
				case EMCFANCTL_FAN_EDGES:
					for(long unsigned int k = 0;k < __arraycount(emcfanctlddcmds);k++) {
						if (emcfanctlddcmds[k].id != EMCFANCTL_FAN_DD_WRITE) {
							fprintf(stderr,"%s [-jdh] device %s <n> %s %s\n",
							    p,
							    emcfanctlcmds[i].cmd,
							    emcfanctlfancmds[j].cmd,
							    emcfanctlddcmds[k].helpargs);
						} else {
							fprintf(stderr,"%s [-jdh] device %s <n> %s %s ",
							    p,
							    emcfanctlcmds[i].cmd,
							    emcfanctlfancmds[j].cmd,
							    emcfanctlddcmds[k].helpargs);
							for(long unsigned int q = 0;q < __arraycount(fan_numedges);q++) {
								fprintf(stderr,"%d|",fan_numedges[q].human_int);
							}
							fprintf(stderr,"\n");
						}
					}
					break;
				case EMCFANCTL_FAN_POLARITY:
					for(long unsigned int k = 0;k < __arraycount(emcfanctlpcmds);k++) {
						fprintf(stderr,"%s [-jdh] device %s <n> %s %s\n",
						    p,
						    emcfanctlcmds[i].cmd,
						    emcfanctlfancmds[j].cmd,
						    emcfanctlpcmds[k].helpargs);
					}
					break;
				case EMCFANCTL_FAN_PWM_BASEFREQ:
					for(long unsigned int k = 0;k < __arraycount(emcfanctlddcmds);k++) {
						if (emcfanctlddcmds[k].id != EMCFANCTL_FAN_DD_WRITE) {
							fprintf(stderr,"%s [-jdh] device %s <n> %s %s\n",
							    p,
							    emcfanctlcmds[i].cmd,
							    emcfanctlfancmds[j].cmd,
							    emcfanctlddcmds[k].helpargs);
						} else {
							fprintf(stderr,"%s [-jdh] device %s <n> %s %s ",
							    p,
							    emcfanctlcmds[i].cmd,
							    emcfanctlfancmds[j].cmd,
							    emcfanctlddcmds[k].helpargs);
							for(long unsigned int q = 0;q < __arraycount(fan_pwm_basefreq);q++) {
								if (fan_pwm_basefreq[q].instance > 0)
									break;
								fprintf(stderr,"%d|",fan_pwm_basefreq[q].human_int);
							}
							fprintf(stderr,"\n");
						}
					}
					break;
				case EMCFANCTL_FAN_PWM_OUTPUTTYPE:
					for(long unsigned int k = 0;k < __arraycount(emcfanctlotcmds);k++) {
						fprintf(stderr,"%s [-jdh] device %s <n> %s %s\n",
						    p,
						    emcfanctlcmds[i].cmd,
						    emcfanctlfancmds[j].cmd,
						    emcfanctlotcmds[k].helpargs);
					}
					break;
				default:
					fprintf(stderr,"%s [-jdh] device %s %s\n",
					    p,
					    emcfanctlcmds[i].cmd,
					    emcfanctlfancmds[j].helpargs);
					break;
				};
			}
			break;
		case EMCFANCTL_APD:
		case EMCFANCTL_SMBUSTO:
			for(long unsigned int j = 0;j < __arraycount(emcfanctlapdsmtocmds);j++) {
				fprintf(stderr,"%s [-jdh] device %s %s\n",
				    p,
				    emcfanctlcmds[i].cmd,
				    emcfanctlapdsmtocmds[j].helpargs);
			}
			break;
		default:
			fprintf(stderr,"%s [-jdh] device %s %s\n",
			    p,emcfanctlcmds[i].cmd,emcfanctlcmds[i].helpargs);
			break;
		};
	}
}

int
valid_cmd(const struct emcfanctlcmd c[], long unsigned int csize, char *cmdtocheck)
{
	int r = -1;

	for(long unsigned int i = 0;i < csize;i++) {
		if (strncmp(cmdtocheck,c[i].cmd,16) == 0) {
			r = i;
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
	uint8_t product_id;
	int product_family;
	int fd = -1, valid, error = 0, validsub = -1, validsubsub = -1;
	bool jsonify = false;
	int start_reg = 0xff, end_reg, value, tvalue, instance;
	int the_fan;

	while ((c = getopt(argc, argv, "djh")) != -1 ) {
		switch (c) {
		case 'd':
			debug = true;
			break;
		case 'j':
			jsonify = true;
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
		fprintf(stderr,"ARGC: %d\n", argc);
		fprintf(stderr,"ARGV[0]: %s ; ARGV[1]: %s ; ARGV[2]: %s ; ARGV[3]: %s; ARGV[4]: %s; ARGV[5]: %s\n",
		    argv[0],argv[1],argv[2],argv[3],argv[4],argv[5]);
	}

	if (argc <= 1) {
		usage();
		exit(0);
	}

	fd = open(argv[0], O_RDWR, 0);
	if (fd == -1) {
		err(EXIT_FAILURE, "open %s", argv[0]);
	}

	error = emcfan_read_register(fd, EMCFAN_PRODUCT_ID, &product_id, debug);

	if (error)
		err(EXIT_FAILURE, "read product_id %d", error);

	int iindex;
	iindex = emcfan_find_info(product_id);
	if (iindex == -1) {
		printf("Unknown info for product_id: %d\n",product_id);
		exit(2);
	}

	product_family = emcfan_chip_infos[iindex].family;

	/* Parse out the command line into what the requested action is */

	valid = valid_cmd(emcfanctlcmds,__arraycount(emcfanctlcmds),argv[1]);
	if (valid != -1) {
		switch (emcfanctlcmds[valid].id) {
		case EMCFANCTL_INFO:
			error = output_emcfan_info(fd, product_id, product_family, jsonify, debug);
			if (error != 0) {
				errno = error;
				err(EXIT_FAILURE, "output info");
			}
			break;
		case EMCFANCTL_APD:
			if (argc >= 3) {
				validsub = valid_cmd(emcfanctlapdsmtocmds,__arraycount(emcfanctlapdsmtocmds),argv[2]);
				if (product_id == EMCFAN_PRODUCT_2103_24 ||
				    product_id == EMCFAN_PRODUCT_2104 ||
				    product_id == EMCFAN_PRODUCT_2106) {
					switch(emcfanctlapdsmtocmds[validsub].id) {
					case EMCFANCTL_APD_READ:
						error = output_emcfan_apd(fd, product_id, product_family, EMCFAN_CHIP_CONFIG, jsonify, debug);
						if (error != 0) {
							fprintf(stderr, "Error output for apd subcommand: %d\n",error);
							exit(1);
						}
						break;
					case EMCFANCTL_APD_ON:
					case EMCFANCTL_APD_OFF:
						tvalue = find_translated_bits_by_str(apd, __arraycount(apd), argv[2]);
						if (tvalue < 0) {
							fprintf(stderr,"Error converting human value: %s\n", argv[4]);
							exit(1);
						}
						error = emcfan_rmw_register(fd, EMCFAN_CHIP_CONFIG, 0, apd, __arraycount(apd), tvalue, debug);
						if (error != 0) {
							errno = error;
							err(EXIT_FAILURE, "read/modify/write register");
						}
						break;
					default:
						fprintf(stderr,"Unhandled subcommand to apd: %s %d\n\n", argv[2], validsub);
						usage();
						exit(1);
						break;
					};
				} else {
					errno = EINVAL;
					err(EXIT_FAILURE, "This chip does not support the APD command");
				}
			} else {
				fprintf(stderr,"Missing arguments to apd command\n\n");
				usage();
				exit(1);
			}
			break;
		case EMCFANCTL_SMBUSTO:
			if (argc >= 3) {
				if (product_id ==  EMCFAN_PRODUCT_2101 ||
				    product_id == EMCFAN_PRODUCT_2101R) {
					start_reg = EMCFAN_2101_CHIP_CONFIG;
					instance = 2101;
				} else {
					if (product_family == EMCFAN_FAMILY_230X) {
						start_reg = EMCFAN_CHIP_CONFIG;
						instance = 2301;
					} else {
						start_reg = EMCFAN_CHIP_CONFIG_2;
						instance = 2103;
					}
				}

				validsub = valid_cmd(emcfanctlapdsmtocmds,__arraycount(emcfanctlapdsmtocmds),argv[2]);
				switch(emcfanctlapdsmtocmds[validsub].id) {
				case EMCFANCTL_APD_READ:
					error = output_emcfan_smbusto(fd, product_id, product_family, start_reg, instance, jsonify, debug);
					if (error != 0) {
						fprintf(stderr, "Error output for SMBUS timeout subcommand: %d\n",error);
						exit(1);
					}
					break;
				case EMCFANCTL_APD_ON:
				case EMCFANCTL_APD_OFF:
					tvalue = find_translated_bits_by_str_instance(smbus_timeout, __arraycount(smbus_timeout), argv[2], instance);
					if (tvalue < 0) {
						fprintf(stderr,"Error converting human value: %s\n", argv[4]);
						exit(1);
					}
					error = emcfan_rmw_register(fd, start_reg, 0, smbus_timeout, __arraycount(smbus_timeout), tvalue, debug);
					if (error != 0) {
						errno = error;
						err(EXIT_FAILURE, "read/modify/write register");
					}
					break;
				default:
					fprintf(stderr,"Unhandled subcommand to SMBUS timeout: %s %d\n\n", argv[2], validsub);
					usage();
					exit(1);
					break;
				};
			} else {
				fprintf(stderr,"Missing arguments to SMBUS timeout command\n\n");
				usage();
				exit(1);
			}
			break;
		case EMCFANCTL_REGISTER:
			if (argc >= 3) {
				validsub = valid_cmd(emcfanctlregistercmds,__arraycount(emcfanctlregistercmds),argv[2]);
				switch (emcfanctlregistercmds[validsub].id) {
				case EMCFANCTL_REGISTER_LIST:
					output_emcfan_register_list(product_id, product_family, jsonify, debug);
					break;
				case EMCFANCTL_REGISTER_READ:
					start_reg = end_reg = 0x00;
					if (argc >= 4) {
						start_reg = (uint8_t)strtoi(argv[3], NULL, 0, 0, 0xff, &error);
						if (error) {
							start_reg = emcfan_reg_by_name(product_id, product_family, argv[3]);
							if (start_reg == -1) {
								fprintf(stderr,"Bad conversion for read register start: %s\n", argv[3]);
								exit(1);
							}
						}
						end_reg = start_reg;
						if (argc == 5) {
							end_reg = (uint8_t)strtoi(argv[4], NULL, 0, 0, 0xff, &error);
							if (error) {
								end_reg = emcfan_reg_by_name(product_id, product_family, argv[4]);
								if (end_reg == -1) {
									fprintf(stderr,"Bad conversion for read register end: %s\n", argv[4]);
									exit(1);
								}
							}
						}
					} else {
						end_reg = 0xff;
					}
					if (end_reg < start_reg) {
						fprintf(stderr,"Register end can not be less than register start: %d %d\n\n", start_reg, end_reg);
						usage();
						exit(1);
					}

					if (debug)
						fprintf(stderr,"register read: START_REG: %d 0x%02X, END_REG: %d 0x%02X\n",start_reg, start_reg, end_reg, end_reg);

					error = output_emcfan_register_read(fd, product_id, product_family, start_reg, end_reg, jsonify, debug);
					if (error != 0) {
						fprintf(stderr, "Error read register: %d\n",error);
						exit(1);
					}

					break;
				case EMCFANCTL_REGISTER_WRITE:
					if (argc == 5) {
						start_reg = (uint8_t)strtoi(argv[3], NULL, 0, 0, 0xff, &error);
						if (error) {
							start_reg = emcfan_reg_by_name(product_id, product_family, argv[3]);
							if (start_reg == -1) {
								fprintf(stderr,"Bad conversion for write register: %s\n", argv[3]);
								exit(1);
							}
						}

						value = (uint8_t)strtoi(argv[4], NULL, 0, 0, 0xff, &error);
						if (error) {
							fprintf(stderr,"Could not convert value for register write.\n");
							usage();
							exit(1);
						}
					} else {
						fprintf(stderr,"Not enough arguments for register write.\n");
						usage();
						exit(1);
					}

					if (debug)
						fprintf(stderr,"register write: START_REG: %d 0x%02X, VALUE: %d 0x%02X\n",start_reg, start_reg, value, value);

					error = emcfan_write_register(fd, start_reg, value, debug);
					if (error != 0) {
						errno = error;
						err(EXIT_FAILURE, "write register");
					}

					break;
				default:
					fprintf(stderr,"Unhandled subcommand to register: %s %d\n\n", argv[2], validsub);
					usage();
					exit(1);
					break;
				};
			} else {
				fprintf(stderr,"Missing arguments to register command\n\n");
				usage();
				exit(1);
			}
			break;
		case EMCFANCTL_FAN:
			if (argc >= 4) {
				the_fan = strtoi(argv[2], NULL, 0, 1, emcfan_chip_infos[iindex].num_fans, &error);
				if (error) {
					fprintf(stderr,"Bad conversion for fan number: %s\n", argv[2]);
					exit(1);
				}
				the_fan--;
				validsub = valid_cmd(emcfanctlfancmds,__arraycount(emcfanctlfancmds),argv[3]);
				switch (emcfanctlfancmds[validsub].id) {
				case EMCFANCTL_FAN_STATUS:
					if (product_id != EMCFAN_PRODUCT_2101 &&
					    product_id != EMCFAN_PRODUCT_2101R) {
						if (product_family == EMCFAN_FAMILY_230X) {
							start_reg = EMCFAN_230X_FAN_STATUS;
							end_reg = EMCFAN_230X_FAN_DRIVE_STATUS;
						} else {
							start_reg = EMCFAN_210_346_FAN_STATUS;
							end_reg = 0xff;
						}
					} else {
						errno = EINVAL;
						err(EXIT_FAILURE, "EMC2101 and EMC2101R do not support this subcommand");
					}
					break;
				case EMCFANCTL_FAN_DRIVE:
					start_reg = emcfan_chip_infos[iindex].fan_drive_registers[the_fan];
					break;
				case EMCFANCTL_FAN_DIVIDER:
					start_reg = emcfan_chip_infos[iindex].fan_divider_registers[the_fan];
					break;
				case EMCFANCTL_FAN_MINEXPECTED_RPM:
				case EMCFANCTL_FAN_EDGES:
					if (product_id != EMCFAN_PRODUCT_2101 &&
					    product_id != EMCFAN_PRODUCT_2101R) {
						if (debug)
							fprintf(stderr,"fan subcommand minexpected_rpm / edges: the_fan=%d\n",the_fan);
						if (product_family == EMCFAN_FAMILY_210X) {
							the_fan = strtoi(argv[2], NULL, 0, 1, emcfan_chip_infos[iindex].num_tachs, &error);
							if (error) {
								fprintf(stderr,"Bad conversion for fan number: %s\n", argv[2]);
								exit(1);
							}
							the_fan--;
							switch(the_fan) {
							case 0:
								start_reg = EMCFAN_210_346_CONFIG_1;
								break;
							case 1:
								start_reg = EMCFAN_210_346_CONFIG_2;
								break;
							default:
								start_reg = 0xff;
								break;
							};
						} else {
							switch(the_fan) {
							case 0:
								start_reg = EMCFAN_230X_CONFIG_1;
								break;
							case 1:
								start_reg = EMCFAN_230X_CONFIG_2;
								break;
							case 2:
								start_reg = EMCFAN_230X_CONFIG_3;
								break;
							case 3:
								start_reg = EMCFAN_230X_CONFIG_4;
								break;
							case 4:
								start_reg = EMCFAN_230X_CONFIG_5;
								break;
							default:
								start_reg = 0xff;
								break;
							};
						}
					} else {
						errno = EINVAL;
						err(EXIT_FAILURE, "EMC2101 and EMC2101R do not support this subcommand");
					}
					break;
				case EMCFANCTL_FAN_POLARITY:
					if (product_id != EMCFAN_PRODUCT_2101 &&
					    product_id != EMCFAN_PRODUCT_2101R) {
						start_reg = EMCFAN_POLARITY_CONFIG;
					} else {
						start_reg = EMCFAN_2101_FAN_CONFIG;
					}
					break;
				case EMCFANCTL_FAN_PWM_BASEFREQ:
					if (product_id != EMCFAN_PRODUCT_2101 &&
					    product_id != EMCFAN_PRODUCT_2101R) {
						if (product_family == EMCFAN_FAMILY_210X)
							start_reg = EMCFAN_210_346_PWM_BASEFREQ;
						else
							if (the_fan <= 2)
								start_reg = EMCFAN_230X_BASE_FREQ_123;
							else
								start_reg = EMCFAN_230X_BASE_FREQ_45;
					} else {
						errno = EINVAL;
						err(EXIT_FAILURE, "EMC2101 and EMC2101R do not support this subcommand");
					}
					break;
				case EMCFANCTL_FAN_PWM_OUTPUTTYPE:
					if (product_family == EMCFAN_FAMILY_230X) {
						start_reg = EMCFAN_230X_OUTPUT_CONFIG;
					} else {
						errno = EINVAL;
						err(EXIT_FAILURE, "This subcommand is only supported on the EMC230X family");
					}
					break;
				default:
					fprintf(stderr,"Unhandled subcommand to fan: %s %d\n\n", argv[3], validsub);
					usage();
					exit(1);
					break;
				};
				if (emcfanctlfancmds[validsub].id == EMCFANCTL_FAN_STATUS) {
					error = output_emcfan_fan_status(fd, product_id, product_family, start_reg, end_reg, the_fan, jsonify, debug);
					if (error != 0) {
						fprintf(stderr, "Error fan status for fan subcommand: %d\n",error);
						exit(1);
					}
				}
				if (emcfanctlfancmds[validsub].id == EMCFANCTL_FAN_DRIVE ||
				    emcfanctlfancmds[validsub].id == EMCFANCTL_FAN_DIVIDER) {
					validsubsub = valid_cmd(emcfanctlddcmds,__arraycount(emcfanctlddcmds),argv[4]);
					switch(emcfanctlddcmds[validsubsub].id) {
					case EMCFANCTL_FAN_DD_READ:
						if (emcfanctlfancmds[validsub].id == EMCFANCTL_FAN_DRIVE)
							error = output_emcfan_drive(fd, product_id, product_family, start_reg, jsonify, debug);
						else
							error = output_emcfan_divider(fd, product_id, product_family, start_reg, jsonify, debug);
						if (error != 0) {
							fprintf(stderr, "Error read drive / divider for fan subcommand: %d\n",error);
							exit(1);
						}
						break;
					case EMCFANCTL_FAN_DD_WRITE:
						value = (uint8_t)strtoi(argv[5], NULL, 0, 0, 0xff, &error);
						if (error) {
							fprintf(stderr,"Could not convert value for fan subcommand write.\n");
							usage();
							exit(1);
						}
						error = emcfan_write_register(fd, start_reg, value, debug);
						if (error != 0) {
							errno = error;
							err(EXIT_FAILURE, "write register");
						}
						break;
					default:
						fprintf(stderr,"Unhandled subsubcommand to fan: %s %d\n\n", argv[4], validsubsub);
						usage();
						exit(1);
						break;
					};
				}
				if (emcfanctlfancmds[validsub].id == EMCFANCTL_FAN_MINEXPECTED_RPM) {
					if (start_reg == 0xff) {
						fprintf(stderr,"fan minexpected rpm subcommand, unknown register\n");
						exit(1);
					}
					validsubsub = valid_cmd(emcfanctlddcmds,__arraycount(emcfanctlddcmds),argv[4]);
					switch(emcfanctlddcmds[validsubsub].id) {
					case EMCFANCTL_FAN_DD_READ:
						error = output_emcfan_minexpected_rpm(fd, product_id, product_family, start_reg, jsonify, debug);
						if (error != 0) {
							fprintf(stderr, "Error read minexpected rpm subcommand: %d\n",error);
							exit(1);
						}
						break;
					case EMCFANCTL_FAN_DD_WRITE:
						value = (int)strtoi(argv[5], NULL, 0, 0, 0xffff, &error);
						if (error) {
							fprintf(stderr,"Could not convert value for minexpected rpm subcommand write.\n");
							usage();
							exit(1);
						}
						tvalue = find_translated_bits_by_hint(fan_minexpectedrpm, __arraycount(fan_minexpectedrpm), value);
						if (tvalue < 0) {
							fprintf(stderr,"Error converting human value: %d %d\n",value, tvalue);
							exit(1);
						}
						error = emcfan_rmw_register(fd, start_reg, value, fan_minexpectedrpm, __arraycount(fan_minexpectedrpm), tvalue, debug);
						if (error != 0) {
							errno = error;
							err(EXIT_FAILURE, "read/modify/write register");
						}
						break;
					default:
						fprintf(stderr,"Unhandled subsubcommand to minexpected rpm subcommand: %s %d\n\n", argv[4], validsubsub);
						usage();
						exit(1);
						break;
					};
				}
				if (emcfanctlfancmds[validsub].id == EMCFANCTL_FAN_EDGES) {
					if (start_reg == 0xff) {
						fprintf(stderr,"fan edges subcommand, unknown register\n");
						exit(1);
					}
					validsubsub = valid_cmd(emcfanctlddcmds,__arraycount(emcfanctlddcmds),argv[4]);
					switch(emcfanctlddcmds[validsubsub].id) {
					case EMCFANCTL_FAN_DD_READ:
						error = output_emcfan_edges(fd, product_id, product_family, start_reg, jsonify, debug);
						if (error != 0) {
							fprintf(stderr, "Error read edges subcommand: %d\n",error);
							exit(1);
						}
						break;
					case EMCFANCTL_FAN_DD_WRITE:
						value = (uint8_t)strtoi(argv[5], NULL, 0, 0, 9, &error);
						if (error) {
							fprintf(stderr,"Could not convert value for edges subcommand write.\n");
							usage();
							exit(1);
						}
						tvalue = find_translated_bits_by_hint(fan_numedges, __arraycount(fan_numedges), value);
						if (tvalue < 0) {
							fprintf(stderr,"Error converting human value: %d\n",value);
							exit(1);
						}
						error = emcfan_rmw_register(fd, start_reg, value, fan_numedges, __arraycount(fan_numedges), tvalue, debug);
						if (error != 0) {
							errno = error;
							err(EXIT_FAILURE, "read/modify/write register");
						}
						break;
					default:
						fprintf(stderr,"Unhandled subsubcommand to minexpected rpm subcommand: %s %d\n\n", argv[4], validsubsub);
						usage();
						exit(1);
						break;
					};
				}
				if (emcfanctlfancmds[validsub].id == EMCFANCTL_FAN_POLARITY) {
					if (start_reg == 0xff) {
						fprintf(stderr,"fan polarity subcommand, unknown register\n");
						exit(1);
					}
					validsubsub = valid_cmd(emcfanctlpcmds,__arraycount(emcfanctlpcmds),argv[4]);
					switch(emcfanctlpcmds[validsubsub].id) {
					case EMCFANCTL_FAN_P_READ:
						error = output_emcfan_polarity(fd, product_id, product_family, start_reg, the_fan, jsonify, debug);
						if (error != 0) {
							fprintf(stderr, "Error output for polarity subcommand: %d\n",error);
							exit(1);
						}
						break;
					case EMCFANCTL_FAN_P_INVERTED:
					case EMCFANCTL_FAN_P_NONINVERTED:
						if (product_id == EMCFAN_PRODUCT_2101 ||
						    product_id == EMCFAN_PRODUCT_2101R) {
							tvalue = find_translated_bits_by_str_instance(fan_polarity, __arraycount(fan_polarity), argv[4], 2101);
						} else {
							tvalue = find_translated_bits_by_str_instance(fan_polarity, __arraycount(fan_polarity), argv[4], the_fan);
						}
						if (tvalue < 0) {
							fprintf(stderr,"Error converting human value: %s\n", argv[4]);
							exit(1);
						}
						error = emcfan_rmw_register(fd, start_reg, 0, fan_polarity, __arraycount(fan_polarity), tvalue, debug);
						if (error != 0) {
							errno = error;
							err(EXIT_FAILURE, "read/modify/write register");
						}
						break;
					default:
						fprintf(stderr,"Unhandled subsubcommand to polarity subcommand: %s %d\n\n", argv[4], validsubsub);
						usage();
						exit(1);
						break;
					};

				}
				if (emcfanctlfancmds[validsub].id == EMCFANCTL_FAN_PWM_BASEFREQ) {
					if (start_reg == 0xff) {
						fprintf(stderr,"fan base_freq subcommand, unknown register\n");
						exit(1);
					}
					validsubsub = valid_cmd(emcfanctlddcmds,__arraycount(emcfanctlddcmds),argv[4]);
					switch(emcfanctlddcmds[validsubsub].id) {
					case EMCFANCTL_FAN_DD_READ:
						if (debug)
							fprintf(stderr,"%s: the_fan=%d\n",__func__,the_fan);
						if (product_id != EMCFAN_PRODUCT_2305)
							error = output_emcfan_pwm_basefreq(fd, product_id, product_family, start_reg, the_fan, jsonify, debug);
						else
							if (the_fan <= 2)
								error = output_emcfan_pwm_basefreq(fd, product_id, product_family, start_reg, the_fan, jsonify, debug);
							else
								error = output_emcfan_pwm_basefreq(fd, product_id, product_family, start_reg,
								    the_fan + 23050, jsonify, debug);
						if (error != 0) {
							fprintf(stderr, "Error read base_freq subcommand: %d\n",error);
							exit(1);
						}
						break;
					case EMCFANCTL_FAN_DD_WRITE:
						value = (int)strtoi(argv[5], NULL, 0, 0, 0xffff, &error);
						if (error) {
							fprintf(stderr,"Could not convert value for base_freq subcommand write.\n");
							usage();
							exit(1);
						}
						if (product_id != EMCFAN_PRODUCT_2305)
							tvalue = find_translated_bits_by_hint_instance(fan_pwm_basefreq, __arraycount(fan_pwm_basefreq), value, the_fan);
						else
							if (the_fan <= 2)
								tvalue = find_translated_bits_by_hint_instance(fan_pwm_basefreq, __arraycount(fan_pwm_basefreq), value, the_fan);
							else
								tvalue = find_translated_bits_by_hint_instance(fan_pwm_basefreq, __arraycount(fan_pwm_basefreq),
								    value, the_fan + 23050);
						if (tvalue < 0) {
							fprintf(stderr,"Error converting human value: %d %d\n",value, tvalue);
							exit(1);
						}
						error = emcfan_rmw_register(fd, start_reg, value, fan_pwm_basefreq, __arraycount(fan_pwm_basefreq), tvalue, debug);
						if (error != 0) {
							errno = error;
							err(EXIT_FAILURE, "read/modify/write register");
						}
						break;
					default:
						fprintf(stderr,"Unhandled subsubcommand to base_freq subcommand: %s %d\n\n", argv[4], validsubsub);
						usage();
						exit(1);
						break;
					};
					break;
				}
				if (emcfanctlfancmds[validsub].id == EMCFANCTL_FAN_PWM_OUTPUTTYPE) {
					if (start_reg == 0xff) {
						fprintf(stderr,"fan pwm_output_type subcommand, unknown register\n");
						exit(1);
					}
					validsubsub = valid_cmd(emcfanctlotcmds,__arraycount(emcfanctlotcmds),argv[4]);
					switch(emcfanctlotcmds[validsubsub].id) {
					case EMCFANCTL_FAN_OT_READ:
						error = output_emcfan_pwm_output_type(fd, product_id, product_family, start_reg, the_fan, jsonify, debug);
						if (error != 0) {
							fprintf(stderr, "Error output for pwm_output_type subcommand: %d\n",error);
							exit(1);
						}
						break;
					case EMCFANCTL_FAN_OT_PUSHPULL:
					case EMCFANCTL_FAN_OT_OPENDRAIN:
						tvalue = find_translated_bits_by_str_instance(fan_pwm_output_type, __arraycount(fan_pwm_output_type), argv[4], the_fan);
						if (tvalue < 0) {
							fprintf(stderr,"Error converting human value: %s\n", argv[4]);
							exit(1);
						}
						error = emcfan_rmw_register(fd, start_reg, 0, fan_pwm_output_type, __arraycount(fan_pwm_output_type), tvalue, debug);
						if (error != 0) {
							errno = error;
							err(EXIT_FAILURE, "read/modify/write register");
						}
						break;
					default:
						fprintf(stderr,"Unhandled subsubcommand to pwm_output_type subcommand: %s %d\n\n", argv[4], validsubsub);
						usage();
						exit(1);
						break;
					};

				}
			} else {
				fprintf(stderr,"Missing arguments to fan command\n\n");
				usage();
				exit(1);
			}
			break;
		default:
			fprintf(stderr,"Unknown handling of command: %d\n",valid);
			exit(2);
			break;
		}
	} else {
		fprintf(stderr,"Unknown command: %s\n\n",argv[1]);
		usage();
		exit(1);
	}
	close(fd);
	exit(0);
}
