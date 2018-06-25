#	$NetBSD: Makefile,v 1.3.14.1 2018/06/25 07:25:27 pgoyette Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	nlspr
SRCS= \
clean-temp.c \
dir-list.c \
file-list.c \
format-awk.c \
format-boost.c \
format-c.c \
format-csharp.c \
format-elisp.c \
format-gcc-internal.c \
format-java.c \
format-librep.c \
format-lisp.c \
format-pascal.c \
format-perl-brace.c \
format-perl.c \
format-php.c \
format-python.c \
format-qt.c \
format-scheme.c \
format-sh.c \
format-tcl.c \
format-ycp.c \
format.c \
gettext-po.c \
hash-string.c \
lang-table.c \
localcharset.c \
message.c \
msgl-ascii.c \
msgl-cat.c \
msgl-charset.c \
msgl-check.c \
msgl-english.c \
msgl-equal.c \
msgl-iconv.c \
open-catalog.c \
plural-count.c \
plural-eval.c \
plural-exp.c \
plural-table.c \
po-charset.c \
po-error.c \
po-gram-gen.c \
po-lex.c \
po-time.c \
po-xerror.c \
read-catalog-abstract.c \
read-catalog.c \
read-csharp.c \
read-java.c \
read-mo.c \
read-po.c \
read-properties.c \
read-resources.c \
read-stringtable.c \
read-tcl.c \
str-list.c \
write-catalog.c \
write-csharp.c \
write-java.c  \
write-mo.c \
write-po.c \
write-properties.c \
write-qt.c \
write-resources.c \
write-stringtable.c \
write-tcl.c

.include "${.CURDIR}/../../Makefile.inc"

.PATH:	${DIST}/gettext-tools/src ${DIST}/gettext-tools/gnulib-lib ${DIST}/gettext-runtime/intl ${DIST}/gettext-tools/libgettextpo
CPPFLAGS+=-I${DIST}/gettext-runtime/intl
CPPFLAGS+=-I${DIST}/gettext-tools/src
CPPFLAGS+=-DLIBDIR=\"/usr/share\"
CPPFLAGS+=-DGETTEXTJAR=\"/usr/share/gettext/gettext.jar\"
CPPFLAGS+=-DGETTEXTDATADIR=\"/usr/share/gettext\"

COPTS.clean-temp.c = -Wno-stack-protector
COPTS.format-java.c = -Wno-stack-protector
COPTS.message.c = -Wno-stack-protector
COPTS.msgl-cat.c = -Wno-stack-protector
COPTS.msgl-charset.c = -Wno-stack-protector
COPTS.msgl-iconv.c = -Wno-stack-protector
COPTS.plural-exp.c = -Wno-stack-protector
COPTS.plural.c = -Wno-stack-protector
COPTS.po-charset.c = -Wno-stack-protector
COPTS.po-gram-gen.c = -Wno-stack-protector
COPTS.read-tcl.c = -Wno-stack-protector
COPTS.write-csharp.c = -Wno-stack-protector
COPTS.write-java.c = -Wno-stack-protector
COPTS.write-mo.c = -Wno-stack-protector
COPTS.write-po.c = -Wno-stack-protector
COPTS.write-tcl.c = -Wno-stack-protector
COPTS.msgl-fsearch.c = -Wno-variably-modified

CWARNFLAGS.clang+=	-Wno-tautological-compare
CWARNFLAGS.clang+=	-Wno-format-extra-args

.include <bsd.lib.mk>

# Override <sys.mk> double suffix rules to prevent *.y being yacced to *.[ch]
# (The latter already exist in the ${DIST}/gettext directory.)
#
.y.c:
	@${_MKSHECHO} "# skipping yacc ${.CURDIR:T}/${.TARGET:T}"
