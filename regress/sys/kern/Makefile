#	$NetBSD: Makefile,v 1.16 2002/08/29 03:46:39 gmcgarry Exp $

SUBDIR+= execve extent getcwd ipf lockf pipe ras \
	sigtramp sysvmsg sysvsem sysvshm unfdpass writev

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
