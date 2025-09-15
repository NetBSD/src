/*	$NetBSD: lightbar.c,v 1.2 2025/09/15 06:42:10 macallan Exp $	*/

/*
 * Copyright (c) 2025 Michael Lorenz
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lightbar.c,v 1.2 2025/09/15 06:42:10 macallan Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/cpu.h>
#include <sys/sysctl.h>
#include <uvm/uvm_extern.h>

#include <dev/ofw/openfirm.h>
#include <machine/autoconf.h>
#include <machine/pio.h>
#include <macppc/dev/dbdma.h>
#include <macppc/dev/obiovar.h>
#include <macppc/dev/i2sreg.h>

#ifdef LIGHTBAR_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

struct lightbar_softc {
	device_t sc_dev;
	int sc_node;
	bus_space_tag_t sc_tag;
	bus_space_handle_t sc_bsh;
	bus_space_handle_t sc_odmah;
	dbdma_regmap_t *sc_odma;
	struct dbdma_command *sc_odmacmd;
	uint32_t sc_baseaddr;
	uint32_t *sc_dmabuf;
	lwp_t *sc_thread;
	int sc_sys, sc_user;
	struct cpu_info *sc_cpu[2];
	struct sysctlnode *sc_sysctl_me;
};

static int lightbar_match(device_t, struct cfdata *, void *);
static void lightbar_attach(device_t, device_t, void *);
static void lightbar_thread(void *);

CFATTACH_DECL_NEW(lightbar, sizeof(struct lightbar_softc), lightbar_match,
	lightbar_attach, NULL, NULL);

/*
 * upper 16 bit are LEDs from the top right to the bottom left 
 * however, the hardware has them rotated so the uppper left bit is in 1
 */
#define LEDMASK(x) ((x << 1) | (((x) & 0x80000000) >> 31))

static int
lightbar_match(device_t parent, struct cfdata *match, void *aux)
{
	struct confargs *ca;
	int soundbus, soundchip;
	char buf[32];

	ca = aux;
	if (strcmp(ca->ca_name, "i2s") != 0)
		return 0;

	if ((soundbus = OF_child(ca->ca_node)) == 0 ||
	    (soundchip = OF_child(soundbus)) == 0)
		return 0;

	if (OF_getprop(soundchip, "virtual", buf, 32) == 0)
		return 200;	/* beat out snapper */

	return 0;
}

static void
lightbar_attach(device_t parent, device_t self, void *aux)
{
	struct lightbar_softc *sc;
	struct confargs *ca;
	struct dbdma_command *cmd;
	struct sysctlnode *node;
	uint32_t reg[6], x;
	int i, timo;

	sc = device_private(self);
	sc->sc_dev = self;

	ca = aux;
	sc->sc_node = ca->ca_node;
	sc->sc_tag = ca->ca_tag;

	sc->sc_odmacmd = dbdma_alloc(4 * sizeof(struct dbdma_command), NULL);

	sc->sc_baseaddr = ca->ca_baseaddr;

	/*
	 * default brightnesss for system and user time bar
	 * can be changed via sysctl later on
	 */
	sc->sc_sys = 2;
	sc->sc_user = 8;

	OF_getprop(sc->sc_node, "reg", reg, sizeof(reg));
	reg[0] += ca->ca_baseaddr;
	reg[2] += ca->ca_baseaddr;

	bus_space_map(sc->sc_tag, reg[0], reg[1], 0, &sc->sc_bsh);
	obio_space_map(reg[2], reg[3], &sc->sc_odmah);
	sc->sc_odma = bus_space_vaddr(sc->sc_tag, sc->sc_odmah);
	DPRINTF("reg %08x odma %08x\n", (uint32_t)sc->sc_bsh, (uint32_t)sc->sc_odmah);

	aprint_normal("\n");

	/* PMF event handler */
	pmf_device_register(sc->sc_dev, NULL, NULL);

	/* enable i2s goop */
	x = obio_read_4(KEYLARGO_FCR1);
	x |= I2S0CLKEN | I2S0EN;
	obio_write_4(KEYLARGO_FCR1, x);

	/* Clear CLKSTOPPEND. */
	bus_space_write_4(sc->sc_tag, sc->sc_bsh, I2S_INT, I2S_INT_CLKSTOPPEND);

	x = obio_read_4(KEYLARGO_FCR1);                /* FCR */
	x &= ~I2S0CLKEN;                /* XXX I2S0 */
	obio_write_4(KEYLARGO_FCR1, x);

	/* Wait until clock is stopped. */
	for (timo = 1000; timo > 0; timo--) {
		if (bus_space_read_4(sc->sc_tag, sc->sc_bsh, I2S_INT) & 
		    I2S_INT_CLKSTOPPEND)
			goto done;
		delay(1);
	}
	DPRINTF("timeout\n");
done:
	bus_space_write_4(sc->sc_tag, sc->sc_bsh, I2S_FORMAT, 0x01fa0000);

	x = obio_read_4(KEYLARGO_FCR1);
	x |= I2S0CLKEN;
	obio_write_4(KEYLARGO_FCR1, x);

	sc->sc_dmabuf = kmem_alloc(4096, KM_SLEEP);

	/* initial pattern, just to say hi */
	for (i = 0; i < 32; i++) {
		sc->sc_dmabuf[i] = LEDMASK(0xaa550000);
	}

	/*
	 * We use a single DMA buffer, with just 8 32bit words which the DBDMA
	 * engine loops over. That way we can:
	 * - get away without using interrupts, just scribble a new pattern into
	 *   the buffer
	 * - play PWM tricks with the LEDs, giving us 8 levels of brightness
	 */
	cmd = sc->sc_odmacmd;
	DBDMA_BUILD(cmd, DBDMA_CMD_OUT_MORE, 0, 32, vtophys((vaddr_t)sc->sc_dmabuf),
		    0, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
	cmd++;
	DBDMA_BUILD(cmd, DBDMA_CMD_NOP, 0, 0,
	    0/*vtophys((vaddr_t)sc->sc_odmacmd)*/, 0, DBDMA_WAIT_NEVER,
	    DBDMA_BRANCH_ALWAYS);

	out32rb(&cmd->d_cmddep, vtophys((vaddr_t)sc->sc_odmacmd));

	dbdma_start(sc->sc_odma, sc->sc_odmacmd);

	sc->sc_cpu[0] = cpu_lookup(0);
	sc->sc_cpu[1] = cpu_lookup(1);
	aprint_normal_dev(sc->sc_dev, "monitoring %s\n",
	    sc->sc_cpu[1] == NULL ? "one CPU" : "two CPUs");

	if (kthread_create(PRI_NONE, 0, NULL, lightbar_thread, sc,
	    &sc->sc_thread, "%s", "lightbar") != 0) {
		aprint_error_dev(self, "unable to create kthread\n");
	}

	/* setup sysctl nodes */
	sysctl_createv(NULL, 0, NULL, (void *) &sc->sc_sysctl_me,
	    CTLFLAG_READWRITE,
	    CTLTYPE_NODE, device_xname(sc->sc_dev), NULL,
	    NULL, 0, NULL, 0,
	    CTL_HW, CTL_CREATE, CTL_EOL);
	sysctl_createv(NULL, 0, NULL, (void *) &node,
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "sys", "system time",
	    NULL, 0, (void *)&sc->sc_sys, 0,
	    CTL_HW,
	    sc->sc_sysctl_me->sysctl_num,
	    CTL_CREATE, CTL_EOL);
	sysctl_createv(NULL, 0, NULL, (void *) &node,
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "user", "user time",
	    NULL, 0, (void *)&sc->sc_user, 0,
	    CTL_HW,
	    sc->sc_sysctl_me->sysctl_num,
	    CTL_CREATE, CTL_EOL);
}

/*
 * this draws a bar into out[0..7], system time as dim, other CPU time as
 * bright, idle as off
 * prev[CPUSTATES] stores old counter values, old[2] old bar lengths
 */
static int
lightbar_update(struct lightbar_softc *sc, uint64_t *cp_time, uint64_t *prev,
		 int *old, uint32_t *out)
{
	uint64_t total = 0;
	int all, sys, idle, syst, i;
	
	for (i = 0; i < CPUSTATES; i++)
		total += cp_time[i] - prev[i];
	idle = (int)(cp_time[CP_IDLE] - prev[CP_IDLE]);
	syst = (int)(cp_time[CP_SYS] - prev[CP_SYS]);
	all = (total - idle) * 8 / total;
	sys = syst * 8 / total;
	for (i = 0; i < CPUSTATES; i++)
		prev[i] = cp_time[i];
	if ((all != old[0]) || (sys != old[1])) {
		for (i = 0; i < sys; i++) out[i] = sc->sc_sys;
		for (; i < all; i++) out[i] = sc->sc_user;
		for (; i < 8; i++) out[i] = 0;
		old[0] = all;
		old[1] = sys;
		return 1;
	}
	return 0;
}

static void
lightbar_thread(void *cookie)
{
	struct lightbar_softc *sc = cookie;
	uint32_t latch;
	uint64_t prev[2 * CPUSTATES];
	int i, j, old[4] = {0, 0, 0, 0}, intensity[16];

	for (i = 0; i <  2 * CPUSTATES; i++)
		prev[i] = 0;

	tsleep(cookie, PRI_NONE, "lights", hz);

	while (1) {
		int update;
		/* draw CPU0's usage into the upper bar */
		update = lightbar_update(sc,
				sc->sc_cpu[0]->ci_schedstate.spc_cp_time,
				prev, old, &intensity[8]);
		if (sc->sc_cpu[1] != NULL) {
			/*
			 * if we have a 2nd CPU draw its usage into the lower
			 * bar
			 */
			update |= lightbar_update(sc,
				sc->sc_cpu[1]->ci_schedstate.spc_cp_time,
				&prev[CPUSTATES], &old[2], intensity);
		} else {
			/*
			 * if we don't have a 2nd CPU just duplicate the bar
			 * from the first
			 */
			for (i = 0; i < 8; i++)
				intensity[i] = intensity[i + 8];
		}
		if (update) {
			/*
			 * this turns our intensity map into a bit pattern for
			 * the hardware - we have 8 samples in our buffer, for
			 * each LED we set the corresponding bit in intensity[j]
			 * samples
			 */
			for (i = 0; i < 8; i++) {
				latch = 0;
				for (j = 0; j < 16; j++) {
					if (intensity[j] > i)
						latch |= 1 << (j + 16);
				}
				sc->sc_dmabuf[i] = LEDMASK(latch);
			}
		}
			
		tsleep(cookie, PRI_NONE, "lights", hz / 5);
	}
	kthread_exit(0);
}
