/*	$NetBSD: obsled.c,v 1.7 2009/11/05 18:16:00 dyoung Exp $	*/

/*
 * Copyright (c) 2004 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obsled.c,v 1.7 2009/11/05 18:16:00 dyoung Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <machine/obs266.h>

#include <powerpc/ibm4xx/dev/gpiovar.h>

struct obsled_softc {
        struct device sc_dev;
	gpio_tag_t sc_tag;
        int sc_addr;
	int sc_led_state;	/* LED status (ON=1/OFF=0) */
	int sc_led_state_mib;
};

static void	obsled_attach(struct device *, struct device *, void *);
static int	obsled_match(struct device *, struct cfdata *, void *);
static int	obsled_sysctl_verify(SYSCTLFN_PROTO);
static void	obsled_set_state(struct obsled_softc *);

CFATTACH_DECL(obsled, sizeof(struct obsled_softc),
	obsled_match, obsled_attach, NULL, NULL);

static int
obsled_match(struct device *parent, struct cfdata *cf, void *aux)
{
        struct gpio_attach_args *ga = aux;

	/* XXX: support only OpenBlockS266 LED */
        if (ga->ga_addr == OBS266_GPIO_LED1)
                return (1);
        else if (ga->ga_addr == OBS266_GPIO_LED2)
                return (1);
        else if (ga->ga_addr == OBS266_GPIO_LED4)
                return (1);

        return (0);
}

static void
obsled_attach(struct device *parent, struct device *self, void *aux)
{
        struct obsled_softc *sc = (struct obsled_softc *)self;
        struct gpio_attach_args *ga = aux;
	struct sysctlnode *node;
	int err, node_mib;
	char led_name[5];
	/* int led = (1 << device_unit(&sc->sc_dev)); */

	snprintf(led_name, sizeof(led_name),
		"led%d", (1 << device_unit(&sc->sc_dev)) & 0x7);
        aprint_naive(": OpenBlockS %s\n", led_name);
        aprint_normal(": OpenBlockS %s\n", led_name);

        sc->sc_tag = ga->ga_tag;
        sc->sc_addr = ga->ga_addr;
	sc->sc_led_state = 0;

	obs266_led_set(OBS266_LED_OFF);

	/* add sysctl interface */
	err = sysctl_createv(NULL, 0,
			NULL, NULL,
			0, CTLTYPE_NODE,
			"hw",
			NULL,
			NULL, 0, NULL, 0,
			CTL_HW, CTL_EOL);
	if (err != 0)
		return;
	err = sysctl_createv(NULL, 0,
			NULL, (const struct sysctlnode **)&node,
			0, CTLTYPE_NODE,
			"obsled",
			NULL,
			NULL, 0, NULL, 0,
			CTL_HW, CTL_CREATE, CTL_EOL);
	if (err  != 0)
		return;
	node_mib = node->sysctl_num;
	err = sysctl_createv(NULL, 0,
			NULL, (const struct sysctlnode **)&node,
			CTLFLAG_READWRITE, CTLTYPE_INT,
			led_name,
			SYSCTL_DESCR("OpenBlockS LED state (0=off, 1=on)"),
			obsled_sysctl_verify, 0, sc, 0,
			CTL_HW, node_mib, CTL_CREATE, CTL_EOL);
	if (err  != 0)
		return;

	sc->sc_led_state_mib = node->sysctl_num;
#if 0
	{
		gpio_tag_t tag = sc->sc_tag;
	 	(*(tag)->io_or_write)((tag)->cookie, sc->sc_addr, 0);
	}
#endif
}

static int
obsled_sysctl_verify(SYSCTLFN_ARGS)
{
	struct obsled_softc *sc = rnode->sysctl_data;
	struct sysctlnode node;
	int err, state;

	node = *rnode;
	state = sc->sc_led_state;
	node.sysctl_data = &state;

	err = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (err != 0 || newp == NULL)
		return err;

	if (node.sysctl_num == sc->sc_led_state_mib) {
		if (state < 0 || state > 1)
			return EINVAL;
		sc->sc_led_state = state;
		obsled_set_state(sc);
	}

	return 0;
}

static void
obsled_set_state(struct obsled_softc *sc)
{
	gpio_tag_t tag = sc->sc_tag;

	(*(tag)->io_or_write)((tag)->cookie,
				sc->sc_addr, 
				(~(sc->sc_led_state) & 1));
	/* GPIO LED input ON=0/OFF=1 */
}

/*
 * Setting LED interface for inside kernel.
 * Argumnt `led' is 3-bit LED state (led=0-7/ON=1/OFF=0).
 */
void
obs266_led_set(int led)
{
	device_t dv;
	deviter_t di;

	/*
	 * Sarching "obsled" devices from device tree.
	 * Do you have something better idea?
	 */
        for (dv = deviter_first(&di, DEVITER_F_ROOT_FIRST); dv != NULL;
	     dv = deviter_next(&di)) {
		if (device_is_a(dv, "obsles")) {
			struct obsled_softc *sc = device_private(dv);
			sc->sc_led_state =
			    (led & (1 << device_unit(dv))) >> device_unit(dv);
			obsled_set_state(sc);
		}
	}
	deviter_release(&di);
}
