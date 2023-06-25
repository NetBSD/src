#ifndef _yy_defines_h_
#define _yy_defines_h_

#define AND 257
#define AT 258
#define ATTACH 259
#define BLOCK 260
#define BUILD 261
#define CHAR 262
#define COLONEQ 263
#define COMPILE_WITH 264
#define CONFIG 265
#define DEFFS 266
#define DEFINE 267
#define DEFOPT 268
#define DEFPARAM 269
#define DEFFLAG 270
#define DEFPSEUDO 271
#define DEFPSEUDODEV 272
#define DEVICE 273
#define DEVCLASS 274
#define DUMPS 275
#define DEVICE_MAJOR 276
#define ENDFILE 277
#define XFILE 278
#define FILE_SYSTEM 279
#define FLAGS 280
#define IDENT 281
#define IOCONF 282
#define LINKZERO 283
#define XMACHINE 284
#define MAJOR 285
#define MAKEOPTIONS 286
#define MAXUSERS 287
#define MAXPARTITIONS 288
#define MINOR 289
#define NEEDS_COUNT 290
#define NEEDS_FLAG 291
#define NO 292
#define CNO 293
#define XOBJECT 294
#define OBSOLETE 295
#define ON 296
#define OPTIONS 297
#define PACKAGE 298
#define PLUSEQ 299
#define PREFIX 300
#define BUILDPREFIX 301
#define PSEUDO_DEVICE 302
#define PSEUDO_ROOT 303
#define ROOT 304
#define SELECT 305
#define SINGLE 306
#define SOURCE 307
#define TYPE 308
#define VECTOR 309
#define VERSION 310
#define WITH 311
#define NUMBER 312
#define PATHNAME 313
#define QSTRING 314
#define WORD 315
#define EMPTYSTRING 316
#define ENDDEFS 317
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union {
	struct	attr *attr;
	struct	devbase *devb;
	struct	deva *deva;
	struct	nvlist *list;
	struct defoptlist *defoptlist;
	struct loclist *loclist;
	struct attrlist *attrlist;
	struct condexpr *condexpr;
	const char *str;
	struct	numconst num;
	int64_t	val;
	u_char	flag;
	devmajor_t devmajor;
	int32_t i32;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE yylval;

#endif /* _yy_defines_h_ */
