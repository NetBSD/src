#	$NetBSD: Makefile,v 1.3 2001/10/19 04:12:41 martin Exp $

DIST=	${.CURDIR}/../../dist
.PATH:	${DIST}/nawk

PROG=	awk
SRCS=	awkgram.y b.c lex.c lib.c main.c parse.c proctab.c run.c tran.c
CPPFLAGS+=	-I${DIST}/nawk -I.
LDADD+=	-lm
DPADD+=	${LIBM}
YHEADER=	yes

# XXX work around toolchain/fpu emul lossage
.if ${MACHINE_ARCH} == "sparc64"
CPPFLAGS+=	-msoft-quad-float
.endif

.include <bsd.prog.mk>
