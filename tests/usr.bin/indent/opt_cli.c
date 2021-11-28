/* $NetBSD: opt_cli.c,v 1.3 2021/11/28 16:05:59 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the option '-cli' ("case label indentation"), which sets the
 * amount of indentation of a 'case' relative to the surrounding 'switch',
 * measured in indentation levels.
 *
 * See also:
 *	lsym_case_label.c
 */

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

#indent run -cli0.5
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
