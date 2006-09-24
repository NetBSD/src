/*	$NetBSD: fw_port.h,v 1.17 2006/09/24 03:53:08 jmcneill Exp $	*/
/*
 * Copyright (c) 2004 KIYOHARA Takashi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#ifndef _FW_PORT_H
#define _FW_PORT_H

#ifdef _KERNEL

#if defined(__FreeBSD__)
#ifdef __DragonFly__
#define OS_STR			"DragonFly"
#define OS_VER			__DragonFly_cc_version
#define OS_VER_STR		"DragonFly-1"
#define PROJECT_STR		"DragonFly Project"
#else
#define OS_STR			"FreeBSD"
#define OS_VER			__FreeBSD_version
#define OS_VER_STR		"FreeBSD-5"
#define PROJECT_STR		"FreeBSD Project"
#endif

#if defined(__DragonFly__) || __FreeBSD_version < 500000
#define dev2unit(x)	((minor(x) & 0xff) | (minor(x) >> 8))
#define unit2minor(x)	(((x) & 0xff) | (((x) << 8) & ~0xffff))
#endif

#ifdef __DragonFly__
typedef d_thread_t fw_proc;
typedef d_thread_t fw_thread;
#include <sys/select.h>
#define M_DONTWAIT MB_DONTWAIT
#elif __FreeBSD_version >= 500000
typedef struct thread fw_proc;
typedef struct thread fw_thread;
#include <sys/selinfo.h>
#else
typedef struct proc fw_thread;
typedef struct proc fw_proc;
#include <sys/select.h>
#endif


#if defined(__DragonFly__) || __FreeBSD_version < 500000
#define CALLOUT_INIT(x) callout_init(x)
#define DEV_T dev_t
#define FW_LOCK
#define FW_UNLOCK
#define THREAD_CREATE(f, sc, p, name, arg) \
     kthread_create(f, (void *)sc, p, name, arg)
#define THREAD_EXIT(x)  kthread_exit()
#else
#define CALLOUT_INIT(x) callout_init(x, 0 /* mpsafe */)
#define DEV_T struct cdev *
#define FW_LOCK         mtx_lock(&Giant)
#define FW_UNLOCK       mtx_unlock(&Giant)
#define THREAD_CREATE(f, sc, p, name, arg) \
     kthread_create(f, (void *)sc, p, 0, 0, name, arg)
#define THREAD_EXIT(x)  kthread_exit(x)
#endif
#define fw_kthread_create(func, arg) \
				func((arg))

/*
 * fw attach macro for FreeBSD
 */
#define FW_ATTACH(dname) 	\
	static int		\
	__CONCAT(dname,_attach)(device_t dev)
#define FW_ATTACH_START(dname, sc, fwa)					\
	struct __CONCAT(dname,_softc) *sc =				\
	    ((struct __CONCAT(dname,_softc) *)device_get_softc(dev));	\
	__attribute__((__unused__))struct fw_attach_args *fwa =		\
	    device_get_ivars(dev)
#define FW_ATTACH_RETURN(r)	return (r)

/*
 * fw detach macro for FreeBSD
 */
#define FW_DETACH(dname)	\
	static int		\
	__CONCAT(dname,_detach)(device_t dev)
#define FW_DETACH_START(dname, sc)					\
	struct __CONCAT(dname,_softc) *sc =				\
	    ((struct __CONCAT(dname,_softc) *)device_get_softc(dev))

/*
 * fw intr macro for FreeBSD
 */
#define FW_INTR(fwohci)	\
	void		\
	fwohci_intr(void *arg)
#define FW_INTR_RETURN(r)	return

/*
 * fw open macro for FreeBSD
 */
#define FW_OPEN(dname)	\
	int		\
	__CONCAT(dname,_open)(DEV_T dev, int flags, int fmt, fw_proc *td)
#define FW_OPEN_START			\
	int unit = DEV2UNIT(dev);	\
	__attribute__((__unused__))struct firewire_softc *sc = \
	    devclass_get_softc(firewire_devclass, unit)

/*
 * fw close macro for FreeBSD
 */
#define FW_CLOSE(dname)		\
	int			\
	__CONCAT(dname,_close)(DEV_T dev, int flags, int fmt, fw_proc *td)
#define FW_CLOSE_START

/*
 * fw read macro for FreeBSD
 */
#define FW_READ(dname)	\
	int		\
	__CONCAT(dname,_read)(DEV_T dev, struct uio *uio, int ioflag) 
#define FW_READ_START

/*
 * fw write macro for FreeBSD
 */
#define FW_WRITE(dname)	\
	int		\
	__CONCAT(dname,_write)(DEV_T dev, struct uio *uio, int ioflag)
#define FW_WRITE_START

/*
 * fw ioctl macro for FreeBSD
 */
#define FW_IOCTL(dname)					\
	int						\
	__CONCAT(dname,_ioctl)				\
	    (DEV_T dev, u_long cmd, caddr_t data, int flag, fw_proc *td)
#define FW_IOCTL_START			\
	int unit = DEV2UNIT(dev);       \
	__attribute__((__unused__))struct firewire_softc *sc = \
	    devclass_get_softc(firewire_devclass, unit)


/*
 * fw poll macro for FreeBSD
 */
#define FW_POLL(dname)	\
	int		\
	__CONCAT(dname,_poll)(DEV_T dev, int events, fw_proc *td)
#define FW_POLL_START

/*
 * fw mmap macro for FreeBSD
 */
#if defined(__DragonFly__) || __FreeBSD_version < 500102
#define FW_MMAP(dname)	\
	int		\
	__CONCAT(dname,_mmap)(DEV_T dev, vm_offset_t offset, int nproto)
#else
#define FW_MMAP(dname)		\
	int			\
	__CONCAT(dname,_mmap)	\
	   (DEV_T dev, vm_offset_t offset, vm_paddr_t *paddr, int nproto)
#endif
#define FW_MMAP_START

/*
 * fw strategy macro for FreeBSD
 */
#define FW_STRATEGY_START		\
	DEV_T dev = bp->bio_dev;	\
	int unit = DEV2UNIT(dev);	\
	__attribute__((__unused__))struct firewire_softc *sc = \
	    devclass_get_softc(firewire_devclass, unit)

/*
 * if macro for FreeBSD
 */
#define IF_STOP(dname)	\
	static void	\
	__CONCAT(dname,_stop)(struct __CONCAT(dname,_softc) *fwip)
#define IF_STOP_START(dname, ifp, sc) \
	struct ifnet *ifp = &(sc)->fwip_if
#define IF_DETACH_START(dname, sc)		\
	struct __CONCAT(dname,_softc) *sc =	\
	    (struct __CONCAT(dname,_softc) *)device_get_softc(dev)
#define IF_INIT(dname)	\
	static void	\
	__CONCAT(dname,_init)(void *arg)
#define IF_INIT_START(dname, sc, ifp)			\
	struct __CONCAT(dname,_softc) *sc =		\
	    ((struct fwip_eth_softc *)arg)->fwip;	\
	struct ifnet *ifp = &(sc)->fwip_if
#define IF_INIT_RETURN(r)	return
#define IF_IOCTL_START(dname, sc)		\
	struct __CONCAT(dname,_softc) *sc =	\
	    ((struct fwip_eth_softc *)ifp->if_softc)->fwip

/*
 * fwohci macro for FreeBSD
 */
#define FWOHCI_INIT_END
#define FWOHCI_DETACH()	\
	int		\
	fwohci_detach(struct fwohci_softc *sc, device_t dev)
#define FWOHCI_DETACH_START
#define FWOHCI_DETACH_END
#define FWOHCI_STOP()	\
	int		\
	fwohci_stop(struct fwohci_softc *sc, device_t dev)
#define FWOHCI_STOP_START
#define FWOHCI_STOP_RETURN(r)	return (r) 

/*
 * firewire macro for FreeBSD
 */
#define FIREWIRE_ATTACH_START		 	\
	device_t pa = device_get_parent(dev);	\
	struct firewire_comm *fc = (struct firewire_comm *)device_get_softc(pa)
#define FWDEV_DESTROYDEV(sc)					\
	do {							\
		int err;					\
		if ((err = fwdev_destroydev((sc))) != 0)	\
			return err;				\
	} while (/*CONSTCOND*/0)
#define FIREWIRE_GENERIC_ATTACH					\
	do {							\
		/* Locate our children */			\
		bus_generic_probe(dev);				\
								\
		/* launch attachement of the added children */	\
		bus_generic_attach(dev);			\
	} while (/*CONSTCOND*/0)
#define FIREWIRE_GENERIC_DETACH					\
	do {							\
		int err;					\
		if ((err = bus_generic_detach(dev)) != 0)	\
			return err;				\
	} while (/*CONSTCOND*/0)
#define FIREWIRE_SBP_ATTACH						\
	do {								\
		fwa.fwdev = fwdev;					\
		fwdev->sbp = device_add_child(fc->bdev, fwa.name, -1);	\
		if (fwdev->sbp) {					\
			device_set_ivars(fwdev->sbp, &fwa);		\
			device_probe_and_attach(fwdev->sbp);		\
		}							\
	} while (/*CONSTCOND*/0)
#define FIREWIRE_SBP_DETACH				\
	do {						\
		if (device_detach(fwdev->sbp) != 0)	\
			return;				\
	} while (/*CONSTCOND*/0)
#define FIREWIRE_CHILDREN_FOREACH_FUNC(func, fdc)			      \
	do {								      \
		device_t *devlistp;					      \
		int i, devcnt;						      \
									      \
		if (device_get_children(fc->bdev, &devlistp, &devcnt) == 0) { \
			for( i = 0 ; i < devcnt ; i++)			      \
				if (device_get_state(devlistp[i]) >=	      \
				    DS_ATTACHED)  {			      \
					(fdc) = device_get_softc(devlistp[i]);\
					if ((fdc)->func != NULL)	      \
						(fdc)->func((fdc));	      \
				}					      \
			free(devlistp, M_TEMP);				      \
		}							      \
	} while (/*CONSTCOND*/0)

/*
 * sbp macro for FreeBSD
 */
#define SBP_ATTACH_START          		\
        struct cam_devq *devq;			\
						\
        bzero(sbp, sizeof(struct sbp_softc));	\
	sbp->fd.dev = dev
#define SBP_SCSIBUS_ATTACH						    \
	do {								    \
		sbp->sim = cam_sim_alloc(sbp_action, sbp_poll, "sbp", sbp,  \
				 device_get_unit(dev),			    \
				 /*untagged*/ 1,			    \
				 /*tagged*/ SBP_QUEUE_LEN - 1,		    \
				 devq);					    \
									    \
		if (sbp->sim == NULL) {					    \
			cam_simq_free(devq);				    \
			return ENXIO;					    \
		}							    \
									    \
		if (xpt_bus_register(sbp->sim, /*bus*/0) != CAM_SUCCESS)    \
			goto fail;					    \
									    \
		if (xpt_create_path(					    \
		    &sbp->path, xpt_periph, cam_sim_path(sbp->sim),	    \
		    CAM_TARGET_WILDCARD, CAM_LUN_WILDCARD) != CAM_REQ_CMP) {\
			xpt_bus_deregister(cam_sim_path(sbp->sim));	    \
			goto fail;					    \
		}							    \
		xpt_async(AC_BUS_RESET, sbp->path, /*arg*/ NULL);	    \
	} while (/*CONSTCOND*/0)
#define SBP_DEVICE(d)		((d)->path)
#define SBP_DEVICE_FREEZE(d, x)	xpt_freeze_devq((d)->path, (x))
#define SBP_DEVICE_THAW(d, x)	xpt_release_devq((d)->path, (x), TRUE)
#define SBP_BUS_FREEZE(b)					\
	do {							\
		if (((b)->sim->flags & SIMQ_FREEZED) == 0) {	\
			xpt_freeze_simq((b)->sim, /*count*/1);	\
			(b)->sim->flags |= SIMQ_FREEZED;	\
		}						\
	} while (/*CONSTCOND*/0)
#define SBP_BUS_THAW(b)						\
	do {							\
		xpt_release_simq((b)->sim, /*run queue*/TRUE);	\
		(b)->sim->flags &= ~SIMQ_FREEZED;		\
	} while (/*CONSTCOND*/0)
#define SBP_DEVICE_PREATTACH()

/*
 * fwip macro for FreeBSD
 */
#define FWIP_ATTACH_START						\
	int unit = device_get_unit(dev);				\
	struct fw_hwaddr *hwaddr = &fwip->fw_softc.fwcom.fc_hwaddr
#define FWIP_ATTACH_SETUP	bzero(fwip, sizeof(struct fwip_softc))

#define FWDEV_MAKEDEV(sc)	fwdev_makedev(sc)

#define FIREWIRE_IFATTACH(ifp, ha) \
				firewire_ifattach((ifp), (ha))
#define FIREWIRE_IFDETACH(ifp)	firewire_ifdetach((ifp));
#define FIREWIRE_BUSRESET(ifp)	firewire_busreset((ifp))
#define FIREWIRE_INPUT(ifp, m, src) \
				firewire_input((ifp), (m), (src))
#define	FWIP_INIT(sc)		fwip_init(&(sc)->fw_softc)
#define	FWIP_STOP(sc)		fwip_stop((sc))
#define FIREWIRE_IOCTL(ifp, cmd, data) \
				firewire_ioctl((ifp), (cmd), (data))
#define IF_INITNAME(ifp, dev, unit)	\
	if_initname((ifp), device_get_name((dev)), (unit));
#define SET_IFFUNC(ifp, start, ioctl, init, stop)	\
	do {						\
		(ifp)->if_start = (start);		\
		(ifp)->if_ioctl = (ioctl);		\
		(ifp)->if_init = (init);		\
	} while (/*CONSTCOND*/0)

/*
 * fwdev macro for FreeBSD
 */
#define FWDEV_OPEN_START	\
	if (DEV_FWMEM(dev))	\
		return fwmem_open(dev, flags, fmt, td)
#define FWDEV_CLOSE_START	\
        if (DEV_FWMEM(dev))	\
		return fwmem_close(dev, flags, fmt, td)
#define FWDEV_READ_START	\
        if (DEV_FWMEM(dev))	\
		return physio(dev, uio, ioflag)
#define FWDEV_WRITE_START	\
        if (DEV_FWMEM(dev))	\
		return physio(dev, uio, ioflag)
#define FWDEV_IOCTL_START	\
	if (DEV_FWMEM(dev))	\
		return fwmem_ioctl(dev, cmd, data, flag, td)
#define FWDEV_IOCTL_REDIRECT	fc->ioctl (dev, cmd, data, flag, td)
#define FWDEV_POLL_START	\
	if (DEV_FWMEM(dev))	\
		return fwmem_poll(dev, events, td)
#if defined(__DragonFly__) || __FreeBSD_version < 500102
#define FWDEV_MMAP_START	\
        if (DEV_FWMEM(dev)) 	\
		return fwmem_mmap(dev, offset, nproto)
#else
#define FWDEV_MMAP_START	\
        if (DEV_FWMEM(dev)) 	\
		return fwmem_mmap(dev, offset, paddr, nproto)
#endif
#define FWDEV_STRATEGY_START		\
	if (DEV_FWMEM(dev)) {		\
		fwmem_strategy(bp);	\
		return;			\
	}

#define XS_REQ_INVALID		CAM_REQ_INVALID
#define XS_SCSI_BUS_RESET	CAM_SCSI_BUS_RESET
#define XS_BDR_SENT		CAM_BDR_SENT
#define XS_DEV_NOT_THERE	CAM_DEV_NOT_THERE
#define XS_CMD_TIMEOUT		CAM_CMD_TIMEOUT
#define XS_REQUEUE_REQ		CAM_REQUEUE_REQ
#define XS_REQ_CMP		CAM_REQ_CMP
#define XS_REQ_CMP_ERR		CAM_REQ_CMP_ERR
#define XS_UA_ABORT		CAM_UA_ABORT
#define XS_SENSE		(CAM_SCSI_STATUS_ERROR | CAM_AUTOSNS_VALID)

typedef union ccb sbp_scsi_xfer;
typedef struct scsi_inquiry_data sbp_scsi_inquiry_data;

#define SCSI_XFER_TARGET(x)	((x)->ccb_h.target_id)
#define SCSI_XFER_LUN(x)	((x)->ccb_h.target_lun)
#define SCSI_XFER_ERROR(x)	((x)->ccb_h.status)
#define SCSI_XFER_DIR(x)	((x)->ccb_h.flags & CAM_DIR_MASK)
#define     SCSI_XFER_DATA_IN	CAM_DIR_IN
#define SCSI_XFER_CALLOUT(x)	((x)->ccb_h.timeout_ch)
#define SCSI_XFER_TIMEOUT(x)	((x)->ccb_h.timeout)
#define SCSI_XFER_OPECODE(x)	((x)->csio.cdb_io.cdb_bytes[0])
#define SCSI_XFER_STATUS(x)	((x)->csio.scsi_status)
#define SCSI_XFER_EVPD(x)	((x)->csio.cdb_io.cdb_bytes[1] & SI_EVPD)
#define SCSI_XFER_CMDLEN(x)	((x)->csio.cdb_len)
#define SCSI_XFER_CMD(x)			\
	(((x)->csio.ccb_h.flags & CAM_CDB_POINTER) ?\
	    (void *)(x)->csio.cdb_io.cdb_ptr : \
	    (void *)&(x)->csio.cdb_io.cdb_bytes)
#define SCSI_XFER_DATALEN(x)	((x)->csio.dxfer_len)
#define SCSI_XFER_DATA(x)	((x)->csio.data_ptr)
#define SCSI_XFER_SENSELEN(x)	((x)->csio.sense_len)
#define SCSI_SENSE_DATA(x)	(&(x)->csio.sense_data)
#define SCSI_INQUIRY_DATA(x)	((x)->csio.data_ptr)
#define SCSI_XFER_FUNCCODE(x)	((x)->ccb_h.func_code)
#define SCSI_XFER_10BCMD_DUMP(x) \
	((x)->csio.cdb_io.cdb_bytes[0]),	\
	((x)->csio.cdb_io.cdb_bytes[1]),	\
	((x)->csio.cdb_io.cdb_bytes[2]),	\
	((x)->csio.cdb_io.cdb_bytes[3]),	\
	((x)->csio.cdb_io.cdb_bytes[4]),	\
	((x)->csio.cdb_io.cdb_bytes[5]),	\
	((x)->csio.cdb_io.cdb_bytes[6]),	\
	((x)->csio.cdb_io.cdb_bytes[7]),	\
	((x)->csio.cdb_io.cdb_bytes[8]), 	\
	((x)->csio.cdb_io.cdb_bytes[9])
#define SCSI_XFER_REQUEST_COMPLETE(x)
#define SCSI_TRANSFER_DONE(x)	xpt_done((x))
#ifdef SID_TYPE
#undef SID_TYPE
#define SID_TYPE 0x1f
#endif

#define NOT_LUN_WILDCARD(l)	((l) != CAM_LUN_WILDCARD)
#define CAM_XFER_FLAGS(x)	((x)->csio.ccb_h.flags)

#define mstohz(ms) \
	(__predict_false((ms) >= 0x20000) ? \
	    (((ms) + 0u) / 1000u) * hz : \
	    (((ms) + 0u) * hz) / 1000u)

#define config_pending_incr()
#define config_pending_decr()

/*
 * bus_dma macros for FreeBSD
 */
typedef bus_dma_tag_t fw_bus_dma_tag_t;

#if defined(__FreeBSD__) && __FreeBSD_version >= 501102
#define fw_bus_dma_tag_create(t,					     \
	    a, b, laddr, haddr, ffunc, farg, s, ns, mxss, f, lfunc, larg, tp)\
	bus_dma_tag_create((t), (a), (b), (laddr), (haddr),		     \
	    (ffunc), (farg), (s), (ns), (mxss), (f), (lfunc), (larg), (tp))
#else
#define fw_bus_dma_tag_create(t, a, b,					\
	    laddr, haddr, ffunc, farg, s, ns, mxss, f, lfunc, larg, tp)	\
	bus_dma_tag_create((t), (a), (b), (laddr), (haddr),		\
	    (ffunc), (farg), (s), (ns), (mxss), (f), (tp))
#endif
#define fw_bus_dma_tag_destroy(t) \
	bus_dma_tag_destroy((t))
#define fw_bus_dmamap_create(t, f, mp) \
	bus_dmamap_create((t), 0, (mp))
#define fw_bus_dmamap_destroy((t), (m)) \
	bus_dmamap_destroy((t), (m))
#define fw_bus_dmamap_load(t, m, b, l, func, a, f) \
	bus_dmamap_load((t), (m), (b), (l), (func), (a), 0)
#define fw_bus_dmamap_load_mbuf(t, m, b, func, a, f) \
	bus_dmamap_load((t), (m), (b), (func), (a), 0)
#define fw_bus_dmamap_unload(t, m) \
	bus_dmamap_unload((t), (m))
#if __FreeBSD_version < 500000
#define fw_bus_dmamap_sync(t, m, op)					\
	do {								\
		switch ((op)) {						\
		(BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE):		\
			bus_dmamap_sync((t), (m), BUS_DMASYNC_PREWRITE);\
			bus_dmamap_sync((t), (m), BUS_DMASYNC_PREREAD);	\
			break;						\
		(BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE):		\
			/* BUS_DMASYNC_POSTWRITE is probably a no-op. */\
			bus_dmamap_sync((t), (m), BUS_DMASYNC_POSTREAD);\
			break;						\
		default:						\
			bus_dmamap_sync((t), (m), (op));		\
		}							\
	} while (/*CONSTCOND*/0)
#else
#define fw_bus_dmamap_sync(t, m, op) \
	bus_dmamap_sync((t), (m), (op))
#endif
#define fw_bus_dmamem_alloc(t, vp, f, mp) \
	bus_dmamem_alloc((t), (vp), (f), (mp))
#define fw_bus_dmamem_free(t, v, m) \
        bus_dmamem_free((t), (v), (m))

#define splfw()		splimp()
#define splfwnet()	splimp()
#define splfwsbp()	splcam()


#elif defined(__NetBSD__)
#define OS_STR			"NetBSD"
#define OS_VER			__NetBSD_Version__
#define OS_VER_STR		"NetBSD-2"
#define PROJECT_STR		"NetBSD Project"

#define SSD_CURRENT_ERROR	0x70
#define SSD_DEFERRED_ERROR	0x71

#define T_RBC				T_SIMPLE_DIRECT

#define SCSI_STATUS_CHECK_COND		SCSI_CHECK
#define SCSI_STATUS_BUSY		SCSI_BUSY
#define SCSI_STATUS_CMD_TERMINATED	SCSI_TERMINATED

#define GIANT_REQUIRED
#define IFF_NEEDSGIANT	0

#define MTAG_FIREWIRE			1394
#define MTAG_FIREWIRE_HWADDR		0
#define MTAG_FIREWIRE_SENDER_EUID	1

#define BUS_SPACE_MAXSIZE_32BIT		0xFFFFFFFF

#define DFLTPHYS			(64 * 1024)	/* fake */

#define dev2unit	minor
#define unit2minor(x)	(((x) & 0xff) | (((x) << 12) & ~0xfffff)) /* XXX */

typedef struct lwp fw_proc;
typedef struct proc fw_thread;
#include <sys/select.h>

#define CALLOUT_INIT(x) callout_init(x)
#define DEV_T dev_t
#define FW_LOCK
#define FW_UNLOCK
#define THREAD_CREATE(f, sc, p, name, arg) \
     kthread_create1(f, (void *)sc, p, name, arg)
#define THREAD_EXIT(x)  kthread_exit(x)
#define fw_kthread_create(func, arg) \
				kthread_create((func), (arg))

struct fwbus_attach_args {
	const char *name;
};


/*
 * fw attach macro for NetBSD
 */
#define FW_ATTACH(dname) 	\
	void			\
	__CONCAT(dname,attach)	\
	    (struct device *parent, struct device *self, void *aux)
#define FW_ATTACH_START(dname, sc, fwa)					\
	struct __CONCAT(dname,_softc) *sc =				\
	    (struct __CONCAT(dname,_softc) *)self;			\
	__attribute__((__unused__))struct fw_attach_args *fwa =		\
	    (struct fw_attach_args *)aux
#define FW_ATTACH_RETURN(r)	return

/*
 * fw detach macro for NetBSD
 */
#define FW_DETACH(dname)	\
	int			\
	__CONCAT(dname,detach)(struct device *self, int flags)
#define FW_DETACH_START(dname, sc)					\
	struct __CONCAT(dname,_softc) *sc =				\
	    (struct __CONCAT(dname,_softc) *)self

/*
 * fw intr macro for NetBSD
 */
#define FW_INTR(fwohci)	\
	int		\
	fwohci_intr(void *arg)
#define FW_INTR_RETURN(r)	return (r)

/*
 * fw open macro for NetBSD
 */
#define FW_OPEN(dname)	\
	int		\
	__CONCAT(dname,_open)(dev_t _dev, int flags, int fmt, fw_proc *td)
#define FW_OPEN_START							\
	struct firewire_softc *sc, *dev;				\
									\
	sc = dev = device_lookup(&ieee1394if_cd, DEV2UNIT(_dev));	\
	if (dev == NULL)						\
		return ENXIO

/*
 * fw close macro for NetBSD
 */
#define FW_CLOSE(dname)		\
	int			\
	__CONCAT(dname,_close)(dev_t _dev, int flags, int fmt, fw_proc *td)
#define FW_CLOSE_START							  \
	int unit = DEV2UNIT(_dev);					  \
	struct firewire_softc *dev = device_lookup(&ieee1394if_cd, unit); \
									  \
	if (dev == NULL)						  \
		return ENXIO

/*
 * fw read macro for NetBSD
 */
#define FW_READ(dname)	\
	int		\
	__CONCAT(dname,_read)(dev_t _dev, struct uio *uio, int ioflag) 
#define FW_READ_START					\
	int unit = DEV2UNIT(_dev);			\
	struct firewire_softc *dev;			\
							\
	dev = device_lookup(&ieee1394if_cd, unit);	\
	if (dev == NULL)				\
		return ENXIO

/*
 * fw write macro for NetBSD
 */
#define FW_WRITE(dname)	\
	int		\
	__CONCAT(dname,_write)(dev_t _dev, struct uio *uio, int ioflag)
#define FW_WRITE_START					\
	int unit = DEV2UNIT(_dev);			\
	struct firewire_softc *dev;			\
							\
	dev = device_lookup(&ieee1394if_cd, unit);	\
	if (dev == NULL)				\
		return ENXIO

/*
 * fw ioctl macro for NetBSD
 */
#define FW_IOCTL(dname)					\
	int						\
	__CONCAT(dname,_ioctl)				\
	    (dev_t _dev, u_long cmd, caddr_t data, int flag, fw_proc *td)
#define FW_IOCTL_START					\
	int unit = DEV2UNIT(_dev);			\
	struct firewire_softc *sc, *dev;		\
							\
	sc = dev = device_lookup(&ieee1394if_cd, unit);	\
	if (dev == NULL)				\
		return ENXIO

/*
 * fw poll macro for NetBSD
 */
#define FW_POLL(dname)	\
	int		\
	__CONCAT(dname,_poll)(dev_t _dev, int events, fw_proc *td)
#define FW_POLL_START					\
	int unit = DEV2UNIT(_dev);			\
	struct firewire_softc *dev;			\
							\
	dev = device_lookup(&ieee1394if_cd, unit);	\
	if (dev == NULL)				\
		return ENXIO

/*
 * fw mmap macro for NetBSD
 */
#define FW_MMAP(dname)	\
	paddr_t		\
	__CONCAT(dname,_mmap)(dev_t _dev, off_t offset, int nproto)
#define FW_MMAP_START					\
	int unit = DEV2UNIT(_dev);			\
	struct firewire_softc *dev;			\
							\
	dev = device_lookup(&ieee1394if_cd, unit);	\
	if (dev == NULL)				\
		return ENXIO

/*
 * fw strategy macro for NetBSD
 */
#define FW_STRATEGY_START				\
	dev_t _dev = bp->bio_dev;			\
	int unit = DEV2UNIT(_dev);			\
	struct firewire_softc *sc, *dev;		\
							\
	sc = dev = device_lookup(&ieee1394if_cd, unit);	\
	if (dev == NULL)				\
		return

/*
 * if macro for NetBSD
 */
#define IF_STOP(dname)	\
	void		\
	__CONCAT(dname,_stop)(struct ifnet *ifp, int disable)
#define IF_STOP_START(dname, ifp, sc)		\
	struct __CONCAT(dname,_softc) *sc =	\
	    ((struct fwip_eth_softc *)(ifp)->if_softc)->fwip
#define IF_DETACH_START(dname, sc)		\
	struct __CONCAT(dname,_softc) *sc =	\
	    (struct __CONCAT(dname,_softc) *)self
#define IF_INIT(dname)	\
	int		\
	__CONCAT(dname,_init)(struct ifnet *ifp)
#define IF_INIT_START(dname, sc, ifp)	\
	struct __CONCAT(dname,_softc) *sc =	\
	    ((struct fwip_eth_softc *)ifp->if_softc)->fwip
#define IF_INIT_RETURN(r)	return (r)
#define IF_IOCTL_START(dname, sc)		\
	struct __CONCAT(dname,_softc) *sc =	\
	    ((struct fwip_eth_softc *)ifp->if_softc)->fwip

/*
 * fwohci macro for NetBSD
 */
#define FWOHCI_INIT_END							      \
	do {								      \
		struct fwbus_attach_args faa;				      \
		faa.name = "ieee1394if";				      \
		sc->sc_shutdownhook = shutdownhook_establish(fwohci_stop, sc);\
		sc->sc_powerhook = powerhook_establish(sc->fc._dev.dv_xname,  \
		    fwohci_power, sc);					      \
		sc->fc.bdev = config_found(sc->fc.dev, &faa, fwohci_print);   \
	} while (/*CONSTCOND*/0)
#define FWOHCI_DETACH()	\
	int		\
	fwohci_detach(struct fwohci_softc *sc, int flags)
#define FWOHCI_DETACH_START		\
	if (sc->fc.bdev != NULL)	\
		config_detach(sc->fc.bdev, flags) 
#define FWOHCI_DETACH_END					\
        if (sc->sc_powerhook != NULL)				\
		powerhook_disestablish(sc->sc_powerhook);	\
	if (sc->sc_shutdownhook != NULL)			\
		shutdownhook_disestablish(sc->sc_shutdownhook)
#define FWOHCI_STOP()	\
	void	\
	fwohci_stop(void *arg)
#define FWOHCI_STOP_START	struct fwohci_softc *sc = arg
#define FWOHCI_STOP_RETURN(r)	return

/*
 * firewire macro for NetBSD
 */
#define FIREWIRE_ATTACH_START						\
	struct firewire_comm *fc = (struct firewire_comm *)parent;	\
									\
	aprint_normal(": IEEE1394 bus\n");				\
									\
	fc->bdev = (struct device *)sc;					\
	sc->dev = &sc->_dev;						\
	SLIST_INIT(&sc->devlist)
#define FWDEV_DESTROYDEV(sc)
#define FIREWIRE_GENERIC_ATTACH						    \
	do {								    \
		struct fw_attach_args faa;				    \
		struct firewire_dev_list *devlist, *elm;		    \
									    \
		devlist = malloc(					    \
		    sizeof (struct firewire_dev_list), M_DEVBUF, M_NOWAIT); \
		if (devlist == NULL)					    \
			break;						    \
									    \
		faa.name = "fwip";					    \
		faa.fc = fc;						    \
		faa.fwdev = NULL;					    \
		devlist->dev = config_found(sc->dev, &faa, firewire_print); \
		if (devlist->dev == NULL) {				    \
			free(devlist, M_DEVBUF);			    \
			break;						    \
		}							    \
									    \
		if (SLIST_EMPTY(&sc->devlist))				    \
			SLIST_INSERT_HEAD(&sc->devlist, devlist, link);	    \
		else {							    \
			for (elm = SLIST_FIRST(&sc->devlist);		    \
			    SLIST_NEXT(elm, link) != NULL;		    \
			    elm = SLIST_NEXT(elm, link));		    \
			SLIST_INSERT_AFTER(elm, devlist, link);		    \
		}							    \
	} while (/*CONSTCOND*/0)
#define FIREWIRE_GENERIC_DETACH						      \
	do {								      \
		struct firewire_dev_list *devlist;			      \
		int err;						      \
									      \
		while ((devlist = SLIST_FIRST(&sc->devlist)) != NULL) {	      \
			if ((err = config_detach(devlist->dev, flags)) != 0)  \
				return err;				      \
			SLIST_REMOVE(					      \
			    &sc->devlist, devlist, firewire_dev_list, link);  \
			free(devlist, M_DEVBUF);			      \
		}							      \
	} while (/*CONSTCOND*/0)
#define FIREWIRE_SBP_ATTACH						      \
	do {								      \
		struct firewire_softc *sc = (struct firewire_softc *)fc->bdev;\
		struct firewire_dev_list *devlist, *elm;		      \
		int locs[IEEE1394IFCF_NLOCS];				      \
									      \
		devlist = malloc(					      \
		    sizeof (struct firewire_dev_list), M_DEVBUF, M_NOWAIT);   \
		if (devlist == NULL) {					      \
			printf("memory allocation failed\n");		      \
			break;						      \
		}							      \
									      \
		locs[IEEE1394IFCF_EUIHI] = fwdev->eui.hi;		      \
		locs[IEEE1394IFCF_EUILO] = fwdev->eui.lo;		      \
									      \
		fwa.fwdev = fwdev;					      \
		fwdev->sbp = config_found_sm_loc(sc->dev, "ieee1394if",	      \
		    locs, &fwa, firewire_print, config_stdsubmatch);	      \
		if (fwdev->sbp == NULL) {				      \
			free(devlist, M_DEVBUF);			      \
			break;						      \
		}							      \
									      \
		devlist->fwdev = fwdev;					      \
		devlist->dev = fwdev->sbp;				      \
									      \
		if (SLIST_EMPTY(&sc->devlist))				      \
			SLIST_INSERT_HEAD(&sc->devlist, devlist, link);	      \
		else {							      \
			for (elm = SLIST_FIRST(&sc->devlist);		      \
			    SLIST_NEXT(elm, link) != NULL;		      \
			    elm = SLIST_NEXT(elm, link));		      \
			SLIST_INSERT_AFTER(elm, devlist, link);		      \
		}							      \
	} while (/*CONSTCOND*/0)
#define FIREWIRE_SBP_DETACH						      \
	do {								      \
		struct firewire_softc *sc = (struct firewire_softc *)fc->bdev;\
		struct firewire_dev_list *devlist;			      \
									      \
		SLIST_FOREACH(devlist, &sc->devlist, link) {		      \
			if (devlist->fwdev != fwdev)			      \
				continue;				      \
			SLIST_REMOVE(					      \
			    &sc->devlist, devlist, firewire_dev_list, link);  \
			free(devlist, M_DEVBUF);			      \
									      \
			if (config_detach(fwdev->sbp, DETACH_FORCE) != 0)     \
				return;					      \
		}							      \
	} while (/*CONSTCOND*/0)
#define FIREWIRE_CHILDREN_FOREACH_FUNC(func, fdc)			      \
	do {								      \
		struct firewire_dev_list *devlist;			      \
		struct firewire_softc *sc = (struct firewire_softc *)fc->bdev;\
									      \
		if (!SLIST_EMPTY(&sc->devlist)) {			      \
			SLIST_FOREACH(devlist, &sc->devlist, link) {	      \
				(fdc) =					      \
				    (struct firewire_dev_comm *)devlist->dev; \
				if ((fdc)->func != NULL)		      \
					(fdc)->func((fdc));		      \
			}						      \
		}							      \
	} while (/*CONSTCOND*/0)

/*
 * sbp macro for NetBSD
 */
#define SBP_ATTACH_START					\
	do {							\
		aprint_normal(": SBP-2/SCSI over IEEE1394\n");	\
								\
		sbp->fd.dev = &sbp->fd._dev;			\
	} while (/*CONSTCOND*/0)
#define SBP_SCSIBUS_ATTACH						    \
	do {								    \
		struct scsipi_adapter *sc_adapter = &sbp->sc_adapter;	    \
		struct scsipi_channel *sc_channel = &sbp->sc_channel;	    \
		struct sbp_target *target = &sbp->target;		    \
									    \
		sc_adapter->adapt_dev = sbp->fd.dev;			    \
		sc_adapter->adapt_nchannels = 1;			    \
		sc_adapter->adapt_max_periph = 1;			    \
		sc_adapter->adapt_request = sbp_scsipi_request;		    \
		sc_adapter->adapt_minphys = sbp_minphys;		    \
		sc_adapter->adapt_openings = 8;				    \
					/*Start with some. Grow as needed.*/\
									    \
		sc_channel->chan_adapter = sc_adapter;			    \
		sc_channel->chan_bustype = &scsi_bustype;		    \
		sc_channel->chan_defquirks = PQUIRK_ONLYBIG;		    \
		sc_channel->chan_channel = 0;				    \
		sc_channel->chan_flags =				    \
		    SCSIPI_CHAN_CANGROW | SCSIPI_CHAN_NOSETTLE;		    \
									    \
		/* We set nluns 0 now */				    \
		sc_channel->chan_ntargets = 1;				    \
		sc_channel->chan_nluns = target->num_lun;		    \
		sc_channel->chan_id = 1;				    \
									    \
		if ((sbp->sc_bus =					    \
		    config_found(sbp->fd.dev, sc_channel, scsiprint)) ==    \
		    NULL) {						    \
			device_printf(sbp->fd.dev, "attach failed\n");	    \
			return;						    \
		}							    \
	} while (/*CONSTCOND*/0)
#define SBP_DEVICE(d)		((d)->periph)
#define SBP_DEVICE_FREEZE(d, x)	scsipi_periph_freeze((d)->periph, (x));
#define SBP_DEVICE_THAW(d, x)						\
	do {								\
		if ((d)->periph)					\
			scsipi_periph_thaw((d)->periph, (x));		\
		/* XXXX */						\
		scsipi_channel_thaw(&(d)->target->sbp->sc_channel, 0);	\
	} while (/*CONSTCOND*/0)
#define SBP_BUS_FREEZE(b)	scsipi_channel_freeze(&(b)->sc_channel, 1)
#define SBP_BUS_THAW(b)		scsipi_channel_thaw(&(b)->sc_channel, 1)
#define SBP_DEVICE_PREATTACH()	\
	if (!sbp->proc)		\
		fw_kthread_create(fw_kthread_create0, sbp)

/*
 * fwip macro for NetBSD
 */
#define FWIP_ATTACH_START						\
	device_t dev = &fwip->fd._dev;					\
	struct fw_hwaddr *hwaddr =					\
	    (struct fw_hwaddr *)&fwip->fw_softc.fwcom.ic_hwaddr
#define FWIP_ATTACH_SETUP	aprint_normal(": IP over IEEE1394\n")

#define FWDEV_MAKEDEV(sc)
#define FIREWIRE_IFATTACH(ifp, ha)					       \
	do {								       \
		if_attach((ifp));					       \
		ieee1394_ifattach((ifp), (const struct ieee1394_hwaddr *)(ha));\
	} while (/*CONSTCOND*/0)
#define FIREWIRE_IFDETACH(ifp)			\
	do {					\
		ieee1394_ifdetach((ifp));	\
		if_detach((ifp));		\
	} while (/*CONSTCOND*/0)
#define FIREWIRE_BUSRESET(ifp)	ieee1394_drain((ifp))
#define FIREWIRE_INPUT(ifp, m, src) \
				ieee1394_input((ifp), (m), (src))
#define	FWIP_INIT(sc)		fwip_init(&(sc)->fwip_if)
#define	FWIP_STOP(sc)		fwip_stop(&(sc)->fwip_if, 1)
#define FIREWIRE_IOCTL(ifp, cmd, data) \
				ieee1394_ioctl((ifp), (cmd), (data))
#define IF_INITNAME(ifp, dev, unit)	\
	strcpy((ifp)->if_xname, (dev)->dv_xname);
#define SET_IFFUNC(ifp, start, ioctl, init, stop)	\
	do {						\
		(ifp)->if_start = (start);		\
		(ifp)->if_ioctl = (ioctl);		\
		(ifp)->if_init = (init);		\
		(ifp)->if_stop = (stop);		\
	} while (/*CONSTCOND*/0)

/*
 * fwdev macro for NetBSD
 */
#define FWDEV_OPEN_START	\
	if (DEV_FWMEM(_dev))	\
		return fwmem_open(_dev, flags, fmt, td)
#define FWDEV_CLOSE_START	\
        if (DEV_FWMEM(_dev))	\
		return fwmem_close(_dev, flags, fmt, td)
#define FWDEV_READ_START	\
        if (DEV_FWMEM(_dev))	\
		return physio(fw_strategy, NULL, _dev, ioflag, minphys, uio)
#define FWDEV_WRITE_START	\
        if (DEV_FWMEM(_dev))	\
		return physio(fw_strategy, NULL, _dev, ioflag, minphys, uio)
#define FWDEV_IOCTL_START	\
	if (DEV_FWMEM(_dev))	\
		return fwmem_ioctl(_dev, cmd, data, flag, td)
#define FWDEV_IOCTL_REDIRECT	fc->ioctl (_dev, cmd, data, flag, td)
#define FWDEV_POLL_START	\
	if (DEV_FWMEM(_dev))	\
		return fwmem_poll(_dev, events, td)
#define FWDEV_MMAP_START	\
        if (DEV_FWMEM(_dev)) 	\
		return fwmem_mmap(_dev, offset, nproto)
#define FWDEV_STRATEGY_START		\
	if (DEV_FWMEM(_dev)) {		\
		fwmem_strategy(bp);	\
		return;			\
	}

#define XS_REQ_INVALID		XS_DRIVER_STUFFUP
#define XS_SCSI_BUS_RESET	XS_RESET
#define XS_BDR_SENT		XS_RESET
#define XS_DEV_NOT_THERE	XS_DRIVER_STUFFUP
#define XS_CMD_TIMEOUT		XS_TIMEOUT
#define XS_REQUEUE_REQ		XS_REQUEUE
#define XS_REQ_CMP		XS_NOERROR
#define XS_REQ_CMP_ERR		XS_DRIVER_STUFFUP
#define XS_UA_ABORT		XS_DRIVER_STUFFUP

typedef struct scsipi_xfer sbp_scsi_xfer;
typedef struct scsipi_inquiry_data sbp_scsi_inquiry_data;

#define SCSI_XFER_TARGET(x)	((x)->xs_periph->periph_target)
#define SCSI_XFER_LUN(x)	((x)->xs_periph->periph_lun)
#define SCSI_XFER_ERROR(x)	((x)->error)
#define SCSI_XFER_DIR(x) \
	((x)->xs_control & (XS_CTL_DATA_IN | XS_CTL_DATA_OUT))
#define     SCSI_XFER_DATA_IN	XS_CTL_DATA_IN
#define SCSI_XFER_CALLOUT(x)	((x)->xs_callout)
#define SCSI_XFER_TIMEOUT(x)	((x)->timeout)
#define SCSI_XFER_OPECODE(x)	((x)->cmd->opcode)
#define SCSI_XFER_STATUS(x)	((x)->xs_status)
#define SCSI_XFER_EVPD(x)	((x)->cmd->bytes[0] & SI_EVPD)
#define     SI_EVPD		0x01
#define SCSI_XFER_CMDLEN(x)	((x)->cmdlen)
#define SCSI_XFER_CMD(x)	((x)->cmd)
#define SCSI_XFER_DATALEN(x)	((x)->datalen)
#define SCSI_XFER_DATA(x)	((x)->data)
#define SCSI_XFER_SENSELEN(x)	(0 /* XXXXX */)
#define SCSI_SENSE_DATA(x)	(&(x)->sense.scsi_sense)
#define SCSI_INQUIRY_DATA(x)	((x)->data)
#define SCSI_XFER_FUNCCODE(x)	XPT_SCSI_IO
#define SCSI_XFER_10BCMD_DUMP(x)\
	((x)->cmd->opcode),	\
	((x)->cmd->bytes[0]),	\
	((x)->cmd->bytes[1]),	\
	((x)->cmd->bytes[2]),	\
	((x)->cmd->bytes[3]),	\
	((x)->cmd->bytes[4]),	\
	((x)->cmd->bytes[5]),	\
	((x)->cmd->bytes[6]),	\
	((x)->cmd->bytes[7]),	\
	((x)->cmd->bytes[8])
#define SCSI_XFER_REQUEST_COMPLETE(x) \
	((x)->resid = 0)
#define SCSI_TRANSFER_DONE(x)	scsipi_done((x))

#define NOT_LUN_WILDCARD(l)	(1)
#define CAM_XFER_FLAGS(x)	(0)	/* XXX */
#define CAM_SCATTER_VALID	(0)	/* XXX */
#define CAM_DATA_PHYS		(0)	/* XXX */
#define XPT_SCSI_IO		(1)	/* XXX */


#define splfw()		splvm()
#define splfwnet()	splnet()
#define splfwsbp()	splbio()
#define splsoftvm()	splbio()

#define roundup2(x, y) roundup((x), (y))
#define rounddown(x, y) ((x) / (y) * (y))

#define timevalcmp(tv1, tv2, op)	timercmp((tv1), (tv2), op)
#define timevalsub(tv1, tv2)		timersub((tv1), (tv2), (tv1))

#define device_get_nameunit(dev) (dev)->dv_xname
#define device_get_unit(dev)		device_unit((dev))

/*
 * queue macros for NetBSD
 */
#define STAILQ_LAST(head, type, field) \
	(STAILQ_EMPTY((head)) ? NULL : \
	(struct type *) \
	((char *)(head)->stqh_last - (size_t)&((struct type *)0)->field))

#define TASK_INIT(task, priority, func, context)


struct fw_hwaddr {
	uint32_t		sender_unique_ID_hi;
	uint32_t		sender_unique_ID_lo;
	uint8_t			sender_max_rec;
	uint8_t			sspd;
	uint16_t		sender_unicast_FIFO_hi;
	uint32_t		sender_unicast_FIFO_lo;
};


/*
 * mbuf macros for NetBSD
 */
#include <sys/mbuf.h>
#define	M_TRYWAIT	M_WAITOK

#define m_tag_alloc(cookie, type, len, wait) \
				m_tag_get((type), (len), (wait))
#define m_tag_locate(m, cookie, type, t) \
				m_tag_find((m), (type), (t))

/*
 * bus_dma macros for NetBSD
 */
#include <machine/bus.h>
struct fw_bus_dma_tag {
	bus_dma_tag_t tag;
	bus_size_t alignment;
	bus_size_t boundary;
	bus_size_t size;
	int nsegments;
	bus_size_t maxsegsz;
	int flags;
};
typedef struct fw_bus_dma_tag *fw_bus_dma_tag_t;
typedef int bus_dmasync_op_t;
typedef void bus_dmamap_callback_t(void *, bus_dma_segment_t *, int, int);
typedef void
    bus_dmamap_callback2_t(void *, bus_dma_segment_t *, int, bus_size_t, int);

#define fw_bus_dma_tag_create(				\
    p, a, b, la, ha, ffunc, farg, maxsz,		\
    nseg, maxsegsz, f, lfunc, larg, dmat)		\
							\
    _fw_bus_dma_tag_create((p), (a), (b), (la), (ha),	\
	(ffunc), (farg), (maxsz), (nseg), (maxsegsz), (f), (dmat))

static __inline int
_fw_bus_dma_tag_create(bus_dma_tag_t parent,
    bus_size_t alignment, bus_size_t boundary,
    bus_addr_t lowaddr, bus_addr_t highaddr,
    void *filtfunc, void *filtfuncarg,
    bus_size_t maxsize, int nsegments, bus_size_t maxsegsz,
    int flags, fw_bus_dma_tag_t *fwdmat)
{
	fw_bus_dma_tag_t tag;

	tag = malloc(sizeof (struct fw_bus_dma_tag), M_DEVBUF, M_NOWAIT);
	if (tag == NULL)
		return ENOMEM;

/* XXXX */
#define BUS_SPACE_MAXADDR_32BIT 0
#define BUS_SPACE_MAXADDR 0

	tag->tag = parent;
	tag->alignment = alignment;
	tag->boundary = boundary;
	tag->size = maxsize;
	tag->nsegments = nsegments;
	tag->maxsegsz = maxsegsz;
	tag->flags = flags;

	*fwdmat = tag;

	return 0;
}
#define fw_bus_dma_tag_destroy(ft) \
	free(ft, M_DEVBUF)
#define fw_bus_dmamap_create(ft, f, mp)		\
	bus_dmamap_create((ft)->tag, (ft)->size,\
	    (ft)->nsegments, (ft)->maxsegsz, (ft)->boundary, (f), (mp))
#define fw_bus_dmamap_destroy(ft, m) \
	bus_dmamap_destroy((ft)->tag, (m))
static __inline int
fw_bus_dmamap_load(fw_bus_dma_tag_t ft, bus_dmamap_t m,
    void *b, bus_size_t l, bus_dmamap_callback_t *func, void *a, int f)   
{
	int lf = f & (BUS_DMA_WAITOK | BUS_DMA_NOWAIT | BUS_DMA_STREAMING |
	    BUS_DMA_READ | BUS_DMA_WRITE |
	    BUS_DMA_BUS1 | BUS_DMA_BUS2 | BUS_DMA_BUS3 | BUS_DMA_BUS4);
	int err = bus_dmamap_load(ft->tag, m, b, l, NULL, lf);
	(func)(a, m->dm_segs, m->dm_nsegs, err);
	return err;
}
static __inline int
fw_bus_dmamap_load_mbuf(fw_bus_dma_tag_t ft, bus_dmamap_t m,
    struct mbuf *b, bus_dmamap_callback2_t *func, void *a, int f)   
{
	int lf = f & (BUS_DMA_WAITOK | BUS_DMA_NOWAIT | BUS_DMA_STREAMING |
	    BUS_DMA_READ | BUS_DMA_WRITE |
	    BUS_DMA_BUS1 | BUS_DMA_BUS2 | BUS_DMA_BUS3 | BUS_DMA_BUS4);
	int err = bus_dmamap_load_mbuf(ft->tag, m, b, lf);
	(func)(a, m->dm_segs, m->dm_nsegs, m->dm_mapsize, err);
	return err;
}
#define fw_bus_dmamap_unload(ft, m) \
	bus_dmamap_unload((ft)->tag, (m))
#define fw_bus_dmamap_sync(ft, m, op) \
	bus_dmamap_sync((ft)->tag, (m), 0, (m)->dm_mapsize, (op))
static __inline int
fw_bus_dmamem_alloc(fw_bus_dma_tag_t ft, void **vp, int f, bus_dmamap_t *mp)
{
        bus_dma_segment_t segs;
	int nsegs, err;
	int af, mf, cf;

	af = f & (BUS_DMA_WAITOK | BUS_DMA_NOWAIT | BUS_DMA_STREAMING |
	    BUS_DMA_BUS1 | BUS_DMA_BUS2 | BUS_DMA_BUS3 | BUS_DMA_BUS4);
	err = bus_dmamem_alloc(ft->tag, ft->size,
	    ft->alignment, ft->boundary, &segs, ft->nsegments, &nsegs, af);
	if (err) {
		printf("fw_bus_dmamem_alloc: failed(1)\n");
		return err;
	}

	mf = f & (BUS_DMA_WAITOK | BUS_DMA_NOWAIT |
	    BUS_DMA_BUS1 | BUS_DMA_BUS2 | BUS_DMA_BUS3 | BUS_DMA_BUS4 |
	    BUS_DMA_COHERENT | BUS_DMA_NOCACHE);
	err = bus_dmamem_map(ft->tag,
	    &segs, nsegs, ft->size, (caddr_t *)vp, mf);
	if (err) {
		printf("fw_bus_dmamem_alloc: failed(2)\n");
		bus_dmamem_free(ft->tag, &segs, nsegs);
		return err;
	}

	if (*mp != NULL)
		return err;

	cf = f & (BUS_DMA_WAITOK | BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW |
	    BUS_DMA_BUS1 | BUS_DMA_BUS2 | BUS_DMA_BUS3 | BUS_DMA_BUS4);
	err = bus_dmamap_create(ft->tag,
	    ft->size, nsegs, ft->maxsegsz, ft->boundary, cf, mp);
	if (err) {
		printf("fw_bus_dmamem_alloc: failed(3)\n");
		bus_dmamem_unmap(ft->tag, (caddr_t)*vp, ft->size);
		bus_dmamem_free(ft->tag, &segs, nsegs);\
	}

	return err;
}
#define fw_bus_dmamem_free(ft, v, m)					\
	do {								\
		bus_dmamem_unmap((ft)->tag, (v), (ft)->size);		\
		bus_dmamem_free((ft)->tag, (m)->dm_segs, (m)->dm_nsegs);\
		bus_dmamap_destroy((ft)->tag, (m));			\
	} while (/*CONSTCOND*/0)


#define device_printf(dev, fmt, ...)			\
	do {						\
		aprint_normal("%s: ", (dev)->dv_xname);	\
		aprint_normal((fmt) ,##__VA_ARGS__);	\
	} while (/*CONSTCOND*/0)


#define CTR0(m, format)
#define CTR1(m, format, p1)


#define OHCI_CSR_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->bst, (sc)->bsh, reg, val)

/*
 * XXXXXXXXXXX
 */
#define atomic_set_int(P, V) (*(u_int*)(P) |= (V))

#endif
#endif
#if defined(__NetBSD__)
#define vm_offset_t caddr_t
#endif
#endif
