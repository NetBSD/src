/*	$NetBSD: i2c.h,v 1.5.4.3 2017/12/03 11:37:59 jdolecek Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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
#include <sys/systm.h>

#include <dev/i2c/i2cvar.h>

#include <linux/pm.h>

struct i2c_adapter;
struct i2c_algorithm;
struct i2c_msg;

#define	I2C_NAME_SIZE	20

/*
 * I2C_M_*: i2c_msg flags
 */
#define	I2C_M_RD		0x01 /* xfer is read, not write */
#define	I2C_M_NOSTART		0x02 /* don't initiate xfer */
#define	I2C_M_TEN		0x04 /* 10-bit chip address */

/*
 * I2C_CLASS_*: i2c_adapter classes
 */
#define	I2C_CLASS_DDC	0x01

/*
 * I2C_FUNC_*: i2c_adapter functionality bits
 */
#define	I2C_FUNC_I2C			0x01
#define	I2C_FUNC_NOSTART		0x02
#define	I2C_FUNC_SMBUS_EMUL		0x04
#define	I2C_FUNC_SMBUS_READ_BLOCK_DATA	0x08
#define	I2C_FUNC_SMBUS_BLOCK_PROC_CALL	0x10
#define	I2C_FUNC_10BIT_ADDR		0x20

/*
 * struct i2c_msg: A single i2c message request on a particular
 * address.  Read if I2C_M_RD is set, write otherwise.
 */
struct i2c_msg {
	i2c_addr_t	addr;
	uint16_t	flags;	/* I2C_M_* */
	uint16_t	len;
	uint8_t		*buf;
};

/*
 * struct i2c_adapter: An i2c bus controller.
 */
struct i2c_adapter {
	char		 		name[I2C_NAME_SIZE];
	const struct i2c_algorithm	*algo;
	void				*algo_data;
	int				retries;
	struct module			*owner;
	unsigned int			class; /* I2C_CLASS_* */
	struct {
		device_t	parent;
	}				dev;
	void				*i2ca_adapdata;
};

/*
 * struct i2c_algorithm: A procedure for transferring an i2c message on
 * an i2c bus, along with a set of flags describing its functionality.
 */
struct i2c_algorithm {
	int		(*master_xfer)(struct i2c_adapter *, struct i2c_msg *,
			    int);
	uint32_t	(*functionality)(struct i2c_adapter *);
};

/*
 * struct i2c_board_info: Parameters to find an i2c bus and a slave on
 * it.  type is the name of an i2c driver; addr is the slave address;
 * platform_data is an extra parameter to pass to the i2c driver.
 */
struct i2c_board_info {
	char			type[I2C_NAME_SIZE];
	uint16_t		addr;
	uint16_t		flags;
	void			*platform_data;
};

#define	I2C_BOARD_INFO(board_type, board_addr)		\
	.type = (board_type),				\
	.addr = (board_addr)

/*
 * struct i2c_client: An i2c slave device at a particular address on a
 * particular bus.
 */
struct i2c_client {
	struct i2c_adapter	*adapter;
	uint16_t		addr;
	uint16_t		flags;
};

/*
 * struct i2c_device_id: Device id naming a class of i2c slave devices
 * and parameters to the driver for the devices.
 */
struct i2c_device_id {
	char		name[I2C_NAME_SIZE];
	unsigned long	driver_data;
};

/*
 * struct i2c_driver: A driver for a class of i2c slave devices.  We
 * don't actually use this.
 */
struct i2c_driver {
	int	(*probe)(struct i2c_client *, const struct i2c_device_id *);
	int	(*remove)(struct i2c_client *);
	struct {
		char			name[I2C_NAME_SIZE];
		const struct dev_pm_ops	pm;
	}	driver;
};

/*
 * Adapter management.  We don't register these in a global database
 * like Linux, so these are just stubs.
 */
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

/* XXX Make the nm output a little more greppable...  */
#define	i2c_master_recv		linux_i2c_master_recv
#define	i2c_master_send		linux_i2c_master_send
#define	i2c_new_device		linux_i2c_new_device
#define	i2c_transfer		linux_i2c_transfer
#define	i2c_unregister_device	linux_i2c_unregister_device

int	i2c_master_send(const struct i2c_client *, const char *, int);
int	i2c_master_recv(const struct i2c_client *, char *, int);
struct i2c_client *
	i2c_new_device(struct i2c_adapter *, const struct i2c_board_info *);
int	i2c_transfer(struct i2c_adapter *, struct i2c_msg *, int);
void	i2c_unregister_device(struct i2c_client *);

#endif  /* _LINUX_I2C_H_ */
