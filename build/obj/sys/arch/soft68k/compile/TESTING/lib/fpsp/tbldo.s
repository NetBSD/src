|	$NetBSD: tbldo.sa,v 1.2 1994/10/26 07:50:18 cgd Exp $

|	MOTOROLA MICROPROCESSOR & MEMORY TECHNOLOGY GROUP
|	M68000 Hi-Performance Microprocessor Division
|	M68040 Software Package 
|
|	M68040 Software Package Copyright (c) 1993, 1994 Motorola Inc.
|	All rights reserved.
|
|	THE SOFTWARE is provided on an "AS IS" basis and without warranty.
|	To the maximum extent permitted by applicable law,
|	MOTOROLA DISCLAIMS ALL WARRANTIES WHETHER EXPRESS OR IMPLIED,
|	INCLUDING IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A
|	PARTICULAR PURPOSE and any warranty against infringement with
|	regard to the SOFTWARE (INCLUDING ANY MODIFIED VERSIONS THEREOF)
|	and any accompanying written materials. 
|
|	To the maximum extent permitted by applicable law,
|	IN NO EVENT SHALL MOTOROLA BE LIABLE FOR ANY DAMAGES WHATSOEVER
|	(INCLUDING WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS
|	PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR
|	OTHER PECUNIARY LOSS) ARISING OF THE USE OR INABILITY TO USE THE
|	SOFTWARE.  Motorola assumes no responsibility for the maintenance
|	and support of the SOFTWARE.  
|
|	You are hereby granted a copyright license to use, modify, and
|	distribute the SOFTWARE so long as this entire notice is retained
|	without alteration in any modified and/or redistributed versions,
|	and that such modified versions are clearly identified as such.
|	No licenses are granted by implication, estoppel or otherwise
|	under any patents or trademarks of Motorola, Inc.

|
|	tbldo.sa 3.1 12/10/90
|
| Modified:
|	8/16/90	chinds	The table was constructed to use only one level
|			of indirection in do_func for monoadic
|			functions.  Dyadic functions require two
|			levels, and the tables are still contained
|			in do_func.  The table is arranged for 
|			index with a 10-bit index, with the first
|			7 bits the opcode, and the remaining 3
|			the stag.  For dyadic functions, all
|			valid addresses are to the generic entry
|			point. 
|

|TBLDO	IDNT    2,1 Motorola 040 Floating Point Software Package

	.text

|	xref	ld_pinf,ld_pone,ld_ppi2
|	xref	t_dz2,t_operr
|	xref	serror,sone,szero,sinf,snzrinx
|	xref	sopr_inf,spi_2,src_nan,szr_inf

|	xref	smovcr
|	xref	pmod,prem,pscale
|	xref	satanh,satanhd
|	xref	sacos,sacosd,sasin,sasind,satan,satand
|	xref	setox,setoxd,setoxm1,setoxm1d,setoxm1i
|	xref	sgetexp,sgetexpd,sgetman,sgetmand
|	xref	sint,sintd,sintrz
|	xref	ssincos,ssincosd,ssincosi,ssincosnan,ssincosz
|	xref	scos,scosd,ssin,ssind,stan,stand
|	xref	scosh,scoshd,ssinh,ssinhd,stanh,stanhd
|	xref	sslog10,sslog2,sslogn,sslognp1
|	xref	sslog10d,sslog2d,sslognd,slognp1d
|	xref	stentox,stentoxd,stwotox,stwotoxd

|	instruction		;opcode-stag Notes
	.global	tblpre
tblpre:
	.long	smovcr		|0x00-0 fmovecr all
	.long	smovcr		|0x00-1 fmovecr all
	.long	smovcr		|0x00-2 fmovecr all
	.long	smovcr		|0x00-3 fmovecr all
	.long	smovcr		|0x00-4 fmovecr all
	.long	smovcr		|0x00-5 fmovecr all
	.long	smovcr		|0x00-6 fmovecr all
	.long	smovcr		|0x00-7 fmovecr all

	.long	sint		|0x01-0 fint norm
	.long	szero		|0x01-1 fint zero 
	.long	sinf		|0x01-2 fint inf
	.long	src_nan		|0x01-3 fint nan
	.long	sintd		|0x01-4 fint denorm inx
	.long	serror		|0x01-5 fint ERROR
	.long	serror		|0x01-6 fint ERROR
	.long	serror		|0x01-7 fint ERROR

	.long	ssinh		|0x02-0 fsinh norm
	.long	szero		|0x02-1 fsinh zero
	.long	sinf		|0x02-2 fsinh inf
	.long	src_nan		|0x02-3 fsinh nan
	.long	ssinhd		|0x02-4 fsinh denorm
	.long	serror		|0x02-5 fsinh ERROR
	.long	serror		|0x02-6 fsinh ERROR
	.long	serror		|0x02-7 fsinh ERROR

	.long	sintrz		|0x03-0 fintrz norm
	.long	szero		|0x03-1 fintrz zero
	.long	sinf		|0x03-2 fintrz inf
	.long	src_nan		|0x03-3 fintrz nan
	.long	snzrinx		|0x03-4 fintrz denorm inx
	.long	serror		|0x03-5 fintrz ERROR
	.long	serror		|0x03-6 fintrz ERROR
	.long	serror		|0x03-7 fintrz ERROR

	.long	serror		|0x04-0 ERROR - illegal extension
	.long	serror		|0x04-1 ERROR - illegal extension
	.long	serror		|0x04-2 ERROR - illegal extension
	.long	serror		|0x04-3 ERROR - illegal extension
	.long	serror		|0x04-4 ERROR - illegal extension
	.long	serror		|0x04-5 ERROR - illegal extension
	.long	serror		|0x04-6 ERROR - illegal extension
	.long	serror		|0x04-7 ERROR - illegal extension

	.long	serror		|0x05-0 ERROR - illegal extension
	.long	serror		|0x05-1 ERROR - illegal extension
	.long	serror		|0x05-2 ERROR - illegal extension
	.long	serror		|0x05-3 ERROR - illegal extension
	.long	serror		|0x05-4 ERROR - illegal extension
	.long	serror		|0x05-5 ERROR - illegal extension
	.long	serror		|0x05-6 ERROR - illegal extension
	.long	serror		|0x05-7 ERROR - illegal extension

	.long	sslognp1		|0x06-0 flognp1 norm
	.long	szero		|0x06-1 flognp1 zero
	.long	sopr_inf		|0x06-2 flognp1 inf
	.long	src_nan		|0x06-3 flognp1 nan
	.long	slognp1d		|0x06-4 flognp1 denorm
	.long	serror		|0x06-5 flognp1 ERROR
	.long	serror		|0x06-6 flognp1 ERROR
	.long	serror		|0x06-7 flognp1 ERROR

	.long	serror		|0x07-0 ERROR - illegal extension
	.long	serror		|0x07-1 ERROR - illegal extension
	.long	serror		|0x07-2 ERROR - illegal extension
	.long	serror		|0x07-3 ERROR - illegal extension
	.long	serror		|0x07-4 ERROR - illegal extension
	.long	serror		|0x07-5 ERROR - illegal extension
	.long	serror		|0x07-6 ERROR - illegal extension
	.long	serror		|0x07-7 ERROR - illegal extension

	.long	setoxm1		|0x08-0 fetoxm1 norm
	.long	szero		|0x08-1 fetoxm1 zero
	.long	setoxm1i		|0x08-2 fetoxm1 inf
	.long	src_nan		|0x08-3 fetoxm1 nan
	.long	setoxm1d		|0x08-4 fetoxm1 denorm
	.long	serror		|0x08-5 fetoxm1 ERROR
	.long	serror		|0x08-6 fetoxm1 ERROR
	.long	serror		|0x08-7 fetoxm1 ERROR

	.long	stanh		|0x09-0 ftanh norm
	.long	szero		|0x09-1 ftanh zero
	.long	sone		|0x09-2 ftanh inf
	.long	src_nan		|0x09-3 ftanh nan
	.long	stanhd		|0x09-4 ftanh denorm
	.long	serror		|0x09-5 ftanh ERROR
	.long	serror		|0x09-6 ftanh ERROR
	.long	serror		|0x09-7 ftanh ERROR

	.long	satan		|0x0a-0 fatan norm
	.long	szero		|0x0a-1 fatan zero
	.long	spi_2		|0x0a-2 fatan inf
	.long	src_nan		|0x0a-3 fatan nan
	.long	satand		|0x0a-4 fatan denorm
	.long	serror		|0x0a-5 fatan ERROR
	.long	serror		|0x0a-6 fatan ERROR
	.long	serror		|0x0a-7 fatan ERROR

	.long	serror		|0x0b-0 ERROR - illegal extension
	.long	serror		|0x0b-1 ERROR - illegal extension
	.long	serror		|0x0b-2 ERROR - illegal extension
	.long	serror		|0x0b-3 ERROR - illegal extension
	.long	serror		|0x0b-4 ERROR - illegal extension
	.long	serror		|0x0b-5 ERROR - illegal extension
	.long	serror		|0x0b-6 ERROR - illegal extension
	.long	serror		|0x0b-7 ERROR - illegal extension

	.long	sasin		|0x0c-0 fasin norm
	.long	szero		|0x0c-1 fasin zero
	.long	t_operr		|0x0c-2 fasin inf
	.long	src_nan		|0x0c-3 fasin nan
	.long	sasind		|0x0c-4 fasin denorm
	.long	serror		|0x0c-5 fasin ERROR
	.long	serror		|0x0c-6 fasin ERROR
	.long	serror		|0x0c-7 fasin ERROR

	.long	satanh		|0x0d-0 fatanh norm
	.long	szero		|0x0d-1 fatanh zero
	.long	t_operr		|0x0d-2 fatanh inf
	.long	src_nan		|0x0d-3 fatanh nan
	.long	satanhd		|0x0d-4 fatanh denorm
	.long	serror		|0x0d-5 fatanh ERROR
	.long	serror		|0x0d-6 fatanh ERROR
	.long	serror		|0x0d-7 fatanh ERROR

	.long	ssin		|0x0e-0 fsin norm
	.long	szero		|0x0e-1 fsin zero
	.long	t_operr		|0x0e-2 fsin inf
	.long	src_nan		|0x0e-3 fsin nan
	.long	ssind		|0x0e-4 fsin denorm
	.long	serror		|0x0e-5 fsin ERROR
	.long	serror		|0x0e-6 fsin ERROR
	.long	serror		|0x0e-7 fsin ERROR

	.long	stan		|0x0f-0 ftan norm
	.long	szero		|0x0f-1 ftan zero
	.long	t_operr		|0x0f-2 ftan inf
	.long	src_nan		|0x0f-3 ftan nan
	.long	stand		|0x0f-4 ftan denorm
	.long	serror		|0x0f-5 ftan ERROR
	.long	serror		|0x0f-6 ftan ERROR
	.long	serror		|0x0f-7 ftan ERROR

	.long	setox		|0x10-0 fetox norm
	.long	ld_pone		|0x10-1 fetox zero
	.long	szr_inf		|0x10-2 fetox inf
	.long	src_nan		|0x10-3 fetox nan
	.long	setoxd		|0x10-4 fetox denorm
	.long	serror		|0x10-5 fetox ERROR
	.long	serror		|0x10-6 fetox ERROR
	.long	serror		|0x10-7 fetox ERROR

	.long	stwotox		|0x11-0 ftwotox norm
	.long	ld_pone		|0x11-1 ftwotox zero
	.long	szr_inf		|0x11-2 ftwotox inf
	.long	src_nan		|0x11-3 ftwotox nan
	.long	stwotoxd		|0x11-4 ftwotox denorm
	.long	serror		|0x11-5 ftwotox ERROR
	.long	serror		|0x11-6 ftwotox ERROR
	.long	serror		|0x11-7 ftwotox ERROR

	.long	stentox		|0x12-0 ftentox norm
	.long	ld_pone		|0x12-1 ftentox zero
	.long	szr_inf		|0x12-2 ftentox inf
	.long	src_nan		|0x12-3 ftentox nan
	.long	stentoxd		|0x12-4 ftentox denorm
	.long	serror		|0x12-5 ftentox ERROR
	.long	serror		|0x12-6 ftentox ERROR
	.long	serror		|0x12-7 ftentox ERROR

	.long	serror		|0x13-0 ERROR - illegal extension
	.long	serror		|0x13-1 ERROR - illegal extension
	.long	serror		|0x13-2 ERROR - illegal extension
	.long	serror		|0x13-3 ERROR - illegal extension
	.long	serror		|0x13-4 ERROR - illegal extension
	.long	serror		|0x13-5 ERROR - illegal extension
	.long	serror		|0x13-6 ERROR - illegal extension
	.long	serror		|0x13-7 ERROR - illegal extension

	.long	sslogn		|0x14-0 flogn norm
	.long	t_dz2		|0x14-1 flogn zero
	.long	sopr_inf		|0x14-2 flogn inf
	.long	src_nan		|0x14-3 flogn nan
	.long	sslognd		|0x14-4 flogn denorm
	.long	serror		|0x14-5 flogn ERROR
	.long	serror		|0x14-6 flogn ERROR
	.long	serror		|0x14-7 flogn ERROR

	.long	sslog10		|0x15-0 flog10 norm
	.long	t_dz2		|0x15-1 flog10 zero
	.long	sopr_inf		|0x15-2 flog10 inf
	.long	src_nan		|0x15-3 flog10 nan
	.long	sslog10d		|0x15-4 flog10 denorm
	.long	serror		|0x15-5 flog10 ERROR
	.long	serror		|0x15-6 flog10 ERROR
	.long	serror		|0x15-7 flog10 ERROR

	.long	sslog2		|0x16-0 flog2 norm
	.long	t_dz2		|0x16-1 flog2 zero
	.long	sopr_inf		|0x16-2 flog2 inf
	.long	src_nan		|0x16-3 flog2 nan
	.long	sslog2d		|0x16-4 flog2 denorm
	.long	serror		|0x16-5 flog2 ERROR
	.long	serror		|0x16-6 flog2 ERROR
	.long	serror		|0x16-7 flog2 ERROR

	.long	serror		|0x17-0 ERROR - illegal extension
	.long	serror		|0x17-1 ERROR - illegal extension
	.long	serror		|0x17-2 ERROR - illegal extension
	.long	serror		|0x17-3 ERROR - illegal extension
	.long	serror		|0x17-4 ERROR - illegal extension
	.long	serror		|0x17-5 ERROR - illegal extension
	.long	serror		|0x17-6 ERROR - illegal extension
	.long	serror		|0x17-7 ERROR - illegal extension

	.long	serror		|0x18-0 ERROR - illegal extension
	.long	serror		|0x18-1 ERROR - illegal extension
	.long	serror		|0x18-2 ERROR - illegal extension
	.long	serror		|0x18-3 ERROR - illegal extension
	.long	serror		|0x18-4 ERROR - illegal extension
	.long	serror		|0x18-5 ERROR - illegal extension
	.long	serror		|0x18-6 ERROR - illegal extension
	.long	serror		|0x18-7 ERROR - illegal extension

	.long	scosh		|0x19-0 fcosh norm
	.long	ld_pone		|0x19-1 fcosh zero
	.long	ld_pinf		|0x19-2 fcosh inf
	.long	src_nan		|0x19-3 fcosh nan
	.long	scoshd		|0x19-4 fcosh denorm
	.long	serror		|0x19-5 fcosh ERROR
	.long	serror		|0x19-6 fcosh ERROR
	.long	serror		|0x19-7 fcosh ERROR

	.long	serror		|0x1a-0 ERROR - illegal extension
	.long	serror		|0x1a-1 ERROR - illegal extension
	.long	serror		|0x1a-2 ERROR - illegal extension
	.long	serror		|0x1a-3 ERROR - illegal extension
	.long	serror		|0x1a-4 ERROR - illegal extension
	.long	serror		|0x1a-5 ERROR - illegal extension
	.long	serror		|0x1a-6 ERROR - illegal extension
	.long	serror		|0x1a-7 ERROR - illegal extension

	.long	serror		|0x1b-0 ERROR - illegal extension
	.long	serror		|0x1b-1 ERROR - illegal extension
	.long	serror		|0x1b-2 ERROR - illegal extension
	.long	serror		|0x1b-3 ERROR - illegal extension
	.long	serror		|0x1b-4 ERROR - illegal extension
	.long	serror		|0x1b-5 ERROR - illegal extension
	.long	serror		|0x1b-6 ERROR - illegal extension
	.long	serror		|0x1b-7 ERROR - illegal extension

	.long	sacos		|0x1c-0 facos norm
	.long	ld_ppi2		|0x1c-1 facos zero
	.long	t_operr		|0x1c-2 facos inf
	.long	src_nan		|0x1c-3 facos nan
	.long	sacosd		|0x1c-4 facos denorm
	.long	serror		|0x1c-5 facos ERROR
	.long	serror		|0x1c-6 facos ERROR
	.long	serror		|0x1c-7 facos ERROR

	.long	scos		|0x1d-0 fcos norm
	.long	ld_pone		|0x1d-1 fcos zero
	.long	t_operr		|0x1d-2 fcos inf
	.long	src_nan		|0x1d-3 fcos nan
	.long	scosd		|0x1d-4 fcos denorm
	.long	serror		|0x1d-5 fcos ERROR
	.long	serror		|0x1d-6 fcos ERROR
	.long	serror		|0x1d-7 fcos ERROR

	.long	sgetexp		|0x1e-0 fgetexp norm
	.long	szero		|0x1e-1 fgetexp zero
	.long	t_operr		|0x1e-2 fgetexp inf
	.long	src_nan		|0x1e-3 fgetexp nan
	.long	sgetexpd		|0x1e-4 fgetexp denorm
	.long	serror		|0x1e-5 fgetexp ERROR
	.long	serror		|0x1e-6 fgetexp ERROR
	.long	serror		|0x1e-7 fgetexp ERROR

	.long	sgetman		|0x1f-0 fgetman norm
	.long	szero		|0x1f-1 fgetman zero
	.long	t_operr		|0x1f-2 fgetman inf
	.long	src_nan		|0x1f-3 fgetman nan
	.long	sgetmand		|0x1f-4 fgetman denorm
	.long	serror		|0x1f-5 fgetman ERROR
	.long	serror		|0x1f-6 fgetman ERROR
	.long	serror		|0x1f-7 fgetman ERROR

	.long	serror		|0x20-0 ERROR - illegal extension
	.long	serror		|0x20-1 ERROR - illegal extension
	.long	serror		|0x20-2 ERROR - illegal extension
	.long	serror		|0x20-3 ERROR - illegal extension
	.long	serror		|0x20-4 ERROR - illegal extension
	.long	serror		|0x20-5 ERROR - illegal extension
	.long	serror		|0x20-6 ERROR - illegal extension
	.long	serror		|0x20-7 ERROR - illegal extension

	.long	pmod		|0x21-0 fmod all
	.long	pmod		|0x21-1 fmod all
	.long	pmod		|0x21-2 fmod all
	.long	pmod		|0x21-3 fmod all
	.long	pmod		|0x21-4 fmod all
	.long	serror		|0x21-5 fmod ERROR
	.long	serror		|0x21-6 fmod ERROR
	.long	serror		|0x21-7 fmod ERROR

	.long	serror		|0x22-0 ERROR - illegal extension
	.long	serror		|0x22-1 ERROR - illegal extension
	.long	serror		|0x22-2 ERROR - illegal extension
	.long	serror		|0x22-3 ERROR - illegal extension
	.long	serror		|0x22-4 ERROR - illegal extension
	.long	serror		|0x22-5 ERROR - illegal extension
	.long	serror		|0x22-6 ERROR - illegal extension
	.long	serror		|0x22-7 ERROR - illegal extension

	.long	serror		|0x23-0 ERROR - illegal extension
	.long	serror		|0x23-1 ERROR - illegal extension
	.long	serror		|0x23-2 ERROR - illegal extension
	.long	serror		|0x23-3 ERROR - illegal extension
	.long	serror		|0x23-4 ERROR - illegal extension
	.long	serror		|0x23-5 ERROR - illegal extension
	.long	serror		|0x23-6 ERROR - illegal extension
	.long	serror		|0x23-7 ERROR - illegal extension

	.long	serror		|0x24-0 ERROR - illegal extension
	.long	serror		|0x24-1 ERROR - illegal extension
	.long	serror		|0x24-2 ERROR - illegal extension
	.long	serror		|0x24-3 ERROR - illegal extension
	.long	serror		|0x24-4 ERROR - illegal extension
	.long	serror		|0x24-5 ERROR - illegal extension
	.long	serror		|0x24-6 ERROR - illegal extension
	.long	serror		|0x24-7 ERROR - illegal extension

	.long	prem		|0x25-0 frem all
	.long	prem		|0x25-1 frem all
	.long	prem		|0x25-2 frem all
	.long	prem		|0x25-3 frem all
	.long	prem		|0x25-4 frem all
	.long	serror		|0x25-5 frem ERROR
	.long	serror		|0x25-6 frem ERROR
	.long	serror		|0x25-7 frem ERROR

	.long	pscale		|0x26-0 fscale all
	.long	pscale		|0x26-1 fscale all
	.long	pscale		|0x26-2 fscale all
	.long	pscale		|0x26-3 fscale all
	.long	pscale		|0x26-4 fscale all
	.long	serror		|0x26-5 fscale ERROR
	.long	serror		|0x26-6 fscale ERROR
	.long	serror		|0x26-7 fscale ERROR

	.long	serror		|0x27-0 ERROR - illegal extension
	.long	serror		|0x27-1 ERROR - illegal extension
	.long	serror		|0x27-2 ERROR - illegal extension
	.long	serror		|0x27-3 ERROR - illegal extension
	.long	serror		|0x27-4 ERROR - illegal extension
	.long	serror		|0x27-5 ERROR - illegal extension
	.long	serror		|0x27-6 ERROR - illegal extension
	.long	serror		|0x27-7 ERROR - illegal extension

	.long	serror		|0x28-0 ERROR - illegal extension
	.long	serror		|0x28-1 ERROR - illegal extension
	.long	serror		|0x28-2 ERROR - illegal extension
	.long	serror		|0x28-3 ERROR - illegal extension
	.long	serror		|0x28-4 ERROR - illegal extension
	.long	serror		|0x28-5 ERROR - illegal extension
	.long	serror		|0x28-6 ERROR - illegal extension
	.long	serror		|0x28-7 ERROR - illegal extension

	.long	serror		|0x29-0 ERROR - illegal extension
	.long	serror		|0x29-1 ERROR - illegal extension
	.long	serror		|0x29-2 ERROR - illegal extension
	.long	serror		|0x29-3 ERROR - illegal extension
	.long	serror		|0x29-4 ERROR - illegal extension
	.long	serror		|0x29-5 ERROR - illegal extension
	.long	serror		|0x29-6 ERROR - illegal extension
	.long	serror		|0x29-7 ERROR - illegal extension

	.long	serror		|0x2a-0 ERROR - illegal extension
	.long	serror		|0x2a-1 ERROR - illegal extension
	.long	serror		|0x2a-2 ERROR - illegal extension
	.long	serror		|0x2a-3 ERROR - illegal extension
	.long	serror		|0x2a-4 ERROR - illegal extension
	.long	serror		|0x2a-5 ERROR - illegal extension
	.long	serror		|0x2a-6 ERROR - illegal extension
	.long	serror		|0x2a-7 ERROR - illegal extension

	.long	serror		|0x2b-0 ERROR - illegal extension
	.long	serror		|0x2b-1 ERROR - illegal extension
	.long	serror		|0x2b-2 ERROR - illegal extension
	.long	serror		|0x2b-3 ERROR - illegal extension
	.long	serror		|0x2b-4 ERROR - illegal extension
	.long	serror		|0x2b-5 ERROR - illegal extension
	.long	serror		|0x2b-6 ERROR - illegal extension
	.long	serror		|0x2b-7 ERROR - illegal extension

	.long	serror		|0x2c-0 ERROR - illegal extension
	.long	serror		|0x2c-1 ERROR - illegal extension
	.long	serror		|0x2c-2 ERROR - illegal extension
	.long	serror		|0x2c-3 ERROR - illegal extension
	.long	serror		|0x2c-4 ERROR - illegal extension
	.long	serror		|0x2c-5 ERROR - illegal extension
	.long	serror		|0x2c-6 ERROR - illegal extension
	.long	serror		|0x2c-7 ERROR - illegal extension

	.long	serror		|0x2d-0 ERROR - illegal extension
	.long	serror		|0x2d-1 ERROR - illegal extension
	.long	serror		|0x2d-2 ERROR - illegal extension
	.long	serror		|0x2d-3 ERROR - illegal extension
	.long	serror		|0x2d-4 ERROR - illegal extension
	.long	serror		|0x2d-5 ERROR - illegal extension
	.long	serror		|0x2d-6 ERROR - illegal extension
	.long	serror		|0x2d-7 ERROR - illegal extension

	.long	serror		|0x2e-0 ERROR - illegal extension
	.long	serror		|0x2e-1 ERROR - illegal extension
	.long	serror		|0x2e-2 ERROR - illegal extension
	.long	serror		|0x2e-3 ERROR - illegal extension
	.long	serror		|0x2e-4 ERROR - illegal extension
	.long	serror		|0x2e-5 ERROR - illegal extension
	.long	serror		|0x2e-6 ERROR - illegal extension
	.long	serror		|0x2e-7 ERROR - illegal extension

	.long	serror		|0x2f-0 ERROR - illegal extension
	.long	serror		|0x2f-1 ERROR - illegal extension
	.long	serror		|0x2f-2 ERROR - illegal extension
	.long	serror		|0x2f-3 ERROR - illegal extension
	.long	serror		|0x2f-4 ERROR - illegal extension
	.long	serror		|0x2f-5 ERROR - illegal extension
	.long	serror		|0x2f-6 ERROR - illegal extension
	.long	serror		|0x2f-7 ERROR - illegal extension

	.long	ssincos		|0x30-0 fsincos norm
	.long	ssincosz		|0x30-1 fsincos zero
	.long	ssincosi		|0x30-2 fsincos inf
	.long	ssincosnan		|0x30-3 fsincos nan
	.long	ssincosd		|0x30-4 fsincos denorm
	.long	serror		|0x30-5 fsincos ERROR
	.long	serror		|0x30-6 fsincos ERROR
	.long	serror		|0x30-7 fsincos ERROR

	.long	ssincos		|0x31-0 fsincos norm
	.long	ssincosz		|0x31-1 fsincos zero
	.long	ssincosi		|0x31-2 fsincos inf
	.long	ssincosnan		|0x31-3 fsincos nan
	.long	ssincosd		|0x31-4 fsincos denorm
	.long	serror		|0x31-5 fsincos ERROR
	.long	serror		|0x31-6 fsincos ERROR
	.long	serror		|0x31-7 fsincos ERROR

	.long	ssincos		|0x32-0 fsincos norm
	.long	ssincosz		|0x32-1 fsincos zero
	.long	ssincosi		|0x32-2 fsincos inf
	.long	ssincosnan		|0x32-3 fsincos nan
	.long	ssincosd		|0x32-4 fsincos denorm
	.long	serror		|0x32-5 fsincos ERROR
	.long	serror		|0x32-6 fsincos ERROR
	.long	serror		|0x32-7 fsincos ERROR

	.long	ssincos		|0x33-0 fsincos norm
	.long	ssincosz		|0x33-1 fsincos zero
	.long	ssincosi		|0x33-2 fsincos inf
	.long	ssincosnan		|0x33-3 fsincos nan
	.long	ssincosd		|0x33-4 fsincos denorm
	.long	serror		|0x33-5 fsincos ERROR
	.long	serror		|0x33-6 fsincos ERROR
	.long	serror		|0x33-7 fsincos ERROR

	.long	ssincos		|0x34-0 fsincos norm
	.long	ssincosz		|0x34-1 fsincos zero
	.long	ssincosi		|0x34-2 fsincos inf
	.long	ssincosnan		|0x34-3 fsincos nan
	.long	ssincosd		|0x34-4 fsincos denorm
	.long	serror		|0x34-5 fsincos ERROR
	.long	serror		|0x34-6 fsincos ERROR
	.long	serror		|0x34-7 fsincos ERROR

	.long	ssincos		|0x35-0 fsincos norm
	.long	ssincosz		|0x35-1 fsincos zero
	.long	ssincosi		|0x35-2 fsincos inf
	.long	ssincosnan		|0x35-3 fsincos nan
	.long	ssincosd		|0x35-4 fsincos denorm
	.long	serror		|0x35-5 fsincos ERROR
	.long	serror		|0x35-6 fsincos ERROR
	.long	serror		|0x35-7 fsincos ERROR

	.long	ssincos		|0x36-0 fsincos norm
	.long	ssincosz		|0x36-1 fsincos zero
	.long	ssincosi		|0x36-2 fsincos inf
	.long	ssincosnan		|0x36-3 fsincos nan
	.long	ssincosd		|0x36-4 fsincos denorm
	.long	serror		|0x36-5 fsincos ERROR
	.long	serror		|0x36-6 fsincos ERROR
	.long	serror		|0x36-7 fsincos ERROR

	.long	ssincos		|0x37-0 fsincos norm
	.long	ssincosz		|0x37-1 fsincos zero
	.long	ssincosi		|0x37-2 fsincos inf
	.long	ssincosnan		|0x37-3 fsincos nan
	.long	ssincosd		|0x37-4 fsincos denorm
	.long	serror		|0x37-5 fsincos ERROR
	.long	serror		|0x37-6 fsincos ERROR
	.long	serror		|0x37-7 fsincos ERROR

|	end
