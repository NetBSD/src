#	$NetBSD: Makefile,v 1.63 2017/12/20 04:46:19 maya Exp $

LIB=		compat
NOPIC=		# defined
LLIBS=		# defined

CPPFLAGS=	${COMPATCPPFLAGS} ${SKIP_AUTODEP::=}

.PATH.c: ${COMPATDIR}

# Common compatibility code, used by all emulations
SRCS=	compat_exec.c

# Include compatibility code common to libcompat and the compat module

.include "Makefile.sysio"
.include "Makefile.sysv"

# only needed during build
libinstall::

.include <bsd.own.mk>
.undef DESTDIR
.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS:O}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o ${LDFLAGS} `NM=${NM} ${LORDER} ${OBJS} | ${TSORT}`

lib${LIB}.po:: ${POBJS:O}
	@echo building profiled ${LIB} library
	@rm -f lib${LIB}.po
	@${LD} -r -o lib${LIB}.po ${LDFLAGS} `NM=${NM} ${LORDER} ${POBJS} | ${TSORT}`

showsources: ${SRCS}
	@echo ${.ALLSRC}
