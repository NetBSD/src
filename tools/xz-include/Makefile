#	$NetBSD: Makefile,v 1.3 2020/06/13 10:49:17 lukem Exp $

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
	    CC=${HOST_CC:Q}

CLEANFILES+=	config.log config.status config.h libtool stamp-h1

realdepends realall: config.status

.include <bsd.hostprog.mk>
