/*	$NetBSD: ibusvar.h,v 1.2.2.2 1999/03/15 02:00:45 nisimura Exp $	*/

#ifndef __IBUSVAR_H
#define __IBUSVAR_H

/*
 * Arguments used to attach devices to an ibus
 */
struct ibus_attach_args {
	char	*ia_name;		/* Device name. */
	u_int32_t ia_addr;		/* Device address. */
	void	*ia_cookie;		/* Device cookie */
};

/* 
 * Arguments used to attach a ibus "device" to its parent
 */
struct ibus_dev_attach_args {
	const char *ibd_busname;		/* XXX should be common */
#ifdef notyet
	bus_space_tag_t	iba_memt;
#endif
	void (*ibd_establish)
		__P((struct device *, void *, int, int (*)(void *), void *));
	void (*ibd_disestablish) __P((struct device *, void *));
	int ibd_ndevs;
	struct ibus_attach_args	*ibd_devs;
};

struct ibus_softc {
	struct device	ibd_dev;
	int		ibd_ndevs;
	struct ibus_attach_args *ibd_devs;
	void (*ibd_establish)
		__P((struct device *, void *, int, int (*)(void *), void *));
	void (*ibd_disestablish) __P((struct device *, void *));
};

void ibus_devattach __P((struct device *, void *));

/*
 * interrrupt estalish functions.
 * These call up to system-specific code to 
 * recompute spl levels.
 */
void ibus_intr_establish
		__P((struct device *, void *, int, int (*)(void *), void *));
void ibus_intr_disestablish __P((struct device *, void *));

#endif /* __IBUSVAR_H */
