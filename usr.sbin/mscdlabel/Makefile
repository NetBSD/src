# $NetBSD: Makefile,v 1.7.18.1 2011/06/23 14:20:48 cherry Exp $

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

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.iso9660.c+=	-Wno-pointer-sign
.endif

.include <bsd.prog.mk>
