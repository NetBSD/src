#ifndef SUB_PROC_H
#define SUB_PROC_H

/*
 * Component Name: 
 *
 * $Date: 2006/03/29 21:09:36 $
 *
 * $Source: /cvsroot/src/gnu/dist/gmake/w32/include/Attic/sub_proc.h,v $
 *
 * $Revision: 1.1.1.1 $
 */

/* $Id: sub_proc.h,v 1.1.1.1 2006/03/29 21:09:36 jmc Exp $ */

#ifdef WINDOWS32

#define EXTERN_DECL(entry, args) extern entry args
#define VOID_DECL void

EXTERN_DECL(HANDLE process_init, (VOID_DECL));
EXTERN_DECL(HANDLE process_init_fd, (HANDLE stdinh, HANDLE stdouth,
	HANDLE stderrh));
EXTERN_DECL(long process_begin, (HANDLE proc, char **argv, char **envp,
	char *exec_path, char *as_user));
EXTERN_DECL(long process_pipe_io, (HANDLE proc, char *stdin_data, 
	int stdin_data_len));
EXTERN_DECL(long process_file_io, (HANDLE proc));
EXTERN_DECL(void process_cleanup, (HANDLE proc));
EXTERN_DECL(HANDLE process_wait_for_any, (VOID_DECL));
EXTERN_DECL(void process_register, (HANDLE proc));
EXTERN_DECL(HANDLE process_easy, (char** argv, char** env));
EXTERN_DECL(BOOL process_kill, (HANDLE proc, int signal));

/* support routines */
EXTERN_DECL(long process_errno, (HANDLE proc));
EXTERN_DECL(long process_last_err, (HANDLE proc));
EXTERN_DECL(long process_exit_code, (HANDLE proc));
EXTERN_DECL(long process_signal, (HANDLE proc));
EXTERN_DECL(char * process_outbuf, (HANDLE proc));
EXTERN_DECL(char * process_errbuf, (HANDLE proc));
EXTERN_DECL(int process_outcnt, (HANDLE proc));
EXTERN_DECL(int process_errcnt, (HANDLE proc));
EXTERN_DECL(void process_pipes, (HANDLE proc, int pipes[3]));

#endif
#endif
