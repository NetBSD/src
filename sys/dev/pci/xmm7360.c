/*	$NetBSD: xmm7360.c,v 1.14 2022/02/12 03:24:36 riastradh Exp $	*/

/*
 * Device driver for Intel XMM7360 LTE modems, eg. Fibocom L850-GL.
 * Written by James Wah
 * james@laird-wah.net
 *
 * Development of this driver was supported by genua GmbH
 *
 * Copyright (c) 2020 genua GmbH <info@genua.de>
 * Copyright (c) 2020 James Wah <james@laird-wah.net>
 *
 * The OpenBSD and NetBSD support was written by Jaromir Dolecek for
 * Moritz Systems Technology Company Sp. z o.o.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES ON
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGE
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef __linux__

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/poll.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <net/rtnetlink.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>

MODULE_LICENSE("Dual BSD/GPL");

static const struct pci_device_id xmm7360_ids[] = {
	{ PCI_DEVICE(0x8086, 0x7360), },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, xmm7360_ids);

/* Actually this ioctl not used for xmm0/rpc device by python code */
#define XMM7360_IOCTL_GET_PAGE_SIZE _IOC(_IOC_READ, 'x', 0xc0, sizeof(u32))

#define xmm7360_os_msleep(msec)		msleep(msec)

#define __unused			/* nothing */

#endif

#if defined(__OpenBSD__) || defined(__NetBSD__)

#ifdef __OpenBSD__
#include "bpfilter.h"
#endif
#ifdef __NetBSD__
#include "opt_inet.h"
#include "opt_gateway.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xmm7360.c,v 1.14 2022/02/12 03:24:36 riastradh Exp $");
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/mutex.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/kthread.h>
#include <sys/poll.h>
#include <sys/fcntl.h>		/* for FREAD/FWRITE */
#include <sys/vnode.h>
#include <uvm/uvm_param.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <net/if.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

#ifdef __OpenBSD__
#include <netinet/if_ether.h>
#include <sys/timeout.h>
#include <machine/bus.h>
#endif

#if NBPFILTER > 0 || defined(__NetBSD__)
#include <net/bpf.h>
#endif

#ifdef __NetBSD__
#include "ioconf.h"
#include <sys/cpu.h>
#endif

#ifdef INET
#include <netinet/in_var.h>
#endif
#ifdef INET6
#include <netinet6/in6_var.h>
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef bus_addr_t dma_addr_t;
typedef void * wait_queue_head_t;	/* just address for tsleep() */

#define WWAN_BAR0	PCI_MAPREG_START
#define WWAN_BAR1	(PCI_MAPREG_START + 4)
#define WWAN_BAR2	(PCI_MAPREG_START + 8)

#define BUG_ON(never_true)	KASSERT(!(never_true))
#define WARN_ON(x)		/* nothing */

#ifdef __OpenBSD__
typedef struct mutex spinlock_t;
#define dev_err(devp, fmt, ...)		\
	printf("%s: " fmt, device_xname(devp), ##__VA_ARGS__)
#define dev_info(devp, fmt, ...)	\
	printf("%s: " fmt, device_xname(devp), ##__VA_ARGS__)
#define	kzalloc(size, flags)	malloc(size, M_DEVBUF, M_WAITOK | M_ZERO)
#define kfree(addr)		free(addr, M_DEVBUF, 0)
#define mutex_init(lock)	mtx_init(lock, IPL_TTY)
#define mutex_lock(lock)	mtx_enter(lock)
#define mutex_unlock(lock)	mtx_leave(lock)
/* In OpenBSD every mutex is spin mutex, and it must not be held on sleep */
#define spin_lock_irqsave(lock, flags)		mtx_enter(lock)
#define spin_unlock_irqrestore(lock, flags)	mtx_leave(lock)

/* Compat defines for NetBSD API */
#define curlwp			curproc
#define LINESW(tp)				(linesw[(tp)->t_line])
#define selnotify(sel, band, note)		selwakeup(sel)
#define cfdata_t				void *
#define device_lookup_private(cdp, unit)	\
	(unit < (*cdp).cd_ndevs) ? (*cdp).cd_devs[unit] : NULL
#define IFQ_SET_READY(ifq)			/* nothing */
#define device_private(devt)			(void *)devt;
#define if_deferred_start_init(ifp, arg)	/* nothing */
#define IF_OUTPUT_CONST				/* nothing */
#define knote_set_eof(kn, f)			(kn)->kn_flags |= EV_EOF | (f)
#define tty_lock()				int s = spltty()
#define tty_unlock()				splx(s)
#define tty_locked()				/* nothing */
#define pmf_device_deregister(dev)		/* nothing */
#if NBPFILTER > 0
#define BPF_MTAP_OUT(ifp, m)						\
                if (ifp->if_bpf) {					\
                        bpf_mtap_af(ifp->if_bpf, m->m_pkthdr.ph_family,	\
			    m, BPF_DIRECTION_OUT);			\
		}
#else
#define BPF_MTAP_OUT(ifp, m)			/* nothing */
#endif

/* Copied from NetBSD <lib/libkern/libkern.h> */
#define __validate_container_of(PTR, TYPE, FIELD)			\
    (0 * sizeof((PTR) - &((TYPE *)(((char *)(PTR)) -			\
    offsetof(TYPE, FIELD)))->FIELD))
#define	container_of(PTR, TYPE, FIELD)					\
    ((TYPE *)(((char *)(PTR)) - offsetof(TYPE, FIELD))			\
	+ __validate_container_of(PTR, TYPE, FIELD))

/* Copied from NetBSD <sys/cdefs.h> */
#define __UNVOLATILE(a)		((void *)(unsigned long)(volatile void *)(a))

#if OpenBSD <= 201911
/* Backward compat with OpenBSD 6.6 */
#define klist_insert(klist, kn)		\
		SLIST_INSERT_HEAD(klist, kn, kn_selnext)
#define klist_remove(klist, kn)		\
		SLIST_REMOVE(klist, kn, knote, kn_selnext)
#define XMM_KQ_ISFD_INITIALIZER		.f_isfd = 1
#else
#define XMM_KQ_ISFD_INITIALIZER		.f_flags = FILTEROP_ISFD
#endif /* OpenBSD <= 201911 */

#endif

#ifdef __NetBSD__
typedef struct kmutex spinlock_t;
#define dev_err			aprint_error_dev
#define dev_info		aprint_normal_dev
#define mutex			kmutex
#define kzalloc(size, flags)	malloc(size, M_DEVBUF, M_WAITOK | M_ZERO)
#define kfree(addr)		free(addr, M_DEVBUF)
#define mutex_init(lock)	mutex_init(lock, MUTEX_DEFAULT, IPL_TTY)
#define mutex_lock(lock)	mutex_enter(lock)
#define mutex_unlock(lock)	mutex_exit(lock)
#define spin_lock_irqsave(lock, flags)	mutex_enter(lock)
#define spin_unlock_irqrestore(lock, flags)	mutex_exit(lock)

/* Compat defines with OpenBSD API */
#define caddr_t			void *
#define proc			lwp
#define LINESW(tp)		(*tp->t_linesw)
#define ttymalloc(speed)	tty_alloc()
#define ttyfree(tp)		tty_free(tp)
#define l_open(dev, tp, p)	l_open(dev, tp)
#define l_close(tp, flag, p)	l_close(tp, flag)
#define ttkqfilter(dev, kn)	ttykqfilter(dev, kn)
#define msleep(ident, lock, prio, wmesg, timo) \
		mtsleep(ident, prio, wmesg, timo, lock)
#define pci_mapreg_map(pa, reg, type, busfl, tp, hp, bp, szp, maxsize) \
	pci_mapreg_map(pa, reg, type, busfl, tp, hp, bp, szp)
#define pci_intr_establish(pc, ih, lvl, func, arg, name) \
	pci_intr_establish_xname(pc, ih, lvl, func, arg, name)
#define suser(l)					\
	kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp)
#define kthread_create(func, arg, lwpp, name)		\
	kthread_create(0, 0, NULL, func, arg, lwpp, "%s", name)
#define MUTEX_ASSERT_LOCKED(lock)	KASSERT(mutex_owned(lock))
#define MCLGETI(m, how, m0, sz)		MCLGET(m, how)
#define m_copyback(m, off, sz, buf, how)		\
					m_copyback(m, off, sz, buf)
#define ifq_deq_begin(ifq)		({		\
		struct mbuf *m0;			\
		IFQ_DEQUEUE(ifq, m0);			\
		m0;					\
})
#define ifq_deq_rollback(ifq, m)	m_freem(m)
#define ifq_deq_commit(ifq, m)		/* nothing to do */
#define ifq_is_oactive(ifq)		true	/* always restart queue */
#define ifq_clr_oactive(ifq)		/* nothing to do */
#define ifq_empty(ifq)			IFQ_IS_EMPTY(ifq)
#define ifq_purge(ifq)			IF_PURGE(ifq)
#define if_enqueue(ifp, m)		ifq_enqueue(ifp, m)
#define if_ih_insert(ifp, func, arg)	(ifp)->_if_input = (func)
#define if_ih_remove(ifp, func, arg)	/* nothing to do */
#define if_hardmtu			if_mtu
#define IF_OUTPUT_CONST			const
#define si_note				sel_klist
#define klist_insert(klist, kn)		\
		SLIST_INSERT_HEAD(klist, kn, kn_selnext)
#define klist_remove(klist, kn)		\
		SLIST_REMOVE(klist, kn, knote, kn_selnext)
#define XMM_KQ_ISFD_INITIALIZER		.f_flags = FILTEROP_ISFD
#define tty_lock()			mutex_spin_enter(&tty_lock)
#define tty_unlock()			mutex_spin_exit(&tty_lock)
#define tty_locked()			KASSERT(mutex_owned(&tty_lock))
#define bpfattach(bpf, ifp, dlt, sz)	bpf_attach(ifp, dlt, sz)
#define NBPFILTER			1
#define BPF_MTAP_OUT(ifp, m)		bpf_mtap(ifp, m, BPF_D_OUT)
#endif /* __NetBSD__ */

#define __user				/* nothing */
#define copy_from_user(kbuf, userbuf, sz)		\
({							\
	int __ret = 0;					\
	int error = copyin(userbuf, kbuf, sz);		\
	if (error != 0)					\
		return -error;				\
	__ret;						\
})
#define copy_to_user(kbuf, userbuf, sz)			\
({							\
	int __ret = 0;					\
	int error = copyout(userbuf, kbuf, sz);		\
	if (error != 0)					\
		return -error;				\
	__ret;						\
})
#define xmm7360_os_msleep(msec)					\
	do {							\
		KASSERT(!cold);					\
		tsleep(xmm, 0, "wwancsl", msec * hz / 1000);	\
	} while (0)

static pktq_rps_hash_func_t xmm7360_pktq_rps_hash_p;
static void *dma_alloc_coherent(struct device *, size_t, dma_addr_t *, int);
static void dma_free_coherent(struct device *, size_t, volatile void *, dma_addr_t);

#ifndef PCI_PRODUCT_INTEL_XMM7360
#define PCI_PRODUCT_INTEL_XMM7360	0x7360
#endif

#define init_waitqueue_head(wqp)	*(wqp) = (wqp)
#define wait_event_interruptible(wq, cond)				\
({									\
	int __ret = 1;							\
	while (!(cond)) {						\
		KASSERT(!cold);						\
		int error = tsleep(wq, PCATCH, "xmmwq", 0);		\
		if (error) {						\
			__ret = (cond) ? 1				\
			    : ((error != ERESTART) ? -error : error);	\
			break;						\
		}							\
	}								\
	__ret;								\
})

#define msecs_to_jiffies(msec)						\
({									\
	KASSERT(hz < 1000);						\
	KASSERT(msec > (1000 / hz));					\
	msec * hz / 1000;						\
})

#define wait_event_interruptible_timeout(wq, cond, jiffies)		\
({									\
	int __ret = 1;							\
	while (!(cond)) {						\
		if (cold) {						\
			for (int loop = 0; loop < 10; loop++) {		\
				delay(jiffies * 1000 * 1000 / hz / 10);	\
				if (cond)				\
					break;				\
			}						\
			__ret = (cond) ? 1 : 0;				\
			break;						\
		}							\
		int error = tsleep(wq, PCATCH, "xmmwq", jiffies);	\
		if (error) {						\
			__ret = (cond) ? 1				\
			    : ((error != ERESTART) ? -error : error);	\
			break;						\
		}							\
	}								\
	__ret;								\
})

#define GFP_KERNEL			0

#endif /* __OpenBSD__ || __NetBSD__ */

/*
 * The XMM7360 communicates via DMA ring buffers. It has one
 * command ring, plus sixteen transfer descriptor (TD)
 * rings. The command ring is mainly used to configure and
 * deconfigure the TD rings.
 *
 * The 16 TD rings form 8 queue pairs (QP). For example, QP
 * 0 uses ring 0 for host->device, and ring 1 for
 * device->host.
 *
 * The known queue pair functions are as follows:
 *
 * 0:	Mux (Raw IP packets, amongst others)
 * 1:	RPC (funky command protocol based in part on ASN.1 BER)
 * 2:	AT trace? port; does not accept commands after init
 * 4:	AT command port
 * 7:	AT command port
 *
 */

/* Command ring, which is used to configure the queue pairs */
struct cmd_ring_entry {
	dma_addr_t ptr;
	u16 len;
	u8 parm;
	u8 cmd;
	u32 extra;
	u32 unk, flags;
};

#define CMD_RING_OPEN	1
#define CMD_RING_CLOSE	2
#define CMD_RING_FLUSH	3
#define CMD_WAKEUP	4

#define CMD_FLAG_DONE	1
#define CMD_FLAG_READY	2

/* Transfer descriptors used on the Tx and Rx rings of each queue pair */
struct td_ring_entry {
	dma_addr_t addr;
	u16 length;
	u16 flags;
	u32 unk;
};

#define TD_FLAG_COMPLETE 0x200

/* Root configuration object. This contains pointers to all of the control
 * structures that the modem will interact with.
 */
struct control {
	dma_addr_t status;
	dma_addr_t s_wptr, s_rptr;
	dma_addr_t c_wptr, c_rptr;
	dma_addr_t c_ring;
	u16 c_ring_size;
	u16 unk;
};

struct status {
	u32 code;
	u32 mode;
	u32 asleep;
	u32 pad;
};

#define CMD_RING_SIZE 0x80

/* All of the control structures can be packed into one page of RAM. */
struct control_page {
	struct control ctl;
	// Status words - written by modem.
	volatile struct status status;
	// Slave ring write/read pointers.
	volatile u32 s_wptr[16], s_rptr[16];
	// Command ring write/read pointers.
	volatile u32 c_wptr, c_rptr;
	// Command ring entries.
	volatile struct cmd_ring_entry c_ring[CMD_RING_SIZE];
};

#define BAR0_MODE	0x0c
#define BAR0_DOORBELL	0x04
#define BAR0_WAKEUP	0x14

#define DOORBELL_TD	0
#define DOORBELL_CMD	1

#define BAR2_STATUS	0x00
#define BAR2_MODE	0x18
#define BAR2_CONTROL	0x19
#define BAR2_CONTROLH	0x1a

#define BAR2_BLANK0	0x1b
#define BAR2_BLANK1	0x1c
#define BAR2_BLANK2	0x1d
#define BAR2_BLANK3	0x1e

#define XMM_MODEM_BOOTING	0xfeedb007
#define XMM_MODEM_READY		0x600df00d

#define XMM_TAG_ACBH		0x41434248	// 'ACBH'
#define XMM_TAG_CMDH		0x434d4448	// 'CMDH'
#define XMM_TAG_ADBH		0x41444248	// 'ADBH'
#define XMM_TAG_ADTH		0x41445448	// 'ADTH'

/* There are 16 TD rings: a Tx and Rx ring for each queue pair */
struct td_ring {
	u8 depth;
	u8 last_handled;
	u16 page_size;

	struct td_ring_entry *tds;
	dma_addr_t tds_phys;

	// One page of page_size per td
	void **pages;
	dma_addr_t *pages_phys;
};

#define TD_MAX_PAGE_SIZE 16384

struct queue_pair {
	struct xmm_dev *xmm;
	u8 depth;
	u16 page_size;
	int tty_index;
	int tty_needs_wake;
#ifdef __linux__
	struct device dev;
#endif
	int num;
	int open;
	struct mutex lock;
	unsigned char user_buf[TD_MAX_PAGE_SIZE];
	wait_queue_head_t wq;

#ifdef __linux__
	struct cdev cdev;
	struct tty_port port;
#endif
#if defined(__OpenBSD__) || defined(__NetBSD__)
	struct selinfo selr, selw;
#endif
};

#define XMM_QP_COUNT	8

struct xmm_dev {
	struct device *dev;

	volatile uint32_t *bar0, *bar2;

	volatile struct control_page *cp;
	dma_addr_t cp_phys;

	struct td_ring td_ring[2 * XMM_QP_COUNT];

	struct queue_pair qp[XMM_QP_COUNT];

	struct xmm_net *net;
	struct net_device *netdev;

	int error;
	int card_num;
	int num_ttys;
	wait_queue_head_t wq;

#ifdef __linux__
	struct pci_dev *pci_dev;

	int irq;

	struct work_struct init_work;	// XXX work not actually scheduled
#endif
};

struct mux_bounds {
	uint32_t offset;
	uint32_t length;
};

struct mux_first_header {
	uint32_t tag;
	uint16_t unknown;
	uint16_t sequence;
	uint16_t length;
	uint16_t extra;
	uint16_t next;
	uint16_t pad;
};

struct mux_next_header {
	uint32_t tag;
	uint16_t length;
	uint16_t extra;
	uint16_t next;
	uint16_t pad;
};

#define MUX_MAX_PACKETS	64

struct mux_frame {
	int n_packets, n_bytes, max_size, sequence;
	uint16_t *last_tag_length, *last_tag_next;
	struct mux_bounds bounds[MUX_MAX_PACKETS];
	uint8_t data[TD_MAX_PAGE_SIZE];
};

struct xmm_net {
	struct xmm_dev *xmm;
	struct queue_pair *qp;
	int channel;

#ifdef __linux__
	struct sk_buff_head queue;
	struct hrtimer deadline;
#endif
	int queued_packets, queued_bytes;

	int sequence;
	spinlock_t lock;
	struct mux_frame frame;
};

static void xmm7360_os_handle_net_frame(struct xmm_dev *, const u8 *, size_t);
static void xmm7360_os_handle_net_dequeue(struct xmm_net *, struct mux_frame *);
static void xmm7360_os_handle_net_txwake(struct xmm_net *);
static void xmm7360_os_handle_tty_idata(struct queue_pair *, const u8 *, size_t);

static void xmm7360_poll(struct xmm_dev *xmm)
{
	if (xmm->cp->status.code == 0xbadc0ded) {
		dev_err(xmm->dev, "crashed but dma up\n");
		xmm->error = -ENODEV;
	}
	if (xmm->bar2[BAR2_STATUS] != XMM_MODEM_READY) {
		dev_err(xmm->dev, "bad status %x\n",xmm->bar2[BAR2_STATUS]);
		xmm->error = -ENODEV;
	}
}

static void xmm7360_ding(struct xmm_dev *xmm, int bell)
{
	if (xmm->cp->status.asleep)
		xmm->bar0[BAR0_WAKEUP] = 1;
	xmm->bar0[BAR0_DOORBELL] = bell;
	xmm7360_poll(xmm);
}

static int xmm7360_cmd_ring_wait(struct xmm_dev *xmm)
{
	// Wait for all commands to complete
	// XXX locking?
	int ret = wait_event_interruptible_timeout(xmm->wq, (xmm->cp->c_rptr == xmm->cp->c_wptr) || xmm->error, msecs_to_jiffies(1000));
	if (ret == 0)
		return -ETIMEDOUT;
	if (ret < 0)
		return ret;
	return xmm->error;
}

static int xmm7360_cmd_ring_execute(struct xmm_dev *xmm, u8 cmd, u8 parm, u16 len, dma_addr_t ptr, u32 extra)
{
	u8 wptr = xmm->cp->c_wptr;
	u8 new_wptr = (wptr + 1) % CMD_RING_SIZE;
	if (xmm->error)
		return xmm->error;
	if (new_wptr == xmm->cp->c_rptr)	// ring full
		return -EAGAIN;

	xmm->cp->c_ring[wptr].ptr = ptr;
	xmm->cp->c_ring[wptr].cmd = cmd;
	xmm->cp->c_ring[wptr].parm = parm;
	xmm->cp->c_ring[wptr].len = len;
	xmm->cp->c_ring[wptr].extra = extra;
	xmm->cp->c_ring[wptr].unk = 0;
	xmm->cp->c_ring[wptr].flags = CMD_FLAG_READY;

	xmm->cp->c_wptr = new_wptr;

	xmm7360_ding(xmm, DOORBELL_CMD);
	return xmm7360_cmd_ring_wait(xmm);
}

static int xmm7360_cmd_ring_init(struct xmm_dev *xmm) {
	int timeout;
	int ret;

	xmm->cp = dma_alloc_coherent(xmm->dev, sizeof(struct control_page), &xmm->cp_phys, GFP_KERNEL);
	BUG_ON(xmm->cp == NULL);

	xmm->cp->ctl.status = xmm->cp_phys + offsetof(struct control_page, status);
	xmm->cp->ctl.s_wptr = xmm->cp_phys + offsetof(struct control_page, s_wptr);
	xmm->cp->ctl.s_rptr = xmm->cp_phys + offsetof(struct control_page, s_rptr);
	xmm->cp->ctl.c_wptr = xmm->cp_phys + offsetof(struct control_page, c_wptr);
	xmm->cp->ctl.c_rptr = xmm->cp_phys + offsetof(struct control_page, c_rptr);
	xmm->cp->ctl.c_ring = xmm->cp_phys + offsetof(struct control_page, c_ring);
	xmm->cp->ctl.c_ring_size = CMD_RING_SIZE;

	xmm->bar2[BAR2_CONTROL] = xmm->cp_phys;
	xmm->bar2[BAR2_CONTROLH] = xmm->cp_phys >> 32;

	xmm->bar0[BAR0_MODE] = 1;

	timeout = 100;
	while (xmm->bar2[BAR2_MODE] == 0 && --timeout)
		xmm7360_os_msleep(10);

	if (!timeout)
		return -ETIMEDOUT;

	xmm->bar2[BAR2_BLANK0] = 0;
	xmm->bar2[BAR2_BLANK1] = 0;
	xmm->bar2[BAR2_BLANK2] = 0;
	xmm->bar2[BAR2_BLANK3] = 0;

	xmm->bar0[BAR0_MODE] = 2;	// enable intrs?

	timeout = 100;
	while (xmm->bar2[BAR2_MODE] != 2 && --timeout)
		xmm7360_os_msleep(10);

	if (!timeout)
		return -ETIMEDOUT;

	// enable going to sleep when idle
	ret = xmm7360_cmd_ring_execute(xmm, CMD_WAKEUP, 0, 1, 0, 0);
	if (ret)
		return ret;

	return 0;
}

static void xmm7360_cmd_ring_free(struct xmm_dev *xmm) {
	if (xmm->bar0)
		xmm->bar0[BAR0_MODE] = 0;
	if (xmm->cp)
		dma_free_coherent(xmm->dev, sizeof(struct control_page), (volatile void *)xmm->cp, xmm->cp_phys);
	xmm->cp = NULL;
	return;
}

static void xmm7360_td_ring_activate(struct xmm_dev *xmm, u8 ring_id)
{
	struct td_ring *ring = &xmm->td_ring[ring_id];
	int ret __diagused;

	xmm->cp->s_rptr[ring_id] = xmm->cp->s_wptr[ring_id] = 0;
	ring->last_handled = 0;
	ret = xmm7360_cmd_ring_execute(xmm, CMD_RING_OPEN, ring_id, ring->depth, ring->tds_phys, 0x60);
	BUG_ON(ret);
}

static void xmm7360_td_ring_create(struct xmm_dev *xmm, u8 ring_id, u8 depth, u16 page_size)
{
	struct td_ring *ring = &xmm->td_ring[ring_id];
	int i;

	BUG_ON(ring->depth);
	BUG_ON(depth & (depth-1));
	BUG_ON(page_size > TD_MAX_PAGE_SIZE);

	memset(ring, 0, sizeof(struct td_ring));
	ring->depth = depth;
	ring->page_size = page_size;
	ring->tds = dma_alloc_coherent(xmm->dev, sizeof(struct td_ring_entry)*depth, &ring->tds_phys, GFP_KERNEL);

	ring->pages = kzalloc(sizeof(void*)*depth, GFP_KERNEL);
	ring->pages_phys = kzalloc(sizeof(dma_addr_t)*depth, GFP_KERNEL);

	for (i=0; i<depth; i++) {
		ring->pages[i] = dma_alloc_coherent(xmm->dev, ring->page_size, &ring->pages_phys[i], GFP_KERNEL);
		ring->tds[i].addr = ring->pages_phys[i];
	}

	xmm7360_td_ring_activate(xmm, ring_id);
}

static void xmm7360_td_ring_deactivate(struct xmm_dev *xmm, u8 ring_id)
{
	xmm7360_cmd_ring_execute(xmm, CMD_RING_CLOSE, ring_id, 0, 0, 0);
}

static void xmm7360_td_ring_destroy(struct xmm_dev *xmm, u8 ring_id)
{
	struct td_ring *ring = &xmm->td_ring[ring_id];
	int i, depth=ring->depth;

	if (!depth) {
		WARN_ON(1);
		dev_err(xmm->dev, "Tried destroying empty ring!\n");
		return;
	}

	xmm7360_td_ring_deactivate(xmm, ring_id);

	for (i=0; i<depth; i++) {
		dma_free_coherent(xmm->dev, ring->page_size, ring->pages[i], ring->pages_phys[i]);
	}

	kfree(ring->pages_phys);
	kfree(ring->pages);

	dma_free_coherent(xmm->dev, sizeof(struct td_ring_entry)*depth, ring->tds, ring->tds_phys);

	ring->depth = 0;
}

static void xmm7360_td_ring_write(struct xmm_dev *xmm, u8 ring_id, const void *buf, int len)
{
	struct td_ring *ring = &xmm->td_ring[ring_id];
	u8 wptr = xmm->cp->s_wptr[ring_id];

	BUG_ON(!ring->depth);
	BUG_ON(len > ring->page_size);
	BUG_ON(ring_id & 1);

	memcpy(ring->pages[wptr], buf, len);
	ring->tds[wptr].length = len;
	ring->tds[wptr].flags = 0;
	ring->tds[wptr].unk = 0;

	wptr = (wptr + 1) & (ring->depth - 1);
	BUG_ON(wptr == xmm->cp->s_rptr[ring_id]);

	xmm->cp->s_wptr[ring_id] = wptr;
}

static int xmm7360_td_ring_full(struct xmm_dev *xmm, u8 ring_id)
{
	struct td_ring *ring = &xmm->td_ring[ring_id];
	u8 wptr = xmm->cp->s_wptr[ring_id];
	wptr = (wptr + 1) & (ring->depth - 1);
	return wptr == xmm->cp->s_rptr[ring_id];
}

static void xmm7360_td_ring_read(struct xmm_dev *xmm, u8 ring_id)
{
	struct td_ring *ring = &xmm->td_ring[ring_id];
	u8 wptr = xmm->cp->s_wptr[ring_id];

	if (!ring->depth) {
		dev_err(xmm->dev, "read on disabled ring\n");
		WARN_ON(1);
		return;
	}
	if (!(ring_id & 1)) {
		dev_err(xmm->dev, "read on write ring\n");
		WARN_ON(1);
		return;
	}

	ring->tds[wptr].length = ring->page_size;
	ring->tds[wptr].flags = 0;
	ring->tds[wptr].unk = 0;

	wptr = (wptr + 1) & (ring->depth - 1);
	BUG_ON(wptr == xmm->cp->s_rptr[ring_id]);

	xmm->cp->s_wptr[ring_id] = wptr;
}

static struct queue_pair * xmm7360_init_qp(struct xmm_dev *xmm, int num, u8 depth, u16 page_size)
{
	struct queue_pair *qp = &xmm->qp[num];

	qp->xmm = xmm;
	qp->num = num;
	qp->open = 0;
	qp->depth = depth;
	qp->page_size = page_size;

	mutex_init(&qp->lock);
	init_waitqueue_head(&qp->wq);
	return qp;
}

static void xmm7360_qp_arm(struct xmm_dev *xmm, struct queue_pair *qp)
{
	while (!xmm7360_td_ring_full(xmm, qp->num*2+1))
		xmm7360_td_ring_read(xmm, qp->num*2+1);
	xmm7360_ding(xmm, DOORBELL_TD);
}

static int xmm7360_qp_start(struct queue_pair *qp)
{
	struct xmm_dev *xmm = qp->xmm;
	int ret;

	mutex_lock(&qp->lock);
	if (qp->open) {
		ret = -EBUSY;
	} else {
		ret = 0;
		qp->open = 1;
	}
	mutex_unlock(&qp->lock);

	if (ret == 0) {
		xmm7360_td_ring_create(xmm, qp->num*2, qp->depth, qp->page_size);
		xmm7360_td_ring_create(xmm, qp->num*2+1, qp->depth, qp->page_size);
		xmm7360_qp_arm(xmm, qp);
	}

	return ret;
}

static void xmm7360_qp_resume(struct queue_pair *qp)
{
	struct xmm_dev *xmm = qp->xmm;

	BUG_ON(!qp->open);
	xmm7360_td_ring_activate(xmm, qp->num*2);
	xmm7360_td_ring_activate(xmm, qp->num*2+1);
	xmm7360_qp_arm(xmm, qp);
}

static int xmm7360_qp_stop(struct queue_pair *qp)
{
	struct xmm_dev *xmm = qp->xmm;
	int ret = 0;

	mutex_lock(&qp->lock);
	if (!qp->open) {
		ret = -ENODEV;
	} else {
		ret = 0;
		/* still holding qp->open to prevent concurrent access */
	}
	mutex_unlock(&qp->lock);

	if (ret == 0) {
		xmm7360_td_ring_destroy(xmm, qp->num*2);
		xmm7360_td_ring_destroy(xmm, qp->num*2+1);

		mutex_lock(&qp->lock);
		qp->open = 0;
		mutex_unlock(&qp->lock);
	}

	return ret;
}

static void xmm7360_qp_suspend(struct queue_pair *qp)
{
	struct xmm_dev *xmm = qp->xmm;

	BUG_ON(!qp->open);
	xmm7360_td_ring_deactivate(xmm, qp->num*2);
}

static int xmm7360_qp_can_write(struct queue_pair *qp)
{
	struct xmm_dev *xmm = qp->xmm;
	return !xmm7360_td_ring_full(xmm, qp->num*2);
}

static ssize_t xmm7360_qp_write(struct queue_pair *qp, const char *buf, size_t size)
{
	struct xmm_dev *xmm = qp->xmm;
	int page_size = qp->xmm->td_ring[qp->num*2].page_size;
	if (xmm->error)
		return xmm->error;
	if (!xmm7360_qp_can_write(qp))
		return 0;
	if (size > page_size)
		size = page_size;
	xmm7360_td_ring_write(xmm, qp->num*2, buf, size);
	xmm7360_ding(xmm, DOORBELL_TD);
	return size;
}

static ssize_t xmm7360_qp_write_user(struct queue_pair *qp, const char __user *buf, size_t size)
{
	int page_size = qp->xmm->td_ring[qp->num*2].page_size;
	int ret;

	if (size > page_size)
		size = page_size;

	ret = copy_from_user(qp->user_buf, buf, size);
	size = size - ret;
	if (!size)
		return 0;
	return xmm7360_qp_write(qp, qp->user_buf, size);
}

static int xmm7360_qp_has_data(struct queue_pair *qp)
{
	struct xmm_dev *xmm = qp->xmm;
	struct td_ring *ring = &xmm->td_ring[qp->num*2+1];

	return (xmm->cp->s_rptr[qp->num*2+1] != ring->last_handled);
}

static ssize_t xmm7360_qp_read_user(struct queue_pair *qp, char __user *buf, size_t size)
{
	struct xmm_dev *xmm = qp->xmm;
	struct td_ring *ring = &xmm->td_ring[qp->num*2+1];
	int idx, nread, ret;
	// XXX locking?
	ret = wait_event_interruptible(qp->wq, xmm7360_qp_has_data(qp) || xmm->error);
	if (ret < 0)
		return ret;
	if (xmm->error)
		return xmm->error;

	idx = ring->last_handled;
	nread = ring->tds[idx].length;
	if (nread > size)
		nread = size;
	ret = copy_to_user(buf, ring->pages[idx], nread);
	nread -= ret;
	if (nread == 0)
		return 0;

	// XXX all data not fitting into buf+size is discarded
	xmm7360_td_ring_read(xmm, qp->num*2+1);
	xmm7360_ding(xmm, DOORBELL_TD);
	ring->last_handled = (idx + 1) & (ring->depth - 1);

	return nread;
}

static void xmm7360_tty_poll_qp(struct queue_pair *qp)
{
	struct xmm_dev *xmm = qp->xmm;
	struct td_ring *ring = &xmm->td_ring[qp->num*2+1];
	int idx, nread;
	while (xmm7360_qp_has_data(qp)) {
		idx = ring->last_handled;
		nread = ring->tds[idx].length;
		xmm7360_os_handle_tty_idata(qp, ring->pages[idx], nread);

		xmm7360_td_ring_read(xmm, qp->num*2+1);
		xmm7360_ding(xmm, DOORBELL_TD);
		ring->last_handled = (idx + 1) & (ring->depth - 1);
	}
}

#ifdef __linux__

static void xmm7360_os_handle_tty_idata(struct queue_pair *qp, const u8 *data, size_t nread)
{
	tty_insert_flip_string(&qp->port, data, nread);
	tty_flip_buffer_push(&qp->port);
}

int xmm7360_cdev_open (struct inode *inode, struct file *file)
{
	struct queue_pair *qp = container_of(inode->i_cdev, struct queue_pair, cdev);
	file->private_data = qp;
	return xmm7360_qp_start(qp);
}

int xmm7360_cdev_release (struct inode *inode, struct file *file)
{
	struct queue_pair *qp = file->private_data;
	return xmm7360_qp_stop(qp);
}

ssize_t xmm7360_cdev_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	struct queue_pair *qp = file->private_data;
	int ret;

	ret = xmm7360_qp_write_user(qp, buf, size);
	if (ret < 0)
		return ret;

	*offset += ret;
	return ret;
}

ssize_t xmm7360_cdev_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	struct queue_pair *qp = file->private_data;
	int ret;

	ret = xmm7360_qp_read_user(qp, buf, size);
	if (ret < 0)
		return ret;

	*offset += ret;
	return ret;
}

static unsigned int xmm7360_cdev_poll(struct file *file, poll_table *wait)
{
	struct queue_pair *qp = file->private_data;
	unsigned int mask = 0;

	poll_wait(file, &qp->wq, wait);

	if (qp->xmm->error)
		return POLLHUP;

	if (xmm7360_qp_has_data(qp))
		mask |= POLLIN | POLLRDNORM;

	if (xmm7360_qp_can_write(qp))
		mask |= POLLOUT | POLLWRNORM;

	return mask;
}

static long xmm7360_cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct queue_pair *qp = file->private_data;

	u32 val;

	switch (cmd) {
		case XMM7360_IOCTL_GET_PAGE_SIZE:
			val = qp->xmm->td_ring[qp->num*2].page_size;
			if (copy_to_user((u32*)arg, &val, sizeof(u32)))
				return -EFAULT;
			return 0;
	}

	return -ENOTTY;
}

static struct file_operations xmm7360_fops = {
	.read		= xmm7360_cdev_read,
	.write		= xmm7360_cdev_write,
	.poll		= xmm7360_cdev_poll,
	.unlocked_ioctl	= xmm7360_cdev_ioctl,
	.open		= xmm7360_cdev_open,
	.release	= xmm7360_cdev_release
};

#endif /* __linux__ */

static void xmm7360_mux_frame_init(struct xmm_net *xn, struct mux_frame *frame, int sequence)
{
	frame->sequence = xn->sequence;
	frame->max_size = xn->xmm->td_ring[0].page_size;
	frame->n_packets = 0;
	frame->n_bytes = 0;
	frame->last_tag_next = NULL;
	frame->last_tag_length = NULL;
}

static void xmm7360_mux_frame_add_tag(struct mux_frame *frame, uint32_t tag, uint16_t extra, void *data, int data_len)
{
	int total_length;
	if (frame->n_bytes == 0)
		total_length = sizeof(struct mux_first_header) + data_len;
	else
		total_length = sizeof(struct mux_next_header) + data_len;

	while (frame->n_bytes & 3)
		frame->n_bytes++;

	BUG_ON(frame->n_bytes + total_length > frame->max_size);

	if (frame->last_tag_next)
		*frame->last_tag_next = frame->n_bytes;

	if (frame->n_bytes == 0) {
		struct mux_first_header *hdr = (struct mux_first_header *)frame->data;
		memset(hdr, 0, sizeof(struct mux_first_header));
		hdr->tag = htonl(tag);
		hdr->sequence = frame->sequence;
		hdr->length = total_length;
		hdr->extra = extra;
		frame->last_tag_length = &hdr->length;
		frame->last_tag_next = &hdr->next;
		frame->n_bytes += sizeof(struct mux_first_header);
	} else {
		struct mux_next_header *hdr = (struct mux_next_header *)(&frame->data[frame->n_bytes]);
		memset(hdr, 0, sizeof(struct mux_next_header));
		hdr->tag = htonl(tag);
		hdr->length = total_length;
		hdr->extra = extra;
		frame->last_tag_length = &hdr->length;
		frame->last_tag_next = &hdr->next;
		frame->n_bytes += sizeof(struct mux_next_header);
	}

	if (data_len) {
		memcpy(&frame->data[frame->n_bytes], data, data_len);
		frame->n_bytes += data_len;
	}
}

static void xmm7360_mux_frame_append_data(struct mux_frame *frame, const void *data, int data_len)
{
	BUG_ON(frame->n_bytes + data_len > frame->max_size);
	BUG_ON(!frame->last_tag_length);

	memcpy(&frame->data[frame->n_bytes], data, data_len);
	*frame->last_tag_length += data_len;
	frame->n_bytes += data_len;
}

static int xmm7360_mux_frame_append_packet(struct mux_frame *frame, const void *data, int data_len)
{
	int expected_adth_size = sizeof(struct mux_next_header) + 4 + (frame->n_packets+1)*sizeof(struct mux_bounds);
	uint8_t pad[16];

	if (frame->n_packets >= MUX_MAX_PACKETS)
		return -1;

	if (frame->n_bytes + data_len + 16 + expected_adth_size > frame->max_size)
		return -1;

	BUG_ON(!frame->last_tag_length);

	frame->bounds[frame->n_packets].offset = frame->n_bytes;
	frame->bounds[frame->n_packets].length = data_len + 16;
	frame->n_packets++;

	memset(pad, 0, sizeof(pad));
	xmm7360_mux_frame_append_data(frame, pad, 16);
	xmm7360_mux_frame_append_data(frame, data, data_len);
	return 0;
}

static int xmm7360_mux_frame_push(struct xmm_dev *xmm, struct mux_frame *frame)
{
	struct mux_first_header *hdr = (void*)&frame->data[0];
	int ret;
	hdr->length = frame->n_bytes;

	ret = xmm7360_qp_write(xmm->net->qp, frame->data, frame->n_bytes);
	if (ret < 0)
		return ret;
	return 0;
}

static int xmm7360_mux_control(struct xmm_net *xn, u32 arg1, u32 arg2, u32 arg3, u32 arg4)
{
	struct mux_frame *frame = &xn->frame;
	int ret;
	uint32_t cmdh_args[] = {arg1, arg2, arg3, arg4};
	unsigned long flags __unused;

	spin_lock_irqsave(&xn->lock, flags);

	xmm7360_mux_frame_init(xn, frame, 0);
	xmm7360_mux_frame_add_tag(frame, XMM_TAG_ACBH, 0, NULL, 0);
	xmm7360_mux_frame_add_tag(frame, XMM_TAG_CMDH, xn->channel, cmdh_args, sizeof(cmdh_args));
	ret = xmm7360_mux_frame_push(xn->xmm, frame);

	spin_unlock_irqrestore(&xn->lock, flags);

	return ret;
}

static void xmm7360_net_flush(struct xmm_net *xn)
{
	struct mux_frame *frame = &xn->frame;
	int ret;
	u32 unknown = 0;

#ifdef __linux__
	/* Never called with empty queue */
	BUG_ON(skb_queue_empty(&xn->queue));
#endif
	BUG_ON(!xmm7360_qp_can_write(xn->qp));

	xmm7360_mux_frame_init(xn, frame, xn->sequence++);
	xmm7360_mux_frame_add_tag(frame, XMM_TAG_ADBH, 0, NULL, 0);

	xmm7360_os_handle_net_dequeue(xn, frame);
	xn->queued_packets = xn->queued_bytes = 0;

	xmm7360_mux_frame_add_tag(frame, XMM_TAG_ADTH, xn->channel, &unknown, sizeof(uint32_t));
	xmm7360_mux_frame_append_data(frame, &frame->bounds[0], sizeof(struct mux_bounds)*frame->n_packets);

	ret = xmm7360_mux_frame_push(xn->xmm, frame);
	if (ret)
		goto drop;

	return;

drop:
	dev_err(xn->xmm->dev, "Failed to ship coalesced frame");
}

static int xmm7360_base_init(struct xmm_dev *xmm)
{
	int ret, i;
	u32 status;

	xmm->error = 0;
	xmm->num_ttys = 0;

	status = xmm->bar2[BAR2_STATUS];
	if (status == XMM_MODEM_BOOTING) {
		dev_info(xmm->dev, "modem still booting, waiting...\n");
		for (i=0; i<100; i++) {
			status = xmm->bar2[BAR2_STATUS];
			if (status != XMM_MODEM_BOOTING)
				break;
			xmm7360_os_msleep(200);
		}
	}

	if (status != XMM_MODEM_READY) {
		dev_err(xmm->dev, "unknown modem status: 0x%08x\n", status);
		return -EINVAL;
	}

	dev_info(xmm->dev, "modem is ready\n");

	ret = xmm7360_cmd_ring_init(xmm);
	if (ret) {
		dev_err(xmm->dev, "Could not bring up command ring %d\n",
		    ret);
		return ret;
	}

	return 0;
}

static void xmm7360_net_mux_handle_frame(struct xmm_net *xn, u8 *data, int len)
{
	struct mux_first_header *first;
	struct mux_next_header *adth;
	int n_packets, i;
	struct mux_bounds *bounds;

	first = (void*)data;
	if (ntohl(first->tag) == XMM_TAG_ACBH)
		return;

	if (ntohl(first->tag) != XMM_TAG_ADBH) {
		dev_info(xn->xmm->dev, "Unexpected tag %x\n", first->tag);
		return;
	}

	adth = (void*)(&data[first->next]);
	if (ntohl(adth->tag) != XMM_TAG_ADTH) {
		dev_err(xn->xmm->dev, "Unexpected tag %x, expected ADTH\n", adth->tag);
		return;
	}

	n_packets = (adth->length - sizeof(struct mux_next_header) - 4) / sizeof(struct mux_bounds);

	bounds = (void*)&data[first->next + sizeof(struct mux_next_header) + 4];

	for (i=0; i<n_packets; i++) {
		if (!bounds[i].length)
			continue;

		xmm7360_os_handle_net_frame(xn->xmm,
		    &data[bounds[i].offset], bounds[i].length);
	}
}

static void xmm7360_net_poll(struct xmm_dev *xmm)
{
	struct queue_pair *qp;
	struct td_ring *ring;
	int idx, nread;
	struct xmm_net *xn = xmm->net;
	unsigned long flags __unused;

	BUG_ON(!xn);

	qp = xn->qp;
	ring = &xmm->td_ring[qp->num*2+1];

	spin_lock_irqsave(&xn->lock, flags);

	if (xmm7360_qp_can_write(qp))
		xmm7360_os_handle_net_txwake(xn);

	while (xmm7360_qp_has_data(qp)) {
		idx = ring->last_handled;
		nread = ring->tds[idx].length;
		xmm7360_net_mux_handle_frame(xn, ring->pages[idx], nread);

		xmm7360_td_ring_read(xmm, qp->num*2+1);
		xmm7360_ding(xmm, DOORBELL_TD);
		ring->last_handled = (idx + 1) & (ring->depth - 1);
	}

	spin_unlock_irqrestore(&xn->lock, flags);
}

#ifdef __linux__

static void xmm7360_net_uninit(struct net_device *dev)
{
}

static int xmm7360_net_open(struct net_device *dev)
{
	struct xmm_net *xn = netdev_priv(dev);
	xn->queued_packets = xn->queued_bytes = 0;
	skb_queue_purge(&xn->queue);
	netif_start_queue(dev);
	return xmm7360_mux_control(xn, 1, 0, 0, 0);
}

static int xmm7360_net_close(struct net_device *dev)
{
	netif_stop_queue(dev);
	return 0;
}

static int xmm7360_net_must_flush(struct xmm_net *xn, int new_packet_bytes)
{
	int frame_size;
	if (xn->queued_packets >= MUX_MAX_PACKETS)
		return 1;

	frame_size = sizeof(struct mux_first_header) + xn->queued_bytes + sizeof(struct mux_next_header) + 4 + sizeof(struct mux_bounds)*xn->queued_packets;

	frame_size += 16 + new_packet_bytes + sizeof(struct mux_bounds);

	return frame_size > xn->frame.max_size;
}

static enum hrtimer_restart xmm7360_net_deadline_cb(struct hrtimer *t)
{
	struct xmm_net *xn = container_of(t, struct xmm_net, deadline);
	unsigned long flags;
	spin_lock_irqsave(&xn->lock, flags);
	if (!skb_queue_empty(&xn->queue) && xmm7360_qp_can_write(xn->qp))
		xmm7360_net_flush(xn);
	spin_unlock_irqrestore(&xn->lock, flags);
	return HRTIMER_NORESTART;
}

static netdev_tx_t xmm7360_net_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct xmm_net *xn = netdev_priv(dev);
	ktime_t kt;
	unsigned long flags;

	if (netif_queue_stopped(dev))
		return NETDEV_TX_BUSY;

	skb_orphan(skb);

	spin_lock_irqsave(&xn->lock, flags);
	if (xmm7360_net_must_flush(xn, skb->len)) {
		if (xmm7360_qp_can_write(xn->qp)) {
			xmm7360_net_flush(xn);
		} else {
			netif_stop_queue(dev);
			spin_unlock_irqrestore(&xn->lock, flags);
			return NETDEV_TX_BUSY;
		}
	}

	xn->queued_packets++;
	xn->queued_bytes += 16 + skb->len;
	skb_queue_tail(&xn->queue, skb);

	spin_unlock_irqrestore(&xn->lock, flags);

	if (!hrtimer_active(&xn->deadline)) {
		kt = ktime_set(0, 100000);
		hrtimer_start(&xn->deadline, kt, HRTIMER_MODE_REL);
	}

	return NETDEV_TX_OK;
}

static void xmm7360_os_handle_net_frame(struct xmm_dev *xmm, const u8 *buf, size_t sz)
{
	struct sk_buff *skb;
	void *p;
	u8 ip_version;

	skb = dev_alloc_skb(sz + NET_IP_ALIGN);
	if (!skb)
		return;
	skb_reserve(skb, NET_IP_ALIGN);
	p = skb_put(skb, sz);
	memcpy(p, buf, sz);

	skb->dev = xmm->netdev;

	ip_version = skb->data[0] >> 4;
	if (ip_version == 4) {
		skb->protocol = htons(ETH_P_IP);
	} else if (ip_version == 6) {
		skb->protocol = htons(ETH_P_IPV6);
	} else {
		kfree_skb(skb);
		return;
	}

	netif_rx(skb);
}

static void xmm7360_os_handle_net_dequeue(struct xmm_net *xn, struct mux_frame *frame)
{
	struct sk_buff *skb;
	int ret;

	while ((skb = skb_dequeue(&xn->queue))) {
		ret = xmm7360_mux_frame_append_packet(frame,
		    skb->data, skb->len);
		kfree_skb(skb);
		if (ret) {
			/* No more space in the frame */
			break;
		}
	}
}

static void xmm7360_os_handle_net_txwake(struct xmm_net *xn)
{
	BUG_ON(!xmm7360_qp_can_write(xn->qp));

	if (netif_queue_stopped(xn->xmm->netdev))
		netif_wake_queue(xn->xmm->netdev);
}

static const struct net_device_ops xmm7360_netdev_ops = {
	.ndo_uninit		= xmm7360_net_uninit,
	.ndo_open		= xmm7360_net_open,
	.ndo_stop		= xmm7360_net_close,
	.ndo_start_xmit		= xmm7360_net_xmit,
};

static void xmm7360_net_setup(struct net_device *dev)
{
	struct xmm_net *xn = netdev_priv(dev);
	spin_lock_init(&xn->lock);
	hrtimer_init(&xn->deadline, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	xn->deadline.function = xmm7360_net_deadline_cb;
	skb_queue_head_init(&xn->queue);

	dev->netdev_ops = &xmm7360_netdev_ops;

	dev->hard_header_len = 0;
	dev->addr_len = 0;
	dev->mtu = 1500;
	dev->min_mtu = 1500;
	dev->max_mtu = 1500;

	dev->tx_queue_len = 1000;

	dev->type = ARPHRD_NONE;
	dev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST;
}

static int xmm7360_create_net(struct xmm_dev *xmm, int num)
{
	struct net_device *netdev;
	struct xmm_net *xn;
	int ret;

	netdev = alloc_netdev(sizeof(struct xmm_net), "wwan%d", NET_NAME_UNKNOWN, xmm7360_net_setup);

	if (!netdev)
		return -ENOMEM;

	SET_NETDEV_DEV(netdev, xmm->dev);

	xmm->netdev = netdev;

	xn = netdev_priv(netdev);
	xn->xmm = xmm;
	xmm->net = xn;

	rtnl_lock();
	ret = register_netdevice(netdev);
	rtnl_unlock();

	xn->qp = xmm7360_init_qp(xmm, num, 128, TD_MAX_PAGE_SIZE);

	if (!ret)
		ret = xmm7360_qp_start(xn->qp);

	if (ret < 0) {
		free_netdev(netdev);
		xmm->netdev = NULL;
		xmm7360_qp_stop(xn->qp);
	}

	return ret;
}

static void xmm7360_destroy_net(struct xmm_dev *xmm)
{
	if (xmm->netdev) {
		xmm7360_qp_stop(xmm->net->qp);
		rtnl_lock();
		unregister_netdevice(xmm->netdev);
		rtnl_unlock();
		free_netdev(xmm->netdev);
		xmm->net = NULL;
		xmm->netdev = NULL;
	}
}

static irqreturn_t xmm7360_irq0(int irq, void *dev_id) {
	struct xmm_dev *xmm = dev_id;
	struct queue_pair *qp;
	int id;

	xmm7360_poll(xmm);
	wake_up(&xmm->wq);
	if (xmm->td_ring) {
		if (xmm->net)
			xmm7360_net_poll(xmm);

		for (id=1; id<XMM_QP_COUNT; id++) {
			qp = &xmm->qp[id];

			/* wake _cdev_read() */
			if (qp->open)
				wake_up(&qp->wq);

			/* tty tasks */
			if (qp->open && qp->port.ops) {
				xmm7360_tty_poll_qp(qp);
				if (qp->tty_needs_wake && xmm7360_qp_can_write(qp) && qp->port.tty) {
					struct tty_ldisc *ldisc = tty_ldisc_ref(qp->port.tty);
					if (ldisc) {
						if (ldisc->ops->write_wakeup)
							ldisc->ops->write_wakeup(qp->port.tty);
						tty_ldisc_deref(ldisc);
					}
					qp->tty_needs_wake = 0;
				}
			}
		}
	}

	return IRQ_HANDLED;
}

static dev_t xmm_base;

static struct tty_driver *xmm7360_tty_driver;

static void xmm7360_dev_deinit(struct xmm_dev *xmm)
{
	int i;
	xmm->error = -ENODEV;

	cancel_work_sync(&xmm->init_work);

	xmm7360_destroy_net(xmm);

	for (i=0; i<XMM_QP_COUNT; i++) {
		if (xmm->qp[i].xmm) {
			if (xmm->qp[i].cdev.owner) {
				cdev_del(&xmm->qp[i].cdev);
				device_unregister(&xmm->qp[i].dev);
			}
			if (xmm->qp[i].port.ops) {
				tty_unregister_device(xmm7360_tty_driver, xmm->qp[i].tty_index);
				tty_port_destroy(&xmm->qp[i].port);
			}
		}
		memset(&xmm->qp[i], 0, sizeof(struct queue_pair));
	}
	xmm7360_cmd_ring_free(xmm);

}

static void xmm7360_remove(struct pci_dev *dev)
{
	struct xmm_dev *xmm = pci_get_drvdata(dev);

	xmm7360_dev_deinit(xmm);

	if (xmm->irq)
		free_irq(xmm->irq, xmm);
	pci_free_irq_vectors(dev);
	pci_release_region(dev, 0);
	pci_release_region(dev, 2);
	pci_disable_device(dev);
	kfree(xmm);
}

static void xmm7360_cdev_dev_release(struct device *dev)
{
}

static int xmm7360_tty_open(struct tty_struct *tty, struct file *filp)
{
	struct queue_pair *qp = tty->driver_data;
	return tty_port_open(&qp->port, tty, filp);
}

static void xmm7360_tty_close(struct tty_struct *tty, struct file *filp)
{
	struct queue_pair *qp = tty->driver_data;
	if (qp)
		tty_port_close(&qp->port, tty, filp);
}

static int xmm7360_tty_write(struct tty_struct *tty, const unsigned char *buffer,
		      int count)
{
	struct queue_pair *qp = tty->driver_data;
	int written;
	written = xmm7360_qp_write(qp, buffer, count);
	if (written < count)
		qp->tty_needs_wake = 1;
	return written;
}

static int xmm7360_tty_write_room(struct tty_struct *tty)
{
	struct queue_pair *qp = tty->driver_data;
	if (!xmm7360_qp_can_write(qp))
		return 0;
	else
		return qp->xmm->td_ring[qp->num*2].page_size;
}

static int xmm7360_tty_install(struct tty_driver *driver, struct tty_struct *tty)
{
	struct queue_pair *qp;
	int ret;

	ret = tty_standard_install(driver, tty);
	if (ret)
		return ret;

	tty->port = driver->ports[tty->index];
	qp = container_of(tty->port, struct queue_pair, port);
	tty->driver_data = qp;
	return 0;
}


static int xmm7360_tty_port_activate(struct tty_port *tport, struct tty_struct *tty)
{
	struct queue_pair *qp = tty->driver_data;
	return xmm7360_qp_start(qp);
}

static void xmm7360_tty_port_shutdown(struct tty_port *tport)
{
	struct queue_pair *qp = tport->tty->driver_data;
	xmm7360_qp_stop(qp);
}


static const struct tty_port_operations xmm7360_tty_port_ops = {
	.activate = xmm7360_tty_port_activate,
	.shutdown = xmm7360_tty_port_shutdown,
};

static const struct tty_operations xmm7360_tty_ops = {
	.open = xmm7360_tty_open,
	.close = xmm7360_tty_close,
	.write = xmm7360_tty_write,
	.write_room = xmm7360_tty_write_room,
	.install = xmm7360_tty_install,
};

static int xmm7360_create_tty(struct xmm_dev *xmm, int num)
{
	struct device *tty_dev;
	struct queue_pair *qp = xmm7360_init_qp(xmm, num, 8, 4096);
	int ret;
	tty_port_init(&qp->port);
	qp->port.low_latency = 1;
	qp->port.ops = &xmm7360_tty_port_ops;
	qp->tty_index = xmm->num_ttys++;
	tty_dev = tty_port_register_device(&qp->port, xmm7360_tty_driver, qp->tty_index, xmm->dev);

	if (IS_ERR(tty_dev)) {
		qp->port.ops = NULL;	// prevent calling unregister
		ret = PTR_ERR(tty_dev);
		dev_err(xmm->dev, "Could not allocate tty?\n");
		tty_port_destroy(&qp->port);
		return ret;
	}

	return 0;
}

static int xmm7360_create_cdev(struct xmm_dev *xmm, int num, const char *name, int cardnum)
{
	struct queue_pair *qp = xmm7360_init_qp(xmm, num, 16, TD_MAX_PAGE_SIZE);
	int ret;

	cdev_init(&qp->cdev, &xmm7360_fops);
	qp->cdev.owner = THIS_MODULE;
	device_initialize(&qp->dev);
	qp->dev.devt = MKDEV(MAJOR(xmm_base), num); // XXX multiple cards
	qp->dev.parent = &xmm->pci_dev->dev;
	qp->dev.release = xmm7360_cdev_dev_release;
	dev_set_name(&qp->dev, name, cardnum);
	dev_set_drvdata(&qp->dev, qp);
	ret = cdev_device_add(&qp->cdev, &qp->dev);
	if (ret) {
		dev_err(xmm->dev, "cdev_device_add: %d\n", ret);
		return ret;
	}
	return 0;
}

static int xmm7360_dev_init(struct xmm_dev *xmm)
{
	int ret;

	ret = xmm7360_base_init(xmm);
	if (ret)
		return ret;

	ret = xmm7360_create_cdev(xmm, 1, "xmm%d/rpc", xmm->card_num);
	if (ret)
		return ret;
	ret = xmm7360_create_cdev(xmm, 3, "xmm%d/trace", xmm->card_num);
	if (ret)
		return ret;
	ret = xmm7360_create_tty(xmm, 2);
	if (ret)
		return ret;
	ret = xmm7360_create_tty(xmm, 4);
	if (ret)
		return ret;
	ret = xmm7360_create_tty(xmm, 7);
	if (ret)
		return ret;
	ret = xmm7360_create_net(xmm, 0);
	if (ret)
		return ret;

	return 0;
}

void xmm7360_dev_init_work(struct work_struct *work)
{
	struct xmm_dev *xmm = container_of(work, struct xmm_dev, init_work);
	xmm7360_dev_init(xmm);
}

static int xmm7360_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	struct xmm_dev *xmm = kzalloc(sizeof(struct xmm_dev), GFP_KERNEL);
	int ret;

	xmm->pci_dev = dev;
	xmm->dev = &dev->dev;

	if (!xmm) {
		dev_err(&(dev->dev), "kzalloc\n");
		return -ENOMEM;
	}

	ret = pci_enable_device(dev);
	if (ret) {
		dev_err(&(dev->dev), "pci_enable_device\n");
		goto fail;
	}
	pci_set_master(dev);

	ret = pci_set_dma_mask(dev, 0xffffffffffffffff);
	if (ret) {
		dev_err(xmm->dev, "Cannot set DMA mask\n");
		goto fail;
	}
	dma_set_coherent_mask(xmm->dev, 0xffffffffffffffff);


	ret = pci_request_region(dev, 0, "xmm0");
	if (ret) {
		dev_err(&(dev->dev), "pci_request_region(0)\n");
		goto fail;
	}
	xmm->bar0 = pci_iomap(dev, 0, pci_resource_len(dev, 0));

	ret = pci_request_region(dev, 2, "xmm2");
	if (ret) {
		dev_err(&(dev->dev), "pci_request_region(2)\n");
		goto fail;
	}
	xmm->bar2 = pci_iomap(dev, 2, pci_resource_len(dev, 2));

	ret = pci_alloc_irq_vectors(dev, 1, 1, PCI_IRQ_MSI | PCI_IRQ_MSIX);
	if (ret < 0) {
		dev_err(&(dev->dev), "pci_alloc_irq_vectors\n");
		goto fail;
	}

	init_waitqueue_head(&xmm->wq);
	INIT_WORK(&xmm->init_work, xmm7360_dev_init_work);

	pci_set_drvdata(dev, xmm);

	ret = xmm7360_dev_init(xmm);
	if (ret)
		goto fail;

	xmm->irq = pci_irq_vector(dev, 0);
	ret = request_irq(xmm->irq, xmm7360_irq0, 0, "xmm7360", xmm);
	if (ret) {
		dev_err(&(dev->dev), "request_irq\n");
		goto fail;
	}

	return ret;

fail:
	xmm7360_dev_deinit(xmm);
	xmm7360_remove(dev);
	return ret;
}

static struct pci_driver xmm7360_driver = {
	.name		= "xmm7360",
	.id_table	= xmm7360_ids,
	.probe		= xmm7360_probe,
	.remove		= xmm7360_remove,
};

static int xmm7360_init(void)
{
	int ret;
	ret = alloc_chrdev_region(&xmm_base, 0, 8, "xmm");
	if (ret)
		return ret;

	xmm7360_tty_driver = alloc_tty_driver(8);
	if (!xmm7360_tty_driver)
		return -ENOMEM;

	xmm7360_tty_driver->driver_name = "xmm7360";
	xmm7360_tty_driver->name = "ttyXMM";
	xmm7360_tty_driver->major = 0;
	xmm7360_tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
	xmm7360_tty_driver->subtype = SERIAL_TYPE_NORMAL;
	xmm7360_tty_driver->flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;
	xmm7360_tty_driver->init_termios = tty_std_termios;
	xmm7360_tty_driver->init_termios.c_cflag = B115200 | CS8 | CREAD | \
						HUPCL | CLOCAL;
	xmm7360_tty_driver->init_termios.c_lflag &= ~ECHO;
	xmm7360_tty_driver->init_termios.c_ispeed = 115200;
	xmm7360_tty_driver->init_termios.c_ospeed = 115200;
	tty_set_operations(xmm7360_tty_driver, &xmm7360_tty_ops);

	ret = tty_register_driver(xmm7360_tty_driver);
	if (ret) {
		pr_err("xmm7360: failed to register xmm7360_tty driver\n");
		return ret;
	}


	ret = pci_register_driver(&xmm7360_driver);
	if (ret)
		return ret;

	return 0;
}

static void xmm7360_exit(void)
{
	pci_unregister_driver(&xmm7360_driver);
	unregister_chrdev_region(xmm_base, 8);
	tty_unregister_driver(xmm7360_tty_driver);
	put_tty_driver(xmm7360_tty_driver);
}

module_init(xmm7360_init);
module_exit(xmm7360_exit);

#endif /* __linux__ */

#if defined(__OpenBSD__) || defined(__NetBSD__)

/*
 * RPC and trace devices behave as regular character device,
 * other devices behave as terminal.
 */
#define DEVCUA(x)	(minor(x) & 0x80)
#define DEVUNIT(x)	((minor(x) & 0x70) >> 4)
#define DEVFUNC_MASK	0x0f
#define DEVFUNC(x)	(minor(x) & DEVFUNC_MASK)
#define DEV_IS_TTY(x)	(DEVFUNC(x) == 2 || DEVFUNC(x) > 3)

struct wwanc_softc {
#ifdef __OpenBSD__
	struct device		sc_devx;	/* gen. device info storage */
#endif
	struct device		*sc_dev;	/* generic device information */
        pci_chipset_tag_t       sc_pc;
        pcitag_t                sc_tag;
	bus_dma_tag_t		sc_dmat;
	pci_intr_handle_t	sc_pih;
        void                    *sc_ih;         /* interrupt vectoring */

	bus_space_tag_t		sc_bar0_tag;
	bus_space_handle_t	sc_bar0_handle;
	bus_size_t		sc_bar0_sz;
	bus_space_tag_t		sc_bar2_tag;
	bus_space_handle_t	sc_bar2_handle;
	bus_size_t		sc_bar2_sz;

	struct xmm_dev		sc_xmm;
	struct tty		*sc_tty[XMM_QP_COUNT];
	struct device		*sc_net;
	struct selinfo		sc_selr, sc_selw;
	bool			sc_resume;
};

struct wwanc_attach_args {
	enum wwanc_type {
		WWMC_TYPE_RPC,
		WWMC_TYPE_TRACE,
		WWMC_TYPE_TTY,
		WWMC_TYPE_NET
	} aa_type;
};

static int     wwanc_match(struct device *, cfdata_t, void *);
static void    wwanc_attach(struct device *, struct device *, void *);
static int     wwanc_detach(struct device *, int);

#ifdef __OpenBSD__
static int     wwanc_activate(struct device *, int);

struct cfattach wwanc_ca = {
        sizeof(struct wwanc_softc), wwanc_match, wwanc_attach,
        wwanc_detach, wwanc_activate
};

struct cfdriver wwanc_cd = {
        NULL, "wwanc", DV_DULL
};
#endif

#ifdef __NetBSD__
CFATTACH_DECL3_NEW(wwanc, sizeof(struct wwanc_softc),
   wwanc_match, wwanc_attach, wwanc_detach, NULL,
   NULL, NULL, DVF_DETACH_SHUTDOWN);

static bool wwanc_pmf_suspend(device_t, const pmf_qual_t *);
static bool wwanc_pmf_resume(device_t, const pmf_qual_t *);
#endif /* __NetBSD__ */

static int
wwanc_match(struct device *parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	return (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_INTEL &&
		PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_XMM7360);
}

static int xmm7360_dev_init(struct xmm_dev *xmm)
{
	int ret;
	int depth, page_size;

	ret = xmm7360_base_init(xmm);
	if (ret)
		return ret;

	/* Initialize queue pairs for later use */
	for (int num = 0; num < XMM_QP_COUNT; num++) {
		switch (num) {
		case 0:	/* net */
			depth = 128;
			page_size = TD_MAX_PAGE_SIZE;
			break;
		case 1:	/* rpc */
		case 3: /* trace */
			depth = 16;
			page_size = TD_MAX_PAGE_SIZE;
			break;
		default: /* tty */
			depth = 8;
			page_size = 4096;
			break;
		}

		xmm7360_init_qp(xmm, num, depth, page_size);
	}

	return 0;
}

static void xmm7360_dev_deinit(struct xmm_dev *xmm)
{
	struct wwanc_softc *sc = device_private(xmm->dev);
	bool devgone = false;
	struct tty *tp;

	xmm->error = -ENODEV;

	/* network device should be gone by now */
	KASSERT(sc->sc_net == NULL);
	KASSERT(xmm->net == NULL);

	/* free ttys */
	for (int i=0; i<XMM_QP_COUNT; i++) {
		tp = sc->sc_tty[i];
		if (tp) {
			KASSERT(DEV_IS_TTY(i));
			if (!devgone) {
				vdevgone(major(tp->t_dev), 0, DEVFUNC_MASK,
				    VCHR);
				devgone = true;
			}
			ttyfree(tp);
			sc->sc_tty[i] = NULL;
		}
	}

	xmm7360_cmd_ring_free(xmm);
}

static void
wwanc_io_wakeup(struct queue_pair *qp, int flag)
{
        if (flag & FREAD) {
                selnotify(&qp->selr, POLLIN|POLLRDNORM, NOTE_SUBMIT);
                wakeup(qp->wq);
        }
        if (flag & FWRITE) {
                selnotify(&qp->selw, POLLOUT|POLLWRNORM, NOTE_SUBMIT);
                wakeup(qp->wq);
        }
}

static int
wwanc_intr(void *xsc)
{
	struct wwanc_softc *sc = xsc;
	struct xmm_dev *xmm = &sc->sc_xmm;
	struct queue_pair *qp;

	xmm7360_poll(xmm);
	wakeup(&xmm->wq);

	if (xmm->net && xmm->net->qp->open && xmm7360_qp_has_data(xmm->net->qp))
		xmm7360_net_poll(xmm);

	for (int func = 1; func < XMM_QP_COUNT; func++) {
		qp = &xmm->qp[func];
		if (!qp->open)
			continue;

		/* Check for input, wwancstart()/wwancwrite() does output */
		if (xmm7360_qp_has_data(qp)) {
			if (DEV_IS_TTY(func)) {
				int s = spltty();
				xmm7360_tty_poll_qp(qp);
				splx(s);
			}
			wwanc_io_wakeup(qp, FREAD);
		}

		/* Wakeup/notify eventual writers */
		if (xmm7360_qp_can_write(qp))
			wwanc_io_wakeup(qp, FWRITE);
	}

	return 1;
}

static int
wwancprint(void *aux, const char *pnp)
{
	struct wwanc_attach_args *wa = aux;

	if (pnp)
                printf("wwanc type %s at %s",
		    (wa->aa_type == WWMC_TYPE_NET) ? "net" : "unk", pnp);
	else
		printf(" type %s",
		    (wa->aa_type == WWMC_TYPE_NET) ? "net" : "unk");

	return (UNCONF);
}

static void
wwanc_attach_finish(struct device *self)
{
	struct wwanc_softc *sc = device_private(self);

	if (xmm7360_dev_init(&sc->sc_xmm)) {
		/* error already printed */
		return;
	}

	/* Attach the network device */
	struct wwanc_attach_args wa;
	memset(&wa, 0, sizeof(wa));
	wa.aa_type = WWMC_TYPE_NET;
	sc->sc_net = config_found(self, &wa, wwancprint, CFARGS_NONE);
}

static void
wwanc_attach(struct device *parent, struct device *self, void *aux)
{
	struct wwanc_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	bus_space_tag_t memt;
	bus_space_handle_t memh;
	bus_size_t sz;
	int error;
	const char *intrstr;
#ifdef __OpenBSD__
	pci_intr_handle_t ih;
#endif
#ifdef __NetBSD__
	pci_intr_handle_t *ih;
	char intrbuf[PCI_INTRSTR_LEN];
#endif

	sc->sc_dev = self;
	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;
	sc->sc_dmat = pa->pa_dmat;

	/* map the register window, memory mapped 64-bit non-prefetchable */
	error = pci_mapreg_map(pa, WWAN_BAR0,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_64BIT,
	    BUS_SPACE_MAP_LINEAR, &memt, &memh, NULL, &sz, 0);
	if (error != 0) {
		printf(": can't map mem space for BAR0 %d\n", error);
		return;
	}
	sc->sc_bar0_tag = memt;
	sc->sc_bar0_handle = memh;
	sc->sc_bar0_sz = sz;

	error = pci_mapreg_map(pa, WWAN_BAR2,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_64BIT,
	    BUS_SPACE_MAP_LINEAR, &memt, &memh, NULL, &sz, 0);
	if (error != 0) {
		bus_space_unmap(sc->sc_bar0_tag, sc->sc_bar0_handle,
		    sc->sc_bar0_sz);
		printf(": can't map mem space for BAR2\n");
		return;
	}
	sc->sc_bar2_tag = memt;
	sc->sc_bar2_handle = memh;
	sc->sc_bar2_sz = sz;

	/* Set xmm members needed for xmm7360_dev_init() */
	sc->sc_xmm.dev = self;
	sc->sc_xmm.bar0 = bus_space_vaddr(sc->sc_bar0_tag, sc->sc_bar0_handle);
	sc->sc_xmm.bar2 = bus_space_vaddr(sc->sc_bar0_tag, sc->sc_bar2_handle);
	init_waitqueue_head(&sc->sc_xmm.wq);

#ifdef __OpenBSD__
	if (pci_intr_map_msi(pa, &ih) && pci_intr_map(pa, &ih)) {
		printf(": can't map interrupt\n");
		goto fail;
	}
	sc->sc_pih = ih;
	intrstr = pci_intr_string(sc->sc_pc, ih);
	printf(": %s\n", intrstr);
#endif
#ifdef __NetBSD__
	if (pci_intr_alloc(pa, &ih, NULL, 0)) {
		printf(": can't map interrupt\n");
		goto fail;
	}
	sc->sc_pih = ih[0];
	intrstr = pci_intr_string(pa->pa_pc, ih[0], intrbuf, sizeof(intrbuf));
	aprint_normal(": LTE modem\n");
	aprint_normal_dev(sc->sc_dev, "interrupting at %s\n", intrstr);
#endif

	/* Device initialized, can establish the interrupt now */
	sc->sc_ih = pci_intr_establish(sc->sc_pc, sc->sc_pih, IPL_NET,
	    wwanc_intr, sc, device_xname(sc->sc_dev));
	if (sc->sc_ih == NULL) {
		device_printf(self, "can't establish interrupt\n");
		return;
	}

#ifdef __NetBSD__
	if (!pmf_device_register(self, wwanc_pmf_suspend, wwanc_pmf_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
#endif

	/*
	 * Device initialization requires working interrupts, so need
	 * to postpone this until they are enabled.
	 */
	config_mountroot(self, wwanc_attach_finish);
	return;

fail:
	bus_space_unmap(sc->sc_bar0_tag, sc->sc_bar0_handle, sc->sc_bar0_sz);
	sc->sc_bar0_tag = 0;
	bus_space_unmap(sc->sc_bar2_tag, sc->sc_bar2_handle, sc->sc_bar2_sz);
	sc->sc_bar2_tag = 0;
	return;
}

static int
wwanc_detach(struct device *self, int flags)
{
	int error;
	struct wwanc_softc *sc = device_private(self);

	if (sc->sc_ih) {
		pci_intr_disestablish(sc->sc_pc, sc->sc_ih);
		sc->sc_ih = NULL;
	}

	if (sc->sc_net) {
		error = config_detach_children(self, flags);
		if (error)
			return error;
		sc->sc_net = NULL;
	}

	pmf_device_deregister(self);

	xmm7360_dev_deinit(&sc->sc_xmm);

	if (sc->sc_bar0_tag) {
		bus_space_unmap(sc->sc_bar0_tag, sc->sc_bar0_handle,
		    sc->sc_bar0_sz);
		sc->sc_bar0_tag = 0;
	}
	sc->sc_xmm.bar0 = NULL;

	if (sc->sc_bar2_tag) {
		bus_space_unmap(sc->sc_bar2_tag, sc->sc_bar2_handle,
		    sc->sc_bar2_sz);
		sc->sc_bar2_tag = 0;
	}
	sc->sc_xmm.bar2 = NULL;

	return 0;
}

static void
wwanc_suspend(struct device *self)
{
	struct wwanc_softc *sc = device_private(self);
	struct xmm_dev *xmm = &sc->sc_xmm;
	struct queue_pair *qp;

	KASSERT(!sc->sc_resume);
	KASSERT(xmm->cp != NULL);

	for (int i = 0; i < XMM_QP_COUNT; i++) {
		qp = &xmm->qp[i];
		if (qp->open)
			xmm7360_qp_suspend(qp);
	}

	xmm7360_cmd_ring_free(xmm);
	KASSERT(xmm->cp == NULL);
}

static void
wwanc_resume(struct device *self)
{
	struct wwanc_softc *sc = device_private(self);
	struct xmm_dev *xmm = &sc->sc_xmm;
	struct queue_pair *qp;

	KASSERT(xmm->cp == NULL);

	xmm7360_base_init(xmm);

	for (int i = 0; i < XMM_QP_COUNT; i++) {
		qp = &xmm->qp[i];
		if (qp->open)
			xmm7360_qp_resume(qp);
	}
}

#ifdef __OpenBSD__

static void
wwanc_defer_resume(void *xarg)
{
	struct device *self = xarg;
	struct wwanc_softc *sc = device_private(self);

	tsleep(&sc->sc_resume, 0, "wwancdr", 2 * hz);

	wwanc_resume(self);

	(void)config_activate_children(self, DVACT_RESUME);

	sc->sc_resume = false;
	kthread_exit(0);
}

static int
wwanc_activate(struct device *self, int act)
{
	struct wwanc_softc *sc = device_private(self);

	switch (act) {
	case DVACT_QUIESCE:
		(void)config_activate_children(self, act);
		break;
	case DVACT_SUSPEND:
		if (sc->sc_resume) {
			/* Refuse to suspend if resume still ongoing */
			device_printf(self,
			    "not suspending, resume still ongoing\n");
			return EBUSY;
		}

		(void)config_activate_children(self, act);
		wwanc_suspend(self);
		break;
	case DVACT_RESUME:
		/*
		 * Modem reinitialization can take several seconds, defer
		 * it via kernel thread to avoid blocking the resume.
		 */
		sc->sc_resume = true;
		kthread_create(wwanc_defer_resume, self, NULL, "wwancres");
		break;
	default:
		break;
	}

	return 0;
}

cdev_decl(wwanc);
#endif /* __OpenBSD__ */

#ifdef __NetBSD__
static bool
wwanc_pmf_suspend(device_t self, const pmf_qual_t *qual)
{
	wwanc_suspend(self);
	return true;
}

static bool
wwanc_pmf_resume(device_t self, const pmf_qual_t *qual)
{
	wwanc_resume(self);
	return true;
}

static dev_type_open(wwancopen);
static dev_type_close(wwancclose);
static dev_type_read(wwancread);
static dev_type_write(wwancwrite);
static dev_type_ioctl(wwancioctl);
static dev_type_poll(wwancpoll);
static dev_type_kqfilter(wwanckqfilter);
static dev_type_tty(wwanctty);

const struct cdevsw wwanc_cdevsw = {
	.d_open = wwancopen,
	.d_close = wwancclose,
	.d_read = wwancread,
	.d_write = wwancwrite,
	.d_ioctl = wwancioctl,
	.d_stop = nullstop,
	.d_tty = wwanctty,
	.d_poll = wwancpoll,
	.d_mmap = nommap,
	.d_kqfilter = wwanckqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};
#endif

static int wwancparam(struct tty *, struct termios *);
static void wwancstart(struct tty *);

static void xmm7360_os_handle_tty_idata(struct queue_pair *qp, const u8 *data, size_t nread)
{
	struct xmm_dev *xmm = qp->xmm;
	struct wwanc_softc *sc = device_private(xmm->dev);
	int func = qp->num;
	struct tty *tp = sc->sc_tty[func];

	KASSERT(DEV_IS_TTY(func));
	KASSERT(tp);

	for (int i = 0; i < nread; i++)
		LINESW(tp).l_rint(data[i], tp);
}

int
wwancopen(dev_t dev, int flags, int mode, struct proc *p)
{
	int unit = DEVUNIT(dev);
	struct wwanc_softc *sc = device_lookup_private(&wwanc_cd, unit);
	struct tty *tp;
	int func, error;

	if (sc == NULL)
		return ENXIO;

	/* Only allow opening the rpc/trace/AT queue pairs */
	func = DEVFUNC(dev);
	if (func < 1 || func > 7)
		return ENXIO;

	if (DEV_IS_TTY(dev)) {
		if (!sc->sc_tty[func]) {
			tp = sc->sc_tty[func] = ttymalloc(1000000);

			tp->t_oproc = wwancstart;
		        tp->t_param = wwancparam;
			tp->t_dev = dev;
			tp->t_sc = (void *)sc;
		} else
			tp = sc->sc_tty[func];

		if (!ISSET(tp->t_state, TS_ISOPEN)) {
			ttychars(tp);
			tp->t_iflag = TTYDEF_IFLAG;
			tp->t_oflag = TTYDEF_OFLAG;
			tp->t_lflag = TTYDEF_LFLAG;
			tp->t_cflag = TTYDEF_CFLAG;
			tp->t_ispeed = tp->t_ospeed = B115200;
			SET(tp->t_cflag, CS8 | CREAD | HUPCL | CLOCAL);

			SET(tp->t_state, TS_CARR_ON);
		} else if (suser(p) != 0) {
			return EBUSY;
		}

		error = LINESW(tp).l_open(dev, tp, p);
		if (error)
			return error;
	}

	/* Initialize ring if qp not open yet */
	xmm7360_qp_start(&sc->sc_xmm.qp[func]);

	return 0;
}

int
wwancread(dev_t dev, struct uio *uio, int flag)
{
	struct wwanc_softc *sc = device_lookup_private(&wwanc_cd, DEVUNIT(dev));
	int func = DEVFUNC(dev);

	KASSERT(sc != NULL);

	if (DEV_IS_TTY(dev)) {
		struct tty *tp = sc->sc_tty[func];

		return (LINESW(tp).l_read(tp, uio, flag));
	} else {
		struct queue_pair *qp = &sc->sc_xmm.qp[func];
		ssize_t ret;
		char *buf;
		size_t size, read = 0;

#ifdef __OpenBSD__
		KASSERT(uio->uio_segflg == UIO_USERSPACE);
#endif

		for (int i = 0; i < uio->uio_iovcnt; i++) {
			buf = uio->uio_iov[i].iov_base;
			size = uio->uio_iov[i].iov_len;

			while (size > 0) {
				ret = xmm7360_qp_read_user(qp, buf, size);
				if (ret < 0) {
					/*
					 * This shadows -EPERM, but that is
					 * not returned by the call stack,
					 * so this condition is safe.
					 */
					return (ret == ERESTART) ? ret : -ret;
				}

				KASSERT(ret > 0 && ret <= size);
				size -= ret;
				buf += ret;
				read += ret;

				/* Reader will re-try if they want more */
				goto out;
			}
		}

out:
		uio->uio_resid -= read;
		uio->uio_offset += read;

		return 0;
	}
}

int
wwancwrite(dev_t dev, struct uio *uio, int flag)
{
	struct wwanc_softc *sc = device_lookup_private(&wwanc_cd, DEVUNIT(dev));
	int func = DEVFUNC(dev);

	if (DEV_IS_TTY(dev)) {
		struct tty *tp = sc->sc_tty[func];

		return (LINESW(tp).l_write(tp, uio, flag));
	} else {
		struct queue_pair *qp = &sc->sc_xmm.qp[func];
		ssize_t ret;
		const char *buf;
		size_t size, wrote = 0;

#ifdef __OpenBSD__
		KASSERT(uio->uio_segflg == UIO_USERSPACE);
#endif

		for (int i = 0; i < uio->uio_iovcnt; i++) {
			buf = uio->uio_iov[i].iov_base;
			size = uio->uio_iov[i].iov_len;

			while (size > 0) {
				ret = xmm7360_qp_write_user(qp, buf, size);
				if (ret < 0) {
					/*
					 * This shadows -EPERM, but that is
					 * not returned by the call stack,
					 * so this condition is safe.
					 */
					return (ret == ERESTART) ? ret : -ret;
				}

				KASSERT(ret > 0 && ret <= size);
				size -= ret;
				buf += ret;
				wrote += ret;
			}
		}

		uio->uio_resid -= wrote;
		uio->uio_offset += wrote;

		return 0;
	}
}

int
wwancioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct wwanc_softc *sc = device_lookup_private(&wwanc_cd, DEVUNIT(dev));
	int error;

	if (DEV_IS_TTY(dev)) {
		struct tty *tp = sc->sc_tty[DEVFUNC(dev)];
		KASSERT(tp);

		error = LINESW(tp).l_ioctl(tp, cmd, data, flag, p);
		if (error >= 0)
			return error;
		error = ttioctl(tp, cmd, data, flag, p);
		if (error >= 0)
			return error;
	}

	return ENOTTY;
}

int
wwancclose(dev_t dev, int flag, int mode, struct proc *p)
{
	struct wwanc_softc *sc = device_lookup_private(&wwanc_cd, DEVUNIT(dev));
	int func = DEVFUNC(dev);

	if (DEV_IS_TTY(dev)) {
		struct tty *tp = sc->sc_tty[func];
		KASSERT(tp);

		CLR(tp->t_state, TS_BUSY | TS_FLUSH);
		LINESW(tp).l_close(tp, flag, p);
		ttyclose(tp);
	}

	xmm7360_qp_stop(&sc->sc_xmm.qp[func]);

	return 0;
}

struct tty *
wwanctty(dev_t dev)
{
	struct wwanc_softc *sc = device_lookup_private(&wwanc_cd, DEVUNIT(dev));
	struct tty *tp = sc->sc_tty[DEVFUNC(dev)];

	KASSERT(DEV_IS_TTY(dev));
	KASSERT(tp);

	return tp;
}

static int
wwancparam(struct tty *tp, struct termios *t)
{
	struct wwanc_softc *sc __diagused = (struct wwanc_softc *)tp->t_sc;
	dev_t dev = tp->t_dev;
	int func __diagused = DEVFUNC(dev);

	KASSERT(DEV_IS_TTY(dev));
	KASSERT(tp == sc->sc_tty[func]);
	/* Can't assert tty_locked(), it's not taken when called via ttioctl()*/

	/* Nothing to set on hardware side, just copy values */
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	return 0;
}

static void
wwancstart(struct tty *tp)
{
	struct wwanc_softc *sc = (struct wwanc_softc *)tp->t_sc;
	dev_t dev = tp->t_dev;
	int func = DEVFUNC(dev);
	struct queue_pair *qp = &sc->sc_xmm.qp[func];
	int n, written;

	KASSERT(DEV_IS_TTY(dev));
	KASSERT(tp == sc->sc_tty[func]);
	tty_locked();

	if (ISSET(tp->t_state, TS_BUSY) || !xmm7360_qp_can_write(qp))
		return;
	if (tp->t_outq.c_cc == 0)
		return;

	/*
	 * If we can write, we can write full qb page_size amount of data.
	 * Once q_to_b() is called, the data must be trasmitted - q_to_b()
	 * removes them from the tty output queue. Partial write is not
	 * possible.
	 */
	KASSERT(sizeof(qp->user_buf) >= qp->page_size);
	SET(tp->t_state, TS_BUSY);
	n = q_to_b(&tp->t_outq, qp->user_buf, qp->page_size);
	KASSERT(n > 0);
	KASSERT(n <= qp->page_size);
	written = xmm7360_qp_write(qp, qp->user_buf, n);
	CLR(tp->t_state, TS_BUSY);

	if (written != n) {
		dev_err(sc->sc_dev, "xmm7360_qp_write(%d) failed %d != %d\n",
		    func, written, n);
		/* nothing to recover, just return */
	}
}

int
wwancpoll(dev_t dev, int events, struct proc *p)
{
	struct wwanc_softc *sc = device_lookup_private(&wwanc_cd, DEVUNIT(dev));
	int func = DEVFUNC(dev);
	struct queue_pair *qp = &sc->sc_xmm.qp[func];
	int mask = 0;

	if (DEV_IS_TTY(dev)) {
#ifdef __OpenBSD__
		return ttpoll(dev, events, p);
#endif
#ifdef __NetBSD__
		struct tty *tp = sc->sc_tty[func];

		return LINESW(tp).l_poll(tp, events, p);
#endif
	}

	KASSERT(!DEV_IS_TTY(dev));

	if (qp->xmm->error) {
		mask |= POLLHUP;
		goto out;
	}

	if (xmm7360_qp_has_data(qp))
		mask |= POLLIN | POLLRDNORM;

	if (xmm7360_qp_can_write(qp))
		mask |= POLLOUT | POLLWRNORM;

out:
	if ((mask & events) == 0) {
		if (events & (POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND))
			selrecord(p, &sc->sc_selr);
                if (events & (POLLOUT | POLLWRNORM))
                        selrecord(p, &sc->sc_selw);
	}

	return mask & events;
}

static void
filt_wwancrdetach(struct knote *kn)
{
	struct queue_pair *qp = (struct queue_pair *)kn->kn_hook;

	tty_lock();
	klist_remove(&qp->selr.si_note, kn);
	tty_unlock();
}

static int
filt_wwancread(struct knote *kn, long hint)
{
	struct queue_pair *qp = (struct queue_pair *)kn->kn_hook;

	kn->kn_data = 0;

	if (!qp->open) {
		knote_set_eof(kn, 0);
		return (1);
	} else {
		kn->kn_data = xmm7360_qp_has_data(qp) ? 1 : 0;
	}

	return (kn->kn_data > 0);
}

static void
filt_wwancwdetach(struct knote *kn)
{
	struct queue_pair *qp = (struct queue_pair *)kn->kn_hook;

	tty_lock();
	klist_remove(&qp->selw.si_note, kn);
	tty_unlock();
}

static int
filt_wwancwrite(struct knote *kn, long hint)
{
	struct queue_pair *qp = (struct queue_pair *)kn->kn_hook;

	kn->kn_data = 0;

	if (qp->open) {
		if (xmm7360_qp_can_write(qp))
			kn->kn_data = qp->page_size;
	}

	return (kn->kn_data > 0);
}

static const struct filterops wwancread_filtops = {
	XMM_KQ_ISFD_INITIALIZER,
	.f_attach	= NULL,
	.f_detach	= filt_wwancrdetach,
	.f_event	= filt_wwancread,
};

static const struct filterops wwancwrite_filtops = {
	XMM_KQ_ISFD_INITIALIZER,
	.f_attach	= NULL,
	.f_detach	= filt_wwancwdetach,
	.f_event	= filt_wwancwrite,
};

int
wwanckqfilter(dev_t dev, struct knote *kn)
{
	struct wwanc_softc *sc = device_lookup_private(&wwanc_cd, DEVUNIT(dev));
	int func = DEVFUNC(dev);
	struct queue_pair *qp = &sc->sc_xmm.qp[func];
	struct klist *klist;

	if (DEV_IS_TTY(func))
		return ttkqfilter(dev, kn);

	KASSERT(!DEV_IS_TTY(func));

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &qp->selr.si_note;
		kn->kn_fop = &wwancread_filtops;
		break;
	case EVFILT_WRITE:
		klist = &qp->selw.si_note;
		kn->kn_fop = &wwancwrite_filtops;
		break;
	default:
		return (EINVAL);
	}

	kn->kn_hook = (void *)qp;

	tty_lock();
	klist_insert(klist, kn);
	tty_unlock();

	return (0);
}

static void *
dma_alloc_coherent(struct device *self, size_t sz, dma_addr_t *physp, int flags)
{
	struct wwanc_softc *sc = device_private(self);
	bus_dma_segment_t seg;
	int nsegs;
	int error;
	caddr_t kva;

	error = bus_dmamem_alloc(sc->sc_dmat, sz, 0, 0, &seg, 1, &nsegs,
	    BUS_DMA_WAITOK);
	if (error) {
		panic("%s: bus_dmamem_alloc(%lu) failed %d\n",
		    device_xname(self), (unsigned long)sz, error);
		/* NOTREACHED */
	}

	KASSERT(nsegs == 1);
	KASSERT(seg.ds_len == round_page(sz));

	error = bus_dmamem_map(sc->sc_dmat, &seg, nsegs, sz, &kva,
	    BUS_DMA_WAITOK | BUS_DMA_COHERENT);
	if (error) {
		panic("%s: bus_dmamem_alloc(%lu) failed %d\n",
		    device_xname(self), (unsigned long)sz, error);
		/* NOTREACHED */
	}

	memset(kva, 0, sz);
	*physp = seg.ds_addr;
	return (void *)kva;
}

static void
dma_free_coherent(struct device *self, size_t sz, volatile void *vaddr, dma_addr_t phys)
{
	struct wwanc_softc *sc = device_private(self);
	bus_dma_segment_t seg;

	sz = round_page(sz);

	bus_dmamem_unmap(sc->sc_dmat, __UNVOLATILE(vaddr), sz);

	/* this does't need the exact seg returned by bus_dmamem_alloc() */
	memset(&seg, 0, sizeof(seg));
	seg.ds_addr = phys;
	seg.ds_len  = sz;
	bus_dmamem_free(sc->sc_dmat, &seg, 1);
}

struct wwan_softc {
#ifdef __OpenBSD__
	struct device		sc_devx;	/* gen. device info storage */
#endif
	struct device		*sc_dev;	/* generic device */
	struct wwanc_softc	*sc_parent;	/* parent device */
	struct ifnet		sc_ifnet;	/* network-visible interface */
	struct xmm_net		sc_xmm_net;
};

static void xmm7360_os_handle_net_frame(struct xmm_dev *xmm, const u8 *buf, size_t sz)
{
	struct wwanc_softc *sc = device_private(xmm->dev);
	struct wwan_softc *sc_if = device_private(sc->sc_net);
	struct ifnet *ifp = &sc_if->sc_ifnet;
	struct mbuf *m;

	KASSERT(sz <= MCLBYTES);

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (!m)
		return;
	if (sz > MHLEN) {
		MCLGETI(m, M_DONTWAIT, NULL, sz);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			return;
		}
	}
	m->m_len = m->m_pkthdr.len = sz;

	/*
	 * No explicit alignment necessary - there is no ethernet header,
	 * so IP address is already aligned.
	 */
	KASSERT(m->m_pkthdr.len == sz);
	m_copyback(m, 0, sz, (const void *)buf, M_NOWAIT);

#ifdef __OpenBSD__
	struct mbuf_list ml = MBUF_LIST_INITIALIZER();
	ml_enqueue(&ml, m);
	if_input(ifp, &ml);
#endif
#ifdef __NetBSD__
	if_percpuq_enqueue(ifp->if_percpuq, m);
#endif
}

static void
xmm7360_os_handle_net_dequeue(struct xmm_net *xn, struct mux_frame *frame)
{
	struct wwan_softc *sc_if =
		container_of(xn, struct wwan_softc, sc_xmm_net);
	struct ifnet *ifp = &sc_if->sc_ifnet;
	struct mbuf *m;
	int ret;

	MUTEX_ASSERT_LOCKED(&xn->lock);

	while ((m = ifq_deq_begin(&ifp->if_snd))) {
		/*
		 * xmm7360_mux_frame_append_packet() requires single linear
		 * buffer, so try m_defrag(). Another option would be
		 * using m_copydata() into an intermediate buffer.
		 */
		if (m->m_next) {
			if (m_defrag(m, M_DONTWAIT) != 0 || m->m_next) {
				/* Can't defrag, drop and continue */
				ifq_deq_commit(&ifp->if_snd, m);
				m_freem(m);
				continue;
			}
		}

		ret = xmm7360_mux_frame_append_packet(frame,
		    mtod(m, void *), m->m_pkthdr.len);
		if (ret) {
			/* No more space in the frame */
			ifq_deq_rollback(&ifp->if_snd, m);
			break;
		}
		ifq_deq_commit(&ifp->if_snd, m);

		/* Send a copy of the frame to the BPF listener */
		BPF_MTAP_OUT(ifp, m);

		m_freem(m);
	}
}

static void xmm7360_os_handle_net_txwake(struct xmm_net *xn)
{
	struct wwan_softc *sc_if =
		container_of(xn, struct wwan_softc, sc_xmm_net);
	struct ifnet *ifp = &sc_if->sc_ifnet;

	MUTEX_ASSERT_LOCKED(&xn->lock);

	KASSERT(xmm7360_qp_can_write(xn->qp));
	if (ifq_is_oactive(&ifp->if_snd)) {
		ifq_clr_oactive(&ifp->if_snd);
#ifdef __OpenBSD__
		ifq_restart(&ifp->if_snd);
#endif
#ifdef __NetBSD__
		if_schedule_deferred_start(ifp);
#endif
	}
}

#ifdef __OpenBSD__
/*
 * Process received raw IPv4/IPv6 packet. There is no encapsulation.
 */
static int
wwan_if_input(struct ifnet *ifp, struct mbuf *m, void *cookie)
{
	const uint8_t *data = mtod(m, uint8_t *);
	void (*input)(struct ifnet *, struct mbuf *);
	u8 ip_version;

	ip_version = data[0] >> 4;

	switch (ip_version) {
	case IPVERSION:
		input = ipv4_input;
		break;
	case (IPV6_VERSION >> 4):
		input = ipv6_input;
		break;
	default:
		/* Unknown protocol, just drop packet */
		m_freem(m);
		return 1;
		/* NOTREACHED */
	}

	/* Needed for tcpdump(1) et.al */
	m->m_pkthdr.ph_rtableid = ifp->if_rdomain;
	m_adj(m, sizeof(u_int32_t));

	(*input)(ifp, m);
	return 1;
}
#endif /* __OpenBSD__ */

#ifdef __NetBSD__
static bool wwan_pmf_suspend(device_t, const pmf_qual_t *);

/*
 * Process received raw IPv4/IPv6 packet. There is no encapsulation.
 */
static void
wwan_if_input(struct ifnet *ifp, struct mbuf *m)
{
	const uint8_t *data = mtod(m, uint8_t *);
	pktqueue_t *pktq = NULL;
	u8 ip_version;

	KASSERT(!cpu_intr_p());
	KASSERT((m->m_flags & M_PKTHDR) != 0);

	if ((ifp->if_flags & IFF_UP) == 0) {
		m_freem(m);
		return;
	}

	if_statadd(ifp, if_ibytes, m->m_pkthdr.len);

	/*
	 * The interface can't receive packets for other host, so never
	 * really IFF_PROMISC even if bpf listener is attached.
	 */
	if (pfil_run_hooks(ifp->if_pfil, &m, ifp, PFIL_IN) != 0)
		return;
	if (m == NULL)
		return;

	ip_version = data[0] >> 4;
	switch (ip_version) {
#ifdef INET
	case IPVERSION:
#ifdef GATEWAY
		if (ipflow_fastforward(m))
			return;
#endif
		pktq = ip_pktq;
		break;
#endif /* INET */
#ifdef INET6
	case (IPV6_VERSION >> 4):
		if (__predict_false(!in6_present)) {
			m_freem(m);
			return;
		}
#ifdef GATEWAY
		if (ip6flow_fastforward(&m))
			return;
#endif
		pktq = ip6_pktq;
		break;
#endif /* INET6 */
	default:
		/* Unknown protocol, just drop packet */
		m_freem(m);
		return;
		/* NOTREACHED */
	}

	KASSERT(pktq != NULL);

	/* No errors.  Receive the packet. */
	m_set_rcvif(m, ifp);

	const uint32_t h = pktq_rps_hash(&xmm7360_pktq_rps_hash_p, m);
	if (__predict_false(!pktq_enqueue(pktq, m, h))) {
		m_freem(m);
	}
}
#endif

/*
 * Transmit raw IPv4/IPv6 packet. No encapsulation necessary.
 */
static int
wwan_if_output(struct ifnet *ifp, struct mbuf *m,
    IF_OUTPUT_CONST struct sockaddr *dst, IF_OUTPUT_CONST struct rtentry *rt)
{
	// there is no ethernet frame, this means no bridge(4) handling
	return (if_enqueue(ifp, m));
}

static int
wwan_if_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct wwan_softc *sc_if = ifp->if_softc;
	int error = 0;
	int s;

	s = splnet();

	switch (cmd) {
#ifdef __NetBSD__
	case SIOCINITIFADDR:
#endif
#ifdef __OpenBSD__
	case SIOCAIFADDR:
	case SIOCAIFADDR_IN6:
	case SIOCSIFADDR:
#endif
		/* Make interface ready to run if address is assigned */
		ifp->if_flags |= IFF_UP;
		if (!(ifp->if_flags & IFF_RUNNING)) {
			ifp->if_flags |= IFF_RUNNING;
			xmm7360_mux_control(&sc_if->sc_xmm_net, 1, 0, 0, 0);
		}
		break;
	case SIOCSIFFLAGS:
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		/* nothing special to do */
		break;
	case SIOCSIFMTU:
		error = ENOTTY;
		break;
	default:
#ifdef __NetBSD__
		/*
		 * Call common code for SIOCG* ioctls. In OpenBSD those ioctls
		 * are handled in ifioctl(), and the if_ioctl is not called
		 * for them at all.
		 */
		error = ifioctl_common(ifp, cmd, data);
		if (error == ENETRESET)
			error = 0;
#endif
#ifdef __OpenBSD__
		error = ENOTTY;
#endif
		break;
	}

	splx(s);

	return error;
}

static void
wwan_if_start(struct ifnet *ifp)
{
	struct wwan_softc *sc = ifp->if_softc;

	mutex_lock(&sc->sc_xmm_net.lock);
	while (!ifq_empty(&ifp->if_snd)) {
		if (!xmm7360_qp_can_write(sc->sc_xmm_net.qp)) {
			break;
		}
		xmm7360_net_flush(&sc->sc_xmm_net);
	}
	mutex_unlock(&sc->sc_xmm_net.lock);
}

static int
wwan_match(struct device *parent, cfdata_t match, void *aux)
{
	struct wwanc_attach_args *wa = aux;

	return (wa->aa_type == WWMC_TYPE_NET);
}

static void
wwan_attach(struct device *parent, struct device *self, void *aux)
{
	struct wwan_softc *sc_if = device_private(self);
	struct ifnet *ifp = &sc_if->sc_ifnet;
	struct xmm_dev *xmm;
	struct xmm_net *xn;

	sc_if->sc_dev = self;
	sc_if->sc_parent = device_private(parent);
	xmm = sc_if->sc_xmm_net.xmm = &sc_if->sc_parent->sc_xmm;
	xn = &sc_if->sc_xmm_net;
	mutex_init(&xn->lock);

	/* QP already initialized in parent, just set pointers and start */
	xn->qp = &xmm->qp[0];
	xmm7360_qp_start(xn->qp);
	xmm->net = xn;

	ifp->if_softc = sc_if;
	ifp->if_flags = IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST \
		| IFF_SIMPLEX;
	ifp->if_ioctl = wwan_if_ioctl;
	ifp->if_start = wwan_if_start;
	ifp->if_mtu = 1500;
	ifp->if_hardmtu = 1500;
	ifp->if_type = IFT_OTHER;
	IFQ_SET_MAXLEN(&ifp->if_snd, xn->qp->depth);
	IFQ_SET_READY(&ifp->if_snd);
	CTASSERT(DEVICE_XNAME_SIZE == IFNAMSIZ);
	bcopy(device_xname(sc_if->sc_dev), ifp->if_xname, IFNAMSIZ);

	/* Call MI attach routines. */
	if_attach(ifp);

	/* Hook custom input and output processing, and dummy sadl */
	ifp->if_output = wwan_if_output;
	if_ih_insert(ifp, wwan_if_input, NULL);
	if_deferred_start_init(ifp, NULL);
	if_alloc_sadl(ifp);
#if NBPFILTER > 0
#ifdef __OpenBSD__
	bpfattach(&ifp->if_bpf, ifp, DLT_LOOP, sizeof(u_int32_t));
#endif
#ifdef __NetBSD__
	bpfattach(&ifp->if_bpf, ifp, DLT_RAW, 0);
#endif
#endif

	printf("\n");

#ifdef __NetBSD__
	xmm7360_pktq_rps_hash_p = pktq_rps_hash_default;

	if (pmf_device_register(self, wwan_pmf_suspend, NULL))
		pmf_class_network_register(self, ifp);
	else
		aprint_error_dev(self, "couldn't establish power handler\n");
#endif
}

static int
wwan_detach(struct device *self, int flags)
{
	struct wwan_softc *sc_if = device_private(self);
	struct ifnet *ifp = &sc_if->sc_ifnet;

	if (ifp->if_flags & (IFF_UP|IFF_RUNNING))
		ifp->if_flags &= ~(IFF_UP|IFF_RUNNING);

	pmf_device_deregister(self);

	if_ih_remove(ifp, wwan_if_input, NULL);
	if_detach(ifp);

	xmm7360_qp_stop(sc_if->sc_xmm_net.qp);

	sc_if->sc_xmm_net.xmm->net = NULL;

	return 0;
}

static void
wwan_suspend(struct device *self)
{
	struct wwan_softc *sc_if = device_private(self);
	struct ifnet *ifp = &sc_if->sc_ifnet;

	/*
	 * Interface is marked down on suspend, and needs to be reconfigured
	 * after resume.
	 */
	if (ifp->if_flags & (IFF_UP|IFF_RUNNING))
		ifp->if_flags &= ~(IFF_UP|IFF_RUNNING);

	ifq_purge(&ifp->if_snd);
}

#ifdef __OpenBSD__
static int
wwan_activate(struct device *self, int act)
{
	switch (act) {
	case DVACT_QUIESCE:
	case DVACT_SUSPEND:
		wwan_suspend(self);
		break;
	case DVACT_RESUME:
		/* Nothing to do */
		break;
	}

	return 0;
}

struct cfattach wwan_ca = {
        sizeof(struct wwan_softc), wwan_match, wwan_attach,
        wwan_detach, wwan_activate
};

struct cfdriver wwan_cd = {
        NULL, "wwan", DV_IFNET
};
#endif /* __OpenBSD__ */

#ifdef __NetBSD__
static bool
wwan_pmf_suspend(device_t self, const pmf_qual_t *qual)
{
	wwan_suspend(self);
	return true;
}

CFATTACH_DECL3_NEW(wwan, sizeof(struct wwan_softc),
   wwan_match, wwan_attach, wwan_detach, NULL,
   NULL, NULL, DVF_DETACH_SHUTDOWN);
#endif /* __NetBSD__ */

#endif /* __OpenBSD__ || __NetBSD__ */
