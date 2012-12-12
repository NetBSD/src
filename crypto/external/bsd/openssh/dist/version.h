/*	$NetBSD: version.h,v 1.10 2012/12/12 18:19:25 christos Exp $	*/
/* $OpenBSD: version.h,v 1.65 2012/07/22 18:19:21 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_6.1"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20121212"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
