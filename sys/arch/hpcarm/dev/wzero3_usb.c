/*	$NetBSD: wzero3_usb.c,v 1.1.2.2 2010/04/30 14:39:25 uebayasi Exp $	*/

/*
 * Copyright (c) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
 * All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wzero3_usb.c,v 1.1.2.2 2010/04/30 14:39:25 uebayasi Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <machine/bus.h>
#include <machine/bootinfo.h>
#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <hpcarm/dev/wzero3_reg.h>

#if defined(WZERO3USB_DEBUG)
#define	DPRINTF(s)	printf s
#else
#define	DPRINTF(s)
#endif

struct wzero3usb_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	int sc_client_pin;
	int sc_host_pin;
	int sc_host_power_pin;

	void *sc_client_ih;
	void *sc_host_ih;
};

static int wzero3usb_match(device_t, cfdata_t, void *);
static void wzero3usb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wzero3usb, sizeof(struct wzero3usb_softc), 
    wzero3usb_match, wzero3usb_attach, NULL, NULL);

static int wzero3usb_client_intr(void *);
static int wzero3usb_host_intr(void *);
static void wzero3usb_host_power(struct wzero3usb_softc *);

static const struct wzero3usb_model {
	platid_mask_t *platid;
	int client_pin;
	int host_pin;
	int host_power_pin;
} wzero3usb_table[] = {
	/* WS003SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS003SH,
		GPIO_WS003SH_USB_CLIENT_DETECT,
		-1,	/* None */
		-1,	/* None */
	},
	/* WS004SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS004SH,
		GPIO_WS003SH_USB_CLIENT_DETECT,
		-1,	/* None */
		-1,	/* None */
	},
	/* WS007SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS007SH,
		GPIO_WS007SH_USB_CLIENT_DETECT,
		GPIO_WS007SH_USB_HOST_DETECT,
		GPIO_WS007SH_USB_HOST_POWER,
	},
	/* WS011SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS011SH,
		GPIO_WS011SH_USB_CLIENT_DETECT,
		GPIO_WS011SH_USB_HOST_DETECT,
		GPIO_WS011SH_USB_HOST_POWER,
	},
	/* XXX: WS020SH */

	{ NULL, -1, -1, -1, }
};

static const struct wzero3usb_model *
wzero3usb_lookup(void)
{
	const struct wzero3usb_model *model;

	for (model = wzero3usb_table; model->platid != NULL; model++) {
		if (platid_match(&platid, model->platid)) {
			return model;
		}
	}
	return NULL;
}

static int
wzero3usb_match(device_t parent, cfdata_t cf, void *aux)
{

	if (strcmp(cf->cf_name, "wzero3usb") != 0)
		return 0;
	if (wzero3usb_lookup() == NULL)
		return 0;
	return 1;
}

static void
wzero3usb_attach(device_t parent, device_t self, void *aux)
{
	struct wzero3usb_softc *sc = device_private(self);
	struct pxaip_attach_args *pxa = aux;
	const struct wzero3usb_model *model;

	sc->sc_dev = self;
	sc->sc_iot = pxa->pxa_iot;

	aprint_normal(": USB Mode detection\n");

	model = wzero3usb_lookup();
	if (model == NULL) {
		aprint_error_dev(self, "unknown model\n");
		return;
	}
	sc->sc_client_pin = model->client_pin;
	sc->sc_host_pin = model->host_pin;
	sc->sc_host_power_pin = model->host_power_pin;

	if (bus_space_map(sc->sc_iot, PXA2X0_USBDC_BASE, PXA270_USBDC_SIZE, 0,
				&sc->sc_ioh)) {
		aprint_error_dev(self, "couldn't map memory space\n");
		return;
	}

	if (sc->sc_client_pin >= 0) {
		sc->sc_client_ih = pxa2x0_gpio_intr_establish(sc->sc_client_pin,
		    IST_EDGE_BOTH, IPL_BIO, wzero3usb_client_intr, sc);
	}
	if (sc->sc_host_pin >= 0) {
		sc->sc_host_ih = pxa2x0_gpio_intr_establish(sc->sc_host_pin,
		    IST_EDGE_BOTH, IPL_BIO, wzero3usb_host_intr, sc);
	}

	/* configure port 2 for input */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, USBDC_UP2OCR,
			USBDC_UP2OCR_HXS | USBDC_UP2OCR_HXOE | 
			USBDC_UP2OCR_DPPDE | USBDC_UP2OCR_DMPDE);

	wzero3usb_host_power(sc);
}

static int
wzero3usb_host_intr(void *v)
{
	struct wzero3usb_softc *sc = (struct wzero3usb_softc *)v;

	DPRINTF(("%s: USB host cable changed: level = %s\n",
	    device_xname(sc->sc_dev),
	    pxa2x0_gpio_get_bit(sc->sc_host_pin) ? "H" : "L"));

	wzero3usb_host_power(sc);

	return 1;
}

static int
wzero3usb_client_intr(void *v)
{
	struct wzero3usb_softc *sc = (struct wzero3usb_softc *)v;

	DPRINTF(("%s: USB client cable changed: level = %s\n",
	    device_xname(sc->sc_dev),
	    pxa2x0_gpio_get_bit(sc->sc_client_pin) ? "H" : "L"));

	(void)sc; /*XXX*/

	return 1;
}

static void
wzero3usb_host_power(struct wzero3usb_softc *sc)
{
	int host_cable;

	if (sc->sc_host_pin >= 0 && sc->sc_host_power_pin >= 0) {
		host_cable = pxa2x0_gpio_get_bit(sc->sc_host_pin);

		if (!host_cable) {
			DPRINTF(("%s: enable USB host power\n",
			    device_xname(sc->sc_dev)));
			pxa2x0_gpio_set_bit(sc->sc_host_power_pin);
		} else {
			DPRINTF(("%s: disable USB host power\n",
			    device_xname(sc->sc_dev)));
			pxa2x0_gpio_clear_bit(sc->sc_host_power_pin);
		}
	}
}
