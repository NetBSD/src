/*	$NetBSD: bi.c,v 1.15 2000/07/06 17:47:02 ragge Exp $ */
/*
 * Copyright (c) 1996 Ludd, University of Lule}, Sweden.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed at Ludd, University of 
 *      Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
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



/*
 * VAXBI specific routines.
 */
/*
 * TODO
 *   handle BIbus errors more gracefully.
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/bi/bireg.h>
#include <dev/bi/bivar.h>

static int bi_print __P((void *, const char *));

struct bi_list bi_list[] = {
	{BIDT_MS820, 1, "ms820"},
	{BIDT_DRB32, 0, "drb32"},
	{BIDT_DWBUA, 0, "dwbua"},
	{BIDT_KLESI, 0, "klesi"},
	{BIDT_KA820, 1, "ka820"},
	{BIDT_DB88,  0, "db88"},
	{BIDT_CIBCA, 0, "cibca"},
	{BIDT_DMB32, 0, "dmb32"},
	{BIDT_CIBCI, 0, "cibci"},
	{BIDT_KA800, 0, "ka800"},
	{BIDT_KDB50, 0, "kdb50"},
	{BIDT_DWMBA, 0, "dwmba"},
	{BIDT_KFBTA, 0, "kfbta"},
	{BIDT_DEBNK, 0, "debnk"},
	{BIDT_DEBNA, 0, "debna"},
	{0,0,0}
};

int
bi_print(aux, name)
	void *aux;
	const char *name;
{
	struct bi_attach_args *ba = aux;
	struct bi_list *bl;
	u_int16_t nr;

	nr = bus_space_read_2(ba->ba_iot, ba->ba_ioh, 0);
	for (bl = &bi_list[0]; bl->bl_nr; bl++)
		if (bl->bl_nr == nr)
			break;

	if (name) {
		if (bl->bl_nr == 0)
			printf("unknown device 0x%x",
			    bus_space_read_2(ba->ba_iot, ba->ba_ioh, 0));
		else
			printf(bl->bl_name);
		printf(" at %s", name);
	}
	printf(" node %d", ba->ba_nodenr);
#ifdef DEBUG
	if (bus_space_read_4(ba->ba_iot, ba->ba_ioh, BIREG_SADR) &&
	    bus_space_read_4(ba->ba_iot, ba->ba_ioh, BIREG_EADR))
		printf(" [sadr %x eadr %x]",
		    bus_space_read_4(ba->ba_iot, ba->ba_ioh, BIREG_SADR),
		    bus_space_read_4(ba->ba_iot, ba->ba_ioh, BIREG_EADR));
#endif
	return bl->bl_havedriver ? UNCONF : UNSUPP;
}

static	int lastiv = 0;

void
bi_attach(sc)
	struct bi_softc *sc;
{
	struct bi_attach_args ba;
	int nodenr;

	printf("\n");

	ba.ba_iot = sc->sc_iot;
	ba.ba_busnr = sc->sc_busnr;
	ba.ba_dmat = sc->sc_dmat;
	ba.ba_intcpu = sc->sc_intcpu;
	/*
	 * Interrupt numbers. All vectors from 256-512 are free, use
	 * them for BI devices and just count them up.
	 * Above 512 are only interrupt vectors for unibus devices.
	 */
	for (nodenr = 0; nodenr < NNODEBI; nodenr++) {
		if (bus_space_map(sc->sc_iot, sc->sc_addr + BI_NODE(nodenr),
		    BI_NODESIZE, 0, &ba.ba_ioh)) {
			printf("bi_attach: bus_space_map failed, node %d\n", 
			    nodenr);
			return;
		}
		if (badaddr((caddr_t)ba.ba_ioh, 4)) {
			bus_space_unmap(sc->sc_iot, ba.ba_ioh, BI_NODESIZE);
			continue;
		}
		ba.ba_nodenr = nodenr;
		ba.ba_ivec = 256 + lastiv;
		lastiv += 4;
		config_found(&sc->sc_dev, &ba, bi_print);
	}
}
