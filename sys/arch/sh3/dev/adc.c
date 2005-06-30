/*	$NetBSD: adc.c,v 1.4 2005/06/30 17:03:54 drochner Exp $ */

/*
 * Copyright (c) 2003 Valeriy E. Ushakov
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
__KERNEL_RCSID(0, "$NetBSD: adc.c,v 1.4 2005/06/30 17:03:54 drochner Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#ifdef GPROF
#include <sys/gmon.h>
#endif

#include <sh3/adcreg.h>
#include <sh3/dev/adcvar.h>

#define ADC_(x)    (*((volatile uint8_t *)SH7709_AD ## x))


struct adc_softc {
	struct device sc_dev;
};

static int	adc_match(struct device *, struct cfdata *, void *);
static void	adc_attach(struct device *, struct device *, void *);

CFATTACH_DECL(adc, sizeof(struct adc_softc),
    adc_match, adc_attach, NULL, NULL);

static int	adc_search(struct device *, struct cfdata *,
			   const locdesc_t *, void *);
static int	adc_print(void *, const char *);


static int
adc_match(struct device *parent, struct cfdata *cfp, void *aux)
{

	/* REMINDER: also in 7727 and 7729 */
	if ((cpu_product != CPU_PRODUCT_7709)
	    && (cpu_product != CPU_PRODUCT_7709A))
		return (0);

	if (strcmp(cfp->cf_name, "adc") != 0)
		return (0);

	return (1);
}


static void
adc_attach(struct device *parent, struct device *self, void *aux)
{
	/* struct adc_softc *sc = (struct adc_softc *)self; */

	ADC_(CSR) = 0;
	ADC_(CR) = 0;

	printf("\n");
	config_search_ia(adc_search, self, "adc", NULL);
}


static int
adc_search(struct device *parent, struct cfdata *cf,
	   const locdesc_t *ldesc, void *aux)
{

	if (config_match(parent, cf, NULL) > 0)
		config_attach(parent, cf, NULL, adc_print);

	return (0);
}


static int
adc_print(void *aux, const char *pnp)
{

	return (pnp ? QUIET : UNCONF);
}


/*
 * Sample specified ADC channel.
 * Must be called at spltty().
 */
int
adc_sample_channel(int chan)
{
	volatile uint8_t *hireg;
	volatile uint8_t *loreg;
	int regoff;
	uint8_t csr;
	int timo;
#ifdef DIAGNOSTIC
	uint8_t cr;
	char bits[128];
#endif
	if ((chan < 0) || (chan >= 8))
		return (-1);

	regoff = (chan & 0x03) << 2;
	hireg = (volatile uint8_t *)(SH7709_ADDRAH + regoff);
	loreg = (volatile uint8_t *)(SH7709_ADDRAL + regoff);

	/* the loop below typically takes 27 iterations on j680 */
	timo = 300;

#ifdef DIAGNOSTIC
	csr = ADC_(CSR);
	if ((csr & SH7709_ADCSR_ADST) != 0) {
		/* another conversion is in progress?! */
		printf("adc_sample_channel(%d): CSR=%s", chan,
		       bitmask_snprintf(csr, SH7709_ADCSR_BITS,
					bits, sizeof(bits)));
		cr = ADC_(CR);
		cr &= ~0x07;	/* three lower bits always read as 1s */
		printf(", CR=%s\n",
		       bitmask_snprintf(cr, SH7709_ADCR_BITS,
					bits, sizeof(bits)));
		return (-1);
	}
#endif

	/* start scanning */
	ADC_(CSR) = chan | SH7709_ADCSR_ADST | SH7709_ADCSR_CKS;

	do {
		csr = ADC_(CSR);
		if (timo-- == 0)
			break;
	} while ((csr & SH7709_ADCSR_ADF) == 0);

	/* stop scanning */
	csr &= ~(SH7709_ADCSR_ADF | SH7709_ADCSR_ADST);
	ADC_(CSR) = csr;

	if (timo <= 0) {
		printf("adc_sample_channel(%d): timed out\n", chan);
		return (-1);
	}

	/*
	 * 10 bit of data: bits [9..2] in the high register, bits
	 * [1..0] in the bits [7..6] of the low register.
	 */
	return (((*hireg << 8) | *loreg) >> 6);
}
