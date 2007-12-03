/* $NetBSD: z8530var.h,v 1.2.2.1 2007/12/03 18:35:21 ad Exp $ */

#include <dev/ic/z8530sc.h>

struct zsc_softc {
	struct	device zsc_dev;		/* required first: base device */
	struct	zs_chanstate *zsc_cs[2];	/* channel A and B soft state */
	/* Machine-dependent part follows... */
	void	*zsc_softintr_cookie;
};

u_char zs_read_reg __P((struct zs_chanstate *cs, u_char reg));
u_char zs_read_csr __P((struct zs_chanstate *cs));
u_char zs_read_data __P((struct zs_chanstate *cs));

void  zs_write_reg __P((struct zs_chanstate *cs, u_char reg, u_char val));
void  zs_write_csr __P((struct zs_chanstate *cs, u_char val));
void  zs_write_data __P((struct zs_chanstate *cs, u_char val));

/* Interrupt priority for the SCC chip; needs to match ZSHARD_PRI. */
#define splzs()		spl4()
#define	IPL_ZS		IPL_HIGH

#define	IPL_ZS		IPL_HIGH

