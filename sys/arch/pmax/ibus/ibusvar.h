/* $NetBSD: ibusvar.h,v 1.6.8.1 1999/12/27 18:33:26 wrstuden Exp $ */

#ifndef _IBUSVAR_H_
#define _IBUSVAR_H_ 1

#include <machine/bus.h>

struct ibus_attach_args;

struct ibus_softc {
	struct device	sc_dev;

	void	(*sc_intr_establish) __P((struct device *, void *,
					int, int (*)(void *), void *));
	void	(*sc_intr_disestablish) __P((struct device *, void *));
};

/*
 * Arguments used to attach an ibus "device" to its parent
 */
struct ibus_dev_attach_args {
	const char *ida_busname;		/* XXX should be common */
	bus_space_tag_t	ida_memt;

	int	ida_ndevs;
	struct ibus_attach_args	*ida_devs;
	void	(*ida_establish) __P((struct device *, void *,
					int, int (*)(void *), void *));
	void	(*ida_disestablish) __P((struct device *, void *));
};

/*
 * Arguments used to attach devices to an ibus
 */
struct ibus_attach_args {
	const char *ia_name;		/* Device name. */
	int	ia_cookie;		/* Device slot (table entry). */
	u_int32_t ia_addr;		/* Device address (KSEG1). */
	int	ia_basz;		/* badaddr() size */
};

void ibusattach __P((struct device *, struct device *, void *));
int  ibusprint __P((void *, const char *));
void ibus_intr_establish __P((struct device *, void * cookie, int level,
			int (*handler)(void *), void *arg));
void ibus_intr_disestablish __P((struct device *, void *));

int  badaddr __P((void *, u_int));

void dec_3100_intr_establish __P((struct device *, void *,
		int, int (*)(void *), void *));
void dec_3100_intr_disestablish __P((struct device *, void *));

void dec_5100_intr_establish __P((struct device *, void *,
		int, int (*)(void *), void *));
void dec_5100_intr_disestablish __P((struct device *, void *));

#endif /* _IBUSVAR_H_ */
