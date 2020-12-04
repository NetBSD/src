/*	$NetBSD: version.h,v 1.34 2020/12/04 18:42:50 christos Exp $	*/
/* $OpenBSD: version.h,v 1.88 2020/09/27 07:22:05 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_8.4"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20201204"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
