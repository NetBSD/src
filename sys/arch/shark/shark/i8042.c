/*	$NetBSD: i8042.c,v 1.3 2003/07/15 03:36:03 lukem Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
**++
** 
**  FACILITY:
**
**    8042 controller functions.
**
**  ABSTRACT:
**
**    This file contains routines to access the 8042 keyboard microprocessor.
**    It hopefully allows a level of abstraction which will permit 
**    simplification of keyboard and mouse drivers which have to share the
**    same registers when talking to the 8042.
**
**  AUTHORS:
**
**    John Court, Digital Equipment Corporation.
**
**  CREATION DATE:
**
**    16/4/1997
**
**--
*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i8042.c,v 1.3 2003/07/15 03:36:03 lukem Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <machine/kerndebug.h>

#include <machine/bus.h>
#include <shark/shark/i8042reg.h>
/* 
** Global variables 
*/

/* Variable to control which debugs printed.  No debug code gets
** built into this driver unless KERN_DEBUG is defined in the config 
** file.  
*/
int i8042debug = KERN_DEBUG_WARNING | KERN_DEBUG_ERROR; 

/*
**++
**  FUNCTIONAL DESCRIPTION
**
**     i8042_flush
**
**     This routine waits until the input and output buffers
**     on the 8042 are empty, discarding any characters 
**     in the input buffer.
**
**  FORMAL PARAMETERS:
**
**     iot    I/O tag for the mapped register space  
**     ioh    I/O handle for the mapped register space
**
**  IMPLICIT INPUTS:
**
**     none.
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     none.
**--
*/
void
i8042_flush( bus_space_tag_t iot, 
             bus_space_handle_t ioh)
{
    /* Wait until input and output buffers are empty */
    (void)i8042_wait_output(iot,ioh);
    while (i8042_wait_input(iot,ioh,I8042_ANY_DATA))
    {
        (void)bus_space_read_1(iot, ioh, KBDATAPO);
    }
    return;
} /* End i8042_flush */
 
/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     i8042_wait_output
**
**     This function is boring.  It just waits until output
**     can be sent to the 8042 buffer.
**
**  FORMAL PARAMETERS:
**
**     iot    I/O tag for the mapped register space  
**     ioh    I/O handle for the mapped register space
**
**  IMPLICIT INPUTS:
**
**     none.
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**   0 - Timed out waiting to send output. 
**   1 - Can now send output to the 8042.
**--
*/
int
i8042_wait_output( bus_space_tag_t    iot,
                   bus_space_handle_t ioh )
{
    register u_int     count;
    int                retValue = 0;
    
    for (count = I8042_WAIT_THRESHOLD; count; count--)
    {
        /* Check if output buffer empty */
        if ((bus_space_read_1(iot, ioh, KBSTATPO) & KBS_IBF) == 0) 
        {
            retValue = 1;
	    break;
        }
    }
    return (retValue);
} /* End i8042_wait_output */


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     i8042_wait_input
**
**     This function waits until input is available to be read from
**     the 8042 output buffer. 
**
**  FORMAL PARAMETERS:
**
**     iot    I/O tag for the mapped register space  
**     ioh    I/O handle for the mapped register space
**     type   Type of input to wait for (auxiliary, keyboard or any).
**
**  IMPLICIT INPUTS:
**
**     none.
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**   0 - Timed out waiting for input
**   1 - Input available to be read
**--
*/
int
i8042_wait_input(bus_space_tag_t    iot,
                 bus_space_handle_t ioh,
                 u_char             type)
{
    register u_int     count;
    register u_char    status;
    int                retValue = 0;

    for (count = I8042_WAIT_THRESHOLD; count; count--)
    {
        /* Check if there is a character to be read */
        status = bus_space_read_1(iot, ioh, KBSTATPO);
        if (((status & type) == type) || 
            ((type == I8042_ANY_DATA) && (status & KBS_DIB)))
        {
            retValue = 1;
	    break;
        }
	I8042_DELAY;
    }
    KERN_DEBUG(i8042debug, KERN_DEBUG_INFO, 
	       ("i8042_wait_input: returning : %s\n\tlast status : 0x%x\n",
		retValue ? "Found Data" : "Exceeded Wait Threshold",
		status));

    return (retValue);
} /* End i8042_wait_input */


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     i8042_cmd
**
**    This function sends a command to the 8042 device or the auxiliary
**    device hanging off it. The command is retried a 
**    number of times if a resend response is received.
**
**  FORMAL PARAMETERS:
**
**     iot               I/O tag for the mapped register space  
**     ioh               I/O handle for the mapped register space
**     auxCmd            An indication of what type of command this is.
**     checkResponse     A switch indicating whether to read a result after
**                       executing the command and compare ot with 
**                       "responseExpected".
**     responseExpected  Only valid if "checkResponse" is non-zero.  This
**                       is compared with the data value read after the 
**                       command has been executed.
**     value             Command to send to the device selected by "auxCmd".
**
**  IMPLICIT INPUTS:
**
**     none.
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**   0 - Failed to send command or receive acknowledgement for it
**   1 - Command sent and responded to successfully.
**--
*/
int
i8042_cmd(bus_space_tag_t    iot,
          bus_space_handle_t ioh,
          u_char             auxCmd,
          u_char             checkResponse,
          u_char             responseExpected,
          u_char             value)
{
    u_int              retries;
    register u_char    c = NULL;
    int                status;  
    
    /* Assume failure
    */
    status = 0;

    for (retries = I8042_RETRIES; 
         i8042_wait_output(iot,ioh) && retries;
         retries--)
    {
        if (auxCmd == I8042_AUX_CMD)
        {
            /* Setup to write command to auxiliary device 
            */
            bus_space_write_1(iot, ioh, KBCMDPO, KBC_AUXWRITE);
            /* Write actual command to selected device
            */
            if (i8042_wait_output(iot,ioh))
            {
                bus_space_write_1(iot, ioh, KBOUTPO, value);
            }
            else
            {
                KERN_DEBUG(i8042debug, KERN_DEBUG_WARNING, 
                     ("i8042_cmd: failed aux device request of 0x%x\n", 
                      value));
                break;
            }
        }
        else if (auxCmd == I8042_CMD)
        {
            /* Write command to keyboard controller requested. 
            */
            bus_space_write_1(iot, ioh, KBCMDPO, value);
        }
        else if (auxCmd == I8042_KBD_CMD)
        {
            /* Write a command to actual keyboard H/W device. 
            */
            bus_space_write_1(iot, ioh, KBOUTPO, value);
        }
        else if (auxCmd == I8042_WRITE_CCB)
        {
            /* Write 8042 Controller Command Byte requested
            */
            bus_space_write_1(iot, ioh, KBCMDPO, K_LDCMDBYTE);
            /* Write actual command to selected device
            */
            if (i8042_wait_output(iot,ioh))
            {
                bus_space_write_1(iot, ioh, KBOUTPO, value);
            }
            else
            {
                KERN_DEBUG(i8042debug, KERN_DEBUG_WARNING, 
                           ("i8042_cmd: failed contoller command byte "
                            "write request of 0x%x\n", 
                            value));
                break;
            }
        }
        else
        {
            KERN_DEBUG(i8042debug, KERN_DEBUG_WARNING, 
                       ("i8042_cmd: invalid device identifier of 0x%x\n", 
                        auxCmd));
            break;
        }

        /* Does anyone need to check the result of this command ?
        */
        if (checkResponse == I8042_CHECK_RESPONSE)
        {
            /* get response from device and check if
            ** successful.
            */
            if (i8042_wait_input(iot,ioh,I8042_ANY_DATA))
            {
                c = bus_space_read_1(iot, ioh, KBDATAPO);
                if (c == responseExpected)
                { 
                    /* Successfull command so we're outa here
                    */
                    status = 1;
                    break;
                }
                else if (c == KBR_RESEND)
                {
                    /* Hmm response was try again so lets. 
                    */
                    KERN_DEBUG(i8042debug, KERN_DEBUG_WARNING, 
                               ("i8042_cmd: resend of 0x%x\n", value));
                }
                else
                {
                    /* response was nothing we expected so we're
                    ** outa here.
                    */
                    KERN_DEBUG(i8042debug, KERN_DEBUG_WARNING, 
                         ("i8042_cmd: unexpected response 0x%x\n", c));
                    break;
                } 
            } /* End If able to get response from device */
            else
            {
                /* Timmed out waiting for a response .... maybe we
                ** weren't meant to get one ??
                */
                KERN_DEBUG(i8042debug, KERN_DEBUG_WARNING, 
                           ("i8042_cmd: no response to command 0x%x\n", 
                            value));
                break;
            }
        } /* End if need to check for response */
        else
        {
            /* Not requested to check for response and we did send the 
            ** command so I guess we were successful :-)
            */
            status = 1;
            break;
        }
    } /* End loop for several retries if needed a response */
    /* Diagnostic output on command value, result and status returned
    */
    KERN_DEBUG(i8042debug, KERN_DEBUG_INFO, 
               ("i8042_cmd: %s Device : Command 0x%x: %s:\n\t "
                "Check Value 0x%x: "
                "Response Value 0x%x: Status being returned 0x%x\n",
                (auxCmd == I8042_AUX_CMD) ? "Auxiliary" : "Keyboard",
                value, 
                (checkResponse == I8042_CHECK_RESPONSE) ? 
                         "Checking response" : "NOT checking response",
                responseExpected, c, status)); 

    return (status);
} /* End i8042_cmd */


