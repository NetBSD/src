/*	$NetBSD: closeout.h,v 1.1.1.1 2016/01/10 21:36:18 christos Exp $	*/

#ifndef CLOSEOUT_H
# define CLOSEOUT_H 1

# ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

void close_stdout_set_status PARAMS ((int status));
void close_stdout_set_file_name PARAMS ((const char *file));
void close_stdout PARAMS ((void));
void close_stdout_status PARAMS ((int status));

#endif
