/*	$NetBSD: cpu.h,v 1.5 1997/06/16 06:17:29 jonathan Exp $	*/

#include <mips/cpu.h>
#include <mips/cpuregs.h>

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	COPY_SIGCODE		/* copy sigcode above user stack in exec */
