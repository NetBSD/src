/*	$NetBSD: pathnames.h,v 1.13.2.1 2018/04/07 04:11:48 pgoyette Exp $	*/
/* $OpenBSD: pathnames.h,v 1.28 2018/02/23 15:58:37 markus Exp $ */

/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */

#define ETCDIR				"/etc"
#define SSHDIR				ETCDIR "/ssh"
#define _PATH_SSH_PIDDIR		"/var/run"

/*
 * System-wide file containing host keys of known hosts.  This file should be
 * world-readable.
 */
#define _PATH_SSH_SYSTEM_HOSTFILE	SSHDIR "/ssh_known_hosts"
/* backward compat for protocol 2 */
#define _PATH_SSH_SYSTEM_HOSTFILE2	SSHDIR "/ssh_known_hosts2"

/*
 * Of these, ssh_host_key must be readable only by root, whereas ssh_config
 * should be world-readable.
 */
#define _PATH_SERVER_CONFIG_FILE	SSHDIR "/sshd_config"
#define _PATH_HOST_CONFIG_FILE		SSHDIR "/ssh_config"
#define _PATH_HOST_DSA_KEY_FILE		SSHDIR "/ssh_host_dsa_key"
#define _PATH_HOST_ECDSA_KEY_FILE	SSHDIR "/ssh_host_ecdsa_key"
#define _PATH_HOST_RSA_KEY_FILE		SSHDIR "/ssh_host_rsa_key"
#define _PATH_HOST_ED25519_KEY_FILE	SSHDIR "/ssh_host_ed25519_key"
#define _PATH_HOST_XMSS_KEY_FILE	SSHDIR "/ssh_host_xmss_key"
#define _PATH_DH_MODULI			ETCDIR "/moduli"

#define _PATH_SSH_PROGRAM		"/usr/bin/ssh"

/*
 * The process id of the daemon listening for connections is saved here to
 * make it easier to kill the correct daemon when necessary.
 */
#define _PATH_SSH_DAEMON_PID_FILE	_PATH_SSH_PIDDIR "/sshd.pid"

/*
 * The directory in user's home directory in which the files reside. The
 * directory should be world-readable (though not all files are).
 */
#define _PATH_SSH_USER_DIR		".ssh"

/*
 * Per-user file containing host keys of known hosts.  This file need not be
 * readable by anyone except the user him/herself, though this does not
 * contain anything particularly secret.
 */
#define _PATH_SSH_USER_HOSTFILE		"~/" _PATH_SSH_USER_DIR "/known_hosts"
/* backward compat for protocol 2 */
#define _PATH_SSH_USER_HOSTFILE2	"~/" _PATH_SSH_USER_DIR "/known_hosts2"

/*
 * Name of the default file containing client-side authentication key. This
 * file should only be readable by the user him/herself.
 */
#define _PATH_SSH_CLIENT_ID_DSA		_PATH_SSH_USER_DIR "/id_dsa"
#define _PATH_SSH_CLIENT_ID_ECDSA	_PATH_SSH_USER_DIR "/id_ecdsa"
#define _PATH_SSH_CLIENT_ID_RSA		_PATH_SSH_USER_DIR "/id_rsa"
#define _PATH_SSH_CLIENT_ID_ED25519	_PATH_SSH_USER_DIR "/id_ed25519"
#define _PATH_SSH_CLIENT_ID_XMSS	_PATH_SSH_USER_DIR "/id_xmss"

/*
 * Configuration file in user's home directory.  This file need not be
 * readable by anyone but the user him/herself, but does not contain anything
 * particularly secret.  If the user's home directory resides on an NFS
 * volume where root is mapped to nobody, this may need to be world-readable.
 */
#define _PATH_SSH_USER_CONFFILE		_PATH_SSH_USER_DIR "/config"

/*
 * File containing a list of those rsa keys that permit logging in as this
 * user.  This file need not be readable by anyone but the user him/herself,
 * but does not contain anything particularly secret.  If the user's home
 * directory resides on an NFS volume where root is mapped to nobody, this
 * may need to be world-readable.  (This file is read by the daemon which is
 * running as root.)
 */
#define _PATH_SSH_USER_PERMITTED_KEYS	_PATH_SSH_USER_DIR "/authorized_keys"

/* backward compat for protocol v2 */
#define _PATH_SSH_USER_PERMITTED_KEYS2	_PATH_SSH_USER_DIR "/authorized_keys2"

/*
 * Per-user and system-wide ssh "rc" files.  These files are executed with
 * /bin/sh before starting the shell or command if they exist.  They will be
 * passed "proto cookie" as arguments if X11 forwarding with spoofing is in
 * use.  xauth will be run if neither of these exists.
 */
#define _PATH_SSH_USER_RC		_PATH_SSH_USER_DIR "/rc"
#define _PATH_SSH_SYSTEM_RC		SSHDIR "/sshrc"

/*
 * Ssh-only version of /etc/hosts.equiv.  Additionally, the daemon may use
 * ~/.rhosts and /etc/hosts.equiv if rhosts authentication is enabled.
 */
#define _PATH_SSH_HOSTS_EQUIV		ETCDIR "/shosts.equiv"
#define _PATH_RHOSTS_EQUIV		"/etc/hosts.equiv"

/*
 * X11 base directory
 */
#ifndef X11BASE
#define X11BASE				"/usr/X11R6"
#endif

/*
 * Default location of askpass
 */
#define _PATH_SSH_ASKPASS_DEFAULT	X11BASE "/bin/ssh-askpass"

/* Location of ssh-keysign for hostbased authentication */
#define _PATH_SSH_KEY_SIGN		"/usr/libexec/ssh-keysign"

/* Location of ssh-pkcs11-helper to support keys in tokens */
#define _PATH_SSH_PKCS11_HELPER		"/usr/libexec/ssh-pkcs11-helper"

/* xauth for X11 forwarding */
#define _PATH_XAUTH			X11BASE "/bin/xauth"

/* UNIX domain socket for X11 server; displaynum will replace %u */
#define _PATH_UNIX_X "/tmp/.X11-unix/X%u"

/* for scp */
#define _PATH_CP			"cp"

/* for sftp */
#define _PATH_SFTP_SERVER		"/usr/libexec/sftp-server"
#define _PATH_LS			"ls"

/* chroot directory for unprivileged user when UsePrivilegeSeparation=yes */
#ifdef __OpenBSD__
#define _PATH_PRIVSEP_CHROOT_DIR	"/var/empty"
#else
#define _PATH_PRIVSEP_CHROOT_DIR	"/var/chroot/sshd"
#endif

#define _PATH_URANDOM			"/dev/urandom"

/* for passwd change */
#define _PATH_PASSWD_PROG		"/usr/bin/passwd"
