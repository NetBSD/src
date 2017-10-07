/*	$NetBSD: version.h,v 1.24 2017/10/07 19:39:19 christos Exp $	*/
/* $OpenBSD: version.h,v 1.80 2017/09/30 22:26:33 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_7.6"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20171007"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
