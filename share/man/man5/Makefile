#	$NetBSD: Makefile,v 1.10 1994/11/30 19:31:07 jtc Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

# missing: dump.0 plot.0 

MAN5=	a.out.0 acct.0 core.0 dir.0 disktab.0 ethers.0 fs.0 fstab.0 group.0 \
	hosts.0 link.0 networks.0 passwd.0 pcfs.0 phones.0 printcap.0 \
	protocols.0 remote.0 resolv.conf.0 rpc.0 services.0 shells.0 stab.0 \
	types.0 utmp.0
MLINKS=	dir.5 dirent.5 fs.5 inode.5 utmp.5 wtmp.5 utmp.5 lastlog.5

all: ${MAN5}

clean depend lint tags:

cleandir:
	rm -f ${MAN5}

install: maninstall

.include <bsd.prog.mk>
