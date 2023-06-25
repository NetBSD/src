#ifndef _yy_defines_h_
#define _yy_defines_h_

#define T_LBRACE 257
#define T_RBRACE 258
#define T_LBRACK 259
#define T_RBRACK 260
#define T_LPAREN 261
#define T_RPAREN 262
#define T_POINT 263
#define T_ARROW 264
#define T_COMPLEMENT 265
#define T_LOGNOT 266
#define T_INCDEC 267
#define T_SIZEOF 268
#define T_BUILTIN_OFFSETOF 269
#define T_TYPEOF 270
#define T_EXTENSION 271
#define T_ALIGNAS 272
#define T_ALIGNOF 273
#define T_ASTERISK 274
#define T_MULTIPLICATIVE 275
#define T_ADDITIVE 276
#define T_SHIFT 277
#define T_RELATIONAL 278
#define T_EQUALITY 279
#define T_AMPER 280
#define T_BITXOR 281
#define T_BITOR 282
#define T_LOGAND 283
#define T_LOGOR 284
#define T_QUEST 285
#define T_COLON 286
#define T_ASSIGN 287
#define T_OPASSIGN 288
#define T_COMMA 289
#define T_SEMI 290
#define T_ELLIPSIS 291
#define T_REAL 292
#define T_IMAG 293
#define T_GENERIC 294
#define T_NORETURN 295
#define T_SCLASS 296
#define T_TYPE 297
#define T_QUAL 298
#define T_ATOMIC 299
#define T_STRUCT_OR_UNION 300
#define T_ASM 301
#define T_BREAK 302
#define T_CASE 303
#define T_CONTINUE 304
#define T_DEFAULT 305
#define T_DO 306
#define T_ELSE 307
#define T_ENUM 308
#define T_FOR 309
#define T_GOTO 310
#define T_IF 311
#define T_PACKED 312
#define T_RETURN 313
#define T_SWITCH 314
#define T_SYMBOLRENAME 315
#define T_WHILE 316
#define T_STATIC_ASSERT 317
#define T_ATTRIBUTE 318
#define T_THEN 319
#define T_NAME 320
#define T_TYPENAME 321
#define T_CON 322
#define T_STRING 323
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union {
	val_t	*y_val;
	sbuf_t	*y_name;
	sym_t	*y_sym;
	op_t	y_op;
	scl_t	y_scl;
	tspec_t	y_tspec;
	tqual_t	y_tqual;
	type_t	*y_type;
	tnode_t	*y_tnode;
	range_t	y_range;
	strg_t	*y_string;
	qual_ptr *y_qual_ptr;
	bool	y_seen_statement;
	struct generic_association *y_generic;
	struct array_size y_array_size;
	bool	y_in_system_header;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE yylval;

#endif /* _yy_defines_h_ */
