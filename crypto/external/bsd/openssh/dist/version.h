/*	$NetBSD: version.h,v 1.25.2.2 2020/04/13 07:45:20 martin Exp $	*/
/* $OpenBSD: version.h,v 1.86 2020/02/14 00:39:20 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_8.2"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20200225"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
