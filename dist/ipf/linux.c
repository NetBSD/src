/*	$NetBSD: linux.c,v 1.1.1.1 2004/03/28 08:55:47 martti Exp $	*/

#include "ipf-linux.h"
#include <linux/devfs_fs_kernel.h>
#include <linux/init.h>

#ifdef CONFIG_PROC_FS
#include <linux/module.h>
#include <linux/proc_fs.h>
#endif

#ifdef MODULE
MODULE_SUPPORTED_DEVICE("ipf");
MODULE_AUTHOR("Darren Reed");
MODULE_DESCRIPTION("IP-Filter Firewall");
MODULE_LICENSE("(C)Copyright 2003 Darren Reed");

MODULE_PARM(fr_flags, "i");
MODULE_PARM(fr_control_forwarding, "i");
MODULE_PARM(fr_update_ipid, "i");
MODULE_PARM(fr_chksrc, "i");
MODULE_PARM(fr_pass, "i");
MODULE_PARM(fr_unreach, "i");
MODULE_PARM(ipstate_logging, "i");
MODULE_PARM(nat_logging, "i");
MODULE_PARM(ipl_suppress, "i");
MODULE_PARM(ipl_logall, "i");
#endif

static int ipf_open(struct inode *, struct file *);
static ssize_t ipf_write(struct file *, const char *, size_t, loff_t *);
static ssize_t ipf_read(struct file *, char *, size_t, loff_t *);
extern int ipf_ioctl(struct inode *, struct file *, u_int, u_long);


int uiomove(address, nbytes, rwflag, uiop)
caddr_t address;
size_t nbytes;
int rwflag;
uio_t *uiop;
{
	int err = 0;

	if (rwflag == UIO_READ) {
		err = copy_to_user(uiop->uio_buf, address, nbytes);
		if (err == 0) {
			uiop->uio_resid -= nbytes;
			uiop->uio_buf += nbytes;
		}
	} else if (rwflag == UIO_WRITE) {
		err = copy_from_user(uiop->uio_buf, address, nbytes);
		if (err == 0) {
			uiop->uio_resid -= nbytes;
			uiop->uio_buf += nbytes;
		}
	}
	if (err)
		return EFAULT;
	return 0;
}


static int ipf_open(struct inode *in, struct file *fp)
{
	int unit, err;

	err = 0;
	unit = MINOR(in->i_rdev);

	if (unit < 0 || unit > IPL_LOGMAX)
		err = -ENXIO;
	return err;
}


static ssize_t ipf_write(struct file *fp, const char *buf, size_t count,
			loff_t *posp)
{
#ifdef IPFILTER_SYNC
	struct inode *i;
	int unit, err;
	uio_t uio;

	i = fp->f_dentry->d_inode;
	unit = MINOR(i->i_rdev);

	if (unit != IPL_LOGSYNC)
		return -ENXIO;

	uio.uio_rw = UIO_WRITE;
        uio.uio_iov = NULL;
        uio.uio_file = fp;
        uio.uio_buf = (char *)buf;
        uio.uio_iovcnt = 0;
        uio.uio_offset = *posp;
        uio.uio_resid = count;

	err = ipfsync_write(&uio);
	if (err > 0)
		err = -err;
	return err;
#else
	return -ENXIO;
#endif
}


static ssize_t ipf_read(struct file *fp, char *buf, size_t count, loff_t *posp)
{
	struct inode *i;
	int unit, err;
	uio_t uio;

	i = fp->f_dentry->d_inode;
	unit = MINOR(i->i_rdev);

	uio.uio_rw = UIO_READ;
        uio.uio_iov = NULL;
        uio.uio_file = fp;
        uio.uio_buf = (char *)buf;
        uio.uio_iovcnt = 0;
        uio.uio_offset = *posp;
        uio.uio_resid = count;

	switch (unit)
	{
#ifdef IPFILTER_SYNC
	case IPL_LOGSYNC :
		err = ipfsync_read(&uio);
		break;
#endif
	default :
		err = ipflog_read(unit, &uio);
		if (err == 0)
			return count - uio.uio_resid;
		break;
	}

	if (err > 0)
		err = -err;
	return err;
}

#ifdef	CONFIG_DEVFS_FS
static	char	*ipf_devfiles[] = { IPL_NAME, IPNAT_NAME, IPSTATE_NAME,
				    IPAUTH_NAME, IPSYNC_NAME, IPSCAN_NAME,
				    IPLOOKUP_NAME, NULL };
#endif

static struct file_operations ipf_fops = {
	open:	ipf_open,
	read:	ipf_read,
	write:	ipf_write,
	ioctl:	ipf_ioctl,
};


#ifdef	CONFIG_DEVFS_FS
static	devfs_handle_t	dh[IPL_LOGSIZE];
#endif
static	int		ipfmajor = 0;


#ifdef	CONFIG_PROC_FS
static int ipf_proc_info(char *buffer, char **start, off_t offset, int len)
{
	snprintf(buffer, len, "ipfmajor %d", ipfmajor);
	buffer[len - 1] = '\0';
	return strlen(buffer);
}
#endif

static int ipfilter_init(void)
{
#ifdef	CONFIG_DEVFS_FS
	char *s;
#endif
	int i;

	ipfmajor = register_chrdev(0, "ipf", &ipf_fops);
	if (ipfmajor < 0) {
		printf("unable to get major for ipf devs\n");
		return -EINVAL;
	}

#ifdef	CONFIG_DEVFS_FS
	for (i = 0; ipf_devfiles[i] != NULL; i++) {
		s = strrchr(ipf_devfiles[i], '/');
		if (s != NULL) {
			dh[i] = devfs_register(NULL, s + 1, DEVFS_FL_DEFAULT,
					       ipfmajor, i, 0600|S_IFCHR,
					       &ipf_fops, NULL);
		}
	}
#endif

	i = iplattach();

#ifdef	CONFIG_PROC_FS
	if (i == 0) {
		struct proc_dir_entry *ipfproc;
		char *defpass;

		ipfproc = proc_net_create("ipfilter", 0, ipf_proc_info);
# ifndef __GENKSYMS__
		if (ipfproc != NULL)
			ipfproc->owner = THIS_MODULE;
# endif
		if (FR_ISPASS(fr_pass))
			defpass = "pass";
		else if (FR_ISBLOCK(fr_pass))
			defpass = "block";
		else
			defpass = "no-match -> block";

		printk(KERN_INFO "%s initialized.  Default = %s all, "
		       "Logging = %s%s\n",
			ipfilter_version, defpass,
#ifdef IPFILTER_LOG
			"enabled",
#else
			"disabled",
#endif
#ifdef IPFILTER_COMPILED
			" (COMPILED)"
#else
			""
#endif
			);

		fr_running = 1;
	}
#endif

	return i;
}


static int ipfilter_fini(void)
{
	int result;
#ifdef	CONFIG_DEVFS_FS
	char *s;
	int i;
#endif

	if (fr_refcnt)
		return EBUSY;

	if (fr_running >= 0) {
		result = ipldetach();
		if (result != 0) {
			if (result > 0)
				result = -result;
			return result;
		}
	}

	fr_running = -2;
#ifdef CONFIG_PROC_FS
	proc_net_remove("ipfilter");
#endif


#ifdef	CONFIG_DEVFS_FS
	for (i = 0; ipf_devfiles[i] != NULL; i++) {
		s = strrchr(ipf_devfiles[i], '/');
		if (s != NULL)
			devfs_unregister_chrdev(ipfmajor, s + 1);
	}
#endif
	unregister_chrdev(ipfmajor, "ipf");
	printk(KERN_INFO "%s unloaded\n", ipfilter_version);

	return 0;
}


static int __init ipf_init(void)
{
	int result;

	result = ipfilter_init();
	return result;
}


static void __exit ipf_fini(void)
{
	(void) ipfilter_fini();
}

module_init(ipf_init)
module_exit(ipf_fini)

