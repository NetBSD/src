#	$Id: Makefile,v 1.12 1993/07/04 14:02:11 cgd Exp $

# NOTE THAT etc *DOES NOT* BELONG IN THE LIST BELOW

SUBDIR+= bin include lib libexec sbin usr.bin usr.sbin share games
SUBDIR+= gnu

SUBDIR+= sys

.ifmake !(install)
SUBDIR+= regress
.endif

regression-tests:
	@echo Running regression tests...
	@( cd regress; make regress )

.include <bsd.subdir.mk>
