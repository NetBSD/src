/*	$NetBSD: version.h,v 1.36 2021/04/19 14:40:15 christos Exp $	*/
/* $OpenBSD: version.h,v 1.90 2021/04/16 03:42:00 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_8.6"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20210419"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
