#	$NetBSD: Makefile,v 1.2 2016/01/13 12:48:51 christos Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	nlsut
SRCS= \
addext.c \
allocsa.c \
argmatch.c \
backupfile.c \
basename.c \
c-ctype.c \
c-strcasecmp.c \
c-strncasecmp.c \
c-strstr.c \
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
full-write.c \
fwriteerror.c \
gcd.c \
getline.c \
gl_linkedhash_list.c \
hash.c \
javacomp.c \
javaexec.c \
javaversion.c \
linebreak.c \
localcharset.c \
lock.c \
mbchar.c \
mbswidth.c \
obstack.c \
pipe.c \
progname.c \
propername.c \
quote.c \
quotearg.c \
safe-read.c \
safe-write.c \
sh-quote.c \
striconv.c \
strnlen1.c \
strstr.c \
tmpdir.c \
wait-process.c \
xallocsa.c \
xasprintf.c \
xerror.c \
xmalloc.c \
xsetenv.c \
xstrdup.c \
xstriconv.c \
xvasprintf.c

.include "${.CURDIR}/../../Makefile.inc"

.PATH:	${DIST}/gettext-tools/gnulib-lib

CPPFLAGS+=-DEXEEXT=\"\" -DLIBDIR=\"/usr/share\"

COPTS.csharpcomp.c = -Wno-stack-protector
COPTS.csharpexec.c = -Wno-stack-protector
COPTS.javacomp.c = -Wno-stack-protector
COPTS.javaexec.c = -Wno-stack-protector

.include <bsd.lib.mk>
