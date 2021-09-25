/* $NetBSD: eisa_machdep.h,v 1.13 2021/09/25 20:16:17 thorpej Exp $ */

/*
 * Copyright (c) 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * Types provided to machine-independent EISA code.
 */
typedef struct alpha_eisa_chipset *eisa_chipset_tag_t;
typedef int eisa_intr_handle_t;

struct alpha_eisa_chipset {
	void	*ec_v;

	void	(*ec_attach_hook)(device_t, device_t,
		    struct eisabus_attach_args *);
	int	(*ec_maxslots)(void *);
	int	(*ec_intr_map)(void *, u_int, eisa_intr_handle_t *);
	const char *(*ec_intr_string)(void *, eisa_intr_handle_t, char *,
		    size_t);
	const struct evcnt *(*ec_intr_evcnt)(void *, eisa_intr_handle_t);
	void	*(*ec_intr_establish)(void *, eisa_intr_handle_t,
		    int, int, int (*)(void *), void *);
	void	(*ec_intr_disestablish)(void *, void *);
};

/*
 * Functions provided to machine-independent EISA code.
 */
void		eisa_attach_hook(device_t, device_t,
		    struct eisabus_attach_args *);
int		eisa_maxslots(eisa_chipset_tag_t);
int		eisa_intr_map(eisa_chipset_tag_t, u_int, eisa_intr_handle_t *);
const char *	eisa_intr_string(eisa_chipset_tag_t, eisa_intr_handle_t,
		    char *, size_t);
const struct evcnt *eisa_intr_evcnt(eisa_chipset_tag_t, eisa_intr_handle_t);
void *		eisa_intr_establish(eisa_chipset_tag_t, eisa_intr_handle_t,
		    int, int, int (*)(void *), void *);
void		eisa_intr_disestablish(eisa_chipset_tag_t, void *);

int		eisa_conf_read_mem(eisa_chipset_tag_t, int, int, int,
		    struct eisa_cfg_mem *);
int		eisa_conf_read_irq(eisa_chipset_tag_t, int, int, int,
		    struct eisa_cfg_irq *);
int		eisa_conf_read_dma(eisa_chipset_tag_t, int, int, int,
		    struct eisa_cfg_dma *);
int		eisa_conf_read_io(eisa_chipset_tag_t, int, int, int,
		    struct eisa_cfg_io *);

/*
 * Internal functions, NOT TO BE USED BY MACHINE-INDEPENDENT CODE!
 */

void	eisa_init(eisa_chipset_tag_t);

extern bus_size_t eisa_config_stride;
extern paddr_t eisa_config_addr;
