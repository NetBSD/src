/* Macro's to cope with the differences between NetBSD and FreeBSD
 */

/*
 * NetBSD
 */

#if defined(__NetBSD__)
#include "opt_usbverbose.h"

#define USBDEVNAME(bdev) ((bdev).dv_xname)

typedef struct device bdevice;			/* base device */

#define usb_timeout(f, d, t, h) timeout((f), (d), (t))
#define usb_untimeout(f, d, h) untimeout((f), (d))

#define USB_DECLARE_DRIVER_NAME_INIT(_1, dname, _2)  \
int __CONCAT(dname,_match) __P((struct device *, struct cfdata *, void *)); \
void __CONCAT(dname,_attach) __P((struct device *, struct device *, void *)); \
\
extern struct cfdriver __CONCAT(dname,_cd); \
\
struct cfattach __CONCAT(dname,_ca) = { \
	sizeof(struct __CONCAT(dname,_softc)), \
	__CONCAT(dname,_match), \
	__CONCAT(dname,_attach) \
}

#define USB_MATCH(dname) \
int \
__CONCAT(dname,_match)(parent, match, aux) \
	struct device *parent; \
	struct cfdata *match; \
	void *aux;

#define USB_MATCH_START(dname, uaa) \
	struct usb_attach_arg *uaa = aux

#define USB_ATTACH(dname) \
void \
__CONCAT(dname,_attach)(parent, self, aux) \
	struct device *parent; \
	struct device *self; \
	void *aux;

#define USB_ATTACH_START(dname, sc, uaa) \
	struct __CONCAT(dname,_softc) *sc = \
		(struct __CONCAT(dname,_softc) *)self; \
	struct usb_attach_arg *uaa = aux

/* Returns from attach */
#define USB_ATTACH_ERROR_RETURN	return
#define USB_ATTACH_SUCCESS_RETURN	return

#define USB_ATTACH_SETUP printf("\n")

#define USB_GET_SC_OPEN(dname, unit, sc) \
	struct __CONCAT(dname,_softc) *sc; \
	if (unit >= __CONCAT(dname,_cd).cd_ndevs) \
		return (ENXIO); \
	sc = __CONCAT(dname,_cd).cd_devs[unit]; \
	if (!sc) \
		return (ENXIO)

#define USB_GET_SC(dname, unit, sc) \
	struct __CONCAT(dname,_softc) *sc = __CONCAT(dname,_cd).cd_devs[unit]

#define USB_DO_ATTACH(dev, bdev, parent, args, print, sub) \
	((dev)->softc = config_found_sm(parent, args, print, sub))

#elif defined(__FreeBSD__)
/*
 * FreeBSD
 */

#include "opt_usb.h"
char *usb_devname(struct device *);
#define USBDEVNAME(bdev) usb_devname(&bdev)

/* XXX Change this when FreeBSD has memset
 */
#define memset(d, v, s)	\
		do{			\
		if ((v) == 0)		\
			bzero((d), (s));	\
		else			\
			panic("Non zero filler for memset, cannot handle!"); \
		} while (0)

typedef device_t bdevice;

/* 
 * To avoid race conditions we first initialise the struct
 * before we use it. The timeout might happen between the
 * setting of the timeout and the setting of timo_handle
 */
#define usb_timeout(f, d, t, h) \
	(callout_handle_init(&(h)), (h) = timeout((f), (d), (t)))
#define usb_untimeout(f, d, h) untimeout((f), (d), (h))

#define USB_DECLARE_DRIVER_NAME_INIT(name, dname, init) \
static device_probe_t __CONCAT(dname,_match); \
static device_attach_t __CONCAT(dname,_attach); \
static device_detach_t __CONCAT(dname,_detach); \
\
static devclass_t __CONCAT(dname,_devclass); \
\
static device_method_t __CONCAT(dname,_methods)[] = { \
        DEVMETHOD(device_probe, __CONCAT(dname,_match)), \
        DEVMETHOD(device_attach, __CONCAT(dname,_attach)), \
        DEVMETHOD(device_detach, __CONCAT(dname,_detach)), \
	init, \
        {0,0} \
}; \
\
static driver_t __CONCAT(dname,_driver) = { \
        name, \
        __CONCAT(dname,_methods), \
        DRIVER_TYPE_MISC, \
        sizeof(struct __CONCAT(dname,_softc)) \
}

#define USB_MATCH(dname) \
static int \
__CONCAT(dname,_match)(device_t device)

#define USB_MATCH_START(dname, uaa) \
        struct usb_attach_arg *uaa = device_get_ivars(device)

#define USB_ATTACH(dname) \
static int
__CONCAT(dname,_attach)(device_t self)

#define USB_ATTACH_START(dname, sc, uaa) \
        struct __CONCAT(dname,_softc) *sc = device_get_softc(self); \
        struct usb_attach_arg *uaa = device_get_ivars(self)

/* Returns from attach */
#define USB_ATTACH_ERROR_RETURN	return ENXIO
#define USB_ATTACH_SUCCESS_RETURN	return 0

#define USB_ATTACH_SETUP \
	usb_device_set_desc(self, devinfo); \
	sc->sc_dev = self

#define USB_GET_SC_OPEN(dname, unit, sc) \
	struct __CONCAT(dname,_softc) *sc = \
		devclass_get_softc(__CONCAT(dname,_devclass), unit);
	if (!sc)
		return (ENXIO)

#define USB_GET_SC(dname, unit, sc) \
	struct __CONCAT(dname,_softc) *sc = \
		devclass_get_softc(__CONCAT(dname,_devclass), unit)

#define USB_DO_ATTACH(dev, bdev, parent, args, print, sub) \
	(device_probe_and_attach((bdev)) == 0 ? ((dev)->softc = (bdev)) : 0)

/* conversion from one type of queue to the other */
#define SIMPLEQ_REMOVE_HEAD	STAILQ_REMOVE_HEAD_QUEUE
#define SIMPLEQ_INSERT_HEAD	STAILQ_INSERT_HEAD
#define SIMPLEQ_INSERT_TAIL	STAILQ_INSERT_TAIL
#define SIMPLEQ_NEXT		STAILQ_NEXT
#define SIMPLEQ_FIRST		STAILQ_FIRST
#define SIMPLEQ_HEAD		STAILQ_HEAD
#define SIMPLEQ_INIT		STAILQ_INIT
#define SIMPLEQ_ENTRY		STAILQ_ENTRY

#endif /* __FreeBSD */

#define NONE {0,0}

#define USB_DECLARE_DRIVER_NAME(name, dname) \
	USB_DECLARE_DRIVER_NAME_INIT(name, dname, NONE )
#define USB_DECLARE_DRIVER_INIT(dname, init) \
	USB_DECLARE_DRIVER_NAME_INIT(##dname, dname, init)
#define USB_DECLARE_DRIVER(dname) \
	USB_DECLARE_DRIVER_NAME_INIT(##dname, dname, NONE )

#undef NONE
