#	$NetBSD: Makefile,v 1.16 1999/02/01 19:05:09 christos Exp $

PROG= amd
SRCS=	conf_parse.c conf_tok.c srvr_nfs.c srvr_amfs_auto.c sched.c rpc_fwd.c \
restart.c opts.c ops_unionfs.c ops_umapfs.c ops_ufs.c ops_tmpfs.c \
ops_tfs.c ops_pcfs.c ops_nullfs.c ops_nfs3.c ops_nfs.c ops_mfs.c \
ops_cdfs.c ops_autofs.c nfs_subr.c nfs_start.c nfs_prot_svc.c mntfs.c \
mapc.c map.c info_union.c info_passwd.c info_nis.c info_ndbm.c \
info_file.c info_hesiod.c get_args.c conf.c clock.c autil.c \
amq_svc.c amq_subr.c amfs_union.c amfs_toplvl.c amfs_root.c amfs_program.c \
amfs_nfsx.c amfs_nfsl.c amfs_linkx.c amfs_link.c amfs_inherit.c amfs_host.c \
amfs_error.c amfs_direct.c amfs_auto.c amd.c am_ops.c 
MAN+= amd.8
YHEADER=1
CLEANFILES+=conf_tok.c conf_parse.c conf_parse.h

conf_tok.o: conf_parse.h

.include <bsd.prog.mk>
