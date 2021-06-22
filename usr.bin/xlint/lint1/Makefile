#	$NetBSD: Makefile,v 1.71 2021/06/22 12:58:28 martin Exp $

.include <bsd.own.mk>

PROG=		lint1
SRCS=		cgram.y \
		ckbool.c ckctype.c ckgetopt.c \
		decl.c emit.c emit1.c err.c func.c init.c inittyp.c lex.c \
		main1.c mem.c mem1.c oper.c print.c scan.l tree.c tyname.c

MAN=		lint.7
YHEADER=
#DBG=		-g
#CPPFLAGS+=	-DYYDEBUG=1
#YFLAGS+=	-v
#LFLAGS+=	-d

CWARNFLAGS.clang+=	-Wno-error=implicit-int-float-conversion
LINTFLAGS+=		-T
LOBJS.${PROG}+=		${SRCS:M*.y:.y=.ln}
#LOBJS.${PROG}+=	${SRCS:M*.l:.l=.ln}
LINTFLAGS.scan.c=	-X 107,330,331,333

CPPFLAGS+=	-DIS_LINT1
CPPFLAGS+=	-I${.CURDIR}
CPPFLAGS+=	${DEBUG:D-DDEBUG}

COPTS.err.c+=	${${ACTIVE_CC} == "clang":? -Wno-format-nonliteral :}

.if ${HAVE_GCC} == 9 &&	\
	(${MACHINE_ARCH} == "sh3el" || ${MACHINE_ARCH} == "sh3eb")
COPTS.lex.c+=	-O0
.endif

BINDIR=		/usr/libexec

CLEANFILES+=	${MAN}
CLEANFILES+=	*.gcno *.gcda *.gcov

.if ${USETOOLS} == "yes"
LINT1=		${TOOLDIR}/libexec/${MACHINE_GNU_PLATFORM}-lint1
.endif
LINT1?=		./${PROG}

${MAN}:		makeman ${LINT1:C/^\.\///} Makefile
	${_MKTARGET_CREATE}
	${HOST_SH} ${.ALLSRC:M*makeman} ${LINT1} -m >${.TARGET}

LDADD+=		-lm
.ifndef HOSTPROG
DPADD+=		${LIBM}

LDADD+=		-ll
DPADD+=		${LIBL}
.endif

.include "Makefile.err-msgs-h"
${SRCS:Nerr.c}: err-msgs.h

add-test: .PHONY
	@set -eu; \
	test=${NAME:Q}; \
	[ "$$test" ] || { \
		echo "usage: ${MAKE} add-test NAME=<name>"; \
		exit; \
	}; \
	\
	cd '../../../tests/usr.bin/xlint/lint1'; \
	if [ -f "$$test.c" ]; then \
		echo "error: test $$test already exists in $$PWD."; \
		exit 1; \
	fi; \
	\
	echo "=> Adding test $$test"; \
	printf '%s\n' \
		'/*	$$''NetBSD$$	*/' \
		"# 3 \"$$test.c\"" \
		'' \
		'/*' \
		' * TODO: Explain the purpose of the test.' \
		' */' \
		'' \
		'/* lint1-extra-flags: -p */' \
		'' \
		'// TODO: Add some code that passes.' \
		'// TODO: Add some code that fails.' \
	> "$$test.c"; \
	> "$$test.exp"; \
	cvs add "$$test.c" "$$test.exp"; \
	printf '%s\n' \
		'/^FILES+=/i' \
		"FILES+=		$$test.c" \
		"FILES+=		$$test.exp" \
		'.' 'w' 'q' \
	| ed Makefile; \
	${MAKE} sync-mi; \
	printf '%s\n' '/^test_case /i' "test_case $$test" '.' 'w' 'q' \
	| ed t_integration.sh

.include <bsd.prog.mk>
