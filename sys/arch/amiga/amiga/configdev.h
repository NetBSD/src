/* this is the information needed by BSD that is passed into the
 * kernel by loadbsd. 
 *
 *	$Id: configdev.h,v 1.2 1994/02/11 06:59:31 chopps Exp $
 */

struct ExpansionRom {
  u_char	er_Type;
  u_char	er_Product;
  u_char	er_Flags;
  u_char	er_pad0;
  u_short	er_Manufacturer;
  u_long	er_SerialNumber;
  u_short	er_InitDiagVec;
  u_long	er_notused;
};

struct ConfigDev {
  u_char	cd_Node[14];	/* really struct Node, but we don't need it */
  u_char	cd_Flags;
  u_char	cd_pad0;
  struct ExpansionRom cd_Rom;
  caddr_t	cd_BoardAddr;
  u_long	cd_BoardSize;
  u_char	cd_notused[2+2+4+4+4*4];	/* fields not needed by BSD */
};
