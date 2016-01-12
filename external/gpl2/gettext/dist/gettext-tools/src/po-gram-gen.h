#ifndef BISON_PO_GRAM_GEN_H
# define BISON_PO_GRAM_GEN_H

#ifndef YYSTYPE
typedef union
{
  struct { char *string; lex_pos_ty pos; bool obsolete; } string;
  struct { string_list_ty stringlist; lex_pos_ty pos; bool obsolete; } stringlist;
  struct { long number; lex_pos_ty pos; bool obsolete; } number;
  struct { lex_pos_ty pos; bool obsolete; } pos;
  struct { char *ctxt; char *id; char *id_plural; lex_pos_ty pos; bool obsolete; } prev;
  struct { char *prev_ctxt; char *prev_id; char *prev_id_plural; char *ctxt; lex_pos_ty pos; bool obsolete; } message_intro;
  struct { struct msgstr_def rhs; lex_pos_ty pos; bool obsolete; } rhs;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	COMMENT	257
# define	DOMAIN	258
# define	JUNK	259
# define	PREV_MSGCTXT	260
# define	PREV_MSGID	261
# define	PREV_MSGID_PLURAL	262
# define	PREV_STRING	263
# define	MSGCTXT	264
# define	MSGID	265
# define	MSGID_PLURAL	266
# define	MSGSTR	267
# define	NAME	268
# define	NUMBER	269
# define	STRING	270


extern YYSTYPE yylval;

#endif /* not BISON_PO_GRAM_GEN_H */
