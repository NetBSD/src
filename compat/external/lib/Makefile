#	$NetBSD: Makefile,v 1.13 2009/11/02 10:13:19 plunky Exp $

#
# This Makefile exists to provide a single point to build
# all compat libraries provided by external packages that may
# be used by other applications in the system.
#

.include <bsd.own.mk>

.if (${MKATF} != "no")
SUBDIR+= ../bsd/atf/lib
.endif

.if (${MKCRYPTO} != "no")
.if (${MKLDAP} != "no")
SUBDIR+= ../bsd/openldap/lib
.endif
.endif

SUBDIR+= ../bsd/bind/lib
SUBDIR+= ../bsd/file/lib
SUBDIR+= ../bsd/flex/lib

.if ${MKBINUTILS} != "no" && ${HAVE_BINUTILS} == "219"
SUBDIR+=        ../gpl3/binutils/lib
.endif

SUBDIR+= ../bsd/libevent/lib

.include <bsd.subdir.mk>
