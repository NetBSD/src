#	$NetBSD: Makefile,v 1.69.2.1 1997/05/23 19:58:25 lukem Exp $
#	@(#)Makefile	5.45.1.1 (Berkeley) 5/6/91

# Missing: mp.h

INCS=	a.out.h ar.h assert.h bitstring.h bm.h cpio.h ctype.h db.h dirent.h \
	disktab.h dlfcn.h err.h fnmatch.h fstab.h fts.h glob.h grp.h hesiod.h \
	ieeefp.h iso646.h kvm.h langinfo.h limits.h locale.h malloc.h math.h \
	md4.h memory.h mpool.h ndbm.h netdb.h netgroup.h nlist.h nl_types.h \
	nsswitch.h paths.h pwd.h ranlib.h re_comp.h regex.h regexp.h resolv.h \
	rmt.h search.h setjmp.h sgtty.h signal.h stab.h stddef.h stdio.h \
	stdlib.h string.h strings.h stringlist.h struct.h sysexits.h tar.h \
	time.h ttyent.h tzfile.h unistd.h util.h utime.h utmp.h vis.h
INCS+=	arpa/ftp.h arpa/inet.h arpa/nameser.h arpa/telnet.h arpa/tftp.h
INCS+=	protocols/dumprestore.h protocols/routed.h protocols/rwhod.h \
	protocols/talkd.h protocols/timed.h
INCS+=	rpc/auth.h rpc/auth_unix.h rpc/clnt.h rpc/pmap_clnt.h rpc/pmap_prot.h \
	rpc/pmap_rmt.h rpc/rpc.h rpc/rpc_msg.h rpc/svc.h rpc/svc_auth.h \
	rpc/types.h rpc/xdr.h
INCS+=	rpcsvc/yp_prot.h rpcsvc/ypclnt.h
INCSDIR=/usr/include

NOOBJ=	noobj

.include <bsd.prog.mk>
