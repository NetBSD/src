/*	$NetBSD: i2c-algo-bit.h,v 1.2.2.1 2014/08/10 06:55:39 tls Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LINUX_I2C_ALGO_BIT_H_
#define _LINUX_I2C_ALGO_BIT_H_

#include <linux/i2c.h>

struct i2c_algo_bit_data {
	void		*data;
	void		(*setsda)(void *, int);
	void		(*setscl)(void *, int);
	int		(*getsda)(void *);
	int		(*getscl)(void *);
	int		(*pre_xfer)(struct i2c_adapter *);
	void		(*post_xfer)(struct i2c_adapter *);
	int		udelay;
	int		timeout;
};

/* XXX Make the nm output a little more greppable...  */
#define	i2c_bit_algo	linux_i2c_bit_algo

extern const struct i2c_algorithm i2c_bit_algo;

static inline int
i2c_bit_add_bus(struct i2c_adapter *adapter)
{

	adapter->algo = &i2c_bit_algo;
	return 0;
}

#endif  /* _LINUX_I2C_ALGO_BIT_H_ */
