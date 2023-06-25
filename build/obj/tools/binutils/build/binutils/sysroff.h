typedef struct { unsigned char *data; int len; } barray; 
typedef  int INT;
typedef  char * CHARS;



#define IT_cs_CODE 0x0
struct IT_cs;
extern void sysroff_swap_cs_in (struct IT_cs *);
extern void sysroff_swap_cs_out (FILE *, struct IT_cs *);
extern void sysroff_print_cs_out (struct IT_cs *);
struct IT_cs { 
	int size; 	/* size */
	int hd; 	/* hd */
	int hs; 	/* hs */
	int un; 	/* un */
	int us; 	/* us */
	int sc; 	/* sc */
	int ss; 	/* ss */
	int er; 	/* er */
	int ed; 	/* ed */
	int sh; 	/* sh */
	int ob; 	/* ob */
	int rl; 	/* rl */
	int du; 	/* du */
	int dps; 	/* dps */
	int dsy; 	/* dsy */
	int dty; 	/* dty */
	int dln; 	/* dln */
	int dso; 	/* dso */
	int dus; 	/* dus */
	int dss; 	/* dss */
	int dbt; 	/* dbt */
	int dpp; 	/* dpp */
	int dfp; 	/* dfp */
	int den; 	/* den */
	int dds; 	/* dds */
	int dar; 	/* dar */
	int dpt; 	/* dpt */
	int dul; 	/* dul */
	int dse; 	/* dse */
	int dot; 	/* dot */
};



#define IT_hd_CODE 0x4
struct IT_hd;
extern void sysroff_swap_hd_in (struct IT_hd *);
extern void sysroff_swap_hd_out (FILE *, struct IT_hd *);
extern void sysroff_print_hd_out (struct IT_hd *);
struct IT_hd { 
#define MTYPE_ABS_LM 0
#define MTYPE_REL_LM 1
#define MTYPE_OMS_OR_LMS 2
#define MTYPE_UNSPEC 0xf
	int mt; 	/* module type */
	int spare1; 	/* spare */
	char *cd;	 /* creation date */
	int nu; 	/* number of units */
	int code; 	/* code */
	char *ver;	 /* version */
	int au; 	/* address update */
	int si; 	/* segment identifier */
	int afl; 	/* address field length */
	int spare2; 	/* spare */
	int spcsz; 	/* space size within segment */
	int segsz; 	/* segment size */
	int segsh; 	/* segment shift */
	int ep; 	/* entry point */
	int uan; 	/* unit appearance number */
	int sa; 	/* section appearance number */
	int sad; 	/* segment address */
	int address; 	/* address */
	char *os;	 /* os name */
	char *sys;	 /* sys name */
	char *mn;	 /* module name */
	char *cpu;	 /* cpu */
};



#define IT_hs_CODE 0x5
struct IT_hs;
extern void sysroff_swap_hs_in (struct IT_hs *);
extern void sysroff_swap_hs_out (FILE *, struct IT_hs *);
extern void sysroff_print_hs_out (struct IT_hs *);
struct IT_hs { 
	int neg; 	/* neg number */
};



#define IT_un_CODE 0x6
struct IT_un;
extern void sysroff_swap_un_in (struct IT_un *);
extern void sysroff_swap_un_out (FILE *, struct IT_un *);
extern void sysroff_print_un_out (struct IT_un *);
struct IT_un { 
#define FORMAT_LM 0
#define FORMAT_OM 1
#define FORMAT_OMS_OR_LMS 2
	int format; 	/* format */
	int spare1; 	/* spare */
	int nsections; 	/* number of sections */
	int nextrefs; 	/* number of external refs */
	int nextdefs; 	/* number of external defs */
	char *name;	 /* unit name */
	char *tool;	 /* tool name */
	char *tcd;	 /* creation date */
	char *linker;	 /* linker name */
	char *lcd;	 /* creation date */
};



#define IT_us_CODE 0x7
struct IT_us;
extern void sysroff_swap_us_in (struct IT_us *);
extern void sysroff_swap_us_out (FILE *, struct IT_us *);
extern void sysroff_print_us_out (struct IT_us *);
struct IT_us { 
	int neg; 	/* negotiation number */
};



#define IT_sc_CODE 0x8
struct IT_sc;
extern void sysroff_swap_sc_in (struct IT_sc *);
extern void sysroff_swap_sc_out (FILE *, struct IT_sc *);
extern void sysroff_print_sc_out (struct IT_sc *);
struct IT_sc { 
	int format; 	/* format */
	int spare; 	/* spare */
	int segadd; 	/* segment address */
	int addr; 	/* address */
	int length; 	/* length */
	int align; 	/* alignment */
#define CONTENTS_CODE 0
#define CONTENTS_DATA 1
#define CONTENTS_STACK 2
#define CONTENTS_DUMMY 3
#define CONTENTS_SPECIAL 4
#define CONTENTS_NONSPEC 0xf
	int contents; 	/* contents */
#define CONCAT_SIMPLE 0
#define CONCAT_SHAREDC 1
#define CONCAT_DUMMY 2
#define CONCAT_GROUP 3
#define CONCAT_SHARED 4
#define CONCAT_PRIVATE 5
#define CONCAT_UNSPEC 0xf
	int concat; 	/* concat */
	int read; 	/* read */
	int write; 	/* write */
	int exec; 	/* exec */
	int init; 	/* initialized */
	int mode; 	/* mode */
	int spare1; 	/* spare */
	char *name;	 /* name */
};



#define IT_ss_CODE 0x9
struct IT_ss;
extern void sysroff_swap_ss_in (struct IT_ss *);
extern void sysroff_swap_ss_out (FILE *, struct IT_ss *);
extern void sysroff_print_ss_out (struct IT_ss *);
struct IT_ss { 
	int neg; 	/* neg number */
};



#define IT_er_CODE 0xc
struct IT_er;
extern void sysroff_swap_er_in (struct IT_er *);
extern void sysroff_swap_er_out (FILE *, struct IT_er *);
extern void sysroff_print_er_out (struct IT_er *);
struct IT_er { 
#define ER_ENTRY 0
#define ER_DATA 1
#define ER_NOTDEF 2
#define ER_NOTSPEC 3
	int type; 	/* symbol type */
	int spare; 	/* spare */
	char *name;	 /* symbol name */
};



#define IT_ed_CODE 0x14
struct IT_ed;
extern void sysroff_swap_ed_in (struct IT_ed *);
extern void sysroff_swap_ed_out (FILE *, struct IT_ed *);
extern void sysroff_print_ed_out (struct IT_ed *);
struct IT_ed { 
	int section; 	/* section appearance number */
#define ED_TYPE_ENTRY 0
#define ED_TYPE_DATA 1
#define ED_TYPE_CONST 2
#define ED_TYPE_NOTSPEC 7
	int type; 	/* symbol type */
	int spare; 	/* spare */
	int address; 	/* symbol address */
	int constant; 	/* constant value */
	char *name;	 /* symbol name */
};



#define IT_sh_CODE 0x1a
struct IT_sh;
extern void sysroff_swap_sh_in (struct IT_sh *);
extern void sysroff_swap_sh_out (FILE *, struct IT_sh *);
extern void sysroff_print_sh_out (struct IT_sh *);
struct IT_sh { 
	int unit; 	/* unit appearance number */
	int section; 	/* section appearance number */
};



#define IT_ob_CODE 0x1c
struct IT_ob;
extern void sysroff_swap_ob_in (struct IT_ob *);
extern void sysroff_swap_ob_out (FILE *, struct IT_ob *);
extern void sysroff_print_ob_out (struct IT_ob *);
struct IT_ob { 
	int saf; 	/* starting address flag */
	int cpf; 	/* compression flag */
	int spare; 	/* spare */
	int address; 	/* starting address */
	int compreps; 	/* comp reps */
	barray data;	 /* data */
};



#define IT_rl_CODE 0x20
struct IT_rl;
extern void sysroff_swap_rl_in (struct IT_rl *);
extern void sysroff_swap_rl_out (FILE *, struct IT_rl *);
extern void sysroff_print_rl_out (struct IT_rl *);
struct IT_rl { 
	int boundary; 	/* boundary of relocatable area */
	int apol; 	/* address polarity */
	int segment; 	/* segment number */
	int sign; 	/* sign of relocation */
	int check; 	/* check range */
	int addr; 	/* reloc address */
	int bitloc; 	/* bit loc */
	int flen; 	/* field length */
	int bcount; 	/* bcount */
#define OP_RELOC_ADDR 1
#define OP_SEC_REF 0
#define OP_EXT_REF 2
	int op; 	/* operator */
	int symn; 	/* symbol number */
	int secn; 	/* section number */
	int copcode_is_3; 	/* const opcode */
	int alength_is_4; 	/* addend length */
	int addend; 	/* addend */
	int aopcode_is_0x20; 	/* plus opcode */
	int dunno; 	/* dunno */
	int end; 	/* end */
};



#define IT_du_CODE 0x30
struct IT_du;
extern void sysroff_swap_du_in (struct IT_du *);
extern void sysroff_swap_du_out (FILE *, struct IT_du *);
extern void sysroff_print_du_out (struct IT_du *);
struct IT_du { 
	int format; 	/* format */
	int optimized; 	/* optimized */
	int stackfrmt; 	/* stackfrmt */
	int spare; 	/* spare */
	int unit; 	/* unit number */
	int sections; 	/* sections */
	/* repeat ptr->sections */
	int *san; 	/* section appearance number */
	/* repeat ptr->sections */
	int *address; 	/* address */
	/* repeat ptr->sections */
	int *length; 	/* section length */
	char *tool;	 /* tool name */
	char *date;	 /* creation date */
};



#define IT_dsy_CODE 0x34
struct IT_dsy;
extern void sysroff_swap_dsy_in (struct IT_dsy *);
extern void sysroff_swap_dsy_out (FILE *, struct IT_dsy *);
extern void sysroff_print_dsy_out (struct IT_dsy *);
struct IT_dsy { 
#define STYPE_VAR 0
#define STYPE_LAB 1
#define STYPE_PROC 2
#define STYPE_FUNC 3
#define STYPE_TYPE 4
#define STYPE_CONST 5
#define STYPE_ENTRY 6
#define STYPE_MEMBER 7
#define STYPE_ENUM 8
#define STYPE_TAG 9
#define STYPE_PACKAGE 10
#define STYPE_GENERIC 11
#define STYPE_TASK 12
#define STYPE_EXCEPTION 13
#define STYPE_PARAMETER 14
#define STYPE_EQUATE 15
#define STYPE_UNSPEC 0x7f
	int type; 	/* symbol type */
	int assign; 	/* assignment info */
	int snumber; 	/* symbol id */
	char *sname;	 /* symbol name */
	int nesting; 	/* nesting level */
#define AINFO_REG 1
#define AINFO_STATIC_EXT_DEF 2
#define AINFO_STATIC_EXT_REF 3
#define AINFO_STATIC_INT 4
#define AINFO_STATIC_COM 5
#define AINFO_AUTO 6
#define AINFO_CONST 7
#define AINFO_UNSPEC 0xff
	int ainfo; 	/* assignment type */
	int dlength; 	/* data length */
	int section; 	/* section number */
	int address; 	/* address */
	char *reg;	 /* register name */
	char *ename;	 /* external name */
	char *constant;	 /* constant */
	int bitunit; 	/* assignment unit */
	int spare2; 	/* spare */
	int field_len; 	/* field length */
	int field_off; 	/* field offset */
	int field_bitoff; 	/* bit offset */
	int evallen; 	/* value length */
	int evalue; 	/* value */
	char *cvalue;	 /* value */
	int qvallen; 	/* value length */
	int qvalue; 	/* value */
	int btype; 	/* basic type */
	int sizeinfo; 	/* size information */
	int sign; 	/* sign */
	int flt_type; 	/* floating point type */
	int sfn; 	/* source file number */
	int sln; 	/* source line number */
	int neg; 	/* negotiation number */
	int magic; 	/* magic */
};



#define IT_dul_CODE 0x52
struct IT_dul;
extern void sysroff_swap_dul_in (struct IT_dul *);
extern void sysroff_swap_dul_out (FILE *, struct IT_dul *);
extern void sysroff_print_dul_out (struct IT_dul *);
struct IT_dul { 
	int max_variable; 	/* max declaration type flag */
	int maxspare; 	/* max spare */
	int max; 	/* maximum */
	char *maxmode;	 /* max mode */
	int min_variable; 	/* min declaration type flag */
	int minspare; 	/* min spare */
	int min; 	/* minimum */
	char *minmode;	 /* min mode */
};



#define IT_dty_CODE 0x36
struct IT_dty;
extern void sysroff_swap_dty_in (struct IT_dty *);
extern void sysroff_swap_dty_out (FILE *, struct IT_dty *);
extern void sysroff_print_dty_out (struct IT_dty *);
struct IT_dty { 
	int end; 	/* end flag */
	int spare; 	/* spare */
	int neg; 	/* negotiation */
};



#define IT_dbt_CODE 0x44
struct IT_dbt;
extern void sysroff_swap_dbt_in (struct IT_dbt *);
extern void sysroff_swap_dbt_out (FILE *, struct IT_dbt *);
extern void sysroff_print_dbt_out (struct IT_dbt *);
struct IT_dbt { 
#define BTYPE_VOID 0
#define BTYPE_UNDEF 1
#define BTYPE_CHAR 2
#define BTYPE_INT 3
#define BTYPE_FLOAT 4
#define BTYPE_BIT 5
#define BTYPE_STRING 6
#define BTYPE_DECIMAL 7
#define BTYPE_ENUM 8
#define BTYPE_STRUCT 9
#define BTYPE_TYPE 10
#define BTYPE_TAG 11
#define BTYPE_UNSPEC 0xff
	int btype; 	/* basic type */
	int bitsize; 	/* size info */
#define SIGN_SIGNED 0
#define SIGN_UNSIGNED 1
#define SIGN_UNSPEC 3
	int sign; 	/* sign */
#define FPTYPE_SINGLE 0
#define FPTYPE_DOUBLE 1
#define FPTYPE_EXTENDED 2
#define FPTYPE_NOTSPEC 0x3f
	int fptype; 	/* floating point type */
	int sid; 	/* symbol id */
	int neg; 	/* negotiation */
};



#define IT_dar_CODE 0x4e
struct IT_dar;
extern void sysroff_swap_dar_in (struct IT_dar *);
extern void sysroff_swap_dar_out (FILE *, struct IT_dar *);
extern void sysroff_print_dar_out (struct IT_dar *);
struct IT_dar { 
	int length; 	/* element length */
	int dims; 	/* dims */
#define VARIABLE_FIXED 0
#define VARIABLE_VARIABLE 1
	/* repeat ptr->dims */
	int *variable; 	/* variable flag */
#define SUB_INTEGER 0
#define SUB_TYPE 1
	/* repeat ptr->dims */
	int *subtype; 	/* subscript type */
	/* repeat ptr->dims */
	int *spare; 	/* spare */
	/* repeat ptr->dims */
	int *sid; 	/* sub symbol id */
	/* repeat ptr->dims */
	int *max_variable; 	/* max declaration type flag */
	/* repeat ptr->dims */
	int *maxspare; 	/* max spare */
	/* repeat ptr->dims */
	int *max; 	/* maximum */
	/* repeat ptr->dims */
	int *min_variable; 	/* min declaration type flag */
	/* repeat ptr->dims */
	int *minspare; 	/* min spare */
	/* repeat ptr->dims */
	int *min; 	/* minimum */
	int neg; 	/* negotiation */
};



#define IT_dso_CODE 0x3a
struct IT_dso;
extern void sysroff_swap_dso_in (struct IT_dso *);
extern void sysroff_swap_dso_out (FILE *, struct IT_dso *);
extern void sysroff_print_dso_out (struct IT_dso *);
struct IT_dso { 
	int sid; 	/* function name */
	int spupdates; 	/* sp update count */
	/* repeat ptr->spupdates */
	int *address; 	/* update address */
	/* repeat ptr->spupdates */
	int *offset; 	/* offset */
};



#define IT_dln_CODE 0x38
struct IT_dln;
extern void sysroff_swap_dln_in (struct IT_dln *);
extern void sysroff_swap_dln_out (FILE *, struct IT_dln *);
extern void sysroff_print_dln_out (struct IT_dln *);
struct IT_dln { 
	int nln; 	/* number of lines */
	/* repeat ptr->nln */
	int *sfn; 	/* source file number */
	/* repeat ptr->nln */
	int *sln; 	/* source line number */
	/* repeat ptr->nln */
	int *section; 	/* section number */
	/* repeat ptr->nln */
	int *from_address; 	/* from address */
	/* repeat ptr->nln */
	int *to_address; 	/* to address */
	/* repeat ptr->nln */
	int *cc; 	/* call count */
	int neg; 	/* neg */
};



#define IT_dpp_CODE 0x46
struct IT_dpp;
extern void sysroff_swap_dpp_in (struct IT_dpp *);
extern void sysroff_swap_dpp_out (FILE *, struct IT_dpp *);
extern void sysroff_print_dpp_out (struct IT_dpp *);
struct IT_dpp { 
	int end; 	/* start/end */
	int spare; 	/* spare */
	int params; 	/* params */
	int neg; 	/* neg number */
};



#define IT_den_CODE 0x4a
struct IT_den;
extern void sysroff_swap_den_in (struct IT_den *);
extern void sysroff_swap_den_out (FILE *, struct IT_den *);
extern void sysroff_print_den_out (struct IT_den *);
struct IT_den { 
	int end; 	/* start/end */
	int spare; 	/* spare */
	int neg; 	/* neg number */
};



#define IT_dfp_CODE 0x48
struct IT_dfp;
extern void sysroff_swap_dfp_in (struct IT_dfp *);
extern void sysroff_swap_dfp_out (FILE *, struct IT_dfp *);
extern void sysroff_print_dfp_out (struct IT_dfp *);
struct IT_dfp { 
	int end; 	/* start/end flag */
	int spare; 	/* spare */
	int nparams; 	/* number of parameters */
	int neg; 	/* neg number */
};



#define IT_dds_CODE 0x4c
struct IT_dds;
extern void sysroff_swap_dds_in (struct IT_dds *);
extern void sysroff_swap_dds_out (FILE *, struct IT_dds *);
extern void sysroff_print_dds_out (struct IT_dds *);
struct IT_dds { 
	int end; 	/* start/end */
	int spare; 	/* spare */
	int neg; 	/* neg number */
};



#define IT_dpt_CODE 0x50
struct IT_dpt;
extern void sysroff_swap_dpt_in (struct IT_dpt *);
extern void sysroff_swap_dpt_out (FILE *, struct IT_dpt *);
extern void sysroff_print_dpt_out (struct IT_dpt *);
struct IT_dpt { 
	int neg; 	/* neg number */
	int dunno; 	/* dunno */
};



#define IT_dse_CODE 0x54
struct IT_dse;
extern void sysroff_swap_dse_in (struct IT_dse *);
extern void sysroff_swap_dse_out (FILE *, struct IT_dse *);
extern void sysroff_print_dse_out (struct IT_dse *);
struct IT_dse { 
	int neg; 	/* neg number */
	int dunno; 	/* dunno */
};



#define IT_dot_CODE 0x56
struct IT_dot;
extern void sysroff_swap_dot_in (struct IT_dot *);
extern void sysroff_swap_dot_out (FILE *, struct IT_dot *);
extern void sysroff_print_dot_out (struct IT_dot *);
struct IT_dot { 
	int unknown; 	/* unknown */
};



#define IT_dss_CODE 0x42
struct IT_dss;
extern void sysroff_swap_dss_in (struct IT_dss *);
extern void sysroff_swap_dss_out (FILE *, struct IT_dss *);
extern void sysroff_print_dss_out (struct IT_dss *);
struct IT_dss { 
	int type; 	/* type */
	int internal; 	/* external/internal */
	int spare; 	/* spare */
	char *package;	 /* package name */
	int id; 	/* symbol id */
	int record; 	/* record type */
	char *rules;	 /* rules */
	int nsymbols; 	/* number of symbols */
	int fixme; 	/* unknown */
};



#define IT_pss_CODE 0x40
struct IT_pss;
extern void sysroff_swap_pss_in (struct IT_pss *);
extern void sysroff_swap_pss_out (FILE *, struct IT_pss *);
extern void sysroff_print_pss_out (struct IT_pss *);
struct IT_pss { 
	int efn; 	/* negotiation number */
	int ns; 	/* number of source files */
	/* repeat ptr->ns */
	int *drb; 	/* directory reference bit */
	/* repeat ptr->ns */
	int *spare; 	/* spare */
	/* repeat ptr->ns */
	char **fname;	 /* completed file name */
	/* repeat ptr->ns */
	int *dan; 	/* directory apperance number */
	int ndir; 	/* number of directories */
	/* repeat ptr->ndir */
	char **dname;	 /* directory name */
};



#define IT_dus_CODE 0x40
struct IT_dus;
extern void sysroff_swap_dus_in (struct IT_dus *);
extern void sysroff_swap_dus_out (FILE *, struct IT_dus *);
extern void sysroff_print_dus_out (struct IT_dus *);
struct IT_dus { 
	int efn; 	/* negotiation number */
	int ns; 	/* number of source files */
	/* repeat ptr->ns */
	int *drb; 	/* directory reference bit */
	/* repeat ptr->ns */
	int *spare; 	/* spare */
	/* repeat ptr->ns */
	char **fname;	 /* completed file name */
	/* repeat ptr->ns */
	int *dan; 	/* directory apperance number */
	int ndir; 	/* number of directories */
	/* repeat ptr->ndir */
	char **dname;	 /* directory name */
};



#define IT_dps_CODE 0x32
struct IT_dps;
extern void sysroff_swap_dps_in (struct IT_dps *);
extern void sysroff_swap_dps_out (FILE *, struct IT_dps *);
extern void sysroff_print_dps_out (struct IT_dps *);
struct IT_dps { 
	int end; 	/* start/end flag */
#define BLOCK_TYPE_COMPUNIT 0
#define BLOCK_TYPE_PROCEDURE 2
#define BLOCK_TYPE_FUNCTION 3
#define BLOCK_TYPE_BLOCK 4
#define BLOCK_TYPE_BASIC 9
	int type; 	/* block type */
	int opt; 	/* optimization */
	int san; 	/* section number */
	int address; 	/* address */
	int block_size; 	/* block size */
	int nesting; 	/* nesting */
	int retaddr; 	/* return address */
	int intrflag; 	/* interrupt function flag */
	int stackflag; 	/* stack update flag */
	int intrpagejmp; 	/* intra page JMP */
	int spare; 	/* spare */
	int neg; 	/* neg number */
};
