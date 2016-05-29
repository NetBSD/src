/*	$NetBSD: ipsec_osdep.h,v 1.24.10.1 2016/05/29 08:44:39 skrll Exp $	*/
/*	$FreeBSD: /repoman/r/ncvs/src/sys/netipsec/ipsec_osdep.h,v 1.1 2003/09/29 22:47:45 sam Exp $	*/

/*
 * Copyright (c) 2003 Jonathan Stone (jonathan@cs.stanford.edu)
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
 */

#ifndef _NETIPSEC_OSDEP_H_
#define _NETIPSEC_OSDEP_H_

#ifdef _KERNEL
/*
 *  Hide porting differences across different 4.4BSD-derived platforms.
 *
 * 1.  KASSERT() differences:
 * 2.  Kernel  Random-number API differences.
 * 3.  Is packet data in an mbuf object writeable?
 * 4.  Packet-header semantics.
 * 5.  Fast mbuf-cluster allocation.
 * 6.  Network packet-output macros.
 * 7.  Elased time, in seconds.
 * 8.  Test if a  socket object opened by  a privileged (super) user.
 * 9.  Global SLIST of all open raw sockets.
 * 10. Global SLIST of known interface addresses.
 * 11. Type of initialization functions.
 * 12. Byte order of ip_off
 */

/*
 *  1. KASSERT and spl differences
 *
 * FreeBSD takes an expression and  parenthesized printf() argument-list.
 * NetBSD takes one arg: the expression being asserted.
 * FreeBSD's SPLASSERT() takes an SPL level as 1st arg and a
 * parenthesized printf-format argument list as the second argument.
 *
 * This difference is hidden by two 2-argument macros and one 1-arg macro:
 *    IPSEC_ASSERT(expr, msg)
 *    IPSEC_SPLASSERT(spl, msg)
 * One further difference is the spl names:
 *    NetBSD splsoftnet equates to FreeBSD splnet;
 *    NetBSD splnet equates to FreeBSD splimp.
 * which is hidden by the macro IPSEC_SPLASSERT_SOFTNET(msg).
 */
#ifdef __FreeBSD__
#define IPSEC_SPLASSERT(x,y) SPLASSERT(x, y)
#define IPSEC_ASSERT(c,m) KASSERT(c, m)
#define IPSEC_SPLASSERT_SOFTNET(m) SPLASSERT(splnet, m)
#endif	/* __FreeBSD__ */

#ifdef	__NetBSD__
#define IPSEC_SPLASSERT(x,y) (void)0
#define IPSEC_ASSERT(c,m) KASSERT(c)
#define IPSEC_SPLASSERT_SOFTNET(m) IPSEC_SPLASSERT(softnet, m)
#endif	/* __NetBSD__ */

/*
 * 2. Kernel Randomness API.
 * FreeBSD uses:
 *    u_int read_random(void *outbuf, int nbytes).
 */
#ifdef __FreeBSD__
#include <sys/random.h>
/* do nothing, use native random code. */
#endif /* __FreeBSD__ */

#ifdef	__NetBSD__
#include <sys/cprng.h>
static __inline u_int read_random(void *p, u_int len);

static __inline u_int
read_random(void *bufp, u_int len)
{
	return cprng_fast(bufp, len);
}
#endif	/* __NetBSD__ */

/*
 * 3. Test for mbuf mutability
 * FreeBSD 4.x uses: M_EXT_WRITABLE
 * NetBSD has M_READONLY(). Use !M_READONLY().
 * Not an exact match to FreeBSD semantics, but adequate for IPsec purposes.
 *
 */
#ifdef __NetBSD__
/* XXX wrong, but close enough for restricted ipsec usage. */
#define M_EXT_WRITABLE(m) (!M_READONLY(m))
#endif	/* __NetBSD__ */

/*
 * 4. mbuf packet-header/packet-tag semantics.
 */
/*
 * nothing.
 */

/*
 * 5. Fast mbuf-cluster allocation.
 */
/*
 * nothing.
 */

/*
 * 6. Network output macros
 * FreeBSD uses the  IF_HANDOFF(), which raises SPL, enqueues
 * a packet, and updates interface counters. NetBSD has IFQ_ENQUE(),
 * which leaves SPL changes up to the caller.
 * For now, we provide an emulation of IF_HANOOFF() which works
 * for protocol input queues.
 */
#ifdef __FreeBSD__
/* nothing to do */
#endif /* __FreeBSD__ */
#ifdef __NetBSD__
#define IF_HANDOFF(ifq, m, f) if_handoff(ifq, m, f, 0)

#include <net/if.h>

static __inline int
if_handoff(struct ifqueue *ifq, struct mbuf *m, struct ifnet *ifp, int adjust)
{
	int s = splnet();

	KERNEL_LOCK(1, NULL);
	if (IF_QFULL(ifq)) {
		IF_DROP(ifq);
		KERNEL_UNLOCK_ONE(NULL);
		splx(s);
		m_freem(m);
		return (0);
	}
	if (ifp != NULL)
		(void)(*ifp->if_transmit)(ifp, m);

	KERNEL_UNLOCK_ONE(NULL);
	splx(s);
	return (1);
}
#endif /* __NetBSD__ */

/*
 * 7. Elapsed Time: time_second as time in seconds.
 * Original FreeBSD fast-ipsec code references a FreeBSD kernel global,
 * time_second().
 * XXX is this the right time scale - shouldn't we measure timeout/life times
 * using a monotonic time scale (time_uptime, mono_time) - why if the FreeBSD
 * base code using UTC based time for this ?
 */

/* protosw glue */
#ifdef __NetBSD__
#include <sys/protosw.h>
#define ipprotosw protosw
#endif	/* __NetBSD__ */

/*
 * 8. Test for "privileged" socket opened by superuser.
 * FreeBSD tests  ((so)->so_cred && (so)->so_cred.cr_uid == 0),
 * NetBSD (1.6N) tests (so)->so_uid == 0).
 * This difference is wrapped inside  the IPSEC_PRIVILEGED_SO() macro.
 *
 */
#ifdef __FreeBSD__
#define IPSEC_PRIVILEGED_SO(so) ((so)->so_cred && (so)->so_cred.cr_uid == 0)
#endif	/* __FreeBSD__ */

#ifdef __NetBSD__
/* superuser opened socket? */
#define IPSEC_PRIVILEGED_SO(so) ((so)->so_uidinfo->ui_uid == 0)
#endif	/* __NetBSD__ */

/*
 * 9. Raw socket list
 * FreeBSD uses: listhead = rawcb_list, SLIST()-next field "list".
 * NetBSD  uses: listhead = rawcb, SLIST()-next field "list"
 *
 * This version of fast-ipsec source code  uses rawcb_list as the head,
 *  and (to avoid namespace collisions) uses rcb_list as the "next" field.
 */
#ifdef __FreeBSD__
#define rcb_list list
#endif /* __FreeBSD__ */
#ifdef __NetBSD__
#define rawcb_list rawcb
#endif	/* __NetBSD__ */


/*
 * 10. List of all known network interfaces.
 * FreeBSD has listhead in_ifaddrhead, with ia_link as link.
 * NetBSD has listhead in_ifaddr, with ia_list as link.
 * No name-clahses, so just #define the appropriate names on NetBSD.
 * NB: Is it worth introducing iterator (find-first-list/find-next-list)
 * functions or macros to encapsulate these?
 */
#ifdef __FreeBSD__
/* nothing to do for raw interface list */
#endif	/* FreeBSD */
#ifdef __NetBSD__
#define ia_link ia_list
#endif	/* __NetBSD__ */

/*
 * 11.  Type of initialization functions.
 */
#ifdef __FreeBSD__
#define INITFN static
#endif
#ifdef __NetBSD__
#define INITFN extern
#endif

/* 12. On FreeBSD, ip_off  assumed in host endian;
 * it is converted (if necessary) by ip_input().
 * On NetBSD, ip_off is in network byte order.
 * We hide the difference with the macro IP_OFF_CONVERT
 */

#ifdef __FreeBSD__
#define IP_OFF_CONVERT(x) (x)
#endif

#ifdef __NetBSD__
#define IP_OFF_CONVERT(x) (htons(x))
#endif

/*
 * 13. IPv6 support, and "generic" inpcb vs. IPv4 pcb vs. IPv6 pcb.
 * To IPv6 V4-mapped addresses (and the KAME-derived implementation
 * of IPv6 v4-mapped addresses)  we must support limited polymorphism:
 * partway down the stack we detect an IPv6 protocol address is really
 * a mapped V4 address, and then start dispatching that address to
 * native IPv4 PCB lookup. In KAME-derived IPsec (including fas-ipsec)
 * some functions must handle arguments which (dynamically) may be either
 * a IPv4 pcb (struct inpcb *) or an IPv6 pcb (struct in6pcb *).
 *
 * In FreeBSD 4.x, sgtrucr in6pcb is syntactic sugar for struct inpcb,
 * so punning between struct inpcb* and struct in6pcb* is trivial.
 * NetBSD until recently used completely different structs for IPv4
 * and IPv6 PCBs. To simplify fast-ipsec coexisting with IPv6,
 * NetBSD's struct inpcb and struct in6pcb were changed to both have
 * common struct, struct inpcb_hdr, as their first member.  NetBSD can
 * thus pass arguments as struct inpcb_hdr*, and dispatch on a v4/v6
 * flag in the inpcb_hdr at runtime.
 *
 * We hide the NetBSD-vs-FreeBSD differences inside the following abstraction:
 *
 *  PCB_T:  a macro name for a struct type which is used as a "generic"
 *      argument for actual arguments  an in4pcb or an in6pcb.
 *
 * PCB_FAMILY(p): given a "generic" pcb_t p, returns the protocol
 *	family (AF_INET, AF_INET6) of the unperlying inpcb/in6pcb.
 *
 * PCB_SOCKET(p): given a "generic" pcb_t p, returns the associated
 *	socket pointer
 *
 * PCB_TO_IN4PCB(p): given generic pcb_t *p, returns a struct inpcb *
 * PCB_TO_IN6PCB(p): given generic pcb_t *p, returns a struct in6pcb *
 *
 * IN4PCB_TO_PCB(inp):  given a struct inpcb *inp,   returns a pcb_t *
 * IN6PCB_TO_PCB(in6p): given a struct in6pcb *in6p, returns a pcb_t *
 */
#ifdef __FreeBSD__
#define PCB_T		struct inpcb
#define PCB_FAMILY(p)	((p)->inp_socket->so_proto->pr_domain->dom_family)
#define	PCB_SOCKET(p)	((p)->inp_socket)

/* Convert generic pcb to IPv4/IPv6 pcb */
#define PCB_TO_IN4PCB(p) (p)
#define PCB_TO_IN6PCB(p) (p)

/* Convert IPv4/IPv6 pcb to generic pcb, for callers of fast-ipsec */
#define IN4PCB_TO_PCB(p) (p)
#define IN6PCB_TO_PCB(p) (p)
#endif	/* __FreeBSD__ */

#ifdef __NetBSD__
#define PCB_T		struct inpcb_hdr
#define PCB_FAMILY(p)	((p)->inph_af)
#define PCB_SOCKET(p)	((p)->inph_socket)

#define PCB_TO_IN4PCB(p) ((struct inpcb *)(p))
#define PCB_TO_IN6PCB(p) ((struct in6pcb *)(p))

#define IN4PCB_TO_PCB(p) ((PCB_T *)(&(p)->inp_head))
#define IN6PCB_TO_PCB(p) ((PCB_T *)(&(p)->in6p_head))
#endif	/* __NetBSD__ */

/*
 * Differences that we don't attempt to hide:
 *
 * A. Initialization code.  This  is the largest difference of all.
 *
 *   FreeBSD uses compile/link-time perl hackery to generate special
 * .o files  with linker sections  that give the moral equivalent of
 * C++ file-level-object constructors. NetBSD has no such facility.
 *
 * Either we implement it (ideally, in a way that can emulate
 * FreeBSD's SYSINIT() macros), or we must take other means
 * to have the per-file init functions called at some appropriate time.
 *
 * In the absence of SYSINIT(), all the file-level init functions
 * now have "extern" linkage. There is a new fast-ipsec init()
 * function which calls each of the per-file in an appropriate order.
 * init_main will arrange to call the fast-ipsec init function
 * after the crypto framework has registered its transforms (including
 * any autoconfigured hardware crypto  accelerators) but before
 * initializing the network stack to send or receive  packet.
 *
 * B. Protosw() differences.
 * CSRG-style BSD TCP/IP uses a generic protocol-dispatch-function
 * where the specific request is identified by an enum argument.
 * FreeBSD replaced that with an array of request-specific
 * function pointers.
 *
 * These differences affect the handlers for key-protocol user requests
 * so pervasively that I gave up on the fast-ipsec code, and re-worked the
 * NetBSD KAME code to match the (relative few) API differences
 * between NetBSD and FreeBSD's KAME netkey, and Fast-IPsec netkey.
 *
 * C. Timeout() versus callout(9):
 * The FreeBSD 4.x netipsec/ code still uses timeout().
 * FreeBSD 4.7 has callout(9), so I just replaced
 * timeout_*() with the nearest callout_*() equivalents,
 * and added a callout handle to the ipsec context.
 *
 * D. SPL name differences.
 * FreeBSD splnet() equates directly to NetBSD's splsoftnet();
 * FreeBSD uses splimp() where (for networking) NetBSD would use splnet().
 */
#endif /* _KERNEL */
#endif /* !_NETIPSEC_OSDEP_H_ */
