typedef union
{
  char *string;
  long number;
  lex_pos_ty pos;
} po_gram_STYPE;
#define	COMMENT	258
#define	DOMAIN	259
#define	JUNK	260
#define	MSGID	261
#define	MSGSTR	262
#define	NAME	263
#define	NUMBER	264
#define	STRING	265


extern po_gram_STYPE po_gram_lval;
