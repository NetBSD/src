/* $NetBSD: token_while_expr.c,v 1.3 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the keyword 'while', followed by a parenthesized expression.
 */

#indent input
int main(int argc,char**argv){int o;while((o=getopt(argc,argv,"x:"))!=-1)
switch(o){case'x':do{o++;}while(o<5);break;default:usage();}return 0;}
#indent end

#indent run
int
main(int argc, char **argv)
{
	int		o;
	while ((o = getopt(argc, argv, "x:")) != -1)
		switch (o) {
		case 'x':
			do {
				o++;
			} while (o < 5);
			break;
		default:
			usage();
/* $ FIXME: The 'return' must be in a separate line. */
		} return 0;
}
#indent end
