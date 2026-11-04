#ifndef _yy_defines_h_
#define _yy_defines_h_

#define INT 257
#define NAME 258
#define LNAME 259
#define PLUSEQ 260
#define MINUSEQ 261
#define MULTEQ 262
#define DIVEQ 263
#define LSHIFTEQ 264
#define RSHIFTEQ 265
#define ANDEQ 266
#define OREQ 267
#define XOREQ 268
#define OROR 269
#define ANDAND 270
#define EQ 271
#define NE 272
#define LE 273
#define GE 274
#define LSHIFT 275
#define RSHIFT 276
#define UNARY 277
#define END 278
#define ALIGN_K 279
#define BLOCK 280
#define BIND 281
#define QUAD 282
#define SQUAD 283
#define LONG 284
#define SHORT 285
#define BYTE 286
#define ASCIZ 287
#define SECTIONS 288
#define PHDRS 289
#define INSERT_K 290
#define AFTER 291
#define BEFORE 292
#define LINKER_VERSION 293
#define DATA_SEGMENT_ALIGN 294
#define DATA_SEGMENT_RELRO_END 295
#define DATA_SEGMENT_END 296
#define SORT_BY_NAME 297
#define SORT_BY_ALIGNMENT 298
#define SORT_NONE 299
#define SORT_BY_INIT_PRIORITY 300
#define REVERSE 301
#define SIZEOF_HEADERS 302
#define OUTPUT_FORMAT 303
#define FORCE_COMMON_ALLOCATION 304
#define OUTPUT_ARCH 305
#define INHIBIT_COMMON_ALLOCATION 306
#define FORCE_GROUP_ALLOCATION 307
#define SEGMENT_START 308
#define INCLUDE 309
#define MEMORY 310
#define REGION_ALIAS 311
#define LD_FEATURE 312
#define NOLOAD 313
#define DSECT 314
#define COPY 315
#define INFO 316
#define OVERLAY 317
#define READONLY 318
#define TYPE 319
#define DEFINED 320
#define TARGET_K 321
#define SEARCH_DIR 322
#define MAP 323
#define ENTRY 324
#define NEXT 325
#define SIZEOF 326
#define ALIGNOF 327
#define ADDR 328
#define LOADADDR 329
#define MAX_K 330
#define MIN_K 331
#define STARTUP 332
#define HLL 333
#define SYSLIB 334
#define FLOAT 335
#define NOFLOAT 336
#define NOCROSSREFS 337
#define NOCROSSREFS_TO 338
#define ORIGIN 339
#define FILL 340
#define LENGTH 341
#define CREATE_OBJECT_SYMBOLS 342
#define INPUT 343
#define GROUP 344
#define OUTPUT 345
#define CONSTRUCTORS 346
#define ALIGNMOD 347
#define AT 348
#define SUBALIGN 349
#define HIDDEN 350
#define PROVIDE 351
#define PROVIDE_HIDDEN 352
#define AS_NEEDED 353
#define CHIP 354
#define LIST 355
#define SECT 356
#define ABSOLUTE 357
#define LOAD 358
#define NEWLINE 359
#define ENDWORD 360
#define ORDER 361
#define NAMEWORD 362
#define ASSERT_K 363
#define LOG2CEIL 364
#define FORMAT 365
#define PUBLIC 366
#define DEFSYMEND 367
#define BASE 368
#define ALIAS 369
#define TRUNCATE 370
#define REL 371
#define INPUT_SCRIPT 372
#define INPUT_MRI_SCRIPT 373
#define INPUT_DEFSYM 374
#define CASE 375
#define EXTERN 376
#define START 377
#define VERS_TAG 378
#define VERS_IDENTIFIER 379
#define GLOBAL 380
#define LOCAL 381
#define VERSIONK 382
#define INPUT_VERSION_SCRIPT 383
#define INPUT_SECTION_ORDERING_SCRIPT 384
#define KEEP 385
#define ONLY_IF_RO 386
#define ONLY_IF_RW 387
#define SPECIAL 388
#define INPUT_SECTION_FLAGS 389
#define ALIGN_WITH_INPUT 390
#define EXCLUDE_FILE 391
#define CONSTANT 392
#define INPUT_DYNAMIC_LIST 393
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union YYSTYPE {
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
  struct flag_info_list *flag_info_list;
  struct flag_info *flag_info;
  int token;
  union etree_union *etree;
  struct phdr_info
    {
      bool filehdr;
      bool phdrs;
      union etree_union *at;
      union etree_union *flags;
    } phdr;
  struct lang_nocrossref *nocrossref;
  struct lang_output_section_phdr_list *section_phdr;
  struct bfd_elf_version_deps *deflist;
  struct bfd_elf_version_expr *versyms;
  struct bfd_elf_version_tree *versnode;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE yylval;

#endif /* _yy_defines_h_ */
