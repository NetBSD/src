/* $NetBSD: ps_ind_level.c,v 1.9 2023/06/15 09:19:07 rillig Exp $ */

/*
 * The indentation of the very first line of a file determines the
 * indentation of the remaining code. This mode is meant for code snippets from
 * function bodies. At this level, function definitions are not recognized
 * properly.
 *
 * Even if later code has a smaller indentation, it is nevertheless indented to
 * the level given by the first line of code.
 *
 * In this particular test, the indentation is set to 5 and the tabulator
 * width is set to 8, to demonstrate an off-by-one error in
 * main_prepare_parsing that has been fixed in indent.c 1.107 from 2021-10-05.
 *
 * The declaration in the first line is indented by 3 tabs, amounting to 24
 * spaces. The initial indentation of the code is intended to be rounded down,
 * to 4 levels of indentation, amounting to 20 spaces.
 */
//indent input
			int indented_by_24;

void function_in_column_1(void){}

			#if indented
#endif
//indent end

/* 5 spaces indentation, 8 spaces per tabulator */
//indent run -i5 -ts8
		    int		    indented_by_24;

		    void	    function_in_column_1(void) {
		    }

#if indented
#endif
//indent end


/*
 * Labels are always indented 2 levels left of the code. The first line starts
 * at indentation level 3, the code in the function is therefore at level 4,
 * and the label is at level 2, sticking out of the code.
 */
//indent input
			int indent_by_24;

void function(void) {
label:;
}
//indent end

//indent run -i8 -ts8 -di0
			int indent_by_24;

			void function(void) {
		label:		;
			}
//indent end


/* Test the indentation computation in code_add_decl_indent. */
//indent input
int level_0;
{
int level_1;
{
int level_2;
{
int level_3;
{
int level_4;
}
}
}
}
//indent end

/*
 * The variables are indented by 16, 21, 26, 31, 36.
 * The variables end up in columns 17, 22, 27, 32, 37.
 */
//indent run -i5 -ts8 -di16 -ldi16
int		level_0;
{
     int	     level_1;
     {
	  int		  level_2;
	  {
	       int	       level_3;
	       {
		    int		    level_4;
	       }
	  }
     }
}
//indent end

/*
 * The variables are indented by 7, 12, 17, 22, 27.
 * The variables end up in columns 8, 13, 18, 23, 28.
 */
//indent run -i5 -ts8 -di7 -ldi7
int    level_0;
{
     int    level_1;
     {
	  int	 level_2;
	  {
	       int    level_3;
	       {
		    int	   level_4;
	       }
	  }
     }
}
//indent end


/*
 * Having function definitions indented to the right is not supported. In that
 * case, indent does not recognize it as a function definition, and it doesn't
 * indent the old-style parameter declarations one level further to the right.
 */
//indent input
			int		old_style(a)
			int		a;
			{
			}
//indent end

//indent run-equals-input
