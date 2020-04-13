if test "${soc}" = "tegra210" ; then
	# enable PCIe
	pci enum
fi

setenv boot_scripts
setenv boot_script_dhcp
run distro_bootcmd
