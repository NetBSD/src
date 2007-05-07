#	$NetBSD: Makefile,v 1.6.4.1 2007/05/07 17:05:27 pavel Exp $

MKPRIVATELIB=	yes

LIB=		ipf

SRCS=		addicmp.c addipopt.c alist_free.c alist_new.c bcopywrap.c \
		binprint.c buildopts.c checkrev.c count6bits.c count4bits.c \
		debug.c facpri.c flags.c fill6bits.c gethost.c \
		getifname.c getnattype.c getport.c getportproto.c \
		getproto.c getsumd.c hostname.c \
		icmpcode.c initparse.c ionames.c \
		ipoptsec.c ipf_dotuning.c ipft_ef.c ipft_hx.c ipft_pc.c \
		ipft_sn.c ipft_td.c ipft_tx.c kmem.c kmemcpywrap.c \
		kvatoname.c load_file.c load_hash.c load_hashnode.c \
		load_http.c load_pool.c load_poolnode.c load_url.c \
		mutex_emul.c nametokva.c nat_setgroupmap.c ntomask.c \
		optname.c optprint.c optprintv6.c optvalue.c portname.c \
		print_toif.c printactivenat.c printaps.c printbuf.c \
		printhash.c printhash_live.c printhashnode.c printip.c \
		printpool.c printpool_live.c \
		printpoolnode.c printfr.c printfraginfo.c printhashdata.c \
		printhostmap.c \
		printifname.c printhostmask.c printlog.c printmask.c \
		printnat.c printpooldata.c printportcmp.c printpacket.c \
		printpacket6.c printproto.c \
		printsbuf.c printstate.c printtunable.c \
		remove_hash.c remove_hashnode.c remove_pool.c \
		remove_poolnode.c resetlexer.c rwlock_emul.c tcpflags.c \
		tcp_flags.c var.c verbose.c v6ionames.c \
		v6optvalue.c

CPPFLAGS.printstate.c=	-Wno-error

.include <bsd.lib.mk>
