/*	$NetBSD: version.h,v 1.25 2018/04/06 18:59:00 christos Exp $	*/
/* $OpenBSD: version.h,v 1.81 2018/03/24 19:29:03 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_7.7"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20180405"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
