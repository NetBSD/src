#	$NetBSD: Makefile,v 1.7 1998/09/13 23:50:39 perry Exp $
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

# XXX 
# [Note by Perry Metzger, Sept. 1998]
# As it stands, the tags support below is hopelessly
# broken. Just look at the names of the architectures it is dealing
# with and where it expects to find the support for them to see why.
# I've left the stuff in this Makefile alone in the hope that someone
# will fix it someday -- I am assuming that it might help that
# person if they have the old Makefile goo left in place as a starting
# point. 
# XXX

# Kernel tags:
# Tags files are built in the top-level directory for each architecture,
# with a makefile listing the architecture-dependent files, etc.  The list
# of common files is in ./Make.tags.inc.  Links to the correct tags file
# are placed in each source directory.  We need to have links to tags files
# from the generic directories that are relative to the machine type, even
# via remote mounts; therefore we use symlinks to $SYSTAGS, which points at
# ${SYSDIR}/${MACHINE}/tags.

ARCH=	hp300 i386 luna68k news3400 pmax sparc tahoe vax

SYSTAGS=/var/db/sys_tags
SYSDIR=/sys

# Directories in which to place tags links (other than machine-dependent)
DGEN=	conf \
	dev dev/scsi \
	hp hp/dev hp/hpux \
	kern libkern \
	miscfs miscfs/deadfs miscfs/fdesc miscfs/fifofs miscfs/kernfs \
	miscfs/lofs miscfs/nullfs miscfs/portal miscfs/procfs \
	miscfs/specfs miscfs/umapfs miscfs/union \
	net netccitt netinet netiso netns nfs scripts sys \
	ufs ufs/ffs ufs/lfs ufs/mfs ufs/ufs \
	vm

tags::
	-for i in ${ARCH}; do \
	    (cd ../$$i && make ${MFLAGS} tags); done

links::
	rm -f ${SYSTAGS}
	ln -s ${SYSDIR}/${MACHINE}/tags ${SYSTAGS}
	-for i in ${DGEN}; do \
	    (cd ../$$i && { rm -f tags; ln -s ${SYSTAGS} tags; }) done
	-for i in ${ARCH}; do \
	    (cd ../$$i && make ${MFLAGS} SYSTAGS=${SYSTAGS} links); done
