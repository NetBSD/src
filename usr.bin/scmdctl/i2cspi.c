/*	$NetBSD: i2cspi.c,v 1.1 2021/12/07 17:39:55 brad Exp $	*/

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
__RCSID("$NetBSD: i2cspi.c,v 1.1 2021/12/07 17:39:55 brad Exp $");
#endif

/* Functions that know how to talk to scmd(4) */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <dev/ic/scmdreg.h>

#include "scmdctl.h"
#include "responses.h"

#define EXTERN
#include "i2cspi.h"

int	i2cspi_clear(int, bool);
int     i2cspi_read_register(int, bool, int, uint8_t, uint8_t, uint8_t *);
int	i2cspi_write_register(int, bool, int, uint8_t, uint8_t);


/* The scmd(4) presents the register space as a linear chunk that can
 * be seek to read and written to without any real messing around
 */
static int
i2cspi_phy_read_register(int fd, bool debug, uint8_t reg, int rlen, uint8_t *buf)
{
	int err;

	if (rlen <= 0)
		return EINVAL;

	err = lseek(fd, reg, SEEK_SET);
	if (err != -1) {
		err = read(fd, buf, rlen);
		if (err == -1)
			err = errno;
		else
			err = 0;
	} else {
		err = errno;
	}

	if (debug)
		fprintf(stderr,"i2cspi_phy_read_register: reg: 0x%02X: rlen: %d: return err: %d\n",reg, rlen, err);

	return err;
}

static int
i2cspi_phy_write_register(int fd, bool debug, uint8_t reg, uint8_t buf)
{
	int err;

	err = lseek(fd, reg, SEEK_SET);
	if (err != -1) {
		err = write(fd, &buf, 1);
		if (err == -1)
			err = errno;
		else
			err = 0;
	} else {
		err = errno;
	}

	if (debug)
		fprintf(stderr,"i2cspi_phy_write_register: reg: 0x%02X: return err: %d\n",reg, err);

	return err;
}

int
i2cspi_clear(int fd, bool debug)
{
	return 0;
}

int
i2cspi_read_register(int fd, bool debug, int a_module, uint8_t reg, uint8_t reg_end, uint8_t *r)
{
	if (reg > SCMD_LAST_REG ||
	    reg_end > SCMD_LAST_REG)
		return EINVAL;

	if (reg_end < reg)
		return EINVAL;

	if (debug)
		fprintf(stderr,"i2cspi_read_register: reg: %d ; reg_rend: %d ; rlen: %d\n",reg, reg_end, reg_end - reg + 1);

	return i2cspi_phy_read_register(fd, debug, reg + (SCMD_REG_SIZE * a_module), reg_end - reg + 1, r);
}

int
i2cspi_write_register(int fd, bool debug, int a_module, uint8_t reg, uint8_t reg_v)
{
	int err;

	if (reg > SCMD_LAST_REG)
		return EINVAL;

	err = i2cspi_phy_write_register(fd, debug, reg + (SCMD_REG_SIZE * a_module), reg_v);

	return err;
}
