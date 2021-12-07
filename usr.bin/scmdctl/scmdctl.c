/*	$NetBSD: scmdctl.c,v 1.1 2021/12/07 17:39:55 brad Exp $	*/

/*
 * Copyright (c) 2021 Brad Spencer <brad@anduin.eldar.org>
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
__RCSID("$NetBSD: scmdctl.c,v 1.1 2021/12/07 17:39:55 brad Exp $");
#endif

/* Main userland program that knows how to talk to the Sparkfun
 * Serial Controlled Motor Driver (SCMD).  The device provides
 * 127 registers that are used to interact with the motors.
 * This program provides some convience commands to work with most
 * of the abilities of the SCMD device.
 *
 * This knows how to talk to a SCMD device via:
 *
 * 1) The uart tty interface that is provided by the SCMD device
 * 2) Userland SPI talking to something like /dev/spi0 directly
 *    In most ways this acts like talking to the tty uart.
 * 3) Using the scmd(4) i2c or spi driver.  This is, by far, the
 *    fastest way to access the driver.  The other methods have
 *    increased latency.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <dev/spi/spi_io.h>

#include <dev/ic/scmdreg.h>

#define EXTERN extern
#include "common.h"
#include "scmdctl.h"
#include "uart.h"
#include "i2cspi.h"
#include "printscmd.h"
#include "responses.h"
#include "scmdctlconst.h"

int	ul_spisetup(int, int);
int	ttysetup(int, speed_t);
int	valid_cmd(const struct scmdcmd[], long unsigned int, char *);


static void
usage(void)
{
	const char *p = getprogname();

	fprintf(stderr, "Usage: %s [-dlh] [-b baud rate] [-s SPI slave addr] device cmd args\n\n",
	    p);

	for(long unsigned int i = 0;i < __arraycount(scmdcmds);i++) {
		fprintf(stderr,"%s [-dlh] [-b baud rate] [-s SPI slave addr] device %s %s\n",
		    p,scmdcmds[i].cmd,scmdcmds[i].helpargs);
	}
}

int
valid_cmd(const struct scmdcmd c[], long unsigned int csize, char *cmdtocheck)
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

/* This is expected to fail if the device is not a classic tty */
int
ttysetup(int fd, speed_t spd)
{
        struct termios  cntrl;

        (void)tcgetattr(fd, &cntrl);
        (void)cfsetospeed(&cntrl, spd);
        (void)cfsetispeed(&cntrl, spd);
        cntrl.c_cflag &= ~(CSIZE|PARENB);
        cntrl.c_cflag |= CS8;
	cntrl.c_cflag |= CLOCAL;
        cntrl.c_iflag &= ~(ISTRIP|ICRNL);
        cntrl.c_oflag &= ~OPOST;
        cntrl.c_lflag &= ~(ICANON|ISIG|IEXTEN|ECHO);
        cntrl.c_cc[VMIN] = 1;
        cntrl.c_cc[VTIME] = 0;
	cntrl.c_iflag &= ~(IXOFF|IXON);
        return tcsetattr(fd, TCSAFLUSH, &cntrl);
}

/* This is for userland SPI and is expected to fail if the device is
 * not a /dev/spiN
 */
int
ul_spisetup(int fd, int slave_addr)
{
	struct timespec ts;
	struct spi_ioctl_configure spi_c;
	int e;

	spi_c.sic_addr = slave_addr;
#define SPI_MODE_0 0
#define SPI_MODE_1 1
#define SPI_MODE_2 2
#define SPI_MODE_3 3
	spi_c.sic_mode = SPI_MODE_0;
	spi_c.sic_speed = 1000000;

	e = ioctl(fd,SPI_IOCTL_CONFIGURE,&spi_c);
	if (e != -1) {
		ts.tv_sec = 0;
		ts.tv_nsec = 50;
		nanosleep(&ts,NULL);
	}

	return e;
}

int
main(int argc, char *argv[])
{
	int c;
	bool debug = false;
	int fd = -1, error, ttyerror = 0, ul_spierror = 0, valid, validsub = -1;
	long baud_rate = 9600;
	long slave_a = 0;
	bool dev_is_uart = true;
	int uart_s = UART_IS_PURE_UART;
	struct scmd_identify_response ir;
	struct scmd_diag_response diag;
	struct scmd_motor_response motors;
	long module;
	char motor;
	int8_t reg_value;
	uint8_t reg = 0, reg_e = 0, ur, ebus_s, lock_state;
	uint8_t register_shadow[SCMD_REG_SIZE];
	int lock_type = -1;
	bool list_names = false;
	struct function_block func_block;
	extern char *optarg;
	extern int optind;

	while ((c = getopt(argc, argv, "db:s:lh")) != -1 ) {
		switch (c) {
		case 'd':
			debug = true;
			break;
		case 'b':
			baud_rate = (long)strtoi(optarg, NULL, 0, 1, LONG_MAX, &error);
			if (error)
				warnc(error, "Conversion of `%s' to a baud rate "
				    "failed, using %ld", optarg, baud_rate);
			break;
		case 's':
			slave_a = (long)strtoi(optarg, NULL, 0, 0, LONG_MAX, &error);
			if (error)
				warnc(error, "Conversion of `%s' to a SPI slave address "
				    "failed, using %ld", optarg, slave_a);
			break;
		case 'l':
			list_names = true;
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

	if (list_names) {
		for(c = 0x00; c < SCMD_REG_SIZE;c++)
			printf("Register %d (0x%02X): %s\n",c,c,scmdregisternames[c]);
		exit(0);
	}

	if (argc <= 1) {
		usage();
		exit(0);
	}

	fd = open(argv[0], O_RDWR, 0);
	if (fd == -1) {
		err(EXIT_FAILURE, "open %s", argv[0]);
	}

	/* Figure out what the device is.  First try uart tty,
	 * then SPI userland and the if those two fail, assume
	 * scmd(4).
	 */
	ttyerror = ttysetup(fd,(speed_t)baud_rate);

	if (ttyerror) {
		ul_spierror = ul_spisetup(fd, slave_a);
		if (ul_spierror) {
			dev_is_uart = false;
		} else {
			uart_s = UART_IS_SPI_USERLAND;
		}
	}
	uart_set_subtype(uart_s, slave_a);

	if (debug) {
		fprintf(stderr, "ttysetup: error return %d\n", ttyerror);
		fprintf(stderr, "ul_spisetup: error return %d\n", ul_spierror);
	}

	/* A UART here is either a tty uart or a SPI userland device.
	 * They mostly end up working the same.
	 */
	if (dev_is_uart) {
		func_block.func_clear = &uart_clear;
		func_block.func_phy_read = &uart_read_register;
		func_block.func_phy_write = &uart_write_register;
	} else {
		func_block.func_clear = &i2cspi_clear;
		func_block.func_phy_read = &i2cspi_read_register;
		func_block.func_phy_write = &i2cspi_write_register;
	}

	valid = valid_cmd(scmdcmds,__arraycount(scmdcmds),argv[1]);

	if (valid != -1) {
		switch (scmdcmds[valid].id) {
		case SCMD_IDENTIFY:
			module = 0;
			if (argc == 3) {
				module = (long)strtoi(argv[2], NULL, 10, 0, 16, &error);
				if (error)
					warnc(error, "Conversion of '%s' module failed,"
					    " using %ld", argv[2], module);
			}
			error = common_identify(&func_block, fd, debug, module, &ir);
			break;
		case SCMD_DIAG:
			module = 0;
			if (argc == 3) {
				module = (long)strtoi(argv[2], NULL, 10, 0, 16, &error);
				if (error)
					warnc(error, "Conversion of '%s' module failed,"
					    " using %ld", argv[2], module);
			}
			error = common_diag(&func_block, fd, debug, module, &diag);
			break;
		case SCMD_MOTOR:
			if (argc >= 3) {
				validsub = valid_cmd(motorsubcmds,__arraycount(motorsubcmds),argv[2]);
				if (validsub != -1) {
					switch (motorsubcmds[validsub].id) {
					case SCMD_SUBMOTORGET:
						module = SCMD_ANY_MODULE;
						if (argc == 4) {
							module = (long)strtoi(argv[3], NULL, 10, 0, 16, &error);
							if (error)
								warnc(error, "Conversion of '%s' module failed,"
								    " using %ld", argv[3], module);
						}
						error = common_get_motor(&func_block, fd, debug, (int)module, &motors);
						break;
					case SCMD_SUBMOTORSET:
						if (argc == 6) {
							module = (long)strtoi(argv[3], NULL, 10, 0, 16, &error);
							if (error)
								warnc(error, "Conversion of '%s' module failed,"
								    " using %ld", argv[3], module);
							motor = argv[4][0];
							reg_value = (int8_t)strtoi(argv[5], NULL, 0, -127, 127, &error);
							if (error)
								err(EXIT_FAILURE,"Bad conversion for set motor for reg_value: %s", argv[5]);
						} else {
							fprintf(stderr,"Missing arguments to set motor command\n\n");
							usage();
							exit(1);
						}
						error = common_set_motor(&func_block, fd, debug, (int)module, motor, reg_value);
						break;
					case SCMD_SUBMOTORINVERT:
						if (argc == 5) {
							module = (long)strtoi(argv[3], NULL, 10, 0, 16, &error);
							if (error)
								warnc(error, "Conversion of '%s' module failed,"
								    " using %ld", argv[3], module);
							motor = argv[4][0];
						} else {
							fprintf(stderr,"Missing arguments to invert motor command\n\n");
							usage();
							exit(1);
						}
						error = common_invert_motor(&func_block, fd, debug, (int)module, motor);
						break;
					case SCMD_SUBMOTORBRIDGE:
						if (argc == 4) {
							module = (long)strtoi(argv[3], NULL, 10, 0, 16, &error);
							if (error)
								warnc(error, "Conversion of '%s' module failed,"
								    " using %ld", argv[3], module);
						} else {
							fprintf(stderr,"Missing arguments to bridge motor command\n\n");
							usage();
							exit(1);
						}
						error = common_bridge_motor(&func_block, fd, debug, (int)module);
						break;
					case SCMD_SUBMOTORDISABLE:
						error = common_enable_disable(&func_block, fd, debug, SCMD_DISABLE);
						break;
					case SCMD_SUBMOTORENABLE:
						error = common_enable_disable(&func_block, fd, debug, SCMD_ENABLE);
						break;
					default:
						fprintf(stderr,"Unhandled subcommand to motor: %s %d\n\n", argv[2], validsub);
						usage();
						exit(1);
						break;
					}
				} else {
					fprintf(stderr,"Unknown subcommand to motor: %s\n\n", argv[2]);
					usage();
					exit(1);
				}
			} else {
				fprintf(stderr,"Missing arguments to motor command\n\n");
				usage();
				exit(1);
			}
			break;
		case SCMD_READ:
			memset(register_shadow,SCMD_HOLE_VALUE + 1,SCMD_REG_SIZE);
			if (argc >= 4) {
				module = (long)strtoi(argv[2], NULL, 10, 0, 16, &error);
				if (error)
					warnc(error, "Conversion of '%s' module failed,"
					    " using %ld", argv[2], module);
				reg = (uint8_t)strtoi(argv[3], NULL, 0, 0, 0x7e, &error);
				if (error) {
					for(c = 0x00; c < SCMD_REG_SIZE;c++)
						if (strncmp(argv[3],scmdregisternames[c],15) == 0)
							break;
					if (c == SCMD_REG_SIZE) {
						fprintf(stderr,"Bad conversion for read register start: %s\n", argv[3]);
						exit(1);
					}
					reg = c;
				}
				reg_e = reg;
				if (argc == 5) {
					reg_e = (uint8_t)strtoi(argv[4], NULL, 0, 0, 0x7e, &error);
					if (error) {
						for(c = 0x00; c < SCMD_REG_SIZE;c++)
							if (strncmp(argv[4],scmdregisternames[c],15) == 0)
								break;
						if (c == SCMD_REG_SIZE) {
							fprintf(stderr,"Bad conversion for read register end: %s\n", argv[4]);
							exit(1);
						}
						reg_e = c;
					}
				}
				if (reg_e < reg) {
					fprintf(stderr,"Register end can not be less than register start: %d %d\n\n", reg, reg_e);
					usage();
					exit(1);
				}
				if (dev_is_uart) {
					error = uart_read_register(fd,debug,module,reg,reg_e,&register_shadow[reg]);
				} else {
					error = i2cspi_read_register(fd,debug,module,reg,reg_e,&register_shadow[reg]);
				}
			} else {
				fprintf(stderr,"Missing arguments to read_register command\n\n");
				usage();
				exit(1);
			}
			break;
		case SCMD_WRITE:
			if (argc == 5) {
				module = (long)strtoi(argv[2], NULL, 10, 0, 16, &error);
				if (error)
					warnc(error, "Conversion of '%s' module failed,"
					    " using %ld", argv[2], module);
				reg = (uint8_t)strtoi(argv[3], NULL, 0, 0, 0x7e, &error);
				if (error) {
					for(c = 0x00; c < SCMD_REG_SIZE;c++)
						if (strncmp(argv[3],scmdregisternames[c],15) == 0)
							break;
					if (c == SCMD_REG_SIZE) {
						fprintf(stderr,"Bad conversion for write register start: %s\n", argv[3]);
						exit(1);
					}
					reg = c;
				}
				reg_value = (int8_t)strtoi(argv[4], NULL, 0, 0, 0xff, &error);
				if (error)
					err(EXIT_FAILURE,"Bad conversion for write register for reg_value: %s", argv[4]);
				if (dev_is_uart) {
					error = uart_write_register(fd,debug,module,reg,reg_value);
				} else {
					error = i2cspi_write_register(fd,debug,module,reg,reg_value);
				}
			} else {
				fprintf(stderr,"Missing arguments to write_register command\n\n");
				usage();
				exit(1);
			}
			break;
		case SCMD_RESTART:
		case SCMD_ENUMERATE:
			error = common_control_1(&func_block, fd, debug, scmdcmds[valid].id);
			break;
		case SCMD_UPDATERATE:
			if (argc >= 3) {
				validsub = valid_cmd(updateratesubcmds,__arraycount(updateratesubcmds),argv[2]);
				if (validsub != -1) {
					switch (updateratesubcmds[validsub].id) {
					case SCMD_SUBURGET:
						error = common_get_update_rate(&func_block, fd, debug, &ur);
						break;
					case SCMD_SUBURSET:
						if (argc == 4) {
							ur = (uint8_t)strtoi(argv[3], NULL, 0, 0, 0xff, &error);
							if (error)
								err(EXIT_FAILURE,"Bad conversion for update_rate: %s", argv[3]);
							error = common_set_update_rate(&func_block, fd, debug, ur);
						} else {
							fprintf(stderr,"Missing arguments to set update_rate command\n\n");
							usage();
							exit(1);
						}
						break;
					case SCMD_SUBURFORCE:
						error = common_force_update(&func_block, fd, debug);
						break;
					default:
						fprintf(stderr,"Unhandled subcommand to updaterate: %s %d\n\n", argv[2], validsub);
						usage();
						exit(1);
						break;
					}
				} else {
					fprintf(stderr,"Unknown subcommand to updaterate: %s\n\n", argv[2]);
					usage();
					exit(1);
				}
			} else {
				fprintf(stderr,"Missing arguments to update_rate command\n\n");
				usage();
				exit(1);
			}
			break;
		case SCMD_EBUS:
			if (argc >= 3) {
				validsub = valid_cmd(ebussubcmds,__arraycount(ebussubcmds),argv[2]);
				if (validsub != -1) {
					switch (ebussubcmds[validsub].id) {
					case SCMD_SUBEBUSGET:
						error = common_get_ebus_speed(&func_block, fd, debug, &ebus_s);
						break;
					case SCMD_SUBEBUSSET:
						if (argc == 4) {
							for(ebus_s = 0; ebus_s < __arraycount(ebus_speeds);ebus_s++)
								if (strncmp(argv[3],ebus_speeds[ebus_s],8) == 0)
									break;
							if (ebus_s == __arraycount(ebus_speeds)) {
								fprintf(stderr,"Bad conversion for set expansion bus speed: %s\n", argv[3]);
								exit(1);
							}
							error = common_set_ebus_speed(&func_block, fd, debug, ebus_s);
						} else {
							fprintf(stderr,"Missing arguments to set expansion_bus command\n\n");
							usage();
							exit(1);
						}
						break;
					default:
						fprintf(stderr,"Unhandled subcommand to expansion_bus: %s %d\n\n", argv[2], validsub);
						usage();
						exit(1);
						break;
					}
				} else {
					fprintf(stderr,"Unknown subcommand to expansion_bus: %s\n\n", argv[2]);
					usage();
					exit(1);
				}
			} else {
				fprintf(stderr,"Missing arguments to expansion_bus_speed command\n\n");
				usage();
				exit(1);
			}
			break;
		case SCMD_LOCK:
			if (argc == 4) {
				validsub = valid_cmd(locksubcmds,__arraycount(locksubcmds),argv[2]);
				if (validsub != -1) {
					lock_type = valid_cmd(lockcmdtypes,__arraycount(lockcmdtypes),argv[3]);
					if (lock_type == -1) {
						fprintf(stderr,"Unknown lock type: %s\n\n", argv[3]);
						usage();
						exit(1);
					}
					lock_type = lockcmdtypes[lock_type].id;

					if (debug)
						fprintf(stderr,"Lock type in lock command: %d\n",lock_type);

					switch (locksubcmds[validsub].id) {
					case SCMD_SUBLOCKGET:
						error = common_get_lock_state(&func_block, fd, debug, lock_type, &lock_state);
						break;
					case SCMD_SUBLOCKLOCK:
						error = common_set_lock_state(&func_block, fd, debug, lock_type, SCMD_LOCK_LOCKED);
						break;
					case SCMD_SUBLOCKUNLOCK:
						error = common_set_lock_state(&func_block, fd, debug, lock_type, SCMD_LOCK_UNLOCK);
						break;
					default:
						fprintf(stderr,"Unhandled subcommand to lock: %s %d\n\n", argv[2], validsub);
						usage();
						exit(1);
						break;
					}
				} else {
					fprintf(stderr,"Unknown subcommand to lock: %s\n\n", argv[2]);
					usage();
					exit(1);
				}
			} else {
				fprintf(stderr,"Missing arguments to lock command\n\n");
				usage();
				exit(1);
			}
			break;
		case SCMD_SPIREADONE:
			error = 0;
			if (dev_is_uart &&
			    uart_s == UART_IS_SPI_USERLAND) {
				error = uart_ul_spi_read_one(fd,debug);
			}
			break;
		default:
			fprintf(stderr,"Unknown handling of command: %d\n",valid);
			exit(2);
			break;
		}

		if (! error) {
			switch (scmdcmds[valid].id) {
			case SCMD_IDENTIFY:
				print_identify(&ir);
				break;
			case SCMD_DIAG:
				print_diag(&diag);
				break;
			case SCMD_MOTOR:
				if (validsub != -1 &&
				    motorsubcmds[validsub].id == SCMD_SUBMOTORGET)
					print_motor(&motors);
				break;
			case SCMD_READ:
				for(int g = reg; g <= reg_e; g++)
					printf("Register %d (0x%02X) (%s): %d (0x%02X)\n",g,g,scmdregisternames[g],register_shadow[g],register_shadow[g]);
				break;
			case SCMD_UPDATERATE:
				if (validsub != -1 &&
				    updateratesubcmds[validsub].id == SCMD_SUBURGET)
					printf("Update rate: %dms\n",ur);
				break;
			case SCMD_EBUS:
				if (validsub != -1 &&
				    ebussubcmds[validsub].id == SCMD_SUBEBUSGET)
					printf("Expansion bus speed: %s (0x%02X)\n",(ebus_s <= 0x03) ? ebus_speeds[ebus_s] : "Unknown",ebus_s);
				break;
			case SCMD_LOCK:
				if (validsub != -1 &&
				    locksubcmds[validsub].id == SCMD_SUBLOCKGET) {
					int x = SCMD_MASTER_LOCK_UNLOCKED;

					if (lock_type == SCMD_LOCAL_USER_LOCK ||
					    lock_type == SCMD_GLOBAL_USER_LOCK)
						x = SCMD_USER_LOCK_UNLOCKED;
					printf("Lock state: %s (0x%02X)\n",(lock_state == x ? "Unlocked" : "Locked"),lock_state);
				}
				break;
			case SCMD_WRITE:
			case SCMD_RESTART:
			case SCMD_ENUMERATE:
			case SCMD_SPIREADONE:
				break;
			default:
				fprintf(stderr,"Unknown printing of command: %d\n",valid);
				exit(2);
				break;
			}
		} else {
			fprintf(stderr,"Error: %d\n", error);
			exit(1);
		}
	} else {
		fprintf(stderr,"Unknown command: %s\n\n",argv[1]);
		usage();
		exit(1);
	}

	close(fd);
	exit(0);
}
