# $NetBSD: Makefile.inc,v 1.28 2007/07/16 19:33:02 joerg Exp $
# Original from FreeBSD, no rcs id.

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/dist/pkg_install

USE_FORT?=yes	# network client

LIBINSTALL != cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
CPPFLAGS+=-I${DIST}/lib -I${.CURDIR}/../lib ${DEBUG} -DBINDIR='"${BINDIR}"'
CPPFLAGS+=-DHAVE_CONFIG_H

# UNDER NO CIRCUMSTANCES SHOULD THIS MAKEFILE TEST THE OUTPUT OF uname!!
# DOING SO BREAKS CROSS COMPILING.

#CHMOD_CMD?= /bin/chmod
#CHOWN_CMD?= /usr/sbin/chown
#CHGRP_CMD?= /usr/bin/chgrp
WARNS=2

#CPPFLAGS+= -DCHMOD_CMD='"${CHMOD_CMD}"' -DCHOWN_CMD='"${CHOWN_CMD}"'
#CPPFLAGS+= -DCHGRP_CMD='"${CHGRP_CMD}"'

DPADD+=	${LIBINSTALL}/libinstall.a
LDADD+=	-L${LIBINSTALL} -linstall -ltermcap

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

#CFLAGS+=	-g
#STRIPFLAG=	# unset
#LDFLAGS+=	-L/usr/pkg/lib -lefence
