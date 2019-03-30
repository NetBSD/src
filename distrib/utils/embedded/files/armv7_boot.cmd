if test "${board}" = "am335x" ; then
	setenv kernel netbsd-BEAGLEBONE.ub
	setenv mmcpart 0:1
	setenv bootargs root=ld0a
else
	setenv use_efi 1
fi

if test "${soc}" = "tegra210" ; then
	# enable PCIe
	pci enum
fi

if test "${use_efi}" = "1" ; then
	setenv boot_scripts
	setenv boot_script_dhcp
	run distro_bootcmd
else
	fatload mmc ${mmcpart} ${kernel_addr_r} ${kernel}
	bootm ${kernel_addr_r} ${bootargs}
fi
