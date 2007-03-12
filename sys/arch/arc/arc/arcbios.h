/*	$NetBSD: arcbios.h,v 1.9.8.1 2007/03/12 05:46:47 rmind Exp $	*/
/*	$OpenBSD: arcbios.h,v 1.1 1998/01/29 15:06:22 pefo Exp $	*/

/*-
 * Copyright (c) 1996 M. Warner Losh.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>

typedef struct arc_sid
{
	char vendor[8];
	char prodid[8];
} arc_sid_t;

typedef enum arc_config_class
{
	arc_SystemClass,
	arc_ProcessorClass,
	arc_CacheClass,
	arc_AdapterClass,
	arc_ControllerClass,
	arc_PeripheralClass,
	arc_MemoryClass
} arc_config_class_t;

typedef enum arc_config_type
{
	arc_System,

	arc_CentralProcessor,
	arc_FloatingPointProcessor,

	arc_PrimaryIcache,
	arc_PrimaryDcache,
	arc_SecondaryIcache,
	arc_SecondaryDcache,
	arc_SecondaryCache,

	arc_EisaAdapter,		/* Eisa adapter         */
	arc_TcAdapter,			/* Turbochannel adapter */
	arc_ScsiAdapter,		/* SCSI adapter         */
	arc_DtiAdapter,			/* AccessBus adapter    */
	arc_MultiFunctionAdapter,

	arc_DiskController,
	arc_TapeController,
	arc_CdromController,
	arc_WormController,
	arc_SerialController,
	arc_NetworkController,
	arc_DisplayController,
	arc_ParallelController,
	arc_PointerController,
	arc_KeyboardController,
	arc_AudioController,
	arc_OtherController,		/* denotes a controller not otherwise defined */

	arc_DiskPeripheral,
	arc_FloppyDiskPeripheral,
	arc_TapePeripheral,
	arc_ModemPeripheral,
	arc_MonitorPeripheral,
	arc_PrinterPeripheral,
	arc_PointerPeripheral,
	arc_KeyboardPeripheral,
	arc_TerminalPeripheral,
	arc_OtherPeripheral,		/* denotes a peripheral not otherwise defined   */
	arc_LinePeripheral,
	arc_NetworkPeripheral,

	arc_SystemMemory
} arc_config_type_t;

typedef u_char arc_dev_flags_t;

/* Wonder how this is aligned... */
typedef struct arc_config
{
	arc_config_class_t	class;		/* Likely these three all */
	arc_config_type_t	type;		/* need to be uchar to make */
	arc_dev_flags_t		flags;		/* the alignment right */
	uint16_t		version;
	uint16_t		revision;
	uint32_t		key;
	uint32_t		affinity_mask;
	uint32_t		config_data_len;
	uint32_t		id_len;
	char 			*id;
} arc_config_t;

typedef enum arc_status
{
	arc_ESUCCESS,			/* Success                   */
	arc_E2BIG,			/* Arg list too long         */
	arc_EACCES,			/* No such file or directory */
	arc_EAGAIN,			/* Try again                 */
	arc_EBADF,			/* Bad file number           */
	arc_EBUSY,			/* Device or resource busy   */
	arc_EFAULT,			/* Bad address               */
	arc_EINVAL,			/* Invalid argument          */
	arc_EIO,			/* I/O error                 */
	arc_EISDIR,			/* Is a directory            */
	arc_EMFILE,			/* Too many open files       */
	arc_EMLINK,			/* Too many links            */
	arc_ENAMETOOLONG,		/* File name too long        */
	arc_ENODEV,			/* No such device            */
	arc_ENOENT,			/* No such file or directory */
	arc_ENOEXEC,			/* Exec format error         */
	arc_ENOMEM,			/* Out of memory             */
	arc_ENOSPC,			/* No space left on device   */
	arc_ENOTDIR,			/* Not a directory           */
	arc_ENOTTY,			/* Not a typewriter          */
	arc_ENXIO,			/* No such device or address */
	arc_EROFS,			/* Read-only file system     */
} arc_status_t;

#ifdef arc
typedef enum {
	ExeceptionBlock,
	SystemParameterBlock,
	FreeMemory,
	BadMemory,
	LoadedProgram,
	FirmwareTemporary,
	FirmwarePermanent,
	FreeContigous
} MEMORYTYPE;
#endif
#ifdef sgi /* note: SGI's systems have different order of types. */
  typedef enum {
	ExeceptionBlock,
	SystemParameterBlock,
	FreeContigous,
	FreeMemory,
	BadMemory,
	LoadedProgram,
	FirmwareTemporary,
	FirmwarePermanent,
  } MEMORYTYPE;
#endif

typedef struct arc_mem {
	MEMORYTYPE	Type;		/* Memory chunk type */
	uint32_t	BasePage;	/* Page no, first page */
	uint32_t	PageCount;	/* Number of pages */
} arc_mem_t;

typedef void *arc_time_t; /* XXX */

typedef struct arc_dsp_stat {
	uint16_t	CursorXPosition;
	uint16_t	CursorYPosition;
	uint16_t	CursorMaxXPosition;
	uint16_t	CursorMaxYPosition;
	u_char		ForegroundColor;
	u_char		BackgroundColor;
	u_char		HighIntensity;
	u_char		Underscored;
	u_char		ReverseVideo;
} arc_dsp_stat_t;

typedef void *arc_dirent_t; /* XXX */
typedef uint32_t arc_open_mode_t; /* XXX */
typedef uint32_t arc_seek_mode_t; /* XXX */
typedef uint32_t arc_mount_t; /* XXX */

typedef struct arc_calls
{
	arc_status_t (*load)(		/* Load 1 */
		char *,			/* Image to load */
		uint32_t,		/* top address */
		uint32_t *,		/* Entry address */
		uint32_t *);		/* Low address */

	arc_status_t (*invoke)(		/* Invoke 2 */
		uint32_t,		/* Entry Address */
		uint32_t,		/* Stack Address */
		uint32_t,		/* Argc */
		char **,		/* argv */
		char **);		/* envp */

	arc_status_t (*execute)(	/* Execute 3 */
		char *,			/* Image path */
		uint32_t,		/* Argc */
		char **,		/* argv */
		char **);		/* envp */

	void (*halt)(void)
	    __attribute__((__noreturn__));	/* Halt 4 */

	void (*power_down)(void)
	    __attribute__((__noreturn__));	/* PowerDown 5 */

	void (*restart)(void)
	    __attribute__((__noreturn__));	/* Restart 6 */

	void (*reboot)(void)
	    __attribute__((__noreturn__));	/* Reboot 7 */

	void (*enter_interactive_mode)(void)
	    __attribute__((__noreturn__));	/* EnterInteractiveMode 8 */

	void (*return_from_main)(void)
	    __attribute__((__noreturn__));	/* ReturnFromMain 9 */

	arc_config_t *(*get_peer)(	/* GetPeer 10 */
		arc_config_t *); 	/* Component */

	arc_config_t *(*get_child)(	/* GetChild 11 */
		arc_config_t *);	/* Component */

	arc_config_t *(*get_parent)(	/* GetParent 12 */
		arc_config_t *);	/* Component */

	arc_status_t (*get_config_data)( /* GetConfigurationData 13 */
		void *,		/* Configuration Data */
		arc_config_t *);	/* Component */

	arc_config_t *(*add_child)(	/* AddChild 14 */
		arc_config_t *,		/* Component */
		arc_config_t *);	/* New Component */

	arc_status_t (*delete_component)( /* DeleteComponent 15 */
		arc_config_t *);	/* Component */

	arc_config_t *(*get_component)( /* GetComponent 16 */
		char *);		/* Path */

	arc_status_t (*save_config)(void); /* SaveConfiguration 17 */

	arc_sid_t *(*get_system_id)(void); /* GetSystemId 18 */

	arc_mem_t *(*get_memory_descriptor)( /* GetMemoryDescriptor 19 */
		arc_mem_t *);		/* MemoryDescriptor */

#ifdef arc
	void (*signal)(			/* Signal 20 */
		uint32_t,		/* Signal number */
/**/		void *);		/* Handler */
#endif
#ifdef sgi
	void *unused;
#endif

	arc_time_t *(*get_time)(void);	/* GetTime 21 */

	uint32_t (*get_relative_time)(void); /* GetRelativeTime 22 */

	arc_status_t (*get_dir_entry)(	/* GetDirectoryEntry 23 */
		uint32_t,		/* FileId */
		arc_dirent_t *,		/* Directory entry */
		uint32_t,		/* Length */
		uint32_t *);		/* Count */

	arc_status_t (*open)(		/* Open 24 */
		char *,			/* Path */
		arc_open_mode_t,	/* Open mode */
		uint32_t *);		/* FileId */

	arc_status_t (*close)(		/* Close 25 */
		uint32_t);		/* FileId */

	arc_status_t (*read)(		/* Read 26 */
		uint32_t,		/* FileId */
		void *,		/* Buffer */
		uint32_t,		/* Length */
		uint32_t *);		/* Count */

	arc_status_t (*get_read_status)( /* GetReadStatus 27 */
		uint32_t);		/* FileId */

	arc_status_t (*write)(		/* Write 28 */
		uint32_t,		/* FileId */
		void *,		/* Buffer */
		uint32_t,		/* Length */
		uint32_t *);		/* Count */

	arc_status_t (*seek)(		/* Seek 29 */
		uint32_t,		/* FileId */
		int64_t *,		/* Offset */
		arc_seek_mode_t); 	/* Mode */

	arc_status_t (*mount)(		/* Mount 30 */
		char *,			/* Path */
		arc_mount_t);		/* Operation */

	const char *(*getenv)(		/* GetEnvironmentVariable 31 */
		const char *);		/* Variable */

	arc_status_t (*putenv)(		/* SetEnvironmentVariable 32 */
		char *,			/* Variable */
		char *);		/* Value */

	arc_status_t (*get_file_info)(void);	/* GetFileInformation 33 */

	arc_status_t (*set_file_info)(void);	/* SetFileInformation 34 */

	void (*flush_all_caches)(void);	/* FlushAllCaches 35 */

	/* note: the followings don't exist on SGI */
#ifdef arc
	arc_status_t (*test_unicode)(	/* TestUnicodeCharacter 36 */
		uint32_t,		/* FileId */
		uint16_t);		/* UnicodeCharacter */

	arc_dsp_stat_t *(*get_display_status)( /* GetDisplayStatus 37 */
		uint32_t);		/* FileId */
#endif
} arc_calls_t;

#define ARC_PARAM_BLK_MAGIC	0x53435241
#define ARC_PARAM_BLK_MAGIC_BUG	0x41524353	/* This is wrong... but req */

typedef struct arc_param_blk
{
	uint32_t	magic;		/* Magic Number */
	uint32_t	length;		/* Length of parameter block */
	uint16_t	version;	/* ?? */
	uint16_t	revision;	/* ?? */
/**/	void *		restart_block;	/* ?? */
/**/	void *		debug_block;	/* Debugging info -- unused */
/**/	void *		general_exp_vect; /* ?? */
/**/	void *		tlb_miss_exp_vect; /* ?? */
	uint32_t	firmware_length; /* Size of Firmware jumptable in bytes */
	arc_calls_t	*firmware_vect;	/* Firmware jumptable */
	uint32_t	vendor_length;	/* Size of Vendor specific jumptable */
/**/	void *		vendor_vect;	/* Vendor specific jumptable */
	uint32_t	adapter_count;	/* ?? */
	uint32_t	adapter0_type;	/* ?? */
	uint32_t	adapter0_length; /* ?? */
/**/	void *		adapter0_vect;	/* ?? */
} arc_param_blk_t;

#define ArcBiosBase ((arc_param_blk_t *) 0x80001000)
#define ArcBios (ArcBiosBase->firmware_vect)

/*
 * All functions except bios_ident() should be called only if
 *	bios_ident() returns >= 0.
 */
int bios_ident(void);
void bios_init_console(void);
int bios_configure_memory(int *, phys_ram_seg_t *, int *);
void bios_save_info(void);
#ifdef arc
void bios_display_info(int *, int *, int *, int *);
#endif

extern char arc_vendor_id[sizeof(((arc_sid_t *)0)->vendor) + 1];
extern unsigned char arc_product_id[sizeof(((arc_sid_t *)0)->prodid)];
extern char arc_id[64 + 1];
extern char arc_displayc_id[64 + 1];
extern arc_dsp_stat_t arc_displayinfo;
extern int arc_cpu_l2cache_size;
