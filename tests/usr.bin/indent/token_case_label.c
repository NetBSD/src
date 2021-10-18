/* $NetBSD: token_case_label.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for formatting of case labels in switch statements.
 */

#indent input
void function(void){switch(expr){case 1:;case 2:break;case 3:switch(
inner){case 4:break;}}}
#indent end

#indent run
void
function(void)
{
	switch (expr) {
	case 1:	;
	case 2:
		break;
	case 3:
		switch (
			inner) {
		case 4:
			break;
		}
	}
}
#indent end

#indent run -cli0.5
void
function(void)
{
	switch (expr) {
	    case 1:;
	    case 2:
		break;
	    case 3:
		switch (
			inner) {
		    case 4:
			break;
		}
	}
}
#indent end
