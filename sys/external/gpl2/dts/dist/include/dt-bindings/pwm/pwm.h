/*	$NetBSD: pwm.h,v 1.1.1.2.2.2 2017/12/03 11:38:39 jdolecek Exp $	*/

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This header provides constants for most PWM bindings.
 *
 * Most PWM bindings can include a flags cell as part of the PWM specifier.
 * In most cases, the format of the flags cell uses the standard values
 * defined in this header.
 */

#ifndef _DT_BINDINGS_PWM_PWM_H
#define _DT_BINDINGS_PWM_PWM_H

#define PWM_POLARITY_INVERTED			(1 << 0)

#endif
