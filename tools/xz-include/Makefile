#	$NetBSD: Makefile,v 1.4 2021/09/18 01:47:11 christos Exp $

.include <bsd.hostinit.mk>

#
# Do *not* set HOSTPROG or HOSTLIB at this point otherwise
# it will create a loop trying to extract the object directory.
#
.include "Makefile.inc"

CONFIGURE_ARGS+=	--enable-threads=no --disable-nls
.if ${MAKEVERBOSE} == 0
CONFIGURE_ARGS+=	--silent
.endif

config.status: ${XZSRCDIR}/configure
	${HOST_SH} ${XZSRCDIR}/configure ${CONFIGURE_ARGS} \
	    CC=${HOST_CC:Q} CPPFLAGS=${HOST_CPPFLAGS:Q}

CLEANFILES+=	config.log config.status config.h libtool stamp-h1

realdepends realall: config.status

.include <bsd.hostprog.mk>
