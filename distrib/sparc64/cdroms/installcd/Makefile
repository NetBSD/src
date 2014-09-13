#	$NetBSD: Makefile,v 1.20 2014/09/13 10:18:24 roy Exp $
CDBASE=		sparc64cd		# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE

# for PRINTOBJDIR
.include <bsd.own.mk>

SYSINSTDIR!= cd ${.CURDIR}/../../../../usr.sbin/sysinst/arch/${MACHINE} && ${PRINTOBJDIR}

CDRELEASE_NOISOS=	true
CDBOOTIMAGEDIR!= cd ${NETBSDSRCDIR}/distrib/sparc64/bootfs && ${PRINTOBJDIR}
CDBOOTIMAGE=    ${CDBOOTIMAGEDIR}/boot.fs
SUN_BOOT_ARGS:=  - - - - ${CDBOOTIMAGE}

CDRUNTIME+=	./bin
CDRUNTIME+=	./dev/MAKEDEV
CDRUNTIME+=	./etc
CDRUNTIME+=	./lib
CDRUNTIME+=	./libdata
CDRUNTIME+=	./libexec
CDRUNTIME+=	./mnt
CDRUNTIME+=	./sbin
# CDRUNTIME+=	./stand
CDRUNTIME+=	./tmp
CDRUNTIME+=	./usr/bin/ftp
CDRUNTIME+=	./usr/bin/getopt
CDRUNTIME+=	./usr/bin/gunzip
CDRUNTIME+=	./usr/bin/gzcat
CDRUNTIME+=	./usr/bin/gzip
CDRUNTIME+=	./usr/bin/grep
CDRUNTIME+=	./usr/bin/less
CDRUNTIME+=	./usr/bin/more
CDRUNTIME+=	./usr/bin/netstat
CDRUNTIME+=	./usr/bin/progress
CDRUNTIME+=	./usr/bin/sed
CDRUNTIME+=	./usr/bin/sort
CDRUNTIME+=	./usr/bin/tip
CDRUNTIME+=	./usr/bin/tput
CDRUNTIME+=	./usr/bin/vi
CDRUNTIME+=	./usr/bin/vmstat
.if ${MKBSDTAR} != "no"
CDRUNTIME+=	./usr/lib/libarchive.so*
.endif
CDRUNTIME+=	./usr/lib/libbz2.so*
CDRUNTIME+=	./usr/lib/libc.so*
CDRUNTIME+=	./usr/lib/libcurses.so*
CDRUNTIME+=	./usr/lib/libedit.so*
CDRUNTIME+=	./usr/lib/libgcc_s.so*
CDRUNTIME+=	./usr/lib/libintl.so*
CDRUNTIME+=	./usr/lib/libkvm.so*
CDRUNTIME+=	./usr/lib/liblzma.so*
CDRUNTIME+=	./usr/lib/libterminfo.so*
CDRUNTIME+=	./usr/lib/libutil.so*
CDRUNTIME+=	./usr/lib/libz.so*
.if (${MKCRYPTO} != "no")
CDRUNTIME+=	./usr/lib/libcrypto.so*
CDRUNTIME+=	./usr/lib/libssl.so*
.endif
CDRUNTIME+=	./usr/libexec/ld.elf_so
CDRUNTIME+=	./usr/libexec/getty
CDRUNTIME+=	./usr/mdec
CDRUNTIME+=	./usr/sbin/chat
CDRUNTIME+=	./usr/sbin/chroot
CDRUNTIME+=	./usr/sbin/installboot
CDRUNTIME+=	./usr/sbin/pppd
CDRUNTIME+=	./usr/sbin/wiconfig
CDRUNTIME+=	./usr/share/misc/terminfo.cdb
CDRUNTIME+=	./usr/share/locale

image_md_pre:
	${MKDIR} cdrom/etc
	${HOST_LN} -fs /tmp/gettytab cdrom/etc/gettytab
	${INSTALL} ${COPY} ${.CURDIR}/etc.ttys cdrom/etc/ttys
	${INSTALL} ${COPY} ${.CURDIR}/etc.rc cdrom/etc/rc
	${INSTALL} ${COPY} -m 0555 ${.CURDIR}/install.sh cdrom/install.sh
	${MKDIR} cdrom/mnt2 cdrom/targetroot
	${INSTALL} ${COPY} -m 0555 ${SYSINSTDIR}/sysinst cdrom
	for f in ${SYSINSTDIR}/sysinstmsgs.??; do \
		${INSTALL} ${COPY} $$f cdrom; \
	done
	${MKDIR} cdrom/var
	${MKDIR} cdrom/kern
	${CP} ${RELEASEDIR}/sparc64/binary/kernel/netbsd-GENERIC.gz cdrom/netbsd

image_md_post:
	SUNLABEL=${TOOL_SUNLABEL:Q} \
		${HOST_SH} ${NETBSDSRCDIR}/distrib/common/sunbootcd.sh \
		${CDIMAGE} ${SUN_BOOT_ARGS}
	

.include "${.CURDIR}/../../../common/Makefile.bootcd"
