/*	$NetBSD: cpu.h,v 1.1.12.1 2002/08/30 00:19:32 gehenna Exp $	*/

#if defined(_KERNEL) && !defined(_LKM)

#include "opt_evbarm_boardtype.h"

#define EVBARM_BOARDTYPE_INTEGRATOR	1
#define EVBARM_BOARDTYPE_IQ80310	2
#define EVBARM_BOARDTYPE_I80321		3
#define EVBARM_BOARDTYPE_IXM1200	4

#endif

#include <arm/cpu.h>
