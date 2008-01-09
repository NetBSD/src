/*	$NetBSD: cuda.c,v 1.3.26.3 2008/01/09 01:47:08 matt Exp $ */

/*-
 * Copyright (c) 2006 Michael Lorenz
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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: cuda.c,v 1.3.26.3 2008/01/09 01:47:08 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/pio.h>
#include <dev/clock_subr.h>
#include <dev/i2c/i2cvar.h>

#include <macppc/dev/viareg.h>
#include <macppc/dev/cudavar.h>

#include <dev/ofw/openfirm.h>
#include <dev/adb/adbvar.h>
#include "opt_cuda.h"

#ifdef CUDA_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

#define CUDA_NOTREADY	0x1	/* has not been initialized yet */
#define CUDA_IDLE	0x2	/* the bus is currently idle */
#define CUDA_OUT	0x3	/* sending out a command */
#define CUDA_IN		0x4	/* receiving data */
#define CUDA_POLLING	0x5	/* polling - II only */

static void cuda_attach(struct device *, struct device *, void *);
static int cuda_match(struct device *, struct cfdata *, void *);
static void cuda_autopoll(void *, int);

static int cuda_intr(void *);

typedef struct _cuda_handler {
	int (*handler)(void *, int, uint8_t *);
	void *cookie;
} CudaHandler;

struct cuda_softc {
	struct device sc_dev;
	void *sc_ih;
	CudaHandler sc_handlers[16];
	struct todr_chip_handle sc_todr;
	struct adb_bus_accessops sc_adbops;
	struct i2c_controller sc_i2c;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
	int sc_node;
	int sc_state;
	int sc_waiting;
	int sc_polling;
	int sc_sent;
	int sc_out_length;
	int sc_received;
	int sc_iic_done;
	int sc_error;
	/* time */
	uint32_t sc_tod;
	uint32_t sc_autopoll;
	uint32_t sc_todev;
	/* ADB */
	void (*sc_adb_handler)(void *, int, uint8_t *);
	void *sc_adb_cookie;
	uint32_t sc_i2c_read_len;
	/* internal buffers */
	uint8_t sc_in[256];
	uint8_t sc_out[256];
};

CFATTACH_DECL(cuda, sizeof(struct cuda_softc),
    cuda_match, cuda_attach, NULL, NULL);

static inline void cuda_write_reg(struct cuda_softc *, int, uint8_t);
static inline uint8_t cuda_read_reg(struct cuda_softc *, int);
static void cuda_idle(struct cuda_softc *);
static void cuda_tip(struct cuda_softc *);
static void cuda_clear_tip(struct cuda_softc *);
static void cuda_in(struct cuda_softc *);
static void cuda_out(struct cuda_softc *);
static void cuda_toggle_ack(struct cuda_softc *);
static void cuda_ack_off(struct cuda_softc *);
static int cuda_intr_state(struct cuda_softc *);

static void cuda_init(struct cuda_softc *);

/*
 * send a message to Cuda.
 */
/* cookie, flags, length, data */
static int cuda_send(void *, int, int, uint8_t *);
static void cuda_poll(void *);
static void cuda_adb_poll(void *);
static int cuda_set_handler(void *, int, int (*)(void *, int, uint8_t *), void *);

static int cuda_error_handler(void *, int, uint8_t *);

static int cuda_todr_handler(void *, int, uint8_t *);
static int cuda_todr_set(todr_chip_handle_t, volatile struct timeval *);
static int cuda_todr_get(todr_chip_handle_t, volatile struct timeval *);

static int cuda_adb_handler(void *, int, uint8_t *);
static void cuda_final(struct device *);

static struct cuda_attach_args *cuda0 = NULL;

/* ADB bus attachment stuff */
static 	int cuda_adb_send(void *, int, int, int, uint8_t *);
static	int cuda_adb_set_handler(void *, void (*)(void *, int, uint8_t *), void *);

/* i2c stuff */
static int cuda_i2c_acquire_bus(void *, int);
static void cuda_i2c_release_bus(void *, int);
static int cuda_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *, size_t,
		    void *, size_t, int);

static int
cuda_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct confargs *ca = aux;

	if (ca->ca_nreg < 8)
		return 0;

	if (ca->ca_nintr < 4)
		return 0;

	if (strcmp(ca->ca_name, "via-cuda") == 0) {
		return 10;	/* beat adb* at obio? */
	}
	
	return 0;
}

static void
cuda_attach(struct device *parent, struct device *dev, void *aux)
{
	struct confargs *ca = aux;
	struct cuda_softc *sc = (struct cuda_softc *)dev;
	struct i2cbus_attach_args iba;
	static struct cuda_attach_args caa;
	int irq = ca->ca_intr[0];
	int node, i, child;
	char name[32];

	node = of_getnode_byname(OF_parent(ca->ca_node), "extint-gpio1");
	if (node)
		OF_getprop(node, "interrupts", &irq, 4);

	printf(" irq %d: ", irq);

	sc->sc_node = ca->ca_node;
	sc->sc_memt = ca->ca_tag;

	sc->sc_sent = 0;
	sc->sc_received = 0;
	sc->sc_waiting = 0;
	sc->sc_polling = 0;
	sc->sc_state = CUDA_NOTREADY;
	sc->sc_error = 0;
	sc->sc_i2c_read_len = 0;

	if (bus_space_map(sc->sc_memt, ca->ca_reg[0] + ca->ca_baseaddr,
	    ca->ca_reg[1], 0, &sc->sc_memh) != 0) {

		printf("%s: unable to map registers\n", dev->dv_xname);
		return;
	}
	sc->sc_ih = intr_establish(irq, IST_EDGE, IPL_TTY, cuda_intr, sc);
	printf("\n");

	for (i = 0; i < 16; i++) {
		sc->sc_handlers[i].handler = NULL;
		sc->sc_handlers[i].cookie = NULL;
	}

	cuda_init(sc);

	/* now attach children */
	config_interrupts(dev, cuda_final);
	cuda_set_handler(sc, CUDA_ERROR, cuda_error_handler, sc);
	cuda_set_handler(sc, CUDA_PSEUDO, cuda_todr_handler, sc);

	child = OF_child(ca->ca_node);
	while (child != 0) {

		if (OF_getprop(child, "name", name, 32) == 0)
			continue;
		if (strncmp(name, "adb", 4) == 0) {

			cuda_set_handler(sc, CUDA_ADB, cuda_adb_handler, sc);
			sc->sc_adbops.cookie = sc;
			sc->sc_adbops.send = cuda_adb_send;
			sc->sc_adbops.poll = cuda_adb_poll;
			sc->sc_adbops.autopoll = cuda_autopoll;
			sc->sc_adbops.set_handler = cuda_adb_set_handler;
			config_found_ia(dev, "adb_bus", &sc->sc_adbops,
			    nadb_print);
		} else if (strncmp(name, "rtc", 4) == 0) {

			sc->sc_todr.todr_gettime = cuda_todr_get;
			sc->sc_todr.todr_settime = cuda_todr_set;
			sc->sc_todr.cookie = sc;
			todr_attach(&sc->sc_todr);
		} 
		child = OF_peer(child);
	}

	caa.cookie = sc;
	caa.set_handler = cuda_set_handler;
	caa.send = cuda_send;
	caa.poll = cuda_poll;
#if notyet
	config_found(dev, &caa, cuda_print);
#endif

	iba.iba_tag = &sc->sc_i2c;
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = cuda_i2c_acquire_bus;
	sc->sc_i2c.ic_release_bus = cuda_i2c_release_bus;
	sc->sc_i2c.ic_send_start = NULL;
	sc->sc_i2c.ic_send_stop = NULL;
	sc->sc_i2c.ic_initiate_xfer = NULL;
	sc->sc_i2c.ic_read_byte = NULL;
	sc->sc_i2c.ic_write_byte = NULL;
	sc->sc_i2c.ic_exec = cuda_i2c_exec;
	config_found_ia(&sc->sc_dev, "i2cbus", &iba, iicbus_print);

	if (cuda0 == NULL)
		cuda0 = &caa;
}

static void
cuda_init(struct cuda_softc *sc)
{
	volatile int i;
	uint8_t reg;

	reg = cuda_read_reg(sc, vDirB);
	reg |= 0x30;	/* register B bits 4 and 5: outputs */
	cuda_write_reg(sc, vDirB, reg);

	reg = cuda_read_reg(sc, vDirB);
	reg &= 0xf7;	/* register B bit 3: input */
	cuda_write_reg(sc, vDirB, reg);
	
	reg = cuda_read_reg(sc, vACR);
	reg &= ~vSR_OUT;	/* make sure SR is set to IN */
	cuda_write_reg(sc, vACR, reg);

	cuda_write_reg(sc, vACR, (cuda_read_reg(sc, vACR) | 0x0c) & ~0x10);

	sc->sc_state = CUDA_IDLE;	/* used by all types of hardware */

	cuda_write_reg(sc, vIER, 0x84); /* make sure VIA interrupts are on */
	cuda_idle(sc);	/* set ADB bus state to idle */

	/* sort of a device reset */
	i = cuda_read_reg(sc, vSR);	/* clear interrupt */
	cuda_write_reg(sc, vIER, 0x04); /* no interrupts while clearing */
	cuda_idle(sc);	/* reset state to idle */
	delay(150);
	cuda_tip(sc);	/* signal start of frame */
	delay(150);
	cuda_toggle_ack(sc);
	delay(150);
	cuda_clear_tip(sc);
	delay(150);
	cuda_idle(sc);	/* back to idle state */
	i = cuda_read_reg(sc, vSR);	/* clear interrupt */
	cuda_write_reg(sc, vIER, 0x84);	/* ints ok now */
}

static void
cuda_final(struct device *dev)
{
	struct cuda_softc *sc = (struct cuda_softc *)dev;

	sc->sc_polling = 0;
#if 0
	{
		int err;
		uint8_t buffer[2], buf2[2];

		/* trying to read */
		printf("reading\n");
		buffer[0] = 0;
		buffer[1] = 1;
		buf2[0] = 0;
		err = cuda_i2c_exec(sc, I2C_OP_WRITE, 0x8a, buffer, 2, buf2, 0, 0);
		buf2[0] = 0;
		err = cuda_i2c_exec(sc, I2C_OP_WRITE | I2C_OP_READ, 0x8a, buffer, 1, buf2, 2, 0);
		printf("buf2: %02x\n", buf2[0]);
	}
#endif
}

static inline void
cuda_write_reg(struct cuda_softc *sc, int offset, uint8_t value)
{

	bus_space_write_1(sc->sc_memt, sc->sc_memh, offset, value);
}

static inline uint8_t
cuda_read_reg(struct cuda_softc *sc, int offset)
{

	return bus_space_read_1(sc->sc_memt, sc->sc_memh, offset);
}

static int
cuda_set_handler(void *cookie, int type, 
    int (*handler)(void *, int, uint8_t *), void *hcookie)
{
	struct cuda_softc *sc = cookie;
	CudaHandler *me;

	if ((type >= 0) && (type < 16)) {
		me = &sc->sc_handlers[type];
		me->handler = handler;
		me->cookie = hcookie;
		return 0;
	}
	return -1;
}

static int
cuda_send(void *cookie, int poll, int length, uint8_t *msg)
{
	struct cuda_softc *sc = cookie;
	int s;

	DPRINTF("cuda_send %08x\n", (uint32_t)cookie);
	if (sc->sc_state == CUDA_NOTREADY)
		return -1;

	s = splhigh();

	if ((sc->sc_state == CUDA_IDLE) /*&& 
	    ((cuda_read_reg(sc, vBufB) & vPB3) == vPB3)*/) {
		/* fine */
		DPRINTF("chip is idle\n");
	} else {
		DPRINTF("cuda state is %d\n", sc->sc_state);
		if (sc->sc_waiting == 0) {
			sc->sc_waiting = 1;
		} else {
			splx(s);
			return -1;
		}
	}

	sc->sc_error = 0;
	memcpy(sc->sc_out, msg, length);
	sc->sc_out_length = length;
	sc->sc_sent = 0;

	if (sc->sc_waiting != 1) {

		delay(150);
		sc->sc_state = CUDA_OUT;
		cuda_out(sc);
		cuda_write_reg(sc, vSR, sc->sc_out[0]);
		cuda_ack_off(sc);
		cuda_tip(sc);
	}
	sc->sc_waiting = 1;

	if (sc->sc_polling || poll || cold) {
		cuda_poll(sc);
	}

	splx(s);

	return 0;
}

static void
cuda_poll(void *cookie)
{
	struct cuda_softc *sc = cookie;
	int s;

	DPRINTF("polling\n");
	while ((sc->sc_state != CUDA_IDLE) ||
	       (cuda_intr_state(sc)) ||
	       (sc->sc_waiting == 1)) {
		if ((cuda_read_reg(sc, vIFR) & vSR_INT) == vSR_INT) {
			s = splhigh();
			cuda_intr(sc);
			splx(s);
		}
	}
}

static void
cuda_adb_poll(void *cookie)
{
	struct cuda_softc *sc = cookie;
	int s;

	s = splhigh();
	cuda_intr(sc);
	splx(s);
}

static void
cuda_idle(struct cuda_softc *sc)
{
	uint8_t reg;

	reg = cuda_read_reg(sc, vBufB);
	reg |= (vPB4 | vPB5);
	cuda_write_reg(sc, vBufB, reg);
}

static void
cuda_tip(struct cuda_softc *sc)
{
	uint8_t reg;

	reg = cuda_read_reg(sc, vBufB);
	reg &= ~vPB5;
	cuda_write_reg(sc, vBufB, reg);
}

static void
cuda_clear_tip(struct cuda_softc *sc)
{
	uint8_t reg;

	reg = cuda_read_reg(sc, vBufB);
	reg |= vPB5;
	cuda_write_reg(sc, vBufB, reg);
}

static void
cuda_in(struct cuda_softc *sc)
{
	uint8_t reg;

	reg = cuda_read_reg(sc, vACR);
	reg &= ~vSR_OUT;
	cuda_write_reg(sc, vACR, reg);
}

static void
cuda_out(struct cuda_softc *sc)
{
	uint8_t reg;

	reg = cuda_read_reg(sc, vACR);
	reg |= vSR_OUT;
	cuda_write_reg(sc, vACR, reg);
}

static void
cuda_toggle_ack(struct cuda_softc *sc)
{
	uint8_t reg;

	reg = cuda_read_reg(sc, vBufB);
	reg ^= vPB4;
	cuda_write_reg(sc, vBufB, reg);
}

static void
cuda_ack_off(struct cuda_softc *sc)
{
	uint8_t reg;

	reg = cuda_read_reg(sc, vBufB);
	reg |= vPB4;
	cuda_write_reg(sc, vBufB, reg);
}

static int
cuda_intr_state(struct cuda_softc *sc)
{
	return ((cuda_read_reg(sc, vBufB) & vPB3) == 0);
}

static int
cuda_intr(void *arg)
{
	struct cuda_softc *sc = arg;
	int i, ending, type;
	uint8_t reg;

	reg = cuda_read_reg(sc, vIFR);		/* Read the interrupts */
	DPRINTF("[");
	if ((reg & 0x80) == 0) {
		DPRINTF("irq %02x]", reg);
		return 0;			/* No interrupts to process */
	}
	DPRINTF(":");

	cuda_write_reg(sc, vIFR, 0x7f);	/* Clear 'em */

switch_start:
	switch (sc->sc_state) {
	case CUDA_IDLE:
		/*
		 * This is an unexpected packet, so grab the first (dummy)
		 * byte, set up the proper vars, and tell the chip we are
		 * starting to receive the packet by setting the TIP bit.
		 */
		sc->sc_in[1] = cuda_read_reg(sc, vSR);
		DPRINTF("start: %02x", sc->sc_in[1]);
		if (cuda_intr_state(sc) == 0) {
			/* must have been a fake start */
			DPRINTF(" ... fake start\n");
			if (sc->sc_waiting) {
				/* start over */
				delay(150);
				sc->sc_state = CUDA_OUT;
				sc->sc_sent = 0;
				cuda_out(sc);
				cuda_write_reg(sc, vSR, sc->sc_out[1]);
				cuda_ack_off(sc);
				cuda_tip(sc);
			}
			break;
		}

		cuda_in(sc);
		cuda_tip(sc);

		sc->sc_received = 1;
		sc->sc_state = CUDA_IN;
		DPRINTF(" CUDA_IN");
		break;

	case CUDA_IN:
		sc->sc_in[sc->sc_received] = cuda_read_reg(sc, vSR);
		DPRINTF(" %02x", sc->sc_in[sc->sc_received]);
		ending = 0;
		if (sc->sc_received > 255) {
			/* bitch only once */
			if (sc->sc_received == 256) {
				printf("%s: input overflow\n",
				    sc->sc_dev.dv_xname);
				ending = 1;
			}
		} else
			sc->sc_received++;
		if (sc->sc_received > 3) {
			if ((sc->sc_in[3] == CMD_IIC) && 
			    (sc->sc_received > (sc->sc_i2c_read_len + 4))) {
				ending = 1;
			}
		}

		/* intr off means this is the last byte (end of frame) */
		if (cuda_intr_state(sc) == 0) {
			ending = 1;
			DPRINTF(".\n");
		} else {
			cuda_toggle_ack(sc);			
		}
		
		if (ending == 1) {	/* end of message? */

			sc->sc_in[0] = sc->sc_received - 1;

			/* reset vars and signal the end of this frame */
			cuda_idle(sc);

			/* check if we have a handler for this message */
			type = sc->sc_in[1];
			if ((type >= 0) && (type < 16)) {
				CudaHandler *me = &sc->sc_handlers[type];

				if (me->handler != NULL) {
					me->handler(me->cookie,
					    sc->sc_received - 1, &sc->sc_in[1]);
				} else {
					printf("no handler for type %02x\n", type);
					panic("barf");
				}
			}

			DPRINTF("CUDA_IDLE");
			sc->sc_state = CUDA_IDLE;
			
			sc->sc_received = 0;

			/*
			 * If there is something waiting to be sent out,
			 * set everything up and send the first byte.
			 */
			if (sc->sc_waiting == 1) {

				DPRINTF("pending write\n");
				delay(1500);	/* required */
				sc->sc_sent = 0;
				sc->sc_state = CUDA_OUT;

				/*
				 * If the interrupt is on, we were too slow
				 * and the chip has already started to send
				 * something to us, so back out of the write
				 * and start a read cycle.
				 */
				if (cuda_intr_state(sc)) {
					cuda_in(sc);
					cuda_idle(sc);
					sc->sc_sent = 0;
					sc->sc_state = CUDA_IDLE;
					sc->sc_received = 0;
					delay(150);
					DPRINTF("too slow - incoming message\n");
					goto switch_start;
				}
				/*
				 * If we got here, it's ok to start sending
				 * so load the first byte and tell the chip
				 * we want to send.
				 */
				DPRINTF("sending ");

				cuda_out(sc);
				cuda_write_reg(sc, vSR,
				    sc->sc_out[sc->sc_sent]);
				cuda_ack_off(sc);
				cuda_tip(sc);
			}
		}
		break;

	case CUDA_OUT:
		i = cuda_read_reg(sc, vSR);	/* reset SR-intr in IFR */

		sc->sc_sent++;
		if (cuda_intr_state(sc)) {	/* ADB intr low during write */

			DPRINTF("incoming msg during send\n");
			cuda_in(sc);	/* make sure SR is set to IN */
			cuda_idle(sc);
			sc->sc_sent = 0;	/* must start all over */
			sc->sc_state = CUDA_IDLE;	/* new state */
			sc->sc_received = 0;
			sc->sc_waiting = 1;	/* must retry when done with
						 * read */
			delay(150);
			goto switch_start;	/* process next state right
						 * now */
			break;
		}
		if (sc->sc_out_length == sc->sc_sent) {	/* check for done */

			sc->sc_waiting = 0;	/* done writing */
			sc->sc_state = CUDA_IDLE;	/* signal bus is idle */
			cuda_in(sc);
			cuda_idle(sc);
			DPRINTF("done sending\n");
		} else {
			/* send next byte */
			cuda_write_reg(sc, vSR, sc->sc_out[sc->sc_sent]);
			cuda_toggle_ack(sc);	/* signal byte ready to
							 * shift */
		}
		break;

	case CUDA_NOTREADY:
		DPRINTF("adb: not yet initialized\n");
		break;

	default:
		DPRINTF("intr: unknown ADB state\n");
		break;
	}

	DPRINTF("]");
	return 1;
}

static int
cuda_error_handler(void *cookie, int len, uint8_t *data)
{
	struct cuda_softc *sc = cookie;

	/* 
	 * something went wrong
	 * byte 3 seems to be the failed command
	 */
	sc->sc_error = 1;
	wakeup(&sc->sc_todev);
	return 0;
}


/* real time clock */

static int
cuda_todr_handler(void *cookie, int len, uint8_t *data)
{
	struct cuda_softc *sc = cookie;

#ifdef CUDA_DEBUG
	int i;
	printf("msg: %02x", data[0]);
	for (i = 1; i < len; i++) {
		printf(" %02x", data[i]);
	}
	printf("\n");
#endif

	switch(data[2]) {
		case CMD_READ_RTC:
			memcpy(&sc->sc_tod, &data[3], 4);
			break;
		case CMD_WRITE_RTC:
			sc->sc_tod = 0xffffffff;
			break;
		case CMD_AUTOPOLL:
			sc->sc_autopoll = 1;
			break;
		case CMD_IIC:
			sc->sc_iic_done = len;
			break;
	}
	wakeup(&sc->sc_todev);
	return 0;
}

#define DIFF19041970 2082844800

static int
cuda_todr_get(todr_chip_handle_t tch, volatile struct timeval *tvp)
{
	struct cuda_softc *sc = tch->cookie;
	int cnt = 0;
	uint8_t cmd[] = { CUDA_PSEUDO, CMD_READ_RTC};

	sc->sc_tod = 0;
	cuda_send(sc, 0, 2, cmd);

	while ((sc->sc_tod == 0) && (cnt < 10)) {
		tsleep(&sc->sc_todev, 0, "todr", 10);
		cnt++;
	}

	if (sc->sc_tod == 0)
		return EIO;

	tvp->tv_sec = sc->sc_tod - DIFF19041970;
	DPRINTF("tod: %ld\n", tvp->tv_sec);
	tvp->tv_usec = 0;
	return 0;
}

static int
cuda_todr_set(todr_chip_handle_t tch, volatile struct timeval *tvp)
{
	struct cuda_softc *sc = tch->cookie;
	uint32_t sec;
	uint8_t cmd[] = {CUDA_PSEUDO, CMD_WRITE_RTC, 0, 0, 0, 0};

	sec = tvp->tv_sec + DIFF19041970;
	memcpy(&cmd[2], &sec, 4);
	sc->sc_tod = 0;
	if (cuda_send(sc, 0, 6, cmd) == 0) {
		while (sc->sc_tod == 0) {
			tsleep(&sc->sc_todev, 0, "todr", 10);
		}
		return 0;
	}
	return -1;
		
}

/* poweroff and reboot */

void
cuda_poweroff()
{
	struct cuda_softc *sc;
	uint8_t cmd[] = {CUDA_PSEUDO, CMD_POWEROFF};

	if (cuda0 == NULL)
		return;
	sc = cuda0->cookie;
	sc->sc_polling = 1;
	cuda0->poll(sc);
	if (cuda0->send(sc, 1, 2, cmd) == 0)
		while (1);
}

void
cuda_restart()
{
	struct cuda_softc *sc;
	uint8_t cmd[] = {CUDA_PSEUDO, CMD_RESET};

	if (cuda0 == NULL)
		return;
	sc = cuda0->cookie;
	sc->sc_polling = 1;
	cuda0->poll(sc);
	if (cuda0->send(sc, 1, 2, cmd) == 0)
		while (1);
}

/* ADB message handling */

static void
cuda_autopoll(void *cookie, int flag)
{
	struct cuda_softc *sc = cookie;
	uint8_t cmd[] = {CUDA_PSEUDO, CMD_AUTOPOLL, (flag != 0)};

	if (cmd[2] == sc->sc_autopoll)
		return;

	sc->sc_autopoll = -1;
	cuda_send(sc, 0, 3, cmd);
	while(sc->sc_autopoll == -1) {
		if (sc->sc_polling || cold) {
			cuda_poll(sc);
		} else
			tsleep(&sc->sc_todev, 0, "autopoll", 100);
	}
}
	
static int
cuda_adb_handler(void *cookie, int len, uint8_t *data)
{
	struct cuda_softc *sc = cookie;

	if (sc->sc_adb_handler != NULL) {
		sc->sc_adb_handler(sc->sc_adb_cookie, len - 1, 
		    &data[1]);
		return 0;
	}
	return -1;
}

static int
cuda_adb_send(void *cookie, int poll, int command, int len, uint8_t *data)
{
	struct cuda_softc *sc = cookie;
	int i, s = 0;
	uint8_t packet[16];

	/* construct an ADB command packet and send it */
	packet[0] = CUDA_ADB;
	packet[1] = command;
	for (i = 0; i < len; i++)
		packet[i + 2] = data[i];
	if (poll || cold) {
		s = splhigh();
		cuda_poll(sc);
	}
	cuda_send(sc, poll, len + 2, packet);
	if (poll || cold) {
		cuda_poll(sc);
		splx(s);
	}
	return 0;
}

static int
cuda_adb_set_handler(void *cookie, void (*handler)(void *, int, uint8_t *),
    void *hcookie)
{
	struct cuda_softc *sc = cookie;

	/* register a callback for incoming ADB messages */
	sc->sc_adb_handler = handler;
	sc->sc_adb_cookie = hcookie;
	return 0;
}

/* i2c message handling */

static int
cuda_i2c_acquire_bus(void *cookie, int flags)
{
	/* nothing yet */
	return 0;
}

static void
cuda_i2c_release_bus(void *cookie, int flags)
{
	/* nothing here either */
}

static int
cuda_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *_send,
    size_t send_len, void *_recv, size_t recv_len, int flags)
{
	struct cuda_softc *sc = cookie;
	const uint8_t *send = _send;
	uint8_t *recv = _recv;
	uint8_t command[16] = {CUDA_PSEUDO, CMD_IIC};

	DPRINTF("cuda_i2c_exec(%02x)\n", addr);
	command[2] = addr;

	memcpy(&command[3], send, min((int)send_len, 12));

	sc->sc_iic_done = 0;
	cuda_send(sc, sc->sc_polling, send_len + 3, command);

	while ((sc->sc_iic_done == 0) && (sc->sc_error == 0)) {
		if (sc->sc_polling || cold) {
			cuda_poll(sc);
		} else
			tsleep(&sc->sc_todev, 0, "i2c", 1000);
	}

	if (sc->sc_error) {
		sc->sc_error = 0;
		return -1;
	}

	/* see if we're supposed to do a read */
	if (recv_len > 0) {
		sc->sc_iic_done = 0;
		command[2] |= 1;
		command[3] = 0;

		/*
		 * XXX we need to do something to limit the size of the answer
		 * - apparently the chip keeps sending until we tell it to stop
		 */
		sc->sc_i2c_read_len = recv_len;
		DPRINTF("rcv_len: %d\n", recv_len);
		cuda_send(sc, sc->sc_polling, 3, command);
		while ((sc->sc_iic_done == 0) && (sc->sc_error == 0)) {
			if (sc->sc_polling || cold) {
				cuda_poll(sc);
			} else
				tsleep(&sc->sc_todev, 0, "i2c", 1000);
		}

		if (sc->sc_error) {
			printf("error trying to read\n");
			sc->sc_error = 0;
			return -1;
		}
	}

	DPRINTF("received: %d\n", sc->sc_iic_done);
	if ((sc->sc_iic_done > 3) && (recv_len > 0)) {
		int rlen;

		/* we got an answer */
		rlen = min(sc->sc_iic_done - 3, recv_len);
		memcpy(recv, &sc->sc_in[4], rlen);
#ifdef CUDA_DEBUG
		{
			int i;
			printf("ret:");
			for (i = 0; i < rlen; i++)
				printf(" %02x", recv[i]);
			printf("\n");
		}
#endif
		return rlen;
	}
	return 0;
}
