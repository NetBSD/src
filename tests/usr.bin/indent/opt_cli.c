/* $NetBSD: opt_cli.c,v 1.7 2023/06/10 17:35:41 rillig Exp $ */

/*
 * Tests for the option '-cli' ("case label indentation"), which sets the
 * amount of indentation of a 'case' relative to the surrounding 'switch',
 * measured in indentation levels.
 *
 * See also:
 *	lsym_case_label.c
 */

//indent input
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
//indent end

//indent run -cli0.5
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
//indent end

//indent run -cli1.5
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
//indent end

//indent run -cli3.25
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
//indent end


/*
 * Test the combination of left-aligned braces and a deep case indentation.
 *
 * When the 'case' labels are that deeply indented, the distance between the
 * braces and the 'case' is between 1 and 2 indentation levels.
 */
//indent input
{
switch (expr)
{
case 1:
}
}
//indent end

//indent run -br -cli3.25
{
	switch (expr) {
				  case 1:
	}
}
//indent end

//indent run -bl -cli3.25
{
	switch (expr)
			{
				  case 1:
			}
}
//indent end

//indent run -bl -cli2.75
{
	switch (expr)
		{
			      case 1:
		}
}
//indent end

//indent run -bl -cli1.25
{
	switch (expr)
	{
		  case 1:
	}
}
//indent end
