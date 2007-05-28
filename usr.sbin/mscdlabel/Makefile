# $NetBSD: Makefile,v 1.6 2007/05/28 12:06:38 tls Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# reads potentially untrustworthy data

PROG=	mscdlabel
SRCS=	main.c dkcksum.c iso9660.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
MAN=	mscdlabel.8

DISKLABEL_SRC=	${NETBSDSRCDIR}/sbin/disklabel

.PATH:	${DISKLABEL_SRC}
CPPFLAGS+=	-I${DISKLABEL_SRC}

.if ${HAVE_GCC} == 4
COPTS.iso9660.c+=	-Wno-pointer-sign
.endif

.include <bsd.prog.mk>
