#	$Id: Makefile,v 1.3 1994/04/23 02:41:59 briggs Exp $

LIB=	kvm
CFLAGS+=-DLIBC_SCCS
SRCS=	kvm.c
.if (${MACHINE} == "amiga") || (${MACHINE} == "mac68k")
CFLAGS+=-D${MACHINE}
.endif

.include <bsd.lib.mk>
