/*	$NetBSD: openpic.c,v 1.5 2003/09/27 04:44:42 matt Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: openpic.c,v 1.5 2003/09/27 04:44:42 matt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <powerpc/openpic.h>

volatile unsigned char *openpic_base;

void
openpic_enable_irq(int irq, int type)
{
	u_int x;

	x = openpic_read(OPENPIC_SRC_VECTOR(irq));
	x &= ~(OPENPIC_IMASK | OPENPIC_SENSE_LEVEL | OPENPIC_SENSE_EDGE);
	if (type == IST_LEVEL)
		x |= OPENPIC_SENSE_LEVEL;
	else
		x |= OPENPIC_SENSE_EDGE;
	openpic_write(OPENPIC_SRC_VECTOR(irq), x);
}

void
openpic_disable_irq(int irq)
{
	u_int x;

	x = openpic_read(OPENPIC_SRC_VECTOR(irq));
	x |= OPENPIC_IMASK;
	openpic_write(OPENPIC_SRC_VECTOR(irq), x);
}

void
openpic_set_priority(int cpu, int pri)
{
	u_int x;

	x = openpic_read(OPENPIC_CPU_PRIORITY(cpu));
	x &= ~OPENPIC_CPU_PRIORITY_MASK;
	x |= pri;
	openpic_write(OPENPIC_CPU_PRIORITY(cpu), x);
}
