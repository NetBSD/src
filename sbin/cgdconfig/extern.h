extern char *yytext;
extern int yylineno;
extern FILE *yyin;

int yylex(void);
void yyerror(const char *);

struct params *cgdparsefile(FILE *);
