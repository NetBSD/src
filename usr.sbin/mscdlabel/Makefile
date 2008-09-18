# $NetBSD: Makefile,v 1.6.12.1 2008/09/18 04:30:08 wrstuden Exp $

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

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.iso9660.c+=	-Wno-pointer-sign
.endif

.include <bsd.prog.mk>
