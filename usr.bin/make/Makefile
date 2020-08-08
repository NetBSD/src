#	$NetBSD: Makefile,v 1.83 2020/08/08 13:50:23 rillig Exp $
#	@(#)Makefile	5.2 (Berkeley) 12/28/90

PROG=	make
SRCS=	arch.c
SRCS+=  buf.c
SRCS+=  compat.c
SRCS+=  cond.c
SRCS+=  dir.c
SRCS+=  enum.c
SRCS+=  for.c
SRCS+=  hash.c
SRCS+=  job.c
SRCS+=  lst.c
SRCS+=  main.c
SRCS+=	make.c
SRCS+=  make_malloc.c
SRCS+=  metachar.c
SRCS+=  parse.c
SRCS+=	str.c
SRCS+=  strlist.c
SRCS+=  suff.c
SRCS+=  targ.c
SRCS+=  trace.c
SRCS+=  var.c
SRCS+=  util.c
HDRS=   buf.h
HDRS+=  config.h
HDRS+=  dir.h
HDRS+=  enum.h
HDRS+=  hash.h
HDRS+=  job.h
HDRS+=  lst.h
HDRS+=  make.h
HDRS+=  make_malloc.h
HDRS+=  meta.h
HDRS+=  metachar.h
HDRS+=  nonints.h
HDRS+=  pathnames.h
HDRS+=  sprite.h
HDRS+=  strlist.h
HDRS+=  trace.h

# Whether to generate a coverage report after running the tests.
USE_COVERAGE?=	no		# works only with gcc; clang9 fails to link
.if ${USE_COVERAGE} == "yes"
COPTS+=		--coverage -O0 -ggdb
LDADD+=		--coverage
CLEANFILES+=	${SRCS:.c=.gcda} ${SRCS:.c=.gcno} ${SRCS:=.gcov}
.endif

# Whether to compile using the Undefined Behavior Sanitizer (GCC, Clang).
USE_UBSAN?=	no
.if ${USE_UBSAN} == "yes"
COPTS+=		-fsanitize=undefined
LDADD+=		-fsanitize=undefined
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

SUBDIR.roff+=	PSD.doc
.if make(obj) || make(clean)
SUBDIR+=	unit-tests
.endif

${SRCS:M*.c:.c=.o}: ${HDRS}

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

COPTS+=		-Wdeclaration-after-statement

# A simple unit-test driver to help catch regressions
test: .MAKE
	cd ${.CURDIR}/unit-tests \
	&& MAKEFLAGS= ${.MAKE} -r -m / TEST_MAKE=${TEST_MAKE:U${.OBJDIR}/${PROG:T}} ${.TARGET}
.if ${USE_COVERAGE} == yes
	gcov ${SRCS}
	sed -i 's,^\([^:]*\): *[0-9]*:,\1: ,' *.gcov
.endif

accept: .MAKE
	cd ${.CURDIR}/unit-tests && ${.MAKE} ${.TARGET}

retest:
	${.MAKE} -C ${.CURDIR}/unit-tests cleandir
.if ${USE_COVERAGE} == yes
	rm -f *.gcov *.gcda
.endif
	${.MAKE} test
