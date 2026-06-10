#	$NetBSD: Makefile,v 1.20 2026/06/10 22:25:02 christos Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

# -DEXTENDED 
# 	if you want the paste & spaste macros.
.include <bsd.own.mk>

PROG=		m4
CPPFLAGS+=	-DEXTENDED -I${.CURDIR}/lib -I${.CURDIR}
SRCS=	parser.y tokenizer.l eval.c expr.c look.c main.c misc.c gnum4.c trace.c
.PATH: ${.CURDIR}/lib
SRCS+=	ohash.c
YHEADER=1
.if (${HOSTPROG:U} == "")
DPADD+=		${LIBUTIL} ${LIBM}
LDADD+=		-lutil -lm
.endif

tokenizer.o: parser.h

CLEANFILES+=parser.c parser.h tokenizer.o

.include <bsd.prog.mk>
