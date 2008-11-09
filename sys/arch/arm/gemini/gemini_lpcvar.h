/*	$NetBSD: gemini_lpcvar.h,v 1.1 2008/11/09 09:15:42 cliff Exp $	*/

#ifndef _ARM_GEMINI_LPCVAR_H
#define _ARM_GEMINI_LPCVAR_H

#include <sys/types.h>
#include <sys/device.h>
#include <machine/bus.h>

#define GEMINI_LPC_LDN_ALL	-1	/* "global" LDN */

typedef void * lpctag_t;
typedef void * lpcintrtag_t;

typedef struct gemini_lpc_bus_ops {
	uint8_t	(*lpc_pnp_read)(lpctag_t, int, uint);
	void	(*lpc_pnp_write)(lpctag_t, int, uint, uint8_t);
	void	(*lpc_pnp_enter)(lpctag_t);
	void	(*lpc_pnp_exit)(lpctag_t);
	void   *(*lpc_intr_establish)(lpcintrtag_t,
			uint, int, int, int (*)(void *), void *);
	void	(*lpc_intr_diestablish)(lpcintrtag_t, void *);
} gemini_lpc_bus_ops_t;

typedef struct gemini_lpc_attach_args {
        void	       *lpc_intrtag;
        void	       *lpc_tag;
	uint		lpc_ldn;
	bus_addr_t	lpc_base;
	bus_addr_t	lpc_addr;
	bus_size_t	lpc_size;
        uint		lpc_intr;
        bus_space_tag_t	lpc_iot;
	gemini_lpc_bus_ops_t *lpc_bus_ops;
} gemini_lpc_attach_args_t;

typedef struct gemini_lpc_softc {
	struct device		sc_dev;
	bus_addr_t		sc_addr;
	bus_size_t		sc_size;
	int			sc_intr;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	void		       *sc_lpchctag;
} gemini_lpc_softc_t;


/* la_flags */
#define LPC_FL_ENABLED    0x01            /* device is enabled */



#endif	/* _ARM_GEMINI_LPCVAR_H */
