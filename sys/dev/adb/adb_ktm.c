/*	$NetBSD: adb_ktm.c,v 1.5 2022/04/06 17:37:31 macallan Exp $	*/

/*-
 * Copyright (c) 2019 Michael Lorenz
 * All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: adb_ktm.c,v 1.5 2022/04/06 17:37:31 macallan Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>

#include <machine/autoconf.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include <machine/adbsys.h>
#include <dev/adb/adbvar.h>

#include "adbdebug.h"

#ifdef KTM_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

/*
 * State info, per mouse instance.
 */
struct ktm_softc {
	device_t	sc_dev;
	struct adb_device *sc_adbdev;
	struct adb_bus_accessops *sc_ops;

	uint8_t		sc_us;		/* cmd to watch for */
	device_t	sc_wsmousedev;
	/* buffers */
	uint8_t		sc_config[8];
	int		sc_left;
	int		sc_right;
	int		sc_poll;
	int		sc_msg_len;
	int		sc_event;
	uint8_t		sc_buffer[16];
};

/*
 * Function declarations.
 */
static int	ktm_match(device_t, cfdata_t, void *);
static void	ktm_attach(device_t, device_t, void *);
static void	ktm_init(struct ktm_softc *);
static void	ktm_write_config(struct ktm_softc *);
static void	ktm_buttons(struct ktm_softc *);
static void	ktm_process_event(struct ktm_softc *, int, uint8_t *);
static int	ktm_send_sync(struct ktm_softc *, uint8_t, int, uint8_t *);

/* Driver definition. */
CFATTACH_DECL_NEW(ktm, sizeof(struct ktm_softc),
    ktm_match, ktm_attach, NULL, NULL);

static int ktm_enable(void *);
static int ktm_ioctl(void *, u_long, void *, int, struct lwp *);
static void ktm_disable(void *);

static void ktm_handler(void *, int, uint8_t *);
static int  ktm_wait(struct ktm_softc *, int);
static int  sysctl_ktm_left(SYSCTLFN_ARGS);
static int  sysctl_ktm_right(SYSCTLFN_ARGS);

const struct wsmouse_accessops ktm_accessops = {
	ktm_enable,
	ktm_ioctl,
	ktm_disable,
};

static int
ktm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct adb_attach_args *aaa = aux;
	if ((aaa->dev->original_addr == ADBADDR_MS) &&
	    (aaa->dev->handler_id == ADBMS_TURBO))
		return 50;	/* beat out adbms */
	else
		return 0;
}

static void
ktm_attach(device_t parent, device_t self, void *aux)
{
	struct ktm_softc *sc = device_private(self);
	struct adb_attach_args *aaa = aux;
	struct wsmousedev_attach_args a;

	sc->sc_dev = self;
	sc->sc_ops = aaa->ops;
	sc->sc_adbdev = aaa->dev;
	sc->sc_adbdev->cookie = sc;
	sc->sc_adbdev->handler = ktm_handler;
	sc->sc_us = ADBTALK(sc->sc_adbdev->current_addr, 0);
	printf(" addr %d: Kensington Turbo Mouse\n",
	    sc->sc_adbdev->current_addr);

	sc->sc_poll = 0;
	sc->sc_msg_len = 0;

	ktm_init(sc);

	a.accessops = &ktm_accessops;
	a.accesscookie = sc;
	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint, CFARGS_NONE);
}

static int
ktm_turbo_csum(uint8_t *d)
{
	int i = 0, sum = 0;

	for (i = 0; i < 7; i++)
		sum ^= d[i];
	return (sum ^ 0xff);
}

static void
ktm_write_config(struct ktm_softc *sc)
{
	uint8_t addr = sc->sc_adbdev->current_addr;

	ktm_send_sync(sc, ADBFLUSH(addr), 0, NULL);
	sc->sc_config[7] = ktm_turbo_csum(sc->sc_config);
	ktm_send_sync(sc, ADBLISTEN(addr, 2), 8, sc->sc_config);
}

static uint8_t button_to_reg[] = {0, 1, 4, 6};

static void ktm_buttons(struct ktm_softc *sc)
{
	uint8_t reg;

	reg = button_to_reg[sc->sc_right] |
	      (button_to_reg[sc->sc_left] << 3);
	sc->sc_config[1] = reg;
}

static void
ktm_init(struct ktm_softc *sc)
{
	const struct sysctlnode *me = NULL, *node = NULL;
	int ret;

	/* Found Kensington Turbo Mouse */

/*
 * byte 0
 - 0x80 enables EMP output
 - 0x08 seems to map both buttons together
 - 0x04 enables the 2nd button
 - initialized to 0x20 on power up, no idea what that does
 
 * byte 1 assigns what which button does
 - 0x08 - button 1 - 1, button 2 - nothing
 - 0x09 - both buttons - 1
 - 0x0a - butoon 1 - 1, button 2 - toggle 1
 - 0x0b - button 1 - 1, button 2 - nothing
 - 0x0c - button 1 - 1, button 2 - 2
 - 0x0e - button 1 - 1, button 2 - 3
 - 0x0f - button 1 - 1, button 2 - toggle 3
 - 0x10 - button 1 toggle 1, button 2 nothing
 - 0x11 - button 1 - toggle 1, button 2 - 1
 - 0x12 - both toggle 1
 - 0x14 - button 1 toggle 1, button 2 - 2
 - 0x21 - button 1 - 2, button 2 - 1
 - 0x31 - button 1 - 3, button 2 - 1

 * byte 2 - 0x40 on powerup, seems to do nothing
 * byte 3 - 0x01 on powerup, seems to do nothing
 * byte 4 programs a delay for button presses, apparently in 1/100 seconds
 * byte 5 and 6 init to 0xff
 * byte 7 is a simple XOR checksum, writes will only stick if it's valid
          as in, b[7] = (b[0] ^ b[1] ^ ... ^ b[6]) ^ 0xff
 */
 
	/* this seems to be the most reasonable default */
	uint8_t data[8] = { 0xa5, 0x0e, 0, 0, 1, 0xff, 0xff, 0 };

	memcpy(sc->sc_config, data, sizeof(data));

	sc->sc_left = 1;
	sc->sc_right = 3;

	ktm_buttons(sc);

#ifdef KTM_DEBUG
	int addr = sc->sc_adbdev->current_addr;
	{
		int i;
		ktm_send_sync(sc, ADBTALK(addr, 2), 0, NULL);
		printf("reg *");
		for (i = 0; i < sc->sc_msg_len; i++)
			printf(" %02x", sc->sc_buffer[i]);
		printf("\n");
	}
#endif

	ktm_write_config(sc);

#ifdef KTM_DEBUG
	int i, reg;
	for (reg = 1; reg < 4; reg++) {
		ktm_send_sync(sc, ADBTALK(addr, reg), 0, NULL);
		printf("reg %d", reg);
		for (i = 0; i < sc->sc_msg_len; i++)
			printf(" %02x", sc->sc_buffer[i]);
		printf("\n");
	}
#endif
	ret = sysctl_createv(NULL, 0, NULL, &me,
	    CTLFLAG_READWRITE,
	    CTLTYPE_NODE, device_xname(sc->sc_dev), NULL,
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	ret = sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "left", "left button assigmnent",
	    sysctl_ktm_left, 1, (void *)sc, 0,
	    CTL_MACHDEP, me->sysctl_num, CTL_CREATE, CTL_EOL);

	ret = sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "right", "right button assigmnent",
	    sysctl_ktm_right, 1, (void *)sc, 0,
	    CTL_MACHDEP, me->sysctl_num, CTL_CREATE, CTL_EOL);
	__USE(ret);
}

static void
ktm_handler(void *cookie, int len, uint8_t *data)
{
	struct ktm_softc *sc = cookie;

#ifdef KTM_DEBUG
	int i;
	printf("%s: %02x - ", device_xname(sc->sc_dev), sc->sc_us);
	for (i = 0; i < len; i++) {
		printf(" %02x", data[i]);
	}
	printf("\n");
#endif
	if (len >= 2) {
		memcpy(sc->sc_buffer, &data[2], len - 2);
		sc->sc_msg_len = len - 2;
		if (data[1] == sc->sc_us) {
			/* make sense of the mouse message */
			ktm_process_event(sc, sc->sc_msg_len, sc->sc_buffer);
			return;
		}
		wakeup(&sc->sc_event);
	} else {
		DPRINTF("bogus message\n");
	}
}

static void
ktm_process_event(struct ktm_softc *sc, int len, uint8_t *buffer)
{
	int buttons = 0, mask, dx, dy, i;
	int button_bit = 1;

	/* Classic Mouse Protocol (up to 2 buttons) */
	for (i = 0; i < 2; i++, button_bit <<= 1)
		/* 0 when button down */
		if (!(buffer[i] & 0x80))
			buttons |= button_bit;
		else
			buttons &= ~button_bit;
	/* Extended Protocol (up to 6 more buttons) */
	for (mask = 0x80; i < len;
	     i += (mask == 0x80), button_bit <<= 1) {
		/* 0 when button down */
		if (!(buffer[i] & mask))
			buttons |= button_bit;
		else
			buttons &= ~button_bit;
		mask = ((mask >> 4) & 0xf) | ((mask & 0xf) << 4);
	}				

	/* EMP crap, additional motion bits */
	int shift = 7, ddx, ddy, sign, smask;

#ifdef KTM_DEBUG
	printf("EMP packet:");
	for (i = 0; i < len; i++)
		printf(" %02x", buffer[i]);
	printf("\n");
#endif
	dx = (int)buffer[1] & 0x7f;
	dy = (int)buffer[0] & 0x7f;
	for (i = 2; i < len; i++) {
		ddx = (buffer[i] & 0x07);
		ddy = (buffer[i] & 0x70) >> 4;
		dx |= (ddx << shift);
		dy |= (ddy << shift);
		shift += 3;
	}
	sign = 1 << (shift - 1);
	smask = 0xffffffff << shift;
	if (dx & sign)
		dx |= smask;
	if (dy & sign)
		dy |= smask;
#ifdef KTM_DEBUG
	printf("%d %d %08x %d\n", dx, dy, smask, shift);
#endif

	if (sc->sc_wsmousedev)
		wsmouse_input(sc->sc_wsmousedev, buttons,
			      dx, -dy, 0, 0,
			      WSMOUSE_INPUT_DELTA);
}

static int
ktm_enable(void *v)
{
	return 0;
}

static int
ktm_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_ADB;
		break;

	default:
		return EPASSTHROUGH;
	}
	return 0;
}

static void
ktm_disable(void *v)
{
}

static int
ktm_wait(struct ktm_softc *sc, int timeout)
{
	int cnt = 0;
	
	if (sc->sc_poll) {
		while (sc->sc_msg_len == -1) {
			sc->sc_ops->poll(sc->sc_ops->cookie);
		}
	} else {
		while ((sc->sc_msg_len == -1) && (cnt < timeout)) {
			tsleep(&sc->sc_event, 0, "ktmio", hz);
			cnt++;
		}
	}
	return (sc->sc_msg_len > 0);
}

static int
ktm_send_sync(struct ktm_softc *sc, uint8_t cmd, int len, uint8_t *msg)
{
	int i;

	sc->sc_msg_len = -1;
	DPRINTF("send: %02x", cmd);
	for (i = 0; i < len; i++)
		DPRINTF(" %02x", msg[i]);
	DPRINTF("\n");
	sc->sc_ops->send(sc->sc_ops->cookie, sc->sc_poll, cmd, len, msg);
	ktm_wait(sc, 3);
	return (sc->sc_msg_len != -1);
}

static int
sysctl_ktm_left(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct ktm_softc *sc = node.sysctl_data;
	int reg = sc->sc_left;

	if (newp) {

		/* we're asked to write */	
		node.sysctl_data = &reg;
		if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {

			reg = *(int *)node.sysctl_data;
			if ((reg != sc->sc_left) &&
			    (reg >= 0) &&
			    (reg < 4)) {
				sc->sc_left = reg;
				ktm_buttons(sc);
				ktm_write_config(sc);
			}
			return 0;
		}
		return EINVAL;
	} else {

		node.sysctl_data = &reg;
		node.sysctl_size = 4;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	}

	return 0;
}

static int
sysctl_ktm_right(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct ktm_softc *sc = node.sysctl_data;
	int reg = sc->sc_right;

	if (newp) {

		/* we're asked to write */	
		node.sysctl_data = &reg;
		if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {

			reg = *(int *)node.sysctl_data;
			if ((reg != sc->sc_right) &&
			    (reg >= 0) &&
			    (reg < 4)) {
				sc->sc_right = reg;
				ktm_buttons(sc);
				ktm_write_config(sc);
			}
			return 0;
		}
		return EINVAL;
	} else {

		node.sysctl_data = &reg;
		node.sysctl_size = 4;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	}

	return 0;
}

SYSCTL_SETUP(sysctl_ktm_setup, "sysctl ktm subtree setup")
{

	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);
}
