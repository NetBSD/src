
/********************************************
print.c
copyright 1992, 1991.  Michael D. Brennan

This is a source file for mawk, an implementation of
the AWK programming language.

Mawk is distributed without warranty under the terms of
the GNU General Public License, version 2, 1991.
********************************************/

/* $Log: print.c,v $
/* Revision 1.2  1993/07/02 23:57:48  jtc
/* Updated to mawk 1.1.4
/*
 * Revision 5.4.1.2  1993/01/20  12:53:11  mike
 * d_to_l()
 *
 * Revision 5.4.1.1  1993/01/15  03:33:47  mike
 * patch3: safer double to int conversion
 *
 * Revision 5.4  1992/11/29  18:03:11  mike
 * when printing integers, convert doubles to
 * longs so output is the same on 16bit systems as 32bit systems
 *
 * Revision 5.3  1992/08/17  14:23:21  brennan
 * patch2: After parsing, only bi_sprintf() uses string_buff.
 *
 * Revision 5.2  1992/02/24  10:52:16  brennan
 * printf and sprintf() can now have more args than % conversions
 * removed HAVE_PRINTF_HD -- it was too obscure
 *
 * Revision 5.1  91/12/05  07:56:22  brennan
 * 1.1 pre-release
 * 
*/

#include "mawk.h"
#include "bi_vars.h"
#include "bi_funct.h"
#include "memory.h"
#include "field.h"
#include "scan.h"
#include "files.h"

static void  PROTO( print_cell, (CELL *, FILE *) ) ;
static STRING* PROTO( do_printf, (FILE *, char *, unsigned, CELL *) ) ;
static void  PROTO( bad_conversion, (int, char *, char *)) ;



/* this can be moved and enlarged  by -W sprintf=num  */
char *sprintf_buff = string_buff ;
char *sprintf_limit = string_buff + SPRINTF_SZ ;

/* Once execute() starts the sprintf code is (belatedly) the only
   code allowed to use string_buff  */

static void print_cell(p, fp)
  register CELL *p ;
  register FILE *fp ;
{ 
  int len ;
  
  switch( p->type )
  {
    case C_NOINIT : break ;
    case C_MBSTRN :
    case C_STRING :
    case C_STRNUM :
        switch( len = string(p)->len )
        {
          case 0 :  break ;
          case 1 :
                    putc(string(p)->str[0],fp) ;
                    break ;

          default :
                    fwrite(string(p)->str, SIZE_T(1), SIZE_T(len), fp) ;
        }
        break ;

    case C_DOUBLE :
	{
	  long ival = d_to_l(p->dval) ;

	  /* integers print as "%[l]d" */ 
	  if ( (double) ival == p->dval )
	        fprintf(fp, INT_FMT, ival) ;
	  else
          fprintf(fp, string(OFMT)->str, p->dval) ;
	}
        break ;

    default :
        bozo("bad cell passed to print_cell") ;
  }
}

/* on entry to bi_print or bi_printf the stack is:

   sp[0] = an integer k
       if ( k < 0 )  output is to a file with name in sp[-1]
       { so open file and sp -= 2 }

   sp[0] = k >= 0 is the number of print args
   sp[-k]   holds the first argument 
*/

CELL *bi_print(sp)
  CELL *sp ; /* stack ptr passed in */
{ 
  register CELL *p ;
  register int k ;
  FILE *fp ;

  k = sp->type ;
  if ( k < 0 )
  { 
    /* k holds redirection */
    if ( (--sp)->type < C_STRING ) cast1_to_s(sp) ;
    fp = (FILE *) file_find( string(sp), k ) ;
    free_STRING(string(sp)) ;
    k = (--sp)->type ;
    /* k now has number of arguments */
  }
  else  fp = stdout ;

  if ( k )  
  { 
    p = sp - k ; /* clear k variables off the stack */
    sp = p - 1 ;
    k-- ;

    while ( k > 0 ) 
    { 
      print_cell(p,fp) ; print_cell(OFS,fp) ;
      cell_destroy(p) ; 
      p++ ; k-- ;
    }
    
    print_cell(p, fp) ;  cell_destroy(p) ;
  }
  else  
  { /* print $0 */
    sp-- ;
    print_cell( &field[0], fp )  ;
  }

  print_cell(ORS , fp) ;
  return sp ;
}
  
/*---------- types and defs for doing printf and sprintf----*/
#define  PF_C		0   /* %c */
#define  PF_S		1   /* %s */
#define  PF_D		2   /* int conversion */
#define  PF_F		3   /* float conversion */

/* for switch on number of '*' and type */
#define  AST(num,type)  ((PF_F+1)*(num)+(type))

/* some picky ANSI compilers go berserk without this */
#if HAVE_PROTOS
typedef int (*PRINTER)(PTR,char *,...) ;
#else
typedef int (*PRINTER)() ;
#endif

/*-------------------------------------------------------*/

static void bad_conversion(cnt, who, format)
  int cnt ; 
  char *who , *format ;
{
  rt_error( "improper conversion(number %d) in %s(\"%s\")", 
	     cnt, who, format ) ;
}

/* the contents of format are preserved,
   caller does CELL cleanup 

   This routine does both printf and sprintf (if fp==0)
*/
static STRING *do_printf( fp, format, argcnt, cp)
  FILE *fp ;
  char *format ; 
  CELL *cp ;  /* ptr to an array of arguments ( on the eval stack) */
  unsigned argcnt ;  /* number of args on eval stack */
{ 
  char  save ;
  char *p ;
  register char *q = format ;
  register char *target ;
  int l_flag , h_flag ;  /* seen %ld or %hd  */
  int ast_cnt ;
  int ast[2] ;
  long lval ;  /* hold integer values */
  int num_conversion = 0 ; /* for error messages */
  char *who ; /*ditto*/
  int pf_type ;  /* conversion type */
  PRINTER printer ; /* pts at fprintf() or sprintf() */

#ifdef   SHORT_INTS
  char xbuff[256] ; /* splice in l qualifier here */
#endif

  if ( fp == (FILE *) 0 ) /* doing sprintf */
  {
    target = sprintf_buff ;
    printer = (PRINTER) sprintf ;
    who = "sprintf" ;
  }
  else /* doing printf */
  {
    target = (char *) fp ; /* will never change */
    printer = (PRINTER) fprintf ;
    who = "printf" ;
  }

  while ( 1 )
  { 
    if ( fp )  /* printf */
    {
      while ( *q != '%' )
	if ( *q == 0 )  return (STRING *) 0 ;
	else
	{ putc(*q,fp) ; q++ ; }
    }
    else  /* sprintf */
    {
      while ( *q != '%' )
	if ( *q == 0 )
	{
		if ( target > sprintf_limit ) /* damaged */
		{
		  /* hope this works */
		  rt_overflow("sprintf buffer",
			  sprintf_limit - sprintf_buff) ;
		}
		else  /* really done */
		{
		  STRING *retval ;
		  int len = target - sprintf_buff ;

		  retval = new_STRING((char*)0, len) ;
		  (void)memcpy(retval->str, sprintf_buff, SIZE_T(len)) ;
		  return retval ;
		}
	}
	else  *target++ = *q++ ;
    }
       

    /* *q == '%' */
    num_conversion++ ;
  
    if ( * ++q == '%' )   /* %% */
    {
	if ( fp )   putc(*q, fp) ; 
	else *target++ = *q ;

	q++ ; continue ;
    }

    /* mark the '%' with p */
    p = q-1 ;

    /* eat the flags */
    while ( *q == '-' || *q == '+' || *q == ' ' ||
            *q == '#' || *q == '0' )  q++ ;

    ast_cnt = 0 ;
    if ( *q == '*' )
    { 
      if ( cp->type != C_DOUBLE ) cast1_to_d(cp) ;
      ast[ast_cnt++] = d_to_i(cp++ ->dval) ;
      argcnt-- ; q++ ;
    }
    else
    while ( scan_code[*(unsigned char *)q] == SC_DIGIT )  q++ ;
    /* width is done */

    if ( *q == '.' )  /* have precision */
    { q++ ;
      if ( *q == '*' )
      {
	if ( cp->type != C_DOUBLE ) cast1_to_d(cp) ;
        ast[ast_cnt++] = d_to_i(cp++ ->dval) ;
        argcnt-- ; q++ ;
      }
      else
      while ( scan_code[*(unsigned char*)q] == SC_DIGIT ) q++ ; 
    }

    if ( argcnt <= 0 )  
        rt_error("not enough arguments passed to %s(\"%s\")",
		  who, format) ;

    l_flag = h_flag = 0 ;

    if ( *q == 'l' ) { q++ ; l_flag = 1 ; }
    else
    if ( *q == 'h' ) { q++ ; h_flag = 1 ; }

    switch( *q++ )
    {
      case 's' :
            if ( l_flag + h_flag ) 
		bad_conversion(num_conversion,who,format) ;
            if ( cp->type < C_STRING ) cast1_to_s(cp) ;
            pf_type = PF_S ;
            break ;

      case 'c' :
            if ( l_flag + h_flag )
		bad_conversion(num_conversion,who,format) ;

	    switch( cp->type )
	    {
	      case C_NOINIT :
		    lval = 0 ;
		    break ;

	      case C_STRNUM :
	      case C_DOUBLE :
		    lval = (long) d_to_i(cp->dval) ;
		    break ;

	      case  C_STRING :
		    lval = string(cp)->str[0] ;
		    break ;

	      case  C_MBSTRN :
		    check_strnum(cp) ;
		    lval = cp->type == C_STRING ?
			string(cp)->str[0] : (long) d_to_i(cp->dval) ;
		    break ;
	      
	      default :
		    bozo("printf %c") ;
	    }

            pf_type = PF_C ;
	    break ;

      case 'd' :
      case 'o' :
      case 'x' :
      case 'X' :
      case 'i' :
      case 'u' :
            if ( cp->type != C_DOUBLE ) cast1_to_d(cp) ;
	    if ( cp->dval > MAX__LONG ) lval = MAX__LONG ;
	    else
	    if ( cp->dval > -MAX__LONG ) lval = (long) cp->dval ;
	    else lval = -MAX__LONG ;

            pf_type = PF_D ;
            break ;
    
      case 'e' :
      case 'g' :
      case 'f' :
      case 'E' :
      case 'G' :
            if ( h_flag + l_flag )
		bad_conversion(num_conversion,who,format) ;
            if ( cp->type != C_DOUBLE ) cast1_to_d(cp) ;
            pf_type = PF_F ;
            break ;

      default : bad_conversion(num_conversion,who,format) ;
    }

    save = *q ;
    *q = 0 ;

#ifdef  SHORT_INTS
    if ( pf_type == PF_D )
    {
      /* need to splice in long modifier */
      strcpy(xbuff, p) ;

      if ( l_flag ) /* do nothing */ ;
      else
      {
	int k = q - p ;

	if ( h_flag )
	{
	  lval = (short) lval ;
	  /* replace the 'h' with 'l' (really!) */
	  xbuff[k-2] = 'l' ;
	  if ( xbuff[k-1] != 'd' && xbuff[k-1] != 'i' ) lval &= 0xffff ;
	}
	else
	{
	  /* the usual case */
	  xbuff[k] = xbuff[k-1] ;
	  xbuff[k-1] = 'l' ;
	  xbuff[k+1] = 0 ;
	}
      }
    }
#endif

    /* ready to call printf() */
    switch( AST(ast_cnt, pf_type ) )
    {
      case AST(0, PF_C )  :
            (*printer)((PTR) target, p, (int) lval) ;
            break ;

      case AST(1, PF_C ) :
            (*printer)((PTR) target, p, ast[0], (int) lval) ;
            break ;

      case AST(2, PF_C ) :
            (*printer)((PTR) target, p, ast[0], ast[1], (int)lval) ;
            break ;

      case AST(0, PF_S) :
            (*printer)((PTR) target, p, string(cp)->str) ;
            break ;

      case AST(1, PF_S) :
            (*printer)((PTR) target, p, ast[0],string(cp)->str) ;
            break ;

      case AST(2, PF_S) :
            (*printer)((PTR) target,p,ast[0],ast[1],string(cp)->str) ;
            break ;

#ifdef  SHORT_INTS
#define	FMT	xbuff     /* format in xbuff */
#else
#define FMT     p         /* p -> format */
#endif
      case AST(0, PF_D) :
            (*printer)((PTR) target, FMT, lval) ;
            break ;

      case AST(1, PF_D) :
            (*printer)((PTR) target, FMT, ast[0], lval) ;
            break ;

      case AST(2, PF_D) :
            (*printer)((PTR) target, FMT, ast[0], ast[1], lval) ;
            break ;

#undef  FMT


      case AST(0, PF_F) :
            (*printer)((PTR) target, p,  cp->dval) ;
            break ;

      case AST(1, PF_F) :
            (*printer)((PTR) target, p, ast[0],  cp->dval) ;
            break ;

      case AST(2, PF_F) :
            (*printer)((PTR) target, p, ast[0], ast[1],  cp->dval) ;
            break ;
    }
    if ( fp == (FILE *) 0 ) while ( *target ) target++ ;
    *q = save ; argcnt-- ; cp++ ;
  }
}

CELL *bi_printf(sp)
  register CELL *sp ;
{ register int k ;
  register CELL *p ;
  FILE *fp ;

  k = sp->type ;
  if ( k  < 0 )
  { 
    /* k has redirection */
    if ( (--sp)->type < C_STRING ) cast1_to_s(sp) ;
    fp = (FILE *) file_find( string(sp), k ) ;
    free_STRING(string(sp)) ;
    k = (--sp)->type ;
    /* k is now number of args including format */
  }
  else  fp = stdout ;

  sp -= k ; /* sp points at the format string */
  k-- ;

  if ( sp->type < C_STRING )  cast1_to_s(sp) ;
  do_printf(fp, string(sp)->str, k, sp+1) ;
  free_STRING(string(sp)) ;

  /* cleanup arguments on eval stack */
  for ( p = sp+1 ; k ; k--, p++ )  cell_destroy(p) ;
  return --sp ;
}

CELL *bi_sprintf(sp)
  CELL *sp ;
{ CELL *p ;
  int argcnt = sp->type ;
  STRING *sval ;

  sp -= argcnt ; /* sp points at the format string */
  argcnt-- ;

  if ( sp->type != C_STRING )  cast1_to_s(sp) ;
  sval = do_printf((FILE *)0, string(sp)->str, argcnt, sp+1) ;
  free_STRING(string(sp)) ;
  sp->ptr = (PTR) sval ;

  /* cleanup */
  for (p = sp+1 ; argcnt ; argcnt--, p++ )  cell_destroy(p) ;

  return sp ;
}


