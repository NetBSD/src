# $NetBSD: Makefile,v 1.5 2006/05/11 23:16:30 mrg Exp $

.include <bsd.own.mk>

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
