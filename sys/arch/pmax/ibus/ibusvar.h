/*	$NetBSD: ibusvar.h,v 1.1 1998/04/19 02:52:45 jonathan Exp $	*/

#ifndef __IBUSVAR_H
#define __IBUSVAR_H


void config_ibus __P((struct device *mb, void *,
	int	printfn __P((void *, const char *)) ));	/* XXX */

/*
 * function types for interrupt establish/diestablish
 */
struct ibus_attach_args;
typedef (ibus_intr_establish_t) __P((void * cookie, int level,
			int (*handler) __P((intr_arg_t)), intr_arg_t arg));
typedef (ibus_intr_disestablish_t)  __P((struct ibus_attach_args *));


/* 
 * Arguments used to attach a ibus "device" to its parent
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
	int	ia_slot;		/* Device slot (table entry). */
	tc_addr_t ia_addr;		/* Device address. */
	int	ia_cookie;		/* Device interrupt "priority" */
};


/*
 * interrrupt estalish functions.
 * These call up to system-specific code to 
 * recompute spl levels.
 */
void	ibus_intr_establish __P((void * cookie, int level,
			int (*handler) __P((intr_arg_t)), intr_arg_t arg));
void	ibus_intr_disestablish __P((struct ibus_attach_args *));
int ibusprint __P((void *aux, const char *pnp));

#endif /* __IBUSVAR_H */
