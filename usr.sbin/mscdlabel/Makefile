# $NetBSD: Makefile,v 1.8.2.1 2012/10/30 19:00:40 yamt Exp $

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

COPTS.iso9660.c+=	-Wno-pointer-sign

.include <bsd.prog.mk>
