/*	$NetBSD: printscmd.c,v 1.1 2021/12/07 17:39:55 brad Exp $	*/

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
__RCSID("$NetBSD: printscmd.c,v 1.1 2021/12/07 17:39:55 brad Exp $");
#endif

/* Functions to print stuff returned from get calls mostly */

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

#include <dev/ic/scmdreg.h>

#define EXTERN extern
#include "scmdctl.h"
#include "responses.h"
#include "common.h"

#undef EXTERN
#define EXTERN
#include "printscmd.h"

void
print_identify(struct scmd_identify_response *r)
{
	printf("ID (ID):\t\t\t%d (0x%02X)\n",r->id,r->id);
	printf("Firmware version (FID):\t\t%d\n",r->fwversion);
	printf("Config bits (CONFIG_BITS):\t%d (0x%02X)\n",r->config_bits,r->config_bits);
	if (r->slv_i2c_address >= SCMD_REMOTE_ADDR_LOW &&
	    r->slv_i2c_address <= SCMD_REMOTE_ADDR_HIGH)
		printf("Slave address (SLAVE_ADDR):\t0x%02X\n",r->slv_i2c_address);
	else
		printf("Slave address (SLAVE_ADDR):\tMaster (0x%02X)\n",r->slv_i2c_address);
}


void
print_diag(struct scmd_diag_response *r)
{
	const char *outputs[] = {
		"Read errors USER port (U_I2C_RD_ERR):\t\t",
		"Write errors USER port (U_I2C_WR_ERR):\t\t",
		"Too much data (U_BUF_DUMPED):\t\t\t",
		"Read errors SLAVE port (E_I2C_RD_ERR):\t\t",
		"Write errors SLAVE port (E_I2C_WR_ERR):\t\t",
		"Main loop (LOOP_TIME):\t\t\t\t",
		"Number of slave polls (SLV_POOL_CNT):\t\t",
		"Highest slave address (SLV_TOP_ADDR):\t\t",
		"Master count SLAVE port errors (MST_E_ERR):\t",
		"Status master board (MST_E_STATUS):\t\t",
		"Failsafe faults (FSAFE_FAULTS):\t\t\t",
		"Out of range register attempts (REG_OOR_CNT):\t",
		"Write lock attempts (REG_RO_WRITE_CNT):\t\t",
		"General test word (GEN_TEST_WORD):\t\t"
	};

	for(int n = 0; n < 14;n++) {
		printf("%s%d (0x%02X)\n",outputs[n],r->diags[n],r->diags[n]);
	}
}

const char *edu_outputs[] = {
	"Disabled",
	"Enabled",
	"Unknown"
};

void
print_motor(struct scmd_motor_response *r)
{
	int x;

	if (r->driver <= 0x01)
		x = r->driver;
	else
		x = 2;
	printf("Driver enable/disable: %s (0x%02X)\n",edu_outputs[x],r->driver);
	for(int n = 0; n < 34;n++) {
		if (r->motorlevels[n] != SCMD_NO_MOTOR) {
			printf("Module %d motor %c: %d (0x%02X) %s %s %s\n",
			    n / 2,
			    (n & 0x01 ? 'B' : 'A'),
			    decode_motor_level(r->motorlevels[n]),
			    r->motorlevels[n],
			    (r->motorlevels[n] < 128 ? "(reverse)" : "(forward)"),
			    (r->invert[n] == true ? "(inverted)" : "(not inverted)"),
			    (r->bridge[n / 2] == true ? "(bridged)" : "(not bridged)")
			);
		}
	}
}
