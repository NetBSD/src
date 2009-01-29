/*	$NetBSD: zusb.c,v 1.4 2009/01/29 12:28:15 nonaka Exp $	*/

/*
 * Copyright (c) 2008 Christopher Gilbert
 * All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: zusb.c,v 1.4 2009/01/29 12:28:15 nonaka Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <zaurus/zaurus/zaurus_reg.h>
#include <zaurus/zaurus/zaurus_var.h>


#ifdef ZUSB_DEBUG
#define	DPRINTF(s)	printf s
#else
#define	DPRINTF(s)
#endif


struct zusb_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	void *sc_client_ih;
	void *sc_host_ih;
};

static int	zusb_match(device_t, cfdata_t, void *);
static void	zusb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(zusb, sizeof(struct zusb_softc), 
    zusb_match, zusb_attach, NULL, NULL);

static int	zusb_client_intr(void *);
static int	zusb_host_intr(void *);
static void	zusb_test_and_enabled_host_port(struct zusb_softc *);

static int
zusb_match(device_t parent, cfdata_t cf, void *aux)
{

	if (ZAURUS_ISC3000)
		return 1;
	return 0;
}

static void
zusb_attach(device_t parent, device_t self, void *aux)
{
	struct zusb_softc *sc = device_private(self);
	struct pxaip_attach_args *pxa = aux;

	aprint_normal(": USB Mode detection\n");
	aprint_naive("\n");

	sc->sc_dev = self;
	sc->sc_iot = pxa->pxa_iot;

	/* Map I/O space */
	if (bus_space_map(sc->sc_iot, PXA2X0_USBDC_BASE, PXA270_USBDC_SIZE, 0,
				&sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev, "couldn't map memory space\n");
		return;
	}

	pxa2x0_gpio_set_function(C3000_USB_DEVICE_PIN, GPIO_IN);
	sc->sc_client_ih = pxa2x0_gpio_intr_establish(C3000_USB_DEVICE_PIN,
	    IST_EDGE_BOTH, IPL_BIO, zusb_client_intr, sc);

	pxa2x0_gpio_set_function(C3000_USB_HOST_PIN, GPIO_IN);
	sc->sc_host_ih = pxa2x0_gpio_intr_establish(C3000_USB_HOST_PIN,
	    IST_EDGE_BOTH, IPL_BIO, zusb_host_intr, sc);

	/* configure port 2 for input */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, USBDC_UP2OCR,
			USBDC_UP2OCR_HXS | USBDC_UP2OCR_HXOE | 
			USBDC_UP2OCR_DPPDE | USBDC_UP2OCR_DMPDE);

	zusb_test_and_enabled_host_port(sc);
}

static int
zusb_client_intr(void *v)
{
	struct zusb_softc *sc = v;

	DPRINTF(("%s: USB client cable changed\n", device_xname(sc->sc_dev)));
	zusb_test_and_enabled_host_port(sc);

	return 1;
}
	
static int
zusb_host_intr(void *v)
{
	struct zusb_softc *sc = v;

	DPRINTF(("%s: USB host cable changed\n", device_xname(sc->sc_dev)));
	zusb_test_and_enabled_host_port(sc);

	return 1;
}
	
static void
zusb_test_and_enabled_host_port(struct zusb_softc *sc)
{
	int host_cable = pxa2x0_gpio_get_bit(C3000_USB_HOST_PIN);
#ifdef ZUSB_DEBUG
	int client_cable = pxa2x0_gpio_get_bit(C3000_USB_DEVICE_PIN);
#endif

	DPRINTF(("%s: USB cable: host %d, client %d\n",
	    device_xname(sc->sc_dev), host_cable, client_cable));

	if (!host_cable) {
		pxa2x0_gpio_set_function(C3000_USB_HOST_POWER_PIN,
		    GPIO_OUT | GPIO_SET);
		DPRINTF(("%s: USB host power enabled\n",
		    device_xname(sc->sc_dev)));
	} else {
		pxa2x0_gpio_set_function(C3000_USB_HOST_POWER_PIN,
		    GPIO_OUT | GPIO_CLR);
		DPRINTF(("%s: USB host power disabled\n",
		    device_xname(sc->sc_dev)));
	}
}
