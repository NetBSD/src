#	$NetBSD: Makefile,v 1.3.12.1 2009/06/09 17:26:27 snj Exp $

DRIVER=		xf86-video-s3
DRIVER_NAME=	s3_drv

SRCS=		s3_bios.c s3_cursor.c s3_dga.c s3_driver.c s3_IBMRGB.c \        
		s3_Ti.c s3_Trio64DAC.c s3_video.c s3_accel.c \                  
		s3_accel_newmmio.c                                              
                                                                               
# PLEASE NOTE: s3_accel.c is compiled two times with different                 
# cpp macros for PIO and NEWMMIO support. Check Makefile.am                    
# before upgrading the driver.                                                 
#                                                                              
BUILDSYMLINKS=	s3_accel.c s3_accel_newmmio.c                                   
CPPFLAGS.s3_accel_newmmio.c+=	-DS3_NEWMMIO=1 \                                
				-I${X11SRCDIR.xf86-video-s3}/src                

CPPFLAGS.s3_driver.c=	-Wno-error	# XXX fix me please
.include "../Makefile.xf86-driver"
