#define EOL 257
#define PATH 258
#define STRING 259
typedef union {
  char *string;
  int  intval;
} YYSTYPE;
extern YYSTYPE yylval;
