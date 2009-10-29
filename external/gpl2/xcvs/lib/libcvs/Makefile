#	$NetBSD: Makefile,v 1.8 2009/10/29 02:36:48 christos Exp $

LIBISPRIVATE=	yes

.include "${.CURDIR}/../../Makefile.inc"

LIB=	cvs

DIST=	${IDIST}/lib
.PATH:	${DIST}
CPPFLAGS+=-I${DIST}
.y.c .y.h:
	@true

SRCS=	__fpending.c asnprintf.c basename.c canon-host.c canonicalize.c \
	chdir-long.c closeout.c cycle-check.c dirname.c dup-safer.c exitfail.c \
	fd-safer.c getdate.c gettime.c md5.c \
	openat.c pagealign_alloc.c printf-args.c printf-parse.c \
	quotearg.c regex.c rpmatch.c save-cwd.c sighandle.c strftime.c \
	stripslash.c vasnprintf.c xalloc-die.c xgetcwd.c xgethostname.c \
	xmalloc.c xreadlink.c yesno.c

getdate.c: getdate.y

.include <bsd.lib.mk>
