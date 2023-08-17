# $NetBSD: Makefile.inc,v 1.4 2023/08/17 15:30:35 christos Exp $

SRCDIR := ${.PARSEDIR}/dist
USE_OPENSSL ?= no

CPPFLAGS+=-DLBL_ALIGN
OPENSSLDIR=     ${NETBSDSRCDIR}/crypto/external/bsd/${EXTERNAL_OPENSSL_SUBDIR}

__sed: .USE
	@rm -f ${.TARGET}
	${TOOL_SED} \
	    -e 's,@MAN_FILE_FORMATS@,5,g' \
	    -e 's,@MAN_MISC_INFO@,7,g' \
	    -e 's,@MAN_ADMIN_COMMANDS@,8,g' \
	    -e 's,3PCAP,3,g' < ${.ALLSRC} > ${.TARGET}
