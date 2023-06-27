/* option `NFS_BOOT_BOOTSTATIC' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFS_BOOT_BOOTSTATIC
 .global _KERNEL_OPT_NFS_BOOT_BOOTSTATIC
 .equiv _KERNEL_OPT_NFS_BOOT_BOOTSTATIC,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFS_BOOT_BOOTSTATIC\n .global _KERNEL_OPT_NFS_BOOT_BOOTSTATIC\n .equiv _KERNEL_OPT_NFS_BOOT_BOOTSTATIC,0x6e074def\n .endif");
#endif
/* option `NFS_BOOT_UDP' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFS_BOOT_UDP
 .global _KERNEL_OPT_NFS_BOOT_UDP
 .equiv _KERNEL_OPT_NFS_BOOT_UDP,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFS_BOOT_UDP\n .global _KERNEL_OPT_NFS_BOOT_UDP\n .equiv _KERNEL_OPT_NFS_BOOT_UDP,0x6e074def\n .endif");
#endif
/* option `NFS_BOOT_TCP' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFS_BOOT_TCP
 .global _KERNEL_OPT_NFS_BOOT_TCP
 .equiv _KERNEL_OPT_NFS_BOOT_TCP,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFS_BOOT_TCP\n .global _KERNEL_OPT_NFS_BOOT_TCP\n .equiv _KERNEL_OPT_NFS_BOOT_TCP,0x6e074def\n .endif");
#endif
/* option `NFS_BOOT_GATEWAY' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFS_BOOT_GATEWAY
 .global _KERNEL_OPT_NFS_BOOT_GATEWAY
 .equiv _KERNEL_OPT_NFS_BOOT_GATEWAY,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFS_BOOT_GATEWAY\n .global _KERNEL_OPT_NFS_BOOT_GATEWAY\n .equiv _KERNEL_OPT_NFS_BOOT_GATEWAY,0x6e074def\n .endif");
#endif
#define	NFS_BOOT_DHCP	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFS_BOOT_DHCP
 .global _KERNEL_OPT_NFS_BOOT_DHCP
 .equiv _KERNEL_OPT_NFS_BOOT_DHCP,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFS_BOOT_DHCP\n .global _KERNEL_OPT_NFS_BOOT_DHCP\n .equiv _KERNEL_OPT_NFS_BOOT_DHCP,0x1\n .endif");
#endif
/* option `NFS_BOOT_BOOTPARAM' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFS_BOOT_BOOTPARAM
 .global _KERNEL_OPT_NFS_BOOT_BOOTPARAM
 .equiv _KERNEL_OPT_NFS_BOOT_BOOTPARAM,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFS_BOOT_BOOTPARAM\n .global _KERNEL_OPT_NFS_BOOT_BOOTPARAM\n .equiv _KERNEL_OPT_NFS_BOOT_BOOTPARAM,0x6e074def\n .endif");
#endif
/* option `NFS_BOOT_BOOTP' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFS_BOOT_BOOTP
 .global _KERNEL_OPT_NFS_BOOT_BOOTP
 .equiv _KERNEL_OPT_NFS_BOOT_BOOTP,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFS_BOOT_BOOTP\n .global _KERNEL_OPT_NFS_BOOT_BOOTP\n .equiv _KERNEL_OPT_NFS_BOOT_BOOTP,0x6e074def\n .endif");
#endif
/* option `NFS_BOOTSTATIC_SERVER' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFS_BOOTSTATIC_SERVER
 .global _KERNEL_OPT_NFS_BOOTSTATIC_SERVER
 .equiv _KERNEL_OPT_NFS_BOOTSTATIC_SERVER,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFS_BOOTSTATIC_SERVER\n .global _KERNEL_OPT_NFS_BOOTSTATIC_SERVER\n .equiv _KERNEL_OPT_NFS_BOOTSTATIC_SERVER,0x6e074def\n .endif");
#endif
/* option `NFS_BOOTSTATIC_SERVADDR' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFS_BOOTSTATIC_SERVADDR
 .global _KERNEL_OPT_NFS_BOOTSTATIC_SERVADDR
 .equiv _KERNEL_OPT_NFS_BOOTSTATIC_SERVADDR,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFS_BOOTSTATIC_SERVADDR\n .global _KERNEL_OPT_NFS_BOOTSTATIC_SERVADDR\n .equiv _KERNEL_OPT_NFS_BOOTSTATIC_SERVADDR,0x6e074def\n .endif");
#endif
/* option `NFS_BOOTSTATIC_MASK' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFS_BOOTSTATIC_MASK
 .global _KERNEL_OPT_NFS_BOOTSTATIC_MASK
 .equiv _KERNEL_OPT_NFS_BOOTSTATIC_MASK,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFS_BOOTSTATIC_MASK\n .global _KERNEL_OPT_NFS_BOOTSTATIC_MASK\n .equiv _KERNEL_OPT_NFS_BOOTSTATIC_MASK,0x6e074def\n .endif");
#endif
/* option `NFS_BOOTSTATIC_GWIP' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFS_BOOTSTATIC_GWIP
 .global _KERNEL_OPT_NFS_BOOTSTATIC_GWIP
 .equiv _KERNEL_OPT_NFS_BOOTSTATIC_GWIP,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFS_BOOTSTATIC_GWIP\n .global _KERNEL_OPT_NFS_BOOTSTATIC_GWIP\n .equiv _KERNEL_OPT_NFS_BOOTSTATIC_GWIP,0x6e074def\n .endif");
#endif
/* option `NFS_BOOTSTATIC_MYIP' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFS_BOOTSTATIC_MYIP
 .global _KERNEL_OPT_NFS_BOOTSTATIC_MYIP
 .equiv _KERNEL_OPT_NFS_BOOTSTATIC_MYIP,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFS_BOOTSTATIC_MYIP\n .global _KERNEL_OPT_NFS_BOOTSTATIC_MYIP\n .equiv _KERNEL_OPT_NFS_BOOTSTATIC_MYIP,0x6e074def\n .endif");
#endif
/* option `NFS_BOOT_RWSIZE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFS_BOOT_RWSIZE
 .global _KERNEL_OPT_NFS_BOOT_RWSIZE
 .equiv _KERNEL_OPT_NFS_BOOT_RWSIZE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFS_BOOT_RWSIZE\n .global _KERNEL_OPT_NFS_BOOT_RWSIZE\n .equiv _KERNEL_OPT_NFS_BOOT_RWSIZE,0x6e074def\n .endif");
#endif
/* option `NFS_BOOT_OPTIONS' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFS_BOOT_OPTIONS
 .global _KERNEL_OPT_NFS_BOOT_OPTIONS
 .equiv _KERNEL_OPT_NFS_BOOT_OPTIONS,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFS_BOOT_OPTIONS\n .global _KERNEL_OPT_NFS_BOOT_OPTIONS\n .equiv _KERNEL_OPT_NFS_BOOT_OPTIONS,0x6e074def\n .endif");
#endif
/* option `NFS_BOOT_BOOTP_REQFILE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_NFS_BOOT_BOOTP_REQFILE
 .global _KERNEL_OPT_NFS_BOOT_BOOTP_REQFILE
 .equiv _KERNEL_OPT_NFS_BOOT_BOOTP_REQFILE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_NFS_BOOT_BOOTP_REQFILE\n .global _KERNEL_OPT_NFS_BOOT_BOOTP_REQFILE\n .equiv _KERNEL_OPT_NFS_BOOT_BOOTP_REQFILE,0x6e074def\n .endif");
#endif
