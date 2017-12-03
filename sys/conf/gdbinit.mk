# $NetBSD: gdbinit.mk,v 1.1.18.2 2017/12/03 11:36:57 jdolecek Exp $

##
## .gdbinit
##

.include "${S}/gdbscripts/Makefile.inc"

EXTRA_CLEAN+= .gdbinit
.gdbinit: Makefile ${S}/gdbscripts/Makefile.inc
	${_MKTARGET_CREATE}
	rm -f .gdbinit
.for __gdbinit in ${SYS_GDBINIT}
	echo "source ${S}/gdbscripts/${__gdbinit}" >> .gdbinit
.endfor
.if defined(GDBINIT) && !empty(GDBINIT)
.for __gdbinit in ${GDBINIT}
	echo "source ${__gdbinit}" >> .gdbinit
.endfor
.endif
