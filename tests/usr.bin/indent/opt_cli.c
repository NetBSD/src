/* $NetBSD: opt_cli.c,v 1.1 2021/10/22 20:54:36 rillig Exp $ */
/* $FreeBSD$ */

#indent input
void
classify(int n)
{
	switch (n) {
	case 0: print("zero"); break;
	case 1: print("one"); break;
	case 2: case 3: print("prime"); break;
	case 4: print("square"); break;
	default: print("large"); break;
	}
}
#indent end

#indent run -cli1.5
void
classify(int n)
{
	switch (n) {
		    case 0:
			print("zero");
			break;
		    case 1:
			print("one");
			break;
		    case 2:
		    case 3:
			print("prime");
			break;
		    case 4:
			print("square");
			break;
		    default:
			print("large");
			break;
	}
}
#indent end
