#	$NetBSD: Makefile,v 1.15 2004/07/02 00:05:23 sjg Exp $

USE_SHLIBDIR=	yes

LIB=	crypt

SRCS=	crypt.c md5crypt.c bcrypt.c crypt-sha1.c util.c

CPPFLAGS+=	-I${.CURDIR}

MAN=	crypt.3
MLINKS= crypt.3 encrypt.3 crypt.3 setkey.3


.if make(*.test)
.SUFFIXES: .test
.c.test:
	${CC} -o $@ -DUNIT_TEST -DMAIN ${LDSTATIC} ${.IMPSRC} ${LDADD}
LDADD+= -L${.OBJDIR} -l${LIB} -L${.OBJDIR}/../../libc
DPADD+= lib${LIB}.a
LDSTATIC?= -static
.include <bsd.prog.mk>
.else
.include <bsd.lib.mk>
.endif
