typedef union
{
  char *string;
  int number;
} po_hash_STYPE;
#define	STRING	258
#define	NUMBER	259
#define	COLON	260
#define	COMMA	261
#define	FILE_KEYWORD	262
#define	LINE_KEYWORD	263
#define	NUMBER_KEYWORD	264


extern po_hash_STYPE po_hash_lval;
