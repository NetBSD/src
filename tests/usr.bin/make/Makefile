# $NetBSD: Makefile,v 1.4 2023/01/19 23:26:15 rillig Exp $

# The tests for make(1) are maintained in src/usr.bin/make/unit-tests
# (UNIT_TESTS_DISTDIR).  We copy them verbatim to ${FILESDIR}/unit-tests
# (UNIT_TESTS_DIR) at install time.
#
# The t_make .sh script (installed as ${FILESDIR}/t_make) is the only
# thing that knows about Atf.  At run time, the t_make simply invokes
# the Makefile in UNIT_TESTS_DIR for each test.

NOMAN=			# defined

.include <bsd.own.mk>

TESTSDIR=		${TESTSBASE}/usr.bin/make

TESTS_SH=		t_make

FILESDIR=		${TESTSDIR}

# Where we install the copies of the files for the tests
UNIT_TESTS_DIR=		${FILESDIR}/unit-tests

# Where we obtain the original files for the tests
UNIT_TESTS_DISTDIR=	${NETBSDSRCDIR}/usr.bin/make/unit-tests

# Which original files to copy
DISTFILES!=	(cd ${UNIT_TESTS_DISTDIR} && echo Makefile *.mk *.inc *.exp)

# Instruct bsd.files.mk to make the copies
.for f in ${DISTFILES:S,^,${UNIT_TESTS_DISTDIR}/,}
FILES+=		${f}
FILESNAME_${f}=	${f:T}
FILESDIR_${f}=	${UNIT_TESTS_DIR}
.endfor

.include <bsd.test.mk>
