/*
 * Automatically generated file from /Users/sidqian/Downloads/summer/L2S/netbsd-src/sys/conf/newvers.sh
 * Do not edit.
 */
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

const char ostype[] = "NetBSD";
const char osrelease[] = "10.99.4";
const char sccs[] = "@(#)" "NetBSD 10.99.4 (TESTING) #0: Sun Jun 25 11:17:11 PDT 2023\n"
"	sidqian@Sid-Qians-MacBook-Pro-2.local:/Users/sidqian/Downloads/summer/L2S/netbsd-src/build/obj/sys/arch/luna68k/compile/TESTING\n";
const char version[] = "NetBSD 10.99.4 (TESTING) #0: Sun Jun 25 11:17:11 PDT 2023\n"
"	sidqian@Sid-Qians-MacBook-Pro-2.local:/Users/sidqian/Downloads/summer/L2S/netbsd-src/build/obj/sys/arch/luna68k/compile/TESTING\n";
const char buildinfo[] = "";
const char kernel_ident[] = "TESTING";
const char copyright[] = "Copyright (c) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,\n"
"    2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013,\n"
"    2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023\n"
"    The NetBSD Foundation, Inc.  All rights reserved.\n"
"Copyright (c) 1982, 1986, 1989, 1991, 1993\n"
"    The Regents of the University of California.  All rights reserved.\n"
"\n";

/*
 * NetBSD identity note.
 */
#ifdef __arm__
#define _SHT_NOTE	%note
#else
#define _SHT_NOTE	@note
#endif

#define	_S(TAG)	__STRING(TAG)
__asm(
	".section\t\".note.netbsd.ident\", \"\"," _S(_SHT_NOTE) "\n"
	"\t.p2align\t2\n"
	"\t.long\t" _S(ELF_NOTE_NETBSD_NAMESZ) "\n"
	"\t.long\t" _S(ELF_NOTE_NETBSD_DESCSZ) "\n"
	"\t.long\t" _S(ELF_NOTE_TYPE_NETBSD_TAG) "\n"
	"\t.ascii\t" _S(ELF_NOTE_NETBSD_NAME) "\n"
	"\t.long\t" _S(__NetBSD_Version__) "\n"
	"\t.p2align\t2\n"
);

