/*
 * Copyright (c) 1993 Adam Glass
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Adam Glass.
 * 4. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Adam Glass ``AS IS'' AND
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
 *
 * $Id: autoconf.c,v 1.11 1994/06/28 22:05:41 gwr Exp $
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/dkstat.h>
#include <sys/conf.h>
#include <sys/dmap.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/vmparam.h>
#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/isr.h>

extern void mainbusattach __P((struct device *, struct device *, void *));
     
struct mainbus_softc {
    struct device mainbus_dev;
};

struct cfdriver mainbuscd = 
{ NULL, "mainbus", always_match, mainbusattach, DV_DULL,
      sizeof(struct mainbus_softc), 0};

void mainbusattach(parent, self, args)
     struct device *parent;
     struct device *self;
     void *args;
{
    struct cfdata *new_match;

    printf("\n");
    while (1) {
	new_match = config_search(NULL, self, NULL);
	if (!new_match) break;
	config_attach(self, new_match, NULL, NULL);
    }
}

int nmi_intr(arg)
     int arg;
{
    printf("nmi interrupt received\n");
    return 1;
}

void configure()
{
    int root_found;
    extern int soft1intr();

    isr_init();
    root_found = config_rootfound("mainbus", NULL);
    if (!root_found)
	panic("configure: autoconfig failed, no device tree root found");
    isr_add(7, nmi_intr, 0);
    isr_add(1, soft1intr, 0);
    isr_cleanup();
    conf_init();
}

int always_match(parent, cf, args)
     struct device *parent;
     struct cfdata *cf;
     void *args;
{
    return 1;
}
