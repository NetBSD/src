#	$NetBSD: Makefile.inc,v 1.1.1.1 1996/06/20 03:12:11 gwr Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

COMPATDIR=	$S/compat/common

COMPAT__SRCS=  compat_exec.c compat_util.c kern_exit_43.c kern_info_09.c \
 kern_info_43.c kern_prot_43.c kern_resource_43.c kern_sig_43.c \
 tty_43.c uipc_syscalls_43.c vfs_syscalls_43.c vm_43.c kern_ipc_10.c

COMPAT_OBJS   = ${COMPAT__SRCS:.c=.o}
COMPAT_DEPENDS= ${COMPAT__SRCS:S@^@${COMPATDIR}/@g}

libcompat.a: ${COMPAT_OBJS}
	@echo building compat library
	@rm -f $@
	@${AR} cq $@ ${COMPATOBJS}
	${RANLIB} $@

clean::
	-rm -f compat_util.h

${COMPAT_OBJS} : compat_util.h

compat_util.h : ${COMPATDIR}/compat_util.h
	cp -p ${COMPATDIR}/compat_util.h $@

compat_exec.o : ${COMPATDIR}/compat_exec.c
	${NORMAL_C}

compat_util.o : ${COMPATDIR}/compat_util.c
	${NORMAL_C}

kern_exit_43.o : ${COMPATDIR}/kern_exit_43.c
	${NORMAL_C}

kern_info_09.o : ${COMPATDIR}/kern_info_09.c
	${NORMAL_C}

kern_info_43.o : ${COMPATDIR}/kern_info_43.c
	${NORMAL_C}

kern_prot_43.o : ${COMPATDIR}/kern_prot_43.c
	${NORMAL_C}

kern_resource_43.o : ${COMPATDIR}/kern_resource_43.c
	${NORMAL_C}

kern_sig_43.o : ${COMPATDIR}/kern_sig_43.c
	${NORMAL_C}

tty_43.o : ${COMPATDIR}/tty_43.c
	${NORMAL_C}

uipc_syscalls_43.o : ${COMPATDIR}/uipc_syscalls_43.c
	${NORMAL_C}

vfs_syscalls_43.o : ${COMPATDIR}/vfs_syscalls_43.c
	${NORMAL_C}

vm_43.o : ${COMPATDIR}/vm_43.c
	${NORMAL_C}

kern_ipc_10.o : ${COMPATDIR}/kern_ipc_10.c
	${NORMAL_C}

