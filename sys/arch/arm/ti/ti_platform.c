/* $NetBSD: ti_platform.c,v 1.2 2017/12/19 09:15:23 skrll Exp $ */

#include "opt_fdt_arm.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ti_platform.c,v 1.2 2017/12/19 09:15:23 skrll Exp $");

#include <sys/param.h>

#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>

#include <uvm/uvm_extern.h>

#include <dev/ic/comreg.h>

extern struct bus_space armv7_generic_bs_tag;
extern struct bus_space armv7_generic_a4x_bs_tag;
extern struct arm32_bus_dma_tag armv7_generic_dma_tag;

static const struct pmap_devmap *
am33xx_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY(0xe4c00000, 0x44c00000, 0x00400000),
		DEVMAP_ENTRY(0xe8000000, 0x48000000, 0x01000000),
		DEVMAP_ENTRY(0xea000000, 0x4a000000, 0x01000000),
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
am33xx_platform_init_attach_args(struct fdt_attach_args *faa)
{
	faa->faa_bst = &armv7_generic_bs_tag;
	faa->faa_a4x_bst = &armv7_generic_a4x_bs_tag;
	faa->faa_dmat = &armv7_generic_dma_tag;
}

static void
am33xx_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA ((CONSADDR - 0x44c00000) + 0xe4c00000)
	volatile uint32_t *uartaddr = (volatile uint32_t *)CONSADDR_VA;

	while ((le32toh(uartaddr[com_lsr]) & LSR_TXRDY) == 0)
		;

	uartaddr[com_data] = htole32(c);
#endif
}

static void
wdelay(bus_space_tag_t bst, bus_space_handle_t bsh)
{
	while (bus_space_read_4(bst, bsh, 0x34) != 0)
		delay(10);
}

static void
am33xx_platform_bootstrap(void)
{
	static bus_space_tag_t bst = &armv7_generic_bs_tag;
	static bus_space_handle_t bsh;

	bus_space_map(bst, 0x44e00000, 0x1000, 0, &bsh);
	bus_space_write_4(bst, bsh, 0x508, 0x1); /* CLKSEL_TIMER2_CLK: CLK_M_OSC */
	bus_space_write_4(bst, bsh, 0x50c, 0x1); /* CLKSEL_TIMER3_CLK: CLK_M_OSC */
	bus_space_write_4(bst, bsh, 0x80, 0x2); /* CM_PER_TIMER2_CLKCTRL: MODULEMODE: ENABLE */
	bus_space_write_4(bst, bsh, 0x84, 0x2); /* CM_PER_TIMER3_CLKCTRL: MODULEMODE: ENABLE */
	bus_space_unmap(bst, bsh, 0x1000);

	bus_space_map(bst, 0x48040000, 0x1000, 0, &bsh); /* TIMER2 for delay() */

	bus_space_write_4(bst, bsh, 0x40, 0); /* Load */
	bus_space_write_4(bst, bsh, 0x3c, 0); /* Counter */
	bus_space_write_4(bst, bsh, 0x38, 3); /* Control */

	bus_space_unmap(bst, bsh, 0x1000);

	bus_space_map(bst, 0x44e35000, 0x1000, 0, &bsh);
	wdelay(bst, bsh);
	bus_space_write_4(bst, bsh, 0x48, 0xAAAA);
	wdelay(bst, bsh);
	bus_space_write_4(bst, bsh, 0x48, 0x5555);
	wdelay(bst, bsh);
	bus_space_unmap(bst, bsh, 0x1000);

	bus_space_map(bst, 0x44e00000, 0x1000, 0, &bsh);
	bus_space_write_4(bst, bsh, 0x4d4, 0); /* suspend watch dog */
	bus_space_unmap(bst, bsh, 0x1000);
}

static u_int
am33xx_platform_uart_freq(void)
{
	return 48000000;
}

static void
am33xx_platform_delay(u_int n)
{
	static bus_space_tag_t bst = &armv7_generic_bs_tag;
	static bus_space_handle_t bsh = 0;

	uint32_t cur, prev;
	long ticks = n * 24;

	if (bsh == 0)
		bus_space_map(bst, 0x48040000, 0x1000, 0, &bsh); /* TIMER2 */

	prev = bus_space_read_4(bst, bsh, 0x3c);
	while (ticks > 0) {
		cur = bus_space_read_4(bst, bsh, 0x3c);
		if (cur > prev)
			ticks -= (cur - prev);
		else
			ticks -= (UINT32_MAX - prev + 1 - cur);
		prev = cur;
	}
}

static const struct arm_platform am33xx_platform = {
	.devmap = am33xx_platform_devmap,
	.init_attach_args = am33xx_platform_init_attach_args,
	.early_putchar = am33xx_platform_early_putchar,
	.bootstrap = am33xx_platform_bootstrap,
	.uart_freq = am33xx_platform_uart_freq,
	.delay = am33xx_platform_delay,
};

void dummysetstatclockrate(int);
void
dummysetstatclockrate(int newhz)
{
}
__weak_alias(setstatclockrate, dummysetstatclockrate);

ARM_PLATFORM(am33xx, "ti,am33xx", &am33xx_platform);

