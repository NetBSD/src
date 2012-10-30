#	$NetBSD: Makefile.inc,v 1.2.2.1 2012/10/30 19:00:37 yamt Exp $
#

.PATH:	${.CURDIR}/cd9660 ${NETBSDSRCDIR}/sys/fs/cd9660

CPPFLAGS+=-I${NETBSDSRCDIR}/sys/fs/cd9660

SRCS+=	cd9660_strings.c cd9660_debug.c cd9660_eltorito.c
SRCS+=	cd9660_write.c cd9660_conversion.c iso9660_rrip.c cd9660_archimedes.c

.if !defined(HOSTPROGNAME)
.for f in cd9660_debug cd9660_write
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor
.endif
