#	$NetBSD: Makefile,v 1.5 2021/01/24 15:43:09 christos Exp $

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-lint
HOST_SRCDIR=	usr.bin/xlint/xlint
HOST_CPPFLAGS=	-DPATH_LIBEXEC=\"${TOOLDIR}/libexec\" \
		-DPATH_LIBLINT=\"${TOOLDIR}/libdata/lint\" \
		-DTARGET_PREFIX=\"${MACHINE_GNU_PLATFORM}-\"


.include "${.CURDIR}/../Makefile.host"

STDBOOL_H=${NETBSDSRCDIR}/${HOST_SRCDIR}/strict-bool-stdbool.h

install: ${TOOLDIR}/libdata/lint/stdbool.h

${TOOLDIR}/libdata/lint/stdbool.h:: ${STDBOOL_H}
	${_MKTARGET_INSTALL}
	mkdir -p ${TOOLDIR}/libdata/lint
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${STDBOOL_H} ${.TARGET}
