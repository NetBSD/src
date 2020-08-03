#	$NetBSD: Makefile.inc,v 1.2 2020/08/03 17:23:34 christos Exp $

.include "${.PARSEDIR}/../Makefile.inc"
WARNS?=		2

.SUFFIXES: .1in .1 .5in .5 .8in .8

# XXX: .USE nodes don't work with suffix rules
.1in.1:
	 ${_MKTARGET_CREATE}
	 ${TOOL_SED} \
	     -e "s,@BIND9_VERSION@,${MAJORVER}.${MINORVER}.${PATCHVER}," \
	     -e "s,@RELEASE_DATE@,," \
	     -e "s,@BIND9_VERSION_STRING@,," \
	     -e "s,@sysconfdir@,//etc," \
	     -e "s,@plugindir@,/usr/lib/named," < ${.ALLSRC} > ${.TARGET}
.5in.5:
	 ${_MKTARGET_CREATE}
	 ${TOOL_SED} \
	     -e "s,@BIND9_VERSION@,${MAJORVER}.${MINORVER}.${PATCHVER}," \
	     -e "s,@RELEASE_DATE@,," \
	     -e "s,@BIND9_VERSION_STRING@,," \
	     -e "s,@sysconfdir@,//etc," \
	     -e "s,@plugindir@,/usr/lib/named," < ${.ALLSRC} > ${.TARGET}
.8in.8:
	 ${_MKTARGET_CREATE}
	 ${TOOL_SED} \
	     -e "s,@BIND9_VERSION@,${MAJORVER}.${MINORVER}.${PATCHVER}," \
	     -e "s,@RELEASE_DATE@,," \
	     -e "s,@BIND9_VERSION_STRING@,," \
	     -e "s,@sysconfdir@,//etc," \
	     -e "s,@plugindir@,/usr/lib/named," < ${.ALLSRC} > ${.TARGET}
