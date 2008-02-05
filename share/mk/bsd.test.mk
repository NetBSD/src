# $NetBSD: bsd.test.mk,v 1.4 2008/02/05 17:50:47 drochner Exp $
#

.include <bsd.init.mk>

TESTSBASE=	/usr/tests

_TESTS=		# empty

.if defined(TESTS_CXX)
PROGS_CXX+=	${TESTS_CXX}
LDADD+=		-latf
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
${_T}: ${TESTS_SH_SRC_${_T}} atf-compile-cookie
	${_MKTARGET_BUILD}
	${TOOL_ATF_COMPILE} -o ${.TARGET}.tmp ${.ALLSRC}
	mv ${.TARGET}.tmp ${.TARGET}
.  endfor
.endif

.if ${USETOOLS} == "yes"
atf-compile-cookie: ${TOOL_ATF_COMPILE}
	touch atf-compile-cookie
.else
atf-compile-cookie:
.endif

FILES+=			Atffile
FILESDIR_Atffile=	${TESTSDIR}
.include <bsd.files.mk>

.if !empty(SCRIPTS) || !empty(PROGS_CXX)
.  include <bsd.prog.mk>
.endif
