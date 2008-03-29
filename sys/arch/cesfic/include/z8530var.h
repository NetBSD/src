/* $NetBSD: z8530var.h,v 1.4 2008/03/29 19:15:34 tsutsui Exp $ */

#include <dev/ic/z8530sc.h>

struct zsc_softc {
	device_t zsc_dev;		/* required first: base device */
	struct	zs_chanstate *zsc_cs[2];	/* channel A and B soft state */
	/* Machine-dependent part follows... */
	void	*zsc_softintr_cookie;
};

uint8_t zs_read_reg(struct zs_chanstate *cs, uint8_t reg);
uint8_t zs_read_csr(struct zs_chanstate *cs);
uint8_t zs_read_data(struct zs_chanstate *cs);

void  zs_write_reg(struct zs_chanstate *cs, uint8_t reg, uint8_t val);
void  zs_write_csr(struct zs_chanstate *cs, uint8_t val);
void  zs_write_data(struct zs_chanstate *cs, uint8_t val);

/* Interrupt priority for the SCC chip; needs to match ZSHARD_PRI. */
#define splzs()		spl4()
#define	IPL_ZS		IPL_HIGH

