/*	$NetBSD: gemini_lpchcvar.h,v 1.2 2008/11/15 05:48:34 cliff Exp $	*/

#ifndef  _ARM_GEMINI_LPHCVAR_H
#define  _ARM_GEMINI_LPHCVAR_H

#include <sys/types.h>
#include <sys/device.h>
#include <machine/bus.h>
#include <arch/arm/gemini/gemini_lpcvar.h>


typedef struct gemini_lpchc_attach_args {
	void	       *lpchc_tag;
        bus_space_tag_t lpchc_iot;
        bus_addr_t      lpchc_addr;
        bus_size_t      lpchc_size;
} gemini_lpchc_attach_args_t;

typedef struct gemini_lpchc_intrq {
	SIMPLEQ_ENTRY(gemini_lpchc_intrq) iq_q;
	int (*iq_func)(void *);
	void *iq_arg;
	uint32_t iq_bit;
	boolean_t iq_isedge;
} gemini_lpchc_intrq_t;

typedef struct gemini_lpchc_softc {
	struct device sc_dev;
	bus_addr_t sc_addr;
	bus_size_t sc_size;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	int sc_intr;
	void *sc_ih;
	SIMPLEQ_HEAD(, gemini_lpchc_intrq) sc_intrq;
} gemini_lpchc_softc_t;

extern void gemini_lpchc_init(lpcintrtag_t);
extern void *gemini_lpchc_intr_establish(lpcintrtag_t, uint, int, int,
        int (*)(void *), void *);
extern void gemini_lpchc_intr_disestablish(lpcintrtag_t, void *);
extern int  gemini_lpchc_intr(void *);



#endif  /* _ARM_GEMINI_LPHCVAR_H */
