#ifndef BISON_GENGTYPE_YACC_H
# define BISON_GENGTYPE_YACC_H

#ifndef YYSTYPE
typedef union {
  type_p t;
  pair_p p;
  options_p o;
  const char *s;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	ENT_TYPEDEF_STRUCT	257
# define	ENT_STRUCT	258
# define	ENT_EXTERNSTATIC	259
# define	ENT_YACCUNION	260
# define	GTY_TOKEN	261
# define	UNION	262
# define	STRUCT	263
# define	ENUM	264
# define	ALIAS	265
# define	PARAM_IS	266
# define	NUM	267
# define	PERCENTPERCENT	268
# define	SCALAR	269
# define	ID	270
# define	STRING	271
# define	ARRAY	272
# define	PERCENT_ID	273
# define	CHAR	274


extern YYSTYPE yylval;

#endif /* not BISON_GENGTYPE_YACC_H */
