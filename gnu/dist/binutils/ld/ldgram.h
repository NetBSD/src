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
      bfd_boolean filehdr;
      bfd_boolean phdrs;
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
# define	DATA_SEGMENT_ALIGN	288
# define	DATA_SEGMENT_RELRO_END	289
# define	DATA_SEGMENT_END	290
# define	SORT_BY_NAME	291
# define	SORT_BY_ALIGNMENT	292
# define	SIZEOF_HEADERS	293
# define	OUTPUT_FORMAT	294
# define	FORCE_COMMON_ALLOCATION	295
# define	OUTPUT_ARCH	296
# define	INHIBIT_COMMON_ALLOCATION	297
# define	SEGMENT_START	298
# define	INCLUDE	299
# define	MEMORY	300
# define	DEFSYMEND	301
# define	NOLOAD	302
# define	DSECT	303
# define	COPY	304
# define	INFO	305
# define	OVERLAY	306
# define	DEFINED	307
# define	TARGET_K	308
# define	SEARCH_DIR	309
# define	MAP	310
# define	ENTRY	311
# define	NEXT	312
# define	SIZEOF	313
# define	ADDR	314
# define	LOADADDR	315
# define	MAX_K	316
# define	MIN_K	317
# define	STARTUP	318
# define	HLL	319
# define	SYSLIB	320
# define	FLOAT	321
# define	NOFLOAT	322
# define	NOCROSSREFS	323
# define	ORIGIN	324
# define	FILL	325
# define	LENGTH	326
# define	CREATE_OBJECT_SYMBOLS	327
# define	INPUT	328
# define	GROUP	329
# define	OUTPUT	330
# define	CONSTRUCTORS	331
# define	ALIGNMOD	332
# define	AT	333
# define	SUBALIGN	334
# define	PROVIDE	335
# define	AS_NEEDED	336
# define	CHIP	337
# define	LIST	338
# define	SECT	339
# define	ABSOLUTE	340
# define	LOAD	341
# define	NEWLINE	342
# define	ENDWORD	343
# define	ORDER	344
# define	NAMEWORD	345
# define	ASSERT_K	346
# define	FORMAT	347
# define	PUBLIC	348
# define	BASE	349
# define	ALIAS	350
# define	TRUNCATE	351
# define	REL	352
# define	INPUT_SCRIPT	353
# define	INPUT_MRI_SCRIPT	354
# define	INPUT_DEFSYM	355
# define	CASE	356
# define	EXTERN	357
# define	START	358
# define	VERS_TAG	359
# define	VERS_IDENTIFIER	360
# define	GLOBAL	361
# define	LOCAL	362
# define	VERSIONK	363
# define	INPUT_VERSION_SCRIPT	364
# define	KEEP	365
# define	ONLY_IF_RO	366
# define	ONLY_IF_RW	367
# define	EXCLUDE_FILE	368


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
