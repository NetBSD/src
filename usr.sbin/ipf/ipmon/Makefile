#	$NetBSD: Makefile,v 1.13.2.2 2000/02/29 23:41:11 he Exp $

PROG=	ipmon
MAN=	ipmon.8
CPPFLAGS+=-DLOGFAC=LOG_LOCAL0
.if (${MACHINE_ARCH} == "vax")
# work around internal compiler error in ipmon.c:main() with -O
CFLAGS=-O0

# Nasty hack to avoid internal gcc errors for 1.4.1
#
.if ${MACHINE} == "vax"
ipmon.o:
	 ${CC} ${CPPFLAGS} -c ${.IMPSRC}
.endif
.endif

.include <bsd.prog.mk>
