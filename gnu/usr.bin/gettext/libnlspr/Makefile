#	$NetBSD: Makefile,v 1.5 2005/05/09 17:19:06 jmc Exp $

MKPRIVATELIB=	yes

.include <bsd.own.mk>

LIB=	nlspr
SRCS= \
dir-list.c \
file-list.c \
format.c \
format-awk.c \
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
localcharset.c \
message.c \
msgl-ascii.c \
msgl-cat.c \
msgl-charset.c \
msgl-english.c \
msgl-equal.c \
msgl-iconv.c \
open-po.c \
plural.c \
plural-eval.c \
plural-count.c \
plural-table.c \
po-charset.c \
po-error.c \
po-gram-gen.c \
po-lex.c \
po-time.c \
read-csharp.c \
read-java.c \
read-mo.c \
read-po-abstract.c \
read-po.c \
read-properties.c \
read-resources.c \
read-stringtable.c \
read-tcl.c \
str-list.c \
write-csharp.c \
write-java.c  \
write-mo.c \
write-po.c \
write-properties.c \
write-qt.c \
write-resources.c \
write-stringtable.c \
write-tcl.c

.include "${.CURDIR}/../Makefile.inc"

.PATH:	${DIST}/gettext/gettext-tools/src ${DIST}/gettext/gettext-tools/lib
CPPFLAGS+=-I${DIST}/gettext/gettext-runtime/intl
CPPFLAGS+=-I${DIST}/gettext/gettext-tools/src
CPPFLAGS+=-DLIBDIR=\"${DESTDIR}/usr/share\"
CPPFLAGS+=-DGETTEXTJAR=\"${DESTDIR}/usr/share/gettext/gettext.jar\"
CPPFLAGS+=-DGETTEXTDATADIR=\"${DESTDIR}/usr/share/gettext\"

.if ${MACHINE_ARCH} == "vax"
depend dependall realdepend realall: stdbool.h

stdbool.h:
	sed -e 's:@HAVE__BOOL@:0:g' \
		${DIST}/gettext/gettext-tools/lib/stdbool_.h > stdbool.h

CLEANFILES+=stdbool.h
CPPFLAGS+=-I.
.endif

.include <bsd.lib.mk>

# Override <sys.mk> double suffix rules to prevent *.y being yacced to *.[ch]
# (The latter already exist in the ${DIST}/gettext directory.)
#
.y.c:
	@${_MKSHECHO} "# skipping yacc ${.CURDIR:T}/${.TARGET:T}"
