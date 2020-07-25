#	$NetBSD: Makefile,v 1.73 2020/07/25 21:00:48 rillig Exp $
#	@(#)Makefile	5.2 (Berkeley) 12/28/90

PROG=	make
SRCS=	arch.c buf.c compat.c cond.c dir.c for.c hash.c job.c main.c
SRCS+=	make.c make_malloc.c metachar.c parse.c
SRCS+=	str.c strlist.c suff.c targ.c trace.c var.c util.c
SRCS+=	lstAppend.c lstAtEnd.c lstAtFront.c lstClose.c lstConcat.c
SRCS+=	lstDatum.c lstDeQueue.c lstDestroy.c lstDupl.c lstEnQueue.c
SRCS+=	lstFind.c lstFindFrom.c lstFirst.c lstForEach.c lstForEachFrom.c
SRCS+=	lstInit.c lstInsert.c lstIsAtEnd.c lstIsEmpty.c lstLast.c lstMember.c
SRCS+=	lstNext.c lstOpen.c lstPrev.c lstRemove.c lstReplace.c lstSucc.c

USE_COVERAGE?=	no		# works only with gcc; clang9 fails to link
.if ${USE_COVERAGE} == "yes"
COPTS=		--coverage -O0 -ggdb
LDADD=		--coverage
CLEANFILES+=	${SRCS:.c=.gcda} ${SRCS:.c=.gcno} ${SRCS:=.gcov}
.endif

USE_META?=	yes
.if ${USE_META:tl} != "no"

SRCS+=		meta.c
CPPFLAGS+=	-DUSE_META

USE_FILEMON?=	ktrace
.  if ${USE_FILEMON:tl} != "no"

.PATH:	${.CURDIR}/filemon
SRCS+=		filemon_${USE_FILEMON}.c
CPPFLAGS+=	-DUSE_FILEMON
CPPFLAGS+=	-DUSE_FILEMON_${USE_FILEMON:tu}

.    if ${USE_FILEMON} == "dev"
FILEMON_H?=	/usr/include/dev/filemon/filemon.h
.      if exists(${FILEMON_H}) && ${FILEMON_H:T} == "filemon.h"
COPTS.filemon_dev.c+= \
		-DHAVE_FILEMON_H -I${FILEMON_H:H}
.      endif
.    endif
.  endif
.endif

.PATH:	${.CURDIR}/lst.lib
SUBDIR.roff+=	PSD.doc
.if make(obj) || make(clean)
SUBDIR+=	unit-tests
.endif

.include <bsd.prog.mk>
.include <bsd.subdir.mk>

CPPFLAGS+=	-DMAKE_NATIVE
COPTS.var.c+=	-Wno-cast-qual
COPTS.job.c+=	-Wno-format-nonliteral
COPTS.parse.c+=	-Wno-format-nonliteral
COPTS.var.c+=	-Wno-format-nonliteral

.if defined(TOOLDIR)
# This is a native NetBSD build, use libutil rather than the local emalloc etc.
CPPFLAGS+=	-DUSE_EMALLOC
LDADD+=		-lutil
DPADD+=		${LIBUTIL}
.endif

COPTS.arch.c+=	${GCC_NO_FORMAT_TRUNCATION}
COPTS.dir.c+=	${GCC_NO_FORMAT_TRUNCATION}
COPTS.main.c+=	${GCC_NO_FORMAT_TRUNCATION} ${GCC_NO_STRINGOP_TRUNCATION}
COPTS.meta.c+=	${GCC_NO_FORMAT_TRUNCATION}
COPTS.parse.c+=	${GCC_NO_FORMAT_TRUNCATION}

# A simple unit-test driver to help catch regressions
test: .MAKE
	cd ${.CURDIR}/unit-tests \
	&& MAKEFLAGS= ${.MAKE} -r -m / TEST_MAKE=${TEST_MAKE:U${.OBJDIR}/${PROG:T}} ${.TARGET}
.if ${USE_COVERAGE} == yes
	gcov ${SRCS}
.endif

accept: .MAKE
	cd ${.CURDIR}/unit-tests && ${.MAKE} ${.TARGET}
