/*	$NetBSD: error.y,v 1.1.1.1 2009/10/29 00:46:53 christos Exp $	*/

%%
S: error
%%
main(){printf("yyparse() = %d\n",yyparse());}
yylex(){return-1;}
yyerror(s)char*s;{printf("%s\n",s);}
