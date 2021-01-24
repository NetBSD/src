#	$NetBSD: Makefile,v 1.7 2021/01/24 16:55:09 rillig Exp $

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-lint
HOST_SRCDIR=	usr.bin/xlint/xlint
HOST_CPPFLAGS=	-DPATH_LIBEXEC=\"${TOOLDIR}/libexec\" \
		-DPATH_LINTLIB=\"${TOOLDIR}/libdata/lint\" \
		-DTARGET_PREFIX=\"${MACHINE_GNU_PLATFORM}-\"


.include "${.CURDIR}/../Makefile.host"

STDBOOL_H=${NETBSDSRCDIR}/${HOST_SRCDIR}/strict-bool-stdbool.h

install: ${TOOLDIR}/libdata/lint/strict-bool/stdbool.h

${TOOLDIR}/libdata/lint/strict-bool/stdbool.h:: ${STDBOOL_H}
	${_MKTARGET_INSTALL}
	mkdir -p ${.TARGET:H}
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${STDBOOL_H} ${.TARGET}
