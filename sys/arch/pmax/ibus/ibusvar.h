/*	$NetBSD: ibusvar.h,v 1.6 1999/04/24 08:01:09 simonb Exp $	*/

#ifndef __IBUSVAR_H
#define __IBUSVAR_H

#include <mips/cpuregs.h>

/*
 * function types for interrupt establish/disestablish
 */
struct ibus_attach_args;
typedef int (ibus_intr_establish_t) __P((void * cookie, int level,
			int (*handler)(intr_arg_t), intr_arg_t arg));
typedef int (ibus_intr_disestablish_t)  __P((struct ibus_attach_args *));


/*
 * Arguments used to attach an ibus "device" to its parent
 */
struct ibus_dev_attach_args {
	const char *ibd_busname;		/* XXX should be common */
#ifdef notyet
	bus_space_tag_t	iba_memt;
#endif
	ibus_intr_establish_t	(*ibd_establish);
	ibus_intr_disestablish_t (*ibd_disestablish);
	int			ibd_ndevs;
	struct ibus_attach_args	*ibd_devs;
};

/*
 * Arguments used to attach devices to an ibus
 */
struct ibus_attach_args {
	char	*ia_name;		/* Device name. */
	int	ia_cookie;		/* Device slot (table entry). */
	u_int32_t ia_addr;		/* Device address. */
};


/*
 * interrupt establish functions.
 * These call up to system-specific code to
 * recompute spl levels.
 */
void	ibus_intr_establish __P((void * cookie, int level,
			int (*handler)(intr_arg_t), intr_arg_t arg));
void	ibus_intr_disestablish __P((struct ibus_attach_args *));
int	ibusprint __P((void *aux, const char *pnp));

#endif /* __IBUSVAR_H */
