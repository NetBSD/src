/*	$NetBSD: cpu.h,v 1.10.60.1 2006/05/24 15:47:53 tron Exp $	*/

#ifndef _COBALT_CPU_H_
#define _COBALT_CPU_H_

#include <mips/cpu.h>

#ifdef _KERNEL
#ifndef _LOCORE
extern u_int cobalt_id;

#define COBALT_ID_QUBE2700	3
#define COBALT_ID_RAQ		4
#define COBALT_ID_QUBE2		5
#define COBALT_ID_RAQ2		6 

#endif /* !_LOCORE */
#endif /* _KERNEL */

#endif /* !_COBALT_CPU_H_ */
