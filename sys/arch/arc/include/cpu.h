/*	$NetBSD: cpu.h,v 1.6 2000/01/23 20:08:19 soda Exp $	*/

#include <mips/cpu.h>
#include <mips/cpuregs.h>

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	COPY_SIGCODE		/* copy sigcode above user stack in exec */
