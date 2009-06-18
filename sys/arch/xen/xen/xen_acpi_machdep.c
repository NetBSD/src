/*	$NetBSD: xen_acpi_machdep.c,v 1.4.24.1 2009/06/18 11:14:14 cegger Exp $	*/

#include "acpi.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xen_acpi_machdep.c,v 1.4.24.1 2009/06/18 11:14:14 cegger Exp $");

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>
#define ACPI_MACHDEP_PRIVATE
#include <machine/acpi_machdep.h>
#include <xen/xen3-public/version.h>

static int
xen_dom0_sleepenter(int sleep_state, int pm1a_control, int pm1b_control)
{
	struct xen_platform_op op = {
		.cmd = XENPF_enter_acpi_sleep,
		.interface_version = XENPF_INTERFACE_VERSION,
		.u = {
			.enter_acpi_sleep = {
				.pm1a_cnt_val = pm1a_control,
				.pm1b_cnt_val = pm1b_control,
				.sleep_state = sleep_state,
			},
		},
	};
 
	return HYPERVISOR_platform_op(&op);
}

static ACPI_STATUS
acpi_md_get_pm1controls(uint32_t *pm1a_control, uint32_t *pm1b_control)
{
	ACPI_STATUS status = 0;
	uint32_t pm1a, pm1b;
	struct acpi_bit_register_info *sleep_type_reg_info;
	struct acpi_bit_register_info *sleep_enable_reg_info;

	*pm1a_control = *pm1b_control = 0;

	sleep_type_reg_info =
		AcpiHwGetBitRegisterInfo(ACPI_BITREG_SLEEP_TYPE_A);
	sleep_enable_reg_info =
		AcpiHwGetBitRegisterInfo(ACPI_BITREG_SLEEP_ENABLE);

	status = AcpiHwRegisterRead(ACPI_MTX_DO_NOT_LOCK,
					ACPI_REGISTER_PM1_CONTROL, &pm1a);
	if (ACPI_FAILURE(status))
		return status;

	/* Clear SLP_EN and SLP_TYP fields */
	pm1a &= ~(sleep_type_reg_info->AccessBitMask |
		  sleep_enable_reg_info->AccessBitMask);
	pm1b = pm1a;

	/* Insert SLP_TYP bits */
	pm1a |= (AcpiGbl_SleepTypeA << sleep_type_reg_info->BitPosition);
	pm1b |= (AcpiGbl_SleepTypeB << sleep_type_reg_info->BitPosition);

	/* Split the writes of SLP_TYP and SLP_EN to workaround
	 * poorly implemented hardware
	 */

	/* Write #1: fill in SLP_TYP data */
	status = AcpiHwRegisterWrite(ACPI_MTX_DO_NOT_LOCK,
					ACPI_REGISTER_PM1A_CONTROL,
					pm1a);
	if (ACPI_FAILURE(status))
		return status;

	status = AcpiHwRegisterWrite(ACPI_MTX_DO_NOT_LOCK,
					ACPI_REGISTER_PM1B_CONTROL,
					pm1b);
	if (ACPI_FAILURE(status))
		return status;

	/* Insert SLP_ENABLE bit */

	pm1a |= sleep_enable_reg_info->AccessBitMask;
	pm1b |= sleep_enable_reg_info->AccessBitMask;

	ACPI_FLUSH_CPU_CACHE();

	*pm1a_control = pm1a;
	*pm1b_control = pm1b;

	return status;
}

int
acpi_md_sleep(int state)
{
	int error;
	ACPI_STATUS status;
	uint32_t pm1a_control, pm1b_control;
	uint32_t xen_version, xen_vermajor, xen_verminor;

	xen_version = HYPERVISOR_xen_version(XENVER_version, NULL);
	xen_vermajor = (xen_version & 0xffff0000) >> 16;
	xen_verminor = (xen_version & 0x0000ffff);

	/* Xen version check. We require Xen 3.x */
	KASSERT(xen_vermajor >= 3);

	/* Xen 3.2 and older have no s3 support. */
	if (xen_verminor < 3) {
		printf("xenacpi: Xen 3.2 and older have no S3 support.\n");
		return -1;
	}

	status = acpi_md_get_pm1controls(&pm1a_control, &pm1b_control);
	if (ACPI_FAILURE(status)) {
		printf("xenacpi: can't enter sleep mode. acpi error %i\n",
			status);
		return -1;
	}

	error = xen_dom0_sleepenter(state, pm1a_control, pm1b_control);

	return error;
}
