struct nwbootinfo {
	union {
		struct {
			unsigned long bp_pagesize;
			unsigned long bp_nrpages;
		} u1_bp;
		char filler1[256];
	} bi_u1;
#define bi_pagesize	bi_u1.u1_bp.bp_pagesize
#define bi_nrpages	bi_u1.u1_bp.bp_nrpages
	union {
		char paths[8][128];
		struct magic {
			unsigned long magic;
			char filler2[1024 - sizeof(unsigned long)];
		} u2_d;
	} bi_u2;
	char bi_cmdline[256];
	char bi_settings[2048];
};
