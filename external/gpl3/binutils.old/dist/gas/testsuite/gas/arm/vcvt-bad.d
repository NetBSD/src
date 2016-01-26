#name: Invalid Immediate field for VCVT (between floating-point and fixed-point, VFP)  
#skip: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd
#error-output: vcvt-bad.l
#as: -mcpu=cortex-a8 -mfpu=vfpv3
