#	$NetBSD: Makefile,v 1.3 2004/08/21 12:12:28 rearnsha Exp $
CPPFLAGS+= -I- -I${.CURDIR}

# Temporary hack to work around ld problems when linking Thumb applications
# where the linker does not correctly insert an interworking veneer.
CFLAGS+=-mlong-calls

.include "${.CURDIR}/../common_elf/Makefile.inc"
