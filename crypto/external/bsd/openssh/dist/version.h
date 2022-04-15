/*	$NetBSD: version.h,v 1.40 2022/04/15 14:00:06 christos Exp $	*/
/* $OpenBSD: version.h,v 1.94 2022/04/04 22:45:25 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_9.0"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20220415"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
