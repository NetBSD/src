/*	$NetBSD: boot.c,v 1.3 2000/07/21 22:26:19 jdolecek Exp $	*/
/*
 * Copyright (c) 1994 Rolf Grossmann
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
 *      This product includes software developed by Rolf Grossmann.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/reboot.h>

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <machine/cpu.h>        /* for NEXT_RAMBASE */

#define LOADADDR        (char *)NEXT_RAMBASE

extern int errno;

/*
 * Boot device is derived from PROM provided information.
 */

extern char *bootprog_rev;
extern char *bootprog_name;
extern int build;
#define KNAMEN 100
char kernel[KNAMEN];
char *entry_point;		/* return value filled in by machdep_start */

extern void rtc_init(void);

extern int try_bootp;

char *
main(char *boot_arg)
{
    printf(">> %s BOOT [%s #%d]\n", bootprog_name, bootprog_rev, build);
    rtc_init();

    try_bootp = 1;

#if 0
    printf("Press return to continue.\n");
    getchar();
#endif

    strcpy(kernel, boot_arg);
    entry_point = NULL;
    
    while (1) {
	errno = 0;
	exec(kernel, LOADADDR, 0);
	if (errno == ENOEXEC && entry_point != NULL) {
          printf("Kernel loaded at 0x%lx\n",(unsigned long)entry_point);
#if 0
          printf("Press return to continue.\n");
          getchar();
#endif
          return entry_point;
        }
	
	printf("load of %s: %s\n", kernel, strerror(errno));
	printf("boot: ");
	gets(kernel);
	/* XXX we have to write this back into boot_arg or even mg->boot* */
	if (kernel[0] == '\0')
		return NULL;
    }
}
