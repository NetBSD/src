#	$NetBSD: Makefile,v 1.2 2002/10/09 16:13:07 itojun Exp $

.include <bsd.own.mk>

.include "Makefile.defs"

PROG=	tar
# tar/src
SRCS+=	buffer.c compare.c create.c delete.c extract.c \
	incremen.c list.c mangle.c misc.c names.c rtapelib.c tar.c update.c
SUBDIR=	libtar

LIBTARDIR!=	cd ${.CURDIR}/libtar; ${MAKE} print-objdir

CPPFLAGS+=	-I${DIST}/lib -I${DIST}/src -I${.CURDIR}/libtar -I${LIBTARDIR}

LDADD+=	-lintl
DPADD+=	${LIBINTL}
LDADD+=	${LIBTARDIR}/libtar.a

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?= -static
.endif

.if ${MKNLS} != "no"
.for F in cs.gmo da.gmo de.gmo es.gmo et.gmo fr.gmo id.gmo \
	it.gmo ja.gmo ko.gmo nl.gmo no.gmo pl.gmo pt.gmo \
	pt_BR.gmo ru.gmo sl.gmo sv.gmo tr.gmo
FILES+=		${F}
FILESDIR_${F}=	/usr/share/locale/${F:.gmo=}/LC_MESSAGES
FILESNAME_${F}=	tar.mo
.endfor
.endif

TEXINFO=	tar.texi
INFOFLAGS=	-I${DIST}/doc

MAN=tar.1

.PATH:	${.CURDIR} ${DIST}/src ${DIST}/po ${DIST}/doc

.include <bsd.info.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
