/* option `I2C_SCAN' is obsolete */
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_I2C_SCAN
 .global _KERNEL_OPT_I2C_SCAN
 .equiv _KERNEL_OPT_I2C_SCAN,0xdeadbeef
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_I2C_SCAN\n .global _KERNEL_OPT_I2C_SCAN\n .equiv _KERNEL_OPT_I2C_SCAN,0xdeadbeef\n .endif");
#endif
