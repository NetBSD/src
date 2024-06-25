/*	$NetBSD: version.h,v 1.45 2024/06/25 16:36:54 christos Exp $	*/
/* $OpenBSD: version.h,v 1.101 2024/03/11 04:59:47 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_9.7"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20240625"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
