#	$NetBSD: Makefile,v 1.11 2012/04/04 10:59:44 joerg Exp $

LIBISPRIVATE=	yes

.include "${.CURDIR}/../../Makefile.inc"

LIB=	cvs

DIST=	${IDIST}/lib
.PATH:	${DIST}
CPPFLAGS+=-I${DIST}

SRCS=	__fpending.c asnprintf.c basename.c canon-host.c canonicalize.c \
	chdir-long.c closeout.c cycle-check.c dirname.c dup-safer.c exitfail.c \
	fd-safer.c getdate.c gettime.c md5.c \
	openat.c pagealign_alloc.c printf-args.c printf-parse.c \
	quotearg.c regex.c rpmatch.c save-cwd.c sighandle.c strftime.c \
	stripslash.c vasnprintf.c xalloc-die.c xgetcwd.c xgethostname.c \
	xmalloc.c xreadlink.c yesno.c

CWARNFLAGS.clang+=	-Wno-string-plus-int

.include <bsd.lib.mk>
# bsd.lib.mk includes bsd.sys.mk which defines the suffix rules, so we
# need to override after.
.y.c .y.h:
	@true
