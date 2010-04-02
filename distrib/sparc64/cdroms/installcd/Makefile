#	$NetBSD: Makefile,v 1.2 2010/04/02 21:29:30 martin Exp $
CDBASE=		sparc64cd		# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE

# for PRINTOBJDIR
.include <bsd.own.mk>

SYSINSTDIR!= cd ${.CURDIR}/../../../utils/sysinst/arch/${MACHINE} && ${PRINTOBJDIR}

CDRELEASE_NOISOS=	true
CDBOOTIMAGEDIR!= cd ${NETBSDSRCDIR}/distrib/sparc64/bootfs && ${PRINTOBJDIR}
CDBOOTIMAGE=    ${CDBOOTIMAGEDIR}/boot.fs
SUN_BOOT_ARGS:=  - - - - ${CDBOOTIMAGE}

CDRUNTIME+=	./bin
CDRUNTIME+=	./dev/MAKEDEV
CDRUNTIME+=	./etc
CDRUNTIME+=	./lib
CDRUNTIME+=	./libexec
CDRUNTIME+=	./mnt
CDRUNTIME+=	./sbin
# CDRUNTIME+=	./stand
CDRUNTIME+=	./tmp
CDRUNTIME+=	./usr/bin/ftp
CDRUNTIME+=	./usr/bin/gzcat
CDRUNTIME+=	./usr/bin/gzip
CDRUNTIME+=	./usr/bin/gunzip
CDRUNTIME+=	./usr/bin/grep
CDRUNTIME+=	./usr/bin/less
CDRUNTIME+=	./usr/bin/more
CDRUNTIME+=	./usr/bin/netstat
CDRUNTIME+=	./usr/bin/progress
CDRUNTIME+=	./usr/bin/sed
CDRUNTIME+=	./usr/bin/sort
CDRUNTIME+=	./usr/bin/tip
CDRUNTIME+=	./usr/bin/vmstat
CDRUNTIME+=	./usr/lib/libbz2.so*
CDRUNTIME+=	./usr/lib/libc.so*
CDRUNTIME+=	./usr/lib/libcurses.so*
CDRUNTIME+=	./usr/lib/libedit.so*
CDRUNTIME+=	./usr/lib/libkvm.so*
CDRUNTIME+=	./usr/lib/libterminfo.so*
CDRUNTIME+=	./usr/lib/libutil.so*
CDRUNTIME+=	./usr/lib/libz.so*
CDRUNTIME+=	./usr/libexec/ld.elf_so
CDRUNTIME+=	./usr/mdec
CDRUNTIME+=	./usr/sbin/chat
CDRUNTIME+=	./usr/sbin/chown
CDRUNTIME+=	./usr/sbin/chroot
CDRUNTIME+=	./usr/sbin/installboot
CDRUNTIME+=	./usr/sbin/pppd
CDRUNTIME+=	./usr/sbin/wiconfig
CDRUNTIME+=	./usr/share/misc/terminfo.db
CDRUNTIME+=	./usr/share/locale

image_md_pre:
	${MKDIR} cdrom/etc
	${CP} ${.CURDIR}/dot.profile cdrom/etc/rc
	${ECHO} "" >> cdrom/etc/rc
	${ECHO} "# exit to single user mode" >> cdrom/etc/rc
	${ECHO} "exit 1" >> cdrom/etc/rc
	${MKDIR} cdrom/mnt2 cdrom/targetroot
	${CP} ${SYSINSTDIR}/sysinst cdrom
	${CHMOD} ugo+rx cdrom/sysinst
	${CP} ${SYSINSTDIR}/sysinstmsgs.?? cdrom
	${MKDIR} cdrom/var
	${MKDIR} cdrom/kern
	${CP} ${RELEASEDIR}/sparc64/binary/kernel/netbsd-GENERIC.gz cdrom/netbsd

image_md_post:
	SUNLABEL=${TOOL_SUNLABEL:Q} \
		${HOST_SH} ${NETBSDSRCDIR}/distrib/common/sunbootcd.sh \
		${CDBASE}.iso ${SUN_BOOT_ARGS}
	

.include "${.CURDIR}/../../../common/Makefile.bootcd"
