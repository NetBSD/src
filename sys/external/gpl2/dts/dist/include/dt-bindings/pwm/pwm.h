/*	$NetBSD: pwm.h,v 1.1.1.1 2017/06/15 20:14:23 jmcneill Exp $	*/

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
