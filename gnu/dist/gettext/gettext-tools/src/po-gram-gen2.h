#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union
{
  struct { char *string; lex_pos_ty pos; bool obsolete; } string;
  struct { string_list_ty stringlist; lex_pos_ty pos; bool obsolete; } stringlist;
  struct { long number; lex_pos_ty pos; bool obsolete; } number;
  struct { lex_pos_ty pos; bool obsolete; } pos;
  struct { struct msgstr_def rhs; lex_pos_ty pos; bool obsolete; } rhs;
} po_gram_stype;
# define YYSTYPE po_gram_stype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	COMMENT	257
# define	DOMAIN	258
# define	JUNK	259
# define	MSGID	260
# define	MSGID_PLURAL	261
# define	MSGSTR	262
# define	NAME	263
# define	NUMBER	264
# define	STRING	265


extern DLL_VARIABLE YYSTYPE po_gram_lval;

#endif /* not BISON_Y_TAB_H */
