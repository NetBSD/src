/*	$NetBSD: if_fmv_isapnp.c,v 1.1 2002/10/05 15:16:14 tsutsui Exp $	*/

/*
 * All Rights Reserved, Copyright (C) Fujitsu Limited 1995
 *
 * This software may be used, modified, copied, distributed, and sold, in
 * both source and binary form provided that the above copyright, these
 * terms and the following disclaimer are retained.  The name of the author
 * and/or the contributor may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND THE CONTRIBUTOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION.
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Portions copyright (C) 1993, David Greenman.  This software may be used,
 * modified, copied, distributed, and sold, in both source and binary form
 * provided that the above copyright and these terms are retained.  Under no
 * circumstances is the author responsible for the proper functioning of this
 * software, nor does the author assume any responsibility for damages
 * incurred with its use.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_fmv_isapnp.c,v 1.1 2002/10/05 15:16:14 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/mb86960reg.h>
#include <dev/ic/mb86960var.h>
#include <dev/ic/fmvreg.h>
#include <dev/ic/fmvvar.h>

#include <dev/isa/isavar.h>

#include <dev/isapnp/isapnpreg.h>
#include <dev/isapnp/isapnpvar.h>
#include <dev/isapnp/isapnpdevs.h>

int	fmv_isapnp_match __P((struct device *, struct cfdata *, void *));
void	fmv_isapnp_attach __P((struct device *, struct device *, void *));

struct fmv_isapnp_softc {
	struct	mb86960_softc sc_mb86960;	/* real "mb86960" softc */

	/* ISAPnP-specific goo. */
	void	*sc_ih;				/* interrupt cookie */
};

CFATTACH_DECL(fmv_isapnp, sizeof(struct fmv_isapnp_softc),
    fmv_isapnp_match, fmv_isapnp_attach, NULL, NULL);

int
fmv_isapnp_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	int pri, variant;

	pri = isapnp_devmatch(aux, &isapnp_fmv_devinfo, &variant);
	if (pri && variant > 0)
		pri = 0;
	return (pri);
}

void
fmv_isapnp_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct fmv_isapnp_softc *isc = (struct fmv_isapnp_softc *)self;
	struct mb86960_softc *sc = &isc->sc_mb86960;
	struct isapnp_attach_args * const ipa = aux;

	printf("\n");

	if (isapnp_config(ipa->ipa_iot, ipa->ipa_memt, ipa)) {
		printf("%s: can't configure isapnp resources\n",
		   sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_bst = ipa->ipa_iot;
	sc->sc_bsh = ipa->ipa_io[0].h;

	fmv_attach(sc);

	/* Establish the interrupt handler. */
	isc->sc_ih = isa_intr_establish(ipa->ipa_ic, ipa->ipa_irq[0].num,
	    ipa->ipa_irq[0].type, IPL_NET, mb86960_intr, sc);
	if (isc->sc_ih == NULL)
		printf("%s: couldn't establish interrupt handler\n",
		    sc->sc_dev.dv_xname);
}
