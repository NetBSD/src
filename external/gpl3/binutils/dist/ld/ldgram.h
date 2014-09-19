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
#define OROR 268
#define ANDAND 269
#define EQ 270
#define NE 271
#define LE 272
#define GE 273
#define LSHIFT 274
#define RSHIFT 275
#define UNARY 276
#define END 277
#define ALIGN_K 278
#define BLOCK 279
#define BIND 280
#define QUAD 281
#define SQUAD 282
#define LONG 283
#define SHORT 284
#define BYTE 285
#define SECTIONS 286
#define PHDRS 287
#define INSERT_K 288
#define AFTER 289
#define BEFORE 290
#define DATA_SEGMENT_ALIGN 291
#define DATA_SEGMENT_RELRO_END 292
#define DATA_SEGMENT_END 293
#define SORT_BY_NAME 294
#define SORT_BY_ALIGNMENT 295
#define SORT_NONE 296
#define SORT_BY_INIT_PRIORITY 297
#define SIZEOF_HEADERS 298
#define OUTPUT_FORMAT 299
#define FORCE_COMMON_ALLOCATION 300
#define OUTPUT_ARCH 301
#define INHIBIT_COMMON_ALLOCATION 302
#define SEGMENT_START 303
#define INCLUDE 304
#define MEMORY 305
#define REGION_ALIAS 306
#define LD_FEATURE 307
#define NOLOAD 308
#define DSECT 309
#define COPY 310
#define INFO 311
#define OVERLAY 312
#define DEFINED 313
#define TARGET_K 314
#define SEARCH_DIR 315
#define MAP 316
#define ENTRY 317
#define NEXT 318
#define SIZEOF 319
#define ALIGNOF 320
#define ADDR 321
#define LOADADDR 322
#define MAX_K 323
#define MIN_K 324
#define STARTUP 325
#define HLL 326
#define SYSLIB 327
#define FLOAT 328
#define NOFLOAT 329
#define NOCROSSREFS 330
#define ORIGIN 331
#define FILL 332
#define LENGTH 333
#define CREATE_OBJECT_SYMBOLS 334
#define INPUT 335
#define GROUP 336
#define OUTPUT 337
#define CONSTRUCTORS 338
#define ALIGNMOD 339
#define AT 340
#define SUBALIGN 341
#define HIDDEN 342
#define PROVIDE 343
#define PROVIDE_HIDDEN 344
#define AS_NEEDED 345
#define CHIP 346
#define LIST 347
#define SECT 348
#define ABSOLUTE 349
#define LOAD 350
#define NEWLINE 351
#define ENDWORD 352
#define ORDER 353
#define NAMEWORD 354
#define ASSERT_K 355
#define FORMAT 356
#define PUBLIC 357
#define DEFSYMEND 358
#define BASE 359
#define ALIAS 360
#define TRUNCATE 361
#define REL 362
#define INPUT_SCRIPT 363
#define INPUT_MRI_SCRIPT 364
#define INPUT_DEFSYM 365
#define CASE 366
#define EXTERN 367
#define START 368
#define VERS_TAG 369
#define VERS_IDENTIFIER 370
#define GLOBAL 371
#define LOCAL 372
#define VERSIONK 373
#define INPUT_VERSION_SCRIPT 374
#define KEEP 375
#define ONLY_IF_RO 376
#define ONLY_IF_RW 377
#define SPECIAL 378
#define INPUT_SECTION_FLAGS 379
#define EXCLUDE_FILE 380
#define CONSTANT 381
#define INPUT_DYNAMIC_LIST 382
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
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
  struct flag_info_list *flag_info_list;
  struct flag_info *flag_info;
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
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE yylval;
