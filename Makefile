#	$Header: /cvsroot/src/Makefile,v 1.10 1993/05/22 07:15:13 cgd Exp $

# NOTE THAT etc *DOES NOT* BELONG IN THE LIST BELOW

SUBDIR+= bin include lib libexec sbin usr.bin usr.sbin share games
SUBDIR+= gnu

.ifmake !(install)
SUBDIR+= regress
.endif

regression-tests:
	@echo Running regression tests...
	@( cd regress; make regress )

.include <bsd.subdir.mk>
