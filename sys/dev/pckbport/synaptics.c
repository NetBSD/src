/*	$NetBSD: synaptics.c,v 1.2 2004/12/28 20:47:18 christos Exp $	*/

/*
 * Copyright (c) 2004, Ales Krenek
 * Copyright (c) 2004, Kentaro A. Kurahone
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the authors nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/kernel.h>
#include <sys/kthread.h>

#include <machine/bus.h>

#include <dev/pckbport/pckbportvar.h>

#include <dev/pckbport/synapticsreg.h>
#include <dev/pckbport/synapticsvar.h>

#include <dev/pckbport/pmsreg.h>
#include <dev/pckbport/pmsvar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

static int pms_synaptics_send_command(pckbport_tag_t, pckbport_slot_t, u_char);
static void pms_synaptics_spawn_thread(void *);
static void pms_synaptics_input(void *, int);
static void pms_synaptics_thread(void *);
static void pms_sysctl_synaptics(struct sysctllog **);
static int pms_sysctl_synaptics_verify(SYSCTLFN_ARGS);

/* Controled by sysctl. */
static int synaptics_slow_limit = SYNAPTICS_SLOW_LIMIT;
static int synaptics_emulate_mid = 0;
static int synaptics_tap_enable = 1;
static int synaptics_tap_length = SYNAPTICS_TAP_LENGTH;
static int synaptics_tap_tolerance = SYNAPTICS_TAP_TOLERANCE;

/* Sysctl nodes. */
static int synaptics_slow_limit_nodenum;
static int synaptics_emulate_mid_nodenum;
static int synaptics_tap_enable_nodenum;
static int synaptics_tap_length_nodenum;
static int synaptics_tap_tolerance_nodenum;

int
pms_synaptics_probe_init(void *vsc)
{
	struct pms_softc *sc = vsc;
	struct synaptics_softc	*syn_sc = &sc->u.synaptics;
	u_char cmd[2], resp[3];
	int res;
	int ver_minor, ver_major;
	struct sysctllog *clog = NULL;

	res = pms_synaptics_send_command(sc->sc_kbctag, sc->sc_kbcslot,
	    SYNAPTICS_IDENTIFY_TOUCHPAD);
	cmd[0] = PMS_SEND_DEV_STATUS;
	res |= pckbport_poll_cmd(sc->sc_kbctag, sc->sc_kbcslot, cmd,
	    1, 3, resp, 0);
	if (res) {
#ifdef SYNAPTICSDEBUG
		printf("synaptics_probe: Identify Touchpad error.\n");
#endif
		return res;
	}

	if (resp[1] != SYNAPTICS_MAGIC_BYTE) {
#ifdef SYNAPTICSDEBUG
		printf("synaptics_probe: Not synaptics.\n");
#endif
		return -1;
	}

	/* Check for minimum version and print a nice message. */
	ver_major = resp[2] & 0x0f;
	ver_minor = resp[0];
	printf("%s: Synaptics touchpad version %d.%d\n",
	    sc->sc_dev.dv_xname, ver_major, ver_minor);
	if (ver_major * 10 + ver_minor < SYNAPTICS_MIN_VERSION) {
		/* No capability query support. */
		syn_sc->caps = 0;
		syn_sc->enable_passthrough = 0;
		syn_sc->has_passthrough = 0;
		syn_sc->has_m_button = 0;
		syn_sc->has_buttons_4_5 = 0;
		syn_sc->has_buttons_4_5_ext = 0;
		goto done;
	}

	/* Query the hardware capabilities. */
	res = pms_synaptics_send_command(sc->sc_kbctag, sc->sc_kbcslot,
	    SYNAPTICS_READ_CAPABILITIES);
	cmd[0] = PMS_SEND_DEV_STATUS;
	res |= pckbport_poll_cmd(sc->sc_kbctag, sc->sc_kbcslot, cmd,
	    1, 3, resp, 0);
	if (res) {
		/* Hmm, failed to get capabilites. */
		printf("%s: Failed to query capabilities.\n",
		    sc->sc_dev.dv_xname);
		return -1;
	}

	syn_sc->caps = (resp[0] << 8) | (resp[3]);
	syn_sc->has_m_button = syn_sc->caps & SYNAPTICS_CAP_MBUTTON;
	syn_sc->has_buttons_4_5 = syn_sc->caps & SYNAPTICS_CAP_4BUTTON;
	if (!(syn_sc->caps & SYNAPTICS_CAP_EXTENDED)) {
		/* Eeep.  No extended capabilities. */
		syn_sc->has_passthrough = 0;
		syn_sc->enable_passthrough = 0;
	} else {
#ifdef SYNAPTICSDEBUG
		printf("synaptics_probe: Capabilities %2x.\n", syn_sc->caps);
#endif
		syn_sc->has_passthrough = syn_sc->caps &
		    SYNAPTICS_CAP_PASSTHROUGH;
		syn_sc->enable_passthrough = 0; /* Not yet. */

		/* Ask about extra buttons to detect up/down. */
		if (syn_sc->caps & SYNAPTICS_CAP_EXTNUM) {
			res = pms_synaptics_send_command(sc->sc_kbctag,
			    sc->sc_kbcslot, SYNAPTICS_EXTENDED_QUERY);
			cmd[0] = PMS_SEND_DEV_STATUS;
			res |= pckbport_poll_cmd(sc->sc_kbctag,
			    sc->sc_kbcslot, cmd, 1, 3, resp, 0);
			if ((!res) && ((resp[1] >> 4) >= 2)) {
				/* Yes. */
				syn_sc->has_buttons_4_5_ext = 1;
			} else {
				/* Guess not. */
				syn_sc->has_buttons_4_5_ext = 0;
			}
		} else
			syn_sc->has_buttons_4_5_ext = 0;
	}

done:
	pms_sysctl_synaptics(&clog);
	syn_sc->buf_full = 0;
	kthread_create(pms_synaptics_spawn_thread, sc);
	pckbport_set_inputhandler(sc->sc_kbctag, sc->sc_kbcslot,
	    pms_synaptics_input, sc, sc->sc_dev.dv_xname);
	return 0;
}

void
pms_synaptics_enable(void *vsc)
{
	struct pms_softc *sc = vsc;
	struct synaptics_softc	*syn_sc = &sc->u.synaptics;
	u_char cmd[2];
	int res;
	u_char mode = SYNAPTICS_MODE_ABSOLUTE | SYNAPTICS_MODE_W;

	res = pms_synaptics_send_command(sc->sc_kbctag, sc->sc_kbcslot, mode);
	cmd[0] = PMS_SET_SAMPLE;
	cmd[1] = 0x14; /* doit */
	res |= pckbport_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot,
	    cmd, 2, 0, 1, NULL);
	syn_sc->last_x = syn_sc->last_y = -1;
	if (res)
		printf("synaptics_enable: Error enabling device.\n");
}

void
pms_synaptics_resume(void *vsc)
{
	struct pms_softc *sc = vsc;
	unsigned char	cmd[1],resp[2] = { 0,0 };
	int	res;

	cmd[0] = PMS_RESET;
	res = pckbport_poll_cmd(sc->sc_kbctag, sc->sc_kbcslot, cmd,
	    1, 2, resp, 1);
	printf("%s: reset on resume %d 0x%02x 0x%02x\n",
	    sc->sc_dev.dv_xname,res,resp[0],resp[1]);
}

static
void pms_sysctl_synaptics(struct sysctllog **clog)
{
	int rc, root_num;
	struct sysctlnode *node;

	if ((rc = sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "hw", NULL,
	    NULL, 0, NULL, 0, CTL_HW, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "synaptics",
	    SYSCTL_DESCR("Synaptics touchpad controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL)) != 0)
	    goto err;

	root_num = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "emulate_mid",
	    SYSCTL_DESCR("Emulate middle button with wheel"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_emulate_mid,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_emulate_mid_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "slow_limit",
	    SYSCTL_DESCR("Slow limit"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_slow_limit,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
	    goto err;

	synaptics_slow_limit_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "tap_enable",
	    SYSCTL_DESCR("Process taps as clicks"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_tap_enable,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_tap_enable_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "tap_length",
	    SYSCTL_DESCR("Tap length"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_tap_length,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_tap_length_nodenum = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "tap_tolerance",
	    SYSCTL_DESCR("Tap tolerance"),
	    pms_sysctl_synaptics_verify, 0,
	    &synaptics_tap_tolerance,
	    0, CTL_HW, root_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	synaptics_tap_tolerance_nodenum = node->sysctl_num;
	return;

err:
	printf("%s: sysctl_createv failed (rc = %d)\n", __func__, rc);
}

static int
pms_sysctl_synaptics_verify(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	/* Sanity check the params. */
	if ((node.sysctl_num == synaptics_slow_limit_nodenum) ||
	    (node.sysctl_num == synaptics_tap_length_nodenum) ||
	    (node.sysctl_num == synaptics_tap_tolerance_nodenum)) {
		if (t < 0)
			return EINVAL;
	} else if ((node.sysctl_num == synaptics_emulate_mid_nodenum) ||
	    (node.sysctl_num == synaptics_tap_enable_nodenum)) {
		if (t != 0 && t != 1)
			return EINVAL;
	} else
		return EINVAL;

	*(int*)rnode->sysctl_data = t;

	return 0;
}

static int
pms_synaptics_send_command(pckbport_tag_t tag, pckbport_slot_t slot,
    u_char syn_cmd)
{
	u_char cmd[2];
	int res;

	/* Need to send 4 Set Resolution commands, with the argument
	 * encoded in the bottom most 2 bits.
	 */
	cmd[0] = PMS_SET_RES; cmd[1] = syn_cmd >> 6;
	res = pckbport_poll_cmd(tag, slot, cmd, 2, 0, NULL, 0);
	cmd[0] = PMS_SET_RES; cmd[1] = (syn_cmd & 0x30) >> 4;
	res |= pckbport_poll_cmd(tag, slot, cmd, 2, 0, NULL, 0);
	cmd[0] = PMS_SET_RES; cmd[1] = (syn_cmd & 0x0c) >> 2;
	res |= pckbport_poll_cmd(tag, slot, cmd, 2, 0, NULL, 0);
	cmd[0] = PMS_SET_RES; cmd[1] = (syn_cmd & 0x03);
	res |= pckbport_poll_cmd(tag, slot, cmd, 2, 0, NULL, 0);

	return res;
}

static void
pms_synaptics_spawn_thread(void *va)
{
	struct pms_softc	*sc = (struct pms_softc *)va;

	kthread_create1(pms_synaptics_thread, va, &sc->u.synaptics.syn_thread,
	    "touchpad");
}

static void
pms_synaptics_input(void *vsc, int data)
{
	struct pms_softc *sc = vsc;
	struct synaptics_softc	*syn_sc = &sc->u.synaptics;
	int	s;

	if (!sc->sc_enabled) {
		/* Interrupts are not expected.	 Discard the byte. */
		return;
	}

	s = splclock();
	sc->current = mono_time;
	splx(s);

	if (sc->inputstate > 0) {
		struct timeval diff;

		timersub(&sc->current, &sc->last, &diff);
		if (diff.tv_sec > 0 || diff.tv_usec >= 40000) {
			printf("pms_input: unusual delay (%ld.%06ld s), "
			    "scheduling reset\n",
			    (long)diff.tv_sec, (long)diff.tv_usec);
			sc->inputstate = 0;
			sc->sc_enabled = 0;
			wakeup(&sc->sc_enabled);
			return;
		}
	}
	sc->last = sc->current;

	switch (sc->inputstate) {
		case 0:
		if ((data & 0xc8) != 0x80) {
#ifdef SYNAPTICSDEBUG
			printf("pms_input: 0x%02x out of sync\n",data);
#endif
			return;	/* not in sync yet, discard input */
		}
		/* fallthrough */
		case 3:
		if ((data & 8) == 8) { 
#ifdef SYNAPTICS_DEBUG
			printf("pms_input: dropped in relative mode, reset\n");
#endif
			sc->inputstate = 0;
			sc->sc_enabled = 0;
			wakeup(&sc->sc_enabled);
			return;
		}
	}

	if (sc->inputstate == 6) { /* missed it last time, buffer was full */
		if (!syn_sc->buf_full) {
			memcpy(syn_sc->buf, sc->packet, sizeof(syn_sc->buf));
			syn_sc->buf_full = 1;
			sc->inputstate = 0;
			memset(sc->packet, 0, 6);
			wakeup(&syn_sc->buf_full);
		}
		else { /* bad luck, drop the packet */
			sc->inputstate = 0;
		}
	}

	sc->packet[sc->inputstate++] = data & 0xff;
	if (sc->inputstate == 6) {
		if (!syn_sc->buf_full) {
			memcpy(syn_sc->buf, sc->packet, sizeof(syn_sc->buf));
			syn_sc->buf_full = 1;
			sc->inputstate = 0;
			memset(sc->packet, 0, 6);
			wakeup(&syn_sc->buf_full);
		}
	}
}

/* Masks for the first byte of a packet */
#define PMS_LBUTMASK 0x01
#define PMS_RBUTMASK 0x02
#define PMS_MBUTMASK 0x04

static void
pms_synaptics_thread(void *va)
{
	struct pms_softc	*sc = (struct pms_softc *) va;
	struct synaptics_softc	*syn_sc = &sc->u.synaptics;
	u_int changed;
	int dx, dy, dz = 0;
	int left, mid, right, up, down;
	int newbuttons = 0;
	int s;
	int tap = 0, tapdrag = 0;
	struct timeval tap_start = { -1, 0 };
	int tap_x = -1, tap_y = -1, moving = 0;
	
	for (;;) {
		int ret;
		tap = 0;
		changed = 0;

		if (tapdrag == 1) {
			struct timeval	tap_len;
			do {
				ret = tsleep(&syn_sc->buf_full, PWAIT, "tap",
				    synaptics_tap_length * hz / 1000000);
				/* XXX: should measure the time */
			} while (ret != EWOULDBLOCK && !syn_sc->buf_full);

			timersub(&sc->current, &tap_start, &tap_len);
			if (tap_len.tv_usec >= synaptics_tap_length) {
				changed = 1;
				tapdrag = 0;
			}
		} else {
			while (!syn_sc->buf_full)
				tsleep(&syn_sc->buf_full, PWAIT, "wait", 0);
		}


		if ((syn_sc->has_passthrough) &&
			((syn_sc->buf[0] & 0xfc) == 0x84) && 
			((syn_sc->buf[3] & 0xcc) == 0xc4))
		{ 
			/* pass through port */
			if (!syn_sc->enable_passthrough)
				goto done;

			/* FIXME: should do something here instead if we don't
			 * want just to drop the packets */

			goto done;
		} else { /* touchpad absolute W mode */
			int x = syn_sc->buf[4] + ((syn_sc->buf[1] & 0x0f)<<8) +
				((syn_sc->buf[3] & 0x10)<<8);
			int y = syn_sc->buf[5] + ((syn_sc->buf[1] & 0xf0)<<4) +
				((syn_sc->buf[3] & 0x20)<<7);
			/* int w = ((syn_sc->buf[3] & 4)>>2) +
				((syn_sc->buf[0] & 4)>>1) +
				((syn_sc->buf[0] & 0x30)>>2); */
			int z = syn_sc->buf[2];

#ifdef notdef
			if (sc->palm_detect && w > 10) {
				z = 0;
				printf("palm detect %d\n",w);
			}
#endif

			/* Basic button handling. */
			left = (syn_sc->buf[0] & PMS_LBUTMASK);
			right = (syn_sc->buf[0] & PMS_RBUTMASK);

			/* Middle button. */
			if (syn_sc->has_m_button) {
				/* Old style Middle Button. */
				mid = ((syn_sc->buf[0] & PMS_LBUTMASK) ^
					(syn_sc->buf[3] & PMS_LBUTMASK));
			} else {
				mid = 0;
			}

			/* Up/Down button. */
			if (syn_sc->has_buttons_4_5) {
				/* Old up/down button. */
				up = left ^ (syn_sc->buf[3] & PMS_LBUTMASK);
				down = right ^ (syn_sc->buf[3] & PMS_RBUTMASK);
			} else if ((syn_sc->has_buttons_4_5_ext) &&
					((syn_sc->buf[0] & PMS_RBUTMASK) ^
					(syn_sc->buf[3] & PMS_RBUTMASK))) {
				/* New up/down button. */
				up = (syn_sc->buf[4] & SYN_1BUTMASK);
				down = (syn_sc->buf[5] & SYN_2BUTMASK);
			} else {
				up = 0;
				down = 0;
			}

			/* Emulate middle button. */
			if (synaptics_emulate_mid) {
				mid = ((up | down) ? 0x2 : 0);
				up = down = 0;
			}

			newbuttons = ((left) ? 0x1 : 0) |
				((mid) ? 0x2 : 0)  |
				((right) ? 0x4 : 0) |
				((up) ? 0x8 : 0) |
				((down) ? 0x10 : 0);

			changed |= (sc->buttons ^ newbuttons);
			sc->buttons = newbuttons;

			if (moving) {
				struct timeval	tap_len;

				if (z < SYNAPTICS_NOTOUCH) {
					moving = 0;

					timersub(&sc->current, &tap_start,
					    &tap_len);
					dx = x-tap_x; dx *= dx;
					dy = y-tap_y; dy *= dy;

					if ((synaptics_tap_enable) &&
						(tap_len.tv_sec == 0) &&
						(tap_len.tv_usec <
						synaptics_tap_length) &&
						(dx+dy <
						synaptics_tap_tolerance *
						synaptics_tap_tolerance)) {
						tap = 1;
					}
					tap_start.tv_sec = -1;
					tap_x = tap_y = -1;

					dx = dy = 0;
					if (tapdrag == 2) {
						tapdrag = 0;
						changed |= 1;
					}
				}	
				else {
					dx = x - syn_sc->last_x;
					dy = y - syn_sc->last_y;
					syn_sc->last_x = x;
					syn_sc->last_y = y;
				}
			}
			else {
				if (z >= SYNAPTICS_NOTOUCH) {
					moving = 1;
					if (tapdrag == 1) tapdrag = 2;
					else {
						tap_x = syn_sc->last_x = x;
						tap_y = syn_sc->last_y = y;
						tap_start = sc->current;
					}
				}
				dx = dy = 0;
			}
			dz = 0;

		}
		s = dx*dx + dy*dy;
		if (s < synaptics_slow_limit) {
			s = s/4 + synaptics_slow_limit*3/4;
			dx = dx * s/synaptics_slow_limit;
			dy = dy * s/synaptics_slow_limit;
		}

		syn_sc->last_x -= dx % 8;
		syn_sc->last_y -= dy % 8;
		dx /= 8;
		dy /= 8;

		if (sc->buttons & 1) tap = 0;

		if (dx || dy || dz || changed || tap) {
			if (tap) {
				wsmouse_input(sc->sc_wsmousedev,
				    sc->buttons | 1, 0,0,0,
				    WSMOUSE_INPUT_DELTA);

				tapdrag = 1;
				tap_start = sc->current;
			}
			else wsmouse_input(sc->sc_wsmousedev,
			    sc->buttons | (tapdrag == 2 ? 1 : 0),
			    dx, dy, dz,
			    WSMOUSE_INPUT_DELTA);
		}

done:
		syn_sc->buf_full = 0;
		memset(syn_sc->buf, 0, 6);
	}
}
