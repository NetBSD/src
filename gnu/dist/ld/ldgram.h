typedef union {
  bfd_vma integer;
  char *name;
  int token;
  union etree_union *etree;
  struct phdr_info
    {
      boolean filehdr;
      boolean phdrs;
      union etree_union *at;
      union etree_union *flags;
    } phdr;
  struct lang_nocrossref *nocrossref;
  struct lang_output_section_phdr_list *section_phdr;
  struct bfd_elf_version_deps *deflist;
  struct bfd_elf_version_expr *versyms;
  struct bfd_elf_version_tree *versnode;
} YYSTYPE;
#define	INT	258
#define	NAME	259
#define	LNAME	260
#define	PLUSEQ	261
#define	MINUSEQ	262
#define	MULTEQ	263
#define	DIVEQ	264
#define	LSHIFTEQ	265
#define	RSHIFTEQ	266
#define	ANDEQ	267
#define	OREQ	268
#define	OROR	269
#define	ANDAND	270
#define	EQ	271
#define	NE	272
#define	LE	273
#define	GE	274
#define	LSHIFT	275
#define	RSHIFT	276
#define	UNARY	277
#define	END	278
#define	ALIGN_K	279
#define	BLOCK	280
#define	BIND	281
#define	QUAD	282
#define	LONG	283
#define	SHORT	284
#define	BYTE	285
#define	SECTIONS	286
#define	PHDRS	287
#define	SIZEOF_HEADERS	288
#define	OUTPUT_FORMAT	289
#define	FORCE_COMMON_ALLOCATION	290
#define	OUTPUT_ARCH	291
#define	INCLUDE	292
#define	MEMORY	293
#define	DEFSYMEND	294
#define	NOLOAD	295
#define	DSECT	296
#define	COPY	297
#define	INFO	298
#define	OVERLAY	299
#define	DEFINED	300
#define	TARGET_K	301
#define	SEARCH_DIR	302
#define	MAP	303
#define	ENTRY	304
#define	NEXT	305
#define	SIZEOF	306
#define	ADDR	307
#define	LOADADDR	308
#define	MAX	309
#define	MIN	310
#define	STARTUP	311
#define	HLL	312
#define	SYSLIB	313
#define	FLOAT	314
#define	NOFLOAT	315
#define	NOCROSSREFS	316
#define	ORIGIN	317
#define	FILL	318
#define	LENGTH	319
#define	CREATE_OBJECT_SYMBOLS	320
#define	INPUT	321
#define	GROUP	322
#define	OUTPUT	323
#define	CONSTRUCTORS	324
#define	ALIGNMOD	325
#define	AT	326
#define	PROVIDE	327
#define	CHIP	328
#define	LIST	329
#define	SECT	330
#define	ABSOLUTE	331
#define	LOAD	332
#define	NEWLINE	333
#define	ENDWORD	334
#define	ORDER	335
#define	NAMEWORD	336
#define	FORMAT	337
#define	PUBLIC	338
#define	BASE	339
#define	ALIAS	340
#define	TRUNCATE	341
#define	REL	342
#define	INPUT_SCRIPT	343
#define	INPUT_MRI_SCRIPT	344
#define	INPUT_DEFSYM	345
#define	CASE	346
#define	EXTERN	347
#define	START	348
#define	VERS_TAG	349
#define	VERS_IDENTIFIER	350
#define	GLOBAL	351
#define	LOCAL	352
#define	VERSION	353
#define	INPUT_VERSION_SCRIPT	354


extern YYSTYPE yylval;
