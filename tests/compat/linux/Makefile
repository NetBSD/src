#	$NetBSD: Makefile,v 1.1 2023/08/19 22:56:44 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/compat/linux
TESTS_SH=	t_inotify

TC_PROGS=	h_inotify_init
TC_PROGS+=	h_inotify_directory
TC_PROGS+=	h_inotify_single_file
TC_PROGS+=	h_inotify_watch_change

.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}

LDFLAGS+=	-nostartfiles -static
CFLAGS+=	-I${.CURDIR}/../../../sys
CFLAGS+=	-I${.CURDIR}
CFLAGS+=	-D_STANDALONE

FILESDIR=	${TESTSDIR}

.for _P in ${TESTS_SH}
.if ${MACHINE_ARCH} == "x86_64"
TESTS_SH_SRC_${_P}=	h_common.sh
.else
TESTS_SH_SRC_${_P}=	h_not_supported.sh
.endif
TESTS_SH_SRC_${_P}+=	${_P}.sh
.endfor

.for _P in ${TC_PROGS}
CLEANFILES+=		${_P}
FILES+=			${_P}

.if ${MACHINE_ARCH} == "x86_64"
PROGS+=			${_P}.out
SRCS.${_P}.out=		${_P}.c
SRCS.${_P}.out+=	h_linux.c
SRCS.${_P}.out+=	h_syscall.c
MAN.${_P}.out=		# empty
FILESMODE_${_P}=	${BINMODE}

proginstall-${_P}.out:
	# Do not install

${_P}: ${_P}.out
	${_MKTARGET_CREATE}
	cp ${.ALLSRC} ${.TARGET}
	${ELFEDIT} --output-osabi Linux ${.TARGET}
.else
${_P}:
	echo '' > ${.TARGET}
.endif

realall: ${_P}
.endfor

CLEANFILES+=	${TESTS_SH}
CLEANFILES+=	Atffile

.include <bsd.test.mk>
