/*	$NetBSD: exec_elf32.c,v 1.1 1997/01/22 23:57:20 cgd Exp $	*/

/*
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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
static char *e32rcsid = "$NetBSD: exec_elf32.c,v 1.1 1997/01/22 23:57:20 cgd Exp $";
#endif /* not lint */

#ifndef ELFSIZE
#define ELFSIZE         32
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include "extern.h"

#if (defined(NLIST_ELF32) && (ELFSIZE == 32)) || \
    (defined(NLIST_ELF64) && (ELFSIZE == 64))

#include <sys/exec_elf.h>

#define CONCAT(x,y)     __CONCAT(x,y)
#define ELFNAME(x)      CONCAT(elf,CONCAT(ELFSIZE,CONCAT(_,x)))
#define ELFNAME2(x,y)   CONCAT(x,CONCAT(_elf,CONCAT(ELFSIZE,CONCAT(_,y))))
#define ELFNAMEEND(x)   CONCAT(x,CONCAT(_elf,ELFSIZE))
#define ELFDEFNNAME(x)  CONCAT(ELF,CONCAT(ELFSIZE,CONCAT(_,x)))

int
ELFNAMEEND(check)(int fd, const char *filename)
{
	Elf_Ehdr eh;
	struct stat sb;

	/*
	 * Check the header to maek sure it's an ELF file (of the
	 * appropriate size).
	 */
	if (fstat(fd, &sb) == -1)
		return 0;
	if (sb.st_size < sizeof eh)
		return 0;
	if (read(fd, &eh, sizeof eh) != sizeof eh)
		return 0;

	if (memcmp(eh.e_ident, Elf_e_ident, Elf_e_siz))
                return 0;

        switch (eh.e_machine) {
        ELFDEFNNAME(MACHDEP_ID_CASES)

        default:
                return 0;
        }

	return 1;
}

int
ELFNAMEEND(hide)(int fd, const char *filename)
{

	fprintf(stderr, "%s: ELF%d executables not currently supported\n",
		filename, ELFSIZE);
	return 1;
}

#endif /* include this size of ELF */
