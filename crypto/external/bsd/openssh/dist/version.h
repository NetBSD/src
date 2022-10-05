/*	$NetBSD: version.h,v 1.41 2022/10/05 22:39:36 christos Exp $	*/
/* $OpenBSD: version.h,v 1.95 2022/09/26 22:18:40 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_9.1"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20221004"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
