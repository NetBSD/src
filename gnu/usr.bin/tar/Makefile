#	$NetBSD: Makefile,v 1.21 2002/08/27 14:46:13 lukem Exp $

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=	tar
SRCS=	buffer.c create.c diffarch.c extract.c fnmatch.c getdate.y \
	getoldopt.c getopt.c getopt1.c gnu.c list.c mangle.c names.c port.c \
	regex.c rtapelib.c tar.c update.c version.c
CPPFLAGS+= -DRETSIGTYPE=void -DDIRENT=1 -DHAVE_SYS_MTIO_H=1 -DHAVE_UNISTD_H=1
CPPFLAGS+= -DHAVE_GETGRGID=1 -DHAVE_GETPWUID=1 -DHAVE_STRING_H=1
CPPFLAGS+= -DHAVE_LIMITS_H=1 -DHAVE_STRSTR=1 -DHAVE_VALLOC=1 -DHAVE_MKDIR=1
CPPFLAGS+= -DHAVE_MKNOD=1 -DHAVE_RENAME=1 -DHAVE_FTRUNCATE=1 -DHAVE_GETCWD=1
CPPFLAGS+= -DHAVE_VPRINTF=1 -DNEEDPAD -I${.CURDIR}
CPPFLAGS+= -DHAVE_TERMIOS_H=1
CPPFLAGS+= -DDEF_AR_FILE=\"/dev/rst0\" -DDEFBLOCKING=20
CPPFLAGS+= -DSTDC_HEADERS -DENABLE_SPARSE

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?= -static
.endif

MAN=tar.1

.include <bsd.prog.mk>
