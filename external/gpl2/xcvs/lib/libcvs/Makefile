#	$NetBSD: Makefile,v 1.6.2.2 2009/05/13 18:53:22 jym Exp $

LIBISPRIVATE=	yes

.include "${.CURDIR}/../../Makefile.inc"

LIB=	cvs

DIST=	${IDIST}/lib
.PATH:	${DIST}
CPPFLAGS+=-I${DIST}

SRCS=	__fpending.c asnprintf.c basename.c canon-host.c canonicalize.c \
	chdir-long.c closeout.c cycle-check.c dirname.c dup-safer.c exitfail.c \
	fd-safer.c getdate.c getdelim.c getline.c gettime.c md5.c \
	memrchr.c openat.c pagealign_alloc.c printf-args.c printf-parse.c \
	quotearg.c regex.c rpmatch.c save-cwd.c sighandle.c strftime.c \
	stripslash.c vasnprintf.c xalloc-die.c xgetcwd.c xgethostname.c \
	xmalloc.c xreadlink.c yesno.c

.include <bsd.lib.mk>
