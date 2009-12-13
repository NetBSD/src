/*	$NetBSD: ntpSnmpSubagentObject.c,v 1.1.1.1 2009/12/13 16:56:34 kardel Exp $	*/

/*****************************************************************************
 *
 *  ntpSnmpSubAgentObject.c
 *
 *  This file provides the callback functions for net-snmp and registers the 
 *  serviced MIB objects with the master agent.
 * 
 *  Each object has its own callback function that is called by the 
 *  master agent process whenever someone queries the corresponding MIB
 *  object. 
 * 
 *  At the moment this triggers a full send/receive procedure for each
 *  queried MIB object, one of the things that are still on my todo list:
 *  a caching mechanism that reduces the number of requests sent to the
 *  ntpd process.
 *
 ****************************************************************************/
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "ntpSnmpSubagentObject.h"
#include <libntpq.h>

/* general purpose buffer length definition */
#define NTPQ_BUFLEN 2048

static int      ntpSnmpSubagentObject = 3;

char ntpvalue[NTPQ_BUFLEN];



/*****************************************************************************
 *
 * ntpsnmpd_strip_string
 *
 *  This function removes white space characters and EOL chars 
 *  from the beginning and end of a given NULL terminated string. 
 *  Be aware that the parameter itself is altered.
 *  
 ****************************************************************************
 * Parameters:
 *	string		char*	The name of the string variable
 *						NOTE: must be NULL terminated!
 * Returns:
 *	int		length of resulting string (i.e. w/o white spaces)
 ****************************************************************************/

int ntpsnmpd_strip_string(char *string)
{
	char newstring[2048] = { 0 };
	int i = 0;
	int j = 0;

	if ( strlen(string) > 2047 ) 
		string[2048]=0;

	j = strlen(string);

	for (i=0;i<strlen(string);i++)
	{
		switch(string[i])
		{
			case 0x09: 	// Tab
			case 0x0A:	// LF
			case 0x0D:	// CR
			case ' ':  	// Space
			  break;
			default:
			  strncpy(newstring,(char *) &string[i], sizeof(newstring));
			  i=2048;
			  break;
		}
	}
	strncpy(string, newstring, j);

	return(strlen(string));
}


/*****************************************************************************
 *
 * ntpsnmpd_parse_string
 *
 *  This function will parse a given NULL terminated string and cut it
 *  into a fieldname and a value part (using the '=' as the delimiter. 
 *  The fieldname will be converted to uppercase and all whitespace 
 *  characters are removed from it.
 *  The value part is stripped, e.g. all whitespace characters are removed
 *  from the beginning and end of the string.
 *  If the value is started and ended with quotes ("), they will be removed
 *  and everything between the quotes is left untouched (including 
 *  whitespace)
 *  Example:
 *     server host name =   hello world!
 *  will result in a field string "SERVERHOSTNAME" and a value
 *  of "hello world!".
 *     My first Parameter		=		"  is this!    "
  * results in a field string "MYFIRSTPARAMETER" and a vaue " is this!    "
 ****************************************************************************
 * Parameters:
 *	src			char*	The name of the source string variable
 *						NOTE: must be NULL terminated!
 *	field			char*	The name of the string which takes the
 *						fieldname
 *	fieldsize		int		The maximum size of the field name
 *	value		char*	The name of the string which takes the
 *						value part
 *	valuesize		int		The maximum size of the value string
 *
 * Returns:
 *	int			length of value string 
 ****************************************************************************/

int ntpsnmpd_parse_string(char *src, char *field, int fieldsize, char *value, int valuesize)
{
	char string[2048];
	int i = 0;
	int j = 0;
	int l = 0;
	int a = 0;

	strncpy(string,  src, sizeof(string));

	a = strlen(string);

	/* Parsing the field name */
	for (i=0;l==0;i++)
	{
		if (i>=a)
		   l=1;
		else
		{
			switch(string[i])
			{
				case 0x09: 	// Tab
				case 0x0A:	// LF
				case 0x0D:	// CR
				case ' ':  	// Space
				  break;
				case '=':
				  l=1;
				  break;
				  
				default:
				  if ( j < fieldsize ) 
				  {
					if ( ( string[i] >= 'a' ) && ( string[i] <='z' ) )
						field[j++]=( string[i] - 32 ); // convert to Uppercase
					else
						field[j++]=string[i]; 
				  }	

			}
		}
	}

	field[j]=0; j=0; value[0]=0;


	/* Now parsing the value */
	for (l=0;i<a;i++)
	{
		if ( ( string[i] > 0x0D ) && ( string[i] != ' ' ) )
		   l = j+1;
		
		if ( ( value[0] != 0 ) || ( ( string[i] > 0x0D ) && ( string[i] != ' ' ) ) )
		{
			if (j < valuesize )
			   value[j++]=string[i];
		}
	}

	value[l]=0;

	if ( value[0]=='"' )
		strcpy(value, (char *) &value[1]);

	if ( value[strlen(value)-1] == '"' ) 
		value[strlen(value)-1]=0;

	return (strlen(value));

}


/*****************************************************************************
 *
 * ntpsnmpd_cut_string
 *
 *  This function will parse a given NULL terminated string and cut it
 *  into fields using the specified delimiter character. 
 *  It will then copy the requested field into a destination buffer
 *  Example:
 *     ntpsnmpd_cut_string(read:my:lips:fool, RESULT, ':', 2, sizeof(RESULT))
 *  will copy "lips" to RESULT.
 ****************************************************************************
 * Parameters:
 *	src			char*	The name of the source string variable
 *						NOTE: must be NULL terminated!
 *	dest			char*	The name of the string which takes the
 *						requested field content
 * 	delim			char	The delimiter character
 *	fieldnumber		int		The number of the required field
 *						(start counting with 0)
 *	maxsize			int		The maximum size of dest
 *
 * Returns:
 *	int			length of resulting dest string 
 ****************************************************************************/

int ntpsnmpd_cut_string(char *src, char *dest, const char delim, int fieldnumber, int maxsize)
{
	char string[2048];
	int i = 0;
	int j = 0;
	int l = 0;
	int a = 0;

	strncpy (string, src, sizeof(string));
	
	a = strlen(string);

        memset (dest, 0, maxsize);

	/* Parsing the field name */
	for (i=0;l<=fieldnumber;i++)
	{
		if (i>=a)
		   l=fieldnumber+1; /* terminate loop */
		else
		{
			if ( string[i] == delim )
			{
				  l++; /* next field */
			}
			else  if ( ( l == fieldnumber) && ( j < maxsize )  )
			{
				dest[j++]=string[i]; 
			}	

		}
	}

	return (strlen(dest));

}


/*****************************************************************************
 *
 *  read_ntp_value
 *
 *  This function retrieves the value for a given variable, currently
 *  this only supports sysvars. It starts a full mode 6 send/receive/parse
 *  iteration and needs to be optimized, e.g. by using a caching mechanism
 *  
 ****************************************************************************
 * Parameters:
 *	variable	char*	The name of the required variable
 *	rbuffer		char*	The buffer where the value goes
 *	maxlength	int	Max. number of bytes for resultbuf
 *
 * Returns:
 *	u_int		number of chars that have been copied to 
 *			rbuffer 
 ****************************************************************************/

unsigned int read_ntp_value(char *variable, char *rbuffer, unsigned int maxlength)
{
	unsigned int i, sv_len = 0;
	char sv_data[NTPQ_BUFLEN];
	
	memset (sv_data,0, NTPQ_BUFLEN);
	sv_len= ntpq_read_sysvars ( sv_data, NTPQ_BUFLEN );
		
	if ( sv_len )
	{
		i=ntpq_getvar( sv_data, sv_len , variable, rbuffer, maxlength);
		return i;
	} else {
		return 0;
	}

}


/*****************************************************************************
 *
 *  The get_xxx functions
 *
 *  The following function calls are callback functions that will be 
 *  used by the master agent process to retrieve a value for a requested 
 *  MIB object. 
 *
 ****************************************************************************/


int get_ntpEntSoftwareName (netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests)
{
 char ntp_softwarename[NTPQ_BUFLEN];
	
   memset (ntp_softwarename, 0, NTPQ_BUFLEN);
	
   switch (reqinfo->mode) {
   case MODE_GET:
   {
	if ( read_ntp_value("product", ntpvalue, NTPQ_BUFLEN) )
       {
	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                             (u_char *)ntpvalue,
                             strlen(ntpvalue)
                            );
       } 
    else  if ( read_ntp_value("version", ntpvalue, NTPQ_BUFLEN) )
    {
	ntpsnmpd_cut_string(ntpvalue, ntp_softwarename, ' ', 0, sizeof(ntp_softwarename)-1);
	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                             (u_char *)ntp_softwarename,
                             strlen(ntp_softwarename)
                            );
    } else {
	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                             (u_char *)"N/A",
                             3
                            );
    }
    break;
    
  }


  default:
	  /* If we cannot get the information we need, we will return a generic error to the SNMP client */
        return SNMP_ERR_GENERR;
  }

  return SNMP_ERR_NOERROR;
}


int get_ntpEntSoftwareVersion (netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests)
{

   switch (reqinfo->mode) {
   case MODE_GET:
   {
    
    if ( read_ntp_value("version", ntpvalue, NTPQ_BUFLEN) )
    {
	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                             (u_char *)ntpvalue,
                             strlen(ntpvalue)
                            );
    } else {
	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                             (u_char *)"N/A",
                             3
                            );
    }
    break;
    
  }


  default:
	  /* If we cannot get the information we need, we will return a generic error to the SNMP client */
        return SNMP_ERR_GENERR;
  }

  return SNMP_ERR_NOERROR;
}


int get_ntpEntSoftwareVersionVal (netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests)
{
   unsigned int i = 0;
   switch (reqinfo->mode) {
   case MODE_GET:
   {
    
    if ( read_ntp_value("versionval", ntpvalue, NTPQ_BUFLEN) )
    {
	i=atoi(ntpvalue);
	snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
                             (u_char *) &i,
                             sizeof (i)
                            );
    } else {
	i = 0;
	snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
                             (u_char *) &i,
                             sizeof(i)
                            );
    }
    break;
    
  }


  default:
	  /* If we cannot get the information we need, we will return a generic error to the SNMP client */
        return SNMP_ERR_GENERR;
  }

  return SNMP_ERR_NOERROR;
}



int get_ntpEntSoftwareVendor (netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests)
{

   switch (reqinfo->mode) {
   case MODE_GET:
   {
    
    if ( read_ntp_value("vendor", ntpvalue, NTPQ_BUFLEN) )
    {
	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                             (u_char *)ntpvalue,
                             strlen(ntpvalue)
                            );
    } else {
	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                             (u_char *)"N/A",
                             3
                            );
    }
    break;

  default:
	  /* If we cannot get the information we need, we will return a generic error to the SNMP client */
        return SNMP_ERR_GENERR;
   }
  }
  return SNMP_ERR_NOERROR;
}


int get_ntpEntSystemType (netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests)
{

   switch (reqinfo->mode) {
   case MODE_GET:
   {
    
    if ( read_ntp_value("systemtype", ntpvalue, NTPQ_BUFLEN) )
    {
	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                             (u_char *)ntpvalue,
                             strlen(ntpvalue)
                            );
    }
	   
    if ( read_ntp_value("system", ntpvalue, NTPQ_BUFLEN) )
    {
	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                             (u_char *)ntpvalue,
                             strlen(ntpvalue)
                            );
    } else {
	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                             (u_char *)"N/A",
                             3
                            );
    }
    break;
    
  }


  default:
	  /* If we cannot get the information we need, we will return a generic error to the SNMP client */
        return SNMP_ERR_GENERR;
  }

  return SNMP_ERR_NOERROR;
}

int get_ntpEntTimeResolution (netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests)
{

   switch (reqinfo->mode) {
   case MODE_GET:
   {
    
    if ( read_ntp_value("resolution", ntpvalue, NTPQ_BUFLEN) )
    {
	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                             (u_char *)ntpvalue,
                             strlen(ntpvalue)
                            );
    } else {
	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                             (u_char *)"N/A",
                             3
                            );
    }
    break;
    
  }


  default:
	  /* If we cannot get the information we need, we will return a generic error to the SNMP client */
        return SNMP_ERR_GENERR;
  }

  return SNMP_ERR_NOERROR;
}


int get_ntpEntTimeResolutionVal (netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests)
{

   unsigned int i = 0;
   switch (reqinfo->mode) {
   case MODE_GET:
   {
    
    if ( read_ntp_value("resolutionval", ntpvalue, NTPQ_BUFLEN) )
    {
	i=atoi(ntpvalue);
	snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
                             (u_char *) &i,
                             sizeof (i)
                            );
    } else {
	i = 0;
	snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
                             (u_char *) &i,
                             sizeof(i)
                            );
    }
    break;
    
  }


  default:
	  /* If we cannot get the information we need, we will return a generic error to the SNMP client */
        return SNMP_ERR_GENERR;
  }

  return SNMP_ERR_NOERROR;
}


int get_ntpEntTimePrecision (netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests)
{
   switch (reqinfo->mode) {
   case MODE_GET:
   {
    
    if ( read_ntp_value("precision", ntpvalue, NTPQ_BUFLEN) )
    {
	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                             (u_char *)ntpvalue,
                             strlen(ntpvalue)
                            );
    } else {
	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                             (u_char *)"N/A",
                             3
                            );
    }
    break;
    
  }


  default:
	  /* If we cannot get the information we need, we will return a generic error to the SNMP client */
        return SNMP_ERR_GENERR;
  }

  return SNMP_ERR_NOERROR;
}

int get_ntpEntTimePrecisionVal (netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests)
{

    int i = 0;
   switch (reqinfo->mode) {
   case MODE_GET:
   {
    
    if ( read_ntp_value("precision", ntpvalue, NTPQ_BUFLEN) )
    {
	i=atoi(ntpvalue);
	snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                             (u_char *) &i,
                             sizeof (i)
                            );
    } else {
	i = 0;
	snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                             (u_char *) &i,
                             sizeof(i)
                            );
    }
    break;
    
  }


  default:
	  /* If we cannot get the information we need, we will return a generic error to the SNMP client */
        return SNMP_ERR_GENERR;
  }

  return SNMP_ERR_NOERROR;
}



int get_ntpEntTimeDistance (netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests)
{
   switch (reqinfo->mode) {
   case MODE_GET:
   {
    
    if ( read_ntp_value("rootdelay", ntpvalue, NTPQ_BUFLEN) )
    {
	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                             (u_char *)ntpvalue,
                             strlen(ntpvalue)
                            );
    } else {
	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                             (u_char *)"N/A",
                             3
                            );
    }
    break;
    
  }


  default:
	  /* If we cannot get the information we need, we will return a generic error to the SNMP client */
        return SNMP_ERR_GENERR;
  }

  return SNMP_ERR_NOERROR;
}


/*
 *
 * Initialize sub agent
 * TODO: Define NTP MIB OID (has to be assigned by IANA)
 * At the moment we use a private MIB branch (enterprises.5597.99)
 */

void
init_ntpSnmpSubagentObject(void)
{
	
    /* Register all MIB objects with the agentx master */
	
  _SETUP_OID_RO( ntpEntSoftwareName ,  	NTPV4_OID , 1, 1, 1, 0  );
  _SETUP_OID_RO( ntpEntSoftwareVersion ,  	NTPV4_OID , 1, 1, 2, 0  );
  _SETUP_OID_RO( ntpEntSoftwareVersionVal ,	NTPV4_OID , 1, 1, 3, 0  );
  _SETUP_OID_RO( ntpEntSoftwareVendor ,  	NTPV4_OID , 1, 1, 4, 0  );
  _SETUP_OID_RO( ntpEntSystemType ,  		NTPV4_OID , 1, 1, 5, 0  );
  _SETUP_OID_RO( ntpEntTimeResolution ,  	NTPV4_OID , 1, 1, 6, 0  );
  _SETUP_OID_RO( ntpEntTimeResolutionVal , 	NTPV4_OID , 1, 1, 7, 0  );
  _SETUP_OID_RO( ntpEntTimePrecision ,  	NTPV4_OID , 1, 1, 8, 0  );
  _SETUP_OID_RO( ntpEntTimePrecisionVal ,  	NTPV4_OID , 1, 1, 9, 0  );
  _SETUP_OID_RO( ntpEntTimeDistance ,  	NTPV4_OID , 1, 1,10, 0  );

}

