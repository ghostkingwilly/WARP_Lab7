/** @file wl_user.c
 *  @brief WARPLab Framework (User Extensions)
 *
 *  This contains the code for WARPLab Framework user extensions.
 *
 *  @copyright Copyright 2013-2015, Mango Communications. All rights reserved.
 *          Distributed under the WARP license  (http://warpproject.org/license)
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

/***************************** Include Files *********************************/

// Xilinx / Standard library includes
#include <xparameters.h>
#include <xil_types.h>
#include <xstatus.h>
#include <stdlib.h>
#include <stdio.h>

// WARPLab includes
#include "wl_common.h"
#include "wl_user.h"


// Peripheral includes
#ifdef WARP_HW_VER_v3
#include <w3_iic_eeprom.h>
#endif

/*************************** Constant Definitions ****************************/

#define EEPROM_EXAMPLE_BUFFER_SIZE                         10


/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/


/*************************** Function Prototypes *****************************/


/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * Process User Defined Commands
 *
 * This function is part of the Ethernet processing system and will process the
 * various user defined commands.
 *
 * @param   socket_index     - Index of the socket on which to send message
 * @param   from             - Pointer to socket address structure (struct sockaddr *) where command is from
 * @param   command          - Pointer to WARPLab Command
 * @param   response         - Pointer to WARPLab Response
 *
 * @return  int              - Status of the command:
 *                                 NO_RESP_SENT - No response has been sent
 *                                 RESP_SENT    - A response has been sent
 *
 * @note    See on-line documentation for more information about the ethernet
 *          packet structure for WARPLab:  www.warpproject.org
 *
 ******************************************************************************/

int user_process_cmd(int socket_index, void * from, wl_cmd_resp * command, wl_cmd_resp * response) {

    //
    // IMPORTANT ENDIAN NOTES:
    //     - command
    //         - header - Already endian swapped by the framework (safe to access directly)
    //         - args   - Must be endian swapped as necessary by code (framework does not know the contents of the command)
    //     - response
    //         - header - Will be endian swapped by the framework (safe to write directly)
    //         - args   - Must be endian swapped as necessary by code (framework does not know the contents of the response)
    //

    // Standard variables
    u32                 resp_sent      = NO_RESP_SENT;

    wl_cmd_resp_hdr   * cmd_hdr        = command->header;
    u32               * cmd_args_32    = command->args;
    u32                 cmd_id         = WL_CMD_TO_CMDID(cmd_hdr->cmd);

    wl_cmd_resp_hdr   * resp_hdr       = response->header;
    u32               * resp_args_32   = response->args;
    u32                 resp_index     = 0;

    // Specific command variables

#ifdef WARP_HW_VER_v3
    // Arguments for EEPROM Read/Write example
    u32  k;
    u32  buffer_32[EEPROM_EXAMPLE_BUFFER_SIZE + 1];        // Extra entry for Null termination of strings
    u8 * buffer_8 = (u8 *) buffer_32;
    u32  length;
    u32  eeprom_addr_offset;
#endif

    // Set up the response header
    resp_hdr->cmd       = cmd_hdr->cmd;
    resp_hdr->length    = 0;
    resp_hdr->num_args  = 0;

    // Process the command
    switch(cmd_id) {

#ifdef WARP_HW_VER_v3

        // Write string to EEPROM
        case CMDID_USER_EEPROM_WRITE_STRING:
            // Process command arguments
            eeprom_addr_offset = Xil_Ntohl(cmd_args_32[0]);
            length             = ((cmd_hdr->length) / sizeof(u32));

            // Check that we have allocated enough space for the write
            if (length <= (EEPROM_EXAMPLE_BUFFER_SIZE << 2)) {

                // Initialize buffer to zero (also serves to null terminate all strings)
                for (k = 0; k < (EEPROM_EXAMPLE_BUFFER_SIZE + 1); k++ ) { buffer_32[k] = 0; }

                // Endian swap all of the characters to write
                for(k = 0; k < length; k++) {
                    buffer_32[k] = Xil_Ntohl(cmd_args_32[k+1]);
                }

                // Write the string to the EEPROM
                for(k = 0; k < (cmd_hdr->length); k++){
                    iic_eeprom_writeByte(EEPROM_BASEADDR, (k + eeprom_addr_offset), buffer_8[k]);
                }

                wl_printf(WL_PRINT_NONE, print_type_user, "Wrote '%s' to EEPROM\n", buffer_8);
            } else {
                wl_printf(WL_PRINT_ERROR, print_type_user, "Message too long (%d characters).  Only %d characters supported.\n",
                        cmd_hdr->length, (EEPROM_EXAMPLE_BUFFER_SIZE << 2));
            }
        break;

        // Read string from EERPOM
        case CMDID_USER_EEPROM_READ_STRING:
            // Process command arguments
            eeprom_addr_offset = Xil_Ntohl(cmd_args_32[0]);
            length             = Xil_Ntohl(cmd_args_32[1]);            // Length of the read (in bytes)

            // Check that we have allocated enough space for the read
            if ( length <= (EEPROM_EXAMPLE_BUFFER_SIZE << 2)) {

                // Initialize buffer to zero (also serves to null terminate all strings)
                for (k = 0; k < (EEPROM_EXAMPLE_BUFFER_SIZE + 1); k++ ) { buffer_32[k] = 0; }

                // Read the string from the EEPROM
                for(k = 0; k < length; k++){
                    buffer_8[k] = iic_eeprom_readByte(EEPROM_BASEADDR, (k + eeprom_addr_offset));
                }

                wl_printf(WL_PRINT_NONE, print_type_user, "Read '%s' from EEPROM\n", buffer_8);

                // Endian swap all the characters to read
                for(resp_index = 0; resp_index < ((length)/sizeof(u32)); resp_index++) {
                    resp_args_32[resp_index] = Xil_Htonl(buffer_32[resp_index]);
                }

                resp_hdr->length  += (resp_index * sizeof(resp_args_32));
                resp_hdr->num_args = resp_index;
            } else {
                wl_printf(WL_PRINT_ERROR, print_type_user, "Requested message too long (%d characters).  Only %d characters supported.\n",
                        length, (EEPROM_EXAMPLE_BUFFER_SIZE << 2));
            }
        break;

#endif

/* Example Additional User Command
        case SOME_CMD_ID:                              <-- Needs to be defined as same number in both Matlab and C
            arg0   = Xil_Ntohl(cmd_args[0]);
            arg1   = Xil_Ntohl(cmd_args[1]);
            result = do_something_with_args(arg0, arg1);

            resp_args_32[resp_index++] = Xil_Htonl(result);
            resp_hdr->length        += (resp_index * sizeof(resp_args_32));
            resp_hdr->numArgs        = resp_index;
        break;
*/

        default:
            wl_printf(WL_PRINT_ERROR, print_type_user, "Unknown user command ID: %d\n", cmd_id);
        break;
    }

    return resp_sent;
}



/*****************************************************************************/
/**
 * @brief User extension subsystem initialization
 *
 * Initializes the user extension subsystem
 *
 * @param   None
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/
int user_init(){
    int status = XST_SUCCESS;

    // User initialization goes here
    //     Framework calls user_init when node is initialized
    //     (on boot and when node 'initialize' command is received)

    return status;
}




