#	$NetBSD: Makefile,v 1.90 2000/06/03 11:23:54 fvdl Exp $
#	@(#)Makefile	8.2 (Berkeley) 1/4/94

SRCTOP=	..
.include <bsd.crypto.mk>

# Doing a make includes builds /usr/include

# Missing: mp.h

INCS=	a.out.h ar.h assert.h bitstring.h bm.h cpio.h ctype.h db.h dirent.h \
	disktab.h dlfcn.h err.h errno.h fmtmsg.h fnmatch.h fstab.h fts.h \
	getopt.h glob.h grp.h hesiod.h ieeefp.h ifaddrs.h \
	iso646.h kvm.h langinfo.h libgen.h \
	limits.h link.h link_aout.h link_elf.h locale.h \
	login_cap.h malloc.h math.h md4.h \
	memory.h mpool.h ndbm.h netconfig.h netdb.h netgroup.h nlist.h \
	nl_types.h \
	nsswitch.h paths.h pwd.h ranlib.h re_comp.h regex.h regexp.h \
	resolv.h rmt.h search.h setjmp.h sgtty.h signal.h stab.h stddef.h \
	stdio.h stdlib.h string.h strings.h stringlist.h struct.h sysexits.h \
	tar.h time.h ttyent.h tzfile.h ulimit.h unistd.h util.h utime.h \
	utmp.h vis.h wchar.h
INCS+=	arpa/ftp.h arpa/inet.h arpa/nameser.h arpa/telnet.h arpa/tftp.h
INCS+=	protocols/dumprestore.h protocols/routed.h protocols/rwhod.h \
	protocols/talkd.h protocols/timed.h
INCS+=	rpcsvc/yp_prot.h rpcsvc/ypclnt.h

INCSDIR=/usr/include

MKOBJ=	no

SUBDIR=	rpc

.if defined(CRYPTOPATH)
.sinclude "${CRYPTOPATH}/include/Makefile.frag"
.endif

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
