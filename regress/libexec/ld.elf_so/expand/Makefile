# $NetBSD: Makefile,v 1.3 2008/03/09 22:44:41 dholland Exp $

NOMAN=# defined (must be above bsd.own.mk)

.include <bsd.own.mk>

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
