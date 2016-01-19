/*	$NetBSD: version.h,v 1.19 2016/01/19 17:10:55 christos Exp $	*/
/* $OpenBSD: version.h,v 1.75 2015/08/21 03:45:26 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_7.1"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20160119"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
