#	$Id: Makefile,v 1.2 1994/02/14 04:34:42 chopps Exp $

LIB=	kvm
CFLAGS+=-DLIBC_SCCS
SRCS=	kvm.c
.if (${MACHINE} == "amiga")
CFLAGS+=-D${MACHINE}
.endif

.include <bsd.lib.mk>
