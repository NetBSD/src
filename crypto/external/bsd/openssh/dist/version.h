/*	$NetBSD: version.h,v 1.14 2015/04/03 23:58:19 christos Exp $	*/
/* $OpenBSD: version.h,v 1.72 2015/03/04 18:53:53 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_6.8"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20150403"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
