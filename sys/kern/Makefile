#	$NetBSD: Makefile,v 1.10 2001/04/05 19:49:40 jhawk Exp $
#
#	from: @(#)Makefile	8.2 (Berkeley) 3/21/94

# Makefile for kernel tags files, init_sysent, etc.

all:
	@echo "make tags, links, init_sysent.c, or vnode_if.c only"
	@false

SYSCALLSRC = makesyscalls.sh syscalls.conf syscalls.master
init_sysent.c syscalls.c ../sys/syscall.h ../sys/syscallargs.h: ${SYSCALLSRC}
	-mv -f init_sysent.c init_sysent.c.bak
	-mv -f syscalls.c syscalls.c.bak
	-mv -f ../sys/syscall.h ../sys/syscall.h.bak
	-mv -f ../sys/syscallargs.h ../sys/syscallargs.h.bak
	sh makesyscalls.sh syscalls.conf syscalls.master

VNODEIFSRC = vnode_if.sh vnode_if.src
vnode_if.c ../sys/vnode_if.h: ${VNODEIFSRC}
	-mv -f vnode_if.c vnode_if.c.bak
	-mv -f ../sys/vnode_if.h ../sys/vnode_if.h.bak
	sh vnode_if.sh vnode_if.src


# Kernel tags:
# Tags files are built in the top-level directory for each architecture,
# with a makefile listing the architecture-dependent files, etc.  The list
# of common files is in ./Make.tags.inc.  Links to the correct tags file
# are placed in each source directory.  We need to have links to tags files
# from the generic directories that are relative to the machine type, even
# via remote mounts; therefore we use symlinks to $SYSTAGS, which points at
# ${SYSDIR}/arch/${MACHINE}/tags.

# Note: only a couple of architectures are currently working right. Add more
# to ARCH as you fix them. Use the i386 as a model.

ARCH=	i386 mac68k

SYSTAGS=/var/db/sys_tags
SYSDIR=..

.ifmake links
# Directories in which to place tags links (other than machine-dependent):
# The invocation below returns every directory that contains sources, the idea
# being, in the face of a constantly varying source tree, that this will be 
# easier to maintain than a fixed list.
DGEN!=	(cd ${SYSDIR};							\
	    find -H . -name arch -prune -o -name "*.[chs]" -print |	\
	    sed -e 's@^\./@@' -e 's@/[^/]*\..*@@' | sort -t / -u )
.endif

tags::
	-for i in ${ARCH}; do \
	    (cd ../arch/$$i && ${MAKE} ${MFLAGS} tags); done

links::
	rm -f ${SYSTAGS}
	ln -s ${SYSDIR}/arch/${MACHINE}/tags ${SYSTAGS}
	-for i in ${DGEN}; do \
	    (cd ../$$i && { rm -f tags; ln -s ${SYSTAGS} tags; }); done
	-for i in ${ARCH}; do \
	    (cd ../arch/$$i && ${MAKE} ${MFLAGS} SYSTAGS=${SYSTAGS} links); done
