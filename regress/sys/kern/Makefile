#	$NetBSD: Makefile,v 1.15 2002/08/28 12:39:49 drochner Exp $

SUBDIR+= execve extent getcwd ipf lockf pipe sigtramp sysvmsg sysvsem sysvshm \
	unfdpass writev

.include <bsd.own.mk>

.if ${MACHINE_CPU} == "alpha" ||		\
    ${MACHINE_CPU} == "i386" ||			\
    ${MACHINE_CPU} == "m68k" ||			\
    ${MACHINE_CPU} == "powerpc" ||		\
    ${MACHINE_CPU} == "hppa" ||			\
    ${MACHINE_CPU} == "sparc" ||		\
    ${MACHINE_CPU} == "sparc64" ||		\
    ${MACHINE_CPU} == "vax"
SUBDIR+=lock
.endif


.include <bsd.subdir.mk>
