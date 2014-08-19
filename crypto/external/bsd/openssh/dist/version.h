/*	$NetBSD: version.h,v 1.8.2.3 2014/08/19 23:45:25 tls Exp $	*/
/* $OpenBSD: version.h,v 1.67.2.1 2013/11/08 01:38:21 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_6.4"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20131108"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
