/*	$NetBSD: version.h,v 1.28 2019/04/20 17:28:19 christos Exp $	*/
/* $OpenBSD: version.h,v 1.84 2019/04/03 15:48:45 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_8.0"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20190418"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
