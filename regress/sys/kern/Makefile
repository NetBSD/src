#	$NetBSD: Makefile,v 1.14 2002/08/28 07:41:02 gmcgarry Exp $

SUBDIR+= execve extent getcwd ipf lockf pipe sigtramp sysvmsg sysvsem sysvshm \
	unfdpass writev

.if ${MACHINE_CPU} == "alpha" ||		\
    ${MACHINE_CPU} == "i386" ||			\
    ${MACHINE_CPU} == "m68k" ||			\
    ${MACHINE_CPU} == "powerpc" ||		\
    ${MACHINE_CPU} == "hppa" ||			\ 
    ${MACHINE_CPU} == "sparc" ||		\
    ${MACHINE_CPU} == "sparc64" ||		\
    ${MACHINE_CPU} == "vax")
SUBDIR+=lock
.endif


.include <bsd.subdir.mk>
