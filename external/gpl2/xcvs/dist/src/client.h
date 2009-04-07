/*
 * Copyright (C) 1994-2005 The Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* Interface between the client and the rest of CVS.  */

/* Stuff shared with the server.  */
char *mode_to_string (mode_t);
int change_mode (const char *, const char *, int);

extern int gzip_level;
extern int file_gzip_level;

struct buffer;

void make_bufs_from_fds (int, int, int, cvsroot_t *,
			 struct buffer **, struct buffer **, int);


#if defined (CLIENT_SUPPORT) || defined (SERVER_SUPPORT)

/* Whether the connection should be encrypted.  */
extern int cvsencrypt;

/* Whether the connection should use per-packet authentication.  */
extern int cvsauthenticate;

# ifdef ENCRYPTION

#   ifdef HAVE_KERBEROS

/* We can't declare the arguments without including krb.h, and I don't
   want to do that in every file.  */
extern struct buffer *krb_encrypt_buffer_initialize ();

#   endif /* HAVE_KERBEROS */

# endif /* ENCRYPTION */

#endif /* defined (CLIENT_SUPPORT) || defined (SERVER_SUPPORT) */

#ifdef CLIENT_SUPPORT
/*
 * Flag variable for seeing whether the server has been started yet.
 * As of this writing, only edit.c:notify_check() uses it.
 */
extern int server_started;

/* Is the -P option to checkout or update specified?  */
extern int client_prune_dirs;

# ifdef AUTH_CLIENT_SUPPORT
extern int use_authenticating_server;
# endif /* AUTH_CLIENT_SUPPORT */
# if defined (AUTH_CLIENT_SUPPORT) || defined (HAVE_GSSAPI)
void connect_to_pserver (cvsroot_t *, struct buffer **, struct buffer **,
                         int, int );
#   ifndef CVS_AUTH_PORT
#     define CVS_AUTH_PORT 2401
#   endif /* CVS_AUTH_PORT */
#   ifndef CVS_PROXY_PORT
#     define CVS_PROXY_PORT 8080
#   endif /* CVS_AUTH_PORT */
# endif /* (AUTH_CLIENT_SUPPORT) || defined (HAVE_GSSAPI) */

# if HAVE_KERBEROS
#   ifndef CVS_PORT
#     define CVS_PORT 1999
#   endif
# endif /* HAVE_KERBEROS */

/* Talking to the server. */
void send_to_server (const char *str, size_t len);
void read_from_server (char *buf, size_t len);

/* Internal functions that handle client communication to server, etc.  */
bool supported_request (const char *);
void option_with_arg (const char *option, const char *arg);

/* Get the responses and then close the connection.  */
int get_responses_and_close (void);

int get_server_responses (void);

/* Start up the connection to the server on the other end.  */
void
open_connection_to_server (cvsroot_t *root, struct buffer **to_server_p,
                           struct buffer **from_server_p);
void
start_server (void);

/* Send the names of all the argument files to the server.  */
void
send_file_names (int argc, char **argv, unsigned int flags);

/* Flags for send_file_names.  */
/* Expand wild cards?  */
# define SEND_EXPAND_WILD 1

/*
 * Send Repository, Modified and Entry.  argc and argv contain only
 * the files to operate on (or empty for everything), not options.
 * local is nonzero if we should not recurse (-l option).
 */
void
send_files (int argc, char **argv, int local, int aflag,
		  unsigned int flags);

/* Flags for send_files.  */
# define SEND_BUILD_DIRS 1
# define SEND_FORCE 2
# define SEND_NO_CONTENTS 4
# define BACKUP_MODIFIED_FILES 8

/* Send an argument to the remote server.  */
void
send_arg (const char *string);

/* Send a string of single-char options to the remote server, one by one.  */
void
send_options (int argc, char * const *argv);

void send_a_repository (const char *, const char *, const char *);

#endif /* CLIENT_SUPPORT */

/*
 * This structure is used to catalog the responses the client is
 * prepared to see from the server.
 */

struct response
{
    /* Name of the response.  */
    const char *name;

#ifdef CLIENT_SUPPORT
    /*
     * Function to carry out the response.  ARGS is the text of the
     * command after name and, if present, a single space, have been
     * stripped off.  The function can scribble into ARGS if it wants.
     * Note that although LEN is given, ARGS is also guaranteed to be
     * '\0' terminated.
     */
    void (*func) (char *args, size_t len);

    /*
     * ok and error are special; they indicate we are at the end of the
     * responses, and error indicates we should exit with nonzero
     * exitstatus.
     */
    enum {response_type_normal, response_type_ok, response_type_error,
	  response_type_redirect} type;
#endif

    /* Used by the server to indicate whether response is supported by
       the client, as set by the Valid-responses request.  */
    enum {
      /*
       * Failure to implement this response can imply a fatal
       * error.  This should be set only for responses which were in the
       * original version of the protocol; it should not be set for new
       * responses.
       */
      rs_essential,

      /* Some clients might not understand this response.  */
      rs_optional,

      /*
       * Set by the server to one of the following based on what this
       * client actually supports.
       */
      rs_supported,
      rs_not_supported
      } status;
};

/* Table of responses ending in an entry with a NULL name.  */

extern struct response responses[];

#ifdef CLIENT_SUPPORT

void client_senddate (const char *date);
void client_expand_modules (int argc, char **argv, int local);
void client_send_expansions (int local, char *where, int build_dirs);
void client_nonexpanded_setup (void);

void send_init_command (void);

extern char **failed_patches;
extern int failed_patches_count;
extern char *toplevel_wd;
void client_import_setup (char *repository);
int client_process_import_file
    (char *message, char *vfile, char *vtag, int targc, char *targv[],
     char *repository, int all_files_binary, int modtime);
void client_import_done (void);
void client_notify (const char *, const char *, const char *, int,
                    const char *);

#if defined AUTH_CLIENT_SUPPORT || defined HAVE_KERBEROS || defined HAVE_GSSAPI
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
struct hostent *init_sockaddr (struct sockaddr_in *, char *, unsigned int);
#endif

#endif /* CLIENT_SUPPORT */
