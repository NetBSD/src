/*	$NetBSD: linux_i2c.c,v 1.2.10.3 2017/12/03 11:38:00 jdolecek Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_i2c.c,v 1.2.10.3 2017/12/03 11:38:00 jdolecek Exp $");

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/queue.h>		/* XXX include order botch: i2cvar.h needs */

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h> /* XXX include order botch */

#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>

static int	netbsd_i2c_transfer(i2c_tag_t, struct i2c_msg *, int);
static i2c_op_t	linux_i2c_flags_op(uint16_t, bool);
static int	linux_i2c_flags_flags(uint16_t);
static uint32_t	linux_i2cbb_functionality(struct i2c_adapter *);
static int	linux_i2cbb_xfer(struct i2c_adapter *, struct i2c_msg *, int);
static void	linux_i2cbb_set_bits(void *, uint32_t);
static uint32_t	linux_i2cbb_read_bits(void *);
static void	linux_i2cbb_set_dir(void *, uint32_t);
static int	linux_i2cbb_send_start(void *, int);
static int	linux_i2cbb_send_stop(void *, int);
static int	linux_i2cbb_initiate_xfer(void *, i2c_addr_t, int);
static int	linux_i2cbb_read_byte(void *, uint8_t *, int);
static int	linux_i2cbb_write_byte(void *, uint8_t, int);

/*
 * Client operations: operations with a particular i2c slave device.
 */

struct i2c_client *
i2c_new_device(struct i2c_adapter *adapter, const struct i2c_board_info *info)
{
	struct i2c_client *client;

	client = kmem_alloc(sizeof(*client), KM_SLEEP);
	client->adapter = adapter;
	client->addr = info->addr;
	client->flags = info->flags;

	return client;
}

void
i2c_unregister_device(struct i2c_client *client)
{

	kmem_free(client, sizeof(*client));
}

int
i2c_master_send(const struct i2c_client *client, const char *buf, int count)
{
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = client->flags & I2C_M_TEN,
		.len = count,
		.buf = __UNCONST(buf),
	};
	int ret;

	KASSERT(0 <= count);

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret <= 0)
		return ret;

	return count;
}

int
i2c_master_recv(const struct i2c_client *client, char *buf, int count)
{
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = (client->flags & I2C_M_TEN) | I2C_M_RD,
		.len = count,
		.buf = buf,
	};
	int ret;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret <= 0)
		return ret;

	return count;
}

/*
 * Adapter operations: operations over an i2c bus via a particular
 * controller.
 */

int
i2c_transfer(struct i2c_adapter *adapter, struct i2c_msg *msgs, int n)
{

	return (*adapter->algo->master_xfer)(adapter, msgs, n);
}

static int
netbsd_i2c_transfer(i2c_tag_t i2c, struct i2c_msg *msgs, int n)
{
	int i;
	int error;

	for (i = 0; i < n; i++) {
		const i2c_op_t op = linux_i2c_flags_op(msgs[i].flags,
		    ((i + 1) == n));
		const int flags = linux_i2c_flags_flags(msgs[i].flags);

		switch (op) {
		case I2C_OP_READ:
		case I2C_OP_READ_WITH_STOP:
			error = iic_exec(i2c, op, msgs[i].addr,
			    NULL, 0, msgs[i].buf, msgs[i].len, flags);
			break;

		case I2C_OP_WRITE:
		case I2C_OP_WRITE_WITH_STOP:
			error = iic_exec(i2c, op, msgs[i].addr,
			    msgs[i].buf, msgs[i].len, NULL, 0, flags);
			break;

		default:
			error = EINVAL;
		}

		if (error)
			/* XXX errno NetBSD->Linux */
			return -error;
	}

	return n;
}

static i2c_op_t
linux_i2c_flags_op(uint16_t flags, bool stop)
{

	if (ISSET(flags, I2C_M_RD))
		return (stop? I2C_OP_READ_WITH_STOP : I2C_OP_READ);
	else
		return (stop? I2C_OP_WRITE_WITH_STOP : I2C_OP_WRITE);
}

static int
linux_i2c_flags_flags(uint16_t flags __unused)
{

	return 0;
}

/* Bit-banging */

const struct i2c_algorithm i2c_bit_algo = {
	.master_xfer	= linux_i2cbb_xfer,
	.functionality	= linux_i2cbb_functionality,
};

static uint32_t
linux_i2cbb_functionality(struct i2c_adapter *adapter __unused)
{
	uint32_t functions = 0;

	functions |= I2C_FUNC_I2C;
	functions |= I2C_FUNC_NOSTART;
	functions |= I2C_FUNC_SMBUS_EMUL;
	functions |= I2C_FUNC_SMBUS_READ_BLOCK_DATA;
	functions |= I2C_FUNC_SMBUS_BLOCK_PROC_CALL;
#if 0
	functions |= I2C_FUNC_10BIT_ADDR;
	functions |= I2C_FUNC_PROTOCOL_MANGLING;
#endif

	return functions;
}

static int
linux_i2cbb_xfer(struct i2c_adapter *adapter, struct i2c_msg *msgs, int n)
{
	struct i2c_algo_bit_data *const abd = adapter->algo_data;
	struct i2c_controller controller = {
		.ic_cookie		= abd,
		.ic_send_start		= linux_i2cbb_send_start,
		.ic_send_stop		= linux_i2cbb_send_stop,
		.ic_initiate_xfer	= linux_i2cbb_initiate_xfer,
		.ic_read_byte		= linux_i2cbb_read_byte,
		.ic_write_byte		= linux_i2cbb_write_byte,
	};
	i2c_tag_t i2c = &controller;
	int error;

	if (abd->pre_xfer) {
		error = (*abd->pre_xfer)(adapter);
		if (error)
			return error;
	}

	error = netbsd_i2c_transfer(i2c, msgs, n);

	if (abd->post_xfer)
		(*abd->post_xfer)(adapter);

	return error;
}

#define	LI2CBB_SDA	0x01
#define	LI2CBB_SCL	0x02
#define	LI2CBB_INPUT	0x04
#define	LI2CBB_OUTPUT	0x08

static struct i2c_bitbang_ops linux_i2cbb_ops = {
	.ibo_set_bits	= linux_i2cbb_set_bits,
	.ibo_set_dir	= linux_i2cbb_set_dir,
	.ibo_read_bits	= linux_i2cbb_read_bits,
	.ibo_bits	= {
		[I2C_BIT_SDA]		= LI2CBB_SDA,
		[I2C_BIT_SCL]		= LI2CBB_SCL,
		[I2C_BIT_INPUT]		= LI2CBB_INPUT,
		[I2C_BIT_OUTPUT]	= LI2CBB_OUTPUT,
	},
};

static void
linux_i2cbb_set_bits(void *cookie, uint32_t bits)
{
	struct i2c_algo_bit_data *const abd = cookie;

	(*abd->setsda)(abd->data, (ISSET(bits, LI2CBB_SDA)? 1 : 0));
	(*abd->setscl)(abd->data, (ISSET(bits, LI2CBB_SCL)? 1 : 0));
}

static uint32_t
linux_i2cbb_read_bits(void *cookie)
{
	struct i2c_algo_bit_data *const abd = cookie;
	uint32_t bits = 0;

	if ((*abd->getsda)(abd->data))
		bits |= LI2CBB_SDA;
	if ((*abd->getscl)(abd->data))
		bits |= LI2CBB_SCL;

	return bits;
}

static void
linux_i2cbb_set_dir(void *cookie __unused, uint32_t bits __unused)
{
	/* Linux doesn't do anything here...  */
}

static int
linux_i2cbb_send_start(void *cookie, int flags)
{

	return i2c_bitbang_send_start(cookie, flags, &linux_i2cbb_ops);
}

static int
linux_i2cbb_send_stop(void *cookie, int flags)
{

	return i2c_bitbang_send_stop(cookie, flags, &linux_i2cbb_ops);
}

static int
linux_i2cbb_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{

	return i2c_bitbang_initiate_xfer(cookie, addr, flags,
	    &linux_i2cbb_ops);
}

static int
linux_i2cbb_read_byte(void *cookie, uint8_t *bytep, int flags)
{

	return i2c_bitbang_read_byte(cookie, bytep, flags, &linux_i2cbb_ops);
}

static int
linux_i2cbb_write_byte(void *cookie, uint8_t byte, int flags)
{

	return i2c_bitbang_write_byte(cookie, byte, flags, &linux_i2cbb_ops);
}
