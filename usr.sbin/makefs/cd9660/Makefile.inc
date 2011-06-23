#	$NetBSD: Makefile.inc,v 1.1.6.1 2011/06/23 14:20:48 cherry Exp $
#

.PATH:	${.CURDIR}/cd9660 ${NETBSDSRCDIR}/sys/fs/cd9660

CPPFLAGS+=-I${NETBSDSRCDIR}/sys/fs/cd9660

SRCS+=	cd9660_strings.c cd9660_debug.c cd9660_eltorito.c
SRCS+=	cd9660_write.c cd9660_conversion.c iso9660_rrip.c cd9660_archimedes.c

.if !defined(HOSTPROGNAME)
.if defined(HAVE_GCC) || defined(HAVE_PCC)
.for f in cd9660_debug cd9660_write
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor
.endif
.endif
