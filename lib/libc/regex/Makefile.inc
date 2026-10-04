#	$NetBSD: Makefile.inc,v 1.11 2026/10/04 03:28:45 mrg Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

# regex sources
.PATH: ${.PARSEDIR}

CPPFLAGS+=-DPOSIX_MISTAKE

SRCS+=	regcomp.c regerror.c regexec.c regfree.c regsub.c

MAN+=	regex.3 re_format.7

MLINKS+=regex.3 regcomp.3 regex.3 regexec.3 regex.3 regerror.3 \
	regex.3 regfree.3 regex.3 regnsub.3 regex.3 regasub.3

LINTFLAGS.regexec.c += -X 129,191
