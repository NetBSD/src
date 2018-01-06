if test "${soc}" = "sunxi" ; then
	setenv kernel netbsd-SUNXI.ub
	setenv bootargs 'root=ld0a'
	setenv mmcpart 0:1
	setenv use_fdt 1
fi
if test "${soc}" = "tegra" ; then
	setenv kernel netbsd-TEGRA.ub
	setenv bootargs root=ld1a
	setenv mmcpart 1:1
	setenv use_fdt 1
fi
if test "${soc}" = "tegra210" ; then
	setenv kernel netbsd-TEGRA.ub
	setenv bootargs root=ld0a
	setenv mmcpart 1:1
	setenv use_fdt 1
	setenv fdtfile ${soc}-${board}.dtb
	# enable PCIe
	pci enum
fi
if test "${board}" = "am335x" ; then
	setenv kernel netbsd-BEAGLEBONE.ub
	setenv mmcpart 0:1
	setenv bootargs root=ld0a
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
