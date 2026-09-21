/*	$NetBSD: version.h,v 1.54 2026/09/21 21:31:00 christos Exp $	*/
/* $OpenBSD: version.h,v 1.110 2026/08/10 23:27:30 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_10.5"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20260921"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
#define SSH_RELEASE	SSH_VERSION
