/*	$NetBSD: i2c.h,v 1.5.2.1 2014/09/21 17:41:52 snj Exp $	*/

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

#ifndef _LINUX_I2C_H_
#define _LINUX_I2C_H_

#include <sys/types.h>
#include <sys/device_if.h>
#include <sys/queue.h>		/* XXX include order botch: i2cvar.h needs */

#include <dev/i2c/i2cvar.h>

struct i2c_adapter;
struct i2c_algorithm;
struct i2c_msg;

#define	I2C_NAME_SIZE	20

#define	I2C_CLASS_DDC	0x01

struct i2c_board_info {
	char			type[I2C_NAME_SIZE];
	uint16_t		addr;
	void			*platform_data;
};

#define	I2C_BOARD_INFO(board_type, board_addr)		\
	.type = (board_type),				\
	.addr = (board_addr)

static inline void
i2c_new_device(const struct i2c_adapter *adapter __unused,
    const struct i2c_board_info *board __unused)
{
}

struct i2c_driver {
};

struct module;
static inline int
i2c_register_driver(const struct module *owner __unused,
    const struct i2c_driver *driver __unused)
{
	return 0;
}

static inline void
i2c_del_driver(const struct i2c_driver *driver __unused)
{
}

struct i2c_client;

struct i2c_adapter {
	char		 		name[I2C_NAME_SIZE];
	const struct i2c_algorithm	*algo;
	void				*algo_data;
	int				retries;
	struct module			*owner;
	unsigned int			class;
	struct {
		device_t	parent;
	}				dev;	/* XXX Kludge for intel_dp.  */
	void				*i2ca_adapdata;
};

static inline int
i2c_add_adapter(struct i2c_adapter *adapter __unused)
{
	return 0;
}

static inline void
i2c_del_adapter(struct i2c_adapter *adapter __unused)
{
}

static inline void *
i2c_get_adapdata(const struct i2c_adapter *adapter)
{

	return adapter->i2ca_adapdata;
}

static inline void
i2c_set_adapdata(struct i2c_adapter *adapter, void *data)
{

	adapter->i2ca_adapdata = data;
}

struct i2c_msg {
	i2c_addr_t	addr;
	uint16_t	flags;
	uint16_t	len;
	uint8_t		*buf;
};

#define	I2C_M_RD		0x01
#define	I2C_M_NOSTART		0x02

#define	I2C_FUNC_I2C			0x01
#define	I2C_FUNC_NOSTART		0x02
#define	I2C_FUNC_SMBUS_EMUL		0x04
#define	I2C_FUNC_SMBUS_READ_BLOCK_DATA	0x08
#define	I2C_FUNC_SMBUS_BLOCK_PROC_CALL	0x10
#define	I2C_FUNC_10BIT_ADDR		0x20

struct i2c_algorithm {
	int		(*master_xfer)(struct i2c_adapter *, struct i2c_msg *,
			    int);
	uint32_t	(*functionality)(struct i2c_adapter *);
};

/* XXX Make the nm output a little more greppable...  */
#define	i2c_transfer	linux_i2c_transfer

int	i2c_transfer(struct i2c_adapter *, struct i2c_msg *, int);

#endif  /* _LINUX_I2C_H_ */
