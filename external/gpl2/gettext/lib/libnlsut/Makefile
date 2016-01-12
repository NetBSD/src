#	$NetBSD: Makefile,v 1.1 2016/01/12 22:58:03 christos Exp $

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
c-strstr.c \
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
gl_linkedhash_list.c \
gcd.c \
getline.c \
hash.c \
lock.c \
javacomp.c \
javaexec.c \
linebreak.c \
mbchar.c \
mbswidth.c \
strnlen1.c \
obstack.c \
pipe.c \
progname.c \
propername.c \
quote.c \
quotearg.c \
safe-read.c \
safe-write.c \
strstr.c \
sh-quote.c \
tmpdir.c \
localcharset.c \
wait-process.c \
xallocsa.c \
xvasprintf.c \
xasprintf.c \
xerror.c \
striconv.c \
xstriconv.c \
xmalloc.c \
xsetenv.c \
javaversion.c \
xstrdup.c

# xgetcwd.c \

.include "${.CURDIR}/../../Makefile.inc"

.PATH:	${DIST}/gettext-tools/gnulib-lib

CPPFLAGS+=-DEXEEXT=\"\" -DLIBDIR=\"/usr/share\"

COPTS.csharpcomp.c = -Wno-stack-protector
COPTS.csharpexec.c = -Wno-stack-protector
COPTS.javacomp.c = -Wno-stack-protector
COPTS.javaexec.c = -Wno-stack-protector

.include <bsd.lib.mk>
