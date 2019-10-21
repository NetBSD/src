#	$NetBSD: Makefile,v 1.26 2019/10/21 02:36:48 jhigh Exp $

.include <bsd.own.mk>

USE_SHLIBDIR=	yes

.if (defined(MKARGON2) && ${MKARGON2} != "no")
HAVE_ARGON2=1
.endif

LIB=	crypt

SRCS=	crypt.c md5crypt.c bcrypt.c crypt-sha1.c util.c pw_gensalt.c
SRCS+=	hmac_sha1.c

.if defined(HAVE_ARGON2)
SRCS+=		crypt-argon2.c
CFLAGS+=	-DHAVE_ARGON2 -I../../external/apache2/argon2/dist/phc-winner-argon2/include/
LDADD+=		-largon2 
.endif

WARNS?=	5

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
