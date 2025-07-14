# $NetBSD: Makefile.inc,v 1.5 2025/07/14 10:25:51 kre Exp $

SRCDIR := ${.PARSEDIR}/dist
USE_OPENSSL ?= no

CPPFLAGS+=-DLBL_ALIGN
OPENSSLDIR=     ${NETBSDSRCDIR}/crypto/external/${EXTERNAL_OPENSSL_SUBDIR}

__sed: .USE
	@rm -f ${.TARGET}
	${TOOL_SED} \
	    -e 's,@MAN_FILE_FORMATS@,5,g' \
	    -e 's,@MAN_MISC_INFO@,7,g' \
	    -e 's,@MAN_ADMIN_COMMANDS@,8,g' \
	    -e 's,3PCAP,3,g' < ${.ALLSRC} > ${.TARGET}
