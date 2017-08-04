#ifndef _yy_defines_h_
#define _yy_defines_h_

#define T_CODE_SET 257
#define T_MB_CUR_MAX 258
#define T_MB_CUR_MIN 259
#define T_COM_CHAR 260
#define T_ESC_CHAR 261
#define T_LT 262
#define T_GT 263
#define T_NL 264
#define T_SEMI 265
#define T_COMMA 266
#define T_ELLIPSIS 267
#define T_RPAREN 268
#define T_LPAREN 269
#define T_QUOTE 270
#define T_NULL 271
#define T_WS 272
#define T_END 273
#define T_COPY 274
#define T_CHARMAP 275
#define T_WIDTH 276
#define T_CTYPE 277
#define T_VARIABLE 278
#define T_ISUPPER 279
#define T_ISLOWER 280
#define T_ISALPHA 281
#define T_ISDIGIT 282
#define T_ISPUNCT 283
#define T_ISXDIGIT 284
#define T_ISSPACE 285
#define T_ISPRINT 286
#define T_ISGRAPH 287
#define T_ISBLANK 288
#define T_ISCNTRL 289
#define T_ISALNUM 290
#define T_ISSPECIAL 291
#define T_ISPHONOGRAM 292
#define T_ISIDEOGRAM 293
#define T_ISENGLISH 294
#define T_ISNUMBER 295
#define T_TOUPPER 296
#define T_TOLOWER 297
#define T_COLLATE 298
#define T_COLLATING_SYMBOL 299
#define T_COLLATING_ELEMENT 300
#define T_ORDER_START 301
#define T_ORDER_END 302
#define T_FORWARD 303
#define T_BACKWARD 304
#define T_POSITION 305
#define T_FROM 306
#define T_UNDEFINED 307
#define T_IGNORE 308
#define T_MESSAGES 309
#define T_YESSTR 310
#define T_NOSTR 311
#define T_YESEXPR 312
#define T_NOEXPR 313
#define T_MONETARY 314
#define T_INT_CURR_SYMBOL 315
#define T_CURRENCY_SYMBOL 316
#define T_MON_DECIMAL_POINT 317
#define T_MON_THOUSANDS_SEP 318
#define T_POSITIVE_SIGN 319
#define T_NEGATIVE_SIGN 320
#define T_MON_GROUPING 321
#define T_INT_FRAC_DIGITS 322
#define T_FRAC_DIGITS 323
#define T_P_CS_PRECEDES 324
#define T_P_SEP_BY_SPACE 325
#define T_N_CS_PRECEDES 326
#define T_N_SEP_BY_SPACE 327
#define T_P_SIGN_POSN 328
#define T_N_SIGN_POSN 329
#define T_INT_P_CS_PRECEDES 330
#define T_INT_N_CS_PRECEDES 331
#define T_INT_P_SEP_BY_SPACE 332
#define T_INT_N_SEP_BY_SPACE 333
#define T_INT_P_SIGN_POSN 334
#define T_INT_N_SIGN_POSN 335
#define T_NUMERIC 336
#define T_DECIMAL_POINT 337
#define T_THOUSANDS_SEP 338
#define T_GROUPING 339
#define T_TIME 340
#define T_ABDAY 341
#define T_DAY 342
#define T_ABMON 343
#define T_MON 344
#define T_ERA 345
#define T_ERA_D_FMT 346
#define T_ERA_T_FMT 347
#define T_ERA_D_T_FMT 348
#define T_ALT_DIGITS 349
#define T_D_T_FMT 350
#define T_D_FMT 351
#define T_T_FMT 352
#define T_AM_PM 353
#define T_T_FMT_AMPM 354
#define T_DATE_FMT 355
#define T_CHAR 356
#define T_NAME 357
#define T_NUMBER 358
#define T_SYMBOL 359
#define T_COLLSYM 360
#define T_COLLELEM 361
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union {
	int		num;
	wchar_t		wc;
	char		*token;
	collsym_t	*collsym;
	collelem_t	*collelem;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE yylval;

#endif /* _yy_defines_h_ */
