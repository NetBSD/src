/*	$NetBSD: nbdb_index_as.c,v 1.2.2.2 2026/05/11 17:13:51 martin Exp $	*/

/*++
/* NAME
/*	nbdb_index_as 3
/* SUMMARY
/*	Non-Berkeley-DB migration service
/* SYNOPSIS
/*	#include <nbdb_index_as.h>
/*
/*	int	nbdb_index_as(
/*	const char *command,
/*	const char *new_type,
/*	const char *source_path,
/*	uid_t	 uid,
/*	gid_t	 gid,
/*	VSTRING *why)
/* DESCRIPTION
/*	nbdb_index_as() runs a command with the specified effective
/*	uid and gid, to create an indexed file from a source file.
/*	The command is killed after 100s. A limited amount of command
/*	output is returned after failure.
/*
/*	The result value is one of the following:
/* .IP NBDB_STAT_OK
/*	The request was successful.
/* .IP NBDB_STAT_ERROR
/*	The request failed. A description of the problem is returned
/*	in \fBwhy\fR.
/* .PP
/*	Arguments:
/* .IP command
/*	Run this command, "postmap" or "postalias".
/* .IP new_type
/*	Create an indexed file of this type.
/* .IP source_path
/*	The pathname of the Berkeley DB source file.
/* .IP uid, gid
/*	Run the command with these effective uid and gid,
/* .IP why
/*	Storage that is updated with an applicable error description.
/* SEE ALSO
/*	nbdb_index_as_test(1t), unit test
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

 /*
  * Utility library.
  */
#include <argv.h>
#include <iostuff.h>
#include <spawn_command.h>
#include <stringops.h>
#include <vstream.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <mail_params.h>
#include <mail_parm_split.h>

 /*
  * Application-specific.
  */
#include <nbdb_index_as.h>
#include <nbdb_util.h>

#define STR(x)  vstring_str(x)

/* nbdb_index_as - index a database with specified type and privileges */

int     nbdb_index_as(const char *command, const char *new_type,
	        const char *source_path, uid_t uid, gid_t gid, VSTRING *why)
{
    VSTRING *command_path;
    VSTRING *command_arg;
    char   *cmd_argv[3];
    ARGV   *export_env;
    static int devnull_fd = -1;
    int     cmd_out[2];
    int     status;

    /*
     * Redirect the command's stdin to /dev/null. If we are going to do this
     * in multiple programs, then we should make it a library feature.
     */
    if (devnull_fd < 0 && (devnull_fd = open("/dev/null", O_RDWR, 0)) < 0) {
	vstring_sprintf(why, "open /dev/null O_RDWR: %m");
	return (NBDB_STAT_ERROR);
    }

    /*
     * Redirect the command's stdout and stderr to a pipe. See comments in
     * pipe_command.c for a similar strategy to capture a limited amount of
     * command output.
     */
    if (pipe(cmd_out) < 0) {
	vstring_sprintf(why, "create %s output pipe: %m", command);
	return (NBDB_STAT_ERROR);
    }
    non_blocking(cmd_out[0], NON_BLOCKING);	/* us */
    non_blocking(cmd_out[1], NON_BLOCKING);	/* them */

    /*
     * Run the command with a generous time limit. If we kill the command
     * then we leave behind a broken database.
     * 
     * TODO(wietse) extend spawn_command() with an option to return instead of
     * terminating after fork() or wait() failure, maybe with fake command
     * output, and certainly without file descriptor leaks.
     */
    cmd_argv[0] = STR(vstring_sprintf(command_path = vstring_alloc(100),
				      "%s/%s", var_command_dir, command));
    cmd_argv[1] = STR(vstring_sprintf(command_arg = vstring_alloc(100),
				      "%s:%s", new_type, source_path));
    cmd_argv[2] = 0;
    export_env = mail_parm_split(VAR_EXPORT_ENVIRON, var_export_environ);
    status = spawn_command(CA_SPAWN_CMD_STDIN(devnull_fd),
			   CA_SPAWN_CMD_STDOUT(cmd_out[1]),
			   CA_SPAWN_CMD_STDERR(cmd_out[1]),
			   CA_SPAWN_CMD_ARGV(cmd_argv),
			   CA_SPAWN_CMD_UID(uid),
			   CA_SPAWN_CMD_GID(gid),
			   CA_SPAWN_CMD_EXPORT(export_env->argv),
			   CA_SPAWN_CMD_TIME_LIMIT(100),
			   CA_SPAWN_CMD_END);

    /*
     * If the command failed, capture a limited amount of command output.
     */
    close(cmd_out[1]);				/* them */
    if (status != 0) {
	ssize_t count;

#define NBDB_MIN(x, y) ((x) < (y) ? (x) : (y))

	VSTRING_RESET(why);
	VSTRING_SPACE(why, VSTREAM_BUFSIZE);
	errno = 0;
	if (read_wait(cmd_out[0], 0) < 0 ||
	    (count = peekfd(cmd_out[0])) <= 0 ||
	    (count = read(cmd_out[0], STR(why),
			  NBDB_MIN(count, vstring_avail(why)))) <= 0) {
	    vstring_sprintf(why, "(%s failure reason unavailable: %m)",
			    command);
	} else {
	    /* Apply some basic hygiene. */
	    vstring_set_payload_size(why, count);
	    VSTRING_TERMINATE(why);
	    (void) translit(STR(why), "\b\r\n", "__");
	}
    }
    close(cmd_out[0]);				/* us */
    vstring_free(command_arg);
    vstring_free(command_path);
    argv_free(export_env);

    return (status == 0 ? NBDB_STAT_OK : NBDB_STAT_ERROR);
}
