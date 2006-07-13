/* $NetBSD: spicvar.h,v 1.1.56.1 2006/07/13 17:49:23 gdamore Exp $ */

#include <dev/sysmon/sysmonvar.h>

struct spic_softc {
	struct device sc_dev;

	bus_space_tag_t	sc_iot;
	bus_space_handle_t sc_ioh;

	struct callout sc_poll;

	int sc_buttons;
	char sc_enabled;

	struct device *sc_wsmousedev;

#define	SPIC_PSWITCH_LID	0
#define	SPIC_PSWITCH_SUSPEND	1
#define	SPIC_PSWITCH_HIBERNATE	2
#define	SPIC_NPSWITCH		3	
	struct sysmon_pswitch sc_smpsw[SPIC_NPSWITCH];
};

void spic_attach(struct spic_softc *);

int spic_intr(void *);
