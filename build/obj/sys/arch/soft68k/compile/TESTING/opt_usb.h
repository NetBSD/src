/* option `USB_DEBUG' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_USB_DEBUG
 .global _KERNEL_OPT_USB_DEBUG
 .equiv _KERNEL_OPT_USB_DEBUG,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_USB_DEBUG\n .global _KERNEL_OPT_USB_DEBUG\n .equiv _KERNEL_OPT_USB_DEBUG,0x6e074def\n .endif");
#endif
/* option `USBHIST_PRINT' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_USBHIST_PRINT
 .global _KERNEL_OPT_USBHIST_PRINT
 .equiv _KERNEL_OPT_USBHIST_PRINT,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_USBHIST_PRINT\n .global _KERNEL_OPT_USBHIST_PRINT\n .equiv _KERNEL_OPT_USBHIST_PRINT,0x6e074def\n .endif");
#endif
/* option `USBHIST_SIZE' not defined */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_USBHIST_SIZE
 .global _KERNEL_OPT_USBHIST_SIZE
 .equiv _KERNEL_OPT_USBHIST_SIZE,0x6e074def
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_USBHIST_SIZE\n .global _KERNEL_OPT_USBHIST_SIZE\n .equiv _KERNEL_OPT_USBHIST_SIZE,0x6e074def\n .endif");
#endif
