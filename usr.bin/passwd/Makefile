#	$NetBSD: Makefile,v 1.24 2000/01/12 05:13:32 mjl Exp $
#	from: @(#)Makefile    8.3 (Berkeley) 4/2/94

SRCTOP=	../..
.include <bsd.crypto.mk>

PROG=	passwd
SRCS=	local_passwd.c yp_passwd.c passwd.c
DPADD+= ${LIBRPCSVC} ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lrpcsvc -lcrypt -lutil
CPPFLAGS+=-I${.CURDIR} -DYP -DLOGIN_CAP

LINKS=	${BINDIR}/passwd ${BINDIR}/yppasswd
MLINKS=	passwd.1 yppasswd.1

BINOWN=	root
BINMODE=4555
.if defined(notyet)
INSTALLFLAGS=-fschg
.endif

.if defined(CRYPTOPATH)
.sinclude "${CRYPTOPATH}/usr.bin/passwd/Makefile.frag"
.endif

.include <bsd.prog.mk>

# getpwent.o: getpwent.c
# 	${COMPILE.c} -UYP ${.IMPSRC}
