/* $NetBSD: eisa_machdep.h,v 1.10.12.1 2014/08/20 00:02:41 tls Exp $ */

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
#define	eisa_attach_hook(p, s, a)					\
    (*(a)->eba_ec->ec_attach_hook)((p), (s), (a))
#define	eisa_maxslots(c)						\
    (*(c)->ec_maxslots)((c)->ec_v)
#define	eisa_intr_map(c, i, hp)						\
    (*(c)->ec_intr_map)((c)->ec_v, (i), (hp))
#define	eisa_intr_string(c, h, buf, len)				\
    (*(c)->ec_intr_string)((c)->ec_v, (h), (buf), (len))
#define	eisa_intr_evcnt(c, h)						\
    (*(c)->ec_intr_evcnt)((c)->ec_v, (h))
#define	eisa_intr_establish(c, h, t, l, f, a)				\
    (*(c)->ec_intr_establish)((c)->ec_v, (h), (t), (l), (f), (a))
#define	eisa_intr_disestablish(c, h)					\
    (*(c)->ec_intr_disestablish)((c)->ec_v, (h))

int	eisa_conf_read_mem(eisa_chipset_tag_t, int, int, int,
	    struct eisa_cfg_mem *);
int	eisa_conf_read_irq(eisa_chipset_tag_t, int, int, int,
	    struct eisa_cfg_irq *);
int	eisa_conf_read_dma(eisa_chipset_tag_t, int, int, int,
	    struct eisa_cfg_dma *);
int	eisa_conf_read_io(eisa_chipset_tag_t, int, int, int,
	    struct eisa_cfg_io *);

/*
 * Internal functions, NOT TO BE USED BY MACHINE-INDEPENDENT CODE!
 */

void	eisa_init(eisa_chipset_tag_t);

extern bus_size_t eisa_config_stride;
extern paddr_t eisa_config_addr;
