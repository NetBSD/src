/*	$NetBSD: version.h,v 1.33 2020/05/28 17:05:49 christos Exp $	*/
/* $OpenBSD: version.h,v 1.87 2020/05/06 20:58:01 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_8.3"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20200528"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
