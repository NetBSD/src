#ifndef BISON_P608_H
# define BISON_P608_H

#ifndef YYSTYPE
typedef union { GTY(())
  long itype;
  tree ttype;
  char *strtype;
  enum tree_code code;
  flagged_type_tree ftype;
  struct unparsed_text *pi;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	IDENTIFIER	257
# define	tTYPENAME	258
# define	SELFNAME	259
# define	PFUNCNAME	260
# define	SCSPEC	261
# define	TYPESPEC	262
# define	CV_QUALIFIER	263
# define	CONSTANT	264
# define	VAR_FUNC_NAME	265
# define	STRING	266
# define	ELLIPSIS	267
# define	SIZEOF	268
# define	ENUM	269
# define	IF	270
# define	ELSE	271
# define	WHILE	272
# define	DO	273
# define	FOR	274
# define	SWITCH	275
# define	CASE	276
# define	DEFAULT	277
# define	BREAK	278
# define	CONTINUE	279
# define	RETURN_KEYWORD	280
# define	GOTO	281
# define	ASM_KEYWORD	282
# define	TYPEOF	283
# define	ALIGNOF	284
# define	SIGOF	285
# define	ATTRIBUTE	286
# define	EXTENSION	287
# define	LABEL	288
# define	REALPART	289
# define	IMAGPART	290
# define	VA_ARG	291
# define	AGGR	292
# define	VISSPEC	293
# define	DELETE	294
# define	NEW	295
# define	THIS	296
# define	OPERATOR	297
# define	CXX_TRUE	298
# define	CXX_FALSE	299
# define	NAMESPACE	300
# define	TYPENAME_KEYWORD	301
# define	USING	302
# define	LEFT_RIGHT	303
# define	TEMPLATE	304
# define	TYPEID	305
# define	DYNAMIC_CAST	306
# define	STATIC_CAST	307
# define	REINTERPRET_CAST	308
# define	CONST_CAST	309
# define	SCOPE	310
# define	EXPORT	311
# define	EMPTY	312
# define	PTYPENAME	313
# define	NSNAME	314
# define	THROW	315
# define	ASSIGN	316
# define	OROR	317
# define	ANDAND	318
# define	MIN_MAX	319
# define	EQCOMPARE	320
# define	ARITHCOMPARE	321
# define	LSHIFT	322
# define	RSHIFT	323
# define	POINTSAT_STAR	324
# define	DOT_STAR	325
# define	UNARY	326
# define	PLUSPLUS	327
# define	MINUSMINUS	328
# define	HYPERUNARY	329
# define	POINTSAT	330
# define	TRY	331
# define	CATCH	332
# define	EXTERN_LANG_STRING	333
# define	ALL	334
# define	PRE_PARSED_CLASS_DECL	335
# define	DEFARG	336
# define	DEFARG_MARKER	337
# define	PRE_PARSED_FUNCTION_DECL	338
# define	TYPENAME_DEFN	339
# define	IDENTIFIER_DEFN	340
# define	PTYPENAME_DEFN	341
# define	END_OF_LINE	342
# define	END_OF_SAVED_INPUT	343


extern YYSTYPE yylval;

#endif /* not BISON_P608_H */
#define YYEMPTY		-2
