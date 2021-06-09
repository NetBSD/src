#	$NetBSD: Makefile,v 1.2.50.1 2021/06/09 19:46:19 martin Exp $

.include <bsd.own.mk>

.PATH: ${.CURDIR}/../dist

SCRIPTSDIR=/usr/bin
SCRIPTS=pcap-config

MAN=pcap-config.1

pcap-config: pcap-config.in
	@rm -f ${.TARGET}
	${TOOL_SED} -e 's,@libdir@,/usr/lib,g' \
	    -e 's,@includedir@,/usr/include,g' \
	    -e 's,@PACKAGE_NAME@,pcap,g' \
	    -e 's,@prefix@,/usr,g' \
	    -e 's,@exec_prefix@,/usr,g' \
	    -e 's/@V_RPATH_OPT@/-Wl,-rpath,/g' \
	    -e 's,@LIBS@,,g' < ${.ALLSRC} > ${.TARGET}
	chmod a+x ${.TARGET}

CLEANFILES+=	pcap-config

NOPROG=
		
.include <bsd.prog.mk>
