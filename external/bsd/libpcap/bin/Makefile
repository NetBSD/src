#	$NetBSD: Makefile,v 1.1 2010/12/05 03:00:44 christos Exp $

.include <bsd.own.mk>

.PATH: ${.CURDIR}/../dist

FILESDIR=/usr/bin
FILESMODE=555
FILES=pcap-config
MAN=pcap-config.1

pcap-config: pcap-config.in
	@rm -f ${.TARGET}
	${TOOL_SED} -e 's,@libdir@,/usr/lib,g' \
	    -e 's,@includedir@,/usr/include,g' \
	    -e 's,@LIBS@,,g' < ${.ALLSRC} > ${.TARGET}
	chmod a+x ${.TARGET}
		

.include <bsd.files.mk>
.include <bsd.man.mk>
