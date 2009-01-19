# $NetBSD: Makefile,v 1.1 2009/01/19 07:13:04 jmmv Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/data ${SRCDIR}/atf-sh

# Files in ${SRCDIR}/subrs.
FILESDIR=	/usr/share/atf
FILESMODE=	444
FILES=		atf.footer.subr \
		atf.header.subr \
		atf.init.subr

.endif

.include <bsd.prog.mk>
