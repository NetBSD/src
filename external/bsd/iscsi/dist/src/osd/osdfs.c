/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By downloading, copying, installing or
 * using the software you agree to this license. If you do not agree to this license, do not download, install,
 * copy or use the software. 
 *
 * Intel License Agreement 
 *
 * Copyright (c) 2002, Intel Corporation
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met: 
 *
 * -Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *  following disclaimer. 
 *
 * -Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *  following disclaimer in the documentation and/or other materials provided with the distribution. 
 *
 * -The name of Intel Corporation may not be used to endorse or promote products derived from this software
 *  without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE. 
 */

/*
 * Object-Based Storage Devices (OSD) Filesystem for Linux
 */


#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/locks.h>
#include <asm/uaccess.h>
#include <endian.h>
#include <linux/blkdev.h>
#include <scsi.h>
#include "osd.h"
#include "osd_ops.h"
#include "iscsiutil.h"
#include "util.c"


/*
 * Contants
 */


#define OSDFS_MAGIC  0xabcdef01
#define MAX_INODES   32768
#define MAX_NAME_LEN 32


/*
 * Types
 */


typedef struct osdfs_link_t {
  char name[MAX_NAME_LEN];
  struct osdfs_link_t* next;
} osdfs_link_t;

typedef struct osdfs_inode_t {
  osdfs_link_t  *link;
} osdfs_inode_t;

typedef struct osdfs_metadata_t {
  uint64_t ObjectID;
  int      used;
} osdfs_metadata_t;


/*
 * Prototypes
 */

static int osdfs_mknod(struct inode *dir, struct dentry *dentry, int mode, int dev);


/*
 * Globals
 */


static struct super_operations osdfs_ops;
static struct address_space_operations osdfs_aops;
static struct file_operations osdfs_dir_operations;
static struct file_operations osdfs_file_operations;
static struct inode_operations osdfs_dir_inode_operations;
static uint32_t root_gid;
static uint64_t root_uid;
static iscsi_mutex_t g_mutex;


/*
 * SCSI transport function for OSD
 */


int osd_exec_via_scsi(void *dev, osd_args_t *args, OSD_OPS_MEM *m) {
  Scsi_Request *SRpnt;
  Scsi_Device *SDpnt;
  unsigned char cdb[256];
  kdev_t kdev = *((kdev_t *) dev);
  void *ptr = NULL;
  int len = 0;

  if (m->send_sg||m->recv_sg) {
    iscsi_err("scatter/gather not yet implemented!\n");
    return -1;
  }

  SDpnt = blk_dev[MAJOR(kdev)].queue(kdev)->queuedata;
  SRpnt = scsi_allocate_request(SDpnt); 
  SRpnt->sr_cmd_len = CONFIG_OSD_CDB_LEN;
  SRpnt->sr_sense_buffer[0] = 0;
  SRpnt->sr_sense_buffer[2] = 0;
  switch(args->service_action) {
    case OSD_WRITE:
    case OSD_SET_ATTR:
      len = m->send_len;
      ptr = m->send_data;
      SRpnt->sr_data_direction = SCSI_DATA_WRITE;
      break;
    case OSD_CREATE:
    case OSD_CREATE_GROUP:
    case OSD_READ:
    case OSD_GET_ATTR:
      len = m->recv_len;
      ptr = m->recv_data;
      SRpnt->sr_data_direction = SCSI_DATA_READ;
      break;
    case OSD_REMOVE:
    case OSD_REMOVE_GROUP:
      SRpnt->sr_data_direction = 0;
      break;
    default:
      iscsi_err("unsupported OSD service action 0x%x\n", args->service_action);
      return -1;
  }
  OSD_ENCAP_CDB(args, cdb);

  /*  Exec SCSI command */

  scsi_wait_req(SRpnt, cdb, ptr, len, 5*HZ, 5);
  if (SRpnt->sr_result!=0) {
    iscsi_err("SCSI command failed (result %u)\n", SRpnt->sr_result);
    scsi_release_request(SRpnt);
    SRpnt = NULL;
    return -1;
  } 
  scsi_release_request(SRpnt);
  SRpnt = NULL;

  return 0;
}

/* 
 * Internal OSDFS functions
 */


/* Directory operations */

static int entries_get(kdev_t dev, uint64_t uid, char **entries, uint32_t *num, uint64_t *size) {
  struct inode inode;
  uint16_t len;

  if (osd_get_one_attr((void *)&dev, root_gid, uid, 0x30000000, 0x0, sizeof(struct inode), &osd_exec_via_scsi, &len, (void *) &inode)!=0) {
    iscsi_err("osd_get_one_attr() failed\n");
    return -1;
  }
  *num = 0; 
  if ((*size=inode.i_size)) {
    char *ptr, *ptr2;
    int n = 0;

    if ((*entries=vmalloc(*size+1))==NULL) {
      iscsi_err("vmalloc() failed\n");
      return -1;
    }
    if (osd_read((void *)&dev, root_gid, uid, 0, *size, *entries, 0, &osd_exec_via_scsi)!=0) {
      iscsi_err("osd_read() failed\n");
      vfree(*entries);
      return -1;
    }
    (*entries)[*size] = 0x0;
    ptr = *entries;
    do {
      n++;
      if ((ptr2=strchr(ptr, '\n'))!=NULL) {
        n++;
        if ((ptr2 = strchr(ptr2+1, '\n'))==NULL) {
          iscsi_err("directory 0x%llx corrupted (line %i)\n", uid, n);
          return -1;
        }
        (*num)++;
      } else {
        iscsi_err("directory 0x%llx corrupted (line %i)\n", uid, n);
        return -1;
      }
      ptr = ptr2+1;
    } while (*ptr);
  }

  return 0;
}

static int entry_add(kdev_t dev, ino_t dir_ino, ino_t entry_ino, 
                     const char *name, uint64_t *new_size) {
  char entry[MAX_NAME_LEN+16];
  uint64_t uid = dir_ino;
  struct inode inode;
  uint16_t len;

  /*  Get size of directory */

  if (osd_get_one_attr((void *)&dev, root_gid, uid, 0x30000000, 0x0, sizeof(struct inode), &osd_exec_via_scsi, &len, (void *) &inode)!=0) {
    iscsi_err("osd_get_one_attr() failed\n");
    return -1;
  }

  /*  Write entry at end */
  
  sprintf(entry, "%s\n", name);
  sprintf(entry+strlen(entry), "%li\n", entry_ino);
  if (osd_write((void *)&dev, root_gid, uid, inode.i_size, strlen(entry), entry, 0, &osd_exec_via_scsi)!=0) {
    iscsi_err("osd_write() failed\n");
    return -1;
  }
  *new_size += strlen(entry);

  return 0;
}

static int entry_del(kdev_t dev, ino_t dir_ino, ino_t ino, const char *name, uint64_t *new_size) {
  char *entries;
  uint32_t num_entries;
  uint64_t size;
  uint64_t dir_uid = (unsigned) dir_ino;

  /*  Read */

  if (entries_get(dev, dir_ino, &entries, &num_entries, &size)!=0) {
    iscsi_err("entries_get() failed\n");
    return -1;
  }
  entries[size] = 0x0;

  iscsi_trace(TRACE_OSDFS, "dir_ino 0x%llx has %u entries\n", dir_uid, num_entries);
  if (num_entries) {
    char *ptr = entries;
    char *tmp = NULL;
    char *nl;
    int n = 0;

    do {
      n++;
      if ((nl=strchr(ptr, '\n'))==NULL) {
        iscsi_err("directory 0x%llx corrupted (line %i)\n", dir_uid, n);
        return -1;
      }
      *nl = 0x0;
      if (!strcmp(ptr, name)) {
        tmp = ptr;
      } 
      *nl = '\n';
      n++;
      if ((ptr=strchr(nl+1, '\n'))==NULL) {
        iscsi_err("directory 0x%llx corrupted (line %i)\n", dir_uid, n);
        return -1;
      }
      ptr++;
    } while (!tmp && *ptr);

    if (!tmp) {
      iscsi_err("entry \"%s\" not found in dir 0x%llx\n", name, dir_uid);
      return -1;
    }
    if (entries+size-ptr) {
      iscsi_trace(TRACE_OSDFS, "writing remaining %u directory bytes at offset %u\n", 
            entries+size-ptr, tmp-entries);
      if (osd_write((void *)&dev, root_gid, dir_uid, tmp-entries, entries+size-ptr, ptr, 0, &osd_exec_via_scsi)!=0) {
        iscsi_err("osd_write() failed\n");
        return -1;
      }
    }
    *new_size = size-(ptr-tmp); 
    vfree(entries);
  } else {
    iscsi_err("dir 0x%llx has no entries\n", dir_uid);
    return -1;
  }

  return 0;
}

static int entry_num(kdev_t dev, ino_t ino) {
  char *entries; 
  uint32_t num_entries;
  uint64_t size;

  if (entries_get(dev, ino, &entries, &num_entries, &size)!=0) {
    iscsi_err("entries_get() failed\n");
    return -1;
  }
  iscsi_trace(TRACE_OSDFS, "ino %li has %i entries\n", ino, num_entries);
  if (num_entries) vfree(entries);
  return num_entries;
}

/* Inode operations */

static void osdfs_set_ops(struct inode *inode) {
  switch (inode->i_mode & S_IFMT) {
    case S_IFREG:
      inode->i_fop = &osdfs_file_operations;
      break;
    case S_IFDIR:
      inode->i_op = &osdfs_dir_inode_operations;
      inode->i_fop = &osdfs_dir_operations;
      break;
    case S_IFLNK:
      inode->i_op = &page_symlink_inode_operations;
      break;
    default:
      iscsi_err("UNKNOWN MODE\n");
  }
  inode->i_mapping->a_ops = &osdfs_aops;
}

static struct inode *osdfs_get_inode(struct super_block *sb, int mode, int dev, const char *name, 
                              uint64_t ObjectID) {
  struct inode *inode;
  ino_t ino = ObjectID;

  iscsi_trace(TRACE_OSDFS, "osdfs_get_inode(\"%s\", mode %i (%s))\n", name, mode,
        S_ISDIR(mode)?"DIR":(S_ISREG(mode)?"REG":"LNK"));

  /*  iget() gets a free VFS inode and subsequently call  */
  /*  osdfds_read_inode() to fill the inode structure. */

  if ((inode=iget(sb, ino))==NULL) {
    iscsi_err("iget() failed\n");
    return NULL;
  }

  return inode;
}


/*
 * Super Operations
 */


static void osdfs_read_inode(struct inode *inode) {
  ino_t ino = inode->i_ino;
  kdev_t dev = inode->i_sb->s_dev;
  uint64_t uid = ino;
  unsigned char *attr;
  uint16_t len;

  iscsi_trace(TRACE_OSDFS, "osdfs_read_inode(ino 0x%x, major %i, minor %i)\n",
        (unsigned) ino, MAJOR(dev), MINOR(dev));

  /*  Get object attributes for rest of inode */

  if ((attr=iscsi_malloc_atomic(sizeof(struct inode)))==NULL) {
    iscsi_err("iscsi_malloc_atomic() failed\n");
  }
  if (osd_get_one_attr((void *)&dev, root_gid, uid, 0x30000000, 0x0, sizeof(struct inode), &osd_exec_via_scsi, &len, attr)!=0) {
    iscsi_err("osd_get_one_attr() failed\n");
    return;
  }

  inode->i_size   = ((struct inode *)(attr))->i_size;
  inode->i_mode   = ((struct inode *)(attr))->i_mode;
  inode->i_nlink  = ((struct inode *)(attr))->i_nlink;
  inode->i_gid    = ((struct inode *)(attr))->i_gid;
  inode->i_uid    = ((struct inode *)(attr))->i_uid;
  inode->i_ctime  = ((struct inode *)(attr))->i_ctime;
  inode->i_atime  = ((struct inode *)(attr))->i_atime;
  inode->i_mtime  = ((struct inode *)(attr))->i_mtime;

  iscsi_free_atomic(attr);

  osdfs_set_ops(inode);
}

void osdfs_dirty_inode(struct inode *inode) {
  iscsi_trace(TRACE_OSDFS, "osdfs_dirty_inode(ino 0x%x)\n", (unsigned) inode->i_ino);
}

void osdfs_write_inode(struct inode *inode, int sync) {
  ino_t ino = inode->i_ino;
  kdev_t dev = inode->i_sb->s_dev;
  uint64_t uid = ino;

  iscsi_trace(TRACE_OSDFS, "osdfs_write_inode(0x%llx)\n", uid);

  if (osd_set_one_attr((void *)&dev, root_gid, uid, 0x30000000, 0x1, sizeof(struct inode), (void *) inode, &osd_exec_via_scsi)!=0) {
    iscsi_err("osd_set_one_attr() failed\n");
  }
  inode->i_state &= ~I_DIRTY;
}

void osdfs_put_inode(struct inode *inode) {
  iscsi_trace(TRACE_OSDFS, "osdfs_put_inode(0x%x)\n", (unsigned) inode->i_ino);
}

void osdfs_delete_inode(struct inode *inode) {
  iscsi_trace(TRACE_OSDFS, "osdfs_delete_inode(%lu)\n", inode->i_ino);
  clear_inode(inode);
}

void osdfs_put_super(struct super_block *sb) {
  iscsi_err("osdfs_put_super() not implemented\n");
}

void osdfs_write_super(struct super_block *sb) {
  iscsi_err("osdfs_write_super() not implemented\n");
}

void osdfs_write_super_lockfs(struct super_block *sb) {
  iscsi_err("osdfs_write_super_lockfs() not implemented\n");
}

void osdfs_unlockfs(struct super_block *sb) {
  iscsi_err("osdfs_unlockfs() not implemented\n");
}

int osdfs_statfs(struct super_block *sb, struct statfs *buff) {
  iscsi_trace(TRACE_OSDFS, "statfs()\n");
  buff->f_type    = OSDFS_MAGIC;
  buff->f_bsize   = PAGE_CACHE_SIZE;
  buff->f_blocks  = 256;
  buff->f_bfree   = 128;
  buff->f_bavail  = 64;
  buff->f_files   = 0;
  buff->f_ffree   = 0;
  buff->f_namelen = MAX_NAME_LEN;

  return 0;
}

int osdfs_remount_fs(struct super_block *sb, int *i, char *c) {
  iscsi_err("osdfs_remount_fs() not implemented\n");

  return -1;
}

void osdfs_clear_inode(struct inode *inode) {
  iscsi_trace(TRACE_OSDFS, "osdfs_clear_inode(ino %lu)\n", inode->i_ino);
}

void osdfs_umount_begin(struct super_block *sb) {
  iscsi_err("osdfs_unmount_begin() not implemented\n");
}

static struct super_operations osdfs_ops = {
  read_inode: osdfs_read_inode,
  dirty_inode: osdfs_dirty_inode,
  write_inode: osdfs_write_inode,
  put_inode: osdfs_put_inode,
  delete_inode: osdfs_delete_inode,
  put_super: osdfs_put_super,
  write_super: osdfs_write_super,
  write_super_lockfs: osdfs_write_super_lockfs,
  unlockfs: osdfs_unlockfs,
  statfs: osdfs_statfs,
  remount_fs: osdfs_remount_fs,
  clear_inode: osdfs_clear_inode,
  umount_begin: osdfs_umount_begin
};


/*
 * Inode operations for directories
 */


static int osdfs_create(struct inode *dir, struct dentry *dentry, int mode) {

  iscsi_trace(TRACE_OSDFS, "osdfs_create(\"%s\")\n", dentry->d_name.name);
  if (osdfs_mknod(dir, dentry, mode | S_IFREG, 0)!=0) {
    iscsi_err("osdfs_mknod() failed\n");
    return -1;
  } 
  iscsi_trace(TRACE_OSDFS, "file \"%s\" is inode 0x%x\n", dentry->d_name.name, (unsigned) dentry->d_inode->i_ino);

  return 0;
}

static struct dentry * osdfs_lookup(struct inode *dir, struct dentry *dentry) {
  const char *name = dentry->d_name.name;
  struct inode *inode = NULL;
  ino_t ino;
  kdev_t dev = dir->i_sb->s_dev;
  uint64_t uid = dir->i_ino;
  char *entries;
  uint32_t num_entries;
  uint64_t size;

  iscsi_trace(TRACE_OSDFS, "osdfs_lookup(\"%s\" in dir ino %lu)\n", name, dir->i_ino);

  /*  Get directory entries */

  ISCSI_LOCK(&g_mutex, return NULL);
  if (entries_get(dev, uid, &entries, &num_entries, &size)!=0) {
    iscsi_err("entries_get() failed\n");
    ISCSI_UNLOCK(&g_mutex, return NULL);
    return NULL;
  }
  ISCSI_UNLOCK(&g_mutex, return NULL);
  iscsi_trace(TRACE_OSDFS, "ino %li has %i entries\n", dir->i_ino, num_entries);

  /*  Search for this entry */

  if (num_entries) {
    char *ptr = entries;
    char *ptr2;

    do {
      if ((ptr2=strchr(ptr, '\n'))!=NULL) {
        *ptr2 = 0x0;
        ptr2 = strchr(ptr2+1, '\n');
        if (!strcmp(ptr, name)) {
          sscanf(ptr+strlen(ptr)+1, "%li", &ino);
          iscsi_trace(TRACE_OSDFS, "found \"%s\" at ino %li\n", name, ino);
          if ((inode=iget(dir->i_sb, ino))==NULL) {
            iscsi_err("iget() failed\n");
            return NULL;
          }
        } 
      }
    } while (ptr2&&(ptr=ptr2+1));
    vfree(entries);
  } 
  if (!inode) {
    iscsi_trace(TRACE_OSDFS, "\"%s\" not found\n", name);
  }
  d_add(dentry, inode);

  return NULL;
}

static int osdfs_link(struct dentry *old_dentry, struct inode * dir, struct dentry * dentry) {
  struct inode *inode = old_dentry->d_inode;
  kdev_t dev = dir->i_sb->s_dev;
  ino_t dir_ino = dir->i_ino;
  ino_t ino = inode->i_ino;
  const char *name = dentry->d_name.name;

  if (S_ISDIR(inode->i_mode)) return -EPERM;
  iscsi_trace(TRACE_OSDFS, "osdfs_link(%lu, \"%s\")\n", ino, name);
  ISCSI_LOCK(&g_mutex, return -1);
  if (entry_add(dev, dir_ino, ino, name, &dir->i_size)!=0) {
    iscsi_err("entry_add() failed\n");
    return -1;
  }
  inode->i_nlink++; 
  atomic_inc(&inode->i_count); 
  osdfs_write_inode(inode, 0);
  osdfs_write_inode(dir, 0);
  d_instantiate(dentry, inode);
  ISCSI_UNLOCK(&g_mutex, return -1);

  return 0; 
}

static int osdfs_unlink(struct inode * dir, struct dentry *dentry) {
  kdev_t dev = dir->i_sb->s_dev;
  struct inode *inode = dentry->d_inode;
  ino_t dir_ino = dir->i_ino;
  ino_t ino = dentry->d_inode->i_ino;
  const char *name = dentry->d_name.name;

  iscsi_trace(TRACE_OSDFS, "osdfs_unlink(\"%s\", ino 0x%x)\n", name, (unsigned) ino);
  ISCSI_LOCK(&g_mutex, return -1);
  switch (inode->i_mode & S_IFMT) {
    case S_IFREG:
    case S_IFLNK:
      break;
    case S_IFDIR:
      if (entry_num(dev, ino)) {
        iscsi_err("directory 0x%x still has %i entries\n", 
                    (unsigned) ino, entry_num(dev, ino));
        ISCSI_UNLOCK(&g_mutex, return -1);
        return -ENOTEMPTY;
      }
  }
  if (entry_del(dev, dir_ino, ino, name, &(dir->i_size))!=0) {
    iscsi_err("entry_del() failed\n");
    ISCSI_UNLOCK(&g_mutex, return -1);
    return -1;
  }
  osdfs_write_inode(dir, 0);
  if (--inode->i_nlink) {
    iscsi_trace(TRACE_OSDFS, "ino 0x%x still has %i links\n", (unsigned) ino, inode->i_nlink);
    osdfs_write_inode(inode, 0);
  } else {
    iscsi_trace(TRACE_OSDFS, "ino 0x%x link count reached 0, removing object\n", (unsigned) ino);
    if (osd_remove((void *)&dev, root_gid, ino, &osd_exec_via_scsi)!=0) {
      iscsi_err("osd_remove() failed\n");
      return -1;
    }
  }
  ISCSI_UNLOCK(&g_mutex, return -1);

  return 0;
}

static int osdfs_symlink(struct inode * dir, struct dentry *dentry, const char * symname) {
  struct inode *inode;

  iscsi_trace(TRACE_OSDFS, "osdfs_symlink(\"%s\"->\"%s\")\n", dentry->d_name.name, symname);
  if (osdfs_mknod(dir, dentry,  S_IRWXUGO | S_IFLNK, 0)!=0) {
    iscsi_err("osdfs_mknod() failed\n");
    return -1;
  } 
  inode = dentry->d_inode;
  if (block_symlink(inode, symname, strlen(symname)+1)!=0) {
    iscsi_err("block_symlink() failed\n");
    return -1;
  }
  iscsi_trace(TRACE_OSDFS, "symbolic link \"%s\" is inode %lu\n", dentry->d_name.name, inode->i_ino);

  return 0; 
}

static int osdfs_mkdir(struct inode * dir, struct dentry * dentry, int mode) {

  iscsi_trace(TRACE_OSDFS, "osdfs_mkdir(\"%s\")\n", dentry->d_name.name);
  if (osdfs_mknod(dir, dentry, mode | S_IFDIR, 0)!=0) {
    iscsi_err("osdfs_mkdir() failed\n");
  } 

  return 0;
}

static int osdfs_mknod(struct inode *dir, struct dentry *dentry, int mode, int dev_in) {
  struct inode *inode = NULL;
  uint64_t uid;
  struct inode attr;
  kdev_t dev = dir->i_sb->s_dev;
  const char *name = dentry->d_name.name;

  iscsi_trace(TRACE_OSDFS, "osdfs_mknod(\"%s\")\n", dentry->d_name.name);

  /*  Create object */

  if (osd_create((void *)&dev, root_gid, &osd_exec_via_scsi, &uid)!=0) {
    iscsi_err("osd_create() failed\n");
    return -1;
  }

  /*  Initialize object attributes */

  memset(&attr, 0, sizeof(struct inode));
  attr.i_mode = mode;
  attr.i_uid = current->fsuid;
  attr.i_gid = current->fsgid;
  attr.i_ctime = CURRENT_TIME;
  attr.i_atime = CURRENT_TIME;
  attr.i_mtime = CURRENT_TIME;
  attr.i_nlink = 1;
  if (osd_set_one_attr((void *)&dir->i_sb->s_dev, root_gid, uid, 0x30000000, 0x1, sizeof(struct inode), 
                        &attr, &osd_exec_via_scsi)!=0) {
    iscsi_err("osd_set_one_attr() failed\n");
    return -1;
  }

  /*  Assign to an inode */

  if ((inode = osdfs_get_inode(dir->i_sb, mode, dev, name, uid))==NULL) {
    iscsi_err("osdfs_get_inode() failed\n");
    return -ENOSPC;
  }
  d_instantiate(dentry, inode);

  /*  Add entry to parent directory */

  if (inode->i_ino != 1) {
    ISCSI_LOCK(&g_mutex, return -1);
    if (entry_add(dev, dir->i_ino, inode->i_ino, name, &dir->i_size)!=0) {
      iscsi_err("entry_add() failed\n");
      return -1;
    }
    osdfs_write_inode(dir, 0);
    ISCSI_UNLOCK(&g_mutex, return -1);
  }

  return 0;
}

static int osdfs_rename(struct inode *old_dir, struct dentry *old_dentry, struct inode *new_dir, struct dentry *new_dentry) {
  kdev_t dev = old_dir->i_sb->s_dev;
  ino_t old_dir_ino =  old_dir->i_ino;
  ino_t new_dir_ino = new_dir->i_ino;
  ino_t old_ino = old_dentry->d_inode->i_ino;
  ino_t new_ino = new_dentry->d_inode?new_dentry->d_inode->i_ino:old_ino;
  const char *old_name = old_dentry->d_name.name;
  const char *new_name = new_dentry->d_name.name;

  iscsi_trace(TRACE_OSDFS, "old_dir = 0x%p (ino 0x%x)\n", old_dir, (unsigned) old_dir_ino);
  iscsi_trace(TRACE_OSDFS, "new_dir = 0x%p (ino 0x%x)\n", new_dir, (unsigned) new_dir_ino);
  iscsi_trace(TRACE_OSDFS, "old_dentry = 0x%p (ino 0x%x)\n", old_dentry, (unsigned) old_ino);
  iscsi_trace(TRACE_OSDFS, "new_dentry = 0x%p (ino 0x%x)\n", new_dentry, (unsigned) new_ino);

  /*
   * If we return -1, the VFS will implement a rename with a combination 
   * of osdfs_unlink() and osdfs_create(). 
   */

  /*  Delete entry from old directory */

  ISCSI_LOCK(&g_mutex, return -1);
  if (entry_del(dev, old_dir_ino, old_ino, old_name, &old_dir->i_size)!=0) {
    iscsi_err("error deleting old entry \"%s\"\n", old_name);
    ISCSI_UNLOCK(&g_mutex, return -1);
    return -1;
  }
  osdfs_write_inode(old_dir, 0);
  ISCSI_UNLOCK(&g_mutex, return -1);

  /*  Unlink entry from new directory */

  if (new_dentry->d_inode) {
    iscsi_trace(TRACE_OSDFS, "unlinking existing file\n");
    if (osdfs_unlink(new_dir, new_dentry)!=0) {
      iscsi_err("osdfs_unlink() failed\n");
      return -1;
    }
  }

  /*  Add entry to new directory (might be the same dir) */

  ISCSI_LOCK(&g_mutex, return -1);
  if (entry_add(dev, new_dir_ino, new_ino, new_name, &new_dir->i_size)!=0) {
    iscsi_err("error adding new entry \"%s\"\n", new_name);
    ISCSI_UNLOCK(&g_mutex, return -1);
    return -1;
  }
  osdfs_write_inode(new_dir, 0);
  ISCSI_UNLOCK(&g_mutex, return -1);

  return 0;
}

static struct inode_operations osdfs_dir_inode_operations = {
	create:		osdfs_create,
	lookup:		osdfs_lookup,
	link:		osdfs_link,
	unlink:		osdfs_unlink,
	symlink:	osdfs_symlink,
	mkdir:		osdfs_mkdir,
	rmdir:		osdfs_unlink,
	mknod:		osdfs_mknod,
	rename:		osdfs_rename,
};


/*
 * File operations (regular files)
 */


static int osdfs_sync_file(struct file * file, struct dentry *dentry, int datasync) {
  iscsi_err("osdfs_syncfile() not implemented\n");
  return -1;
}

static struct file_operations osdfs_file_operations = {
	read:		generic_file_read,
	write:		generic_file_write,
	mmap:		generic_file_mmap,
	fsync:		osdfs_sync_file,
};


/*
 * File operations (directories)
 */


static int osdfs_readdir(struct file * filp, void * dirent, filldir_t filldir) {
  struct dentry *dentry = filp->f_dentry;
  const char *name;
  ino_t ino = dentry->d_inode->i_ino;
  kdev_t dev = dentry->d_inode->i_sb->s_dev;
  int offset = filp->f_pos;
  char *entries, *ptr, *ptr2;
  uint32_t num_entries;
  uint64_t size;
  uint64_t uid = ino;

  name = dentry->d_name.name;
  iscsi_trace(TRACE_OSDFS, "osdfs_readdir(\"%s\", ino 0x%x, offset %i)\n", 
        name, (unsigned) ino, offset);
  ISCSI_LOCK(&g_mutex, return -1);
  if (entries_get(dev, uid, &entries, &num_entries, &size)!=0) {
    iscsi_err("entries_get() failed\n");
    ISCSI_UNLOCK(&g_mutex, return -1);
    return -1;
  }
  ISCSI_UNLOCK(&g_mutex, return -1);

  /*  Update the offset if our number of entries has changed since the last  */
  /*  call to osdfs_readdir().  filp->private_data stores the number of  */
  /*  entries this directory had on the last call. */

  if (offset) {
    if (((int)filp->private_data)>num_entries) {
      filp->f_pos = offset -= (((int)filp->private_data)-num_entries);
      filp->private_data = (void *) num_entries;
    }
  } else {
    filp->private_data = (void *) num_entries;
  }

  switch (offset) {

    case 0:

      iscsi_trace(TRACE_OSDFS, "adding \".\" (ino 0x%x)\n", (unsigned) ino);
      if (filldir(dirent, ".", 1, filp->f_pos++, ino, DT_DIR) < 0) {
        iscsi_err("filldir() failed for \".\"??\n");
        vfree(entries);
        return -1;
      }
   
    case 1:

      iscsi_trace(TRACE_OSDFS, "adding \"..\" (ino 0x%x)\n", (unsigned) dentry->d_parent->d_inode->i_ino);
      if (filldir(dirent, "..", 2, filp->f_pos++, dentry->d_parent->d_inode->i_ino, DT_DIR) < 0) {
        iscsi_err("filldir() failed for \"..\"??\n");
        vfree(entries);
        return -1;
      }

    default:

      if (!num_entries) return 0;
      ptr = entries;
      offset -= 2;
      do {
        if ((ptr2=strchr(ptr, '\n'))!=NULL) {
          *ptr2 = 0x0; 
          ptr2 = strchr(ptr2+1, '\n');
          if (offset>0) {
            offset--;
          } else {
            sscanf(ptr+strlen(ptr)+1, "%li", &ino);
            iscsi_trace(TRACE_OSDFS, "adding \"%s\" (ino 0x%x)\n", ptr, (unsigned) ino);
            if (filldir(dirent, ptr, strlen(ptr), filp->f_pos++, ino, DT_UNKNOWN) < 0) {
              vfree(entries);
              return 0;
            }
          }
        } 
      } while (ptr2&&(ptr=ptr2+1));
  }
  if (num_entries) vfree(entries);

  return 0;
}

static struct file_operations osdfs_dir_operations = {
	read:		generic_read_dir,
	readdir:        osdfs_readdir,
	fsync:		osdfs_sync_file,
};


/*
 * Address space operations
 */


static int osdfs_readpage(struct file *file, struct page * page) {
  uint64_t Offset = page->index<<PAGE_CACHE_SHIFT;
  uint64_t Length = 1<<PAGE_CACHE_SHIFT;
  struct inode *inode = page->mapping->host;
  kdev_t dev = inode->i_sb->s_dev;
  ino_t ino = inode->i_ino;
  uint64_t len;
  uint64_t uid = ino;
 
  iscsi_trace(TRACE_OSDFS, "osdfs_readpage(ino %lu, Offset %llu, Length %llu)\n", ino, Offset, Length);
  if (Offset+Length>inode->i_size) {
    len =  inode->i_size-Offset;
  } else {
    len = Length;
  }
  if (!Page_Uptodate(page)) {
    memset(kmap(page), 0, PAGE_CACHE_SIZE);
    if (osd_read((void *)&dev, root_gid, uid, Offset, len, page->virtual, 0, &osd_exec_via_scsi)!=0) {
      iscsi_err("osd_read() failed\n");
      UnlockPage(page);
      return -1;;
    }
    kunmap(page);
    flush_dcache_page(page);
    SetPageUptodate(page);
  } else {
    iscsi_err("The page IS up to date???\n");
  }
  UnlockPage(page);

  return 0;
}

static int osdfs_prepare_write(struct file *file, struct page *page, unsigned offset, unsigned to) {
  iscsi_trace(TRACE_OSDFS, "osdfs_prepare_write(ino %lu, offset %u, to %u)\n", page->mapping->host->i_ino, offset, to);
  return 0;
}

static int osdfs_commit_write(struct file *file, struct page *page, unsigned offset, unsigned to) {
  uint64_t Offset = (page->index<<PAGE_CACHE_SHIFT)+offset;
  uint64_t Length = to-offset;
  struct inode *inode = page->mapping->host;
  kdev_t dev = inode->i_sb->s_dev;
  ino_t ino = inode->i_ino;
  uint64_t uid = ino;

  iscsi_trace(TRACE_OSDFS, "osdfs_commit_write(ino %lu, offset %u, to %u, Offset %llu, Length %llu)\n",
        ino, offset, to, Offset, Length);
  if (osd_write((void *)&dev, root_gid, uid, Offset, Length, page->virtual+offset, 0, &osd_exec_via_scsi)!=0) {
    iscsi_err("osd_write() failed\n");
    return -1;
  }
  if (Offset+Length>inode->i_size) {
    inode->i_size = Offset+Length;  
  }
  osdfs_write_inode(inode, 0);

  return 0;
}

static struct address_space_operations osdfs_aops = {
	readpage:	osdfs_readpage,
	writepage:	NULL,
	prepare_write:	osdfs_prepare_write,
	commit_write:	osdfs_commit_write
};


/*
 * Superblock operations
 */


static struct super_block *osdfs_read_super(struct super_block *sb, void *data, int silent) {
  char opt[64];
  char *ptr, *ptr2;
  struct inode attr;
  struct inode *inode;

  iscsi_trace(TRACE_OSDFS, "osdfs_read_super(major %i minor %i)\n", MAJOR(sb->s_dev),  MINOR(sb->s_dev));

  root_gid = root_uid = 0;

  /* Parse options */

  ptr = (char *)data;
  while (ptr&&strlen(ptr)) {
    if ((ptr2=strchr(ptr, ','))) {
      strncpy(opt, ptr, ptr2-ptr);
      opt[ptr2-ptr] = 0x0;
      ptr = ptr2+1;
    } else {
      strcpy(opt, ptr);
      ptr = 0x0;
    }
    if (!strncmp(opt, "uid=", 3)) {
      if (sscanf(opt, "uid=0x%Lx", &root_uid)!=1) {
        iscsi_err("malformed option \"%s\"\n", opt);
        return NULL;
      }
    } else if (!strncmp(opt, "gid=", 3)) {
      if (sscanf(opt, "gid=0x%x", &root_gid)!=1) {
        iscsi_err("malformed option \"%s\"\n", opt);
        return NULL;
      }
    } else {
      iscsi_err("unknown option \"%s\"\n", opt);
      return NULL;
    }
  }
 
  /*  Initialize superblock */

  sb->s_blocksize      = PAGE_CACHE_SIZE;
  sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
  sb->s_magic          = OSDFS_MAGIC;
  sb->s_op             = &osdfs_ops;

  if ((root_uid==0)||(root_gid==0)) {

    /*  Create group object for root directory */

    if (osd_create_group((void *)&sb->s_dev, &osd_exec_via_scsi, &root_gid)!=0) {
      iscsi_err("osd_create_group() failed\n");
      return NULL;
    }
    printf("** ROOT DIRECTORY GROUP OBJECT IS 0x%x **\n", root_gid);

    /*  Create user object for root directory */

    if (osd_create((void *)&sb->s_dev, root_gid, &osd_exec_via_scsi, &root_uid)!=0) {
      iscsi_err("osd_create() failed\n");
      return NULL;
    }
    printf("** ROOT DIRECTORY USER OBJECT IS 0x%llx **\n", root_uid);

    /*  Initialize Attributes */

    memset(&attr, 0, sizeof(struct inode));
    attr.i_mode = S_IFDIR | 0755;
    if (osd_set_one_attr((void *)&sb->s_dev, root_gid, root_uid, 0x30000000, 0x1, sizeof(struct inode), (void *) &attr, &osd_exec_via_scsi)!=0) {
      iscsi_err("osd_set_one_attr() failed\n");
      return NULL;
    }
  } else {
    iscsi_trace(TRACE_OSDFS, "using root directory in 0x%x:0x%llx\n", root_gid, root_uid);
  } 

  /*  Create inode for root directory */
    
  if ((inode=osdfs_get_inode(sb, S_IFDIR | 0755, 0, "/", root_uid))==NULL) {
    iscsi_err("osdfs_get_inode() failed\n");
    return NULL;
  }
  if ((sb->s_root=d_alloc_root(inode))==NULL) {
    iscsi_err("d_alloc_root() failed\n");
    iput(inode);
    return NULL;
  }

  return sb;
}

static DECLARE_FSTYPE_DEV(osdfs_fs_type, "osdfs", osdfs_read_super);


/*
 * Module operations
 */


static int __init init_osdfs_fs(void) {
  iscsi_trace(TRACE_OSDFS, "init_osdfs_fs()\n");
  ISCSI_MUTEX_INIT(&g_mutex, return -1);
  return register_filesystem(&osdfs_fs_type);
}

static void __exit exit_osdfs_fs(void) {
  iscsi_trace(TRACE_OSDFS, "exit_osdfs_fs()\n");
  ISCSI_MUTEX_DESTROY(&g_mutex, printk("mutex_destroy() failed\n"));
  unregister_filesystem(&osdfs_fs_type);
}

module_init(init_osdfs_fs)
module_exit(exit_osdfs_fs)
