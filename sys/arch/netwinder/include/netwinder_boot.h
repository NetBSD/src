/*	$NetBSD: netwinder_boot.h,v 1.2 2002/04/03 05:37:00 thorpej Exp $	*/

struct nwbootinfo {
	union {
		struct {
			unsigned long bp_pagesize;
			unsigned long bp_nrpages;
			unsigned long bp_ramdisk_size;	/* not used */
			unsigned long bp_flags;		/* not used */
			unsigned long bp_rootdev;
		} u1_bp;
		char filler1[256];
	} bi_u1;
#define bi_pagesize	bi_u1.u1_bp.bp_pagesize
#define bi_nrpages	bi_u1.u1_bp.bp_nrpages
#define	bi_rootdev	bi_u1.u1_bp.bp_rootdev
	union {
		char paths[8][128];
		struct magic {
			unsigned long magic;
			char filler2[1024 - sizeof(unsigned long)];
		} u2_d;
	} bi_u2;
	char bi_cmdline[1024];
};
