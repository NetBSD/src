
/********************************************
files.h
copyright 1991, Michael D. Brennan

This is a source file for mawk, an implementation of
the AWK programming language.

Mawk is distributed without warranty under the terms of
the GNU General Public License, version 2, 1991.
********************************************/

/*$Log: files.h,v $
/*Revision 1.2  1993/07/02 23:57:22  jtc
/*Updated to mawk 1.1.4
/*
 * Revision 5.2  1992/12/17  02:48:01  mike
 * 1.1.2d changes for DOS
 *
 * Revision 5.1  1991/12/05  07:59:18  brennan
 * 1.1 pre-release
 *
*/

#ifndef   FILES_H
#define   FILES_H

/* IO redirection types */
#define  F_IN           (-5)
#define  PIPE_IN        (-4)
#define  PIPE_OUT       (-3)
#define  F_APPEND       (-2)
#define  F_TRUNC        (-1)

extern char *shell ; /* for pipes and system() */

PTR  PROTO(file_find, (STRING *, int)) ;
int  PROTO(file_close, (STRING *)) ;
PTR  PROTO(get_pipe, (char *, int, int *) ) ;
int  PROTO(wait_for, (int) ) ;
void  PROTO( close_out_pipes, (void) ) ;

#if  HAVE_FAKE_PIPES
void PROTO(close_fake_pipes, (void)) ;
int  PROTO(close_fake_outpipe, (char *,int)) ;
char *PROTO(tmp_file_name, (int, char*)) ;
#endif

#if MSDOS
int  PROTO(DOSexec, (char *)) ;
int  PROTO(binmode, (void)) ;
void PROTO(set_binmode, (int)) ;
void PROTO(enlarge_output_buffer, (FILE*)) ;
#endif


#endif
