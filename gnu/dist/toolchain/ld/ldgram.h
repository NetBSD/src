#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union {
  bfd_vma integer;
  char *name;
  const char *cname;
  struct wildcard_spec wildcard;
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
# define	SIZEOF_HEADERS	289
# define	OUTPUT_FORMAT	290
# define	FORCE_COMMON_ALLOCATION	291
# define	OUTPUT_ARCH	292
# define	INCLUDE	293
# define	MEMORY	294
# define	DEFSYMEND	295
# define	NOLOAD	296
# define	DSECT	297
# define	COPY	298
# define	INFO	299
# define	OVERLAY	300
# define	DEFINED	301
# define	TARGET_K	302
# define	SEARCH_DIR	303
# define	MAP	304
# define	ENTRY	305
# define	NEXT	306
# define	SIZEOF	307
# define	SIZEOF_UNADJ	308
# define	ADDR	309
# define	LOADADDR	310
# define	MAX_K	311
# define	MIN_K	312
# define	STARTUP	313
# define	HLL	314
# define	SYSLIB	315
# define	FLOAT	316
# define	NOFLOAT	317
# define	NOCROSSREFS	318
# define	ORIGIN	319
# define	FILL	320
# define	LENGTH	321
# define	CREATE_OBJECT_SYMBOLS	322
# define	INPUT	323
# define	GROUP	324
# define	OUTPUT	325
# define	CONSTRUCTORS	326
# define	ALIGNMOD	327
# define	AT	328
# define	PROVIDE	329
# define	CHIP	330
# define	LIST	331
# define	SECT	332
# define	ABSOLUTE	333
# define	LOAD	334
# define	NEWLINE	335
# define	ENDWORD	336
# define	ORDER	337
# define	NAMEWORD	338
# define	ASSERT_K	339
# define	FORMAT	340
# define	PUBLIC	341
# define	BASE	342
# define	ALIAS	343
# define	TRUNCATE	344
# define	REL	345
# define	INPUT_SCRIPT	346
# define	INPUT_MRI_SCRIPT	347
# define	INPUT_DEFSYM	348
# define	CASE	349
# define	EXTERN	350
# define	START	351
# define	VERS_TAG	352
# define	VERS_IDENTIFIER	353
# define	GLOBAL	354
# define	LOCAL	355
# define	VERSIONK	356
# define	INPUT_VERSION_SCRIPT	357
# define	KEEP	358
# define	EXCLUDE_FILE	359


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
