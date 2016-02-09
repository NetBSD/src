/*	Id: platform.c,v 1.4 2011/05/27 06:32:57 plunky Exp 	*/	
/*	$NetBSD: platform.c,v 1.1.1.1 2016/02/09 20:29:12 plunky Exp $	*/

/*-
 * Copyright (c) 2011 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <string.h>

#include "driver.h"

struct list {
	int *		opt;
	const char *	str;
};

static struct {
	int always;
	int crt0;
	int gcrt0;
	int crt1;
	int gcrt1;
	int crt2;
	int dllcrt2;
	int crti;
	int crtbegin;
	int crtbeginS;
	int crtbeginT;
	int crtend;
	int crtendS;
	int crtn;
} use;

static struct list startfiles[] = {
    { &use.crt0,	"crt0.o"					},
    { &use.gcrt0,	"gcrt0.o"					},
    { &use.crt1,	"crt1.o"					},
    { &use.gcrt1,	"gcrt1.o"					},
    { &use.crt2,	"crt2.o"					},
    { &use.dllcrt2,	"dllcrt2.o"					},
    { &use.crti,	"crti.o"					},
    { &use.crtbegin,	"crtbegin.o"					},
    { &use.crtbeginS,	"crtbeginS.o"					},
    { &use.crtbeginT,	"crtbeginT.o"					},
};

static struct list endfiles[] = {
    { &use.crtend,	"crtend.o"					},
    { &use.crtendS,	"crtendS.o"					},
    { &use.crtn,	"crtn.o"					},
};

struct list early_linker[] = {
    { &use.always,	"-dynamic-linker"				},
    { &os.netbsd,	"/libexec/ld.elf.so"				},
    { &os.linux,	"/lib64/ld-linux-x86-64.so.2"			},
    { &use.always,	"-m"						},
    { &mach.i386,	"elf_386"					},
    { &mach.amd64,	"elf_x86_64"					},
};


static const char * const sysincdir_list_values0[] = {
    "=/usr/include", NULL
};
static const char * const sysincdir_list_values1[] = {
    /* XXX fix up for libpcc? */
    "=/usr/lib/gcc/x86_64-linux-gnu/4.4/include", NULL
};
static const struct platform_specific sysincdir_list[] = {
    { ARCH_ANY, OS_ANY, sysincdir_list_values0 },
    { ARCH_X86_64, OS_LINUX, sysincdir_list_values1 },
};

static const char * const crtdir_list_values0[] = {
    "=/usr/lib/i386", "=/usr/lib", NULL
};
static const char * const crtdir_list_values1[] = {
    "=/usr/lib", NULL
};
static const char * const crtdir_list_values2[] = {
    "=/usr/lib64", "=/usr/lib/gcc/x86_64-linux-gnu/4.4", NULL
};
static const struct platform_specific crtdir_list[] = {
    { ARCH_I386, OS_NETBSD, crtdir_list_values0 },
    { ARCH_X86_64, OS_NETBSD, crtdir_list_values1 },
    { ARCH_X86_64, OS_LINUX, crtdir_list_values2 },
};

static const char * const stdlib_list_values0[] = {
    "-L/usr/lib/gcc/x86_64-linux-gnu/4.4", NULL
};
static const char * const stdlib_list_values1[] = {
    "-lgcc", "--as-needed", "-lgcc_s", "--no-as-needed",
    "-lc", "-lgcc", "--as-needed", "-lgcc_s", "--no-as-needed", NULL
};
static const struct platform_specific stdlib_list[] = {
    { ARCH_X86_64, OS_LINUX, stdlib_list_values0 },
    { ARCH_ANY, OS_ANY, stdlib_list_values1 },
};

static const char * const program_dirs_values0[] = {
    LIBEXECDIR, NULL
};
static const struct platform_specific program_dirs[] = {
    { ARCH_ANY, OS_ANY, program_dirs_values0 },
};

#define	ARRAYLEN(a)	(sizeof(a) / sizeof((a)[0]))
#define	ARRAYPAIR(a)	a, ARRAYLEN(a)

static const struct {
	const struct platform_specific *initializer;
	size_t len;
	struct strlist *list;
} platform_specific_inits[] = {
    { ARRAYPAIR(early_program_csu), &early_program_csu_files },
    { ARRAYPAIR(late_program_csu), &late_program_csu_files },
    { ARRAYPAIR(early_dso_csu), &early_dso_csu_files },
    { ARRAYPAIR(late_dso_csu), &late_dso_csu_files },
    { ARRAYPAIR(predefined_macros), &preprocessor_flags },
    { ARRAYPAIR(early_linker), &early_linker_flags },
    { ARRAYPAIR(sysincdir_list), &sysincdirs },
    { ARRAYPAIR(crtdir_list), &crtdirs },
    { ARRAYPAIR(stdlib_list), &stdlib_flags },
    { ARRAYPAIR(program_dirs), &progdirs },
};

void
init_platform_specific(const char *os_name, const char *arch_name)
{
	enum os os;
	enum architecture arch;
	size_t i, j, len;
	const struct platform_specific *initializer;
	struct strlist *l;

	os = OS_ANY;
	for (i = 0; i < ARRAYLEN(os_mapping); ++i) {
		if (strcmp(os_mapping[i].name, os_name) == 0) {
			os = os_mapping[i].os;
			break;
		}
	}
	if (os == OS_ANY)
		error("unknown Operating System: %s", os_name);

	arch = ARCH_ANY;
	for (i = 0; i < ARRAYLEN(arch_mapping); ++i) {
		if (strcmp(arch_mapping[i].name, arch_name) == 0) {
			arch = arch_mapping[i].arch;
			break;
		}
	}
	if (arch == ARCH_ANY)
		error("unknown architecture: %s", arch_name);

	for (i = 0; i < ARRAYLEN(platform_specific_inits); ++i) {
		initializer = platform_specific_inits[i].initializer;
		len = platform_specific_inits[i].len;
		l = platform_specific_inits[i].list;
		for (j = 0; j < len; ++j) {
			if (initializer[j].arch != arch &&
			    initializer[j].arch != ARCH_ANY)
				continue;
			if (initializer[j].os != os &&
			    initializer[j].os != OS_ANY)
				continue;

			strlist_append_array(l, initializer[j].values);
		}
	}

	preprocessor = PREPROCESSOR;
	compiler = C_COMPILER;
	assembler = ASSEMBLER;
	linker = LINKER;
}
