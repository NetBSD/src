#	$NetBSD: Makefile,v 1.7 2011/07/04 21:54:18 mrg Exp $

#Uncomment the next line to enable the new .init fallthru
CPPFLAGS+=	-I${.CURDIR}/../sparc_elf

ELFSIZE=64

.include "${.CURDIR}/../common_elf/Makefile.inc"

# XXX something is wrong with the -O2 crt0.o and the code in ___start
# accesses data outside of the mapped space -> crash
.if defined(HAVE_GCC) && ${HAVE_GCC} == 45
DBG+=   -O1
.endif
