# $NetBSD: Makefile.inc,v 1.25 2003/09/23 05:39:41 grant Exp $
# Original from FreeBSD, no rcs id.

.include <bsd.own.mk>

LIBINSTALL != cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../lib ${DEBUG} -DBINDIR='"${BINDIR}"'

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
