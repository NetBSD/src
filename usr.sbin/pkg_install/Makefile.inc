# $NetBSD: Makefile.inc,v 1.20 2002/09/15 23:56:57 thorpej Exp $
# Original from FreeBSD, no rcs id.

.include <bsd.own.mk>

LIBINSTALL != cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../lib ${DEBUG} -DBINDIR='"${BINDIR}"'

# If building for Zoularis on Solaris, some other Makefile should do:
#LOCALBASE?= /usr/pkg
#TAR_CMD?= tar
#FTP_CMD?= ftp
#ZOULARISBASE?=  ${LOCALBASE}/bsd
#TAR_FULLPATHNAME?= ${ZOULARISBASE}/bin/${TAR_CMD}
#FTP_FULLPATHNAME?= ${ZOULARISBASE}/bin/${FTP_CMD}
#CPPFLAGS+=-DTAR_CMD='"${TAR_CMD}"' -DTAR_FULLPATHNAME='"${TAR_FULLPATHNAME}"'
#CPPFLAGS+=-DFTP_CMD='"${FTP_CMD}"' -DFTP_FULLPATHNAME='"${FTP_FULLPATHNAME}"'
#BINDIR=${ZOULARISBASE}/bin
#CHMOD?= /usr/bin/chmod
#CHOWN?= /usr/bin/chown
#CHGRP?= /usr/bin/chgrp
#WARNS=0

# If building for Zoularis on Linux, some other Makefile should do:
#CHMOD?= /bin/chmod
#CHOWN?= /bin/chown
#CHGRP?= /bin/chgrp

# UNDER NO CIRCUMSTANCES SHOULD THIS MAKEFILE TEST THE OUTPUT OF uname!!
# DOING DO BREAKS CROSS COMPILING.

CHMOD?= /bin/chmod
CHOWN?= /usr/sbin/chown
CHGRP?= /usr/bin/chgrp
WARNS=2

CPPFLAGS+= -DCHMOD='"${CHMOD}"' -DCHOWN='"${CHOWN}"' -DCHGRP='"${CHGRP}"'

DPADD+=	${LIBINSTALL}/libinstall.a
LDADD+=	-L${LIBINSTALL} -linstall

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

#CFLAGS+=	-g
#STRIPFLAG=	# unset
#LDFLAGS+=	-L/usr/pkg/lib -lefence
