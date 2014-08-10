/*	Id: platform.c,v 1.5 2012/08/09 11:41:28 ragge Exp 	*/	
/*	$NetBSD: platform.c,v 1.1.1.1.20.1 2014/08/10 07:10:07 tls Exp $	*/

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
#include "config.h"

#include "ccconfig.h"

#ifndef NEW_DRIVER_CFG
#define	USE_OLD_DRIVER_CFG
#endif

#define _MKS(x)		#x
#define MKS(x)		_MKS(x)

#ifndef PREPROCESSOR
#define PREPROCESSOR	"cpp"
#endif

#ifndef COMPILER
#define COMPILER	"ccom"
#endif

#ifndef ASSEMBLER
#define ASSEMBLER	"as"
#endif

#ifndef LINKER
#define LINKER		"ld"
#endif

enum architecture {
	ARCH_ANY,
	ARCH_I386,
	ARCH_X86_64
};

static const struct {
	enum architecture arch;
	const char *name;
} arch_mapping[] = {
#ifdef USE_OLD_DRIVER_CFG
	{ ARCH_ANY, TARGMACH },
#else
	{ ARCH_I386, "i386" },
	{ ARCH_X86_64, "x86_64" } ,
#endif
};

enum os {
	OS_ANY,
	OS_NETBSD,
	OS_LINUX
};

static const struct {
	enum os os;
	const char *name;
} os_mapping[] = {
#ifdef USE_OLD_DRIVER_CFG
	{ OS_ANY, TARGOS },
#else
	{ OS_NETBSD, "netbsd" },
	{ OS_LINUX, "linux" },
#endif
};

struct platform_specific {
	enum architecture arch;
	enum os os;
	const char * const *values;
};

#ifndef USE_OLD_DRIVER_CFG
static const char * const early_program_csu_values0[] = { "crt0.o", NULL };
static const char * const early_program_csu_values1[] = { "crt1.o", NULL };
#else
static const char * const early_program_csu_values2[] = {
	CRT0FILE, NULL
};
#endif

static const struct platform_specific early_program_csu[] = {
#ifdef USE_OLD_DRIVER_CFG
	{ ARCH_ANY, OS_ANY, early_program_csu_values2 },
#else
	{ ARCH_ANY, OS_NETBSD, early_program_csu_values0 },
	{ ARCH_ANY, OS_LINUX, early_program_csu_values1 },
#endif
};

static const char * const late_program_csu_values0[] = {
    "crtend.o", "crtn.o", NULL
};
static const struct platform_specific late_program_csu[] = {
    { ARCH_ANY, OS_ANY, late_program_csu_values0 },
};

static const char * const early_dso_csu_values0[] = {
    "crtio", "crtbeginS.o", NULL
};
static const struct platform_specific early_dso_csu[] = {
    { ARCH_ANY, OS_ANY, early_dso_csu_values0 },
};

static const char * const late_dso_csu_values0[] = {
    "crtendS.o", "crtn.o", NULL
};
static const struct platform_specific late_dso_csu[] = {
    { ARCH_ANY, OS_ANY, late_dso_csu_values0 },
};

static const char * const predefined_macros_values0[] = {
    "-D__x86_64__", "-D__x86_64", "-D__amd64__", "-D__amd64", NULL
};
static const char * const predefined_macros_values1[] = {
    "-D__NetBSD__", "-D__ELF__", NULL
};
static const char * const predefined_macros_values2[] = {
    "-D__linux__", "-D__ELF__", NULL
};
static const char * const predefined_macros_values3[] = {
    "-D__i386__", NULL
};
static const char * const predefined_macros_values4[] = {
    "-D__PCC__=" MKS(PCC_MAJOR),
    "-D__PCC_MINOR__=" MKS(PCC_MINOR),
    "-D__PCC_MINORMINOR__=" MKS(PCC_MINORMINOR),
    "-D__VERSION__=" MKS(VERSSTR),
    "-D__STDC_ISO_10646__=200009L",
    NULL
};
static const char * const predefined_macros_values5[] = {
    "-D__GNUC__=4",
    "-D__GNUC_MINOR__=3",
    "-D__GNUC_PATCHLEVEL__=1",
    "-D__GNUC_STDC_INLINE__=1",
    NULL
};
static const struct platform_specific predefined_macros[] = {
    { ARCH_X86_64, OS_ANY, predefined_macros_values0 },
    { ARCH_ANY, OS_NETBSD, predefined_macros_values1 },
    { ARCH_ANY, OS_LINUX, predefined_macros_values2 },
    { ARCH_I386, OS_ANY, predefined_macros_values3 },
    { ARCH_ANY, OS_ANY, predefined_macros_values4 },
    { ARCH_ANY, OS_ANY, predefined_macros_values5 },
};

static const char * const early_linker_values0[] = {
    "-dynamic-linker", "/libexec/ld.elf_so", NULL
};
static const char * const early_linker_values1[] = {
    "-m", "elf_i386", NULL
};
static const char * const early_linker_values2[] = {
    "-m", "elf_x86_64", NULL
};
static const char * const early_linker_values3[] = {
    "-dynamic-linker", "/lib64/ld-linux-x86-64.so.2", NULL
};
static const char * const early_linker_values4[] = {
    "-m", "elf_i386", NULL
};
static const char * const early_linker_values5[] = {
    "-m", "elf_x86_64", NULL
};
static const struct platform_specific early_linker[] = {
    { ARCH_ANY, OS_NETBSD, early_linker_values0 },
    { ARCH_I386, OS_NETBSD, early_linker_values1 },
    { ARCH_X86_64, OS_NETBSD, early_linker_values2 },
    { ARCH_ANY, OS_LINUX, early_linker_values3 },
    { ARCH_I386, OS_LINUX, early_linker_values4 },
    { ARCH_X86_64, OS_LINUX, early_linker_values5 },
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
	compiler = COMPILER;
	assembler = ASSEMBLER;
	linker = LINKER;
}
