#ifndef _yy_defines_h_
#define _yy_defines_h_

#define FIRSTTOKEN 257
#define PROGRAM 258
#define PASTAT 259
#define PASTAT2 260
#define XBEGIN 261
#define XEND 262
#define NL 263
#define ARRAY 264
#define MATCH 265
#define NOTMATCH 266
#define MATCHOP 267
#define FINAL 268
#define DOT 269
#define ALL 270
#define CCL 271
#define NCCL 272
#define CHAR 273
#define OR 274
#define STAR 275
#define QUEST 276
#define PLUS 277
#define EMPTYRE 278
#define ZERO 279
#define AND 280
#define BOR 281
#define APPEND 282
#define EQ 283
#define GE 284
#define GT 285
#define LE 286
#define LT 287
#define NE 288
#define IN 289
#define ARG 290
#define BLTIN 291
#define BREAK 292
#define CLOSE 293
#define CONTINUE 294
#define DELETE 295
#define DO 296
#define EXIT 297
#define FOR 298
#define FUNC 299
#define GENSUB 300
#define SUB 301
#define GSUB 302
#define IF 303
#define INDEX 304
#define LSUBSTR 305
#define MATCHFCN 306
#define NEXT 307
#define NEXTFILE 308
#define ADD 309
#define MINUS 310
#define MULT 311
#define DIVIDE 312
#define MOD 313
#define ASSIGN 314
#define ASGNOP 315
#define ADDEQ 316
#define SUBEQ 317
#define MULTEQ 318
#define DIVEQ 319
#define MODEQ 320
#define POWEQ 321
#define PRINT 322
#define PRINTF 323
#define SPRINTF 324
#define ELSE 325
#define INTEST 326
#define CONDEXPR 327
#define POSTINCR 328
#define PREINCR 329
#define POSTDECR 330
#define PREDECR 331
#define VAR 332
#define IVAR 333
#define VARNF 334
#define CALL 335
#define NUMBER 336
#define STRING 337
#define REGEXPR 338
#define GETLINE 339
#define RETURN 340
#define SPLIT 341
#define SUBSTR 342
#define WHILE 343
#define CAT 344
#define NOT 345
#define UMINUS 346
#define UPLUS 347
#define POWER 348
#define DECR 349
#define INCR 350
#define INDIRECT 351
#define LASTTOKEN 352
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union {
	Node	*p;
	Cell	*cp;
	int	i;
	char	*s;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE yylval;

#endif /* _yy_defines_h_ */
