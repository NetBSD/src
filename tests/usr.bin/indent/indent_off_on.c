/* $NetBSD: indent_off_on.c,v 1.1 2021/10/19 20:20:25 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the comments 'INDENT OFF' and 'INDENT ON', which temporarily
 * disable formatting.
 */

#indent input
{}

/*INDENT OFF*/
/*INDENT ON*/

{}
#indent end

/*
 * XXX: It is asymmetric that 'INDENT OFF' is kept as is, while 'INDENT ON'
 * gets enclosed with spaces.
 */
#indent run
{
}
/* $ FIXME: This empty line must stay. */
/*INDENT OFF*/
/* INDENT ON */

{
}
#indent end


#indent input
{}
 /* INDENT OFF */
 /* INDENT ON */
{}
#indent end

/*
 * XXX: It is asymmetric that 'INDENT OFF' is indented, while 'INDENT ON'
 * is aligned.
 */
#indent run
{
}
 /* INDENT OFF */
/* INDENT ON */
{
}
#indent end


#indent input
{}
	/* INDENT OFF */
	/* INDENT ON */
{}
#indent end

/*
 * XXX: It is asymmetric that 'INDENT OFF' is indented, while 'INDENT ON'
 * is aligned.
 */
#indent run
{
}
	/* INDENT OFF */
/* INDENT ON */
{
}
#indent end

/*
 * The INDENT comments can be written without space between the words, but
 * nobody does this.
 */
#indent input
int   decl   ;
/*INDENTOFF*/
int   decl   ;
/*INDENTON*/
int   decl   ;
#indent end

#indent run -di0
int decl;
/*INDENTOFF*/
int   decl   ;
/* INDENTON */
int decl;
#indent end


/*
 * Any whitespace around the 'INDENT ON/OFF' is ignored, as is any whitespace
 * between the two words.
 */
#indent input
int decl;
/*		INDENT		OFF		*/
int   decl   ;
/*		INDENT		ON		*/
int decl;
#indent end

/*
 * XXX: It is asymmetric that 'INDENT OFF' is indented, while 'INDENT ON'
 * is aligned.
 */
#indent run -di0
int decl;
/*		INDENT		OFF		*/
int   decl   ;
/* INDENT		ON		*/
int decl;
#indent end
