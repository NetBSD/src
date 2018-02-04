/*
 * Copyright (c) 1997, 2000 Hellmuth Michaelis. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *---------------------------------------------------------------------------
 *
 *	q931_util.c - utility functions to print Q.931 traces
 *	-----------------------------------------------------
 *
 *	$Id: q931_util.c,v 1.4 2018/02/04 09:01:12 mrg Exp $ 
 *
 * $FreeBSD$
 *
 *      last edit-date: [Tue Feb 15 13:52:09 2000]
 *
 *---------------------------------------------------------------------------*/

#include "trace.h"

/*---------------------------------------------------------------------------*
 *	decode and print the cause
 *---------------------------------------------------------------------------*/
int
p_q931cause(char *pbuf, unsigned char *buf)
{
	int j;
	int len;
	int i = 0;
	int ls;
	int r = 0;
	int rflag = 0;
	
	i++;	/* index -> length */

	len = buf[i];

	i++;	/* coding/location */
	len--;

	ls = buf[i];
	
	i++;
	len--;
	
	if (!(buf[i-1] & 0x80))
	{
		r = buf[i];		
		rflag = 1;
		i++;
		len--;
	}

	sprintf((pbuf+strlen(pbuf)), "%s ", print_cause_q850(buf[i] & 0x7f));

	sprintf((pbuf+strlen(pbuf)), "\n          (location=");
	
	switch (ls & 0x0f)
	{
	case 0x00:
		sprintf((pbuf+strlen(pbuf)), "user");
		break;
	case 0x01:
		sprintf((pbuf+strlen(pbuf)), "private network serving local user");
		break;
	case 0x02:
		sprintf((pbuf+strlen(pbuf)), "public network serving local user");
		break;
	case 0x03:
		sprintf((pbuf+strlen(pbuf)), "transit network");
		break;
	case 0x04:
		sprintf((pbuf+strlen(pbuf)), "public network serving remote user");
		break;
	case 0x05:
		sprintf((pbuf+strlen(pbuf)), "private network serving remote user");
		break;
	case 0x07:
		sprintf((pbuf+strlen(pbuf)), "international network");
		break;
	case 0x0a:
		sprintf((pbuf+strlen(pbuf)), "network beyond interworking point");
		break;
	default:
		sprintf((pbuf+strlen(pbuf)), "reserved (0x%02x)", ls & 0x0f);
		break;
	}

	sprintf((pbuf+strlen(pbuf)), ", std=");

	switch ((ls & 0x60) >> 5)
	{
		case 0:
			sprintf((pbuf+strlen(pbuf)), "CCITT");
			break;
		case 1:
			sprintf((pbuf+strlen(pbuf)), "ISO/IEC");
			break;
		case 2:
			sprintf((pbuf+strlen(pbuf)), "National");
			break;
		case 3:
			sprintf((pbuf+strlen(pbuf)), "Local");
			break;
	}

	if (rflag)
	{
		sprintf((pbuf+strlen(pbuf)), ", rec=");

		switch (r & 0x7f)
		{
			case 0:
				sprintf((pbuf+strlen(pbuf)), "Q.931");
				break;
			case 3:
				sprintf((pbuf+strlen(pbuf)), "X.21");
				break;
			case 4:
				sprintf((pbuf+strlen(pbuf)), "X.25");
				break;
			case 5:
				sprintf((pbuf+strlen(pbuf)), "Q.1031/Q.1051");
				break;
			default:
				sprintf((pbuf+strlen(pbuf)), "Reserved");
				break;
		}
	}

	sprintf((pbuf+strlen(pbuf)),")");
	
	i++;
	len--;
	
	for (j = 0; j < len; j++)
		sprintf((pbuf+strlen(pbuf))," 0x%02x", buf[j+i]);
	
	sprintf((pbuf+strlen(pbuf)),"]");

	i += (len+1);
	
	return(i);
}

/*---------------------------------------------------------------------------*
 *	decode and print the bearer capability
 *---------------------------------------------------------------------------*/
int
p_q931bc(char *pbuf, unsigned char *buf)
{
	int len;
	int i = 0;
	int mr = 0;
	
	i++;	/* index -> length */

	len = buf[i];

	i++;

	sprintf((pbuf+strlen(pbuf)), "\n          cap=");
	
	switch (buf[i] & 0x1f)
	{
		case 0x00:
			sprintf((pbuf+strlen(pbuf)), "speech");
			break;
		case 0x08:
			sprintf((pbuf+strlen(pbuf)), "unrestricted digital information");
			break;
		case 0x09:
			sprintf((pbuf+strlen(pbuf)), "restricted digital information");
			break;
		case 0x10:
			sprintf((pbuf+strlen(pbuf)), "3.1 kHz audio");
			break;
		case 0x11:
			sprintf((pbuf+strlen(pbuf)), "unrestricted digital information with tones");
			break;
		case 0x18:
			sprintf((pbuf+strlen(pbuf)), "video");
			break;
		default:
			sprintf((pbuf+strlen(pbuf)), "reserved (0x%02x)", buf[i] & 0x0f);
			break;
	}

	sprintf((pbuf+strlen(pbuf)), "\n          std=");

	switch ((buf[i] & 0x60) >> 5)
	{
		case 0:
			sprintf((pbuf+strlen(pbuf)), "CCITT");
			break;
		case 1:
			sprintf((pbuf+strlen(pbuf)), "ISO/IEC");
			break;
		case 2:
			sprintf((pbuf+strlen(pbuf)), "National");
			break;
		case 3:
			sprintf((pbuf+strlen(pbuf)), "NSI Std");
			break;
	}

	i++;
	len--;
	
	sprintf((pbuf+strlen(pbuf)), "\n          rate=");
	
	switch (buf[i] & 0x1f)
	{
		case 0x00:
			sprintf((pbuf+strlen(pbuf)), "packet mode");
			break;
		case 0x10:
			sprintf((pbuf+strlen(pbuf)), "64 kbit/s");
			break;
		case 0x11:
			sprintf((pbuf+strlen(pbuf)), "2 x 64 kbit/s");
			break;
		case 0x13:
			sprintf((pbuf+strlen(pbuf)), "384 kbit/s");
			break;
		case 0x15:
			sprintf((pbuf+strlen(pbuf)), "1536 kbit/s");
			break;
		case 0x17:
			sprintf((pbuf+strlen(pbuf)), "1920 kbit/s");
			break;
		case 0x18:
			sprintf((pbuf+strlen(pbuf)), "Multirate");
			mr = 1;
			break;
		default:
			sprintf((pbuf+strlen(pbuf)), "reserved (0x%02x)", buf[i] & 0x0f);
			break;
	}

	sprintf((pbuf+strlen(pbuf)), "\n          mode=");

	switch ((buf[i] & 0x60) >> 5)
	{
		case 0:
			sprintf((pbuf+strlen(pbuf)), "circuit");
			break;
		case 2:
			sprintf((pbuf+strlen(pbuf)), "packet");
			break;
		default:
			sprintf((pbuf+strlen(pbuf)), "reserved (0x%02x)", ((buf[i] & 0x60) >> 5));
			break;
	}

	i++;
	len--;

	if (!len)
		goto exit;
	
	if (mr)
	{
		sprintf((pbuf+strlen(pbuf)), "\n          rate multiplier=%d", buf[i] & 0x7f);
		i++;
		len--;
	}

	if (!len)
		goto exit;
	
			sprintf((pbuf+strlen(pbuf)), "\n          layer1=");
	
			switch (buf[i] & 0x1f)
			{
				case 0x01:
					sprintf((pbuf+strlen(pbuf)), "V.110");
					break;
				case 0x02:
					sprintf((pbuf+strlen(pbuf)), "G.711 mu-law");
					break;
				case 0x03:
					sprintf((pbuf+strlen(pbuf)), "G.711 A-law");
					break;
				case 0x04:
					sprintf((pbuf+strlen(pbuf)), "G.721");
					break;
				case 0x05:
					sprintf((pbuf+strlen(pbuf)), "H.221/H.242");
					break;
				case 0x07:
					sprintf((pbuf+strlen(pbuf)), "Non-Std");
					break;
				case 0x08:
					sprintf((pbuf+strlen(pbuf)), "V.120");
					break;
				case 0x09:
					sprintf((pbuf+strlen(pbuf)), "X.31");
					break;
				default:
					sprintf((pbuf+strlen(pbuf)), "reserved (0x%02x)", buf[i] & 0x0f);
					break;
			}
			i++;
			len--;
	
		if (!len)
			goto exit;

	if (!(buf[i-1] & 0x80))
		{
			sprintf((pbuf+strlen(pbuf)), "\n          user rate=0x%02x ", buf[i] & 0x1f);
	
			if (buf[i] & 0x40)
				sprintf((pbuf+strlen(pbuf)), "(async,");
			else
				sprintf((pbuf+strlen(pbuf)), "(sync,");		
	
			if (buf[i] & 0x20)
				sprintf((pbuf+strlen(pbuf)), "in-band neg. possible)");
			else
				sprintf((pbuf+strlen(pbuf)), "in-band neg not possible)");
			
			i++;
			len--;
		}
	
		if (!len)
			goto exit;

	if (!(buf[i-1] & 0x80))
		{
			sprintf((pbuf+strlen(pbuf)), "\n          clk/flow=0x%02x", buf[i] & 0x1f);
	
			sprintf((pbuf+strlen(pbuf)), "\n          intermediate rate=");
			
			switch ((buf[i] & 0x60) >> 5)
			{
				case 0:
					sprintf((pbuf+strlen(pbuf)), "not used");
					break;
				case 1:
					sprintf((pbuf+strlen(pbuf)), "8 kbit/s");
					break;
				case 2:
					sprintf((pbuf+strlen(pbuf)), "16 kbit/s");
					break;
				case 3:
					sprintf((pbuf+strlen(pbuf)), "32 kbit/s");
					break;
			}
			i++;
			len--;
		}
	
		if (!len)
			goto exit;

	if (!(buf[i-1] & 0x80))
		{
			sprintf((pbuf+strlen(pbuf)), "\n          hdr/mfrm/etc.=0x%02x", buf[i]);
			i++;
			len--;
		}
	
		if (!len)
			goto exit;

	if (!(buf[i-1] & 0x80))
		{
			sprintf((pbuf+strlen(pbuf)), "\n          stop/data/parity=0x%02x", buf[i]);
			i++;
			len--;
		}
	
		if (!len)
			goto exit;

	if (!(buf[i-1] & 0x80))
		{
			sprintf((pbuf+strlen(pbuf)), "\n          modemtype=0x%02x", buf[i]);
			i++;
			len--;
		}

	if (!len)
		goto exit;

	switch (buf[i] & 0x7f)
	{
		case 0x42:
			sprintf((pbuf+strlen(pbuf)), "\n          layer2=Q.921/I.441");
			break;
		case 0x46:
			sprintf((pbuf+strlen(pbuf)), "\n          layer2=X.25 link");
			break;
		default:
			sprintf((pbuf+strlen(pbuf)), "\n          layer2=0x%02x",(buf[i] & 0x7f));
			break;
	}
	i++;
	len--;

	if (!len)
		goto exit;
	
	switch (buf[i] & 0x7f)
	{
		case 0x62:
			sprintf((pbuf+strlen(pbuf)), "\n          layer3=Q.921/I.441");
			break;
		case 0x66:
			sprintf((pbuf+strlen(pbuf)), "\n          layer3=X.25 packet");
			break;
		default:
			sprintf((pbuf+strlen(pbuf)), "\n          layer3=0x%02x",(buf[i] & 0x7f));
			break;
	}
	i++;
	len--;

exit:	
	sprintf((pbuf+strlen(pbuf)), "]");	
	return(i);
}

/*---------------------------------------------------------------------------*
 *	decode and print the ISDN (telephone) number
 *---------------------------------------------------------------------------*/
int
p_q931address(char *pbuf, unsigned char *buf)
{
	int j;
	int len;
	int i = 0;
	int tp;
	int ind = 0;
	int indflag = 0;
	
	i++;	/* index -> length */
	len = buf[i];

	i++;	/* index -> type/plan */
	tp = buf[i];
	
	i++;
	len--;
	
	if (!(tp & 0x80))
	{
		ind = buf[i];		
		indflag = 1;
		i++;
		len--;
	}

	for (j = 0; j < len; j++)
	{
		sprintf((pbuf+strlen(pbuf)),"%c", buf[j+i]);
	}

	switch ((tp & 0x70) >> 4)
	{
		case 0:
			sprintf((pbuf+strlen(pbuf)), " (type=unknown, ");
			break;
		case 1:
			sprintf((pbuf+strlen(pbuf)), " (type=international, ");
			break;
		case 2:
			sprintf((pbuf+strlen(pbuf)), " (type=national, ");
			break;
		case 3:
			sprintf((pbuf+strlen(pbuf)), " (type=network specific, ");
			break;
		case 4:
			sprintf((pbuf+strlen(pbuf)), " (type=subscriber, ");
			break;
		case 6:
			sprintf((pbuf+strlen(pbuf)), " (type=abbreviated, ");
			break;
		default:
			sprintf((pbuf+strlen(pbuf)), " (type=reserved (%d), ", ((tp & 0x70) >> 4));
			break;
	}

	switch (tp & 0x0f)
	{
		case 0:
			sprintf((pbuf+strlen(pbuf)), "plan=unknown");
			break;
		case 1:
			sprintf((pbuf+strlen(pbuf)), "plan=ISDN");
			break;
		case 3:
			sprintf((pbuf+strlen(pbuf)), "plan=Data");
			break;
		case 4:
			sprintf((pbuf+strlen(pbuf)), "plan=Telex");
			break;
		case 8:
			sprintf((pbuf+strlen(pbuf)), "plan=National");
			break;
		case 9:
			sprintf((pbuf+strlen(pbuf)), "plan=private");
			break;
		default:
			sprintf((pbuf+strlen(pbuf)), "plan=reserved (%d)", (tp & 0x0f));
			break;
	}

	if (indflag)
	{
		sprintf((pbuf+strlen(pbuf)), ",\n          ");
		switch ((ind & 0x60) >> 5)
		{
			case 0:
				sprintf((pbuf+strlen(pbuf)), "presentation allowed, ");
				break;
			case 1:
				sprintf((pbuf+strlen(pbuf)), "presentation restricted, ");
				break;
			case 2:
				sprintf((pbuf+strlen(pbuf)), "number not available, ");
				break;
			case 3:
				sprintf((pbuf+strlen(pbuf)), "reserved, ");
				break;
		}

		switch (ind & 0x03)
		{
			case 0:
				sprintf((pbuf+strlen(pbuf)), "screening user provided: not screened");
				break;
			case 1:
				sprintf((pbuf+strlen(pbuf)), "screening user provided: verified & passed");
				break;
			case 2:
				sprintf((pbuf+strlen(pbuf)), "screening user provided: verified & failed");
				break;
			case 3:
				sprintf((pbuf+strlen(pbuf)), "screening network provided");
				break;
		}
	}		

	sprintf((pbuf+strlen(pbuf)),")]");

	i += j;

	return(i);
}

/*---------------------------------------------------------------------------*
 *	decode and print HL comatibility
 *---------------------------------------------------------------------------*/
int
p_q931high_compat(char *pbuf, unsigned char *buf)
{
	int len = buf[1];

	sprintf(pbuf+strlen(pbuf), " standard=");

	switch ((buf[2] >> 5) & 0x03)
	{
		case 0: sprintf(pbuf+strlen(pbuf), "CCITT");
			break;
		case 1: sprintf(pbuf+strlen(pbuf), "unknown international standard");
			break;
		case 2: sprintf(pbuf+strlen(pbuf), "unknown national standard");
			break;
		case 3: sprintf(pbuf+strlen(pbuf), "local network standard");
	}

	len--;

	sprintf(pbuf+strlen(pbuf), ", characteristics=");

	switch (buf[3] & 0x7f)
	{
		case 0x01:
			sprintf(pbuf+strlen(pbuf), "Telephony");
			break;
		case 0x04:
			sprintf(pbuf+strlen(pbuf), "Fax Group 2/3");
			break;
		case 0x21:
			sprintf(pbuf+strlen(pbuf), "Fax Group 4 Class I (F.184)");
			break;
		case 0x24:
			sprintf(pbuf+strlen(pbuf), "Teletex basic/mixed (F.230) or Fax Group 4 Class II/III (F.184)");
			break;
		case 0x28:
			sprintf(pbuf+strlen(pbuf), "Teletex basic/processable (F.220)");
			break;
		case 0x31:
			sprintf(pbuf+strlen(pbuf), "Teletex basic mode (F.200)");
			break;
		case 0x32:
			sprintf(pbuf+strlen(pbuf), "Videotex (F.300 and T.101)");
			break;
		case 0x35:
			sprintf(pbuf+strlen(pbuf), "Telex (F.60)");
			break;
		case 0x38:
			sprintf(pbuf+strlen(pbuf), "MHS (X.400 series)");
			break;
		case 0x41:
			sprintf(pbuf+strlen(pbuf), "OSI application (X.200 series)");
			break;
		case 0x5e:
			sprintf(pbuf+strlen(pbuf), "Maintenance");
			break;
		case 0x5f:
			sprintf(pbuf+strlen(pbuf), "Management");
			break;
		case 0x7f:
			sprintf(pbuf+strlen(pbuf), "reserved");
			break;
		default:
			sprintf(pbuf+strlen(pbuf), "UNKNOWN (0x%02x)", buf[3]);
			break;
	}

	len--;

	if (!len)
	{
		sprintf(pbuf+strlen(pbuf), "]");
		return 4;
	}
	
	sprintf(pbuf+strlen(pbuf), " of ");

	switch (buf[4] & 0x7f)
	{
		case 0x01:
			sprintf(pbuf+strlen(pbuf), "Telephony");
			break;
		case 0x04:
			sprintf(pbuf+strlen(pbuf), "Fax Group 2/3");
			break;
		case 0x21:
			sprintf(pbuf+strlen(pbuf), "Fax Group 4 Class I (F.184)");
			break;
		case 0x24:
			sprintf(pbuf+strlen(pbuf), "Teletex basic/mixed (F.230) or Fax Group 4 Class II/III (F.184)");
			break;
		case 0x28:
			sprintf(pbuf+strlen(pbuf), "Teletex basic/processable (F.220)");
			break;
		case 0x31:
			sprintf(pbuf+strlen(pbuf), "Teletex basic mode (F.200)");
			break;
		case 0x32:
			sprintf(pbuf+strlen(pbuf), "Videotex (F.300 and T.101)");
			break;
		case 0x35:
			sprintf(pbuf+strlen(pbuf), "Telex (F.60)");
			break;
		case 0x38:
			sprintf(pbuf+strlen(pbuf), "MHS (X.400 series)");
			break;
		case 0x41:
			sprintf(pbuf+strlen(pbuf), "OSI application (X.200 series)");
			break;
		case 0x7f:
			sprintf(pbuf+strlen(pbuf), "reserved");
			break;
		default:
			sprintf(pbuf+strlen(pbuf), "UNKNOWN (0x%02x)", buf[3]);
			break;
	}
	sprintf(pbuf+strlen(pbuf), "]");
	return 5;
}

/*---------------------------------------------------------------------------*
 *	decode and print user-user IE
 *---------------------------------------------------------------------------*/
int
p_q931user_user(char *pbuf, unsigned char *buf)
{
	int j;
	int len;
	int i = 0;
	int pd;
	
	i++;	/* index -> length */
	len = buf[i];

	i++;	/* index -> protocoldiscriminator */
	pd = buf[i];
	
	switch (pd)
	{
		case 0:
			sprintf((pbuf+strlen(pbuf)), "(pd=user-specific");
			break;
		case 1:
			sprintf((pbuf+strlen(pbuf)), "(pd=OSI high-layer protocols");
			break;
		case 2:
			sprintf((pbuf+strlen(pbuf)), "(pd=X.244");
			break;
		case 3:
			sprintf((pbuf+strlen(pbuf)), "(pd=reserved for system management");
			break;
		case 4:
			sprintf((pbuf+strlen(pbuf)), "(pd=IA5 characters");
			break;
		case 6:
			sprintf((pbuf+strlen(pbuf)), "(pd=X.208/X.209 coded user info");
			break;
		case 7:
			sprintf((pbuf+strlen(pbuf)), "(pd=V.120 rate adaption");
			break;
		case 8:
			sprintf((pbuf+strlen(pbuf)), "(pd=Q.931/I.451 user network call control messages");
			break;
		default:
			if (pd >= 0x10 && pd <= 0x3f)
				sprintf((pbuf+strlen(pbuf)), "(pd=0x%x=reserved for other L3 protocols incl. X.25", pd);
			else if (pd >= 0x40 && pd <= 0x47)
				sprintf((pbuf+strlen(pbuf)), "(pd=0x%x=national use", pd);
			else if (pd >= 0x48 && pd <= 0x4f)
				sprintf((pbuf+strlen(pbuf)), "(pd=0x%x=reserved for ETSI", pd);
			else if (pd >= 0x50 && pd <= 0xfe)
				sprintf((pbuf+strlen(pbuf)), "(pd=0x%x=reserved for other L3 protocols incl. X.25", pd);
			else
				sprintf((pbuf+strlen(pbuf)), "(pd=0x%x=reserved", pd);
			break;
	}

	i++;
	len--;

	sprintf((pbuf+strlen(pbuf)),": (");
	
	for (j = 0; j < len; j++)
	{
		if (pd == 4)
			sprintf((pbuf+strlen(pbuf)),"%c", buf[j+i]);
		else
			sprintf((pbuf+strlen(pbuf)),"0x%2x", buf[j+i]);
	}

	sprintf((pbuf+strlen(pbuf)),"))]");

	i += j;

	return(i);
}

/*---------------------------------------------------------------------------*
 *	decode and notification indicator IE (Q.932, p44)
 *---------------------------------------------------------------------------*/
int
p_q931notification(char *pbuf, unsigned char *buf)
{
	int j = 0;
	int len;
	int i = 0;
	int nd;
	
	i++;	/* index -> length */
	len = buf[i];

	i++;	/* index -> notification description */
	nd = buf[i];
	
	switch (nd)
	{
		case 0x80:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, user suspended", nd);
			break;
		case 0x81:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, user resumed", nd);
			break;
		case 0x82:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, bearer service changed", nd);
			break;

		case 0x83:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, BER coded information", nd);
			break;

		case 0xc2:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, conference established", nd);
			break;
		case 0xc3:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, conference disconnected", nd);
			break;
		case 0xc4:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, other party added", nd);
			break;
		case 0xc5:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, isolated", nd);
			break;
		case 0xc6:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, reattached", nd);
			break;
		case 0xc7:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, other party isolated", nd);
			break;
		case 0xc8:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, other party reattached", nd);
			break;
		case 0xc9:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, other party split", nd);
			break;
		case 0xca:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, other party disconnected", nd);
			break;
		case 0xcb:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, conference floating", nd);
			break;
		case 0xcc:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, conference disconnected, preemption", nd);
			break;
		case 0xcf:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, conference floating, server user preempted", nd);
			break;

		case 0xe0:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, call is a waiting call", nd);
			break;			
		case 0xe8:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, diversion activated", nd);
			break;			
		case 0xe9:	/* ECT, EN 300 369-1 V1.2.4 p12) */
			sprintf((pbuf+strlen(pbuf)), "0x%2x, call transferred, alerting", nd);
			break;
		case 0xea:	/* ECT, EN 300 369-1 V1.2.4 p12) */
			sprintf((pbuf+strlen(pbuf)), "0x%2x, call transferred, active", nd);
			break;
		case 0xee:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, reverse charging", nd);
			break;

		case 0xf9:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, remote hold", nd);
			break;
		case 0xfa:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, remote retrieval", nd);
			break;
		case 0xfb:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, call is diverting", nd);
			break;
		default:
			sprintf((pbuf+strlen(pbuf)), "0x%2x, undefined", nd);
			break;
	}

	i++;
	len--;

	if (len)
	{
		sprintf((pbuf+strlen(pbuf)),": (");
		
		for (; j < len; j++)
		{
			if (nd == 4)
				sprintf((pbuf+strlen(pbuf)),"%c", buf[j+i]);
			else
				sprintf((pbuf+strlen(pbuf)),"0x%2x", buf[j+i]);
		}
	
		sprintf((pbuf+strlen(pbuf)),")");
	}
	sprintf((pbuf+strlen(pbuf)),"]");
	i += j;

	return(i);
}

/*---------------------------------------------------------------------------*
 *	decode and print redirecting/redirection number
 *---------------------------------------------------------------------------*/
int
p_q931redir(char *pbuf, unsigned char *buf)
{
	int j;
	int len;
	int i = 0;
	int tp;
	int ind = 0;
	int indflag = 0;
	int reas = 0;
	int reasflag = 0;
	
	i++;	/* index -> length */
	len = buf[i];

	i++;	/* index -> type/plan */
	tp = buf[i];
	
	i++;
	len--;
	
	if (!(tp & 0x80))
	{
		ind = buf[i];		
		indflag = 1;
		i++;
		len--;

		if (!(ind & 0x80))
		{
			reas = buf[i];		
			reasflag = 1;
			i++;
			len--;
		}
	}

	for (j = 0; j < len; j++)
	{
		sprintf((pbuf+strlen(pbuf)),"%c", buf[j+i]);
	}

	switch ((tp & 0x70) >> 4)
	{
	case 0:
		sprintf((pbuf+strlen(pbuf)), " (type=unknown, ");
		break;
	case 1:
		sprintf((pbuf+strlen(pbuf)), " (type=international, ");
		break;
	case 2:
		sprintf((pbuf+strlen(pbuf)), " (type=national, ");
		break;
	case 3:
		sprintf((pbuf+strlen(pbuf)), " (type=network specific, ");
		break;
	case 4:
		sprintf((pbuf+strlen(pbuf)), " (type=subscriber, ");
		break;
	case 6:
		sprintf((pbuf+strlen(pbuf)), " (type=abbreviated, ");
		break;
	default:
		sprintf((pbuf+strlen(pbuf)), " (type=reserved (%d), ", ((tp & 0x70) >> 4));
		break;
	}

	switch (tp & 0x0f)
	{
	case 0:
		sprintf((pbuf+strlen(pbuf)), "plan=unknown");
		break;
	case 1:
		sprintf((pbuf+strlen(pbuf)), "plan=ISDN");
		break;
	case 3:
		sprintf((pbuf+strlen(pbuf)), "plan=Data");
		break;
	case 4:
		sprintf((pbuf+strlen(pbuf)), "plan=Telex");
		break;
	case 8:
		sprintf((pbuf+strlen(pbuf)), "plan=National");
		break;
	case 9:
		sprintf((pbuf+strlen(pbuf)), "plan=private");
		break;
	default:
		sprintf((pbuf+strlen(pbuf)), "plan=reserved (%d)", (tp & 0x0f));
		break;
	}

	if (indflag)
	{
		sprintf((pbuf+strlen(pbuf)), ",\n          ");
		switch ((ind & 0x60) >> 5)
		{
		case 0:
			sprintf((pbuf+strlen(pbuf)), "presentation allowed");
			break;
		case 1:
			sprintf((pbuf+strlen(pbuf)), "presentation restricted");
			break;
		case 2:
			sprintf((pbuf+strlen(pbuf)), "number not available");
			break;
		case 3:
			sprintf((pbuf+strlen(pbuf)), "reserved");
			break;
		}
	}		

	if (reasflag)
	{
		sprintf((pbuf+strlen(pbuf)), ",\n          ");
		switch (reas & 0x0f)
		{
		case 0:
			sprintf((pbuf+strlen(pbuf)), "reason for diversion: unknown");
			break;
		case 1:
			sprintf((pbuf+strlen(pbuf)), "reason for diversion: call forwarding busy");
			break;
		case 2:
			sprintf((pbuf+strlen(pbuf)), "reason for diversion: call forwarding unconditional");
			break;
		case 0xa:
			sprintf((pbuf+strlen(pbuf)), "reason for diversion: called DTE");
			break;
		case 0xf:
			sprintf((pbuf+strlen(pbuf)), "reason for diversion: call forwarding unconditional");
			break;
		default:
			sprintf((pbuf+strlen(pbuf)), "reason for diversion: reserved (0x%2x)",reas & 0x0f);
			break;
		}
	}		

	sprintf((pbuf+strlen(pbuf)),")]");

	i += j;

	return(i);
}

/* EOF */

