/*	$NetBSD: version.h,v 1.15 2015/07/03 01:00:00 christos Exp $	*/
/* $OpenBSD: version.h,v 1.73 2015/07/01 01:55:13 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_6.9"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20150602"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
