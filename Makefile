#	$Id: Makefile,v 1.13 1993/07/07 12:55:48 brezak Exp $

# NOTE THAT etc *DOES NOT* BELONG IN THE LIST BELOW

SUBDIR+= bin include lib libexec sbin usr.bin usr.sbin share games
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

.include <bsd.subdir.mk>
