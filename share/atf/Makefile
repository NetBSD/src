# $NetBSD: Makefile,v 1.1 2007/11/12 14:57:35 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/subrs

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/atf
FILESMODE=	444
FILES=		atf.config.subr \
		atf.footer.subr \
		atf.header.subr \
		atf.init.subr
.endif

realall: atf.config.subr
atf.config.subr: atf.config.subr.in
	sed -e 's,@SH_TRAP_DASH@,yes,g' \
	    <${SRCDIR}/subrs/atf.config.subr.in >atf.config.subr
CLEANFILES+=	atf.config.subr

.include <bsd.prog.mk>
