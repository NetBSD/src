# $NetBSD: Makefile,v 1.2.8.1 2008/03/24 07:14:47 keiichi Exp $

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
