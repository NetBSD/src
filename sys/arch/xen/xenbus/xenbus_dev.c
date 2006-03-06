/*
 * xenbus_dev.c
 * 
 * Driver giving user-space access to the kernel's xenbus connection
 * to xenstore.
 * 
 * Copyright (c) 2005, Christian Limpach
 * Copyright (c) 2005, Rusty Russell, IBM Corporation
 * 
 * This file may be distributed separately from the Linux kernel, or
 * incorporated into other software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/uio.h>
#include <linux/notifier.h>
#include <linux/wait.h>
#include <linux/fs.h>

#include "xenbus_comms.h"

#include <asm/uaccess.h>
#include <asm/hypervisor.h>
#include <asm-xen/xenbus.h>
#include <asm-xen/xen_proc.h>
#include <asm/hypervisor.h>

struct xenbus_dev_transaction {
	struct list_head list;
	struct xenbus_transaction *handle;
};

struct xenbus_dev_data {
	/* In-progress transaction. */
	struct list_head transactions;

	/* Partial request. */
	unsigned int len;
	union {
		struct xsd_sockmsg msg;
		char buffer[PAGE_SIZE];
	} u;

	/* Response queue. */
#define MASK_READ_IDX(idx) ((idx)&(PAGE_SIZE-1))
	char read_buffer[PAGE_SIZE];
	unsigned int read_cons, read_prod;
	wait_queue_head_t read_waitq;
};

static struct proc_dir_entry *xenbus_dev_intf;

static ssize_t xenbus_dev_read(struct file *filp,
			       char __user *ubuf,
			       size_t len, loff_t *ppos)
{
	struct xenbus_dev_data *u = filp->private_data;
	int i;

	if (wait_event_interruptible(u->read_waitq,
				     u->read_prod != u->read_cons))
		return -EINTR;

	for (i = 0; i < len; i++) {
		if (u->read_cons == u->read_prod)
			break;
		put_user(u->read_buffer[MASK_READ_IDX(u->read_cons)], ubuf+i);
		u->read_cons++;
	}

	return i;
}

static void queue_reply(struct xenbus_dev_data *u,
			char *data, unsigned int len)
{
	int i;

	for (i = 0; i < len; i++, u->read_prod++)
		u->read_buffer[MASK_READ_IDX(u->read_prod)] = data[i];

	BUG_ON((u->read_prod - u->read_cons) > sizeof(u->read_buffer));

	wake_up(&u->read_waitq);
}

static ssize_t xenbus_dev_write(struct file *filp,
				const char __user *ubuf,
				size_t len, loff_t *ppos)
{
	struct xenbus_dev_data *u = filp->private_data;
	struct xenbus_dev_transaction *trans;
	void *reply;
	int err = 0;

	if ((len + u->len) > sizeof(u->u.buffer))
		return -EINVAL;

	if (copy_from_user(u->u.buffer + u->len, ubuf, len) != 0)
		return -EFAULT;

	u->len += len;
	if (u->len < (sizeof(u->u.msg) + u->u.msg.len))
		return len;

	switch (u->u.msg.type) {
	case XS_TRANSACTION_START:
	case XS_TRANSACTION_END:
	case XS_DIRECTORY:
	case XS_READ:
	case XS_GET_PERMS:
	case XS_RELEASE:
	case XS_GET_DOMAIN_PATH:
	case XS_WRITE:
	case XS_MKDIR:
	case XS_RM:
	case XS_SET_PERMS:
		reply = xenbus_dev_request_and_reply(&u->u.msg);
		if (IS_ERR(reply)) {
			err = PTR_ERR(reply);
		} else {
			if (u->u.msg.type == XS_TRANSACTION_START) {
				trans = kmalloc(sizeof(*trans), GFP_KERNEL);
				trans->handle = (struct xenbus_transaction *)
					simple_strtoul(reply, NULL, 0);
				list_add(&trans->list, &u->transactions);
			} else if (u->u.msg.type == XS_TRANSACTION_END) {
				list_for_each_entry(trans, &u->transactions,
						    list)
					if ((unsigned long)trans->handle ==
					    (unsigned long)u->u.msg.tx_id)
						break;
				BUG_ON(&trans->list == &u->transactions);
				list_del(&trans->list);
				kfree(trans);
			}
			queue_reply(u, (char *)&u->u.msg, sizeof(u->u.msg));
			queue_reply(u, (char *)reply, u->u.msg.len);
			kfree(reply);
		}
		break;

	default:
		err = -EINVAL;
		break;
	}

	if (err == 0) {
		u->len = 0;
		err = len;
	}

	return err;
}

static int xenbus_dev_open(struct inode *inode, struct file *filp)
{
	struct xenbus_dev_data *u;

	if (xen_start_info->store_evtchn == 0)
		return -ENOENT;

	nonseekable_open(inode, filp);

	u = kmalloc(sizeof(*u), GFP_KERNEL);
	if (u == NULL)
		return -ENOMEM;

	memset(u, 0, sizeof(*u));
	INIT_LIST_HEAD(&u->transactions);
	init_waitqueue_head(&u->read_waitq);

	filp->private_data = u;

	return 0;
}

static int xenbus_dev_release(struct inode *inode, struct file *filp)
{
	struct xenbus_dev_data *u = filp->private_data;
	struct xenbus_dev_transaction *trans, *tmp;

	list_for_each_entry_safe(trans, tmp, &u->transactions, list) {
		xenbus_transaction_end(trans->handle, 1);
		list_del(&trans->list);
		kfree(trans);
	}

	kfree(u);

	return 0;
}

static struct file_operations xenbus_dev_file_ops = {
	.read = xenbus_dev_read,
	.write = xenbus_dev_write,
	.open = xenbus_dev_open,
	.release = xenbus_dev_release,
};

static int __init
xenbus_dev_init(void)
{
	xenbus_dev_intf = create_xen_proc_entry("xenbus", 0400);
	if (xenbus_dev_intf)
		xenbus_dev_intf->proc_fops = &xenbus_dev_file_ops;

	return 0;
}

__initcall(xenbus_dev_init);

/*
 * Local variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
