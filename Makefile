#	$Id: Makefile,v 1.16 1994/02/08 23:20:40 cgd Exp $

# NOTE THAT etc *DOES NOT* BELONG IN THE LIST BELOW

SUBDIR+= lib include bin libexec sbin usr.bin usr.sbin share games
SUBDIR+= gnu

SUBDIR+= sys

.if exists(regress)
.ifmake !(install)
SUBDIR+= regress
.endif

regression-tests:
	@echo Running regression tests...
	@( cd regress; make regress )
.endif

afterinstall:
	(cd share/man && make makedb)

build:
	(cd include && make install)
	make cleandir
	(cd lib && make depend && make && make install)
	(cd gnu/lib && make depend && make && make install)
.if exists(kerberosIV)
	(cd kerberosIV && make depend && make && make install)
.endif
	make depend && make && make install

.include <bsd.subdir.mk>
