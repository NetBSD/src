/* $NetBSD: acpi_timer.c,v 1.2.2.2 2006/06/26 12:50:37 yamt Exp $ */

#include <sys/types.h>

#ifdef __HAVE_TIMECOUNTER

#include <sys/systm.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <dev/acpi/acpica.h>
#include <dev/acpi/acpi_timer.h>

static u_int acpitimer_read(struct timecounter *);

static struct timecounter acpi_timecounter = {
	acpitimer_read,
	0,
	0x00ffffff,
	PM_TIMER_FREQUENCY,
	"ACPI_PM_TMR",
	900
};

int
acpitimer_init()
{
	uint32_t bits;
	ACPI_STATUS res;

	res = AcpiGetTimerResolution(&bits);
	if (res != AE_OK)
		return (-1);

	if (bits == 32)
		acpi_timecounter.tc_counter_mask = 0xffffffff;
	tc_init(&acpi_timecounter);

	printf("acpitimer: %d bits\n", bits);

	return (0);
}

/*
 * Some chipsets (PIIX4 variants) do not latch correctly; there
 * is a chance that a transition is hit.
 * For now, just be conservative. We might detect the situation later
 * (by testing, or chipset quirks).
 */
static u_int
acpitimer_read(struct timecounter *tc)
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

#else

int
acpitimer_init()
{

	return (0);
}

#endif
