# $NetBSD: Makefile.inc,v 1.10.8.3 2003/03/15 20:12:31 he Exp $
# Original from FreeBSD, no rcs id.

.include <bsd.own.mk>

LIBINSTALL != cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../lib ${DEBUG} -DBINDIR='"${BINDIR}"'

# If building for Zoularis on Solaris, some other Makefile should do:
#LOCALBASE?= /usr/pkg
#ZOULARISBASE?=  ${LOCALBASE}/bsd
#BINDIR=${ZOULARISBASE}/bin
#TAR_CMD?= ${BINDIR}/tar
#FTP_CMD?= ${BINDIR}/ftp
#CPPFLAGS+=-DTAR_CMD='"${TAR_CMD}"' -DFTP_CMD='"${FTP_CMD}"'
#CHMOD_CMD?= /usr/bin/chmod
#CHOWN_CMD?= /usr/bin/chown
#CHGRP_CMD?= /usr/bin/chgrp
#WARNS=0

# If building for Zoularis on Linux, some other Makefile should do:
#CHMOD_CMD?= /bin/chmod
#CHOWN_CMD?= /bin/chown
#CHGRP_CMD?= /bin/chgrp

# UNDER NO CIRCUMSTANCES SHOULD THIS MAKEFILE TEST THE OUTPUT OF uname!!
# DOING SO BREAKS CROSS COMPILING.

#CHMOD_CMD?= /bin/chmod
#CHOWN_CMD?= /usr/sbin/chown
#CHGRP_CMD?= /usr/bin/chgrp
WARNS=1

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
