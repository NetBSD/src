#ifndef __DEV_PPBUS_LPTVAR_H
#define __DEV_PPBUS_LPTVAR_H

#include <machine/vmparam.h>
#include <dev/ppbus/ppbus_device.h>

/* #define LPINITRDY       4       wait up to 4 seconds for a ready 
#define BUFSTATSIZE     32
#define LPTOUTINITIAL   10       initial timeout to wait for ready 1/10 s 
#define LPTOUTMAX       1        maximal timeout 1 s */
#define LPPRI           (PZERO+8)
#define BUFSIZE		PAGE_SIZE 

#define	LPTUNIT(s)	(minor(s) & 0x1f)
#define	LPTFLAGS(s)	(minor(s) & 0xe0)

/* Wait up to 16 seconds for a ready */
#define	LPT_TIMEOUT		((hz)*16)	
#define LPT_STEP		((hz)/4)

struct lpt_softc {
	struct ppbus_device_softc ppbus_dev;

#define LPT_OK 1
#define LPT_NOK 0
	u_int8_t sc_dev_ok;

	short sc_state;
/* bits for state */
#define OPEN            (unsigned)(1<<0)  /* device is open */
#define ASLP            (unsigned)(1<<1)  /* awaiting draining of printer */
#define EERROR          (unsigned)(1<<2)  /* error was received from printer */
#define OBUSY           (unsigned)(1<<3)  /* printer is busy doing output */
#define LPTOUT          (unsigned)(1<<4)  /* timeout while not selected */
#define TOUT            (unsigned)(1<<5)  /* timeout while not selected */
#define LPTINIT         (unsigned)(1<<6)  /* waiting to initialize for open */
#define INTERRUPTED     (unsigned)(1<<7)  /* write call was interrupted */
#define HAVEBUS         (unsigned)(1<<8)  /* the driver owns the bus */

	/* 
	 * default case: negative prime, negative ack, handshake strobe,
	 * prime once
	 */
	u_char  sc_control;
	char sc_flags;
#define	LPT_AUTOLF	0x20	/* automatic LF on CR */
#define	LPT_NOPRIME	0x40	/* don't prime on open */
#define	LPT_NOINTR	0x80	/* do not use interrupt */

	char * sc_inbuf;
	char * sc_outbuf;
	bus_addr_t sc_in_baddr;
	bus_addr_t sc_out_baddr;
	size_t sc_xfercnt;
	/* char sc_primed;
	char * sc_cp;*/
	u_short sc_irq;		/* IRQ status of port */
#define LP_HAS_IRQ      0x01    /* we have an irq available */
#define LP_USE_IRQ      0x02    /* we are using our irq */
#define LP_ENABLE_IRQ   0x04    /* enable IRQ on open */
#define LP_ENABLE_EXT   0x10    /* we shall use advanced mode when possible */
	u_char sc_backoff;	/* time to call lptout() again */

	struct resource * intr_resource; /* interrupt resource */
	void * intr_cookie;              /* interrupt registration cookie */
};

#define MAX_SLEEP       (hz*5)  /* Timeout while waiting for device ready */
#define MAX_SPIN        20      /* Max delay for device ready in usecs */

#ifdef _KERNEL

#ifdef LPT_DEBUG
static volatile int lptdebug = 1;
#ifndef LPT_DPRINTF
#define LPT_DPRINTF(arg) if(lptdebug) printf arg
#else
#define LPT_DPRINTF(arg) if(lptdebug) printf("WARNING: LPT_DPRINTF " \
	"REDEFINED!!!")
#endif /* LPT_DPRINTF */
#else
#define LPT_DPRINTF(arg)
#endif /* LPT_DEBUG */

#ifdef LPT_VERBOSE
static volatile int lptverbose = 1;
#ifndef LPT_VPRINTF
#define LPT_VPRINTF(arg) if(lptverbose) printf arg
#else
#define LPT_VPRINTF(arg) if(lptverbose) printf("WARNING: LPT_VPRINTF " \
	"REDEFINED!!!")
#endif /* LPT_VPRINTF */
#else
#define LPT_VPRINTF(arg)
#endif /* LPT_VERBOSE */

#endif /* _KERNEL */
#endif
