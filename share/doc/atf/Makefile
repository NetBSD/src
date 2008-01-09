# $NetBSD: Makefile,v 1.1.4.2 2008/01/09 01:38:47 matt Exp $

.include <bsd.own.mk>

SRCDIR=	${NETBSDSRCDIR}/dist/atf
.PATH:	${SRCDIR} ${SRCDIR}/doc

.if ${MKDOC} != "no"
FILESDIR=	/usr/share/doc/atf
FILES=		AUTHORS COPYING NEWS README ROADMAP
.endif

MAN=	atf.7 atf-formats.5 atf-test-case.4 atf-test-program.1

CLEANFILES+=	atf.7 atf.7.tmp
atf.7: atf.7.in
	sed -e 's,__DOCDIR__,/usr/share/doc/atf,g' \
	    <${SRCDIR}/doc/atf.7.in >atf.7.tmp
	mv atf.7.tmp atf.7

.include <bsd.files.mk>
.include <bsd.man.mk>
