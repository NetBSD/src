#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union {
  bfd_vma integer;
  struct big_int
    {
      bfd_vma integer;
      char *str;
    } bigint;
  fill_type *fill;
  char *name;
  const char *cname;
  struct wildcard_spec wildcard;
  struct wildcard_list *wildcard_list;
  struct name_list *name_list;
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
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	INT	257
# define	NAME	258
# define	LNAME	259
# define	PLUSEQ	260
# define	MINUSEQ	261
# define	MULTEQ	262
# define	DIVEQ	263
# define	LSHIFTEQ	264
# define	RSHIFTEQ	265
# define	ANDEQ	266
# define	OREQ	267
# define	OROR	268
# define	ANDAND	269
# define	EQ	270
# define	NE	271
# define	LE	272
# define	GE	273
# define	LSHIFT	274
# define	RSHIFT	275
# define	UNARY	276
# define	END	277
# define	ALIGN_K	278
# define	BLOCK	279
# define	BIND	280
# define	QUAD	281
# define	SQUAD	282
# define	LONG	283
# define	SHORT	284
# define	BYTE	285
# define	SECTIONS	286
# define	PHDRS	287
# define	SORT	288
# define	DATA_SEGMENT_ALIGN	289
# define	DATA_SEGMENT_END	290
# define	SIZEOF_HEADERS	291
# define	OUTPUT_FORMAT	292
# define	FORCE_COMMON_ALLOCATION	293
# define	OUTPUT_ARCH	294
# define	INHIBIT_COMMON_ALLOCATION	295
# define	INCLUDE	296
# define	MEMORY	297
# define	DEFSYMEND	298
# define	NOLOAD	299
# define	DSECT	300
# define	COPY	301
# define	INFO	302
# define	OVERLAY	303
# define	DEFINED	304
# define	TARGET_K	305
# define	SEARCH_DIR	306
# define	MAP	307
# define	ENTRY	308
# define	NEXT	309
# define	SIZEOF	310
# define	ADDR	311
# define	LOADADDR	312
# define	MAX_K	313
# define	MIN_K	314
# define	STARTUP	315
# define	HLL	316
# define	SYSLIB	317
# define	FLOAT	318
# define	NOFLOAT	319
# define	NOCROSSREFS	320
# define	ORIGIN	321
# define	FILL	322
# define	LENGTH	323
# define	CREATE_OBJECT_SYMBOLS	324
# define	INPUT	325
# define	GROUP	326
# define	OUTPUT	327
# define	CONSTRUCTORS	328
# define	ALIGNMOD	329
# define	AT	330
# define	PROVIDE	331
# define	CHIP	332
# define	LIST	333
# define	SECT	334
# define	ABSOLUTE	335
# define	LOAD	336
# define	NEWLINE	337
# define	ENDWORD	338
# define	ORDER	339
# define	NAMEWORD	340
# define	ASSERT_K	341
# define	FORMAT	342
# define	PUBLIC	343
# define	BASE	344
# define	ALIAS	345
# define	TRUNCATE	346
# define	REL	347
# define	INPUT_SCRIPT	348
# define	INPUT_MRI_SCRIPT	349
# define	INPUT_DEFSYM	350
# define	CASE	351
# define	EXTERN	352
# define	START	353
# define	VERS_TAG	354
# define	VERS_IDENTIFIER	355
# define	GLOBAL	356
# define	LOCAL	357
# define	VERSIONK	358
# define	INPUT_VERSION_SCRIPT	359
# define	KEEP	360
# define	EXCLUDE_FILE	361


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
