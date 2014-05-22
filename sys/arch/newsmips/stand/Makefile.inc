#	$NetBSD: Makefile.inc,v 1.7.4.1 2014/05/22 11:40:01 yamt Exp $

BINDIR= /usr/mdec

# if there is a 'version' file, add rule for vers.c and add it to SRCS
# and CLEANFILES
.if exists(version)
.PHONY: vers.c
vers.c: version
	${_MKTARGET_CREATE}
	${HOST_SH} ${S}/conf/newvers_stand.sh ${${MKREPRO} == "yes" :?:-D} \
	    ${.CURDIR}/version ${MACHINE}
SRCS+=  vers.c
CLEANFILES+= vers.c
.endif

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBCRTI=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=
