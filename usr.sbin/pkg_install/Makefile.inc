# $NetBSD: Makefile.inc,v 1.14 2001/01/07 08:00:58 enami Exp $
# Original from FreeBSD, no rcs id.

.include <bsd.own.mk>

LIBINSTALL != cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../lib ${DEBUG} -DBINDIR='"${BINDIR}"'

.if !defined(OPSYS)
.if exists(/usr/bin/uname)
OPSYS!= /usr/bin/uname -s
.elif exists(/bin/uname)
OPSYS!= /bin/uname -s
.else
OPSYS="unknown"
.endif
.endif

.if ${OPSYS} == "NetBSD"
CHMOD= /bin/chmod
CHOWN= /usr/sbin/chown
CHGRP= /usr/bin/chgrp
.elif ${OPSYS} == "SunOS"
LOCALBASE?= /usr/local
TAR_CMD?= gtar
TAR_FULLPATHNAME?= ${LOCALBASE}/bin/${TAR_CMD}
CPPFLAGS+=-DTAR_CMD='"${TAR_CMD}"' -DTAR_FULLPATHNAME='"${TAR_FULLPATHNAME}"'
BINDIR=${LOCALBASE}/bsd/bin
CHMOD= /usr/bin/chmod
CHOWN= /usr/bin/chown
CHGRP= /usr/bin/chgrp
WARNS=0
.elif ${OPSYS} == "Linux"
CHMOD= /bin/chmod
CHOWN= /bin/chown
CHGRP= /bin/chgrp
.endif

CPPFLAGS+= -DCHMOD='"${CHMOD}"' -DCHOWN='"${CHOWN}"' -DCHGRP='"${CHGRP}"'

DPADD+=	${LIBINSTALL}/libinstall.a
LDADD+=	-L${LIBINSTALL} -linstall

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif






#CFLAGS+=	-g
#STRIPFLAG=	# unset
#LDFLAGS+=	-L/usr/pkg/lib -lefence
