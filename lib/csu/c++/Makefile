#	$NetBSD: Makefile,v 1.16 2004/03/09 20:04:00 skrll Exp $

.include <bsd.own.mk>

CPPFLAGS+=	-DLIBC_SCCS
.if ${MKPIC} != "no"
COPTS+=		-fPIC
.endif
OBJS=		c++rt0.o

realall: ${OBJS}

c++rt0.o: c++rt0.c
	${COMPILE.c} ${.ALLSRC}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

FILES=${OBJS}
FILESDIR=${LIBDIR}

.include <bsd.prog.mk>
