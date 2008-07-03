/*	$Id: at91sam9261bus.c,v 1.1.22.1 2008/07/03 18:37:51 simonb Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: at91sam9261bus.c,v 1.1.22.1 2008/07/03 18:37:51 simonb Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>

#include <arm/at91/at91sam9261busvar.h>

const struct at91bus_machdep at91sam9261bus = {
	at91sam9261bus_init,
	at91sam9261bus_attach_cn,
	at91sam9261bus_devmap,

	/* clocking support: */
	at91sam9261bus_peripheral_clock,

	/* PIO support: */
	at91sam9261bus_pio_port,
	at91sam9261bus_gpio_mask,

	/* interrupt handling support: */
	at91sam9261bus_intr_init,
	at91sam9261bus_intr_establish,
	at91sam9261bus_intr_disestablish,
	at91sam9261bus_intr_poll,
	at91sam9261bus_intr_dispatch,

	/* configuration */
	at91sam9261bus_peripheral_name,
	at91sam9261bus_search_peripherals
};

void at91sam9261bus_init(struct at91bus_clocks *clocks)
{
	pmap_devmap_register(at91_devmap());
	at91pmc_get_clocks(clocks);
}

const struct pmap_devmap *at91sam9261bus_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
	    {
		AT91SAM9261_APB_VBASE,
		AT91SAM9261_APB_HWBASE,
		AT91SAM9261_APB_SIZE,
		VM_PROT_READ | VM_PROT_WRITE,
		PTE_NOCACHE
	    },

	    {
		0,
		0,
		0,
		0,
		0
	    }
	};

	return devmap;
}

void at91sam9261bus_peripheral_clock(int pid, int enable)
{
	switch (pid) {
	case PID_UHP:
		if (enable)
			PMCREG(PMC_SCER) = PMC_SCSR_UHP;
		else
			PMCREG(PMC_SCDR) = PMC_SCSR_UHP;
		break;
	}
	at91pmc_peripheral_clock(pid, enable);
}

at91pio_port at91sam9261bus_pio_port(int pid)
{
	switch (pid) {
	case PID_PIOA:	return AT91_PIOA;
	case PID_PIOB:	return AT91_PIOB;
	case PID_PIOC:	return AT91_PIOC;
	default:		panic("%s: pid %d not valid", __FUNCTION__, pid);
	}
	
}

uint32_t at91sam9261bus_gpio_mask(int pid)
{
	return 0xFFFFFFFFUL;
}

const char *at91sam9261bus_peripheral_name(int pid)
{
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
	case PID_SSC0:	return "SSC0";
	case PID_SSC1:	return "SSC1";
	case PID_SSC2:	return "SSC2";
	case PID_TC0:	return "TC0";
	case PID_TC1:	return "TC1";
	case PID_TC2:	return "TC2";
	case PID_UHP:	return "UHP";
	case PID_LCDC:	return "LCDC";
	case PID_IRQ0:	return "IRQ0";
	case PID_IRQ1:	return "IRQ1";
	case PID_IRQ2:	return "IRQ2";
	default:	panic("%s: invalid pid %d", __FUNCTION__, pid);
	}
}

void at91sam9261bus_search_peripherals(device_t self, 
				       device_t (*found_func)(device_t, bus_addr_t, int))
{
	static const struct {
		bus_addr_t	addr;
		int		pid;
	} table[] = {
		{AT91SAM9261_PMC_BASE,		-1},
		{AT91SAM9261_AIC_BASE,		-1},
		{AT91SAM9261_PIT_BASE,		PID_SYSIRQ},
		{AT91SAM9261_TC0_BASE,		PID_TC0},
		{AT91SAM9261_TC1_BASE,		PID_TC1},
		{AT91SAM9261_TC2_BASE,		PID_TC2},
		{AT91SAM9261_DBGU_BASE,		PID_SYSIRQ},
		{AT91SAM9261_PIOA_BASE,		PID_PIOA},
		{AT91SAM9261_PIOB_BASE,		PID_PIOB},
		{AT91SAM9261_PIOC_BASE,		PID_PIOC},
		{AT91SAM9261_USART0_BASE,	PID_US0},
		{AT91SAM9261_USART1_BASE,	PID_US1},
		{AT91SAM9261_USART2_BASE,	PID_US2},
		{AT91SAM9261_SSC0_BASE,		PID_SSC0},
		{AT91SAM9261_SSC1_BASE,		PID_SSC1},
		{AT91SAM9261_SSC2_BASE,		PID_SSC2},
		{AT91SAM9261_TWI_BASE,		PID_TWI},
		{AT91SAM9261_SPI0_BASE,		PID_SPI0},
		{AT91SAM9261_SPI1_BASE,		PID_SPI1},
		{AT91SAM9261_UHP_BASE,		PID_UHP},
		{AT91SAM9261_LCD_BASE,		PID_LCDC},
		{AT91SAM9261_UDP_BASE,		PID_UDP},
		{AT91SAM9261_MCI_BASE,		PID_MCI},
		{0, 0}
	};
	int i;

	for (i = 0; table[i].addr; i++)
		(*found_func)(self, table[i].addr, table[i].pid);
}

