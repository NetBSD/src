/*
 * 	$Id: isofs_bmap.c,v 1.4.2.1 1993/11/14 22:40:41 mycroft Exp $
 */

#include <sys/param.h>
#include <sys/namei.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/mount.h>

#include <isofs/iso.h>
#include <isofs/isofs_node.h>

iso_bmap(ip, lblkno, result)
struct iso_node *ip;
int lblkno;
daddr_t *result;
{
	*result = (ip->i_number + lblkno)
		  * (ip->i_mnt->im_bsize / DEV_BSIZE);
	return (0);
}
