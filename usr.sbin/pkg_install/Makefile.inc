# $NetBSD: Makefile.inc,v 1.9 1999/08/23 10:34:53 agc Exp $
# Original from Freebsd, no rcs id.

LIBINSTALL != cd ${.CURDIR}/../lib;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CPPFLAGS+=-I${.CURDIR}/../lib ${DEBUG} -DBINDIR='"${BINDIR}"'

.if !defined(OPSYS)
OPSYS!= /usr/bin/uname -s
.endif

.if ${OPSYS} == "NetBSD"
CHMOD= /bin/chmod
CHOWN= /usr/sbin/chown
CHGRP= /usr/bin/chgrp
.elif ${OPSYS} == "SunOS"
.if exists(/etc/mk.conf)
.include "/etc/mk.conf"
.endif
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
