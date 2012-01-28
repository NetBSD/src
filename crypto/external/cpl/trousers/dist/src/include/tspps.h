
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#ifndef _TSPPS_H_
#define _TSPPS_H_

#define PASSWD_BUFSIZE		4096

#define TSS_USER_PS_DIR		".trousers"
#define TSS_USER_PS_FILE	"user.data"

TSS_RESULT	   get_file(int *);
int		   put_file(int);
inline TSS_RESULT  read_data(int, void *, UINT32);
inline TSS_RESULT  write_data(int, void *, UINT32);
UINT32		   psfile_get_num_keys(int);
TSS_RESULT	   psfile_get_parent_uuid_by_uuid(int, TSS_UUID *, TSS_UUID *);
TSS_RESULT	   psfile_remove_key_by_uuid(int, TSS_UUID *);
TSS_RESULT	   psfile_get_key_by_uuid(int, TSS_UUID *, BYTE *);
TSS_RESULT	   psfile_get_ps_type_by_uuid(int, TSS_UUID *, UINT32 *);
TSS_RESULT	   psfile_is_pub_registered(int, TCPA_STORE_PUBKEY *, TSS_BOOL *);
TSS_RESULT	   psfile_is_key_registered(int, TSS_UUID *, TSS_BOOL *);
TSS_RESULT	   psfile_get_uuid_by_pub(int, UINT32, BYTE *, TSS_UUID *);
TSS_RESULT	   psfile_write_key(int, TSS_UUID *, TSS_UUID *, UINT32, BYTE *, UINT16);
TSS_RESULT	   psfile_get_key_by_pub(int, TSS_UUID *, UINT32, BYTE *, BYTE *);
TSS_RESULT	   psfile_get_registered_keys(int, TSS_UUID *, TSS_UUID *, UINT32 *, TSS_KM_KEYINFO **);
TSS_RESULT	   psfile_get_registered_keys2(int, TSS_UUID *, TSS_UUID *, UINT32 *, TSS_KM_KEYINFO2 **);
TSS_RESULT	   psfile_remove_key(int, TSS_UUID *);
TSS_RESULT	   psfile_get_parent_ps_type(int, TSS_UUID *, UINT32 *);
TSS_RESULT	   psfile_get_cache_entry_by_uuid(int, TSS_UUID *, struct key_disk_cache *);
TSS_RESULT	   psfile_get_cache_entry_by_pub(int, UINT32, BYTE *, struct key_disk_cache *);
void		   psfile_close(int);

TSS_RESULT	   ps_remove_key(TSS_UUID *);
TSS_RESULT	   ps_write_key(TSS_UUID *, TSS_UUID *, UINT32, UINT32, BYTE *);
TSS_RESULT	   ps_get_key_by_uuid(TSS_HCONTEXT, TSS_UUID *, TSS_HKEY *);
TSS_RESULT	   ps_init_disk_cache();
TSS_RESULT	   ps_close();
TSS_RESULT	   ps_get_key_by_pub(TSS_HCONTEXT, UINT32, BYTE *, TSS_HKEY *);
TSS_RESULT	   ps_get_parent_uuid_by_uuid(TSS_UUID *, TSS_UUID *);
TSS_RESULT	   ps_get_parent_ps_type_by_uuid(TSS_UUID *, UINT32 *);
TSS_RESULT	   ps_is_key_registered(TSS_UUID *, TSS_BOOL *);
TSS_RESULT	   ps_get_registered_keys(TSS_UUID *uuid, TSS_UUID *, UINT32 *size, TSS_KM_KEYINFO **);
TSS_RESULT	   ps_get_registered_keys2(TSS_UUID *uuid, TSS_UUID *, UINT32 *size, TSS_KM_KEYINFO2 **);

#ifdef TSS_BUILD_PS
#define PS_close()	ps_close()
#else
#define PS_close()
#endif

#endif
