#	$NetBSD: Makefile,v 1.3 2005/05/09 17:19:06 jmc Exp $

MKPRIVATELIB=	yes

.include <bsd.own.mk>

LIB=	nlsut
SRCS= \
addext.c \
allocsa.c \
argmatch.c \
backupfile.c \
basename.c \
c-strcasecmp.c \
c-strncasecmp.c \
classpath.c \
closeout.c \
concatpath.c \
copy-file.c \
csharpcomp.c \
csharpexec.c \
error-progname.c \
error.c \
execute.c \
exitfail.c \
fatal-signal.c \
findprog.c \
fstrcmp.c \
fwriteerror.c \
full-write.c \
gcd.c \
getline.c \
getndelim2.c \
hash.c \
javacomp.c \
javaexec.c \
linebreak.c \
mbswidth.c \
obstack.c \
pipe.c \
progname.c \
quote.c \
quotearg.c \
safe-read.c \
safe-write.c \
sh-quote.c \
stpcpy.c \
stpncpy.c \
tmpdir.c \
wait-process.c \
xallocsa.c \
xerror.c \
xgetcwd.c \
xmalloc.c \
xsetenv.c \
xstrdup.c

.include "${.CURDIR}/../Makefile.inc"

.PATH:	${DIST}/gettext/gettext-tools/lib

CPPFLAGS+=-DEXEEXT=\"\"

.if ${MACHINE_ARCH} == "vax"
depend dependall realdepend realall: stdbool.h

stdbool.h:
	sed -e 's:@HAVE__BOOL@:0:g' \
		${DIST}/gettext/gettext-tools/lib/stdbool_.h > stdbool.h

CLEANFILES+=stdbool.h
CPPFLAGS+=-I.
.endif

.include <bsd.lib.mk>
