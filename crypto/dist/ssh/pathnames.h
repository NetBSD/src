/* $NetBSD: pathnames.h,v 1.1.1.2 2001/01/14 04:51:03 itojun Exp $ */

#ifndef _PATHNAMES_H
#define _PATHNAMES_H

/* system utility and file paths */

#define _PATH_CP	"/bin/cp"
#define _PATH_MOTD	"/etc/motd"
#define _PATH_LOGIN	"/usr/bin/login"
#define	_PATH_RSH	"/usr/bin/rsh"
#define _PATH_URANDOM	"/dev/urandom"

/* X Window utility and file paths */

#define _PATH_XAUTH	"/usr/X11R6/bin/xauth"
#define _PATH_XUNIX_DIR	"/tmp/.X11-unix/X"
#define _PATH_SSH_ASKPASS	"/usr/X11R6/bin/ssh-askpass"

/* paths specific to the Secure Shell */

/* mk?temp input string for process-specific scratch dir */
#define	_PATH_SSH_TMPDIR _PATH_TMP "/ssh-XXXXXXXX"

/* daemon pid file */
#define _PATH_SSH_DAEMON_PID_FILE	_PATH_VARRUN "sshd.pid"

/* System-wide files containing host keys of known hosts. */
#define _PATH_SSH_SYSTEM_HOSTFILE	"/etc/ssh_known_hosts"
#define _PATH_SSH_SYSTEM_HOSTFILE2	"/etc/ssh_known_hosts2"

/* host keys (should be readable only by root) */
#define _PATH_HOST_KEY_FILE		"/etc/ssh_host_key"
#define _PATH_HOST_DSA_KEY_FILE		"/etc/ssh_host_dsa_key"
#define _PATH_DH_PRIMES			"/etc/primes"
 
/* client and server config files */
#define _PATH_CLIENT_CONFIG_FILE	"/etc/ssh.conf"
#define _PATH_SERVER_CONFIG_FILE	"/etc/sshd.conf"

/* the ssh utility itself as called from scp */
#define _PATH_SSH			"/usr/bin/ssh"

/* ssh-only version of hosts.equiv */
#define _PATH_SSH_HEQUIV		"/etc/shosts.equiv"

/*
 * name of the directory in each users home directory containing private 
 * configuration and data files.  May need to change if this secure shell
 * implementation becomes incompatible with other implementations.
 */
#define _PATH_SSH_USER_DIR		".ssh"

/*
 * Per-user and system-wide ssh "rc" files.  These files are executed with
 * /bin/sh before starting the shell or command if they exist.  They will be
 * passed "proto cookie" as arguments if X11 forwarding with spoofing is in
 * use.  xauth will be run if neither of these exists.
 */
#define _PATH_SSH_USER_RC		_PATH_SSH_USER_DIR "/rc"
#define _PATH_SSH_SYSTEM_RC		"/etc/sshrc"

/* Per-user file containing host keys of known hosts. */
#define _PATH_SSH_USER_HOSTFILE		"~/" _PATH_SSH_USER_DIR "/known_hosts"
#define _PATH_SSH_USER_HOSTFILE2	"~/" _PATH_SSH_USER_DIR "/known_hosts2"

/*
 * Name of the default file containing client-side authentication key. This
 * file should only be readable by the user him/herself.
 */
#define _PATH_SSH_CLIENT_IDENTITY	_PATH_SSH_USER_DIR "/identity"
#define _PATH_SSH_CLIENT_ID_DSA		_PATH_SSH_USER_DIR "/id_dsa"
#define _PATH_SSH_CLIENT_ID_RSA		_PATH_SSH_USER_DIR "/id_rsa"

/*
 * Configuration and environment files in user's home directory.  These
 * need not be readable by anyone but the user him/herself, but do not
 * contain anything particularly secret.  If the user's home directory
 * resides on an NFS volume where root is mapped to nobody, this may need
 * to be world-readable.
 */
#define _PATH_SSH_USER_CONFFILE		_PATH_SSH_USER_DIR "/config"
#define _PATH_SSH_USER_ENVIRONMENT	_PATH_SSH_USER_DIR "/environment"

/*
 * File containing a list of those rsa keys that permit logging in as this
 * user.  This file need not be readable by anyone but the user him/herself,
 * but does not contain anything particularly secret.  If the user's home
 * directory resides on an NFS volume where root is mapped to nobody, this
 * may need to be world-readable.  (This file is read by the daemon which is
 * running as root.)
 */
#define _PATH_SSH_USER_PERMITTED_KEYS	_PATH_SSH_USER_DIR "/authorized_keys"
#define _PATH_SSH_USER_PERMITTED_KEYS2	_PATH_SSH_USER_DIR "/authorized_keys2"

#endif
