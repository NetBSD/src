/*	$NetBSD: emcfanctl.h,v 1.1 2025/03/11 13:56:48 brad Exp $	*/

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

#ifndef _EMCFANCTL_H_
#define _EMCFANCTL_H_

/* Top level commands */
#define EMCFANCTL_INFO 1
#define EMCFANCTL_REGISTER 2
#define EMCFANCTL_FAN 3
#define EMCFANCTL_APD 4
#define EMCFANCTL_SMBUSTO 5

/* REGISTER sub commands */
#define EMCFANCTL_REGISTER_LIST 1
#define EMCFANCTL_REGISTER_READ 2
#define EMCFANCTL_REGISTER_WRITE 3

/* FAN sub commands */
#define EMCFANCTL_FAN_STATUS 1
#define EMCFANCTL_FAN_DRIVE 2
#define EMCFANCTL_FAN_DIVIDER 3
#define EMCFANCTL_FAN_MINEXPECTED_RPM 4
#define EMCFANCTL_FAN_EDGES 5
#define EMCFANCTL_FAN_POLARITY 6
#define EMCFANCTL_FAN_PWM_BASEFREQ 7
#define EMCFANCTL_FAN_PWM_OUTPUTTYPE 8

/* APD and SMSBUS timeout sub command */

#define EMCFANCTL_APD_READ 1
#define EMCFANCTL_APD_ON 2
#define EMCFANCTL_APD_OFF 3

/* FAN driver, divider, edges, min_expected_rpm
 * and base_freqsub commands
 */
#define EMCFANCTL_FAN_DD_READ 1
#define EMCFANCTL_FAN_DD_WRITE 2

/* FAN polarity sub commands */
#define EMCFANCTL_FAN_P_READ 1
#define EMCFANCTL_FAN_P_INVERTED 2
#define EMCFANCTL_FAN_P_NONINVERTED 3

/* FAN pwm_output_type sub commands */
#define EMCFANCTL_FAN_OT_READ 1
#define EMCFANCTL_FAN_OT_PUSHPULL 2
#define EMCFANCTL_FAN_OT_OPENDRAIN 3

struct emcfanctlcmd {
	const char	*cmd;
	const int	id;
	const char	*helpargs;
};

struct emcfan_registers {
	const char	*name;
	const uint8_t	reg;
};

#define EMCFAN_TRANSLATE_INT 1
#define EMCFAN_TRANSLATE_STR 2
#define EMCFAN_NO_INSTANCE -10191

struct emcfan_bits_translate {
	const int	type;
	const uint8_t	clear_mask;
	const uint8_t	bit_mask;
	const int	human_int;
	const char	*human_str;
	const int	instance;
};

#define EMCFAN_FAMILY_UNKNOWN 0

#endif
