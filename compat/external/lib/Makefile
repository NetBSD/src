#	$NetBSD: Makefile,v 1.9 2009/09/11 16:01:59 mrg Exp $

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

.if ${MKBINUTILS} != "no" && ${HAVE_BINUTILS} == "219"
SUBDIR+=        ../gpl3/binutils/lib
.endif

.include <bsd.subdir.mk>
