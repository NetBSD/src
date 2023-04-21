/*	$NetBSD: at91sam9260bus.c,v 1.2 2023/04/21 15:00:48 skrll Exp $	*/
/*
 * Copied from at91sam9261bus.c
 * Adaptation to AT91SAM9260 by Aymeric Vincent is in the public domain.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: at91sam9260bus.c,v 1.2 2023/04/21 15:00:48 skrll Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>

#include <arm/at91/at91sam9260busvar.h>

const struct at91bus_machdep at91sam9260bus = {
	at91sam9260bus_init,
	at91sam9260bus_attach_cn,
	at91sam9260bus_devmap,

	/* clocking support: */
	at91sam9260bus_peripheral_clock,

	/* PIO support: */
	at91sam9260bus_pio_port,
	at91sam9260bus_gpio_mask,

	/* interrupt handling support: */
	at91sam9260bus_intr_init,
	at91sam9260bus_intr_establish,
	at91sam9260bus_intr_disestablish,
	at91sam9260bus_intr_poll,
	at91sam9260bus_intr_dispatch,

	/* configuration */
	at91sam9260bus_peripheral_name,
	at91sam9260bus_search_peripherals
};

void at91sam9260bus_init(struct at91bus_clocks *clocks) {
	pmap_devmap_register(at91_devmap());
	at91pmc_get_clocks(clocks);
}

const struct pmap_devmap *at91sam9260bus_devmap(void) {
	static const struct pmap_devmap devmap[] = {
	    {
		AT91SAM9260_APB_VBASE,
		AT91SAM9260_APB_HWBASE,
		AT91SAM9260_APB_SIZE,
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE
	    },

	    {
		0, 0, 0, 0, 0
	    }
	};

	return devmap;
}

void at91sam9260bus_peripheral_clock(int pid, int enable) {
	switch (pid) {
	case PID_UHP:
		if (enable)
			PMCREG(PMC_SCER) = PMC_SCSR_SAM_UHP;
		else
			PMCREG(PMC_SCDR) = PMC_SCSR_SAM_UHP;
		break;
	}
	at91pmc_peripheral_clock(pid, enable);
}

at91pio_port at91sam9260bus_pio_port(int pid) {
	switch (pid) {
	case PID_PIOA:	return AT91_PIOA;
	case PID_PIOB:	return AT91_PIOB;
	case PID_PIOC:	return AT91_PIOC;
	default:	panic("%s: pid %d not valid", __FUNCTION__, pid);
	}

}

uint32_t at91sam9260bus_gpio_mask(int pid) {
	return 0xFFFFFFFFUL;
}

const char *at91sam9260bus_peripheral_name(int pid) {
	switch (pid) {
	case PID_FIQ:	return "FIQ";
	case PID_SYSIRQ:return "SYS";
	case PID_PIOA:	return "PIOA";
	case PID_PIOB:	return "PIOB";
	case PID_PIOC:	return "PIOC";
	case PID_US0:	return "USART0";
	case PID_US1:	return "USART1";
	case PID_US2:	return "USART2";
	case PID_MCI:	return "MCI";
	case PID_UDP:	return "UDP";
	case PID_TWI:	return "TWI";
	case PID_SPI0:	return "SPI0";
	case PID_SPI1:	return "SPI1";
	case PID_SSC:	return "SSC";
	case PID_TC0:	return "TC0";
	case PID_TC1:	return "TC1";
	case PID_TC2:	return "TC2";
	case PID_UHP:	return "UHP";
	case PID_EMAC:	return "EMAC";
	case PID_US3:	return "USART3";
	case PID_US4:	return "USART4";
	case PID_US5:	return "USART5";
	case PID_TC3:	return "TC3";
	case PID_TC4:	return "TC4";
	case PID_TC5:	return "TC5";
	case PID_IRQ0:	return "IRQ0";
	case PID_IRQ1:	return "IRQ1";
	case PID_IRQ2:	return "IRQ2";
	default:	panic("%s: invalid pid %d", __FUNCTION__, pid);
	}
}

void at91sam9260bus_search_peripherals(device_t self,
			   device_t found_func(device_t, bus_addr_t, int)) {
	static const struct {
		bus_addr_t	addr;
		int		pid;
	} table[] = {
		{AT91SAM9260_PMC_BASE,		-1},
		{AT91SAM9260_AIC_BASE,		-1},
		{AT91SAM9260_PIT_BASE,		PID_SYSIRQ},
		{AT91SAM9260_TC0_BASE,		PID_TC0},
		{AT91SAM9260_TC1_BASE,		PID_TC1},
		{AT91SAM9260_TC2_BASE,		PID_TC2},
		{AT91SAM9260_DBGU_BASE,		PID_SYSIRQ},
		{AT91SAM9260_PIOA_BASE,		PID_PIOA},
		{AT91SAM9260_PIOB_BASE,		PID_PIOB},
		{AT91SAM9260_PIOC_BASE,		PID_PIOC},
		{AT91SAM9260_USART0_BASE,	PID_US0},
		{AT91SAM9260_USART1_BASE,	PID_US1},
		{AT91SAM9260_USART2_BASE,	PID_US2},
		{AT91SAM9260_SSC_BASE,		PID_SSC},
//		{AT91SAM9260_EMAC_BASE,		PID_EMAC},
		{AT91SAM9260_TWI_BASE,		PID_TWI},
		{AT91SAM9260_SPI0_BASE,		PID_SPI0},
		{AT91SAM9260_SPI1_BASE,		PID_SPI1},
		{AT91SAM9260_UHP_BASE,		PID_UHP},
		{AT91SAM9260_UDP_BASE,		PID_UDP},
		{AT91SAM9260_MCI_BASE,		PID_MCI},
		{0, 0}
	};
	int i;

	for (i = 0; table[i].addr; i++)
		found_func(self, table[i].addr, table[i].pid);
}

