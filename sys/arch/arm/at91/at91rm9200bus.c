/*	$Id: at91rm9200bus.c,v 1.4 2023/04/21 15:04:47 skrll Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: at91rm9200bus.c,v 1.4 2023/04/21 15:04:47 skrll Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>

#include <arm/at91/at91rm9200busvar.h>

const struct at91bus_machdep at91rm9200bus = {
	at91rm9200bus_init,
	at91rm9200bus_attach_cn,
	at91rm9200bus_devmap,

	/* clocking support: */
	at91rm9200bus_peripheral_clock,

	/* PIO support: */
	at91rm9200bus_pio_port,
	at91rm9200bus_gpio_mask,

	/* interrupt handling support: */
	at91rm9200bus_intr_init,
	at91rm9200bus_intr_establish,
	at91rm9200bus_intr_disestablish,
	at91rm9200bus_intr_poll,
	at91rm9200bus_intr_dispatch,

	/* configuration */
	at91rm9200bus_peripheral_name,
	at91rm9200bus_search_peripherals
};

void at91rm9200bus_init(struct at91bus_clocks *clocks)
{
	pmap_devmap_register(at91_devmap());
	at91pmc_get_clocks(clocks);
}

const struct pmap_devmap *at91rm9200bus_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
	    DEVMAP_ENTRY(
		AT91RM9200_APB_VBASE,
		AT91RM9200_APB_HWBASE,
		AT91RM9200_APB_SIZE
	    ),
	    DEVMAP_ENTRY_END
	};

	return devmap;
}

void at91rm9200bus_peripheral_clock(int pid, int enable)
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

at91pio_port at91rm9200bus_pio_port(int pid)
{
	switch (pid) {
	case PID_PIOA:	return AT91_PIOA;
	case PID_PIOB:	return AT91_PIOB;
	case PID_PIOC:	return AT91_PIOC;
	case PID_PIOD:	return AT91_PIOD;
	default:		panic("%s: pid %d not valid", __FUNCTION__, pid);
	}

}

uint32_t at91rm9200bus_gpio_mask(int pid)
{
	return 0xFFFFFFFFUL;
}

const char *at91rm9200bus_peripheral_name(int pid)
{
	switch (pid) {
	case PID_FIQ:	return "FIQ";
	case PID_SYSIRQ:return "SYS";
	case PID_PIOA:	return "PIOA";
	case PID_PIOB:	return "PIOB";
	case PID_PIOC:	return "PIOC";
	case PID_PIOD:	return "PIOD";
	case PID_US0:	return "USART0";
	case PID_US1:	return "USART1";
	case PID_US2:	return "USART2";
	case PID_US3:	return "USART3";
	case PID_MCI:	return "MCI";
	case PID_UDP:	return "UDP";
	case PID_TWI:	return "TWI";
	case PID_SPI:	return "SPI";
	case PID_SSC0:	return "SSC0";
	case PID_SSC1:	return "SSC1";
	case PID_SSC2:	return "SSC2";
	case PID_TC0:	return "TC0";
	case PID_TC1:	return "TC1";
	case PID_TC2:	return "TC2";
	case PID_TC3:	return "TC3";
	case PID_TC4:	return "TC4";
	case PID_TC5:	return "TC5";
	case PID_UHP:	return "UHP";
	case PID_EMAC:	return "EMAC";
	case PID_IRQ0:	return "IRQ0";
	case PID_IRQ1:	return "IRQ1";
	case PID_IRQ2:	return "IRQ2";
	case PID_IRQ3:	return "IRQ3";
	case PID_IRQ4:	return "IRQ4";
	case PID_IRQ5:	return "IRQ5";
	case PID_IRQ6:	return "IRQ6";
	default:	panic("%s: invalid pid %d", __FUNCTION__, pid);
	}
}

void at91rm9200bus_search_peripherals(device_t self,
				   device_t (*found_func)(device_t, bus_addr_t, int))
{
	static const struct {
		bus_addr_t	addr;
		int		pid;
	} table[] = {
		{AT91RM9200_PMC_BASE,		-1},
		{AT91RM9200_AIC_BASE,		-1},
		{AT91RM9200_ST_BASE,		PID_SYSIRQ},
		{AT91RM9200_TC0_BASE,		PID_TC0},
		{AT91RM9200_TC1_BASE,		PID_TC1},
		{AT91RM9200_TC2_BASE,		PID_TC2},
		{AT91RM9200_TC3_BASE,		PID_TC3},
		{AT91RM9200_TC4_BASE,		PID_TC4},
		{AT91RM9200_TC5_BASE,		PID_TC5},
		{AT91RM9200_DBGU_BASE,		PID_SYSIRQ},
		{AT91RM9200_PIOA_BASE,		PID_PIOA},
		{AT91RM9200_PIOB_BASE,		PID_PIOB},
		{AT91RM9200_PIOC_BASE,		PID_PIOC},
		{AT91RM9200_PIOD_BASE,		PID_PIOD},
		{AT91RM9200_USART0_BASE,	PID_US0},
		{AT91RM9200_USART1_BASE,	PID_US1},
		{AT91RM9200_USART2_BASE,	PID_US2},
		{AT91RM9200_USART3_BASE,	PID_US3},
		{AT91RM9200_SSC0_BASE,		PID_SSC0},
		{AT91RM9200_SSC1_BASE,		PID_SSC1},
		{AT91RM9200_SSC2_BASE,		PID_SSC2},
		{AT91RM9200_TWI_BASE,		PID_TWI},
		{AT91RM9200_SPI_BASE,		PID_SPI},
		{AT91RM9200_EMAC_BASE,		PID_EMAC},
		{AT91RM9200_UHP_BASE,		PID_UHP},
		{AT91RM9200_UDP_BASE,		PID_UDP},
		{AT91RM9200_MCI_BASE,		PID_MCI},
		{AT91RM9200_RTC_BASE,		PID_SYSIRQ},
		{0, 0}
	};
	int i;

	for (i = 0; table[i].addr; i++)
		(*found_func)(self, table[i].addr, table[i].pid);
}

