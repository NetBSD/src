#	$NetBSD: Makefile,v 1.1.1.2 1997/07/24 21:22:36 christos Exp $

PROG= amd
SRCS=	conf_parse.c conf_tok.c srvr_nfs.c srvr_afs.c sched.c rpc_fwd.c \
restart.c opts.c ops_ufs.c ops_sfsx.c ops_sfs.c ops_pfs.c \
ops_pcfs.c ops_nfsx.c ops_nfs3.c ops_nfs.c ops_mfs.c ops_ifs.c \
ops_host.c ops_efs.c ops_cdfs.c ops_afs.c nfs_subr.c nfs_start.c \
nfs_prot_svc.c mntfs.c mapc.c map.c info_union.c info_passwd.c \
info_nis.c info_ndbm.c info_file.c get_args.c conf.c clock.c \
autil.c amq_svc.c amq_subr.c amd.c am_ops.c 
MAN+= amd.8
CLEANFILES+=conf_tok.c conf_parse.c conf_parse.h y.tab.h

conf_tok.o: conf_parse.h

conf_parse.h:	y.tab.h
	@rm -f ${.TARGET}
	@cp -p ${.ALLSRC} ${.TARGET}

.include <bsd.prog.mk>
