#	$NetBSD: Makefile,v 1.22 1999/07/12 21:36:10 aidan Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM definition

.if !exists(${.CURDIR}/../../domestic/usr.bin/login) && \
    !exists(${.CURDIR}/../../crypto-us/usr.bin/login) || \
    defined(EXPORTABLE_SYSTEM) || \
    make(obj) || make(clean) || make(cleandir) || make(distclean)

PROG=	login
SRCS=	login.c
DPADD+=	${LIBUTIL} ${LIBCRYPT}
LDADD+=	-lutil -lcrypt
BINOWN=	root
BINMODE=4555
INSTALLFLAGS=-fschg

.include <bsd.own.mk>

.ifdef SKEY
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+=	-lskey
.endif

.include <bsd.prog.mk>

.else	# EXPORTABLE_SYSTEM

MAN=	login.1

.include <bsd.man.mk>

.endif	# EXPORTABLE_SYSTEM
