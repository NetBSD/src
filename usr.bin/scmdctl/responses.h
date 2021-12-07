/*	$NetBSD: responses.h,v 1.1 2021/12/07 17:39:55 brad Exp $	*/

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

#include <stdint.h>

#ifndef _RESPONSES_H_
#define _RESPONSES_H_

struct scmd_identify_response {
	uint8_t		fwversion;
	uint8_t		id;
	uint8_t		slv_i2c_address;
	uint8_t		config_bits;
};

struct scmd_diag_response {
	uint8_t		diags[14];
};

struct scmd_motor_response {
	uint8_t		driver;
	int		motorlevels[34];
	bool		invert[34];
	bool		bridge[17];
};

#endif /* _RESPONSES_H_ */
