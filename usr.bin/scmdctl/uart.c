/*	$NetBSD: uart.c,v 1.2 2022/04/10 09:50:47 andvar Exp $	*/

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

#ifdef __RCSID
__RCSID("$NetBSD: uart.c,v 1.2 2022/04/10 09:50:47 andvar Exp $");
#endif

/* Functions that know how to talk to a SCMD using the uart tty
 * mode or via SPI userland, which ends up being mostly the same.
 *
 * Some of this is the same stuff that the kernel scmd(4) driver
 * ends up doing.
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
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <dev/spi/spi_io.h>

#include <dev/ic/scmdreg.h>

#include "scmdctl.h"
#include "responses.h"

#define EXTERN
#include "uart.h"


static int uart_subtype = -1;
static int uart_spi_slave_addr = -1;

/* The uart tty mode of the SCMD device is useful for human or
 * machine use.  However you can't really know what state it is in
 * so send some junk and look for '>' character indicating a new
 * command can be entered.  Usually this won't be needed, but
 * you can never know when it is.
 */
int
uart_clear(int fd, bool debug)
{
	const char jcmd[4] = "qq\r\n";
	char input;
	int i;

	if (uart_subtype == UART_IS_PURE_UART) {
		i = write(fd,jcmd,4);
		if (i == 4) {
			i = read(fd,&input,1);
			while (input != '>') {
				if (debug)
					fprintf(stderr,"uart_clear: %c\n",input);
				i = read(fd,&input,1);
			}
		} else {
			return EINVAL;
		}
	}

	return 0;
}

/* The SCMD device will echo back the characters in uart tty mode.
 * Eat them here.
 */
static int
pure_uart_send_cmd(int fd, const char *s, char *ibuf, int len)
{
	int i;

	i = write(fd,s,len);
	if (i == len) {
		i = read(fd,ibuf,len);
		return 0;
	} else {
		return EINVAL;
	}
}

/* In pure uart tty mode, the command is sent as text and we are
 * looking for '>'.  There is not a lot that can go wrong, but
 * noise on the line is one of them, and that really is not detected here.
 * This is probably the least reliable method of speaking to a SCMD
 * device.
 */
static int
uart_get_response(int fd, bool debug, char *obuf, int len)
{
	int n,i;
	char c;

	memset(obuf,0,len);
	n = 0;
	i = read(fd,&c,1);
	if (i == -1)
		return EINVAL;
	while (c != '>' && c != '\r' && n < len) {
		obuf[n] = c;
		if (debug)
			fprintf(stderr,"uart_get_response: looking for EOL or NL: %d %d -%c-\n",i,n,c);
		n++;
		i = read(fd,&c,1);
	}

	if (c != '>') {
		i = read(fd,&c,1);
		if (i == -1)
			return EINVAL;
		while (c != '>') {
			if (debug)
				fprintf(stderr,"uart_get_response: draining: %d -%c-\n",i,c);
			i = read(fd,&c,1);
			if (i == -1)
				return EINVAL;
		}
	}

	return 0;
}

/* This handles the two uart cases.  Either pure tty uart or SPI
 * userland.  The first uses text commands and the second is binary,
 * but has the strange read situation that scmd(4) has.
 */
static int
uart_phy_read_register(int fd, bool debug, uint8_t reg, uint8_t *buf)
{
	int err;
	char cmdbuf[9];
	char qbuf[10];
	struct timespec ts;
	struct spi_ioctl_transfer spi_t;
	uint8_t b;

	if (SCMD_IS_HOLE(reg)) {
		*buf = SCMD_HOLE_VALUE;
		return 0;
	}

	switch (uart_subtype) {
	case UART_IS_PURE_UART:
		sprintf(cmdbuf, "R%02X\r\n", reg);
		err = pure_uart_send_cmd(fd, cmdbuf, qbuf, 5);
		if (! err) {
			err = uart_get_response(fd, debug, qbuf, 5);
			*buf = (uint8_t)strtol(qbuf,NULL,16);
		}
		break;
	case UART_IS_SPI_USERLAND:
		spi_t.sit_addr = uart_spi_slave_addr;
		reg = reg | 0x80;
		spi_t.sit_send = &reg;
		spi_t.sit_sendlen = 1;
		spi_t.sit_recv = NULL;
		spi_t.sit_recvlen = 0;

		err = ioctl(fd,SPI_IOCTL_TRANSFER,&spi_t);
		if (debug)
			fprintf(stderr,"uart_phy_read_register: IOCTL UL SPI send err: %d ; reg: %02x ; xreg: %02x\n",
			    err,reg,reg & 0x7f);

		if (err == -1)
			return errno;

		ts.tv_sec = 0;
		ts.tv_nsec = 50;
		nanosleep(&ts,NULL);

		spi_t.sit_addr = uart_spi_slave_addr;
		spi_t.sit_send = NULL;
		spi_t.sit_sendlen = 0;
		b = SCMD_HOLE_VALUE;
		spi_t.sit_recv = &b;
		spi_t.sit_recvlen = 1;

		err = ioctl(fd,SPI_IOCTL_TRANSFER,&spi_t);
		if (debug)
			fprintf(stderr,"uart_phy_read_register: IOCTL UL SPI receive 1 err: %d ; b: %02x\n",
			    err,b);

		if (err == -1)
			return errno;

		ts.tv_sec = 0;
		ts.tv_nsec = 50;
		nanosleep(&ts,NULL);

		*buf = (uint8_t)b;

		/* Bogus read that is needed */
		spi_t.sit_addr = uart_spi_slave_addr;
		spi_t.sit_send = NULL;
		spi_t.sit_sendlen = 0;
		b = SCMD_HOLE_VALUE;
		spi_t.sit_recv = &b;
		spi_t.sit_recvlen = 1;

		err = ioctl(fd,SPI_IOCTL_TRANSFER,&spi_t);
		if (debug)
			fprintf(stderr,"uart_phy_read_register: IOCTL UL SPI receive 2 err: %d ; b: %02x\n",
			    err,b);

		if (err == -1)
			return errno;

		ts.tv_sec = 0;
		ts.tv_nsec = 50;
		nanosleep(&ts,NULL);

		break;
	default:
		return EINVAL;
		break;
	}

	return err;
}

/* Like read, this handles the two uart cases. */
static int
uart_phy_write_register(int fd, bool debug, uint8_t reg, uint8_t buf)
{
	int err;
	char cmdbuf[9];
	char qbuf[10];
	struct timespec ts;
	struct spi_ioctl_transfer spi_t;

	if (SCMD_IS_HOLE(reg)) {
		return 0;
	}

	switch (uart_subtype) {
	case UART_IS_PURE_UART:
		sprintf(cmdbuf, "W%02X%02X\r\n", reg, buf);
		err = pure_uart_send_cmd(fd, cmdbuf, qbuf, 7);
		if (! err) {
			err = uart_get_response(fd, debug, qbuf, 10);
		}
		break;
	case UART_IS_SPI_USERLAND:
		spi_t.sit_addr = uart_spi_slave_addr;
		reg = reg & 0x7f;
		spi_t.sit_send = &reg;
		spi_t.sit_sendlen = 1;
		spi_t.sit_recv = NULL;
		spi_t.sit_recvlen = 0;

		err = ioctl(fd,SPI_IOCTL_TRANSFER,&spi_t);
		if (debug)
			fprintf(stderr,"uart_phy_write_register: IOCTL UL SPI write send 1 err: %d ; reg: %02x ; xreg: %02x\n",
			    err,reg,reg & 0x7f);

		if (err == -1)
			return errno;

		spi_t.sit_addr = uart_spi_slave_addr;
		spi_t.sit_send = &buf;
		spi_t.sit_sendlen = 1;
		spi_t.sit_recv = NULL;
		spi_t.sit_recvlen = 0;

		err = ioctl(fd,SPI_IOCTL_TRANSFER,&spi_t);
		if (debug)
			fprintf(stderr,"uart_phy_write_register: IOCTL UL SPI write send 2 err: %d ; buf: %02x\n",
			    err,buf);

		if (err == -1)
			return errno;

		ts.tv_sec = 0;
		ts.tv_nsec = 50;
		nanosleep(&ts,NULL);

		break;
	default:
		return EINVAL;
		break;
	}

	return err;
}

static int
uart_local_read_register(int fd, bool debug, uint8_t reg, uint8_t reg_end, uint8_t *r)
{
	uint8_t b;
	int err = 0;

	for(int q = reg, g = 0; q <= reg_end; q++, g++) {
		err = uart_phy_read_register(fd, debug, q, &b);
		if (!err)
			r[g] = b;
	}

	return err;
}

/* When speaking to a SCMD device in any uart mode the view port for
 * chained slave modules has to be handled in userland.  This is simular
 * to what the scmd(4) kernel driver ends up doing, but is much slower.
 */
static int
uart_set_view_port(int fd, bool debug, int a_module, uint8_t vpi2creg)
{
	int err;
	uint8_t vpi2caddr = (SCMD_REMOTE_ADDR_LOW + a_module) - 1;

	if (debug)
		fprintf(stderr, "uart_set_view_port: View port addr: %02x ; View port register: %02x\n",
		    vpi2caddr, vpi2creg);

	err = uart_phy_write_register(fd, debug, SCMD_REG_REM_ADDR, vpi2caddr);
	if (! err)
		err = uart_phy_write_register(fd, debug, SCMD_REG_REM_OFFSET, vpi2creg);

	return err;
}

static int
uart_remote_read_register(int fd, bool debug, int a_module, uint8_t reg, uint8_t reg_end, uint8_t *r)
{
	int err;
	int c;
	uint8_t b;

	for(int q = reg, g = 0; q <= reg_end; q++, g++) {
		err = uart_set_view_port(fd, debug, a_module, q);
		if (err)
			break;

		b = 0xff; /* you can write anything here.. it doesn't matter */
		err = uart_phy_write_register(fd, debug, SCMD_REG_REM_READ, b);
		if (err)
			break;

		/* So ...  there is no way to really know that the data is ready and
		 * there is no way to know if there was an error in the master module reading
		 * the data from the slave module.  The data sheet says wait 5ms.. so we will
		 * wait a bit and see if the register cleared, but don't wait forever...  I
		 * can't see how it would not be possible to read junk at times.
		 */
		c = 0;
		do {
			sleep(1);
			err = uart_phy_read_register(fd, debug, SCMD_REG_REM_READ, &b);
			c++;
		} while ((c < 10) && (b != 0x00) && (!err));

		/* We can only hope that whatever was read from the slave module is there */
		if (err)
			break;
		err = uart_phy_read_register(fd, debug, SCMD_REG_REM_DATA_RD, &b);
		if (err)
			break;
		r[g] = b;
	}

	return err;
}

void
uart_set_subtype(int subt, int spi_s_addr)
{
	uart_subtype = subt;
	uart_spi_slave_addr = spi_s_addr;

	return;
}

/* Unlike scmd(4) local reads and remote module reads are done very
 * differently.
 */
int
uart_read_register(int fd, bool debug, int a_module, uint8_t reg, uint8_t reg_end, uint8_t *r)
{
	int err;

	if (reg > SCMD_LAST_REG ||
	    reg_end > SCMD_LAST_REG)
		return EINVAL;

	if (reg_end < reg)
		return EINVAL;

	err = uart_clear(fd, debug);
	if (! err) {
		if (a_module == 0) {
			err = uart_local_read_register(fd, debug, reg, reg_end, r);
		} else {
			err = uart_remote_read_register(fd, debug, a_module, reg, reg_end, r);
		}
	}

	return err;
}

static int
uart_remote_write_register(int fd, bool debug, int a_module, uint8_t reg, uint8_t reg_v)
{
	int err;
	int c;
	uint8_t b;

	err = uart_set_view_port(fd, debug, a_module, reg);
	if (! err) {
		/* We just sort of send this write off and wait to see if the register
		 * clears.  There really isn't any indication that the data made it to the
		 * slave modules.
		 */
		err = uart_phy_write_register(fd, debug, SCMD_REG_REM_DATA_WR, reg_v);
		if (! err) {
			b = 0xff; /* you can write anything here.. it doesn't matter */
			err = uart_phy_write_register(fd, debug, SCMD_REG_REM_WRITE, b);
			if (! err) {
				c = 0;
				do {
					sleep(1);
					err = uart_phy_read_register(fd, debug, SCMD_REG_REM_WRITE, &b);
					c++;
				} while ((c < 10) && (b != 0x00) && (!err));
			}
		}
	}

	return err;
}

/* Like reads, writes are done very differently between scmd(4) and
 * the uart modes.
 */
int
uart_write_register(int fd, bool debug, int a_module, uint8_t reg, uint8_t reg_v)
{
	int err;

	if (reg > SCMD_LAST_REG)
		return EINVAL;

	err = uart_clear(fd, debug);
	if (! err) {
		if (a_module == 0) {
			err = uart_phy_write_register(fd, debug, reg, reg_v);
		} else {
			err = uart_remote_write_register(fd, debug, a_module, reg, reg_v);
		}
	}

	return err;
}

/* This is a special ability to do a single SPI receive that has the
 * hope of resyncing the device should it get out of sync in SPI mode.
 * This will work for either SPI userland mode or scmd(4) when attached
 * to the SPI bus as you can still write to /dev/spiN then too.
 */
int
uart_ul_spi_read_one(int fd, bool debug)
{
	int err = 0;
	struct timespec ts;
	struct spi_ioctl_transfer spi_t;
	uint8_t b;

	if (uart_subtype == UART_IS_SPI_USERLAND) {
		spi_t.sit_addr = uart_spi_slave_addr;
		spi_t.sit_send = NULL;
		spi_t.sit_sendlen = 0;
		b = SCMD_HOLE_VALUE;
		spi_t.sit_recv = &b;
		spi_t.sit_recvlen = 1;

		err = ioctl(fd,SPI_IOCTL_TRANSFER,&spi_t);
		if (debug)
			fprintf(stderr,"uart_ul_spi_read_one: IOCTL UL SPI receive 1 err: %d ; b: %02x\n",
			    err,b);

		if (err == -1)
			return errno;

		ts.tv_sec = 0;
		ts.tv_nsec = 50;
		nanosleep(&ts,NULL);
	}

	return err;
}
