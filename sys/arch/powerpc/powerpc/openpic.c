/*	$NetBSD: openpic.c,v 1.2 2001/02/04 17:35:28 briggs Exp $	*/

#include <sys/types.h>
#include <sys/param.h>
#include <powerpc/openpic.h>

volatile unsigned char *openpic_base;

void
openpic_init(unsigned char *base, int topirq)
{
	int irq, maxirq;
	u_int x;

	openpic_base = (volatile unsigned char *) base;

	x = openpic_read(OPENPIC_FEATURE);
	maxirq = (x >> 16) & 0x7ff;

	/* disable all interrupts */
	for (irq = 0; irq < maxirq; irq++)
		openpic_write(OPENPIC_SRC_VECTOR(irq), OPENPIC_IMASK);

	openpic_set_priority(0, 15);

	/* we don't need 8259 pass through mode */
	x = openpic_read(OPENPIC_CONFIG);
	x |= OPENPIC_CONFIG_8259_PASSTHRU_DISABLE;
	openpic_write(OPENPIC_CONFIG, x);

	/* send all interrupts to cpu 0 */
	for (irq = 0; irq < topirq; irq++)
		openpic_write(OPENPIC_IDEST(irq), 1 << 0);

	for (irq = 0; irq < topirq; irq++) {
		x = OPENPIC_INIT_SRC(irq);
		openpic_write(OPENPIC_SRC_VECTOR(irq), x);
	}

	openpic_write(OPENPIC_SPURIOUS_VECTOR, 0xff);

	openpic_set_priority(0, 0);

	/* clear all pending interrunts */
	for (irq = 0; irq < maxirq; irq++) {
		openpic_read_irq(0);
		openpic_eoi(0);
	}

	for (irq = 0; irq < topirq; irq++)
		openpic_disable_irq(irq);
}

void
openpic_enable_irq(irq, type)
	int irq, type;
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
openpic_disable_irq(irq)
	int irq;
{
	u_int x;

	x = openpic_read(OPENPIC_SRC_VECTOR(irq));
	x |= OPENPIC_IMASK;
	openpic_write(OPENPIC_SRC_VECTOR(irq), x);
}

void
openpic_set_priority(cpu, pri)
	int cpu, pri;
{
	u_int x;

	x = openpic_read(OPENPIC_CPU_PRIORITY(cpu));
	x &= ~OPENPIC_CPU_PRIORITY_MASK;
	x |= pri;
	openpic_write(OPENPIC_CPU_PRIORITY(cpu), x);
}
