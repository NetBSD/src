#	$NetBSD: Makefile,v 1.8 2014/07/26 19:34:08 dholland Exp $
CDBASE=		vaxcd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDKERNELS=	${RELEASEDIR}/${MACHINE}/binary/kernel/netbsd-GENERIC.gz	netbsd.gz
CDRELEASE_NOISOS=true
CDMAKEFSEXTRAOPTS+=-F ./fs.spec

CLEANFILES+= fs.spec

# for PRINTOBJDIR
.include <bsd.own.mk>
SYSINSTDIR!= cd ${.CURDIR}/../../../../usr.sbin/sysinst/arch/${MACHINE} && ${PRINTOBJDIR}

# prepare mutliuser install environment
image_md_pre:
	${RM} -f cdrom/boot
	${CP} ${DESTDIR}/usr/mdec/boot cdrom/boot
	${RM} -f bootxx.${MACHINE}
	${CP} ${DESTDIR}/usr/mdec/xxboot bootxx.${MACHINE}
	${RM} -rf cdrom/libexec/dhcpcd-hooks/*
	${MKDIR} -p cdrom/libexec/dhcpcd-hooks
	${CP} ${.CURDIR}/../../../common/10-resolv.conf cdrom/libexec/dhcpcd-hooks
	${CP} ${.CURDIR}/../../../common/99-print-sysinst cdrom/libexec/dhcpcd-hooks
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
	${TOOL_GZIP} -d -f cdrom/netbsd.gz
	echo '. type=dir optional' > ./fs.spec
	echo './dev type=dir optional' >> ./fs.spec
	${HOST_SH} cdrom/dev/MAKEDEV -s all | ${TOOL_SED} -e '1d' -e 's:^\./:./dev/:' >> ./fs.spec
	${MKDIR} cdrom/var
	${MKDIR} cdrom/kern

# make the CD bootable
image_md_post:
	${TOOL_INSTALLBOOT} -m${MACHINE} ${CDIMAGE} bootxx.${MACHINE}

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

.include "${.CURDIR}/../../../common/Makefile.bootcd"
