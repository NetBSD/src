#include "sys/param.h"
#include "sys/systm.h"
#include "sys/protosw.h"
#include "sys/socket.h"
#include "sys/device.h"
#include "net/if.h"

#ifdef INET
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/in_var.h"
#include "netinet/ip.h"
#include "netinet/if_ether.h"
#endif

#include "machine/autoconf.h"
#include "machine/cpu.h"
#include "machine/isr.h"
#include "machine/mtpr.h"
#include "machine/obio.h"
#include "machine/idprom.h"

#include "bpfilter.h"

#include "if_lereg.h"
#include "if_le.h"

/*
 * things to do:
 *    allocate dvma area memory for dual access
 *    use/default config parameters for lance configuration
 *    setup isr handler
 *    set ethernet address
 *    set meta address
 *
 */

int le_machdep_attach(parent, self, args)
     struct device *parent;
     struct device *self;
     void *args;
{
    caddr_t dvma_malloc(), le_addr;
    int level, leintr(), unit = DEVICE_UNIT(self);
    struct le_softc *le = (struct le_softc *) self;
    struct obio_cf_loc *obio_loc = OBIO_LOC(self);
    
    /* allocate "shared" memory */
    le->sc_r2 =
	(struct lereg2 *) dvma_malloc(sizeof(struct lereg2)); 
    if (!le->sc_r2) {
	printf(": not enough dvma space\n");
	return 1;
    }
    idprom_etheraddr(le->sc_addr); /* ethernet addr */
    le_addr = OBIO_DEFAULT_PARAM(caddr_t, obio_loc->obio_addr, OBIO_AMD_ETHER);

    /* register access */
    le->sc_r1 = (struct lereg1 *)
	obio_alloc(le_addr, (caddr_t) OBIO_AMD_ETHER_SIZE, OBIO_WRITE);
    if (!le->sc_r1) {
	printf(": not enough obio space\n");
	return 1;
    }
    level = OBIO_DEFAULT_PARAM(int, obio_loc->obio_level, 3);
    obio_print(le_addr, level);
    le->sc_machdep = NULL;
    isr_add(level, leintr, unit);
    return 0;
}

int le_machdep_match(parent, cf, args)
     struct device *parent;
     struct cfdata *cf;
     void *args;
{
    caddr_t le_addr;
    struct obio_cf_loc *obio_loc = (struct obio_cf_loc *) CFDATA_LOC(cf);

    le_addr = OBIO_DEFAULT_PARAM(caddr_t, obio_loc->obio_addr, OBIO_AMD_ETHER);
    return /* !obio_probe_byte(le_addr)*/ 1;
}
