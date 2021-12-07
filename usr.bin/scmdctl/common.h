/*	$NetBSD: common.h,v 1.1 2021/12/07 17:39:55 brad Exp $	*/

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

#ifndef _COMMON_H_
#define _COMMON_H_

#include "responses.h"

struct function_block {
	int	(*func_clear)(int, bool);
	int	(*func_phy_read)(int, bool, int, uint8_t, uint8_t, uint8_t *);
	int	(*func_phy_write)(int, bool, int, uint8_t, uint8_t);
};

EXTERN int decode_motor_level(int);
EXTERN int common_clear(struct function_block *, int, bool);
EXTERN int common_identify(struct function_block *, int, bool, int, struct scmd_identify_response *);
EXTERN int common_diag(struct function_block *, int, bool, int, struct scmd_diag_response *);
EXTERN int common_get_motor(struct function_block *, int, bool, int, struct scmd_motor_response *);
EXTERN int common_set_motor(struct function_block *, int, bool, int, char, int8_t);
EXTERN int common_invert_motor(struct function_block *, int, bool, int, char);
EXTERN int common_bridge_motor(struct function_block *, int, bool, int);
EXTERN int common_enable_disable(struct function_block *, int, bool, int);
EXTERN int common_control_1(struct function_block *, int, bool, int);
EXTERN int common_get_update_rate(struct function_block *, int, bool, uint8_t *);
EXTERN int common_set_update_rate(struct function_block *, int, bool, uint8_t);
EXTERN int common_force_update(struct function_block *, int, bool);
EXTERN int common_get_ebus_speed(struct function_block *, int, bool, uint8_t *);
EXTERN int common_set_ebus_speed(struct function_block *, int, bool, uint8_t);
EXTERN int common_get_lock_state(struct function_block *, int, bool, int, uint8_t *);
EXTERN int common_set_lock_state(struct function_block *, int, bool, int, uint8_t);

#endif
