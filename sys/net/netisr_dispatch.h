/* $NetBSD: netisr_dispatch.h,v 1.1 2000/02/21 20:36:14 erh Exp $ */

/*
 * netisr_dispatch: This file is included by the 
 *	machine dependant softnet function.  The
 *	DONETISR macro should be set before including
 *	this file.  i.e.:
 *
 * softintr() {
 *	...do setup stuff...
 *	#define DONETISR(bit, fn) do { ... } while (0)
 *	#include <net/netisr_dispatch.h>
 *	#undef DONETISR
 *	...do cleanup stuff.
 * }
 */
#include "opt_inet.h"
#include "opt_atalk.h"
#include "opt_ccitt.h"
#include "opt_iso.h"
#include "opt_ns.h"
#include "opt_natm.h"
#include "ppp.h"
#ifdef INET
#include "arp.h"
#if NARP > 0
	DONETISR(NETISR_ARP,arpintr);
#endif
	DONETISR(NETISR_IP,ipintr);
#endif
#ifdef INET6
	DONETISR(NETISR_IPV6,ip6intr);
#endif
#ifdef NETATALK
	DONETISR(NETISR_ATALK,atintr);
#endif
#ifdef NS
	DONETISR(NETISR_NS,nsintr);
#endif
#ifdef ISO
	DONETISR(NETISR_ISO,clnlintr);
#endif
#ifdef CCITT
	DONETISR(NETISR_CCITT,ccittintr);
#endif
#ifdef NATM
	DONETISR(NETISR_NATM,natmintr);
#endif
#if NPPP > 0
	DONETISR(NETISR_PPP,pppintr);
#endif
