/* $NetBSD: acpi_timer.c,v 1.9.10.1 2008/01/23 19:27:33 bouyer Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_timer.c,v 1.9.10.1 2008/01/23 19:27:33 bouyer Exp $");

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <dev/acpi/acpica.h>
#include <dev/acpi/acpi_timer.h>

static int acpitimer_test(void);
static uint32_t acpitimer_delta(uint32_t, uint32_t);
static u_int acpitimer_read_safe(struct timecounter *);
static u_int acpitimer_read_fast(struct timecounter *);

static struct timecounter acpi_timecounter = {
	acpitimer_read_safe,
	0,
	0x00ffffff,
	PM_TIMER_FREQUENCY,
	"ACPI-Safe",
	900,
	NULL,
	NULL,
};

int
acpitimer_init()
{
	uint32_t bits;
	int i, j;
	ACPI_STATUS res;

	res = AcpiGetTimerResolution(&bits);
	if (res != AE_OK)
		return (-1);

	if (bits == 32)
		acpi_timecounter.tc_counter_mask = 0xffffffff;

	j = 0;
	for (i = 0; i < 10; i++)
		j += acpitimer_test();

	if (j >= 10) {
		acpi_timecounter.tc_name = "ACPI-Fast";
		acpi_timecounter.tc_get_timecount = acpitimer_read_fast;
		acpi_timecounter.tc_quality = 1000;
	}
		
	tc_init(&acpi_timecounter);
	aprint_normal("%s %d-bit timer\n", acpi_timecounter.tc_name, bits);

	return (0);
}

static u_int
acpitimer_read_fast(struct timecounter *tc)
{
	uint32_t t;

	AcpiGetTimer(&t);
	return (t);
}

/*
 * Some chipsets (PIIX4 variants) do not latch correctly; there
 * is a chance that a transition is hit.
 */
static u_int
acpitimer_read_safe(struct timecounter *tc)
{
	uint32_t t1, t2, t3;

	AcpiGetTimer(&t2);
	AcpiGetTimer(&t3);
	do {
		t1 = t2;
		t2 = t3;
		AcpiGetTimer(&t3);
	} while ((t1 > t2) || (t2 > t3));
	return (t2);
}

static uint32_t
acpitimer_delta(uint32_t end, uint32_t start)
{
	uint32_t delta;
	u_int mask = acpi_timecounter.tc_counter_mask;

	if (end >= start)
		delta = end - start;
	else
		delta = ((mask - start) + end + 1) & mask;

	return (delta);
}

#define N 2000
static int
acpitimer_test()
{
	uint32_t last, this, delta;
	int minl, maxl, n;

	minl = 10000000;
	maxl = 0;

	x86_disable_intr();
	AcpiGetTimer(&last);
	for (n = 0; n < N; n++) {
		AcpiGetTimer(&this);
		delta = acpitimer_delta(this, last);
		if (delta > maxl)
			maxl = delta;
		else if (delta < minl)
			minl = delta;
		last = this;
	}
	x86_enable_intr();

	if (maxl - minl > 2 )
		n = 0;
	else if (minl < 0 || maxl == 0)
		n = 0;
	else
		n = 1;

	return (n);
}
