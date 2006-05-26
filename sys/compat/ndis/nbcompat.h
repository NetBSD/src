#ifndef _NBCOMPAT_H_
#define _NBCOMPAT_H_

#include <sys/systm.h>
#include <sys/lkm.h>
#include <sys/cdefs.h>
#include <sys/queue.h>
#ifdef _KERNEL
#include <sys/device.h>
#else
typedef struct device *device_t;
#endif

#define CTLFLAG_RW			CTLFLAG_READWRITE

#define mtx				lock
#define mtx_init(mtx, desc, type, opts)	lockinit(mtx, PWAIT, desc, 0, 0/*LK_CANRECURSE*/)
/*
#define mtx_lock(mtx)		ndis_mtx_ipl = splnet() lockmgr(mtx, LK_EXCLUSIVE? LK_SHARED, NULL)
#define mtx_unlock(mtx)         splx(ndis_mtx_ipl)	lockmgr(mtx, LK_RELEASE, NULL)
*/

void mtx_lock(struct mtx *mutex);
void mtx_unlock(struct mtx *mutex);

#define mtx_destroy(mtx)

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
#define I386_BUS_SPACE_MEM	1

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
#define I386_BUS_SPACE_IO	0

#define device_get_nameunit(dev)	(dev)->dv_xname

int tvtohz(struct timeval *tv);

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
typedef struct lkm_table *	module_t;

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
    struct proc **newpp, void *stack, size_t stacksize, const char *fmt, ...);

/*
 * NetBSD miss some atomic function so we add this function here. Note it
 * is x86 function ( taken from FreeBSD atomic.h)
 */

static 
__inline void atomic_add_long(volatile u_long *p, u_long v) {
     __asm __volatile("lock ; addl %1,%0" : "+m" (*p) : "ir" (v)); 
} 

static
 __inline void atomic_subtract_long(volatile u_long *p, u_long v){
     __asm __volatile("lock ; subl %1,%0" : "+m" (*p) : "ir" (v)); 
}

static
 __inline void atomic_store_rel_int( volatile u_int *p, u_int v){
     __asm __volatile("xchgl %1,%0" : "+m" (*p), "+r" (v) : : "memory");
}

static __inline int
atomic_cmpset_int(volatile u_int *dst, u_int expe, u_int src)
{
        int res = expe;

        __asm __volatile(
        "       pushfl ;                "
        "       cli ;                   "
        "       cmpl    %0,%2 ;         "
        "       jne     1f ;            "
        "       movl    %1,%2 ;         "
        "1:                             "
        "       sete    %%al;           "
        "       movzbl  %%al,%0 ;       "
        "       popfl ;                 "
        "# atomic_cmpset_int"
        : "+a" (res)                    /* 0 (result) */
        : "r" (src),                    /* 1 */
          "m" (*(dst))                  /* 2 */
        : "memory");

        return (res);
}

#define atomic_cmpset_acq_int           atomic_cmpset_int

#endif /* _NBCOMPAT_H_ */
