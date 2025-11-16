/* $NetBSD: boot.c,v 1.1 2025/11/16 20:11:47 jmcneill Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>

#include <powerpc/include/psl.h>
#include <powerpc/include/spr.h>

#include "cache.h"
#include "console.h"
#include "gpio.h"
#include "miniipc.h"
#include "sdmmc.h"
#include "timer.h"

static const char * const names[] = {
	"netbsd", "netbsd.gz",
	"onetbsd", "onetbsd.gz",
	"netbsd.old", "onetbsd.old.gz",
};
#define NUMNAMES	__arraycount(names)

static int	exec_netbsd(const char *);

int
main(void)
{
	extern uint8_t edata[], end[];
	int curname;

	memset(&edata, 0, end - edata);	/* clear BSS */

	console_init();

	gpio_set(GPIO_SLOT_LED);

	if (!miniipc_probe()) {
		panic("MINI IPC not found!");
	}

	sdmmc_init();

	for (curname = 0; curname < NUMNAMES; curname++) {
		printf("booting %s ", names[curname]);
		exec_netbsd(names[curname]);
	}

	panic("No bootable kernel found");

	return 0;
}

static int
exec_netbsd(const char *fname)
{
	u_long marks[MARK_MAX];
	void (*entry)(void);
	int fd;

	memset(marks, 0, sizeof(marks));
	fd = loadfile(fname, marks, LOAD_KERNEL);
	if (fd == -1) {
		return -1;
	}

	gpio_clear(GPIO_SLOT_LED);

	entry = (void *)marks[MARK_ENTRY];
	cache_dcbf((void *)marks[MARK_START],
		   marks[MARK_END] - marks[MARK_START]);
	cache_icbi((void *)marks[MARK_START],
		   marks[MARK_END] - marks[MARK_START]);

	entry();
	panic("Unexpected return from kernel");
}

__dead void
_rtt(void)
{
	int led = 0;
	for (;;) {
		if (led) {
			gpio_set(GPIO_SLOT_LED);
		} else {
			gpio_clear(GPIO_SLOT_LED);
		}
		timer_udelay(1000000);
		led ^= 1;
	}
}
