# first stage lego mindstorms ev3
if test "${hostname}" = "EV3" ; then
	setenv ubootaddr 0xC10C0000
	mmc read 0 ${ubootaddr} 0x10 0x400
	go ${ubootaddr}
fi
# second stage lego mindstorms ev3
if test "${fdtfile}" = "da850-lego-ev3.dtb" ; then
	setenv bootargs 'root=ld0a'
	fatload mmc 0 0xc2000000 netbsd-GENERIC_V5.ub
    fatload mmc 0 0xc1000000 dtb/ti/davinci/da850-lego-ev3.dtb
    bootm 0xc2000000 - 0xc1000000
fi