#	$NetBSD: Makefile,v 1.18.4.1 2007/09/03 07:02:26 wrstuden Exp $

.include <bsd.own.mk>

SUBDIR+= csu libc libevent libposix libpthread librt libutil

.if (${MKSKEY} != "no")
SUBDIR+= libskey
.endif

.if ${MKCRYPTO} != "no"
SUBDIR+= libcrypto libdes
.endif

.include <bsd.subdir.mk>
