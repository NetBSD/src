# $NetBSD: Makefile,v 1.5 2008/10/30 21:37:55 mrg Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/data ${SRCDIR}/atf-sh

# Files in ${SRCDIR}/subrs.
FILESDIR=	/usr/share/atf
FILESMODE=	444
FILES=		atf.footer.subr \
		atf.header.subr \
		atf.init.subr
.endif

.include <bsd.prog.mk>
