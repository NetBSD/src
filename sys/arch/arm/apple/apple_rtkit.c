/*	$NetBSD: apple_rtkit.c,v 1.1 2022/04/27 08:06:20 skrll Exp $ */
/*	$OpenBSD: rtkit.c,v 1.3 2022/01/10 09:07:28 kettenis Exp $	*/

/*
 * Copyright (c) 2021 Mark Kettenis <kettenis@openbsd.org>
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

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>

#include <arm/apple/apple_rtkit.h>
#include <arm/apple/apple_mbox.h>


#define RTKIT_EP_MGMT			0
#define RTKIT_EP_CRASHLOG		1
#define RTKIT_EP_SYSLOG			2
#define RTKIT_EP_DEBUG			3
#define RTKIT_EP_IOREPORT		4

#define RTKIT_MGMT_TYPE_MASK		__BITS(59, 52)
#define RTKIT_MGMT_TYPE(x)		__SHIFTOUT((x), RTKIT_MGMT_TYPE_MASK)

#define RTKIT_MGMT_PWR_STATE_MASK	__BITS(7, 0)
#define RTKIT_MGMT_PWR_STATE(x)		__SHIFTOUT((x), RTKIT_MGMT_PWR_STATE_MASK)
#define RTKIT_MGMT_PWR_STATE_ON		0x20

#define RTKIT_MGMT_HELLO		1
#define RTKIT_MGMT_HELLO_ACK		2
#define RTKIT_MGMT_STARTEP		5
#define RTKIT_MGMT_IOP_PWR_STATE	6
#define RTKIT_MGMT_IOP_PWR_STATE_ACK	7
#define RTKIT_MGMT_EPMAP		8

#define RTKIT_MGMT_HELLO_MINVER_MASK	__BITS(15, 0)
#define RTKIT_MGMT_HELLO_MINVER(x)	__SHIFTOUT((x), RTKIT_MGMT_HELLO_MINVER_MASK)
#define RTKIT_MGMT_HELLO_MAXVER_MASK	__BITS(31, 16)
#define RTKIT_MGMT_HELLO_MAXVER(x)	__SHIFTOUT((x), RTKIT_MGMT_HELLO_MAXVER_MASK)

#define RTKIT_MGMT_STARTEP_EP_SHIFT	32
#define RTKIT_MGMT_STARTEP_EP_MASK	__BITS(39, 32)
#define RTKIT_MGMT_STARTEP_START	__BIT(1)

#define RTKIT_MGMT_EPMAP_LAST		__BIT(51)
#define RTKIT_MGMT_EPMAP_BASE_MASK	__BITS(34, 32)
#define RTKIT_MGMT_EPMAP_BASE(x)	__SHIFTOUT((x), RTKIT_MGMT_EPMAP_BASE_MASK)
#define RTKIT_MGMT_EPMAP_BITMAP_MASK	__BITS(31, 0)
#define RTKIT_MGMT_EPMAP_BITMAP(x)	__SHIFTOUT((x), RTKIT_MGMT_EPMAP_BITMAP_MASK)
#define RTKIT_MGMT_EPMAP_MORE		__BIT(0)

#define RTKIT_BUFFER_REQUEST		1
#define RTKIT_BUFFER_ADDR_MASK		__BITS(41, 0)
#define RTKIT_BUFFER_ADDR(x)		__SHIFTOUT((x), RTKIT_BUFFER_ADDR_MASK)
#define RTKIT_BUFFER_SIZE_MASK		__BITS(51, 44)
#define RTKIT_BUFFER_SIZE(x)		__SHIFTOUT((x), RTKIT_BUFFER_SIZE_MASK)

/* Versions we support. */
#define RTKIT_MINVER			11
#define RTKIT_MAXVER			12

struct rtkit_state {
	struct fdtbus_mbox_channel	*mc;
	int			pwrstate;
	uint64_t		epmap;
	void			(*callback[32])(void *, uint64_t);
	void			*arg[32];
};

static int
rtkit_recv(struct fdtbus_mbox_channel *mc, struct apple_mbox_msg *msg)
{
	int error, timo;

	for (timo = 0; timo < 10000; timo++) {
		error = fdtbus_mbox_recv(mc, msg, sizeof(*msg));
		if (error == 0)
			break;
		delay(10);
	}

	return error;
}

static int
rtkit_send(struct fdtbus_mbox_channel *mc, uint32_t endpoint,
    uint64_t type, uint64_t data)
{
	struct apple_mbox_msg msg;

	msg.data0 = __SHIFTIN(type, RTKIT_MGMT_TYPE_MASK) | data;
	msg.data1 = endpoint;
	return fdtbus_mbox_send(mc, &msg, sizeof(msg));
}

static int
rtkit_start(struct rtkit_state *state, uint32_t endpoint)
{
	struct fdtbus_mbox_channel *mc = state->mc;
	uint64_t reply;

	reply = __SHIFTIN(endpoint, RTKIT_MGMT_STARTEP_EP_MASK);
	reply |= RTKIT_MGMT_STARTEP_START;
	return rtkit_send(mc, RTKIT_EP_MGMT, RTKIT_MGMT_STARTEP, reply);
}

static int
rtkit_handle_mgmt(struct rtkit_state *state, struct apple_mbox_msg *msg)
{
	struct fdtbus_mbox_channel *mc = state->mc;
	uint64_t minver, maxver, ver;
	uint64_t base, bitmap, reply;
	uint32_t endpoint;
	int error;

	switch (RTKIT_MGMT_TYPE(msg->data0)) {
	case RTKIT_MGMT_HELLO:
		minver = RTKIT_MGMT_HELLO_MINVER(msg->data0);
		maxver = RTKIT_MGMT_HELLO_MAXVER(msg->data0);
		if (minver > RTKIT_MAXVER) {
			printf("unsupported minimum firmware version "
			    "%"PRId64"\n", minver);
			return EINVAL;
		}
		if (maxver < RTKIT_MINVER) {
			printf("unsupported maximum firmware version "
			"%"PRId64"\n", maxver);
			return EINVAL;
		}
		ver = MIN(RTKIT_MAXVER, maxver);
		error = rtkit_send(mc, RTKIT_EP_MGMT, RTKIT_MGMT_HELLO_ACK,
		    __SHIFTIN(ver, RTKIT_MGMT_HELLO_MINVER_MASK) |
		    __SHIFTIN(ver, RTKIT_MGMT_HELLO_MAXVER_MASK));
		if (error)
			return error;
		break;

	case RTKIT_MGMT_IOP_PWR_STATE_ACK:
		state->pwrstate = RTKIT_MGMT_PWR_STATE(msg->data0);
		break;

	case RTKIT_MGMT_EPMAP:
		base = RTKIT_MGMT_EPMAP_BASE(msg->data0);
		bitmap = RTKIT_MGMT_EPMAP_BITMAP(msg->data0);
		state->epmap |= (bitmap << (base * 32));
		reply = __SHIFTIN(base, RTKIT_MGMT_EPMAP_BASE_MASK);
		if (msg->data0 & RTKIT_MGMT_EPMAP_LAST)
			reply |= RTKIT_MGMT_EPMAP_LAST;
		else
			reply |= RTKIT_MGMT_EPMAP_MORE;
		error = rtkit_send(state->mc, RTKIT_EP_MGMT,
		    RTKIT_MGMT_EPMAP, reply);
		if (error)
			return error;
		if (msg->data0 & RTKIT_MGMT_EPMAP_LAST) {
			for (endpoint = 1; endpoint < 32; endpoint++) {
				if ((state->epmap & __BIT(endpoint)) == 0)
					continue;

				switch (endpoint) {
				case RTKIT_EP_CRASHLOG:
				case RTKIT_EP_DEBUG:
				case RTKIT_EP_IOREPORT:
					error = rtkit_start(state, endpoint);
					if (error)
						return error;
					break;
				}
			}
		}
		break;
	default:
		printf("unhandled management event "
		    "0x%016"PRIx64"\n", msg->data0);
		return EIO;
	}

	return 0;
}

static int
rtkit_handle_crashlog(struct rtkit_state *state, struct apple_mbox_msg *msg)
{
	struct fdtbus_mbox_channel *mc = state->mc;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	switch (RTKIT_MGMT_TYPE(msg->data0)) {
	case RTKIT_BUFFER_REQUEST:
		addr = RTKIT_BUFFER_ADDR(msg->data0);
		size = RTKIT_BUFFER_SIZE(msg->data0);
		// XXXNH WTF is this conditional
		if (addr)
			break;

		error = rtkit_send(mc, RTKIT_EP_CRASHLOG, RTKIT_BUFFER_REQUEST,
		    __SHIFTIN(size, RTKIT_BUFFER_SIZE_MASK) | addr);
		if (error)
			return error;
		break;
	default:
		printf("unhandled crashlog event "
		    "0x%016"PRIx64"\n", msg->data0);
		return EIO;
	}

	return 0;
}

static int
rtkit_handle_ioreport(struct rtkit_state *state, struct apple_mbox_msg *msg)
{
	struct fdtbus_mbox_channel *mc = state->mc;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	switch (RTKIT_MGMT_TYPE(msg->data0)) {
	case RTKIT_BUFFER_REQUEST:
		addr = RTKIT_BUFFER_ADDR(msg->data0);
		size = RTKIT_BUFFER_SIZE(msg->data0);
		// XXXNH WTF is this conditional
		if (addr)
			break;
		error = rtkit_send(mc, RTKIT_EP_IOREPORT, RTKIT_BUFFER_REQUEST,
		    __SHIFTIN(size, RTKIT_BUFFER_SIZE_MASK) | addr);
		if (error)
			return error;
		break;
	default:
		printf("unhandled ioreport event"
		    "0x%016"PRIx64"\n", msg->data0);
		return EIO;
	}

	return 0;
}

int
rtkit_poll(struct rtkit_state *state)
{
	struct fdtbus_mbox_channel *mc = state->mc;
	struct apple_mbox_msg msg;
	void (*callback)(void *, uint64_t);
	void *arg;
	uint32_t endpoint;
	int error;

	error = rtkit_recv(mc, &msg);
	if (error)
		return error;

	endpoint = msg.data1;
	switch (endpoint) {
	case RTKIT_EP_MGMT:
		error = rtkit_handle_mgmt(state, &msg);
		if (error)
			return error;
		break;
	case RTKIT_EP_CRASHLOG:
		error = rtkit_handle_crashlog(state, &msg);
		if (error)
			return error;
		break;
	case RTKIT_EP_IOREPORT:
		error = rtkit_handle_ioreport(state, &msg);
		if (error)
			return error;
		break;
	default:
		if (endpoint >= 32 && endpoint < 64 &&
		    state->callback[endpoint - 32]) {
			callback = state->callback[endpoint - 32];
			arg = state->arg[endpoint - 32];
			callback(arg, msg.data0);
			break;
		}

		printf("unhandled endpoint %d\n", msg.data1);
		return EIO;
	}

	return 0;
}

static void
rtkit_rx_callback(void *cookie)
{
	rtkit_poll(cookie);
}

struct rtkit_state *
rtkit_init(int node, const char *name)
{
	struct rtkit_state *state;

	state = kmem_zalloc(sizeof(*state), KM_SLEEP);
	if (name) {
		state->mc = fdtbus_mbox_get(node, name, rtkit_rx_callback, state);
	} else {
		state->mc = fdtbus_mbox_get_index(node, 0, rtkit_rx_callback, state);
	}
	if (state->mc == NULL) {
		kmem_free(state, sizeof(*state));

		return NULL;
	}

	return state;
}

int
rtkit_boot(struct rtkit_state *state)
{
	struct fdtbus_mbox_channel *mc = state->mc;
	int error;

	/* Wake up! */
	error = rtkit_send(mc, RTKIT_EP_MGMT, RTKIT_MGMT_IOP_PWR_STATE,
	    RTKIT_MGMT_PWR_STATE_ON);
	if (error) {
		return error;
	}

	while (state->pwrstate != RTKIT_MGMT_PWR_STATE_ON)
		rtkit_poll(state);

	return 0;
}

int
rtkit_start_endpoint(struct rtkit_state *state, uint32_t endpoint,
    void (*callback)(void *, uint64_t), void *arg)
{
	if (endpoint < 32 || endpoint >= 64)
		return EINVAL;

	if ((state->epmap & __BIT(endpoint)) == 0)
		return EINVAL;

	state->callback[endpoint - 32] = callback;
	state->arg[endpoint - 32] = arg;
	return rtkit_start(state, endpoint);
}

int
rtkit_send_endpoint(struct rtkit_state *state, uint32_t endpoint,
    uint64_t data)
{

	return rtkit_send(state->mc, endpoint, 0, data);
}
