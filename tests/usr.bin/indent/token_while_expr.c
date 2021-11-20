/* $NetBSD: token_while_expr.c,v 1.2 2021/11/20 16:54:17 rillig Exp $ */
/* $FreeBSD$ */

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
