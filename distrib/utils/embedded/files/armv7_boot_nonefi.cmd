if test "${board}" = "am335x" ; then
	setenv kernel netbsd.ub
	setenv mmcpart 0:1
	setenv bootargs root=wd0a
fi
if test "${board}" = "de0-nano-soc" ; then
	setenv kernel netbsd.ub
	setenv bootargs 'root=wd0a'
	setenv mmcpart 0:1
	setenv use_fdt 1
fi
if test "${soc}" = "exynos" ; then
	setenv kernel netbsd.ub
	setenv bootargs 'root=wd0a'
	setenv mmcpart 2:1
	setenv use_fdt 1
fi
if test "${soc}" = "sunxi" ; then
	setenv kernel netbsd.ub
	setenv bootargs 'root=wd0a'
	setenv mmcpart 0:1
	setenv use_fdt 1
fi
if test "${soc}" = "tegra" ; then
	setenv kernel netbsd.ub
	setenv bootargs root=wd0a
	setenv mmcpart 1:1
	setenv use_fdt 1
fi
if test "${soc}" = "tegra124" ; then
	setenv kernel netbsd.ub
	setenv bootargs root=wd0a
	setenv mmcpart 1:1
	setenv use_fdt 1
fi

if test "${kernel}" = "" ; then
	echo '>>>'
	echo '>>> Target device is not supported by this script.'
	echo '>>>'
	exit
fi

if test "${use_fdt}" = "1" ; then
	fatload mmc ${mmcpart} ${kernel_addr_r} ${kernel}
	fatload mmc ${mmcpart} ${fdt_addr_r} ${fdtfile}
	fdt addr ${fdt_addr_r}
	bootm ${kernel_addr_r} - ${fdt_addr_r}
else
	fatload mmc ${mmcpart} ${kernel_addr_r} ${kernel}
	bootm ${kernel_addr_r} ${bootargs}
fi
