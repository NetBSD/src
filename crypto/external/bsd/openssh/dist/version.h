/*	$NetBSD: version.h,v 1.49 2025/02/18 17:53:24 christos Exp $	*/
/* $OpenBSD: version.h,v 1.103 2024/09/19 22:17:44 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_9.9"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20250218"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
