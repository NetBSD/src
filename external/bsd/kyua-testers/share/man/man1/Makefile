# $NetBSD: Makefile,v 1.1.12.2 2014/05/22 15:45:39 yamt Exp $

SRCDIR=		${NETBSDSRCDIR}/external/bsd/kyua-testers/dist
.PATH:		${SRCDIR}

MAN=		kyua-tester.1

CLEANFILES+=	kyua-tester.1
kyua-tester.1:	kyua-tester.1.in
	sed -e 's,__PACKAGE__,kyua-testers,g' \
	    -e 's,__TMPDIR__,/tmp,g' \
	    <${.ALLSRC} >${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}

.include <bsd.man.mk>
