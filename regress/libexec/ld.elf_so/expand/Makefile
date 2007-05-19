# $NetBSD: Makefile,v 1.2 2007/05/19 15:38:06 christos Exp $

.include <bsd.own.mk>

MKMAN=no
CPPFLAGS+=-DDEBUG_EXPAND
PROG=expand
.PATH.c: ${NETBSDSRCDIR}/libexec/ld.elf_so
CLEANFILES+=output tst

regress: ${PROG} output
	./${PROG} '$${ORIGIN}' '$$ORIGIN/' '$$PLATFORM' '$$OSNAME' '$$OSREL' \
	    '$$HWCAP/' '$$ISALIST' '$$$$' '$$foo' '$${ORIGIN}:' '$$ORIGIN:' \
	    '$$OSREL/' '$${PLATFORM}$$OSNAME' > tst
	diff output tst

output: Makefile
	(echo '.'; \
	 echo './'; \
	 uname -p; \
	 uname -s; \
	 uname -r; \
	 echo '/'; \
	 echo ; \
	 echo '$$$$'; \
	 echo '$$foo'; \
	 echo '.:'; \
	 echo '.:'; \
	 echo "$$(uname -r)/"; \
	 echo "$$(uname -p)$$(uname -s)") > $@

.include <bsd.prog.mk>
