/* $NetBSD: ps_ind_level.c,v 1.1 2021/10/18 22:46:33 rillig Exp $ */
/* $FreeBSD$ */

/*
 * The indentation of the very first line of a file determines the
 * indentation of the remaining code. Even if later code has a smaller
 * indentation, it is nevertheless indented to the level given by the first
 * line of code.
 *
 * In this particular test, the indentation is set to 5 and the tabulator
 * width is set to 8, to demonstrate an off-by-one error in
 * main_prepare_parsing that has been fixed in indent.c 1.107 from 2021-10-05.
 *
 * The declaration in the first line is indented by 3 tabs, amounting to 24
 * spaces. The initial indentation of the code is intended to be rounded down,
 * to 4 levels of indentation, amounting to 20 spaces.
 */
#indent input
			int declaration_in_column_25;

void function_in_column_1(void){}
#indent end

/* 5 spaces indentation, 8 spaces per tabulator */
#indent run -i5 -ts8
		    int		    declaration_in_column_25;

		    void	    function_in_column_1(void){
		    }
#indent end
/*
 * In the above function declaration, the space between '){' is missing. This
 * is because the tokenizer only recognizes function definitions if they start
 * at indentation level 0, but this declaration starts at indentation level 4,
 * due to the indentation in line 1. It's an edge case and probably not worth
 * fixing.
 *
 * See 'in_parameter_declaration = true'.
 */


/*
 * Labels are always indented 2 levels left of the code. The first line starts
 * at indentation level 3, the code in the function is therefore at level 4,
 * and the label is at level 2, sticking out of the code.
 */
#indent input
			int indent_by_24;

void function(void) {
label:;
}
#indent end

#indent run -i8 -ts8 -di0
			int indent_by_24;

			void function(void){
		label:		;
			}
#indent end
