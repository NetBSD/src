#	$NetBSD: Makefile,v 1.1.1.1 2005/04/29 14:59:32 christos Exp $

PROG=	msggrep

.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.inc.prog"

LIBGREPDIR!=	cd ${.CURDIR}/../libgrep && ${PRINTOBJDIR}
DPADD+= ${LIBGREPDIR}/libgrep.a
LDADD+= -L${LIBGREPDIR} -lgrep

CPPFLAGS+=-I${DIST}/gettext/gettext-tools/libgrep
