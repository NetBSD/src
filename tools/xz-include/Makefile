#	$NetBSD: Makefile,v 1.2 2018/09/25 11:41:35 joerg Exp $

.include <bsd.hostinit.mk>

#
# Do *not* set HOSTPROG or HOSTLIB at this point otherwise
# it will create a loop trying to extract the object directory.
#
.include "Makefile.inc"

CONFIGURE_ARGS+=	--enable-threads=no --disable-nls

config.status: ${XZSRCDIR}/configure
	${HOST_SH} ${XZSRCDIR}/configure ${CONFIGURE_ARGS} \
	    CC=${HOST_CC:Q}

CLEANFILES+=	config.log config.status config.h libtool stamp-h1

realdepends realall: config.status

.include <bsd.hostprog.mk>
