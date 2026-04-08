/*	$NetBSD: version.h,v 1.53 2026/04/08 18:58:42 christos Exp $	*/
/* $OpenBSD: version.h,v 1.108 2026/04/02 07:51:12 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_10.3"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20260408"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
#define SSH_RELEASE	SSH_VERSION
