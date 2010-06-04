# $NetBSD: bsd.test.mk,v 1.8 2010/06/04 08:35:09 jmmv Exp $
#

.include <bsd.init.mk>

TESTSBASE=	/usr/tests

_TESTS=		# empty

.if defined(TESTS_C)
PROGS+=		${TESTS_C}
LDADD+=		-latf-c
.  for _T in ${TESTS_C}
BINDIR.${_T}=	${TESTSDIR}
MAN.${_T}?=	# empty
_TESTS+=	${_T}
.  endfor
.endif

.if defined(TESTS_CXX)
PROGS_CXX+=	${TESTS_CXX}
LDADD+=		-latf-c++ -latf-c
.  for _T in ${TESTS_CXX}
BINDIR.${_T}=	${TESTSDIR}
MAN.${_T}?=	# empty
_TESTS+=	${_T}
.  endfor
.endif

.if defined(TESTS_SH)

.  for _T in ${TESTS_SH}
SCRIPTS+=		${_T}
SCRIPTSDIR_${_T}=	${TESTSDIR}

_TESTS+=		${_T}
CLEANFILES+=		${_T} ${_T}.tmp

TESTS_SH_SRC_${_T}?=	${_T}.sh
${_T}: ${TESTS_SH_SRC_${_T}}
	${_MKTARGET_BUILD}
	echo '#! /usr/bin/atf-sh' >${.TARGET}.tmp
	cat ${.ALLSRC} >>${.TARGET}.tmp
	chmod +x ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}
.  endfor
.endif

.if !defined(NOATFFILE)
FILES+=			Atffile
FILESDIR_Atffile=	${TESTSDIR}
.include <bsd.files.mk>
.endif

.if !empty(SCRIPTS) || !empty(PROGS) || !empty(PROGS_CXX)
.  include <bsd.prog.mk>
.endif
