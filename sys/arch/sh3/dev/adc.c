/*	$NetBSD: adc.c,v 1.10.2.1 2009/05/13 17:18:21 jym Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: adc.c,v 1.10.2.1 2009/05/13 17:18:21 jym Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <sh3/adcreg.h>
#include <sh3/dev/adcvar.h>

#define ADC_(x)    (*((volatile uint8_t *)SH7709_AD ## x))


static int	adc_match(device_t, cfdata_t, void *);
static void	adc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(adc, 0,
    adc_match, adc_attach, NULL, NULL);

static int	adc_search(device_t, cfdata_t, const int *, void *);
static int	adc_print(void *, const char *);


static int
adc_match(device_t parent, cfdata_t cf, void *aux)
{

	/* REMINDER: also in 7727 and 7729 */
	if ((cpu_product != CPU_PRODUCT_7709)
	    && (cpu_product != CPU_PRODUCT_7709A)
	    && (cpu_product != CPU_PRODUCT_7706))
		return (0);

	if (strcmp(cf->cf_name, "adc") != 0)
		return (0);

	return (1);
}


static void
adc_attach(device_t parent, device_t self, void *aux)
{

	ADC_(CSR) = 0;
	ADC_(CR) = 0;

	aprint_naive("\n");
	aprint_normal("\n");

	config_search_ia(adc_search, self, "adc", NULL);

	/*
	 * XXX: TODO: provide hooks to manage power.  For now register
	 * null hooks which is no worse than before.
	 *
	 * NB: ADC registers are reset by standby!
	 */
	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "unable to establish power handler\n");
}


static int
adc_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
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
	        snprintb(bits, sizeof(buts), SH7709_ADCSR_BITS, csr);
		printf("adc_sample_channel(%d): CSR=%s", chan, bits);
		cr = ADC_(CR);
		cr &= ~0x07;	/* three lower bits always read as 1s */
	        snprintb(bits, sizeof(buts), SH7709_ADCR_BITS, cr);
		printf(", CR=%s\n", bits);
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
