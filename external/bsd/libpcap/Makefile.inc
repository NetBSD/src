# $NetBSD: Makefile.inc,v 1.1.2.2 2020/04/08 14:04:07 martin Exp $

SRCDIR := ${.PARSEDIR}/dist

__sed: .USE
	@rm -f ${.TARGET}
	${TOOL_SED} \
	    -e 's,@MAN_FILE_FORMATS@,5,g' \
	    -e 's,@MAN_MISC_INFO@,7,g' \
	    -e 's,@MAN_ADMIN_COMMANDS@,8,g' \
	    -e 's,3PCAP,3,g' < ${.ALLSRC} > ${.TARGET}
