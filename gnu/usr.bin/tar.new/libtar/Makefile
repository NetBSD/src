#	$NetBSD: Makefile,v 1.3 2002/10/13 18:41:49 martin Exp $

NOLINT=		# defined
NOMAN=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

# only needed during build - prevent installation of library
libinstall::

.include <bsd.own.mk>

.include "../Makefile.defs"

LIB=	tar
# tar/lib
SRCS+=	addext.c argmatch.c backupfile.c basename.c dirname.c error.c \
	exclude.c full-write.c getdate.c getopt.c getopt1.c getstr.c \
	hash.c human.c modechange.c msleep.c prepargs.c print-copyr.c \
	quotearg.c safe-read.c save-cwd.c savedir.c \
	unicodeio.c xgetcwd.c xmalloc.c xstrdup.c \
	xstrtoimax.c xstrtoul.c xstrtoumax.c
#SRCS+=	getdate.y
SRCS+=	fnmatch.c getline.c mktime.c

CPPFLAGS+=	-I${DIST}/lib -I${.CURDIR} -I.

.depend fnmatch.o exclude.o: fnmatch.h
fnmatch.h: fnmatch.hin
	cp $> fnmatch.h
CLEANFILES+=	fnmatch.h

.PATH:	${.CURDIR} ${DIST}/lib

.include <bsd.lib.mk>

# don't try to recreate getdate.c
.y.c:
