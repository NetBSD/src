/* $NetBSD: indent_off_on.c,v 1.15 2023/06/04 22:20:04 rillig Exp $ */

/*
 * Tests for the comments 'INDENT OFF' and 'INDENT ON', which temporarily
 * disable formatting, copying the input directly to the output.  Internally,
 * indent still keeps track of the number of braces and other indentation.
 */

//indent input
{}

/*INDENT OFF*/
/*INDENT ON*/

{}
//indent end

//indent run
{
}

/*INDENT OFF*/
/*INDENT ON*/

{
}
//indent end


//indent input
{}


/*INDENT OFF*/
/*INDENT ON*/
//indent end

//indent run
{
}


/*INDENT OFF*/
/*INDENT ON*/
//indent end


//indent input
{}
 /* INDENT OFF */
 /* INDENT ON */
{}
//indent end

//indent run
{
}
 /* INDENT OFF */
 /* INDENT ON */
{
}
//indent end


//indent input
{}
	/* INDENT OFF */
	/* INDENT ON */
{}
//indent end

//indent run
{
}
	/* INDENT OFF */
	/* INDENT ON */
{
}
//indent end


/*
 * The INDENT comments can be written without space between the words, but
 * nobody does this.
 */
//indent input
int   decl   ;
/*INDENTOFF*/
int   decl   ;
/*INDENTON*/
int   decl   ;
//indent end

//indent run -di0
int decl;
/*INDENTOFF*/
int   decl   ;
/*INDENTON*/
int decl;
//indent end


/*
 * Any whitespace around the 'INDENT ON/OFF' is ignored, as is any whitespace
 * between the two words.
 */
//indent input
int   decl   ;
/*		INDENT		OFF		*/
int   decl   ;
/*		INDENT		ON		*/
int   decl   ;
//indent end

//indent run -di0
int decl;
/*		INDENT		OFF		*/
int   decl   ;
/*		INDENT		ON		*/
int decl;
//indent end


//indent input
/*INDENT OFF*/
/* No formatting takes place here. */
int format( void ) {{{
/*INDENT ON*/
}}}
//indent end

//indent run
/*INDENT OFF*/
/* No formatting takes place here. */
int format( void ) {{{
/*INDENT ON*/
		}
	}
}
//indent end


//indent input
/* INDENT OFF */
void indent_off ( void ) ;
/*  INDENT */
void indent_on ( void ) ;
/* INDENT OFF */
void indent_off ( void ) ;
	/* INDENT ON */
void indent_on ( void ) ;	/* the comment may be indented */
/* INDENT		OFF					*/
void indent_off ( void ) ;
/* INDENTATION ON */
void indent_still_off ( void ) ;	/* due to the word 'INDENTATION' */
/* INDENT ON * */
void indent_still_off ( void ) ;	/* due to the extra '*' at the end */
/* INDENT ON */
void indent_on ( void ) ;
/* INDENT: OFF */
void indent_still_on ( void ) ;	/* due to the colon in the middle */
/* INDENT OFF */		/* extra comment */
void indent_still_on ( void ) ;	/* due to the extra comment to the right */
//indent end

//indent run
/* INDENT OFF */
void indent_off ( void ) ;
/*  INDENT */
void		indent_on(void);
/* INDENT OFF */
void indent_off ( void ) ;
	/* INDENT ON */
void		indent_on(void);	/* the comment may be indented */
/* INDENT		OFF					*/
void indent_off ( void ) ;
/* INDENTATION ON */
void indent_still_off ( void ) ;	/* due to the word 'INDENTATION' */
/* INDENT ON * */
void indent_still_off ( void ) ;	/* due to the extra '*' at the end */
/* INDENT ON */
void		indent_on(void);
/* INDENT: OFF */
void		indent_still_on(void);	/* due to the colon in the middle */
/* $ The extra comment got moved to a separate line, but indenting is still */
/* $ on because the 'INDENT OFF' comment was not in a line of its own. */
/* INDENT OFF */
/* extra comment */
void		indent_still_on(void);	/* due to the extra comment to the
					 * right */
//indent end


/*
 * Try to confuse indent by having a string literal that has an embedded
 * INDENT comment.  Indent doesn't get confused though because it requires the
 * INDENT comment to go from the very beginning of the line to the very end of
 * the line.
 */
//indent input
const char *str = "\
/* INDENT OFF */\
"   ,   ch;
//indent end

//indent run
const char     *str = "\
/* INDENT OFF */\
", ch;
//indent end


/*
 * The keywords in the INDENT comments must all be uppercase.
 */
//indent input
int   on   ;
/* indent off */
int   still_on   ;
/* INDENT off */
int   still_on   ;
/* indent OFF */
int   still_on   ;
/* INDENT OFF */
int   finally_off   ;
//indent end

//indent run -di0
int on;
/* indent off */
int still_on;
/* INDENT off */
int still_on;
/* indent OFF */
int still_on;
/* INDENT OFF */
int   finally_off   ;
//indent end


/*
 * Ensure that in 'INDENT OFF' mode, no blank line is added between lines, even
 * when requested via the -bacc option.
 */
//indent input
/* INDENT OFF */
int declaration;
#if 0
#endif
int declaration;
/* INDENT ON */
//indent end

//indent run-equals-input -bacc


/*
 * If an 'INDENT OFF' comment directly follows a line continuation, the line
 * continuation is dropped but the rest of the line is still formatted.
 */
//indent input
int x ; \
/* INDENT OFF */
  int y ;
/* INDENT ON */
int z ;
//indent end

//indent run
int		x;
/* INDENT OFF */
  int y ;
/* INDENT ON */
int		z;
//indent end
