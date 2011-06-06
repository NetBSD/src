#ifndef _NBCOMPAT_H_
#define _NBCOMPAT_H_

#include <sys/systm.h>
#include <sys/module.h>
#include <sys/cdefs.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/device.h>

#define CTLFLAG_RW			CTLFLAG_READWRITE

#define mtx				kmutex
#define mtx_init(mtx, desc, type, opts)	mutex_init(mtx, MUTEX_DEFAULT, IPL_NONE)

#define mtx_lock(mtx)			mutex_enter(mtx)
#define mtx_unlock(mtx)			mutex_exit(mtx)

#define	mtx_lock_spin(mtx)		mutex_spin_enter(mtx);
#define	mtx_unlock_spin(mtx)		mutex_spin_exit(mtx);

void mtx_lock(struct mtx *mutex);
void mtx_unlock(struct mtx *mutex);

#define mtx_destroy(mtx)		mutex_destroy(mtx)

/* I don't think this is going to work
struct sysctl_ctx_entry {
	struct ctlname	*entry;
	TAILQ_ENTRY(sysctl_ctx_entry) link;
};

TAILQ_HEAD(sysctl_ctx_list, sysctl_ctx_entry);
*/

#define ETHER_ALIGN 2
#define BUS_SPACE_MAXADDR_32BIT 0xFFFFFFFF
#ifdef PAE
#define BUS_SPACE_MAXADDR	0xFFFFFFFFFULL
#else
#define BUS_SPACE_MAXADDR	0xFFFFFFFF
#endif
#define BUS_SPACE_MAXSIZE_32BIT 0xFFFFFFFF
#define I386_BUS_SPACE_MEM	x86_bus_space_mem

#define device_get_softc	(struct ndis_softc *)
#define ticks			hardclock_ticks
#define p_siglist		p_sigctx.ps_siglist

#ifndef __DECONST
#define __DECONST(type, var)	((type)(uintptr_t)(const void *)(var))
#endif

/* 4096 on x86 */
#ifndef PAGE_SIZE
#define PAGE_SIZE		4096
#endif
#define I386_BUS_SPACE_IO	x86_bus_space_io

#define device_get_nameunit(dev)	device_xname(dev)

/* FreeBSD Loadable Kernel Module commands that have NetBSD counterparts */
#define MOD_LOAD 	LKM_E_LOAD
#define MOD_UNLOAD	LKM_E_UNLOAD

/* ethercom/arpcom */
#define ac_if ec_if

#ifdef __NetBSD__
#define MAX_SYSCTL_LEN 256
#endif

/* Capabilities that interfaces can advertise. */
/* TODO: is this the correct mapping? */
#define IFCAP_TXCSUM (IFCAP_CSUM_IPv4_Tx|IFCAP_CSUM_TCPv4_Tx|IFCAP_CSUM_UDPv4_Tx|IFCAP_CSUM_TCPv6_Tx|IFCAP_CSUM_UDPv6_Tx)
#define IFCAP_RXCSUM (IFCAP_CSUM_IPv4_Rx|IFCAP_CSUM_TCPv4_Rx|IFCAP_CSUM_UDPv4_Rx|IFCAP_CSUM_TCPv6_Rx|IFCAP_CSUM_UDPv6_Rx)
#define CSUM_IP  M_CSUM_IPv4 /*(IFCAP_CSUM_IPv4_Rx |IFCAP_CSUM_IPv4_Tx)*/
#define CSUM_TCP M_CSUM_TCPv4 /*(IFCAP_CSUM_TCPv4_Rx|IFCAP_CSUM_TCPv4_Tx)*/
#define CSUM_UDP M_CSUM_UDPv4 /*(IFCAP_CSUM_UDPv4_Rx|IFCAP_CSUM_UDPv4_Tx)*/

typedef vaddr_t			vm_offset_t;
typedef vsize_t			vm_size_t;
typedef uint16_t		linker_file_t;

/* Write our own versions of some FreeBSD functions */
struct ndis_resource;
#define SYS_RES_IOPORT 0
#define SYS_RES_MEMORY 1
int     bus_release_resource(device_t dev, int type, int rid,
                             struct ndis_resource *r);
int	device_is_attached(device_t dev);

/* This is the same thing as NetBSD's kthread_create1(), except
 * the stack can be specified.
 */
int
ndis_kthread_create(void (*func)(void *), void *arg,
    struct proc **newpp, void *stack, size_t stacksize, const char *name);

#endif /* _NBCOMPAT_H_ */
