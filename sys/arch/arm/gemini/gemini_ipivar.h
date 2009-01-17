/*	$NetBSD: gemini_ipivar.h,v 1.1.4.2 2009/01/17 13:27:52 mjf Exp $	*/

#ifndef _GEMINI_IPIVAR_H
#define _GEMINI_IPIVAR_H

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

typedef struct gemini_ipi_intrq {
	SIMPLEQ_ENTRY(gemini_ipi_intrq) iq_q;
	int (*iq_func)(void *);
	void *iq_arg;
} gemini_ipi_intrq_t;

typedef struct gemini_ipi_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
        bus_space_handle_t sc_ioh;
        bus_addr_t sc_addr;
        bus_size_t sc_size;
	int sc_intr;
	void *sc_ih; 
	SIMPLEQ_HEAD(, gemini_ipi_intrq) sc_intrq;
} gemini_ipi_softc_t;

extern void *ipi_intr_establish(int (*)(void *), void *);
extern void  ipi_intr_disestablish(void *); 
extern int   ipi_send(void);

#endif /* _GEMINI_IPIVAR_H */
