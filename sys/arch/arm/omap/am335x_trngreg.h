/* $NetBSD: am335x_trngreg.h,v 1.1.4.2 2015/09/22 12:05:38 skrll Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _AM335X_TRNGREG_H
#define _AM335X_TRNGREG_H

#define TRNG_OUTPUT_L_REG	0x00
#define TRNG_OUTPUT_H_REG	0x04
#define TRNG_STATUS_REG		0x08
#define TRNG_STATUS_READY		__BIT(0)
#define TRNG_INTACK_REG		0x10
#define TRNG_INTACK_READY		__BIT(0)
#define TRNG_CONTROL_REG	0x14
#define TRNG_CONTROL_STARTUP_CYCLES	__BITS(31,16)
#define TRNG_CONTROL_ENABLE		__BIT(10)
#define TRNG_CONFIG_REG		0x18
#define TRNG_CONFIG_MAX_REFILL		__BITS(31,16)
#define TRNG_CONFIG_MIN_REFILL		__BITS(7,0)

#endif /* !_AM335X_TRNGREG_H */
