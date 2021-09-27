/*	$NetBSD: version.h,v 1.38 2021/09/27 17:03:13 christos Exp $	*/
/* $OpenBSD: version.h,v 1.92 2021/09/26 14:01:11 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_8.8"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20210927"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
