#	$NetBSD: Makefile,v 1.4.10.1 2007/08/28 18:04:34 matt Exp $
CPPFLAGS+= -I${.CURDIR}

# Temporary hack to work around ld problems when linking Thumb applications
# where the linker does not correctly insert an interworking veneer.
CFLAGS+=-mlong-calls

CPUFLAGS.crti.c+= -marm
CPUFLAGS.crtn.c+= -marm

.include "${.CURDIR}/../common_elf/Makefile.inc"
