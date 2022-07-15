#	$NetBSD: Makefile,v 1.20 2022/07/15 04:18:55 mrg Exp $

.include <bsd.own.mk>

XORGSRC=	${X11SRCDIR.xorg-server}

XORGINC_SUBDIR=	Xext composite dbe dri3 exa fb glamor glx include mi \
		miext/damage miext/shadow miext/sync \
		present randr render xkb record
XF86_SUBDIR=	common ddc dri fbdevhw i2c int10 modes \
		os-support os-support/bus parser rac ramdac shadowfb \
		vbe vgahw xf1bpp xf4bpp xf8_16bpp xf8_32bpp dri2

.for _dir in ${XORGINC_SUBDIR}
.PATH: ${XORGSRC}/${_dir}
.endfor

.for _dir in ${XF86_SUBDIR}
.PATH: ${XORGSRC}/hw/xfree86/${_dir}
.endfor

.PATH: ${XORGSRC}/../include

INCSDIR=${X11INCDIR}/xorg
INCS=	XIstubs.h Xprintf.h \
	callback.h client.h closestr.h closure.h colormap.h \
	colormapst.h compiler.h compositeext.h cursor.h cursorstr.h \
	damage.h damagestr.h \
	dbestruct.h dgaproc.h displaymode.h \
	dix.h dixaccess.h dixevents.h dixfont.h dixfontstr.h \
	dixgrabs.h dixstruct.h dri.h dristruct.h \
	dri3.h \
	edid.h exa.h exevents.h \
	eventconvert.h events.h eventstr.h \
	extension.h extinit.h extnsionst.h \
	fb.h fbdevhw.h fboverlay.h \
	fbpict.h fbrop.h fourcc.h gc.h gcstruct.h geext.h glamor.h globals.h \
	glyphstr.h glx_extinit.h glxvndabi.h \
	i2c_def.h input.h inputstr.h inpututils.h \
	list.h \
	mi.h micmap.h micoord.h \
	migc.h miline.h mipict.h mipointer.h mipointrst.h misc.h miscstruct.h \
	mistruct.h mizerarc.h opaque.h \
	misync.h misyncstr.h misyncshm.h misyncfd.h \
	nonsdk_extinit.h optionstr.h \
	os.h picture.h picturestr.h pixmap.h pixmapstr.h \
	present.h presentext.h \
	privates.h \
	property.h propertyst.h protocol-versions.h ptrveloc.h \
	randrstr.h region.h regionstr.h registry.h resource.h rgb.h \
	sarea.h screenint.h scrnintstr.h \
	securitysrv.h selection.h servermd.h \
	shadow.h shadowfb.h shmint.h \
	validate.h vbe.h vbeModes.h vgaHW.h \
	version-config.h \
	vndserver.h \
	window.h windowstr.h \
	xaarop.h \
	xkbDflts.h xkb.h set.h \
	xf86.h xf86platformBus.h \
	xf86Crtc.h xf86Cursor.h xf86DDC.h \
	xf86MatchDrivers.h \
	xf86Modes.h xf86Module.h xf86Optionstr.h xf86Opt.h xf86Optrec.h \
	xf86Parser.h xf86Pci.h xf86PciInfo.h \
	xf86Priv.h xf86Privstr.h xf86RandR12.h \
	xf86Xinput.h xf86_OSlib.h \
	xf86_OSproc.h xf86cmap.h xf86fbman.h \
	xf86i2c.h xf86int10.h xf86sbusBus.h xf86str.h xf86xv.h \
	xkbstr.h xkbrules.h xserver-properties.h \
	xkbfile.h \
	xf86xvmc.h xf86xvpriv.h xisb.h xorg-server.h xorgVersion.h xvdix.h \
	xvmcext.h hotplug.h xkbsrv.h dri2.h rrtransform.h \
	xsha1.h \
	xserver_poll.h  \

.include <bsd.x11.mk>
.include <bsd.prog.mk>
