/*
 * Copyright (c) 1994 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou.
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

#ifndef lint
/*static char sccsid[] = "from: @(#)reboot.c	5.11 (Berkeley) 2/27/91";*/
static char rcsid[] = "$Id: reboot.c,v 1.1 1994/04/18 08:47:34 cgd Exp $";
#endif /* not lint */

#include <sys/signal.h>
#include <sys/syscall.h>
#include <sys/reboot.h>
#include <string.h>
#include <unistd.h>

#define errprint(x)	write(STDERR_FILENO, x, strlen(x))

main(argc, argv)
	int argc;
	char **argv;
{
	int howto;
	char *name;

	name = strrchr(argv[0], '/');
	if (name == NULL)
		name = argv[0];
	else
		name++;
	if (strcmp("halt", name) == 0)
		howto = RB_HALT;
	else
		howto = RB_AUTOBOOT;

	syscall(SYS_reboot, howto);
	errprint(name);
	errprint(" failed");
	kill(1, SIGHUP);
	exit(1);
}
