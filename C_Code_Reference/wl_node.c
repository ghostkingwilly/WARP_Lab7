/** @file wl_node.c
 *  @brief WARPLab Framework (Node)
 *
 *  This contains the code for WARPLab Framework.
 *
 *  @copyright Copyright 2013-2015, Mango Communications. All rights reserved.
 *          Distributed under the WARP license  (http://warpproject.org/license)
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */



/**********************************************************************************************************************/
/**
 * @brief Common Functions
 *
 **********************************************************************************************************************/

/***************************** Include Files *********************************/

// Xilinx / Standard library includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <xparameters.h>
#include <xio.h>

// Xilinx Peripheral includes
#include <xtmrctr.h>

// WARPLab includes
#include "wl_common.h"
#include "wl_node.h"
#include "wl_baseband.h"
#include "wl_interface.h"
#include "wl_transport.h"
#include "wl_user.h"
#include "wl_trigger_manager.h"



/*************************** Constant Definitions ****************************/

//
// See wl_common.h for commonly modified control parameters
//

/*********************** Global Variable Definitions *************************/

extern int                   sock_unicast;                 // UDP socket for unicast traffic to / from the board
extern struct sockaddr_in    addr_unicast;



/*************************** Variable Definitions ****************************/

u16                          node;                         // Node ID
static u8                    dram_present;                 // Is DRAM present and available for use
static u8                    configure_buffers;            // Configure the baseband buffers

#if ALLOW_ETHERNET_PAUSE
u8                           ethernet_pause;               // State variable to pause the reception of Ethernet packets
#endif

/*************************** Functions Prototypes ****************************/

void blink_node( int num_blinks, int blink_time );

void set_node_error_status( int status );


/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * Node Transport Processing
 *
 * This function is how the node processes Ethernet frames from the Transport.  This
 * function will be used as the transport callback for Host-to-Node messages.  Based
 * on the Command Group field in the Command header, this function will call the
 * appropriate sub-system to continue processing the packet.
 *
 * @param   socket_index     - Index of the socket on which message was received
 * @param   from             - Pointer to socket address structure from which message was received
 * @param   recv_buffer      - Pointer to transport buffer with received message
 * @param   send_buffer      - Pointer to transport buffer for a node response to the message
 *
 * @return  None
 *
 * @note    If this packet is a host to node message, then the process_hton_msg_callback
 *          is used to further process the packet.  This method will strip off the
 *          WARP transport header for future packet processing.
 *
 *****************************************************************************/
int  node_rx_from_transport(int socket_index, struct sockaddr * from, warp_ip_udp_buffer * recv_buffer, warp_ip_udp_buffer * send_buffer) {

    u8                  cmd_group;
    u32                 resp_sent      = NO_RESP_SENT;
    u32                 resp_length;

    wl_cmd_resp_hdr   * cmd_hdr;
    wl_cmd_resp         command;
    wl_cmd_resp_hdr   * resp_hdr;
    wl_cmd_resp         response;

    // Initialize the Command/Response structures
    cmd_hdr             = (wl_cmd_resp_hdr *)(recv_buffer->offset);
    command.header      = cmd_hdr;
    command.args        = (u32 *)((recv_buffer->offset) + sizeof(wl_cmd_resp_hdr));
    command.buffer      = (void *)(recv_buffer);

    resp_hdr            = (wl_cmd_resp_hdr *)(send_buffer->offset);
    response.header     = resp_hdr;
    response.args       = (u32 *)((send_buffer->offset) + sizeof(wl_cmd_resp_hdr));
    response.buffer     = (void *)(send_buffer);

    // Endian swap the command header so future processing can understand it
    cmd_hdr->cmd        = Xil_Ntohl(cmd_hdr->cmd);
    cmd_hdr->length     = Xil_Ntohs(cmd_hdr->length);
    cmd_hdr->num_args   = Xil_Ntohs(cmd_hdr->num_args);

    // Send command to appropriate processing sub-system
    cmd_group           = WL_CMD_TO_GRP(cmd_hdr->cmd);

    switch(cmd_group){
        case GROUP_NODE:
            resp_sent = node_process_cmd(socket_index, from, &command, &response);
        break;
        case GROUP_TRANSPORT:
            resp_sent = transport_process_cmd(socket_index, from, &command, &response);
        break;
        case GROUP_INTERFACE:
            resp_sent = ifc_process_cmd(socket_index, from, &command, &response);
        break;
        case GROUP_BASEBAND:
            resp_sent = baseband_process_cmd(socket_index, from, &command, &response);
        break;
        case GROUP_TRIGGER_MANAGER:
            resp_sent = trigmngr_process_cmd(socket_index, from, &command, &response);
        break;
        case GROUP_USER:
            resp_sent = user_process_cmd(socket_index, from, &command, &response);
        break;
        default:
            wl_printf(WL_PRINT_ERROR, print_type_node, "Unknown command group: %d\n", cmd_group);
        break;
    }

    // Adjust the length of the response to include the response data from the sub-system and the
    // response header
    //
    if((resp_sent == NO_RESP_SENT) || (resp_sent == NODE_NOT_READY)) {
        resp_length = (resp_hdr->length + sizeof(wl_cmd_resp_hdr));

        // Keep the length and size of the response in sync since we are adding bytes to the buffer
        send_buffer->length += resp_length;
        send_buffer->size   += resp_length;
    }

    // Endian swap the response header before returning
    resp_hdr->cmd       = Xil_Ntohl(resp_hdr->cmd);
    resp_hdr->length    = Xil_Ntohs(resp_hdr->length);
    resp_hdr->num_args  = Xil_Ntohs(resp_hdr->num_args);

    // Return the status
    return resp_sent;
}



/*****************************************************************************/
/**
 * Node Send Early Response
 *
 * Allows a node to send a response back to the host before the command has
 * finished being processed.  This is to minimize the latency between commands
 * since the node is able to finish processing the command during the time
 * it takes to communicate to the host and receive another command.
 *
 * @param   socket_index     - Index of the socket on which message was received
 * @param   to               - Pointer to socket address structure to which message will be sent
 * @param   resp_hdr         - Pointer to WARPLab Command / Response header for outgoing message
 * @param   buffers          - Pointer to array of IP/UDP buffers that contain the outgoing message
 * @param   num_buffers      - Number of IP/UDP buffers in the array
 *
 * @return  None
 *
 * @note    This function can only send one buffer at a time and will modify both the
 *          response header and buffer length to create an appropriate outgoing message.
 *
 *****************************************************************************/
void node_send_early_resp(int socket_index, void * to, wl_cmd_resp_hdr * resp_hdr, void * buffer) {
    //
    // This function is used to send a response back to the host outside the normal command processing
    // (ie the response does not complete the steps in node_rx_from_transport() after distribution
    // to the different group processing commands), this method must perform the necessary manipulation
    // of the response header and the buffer size so that the message is ready to be sent and then
    // restore the contents so that everything is ready to be used if additional responses are required.
    //

    // wl_printf(WL_PRINT_DEBUG, print_type_node, "Send early response:  cmd = 0x%08x   length = %d\n", resp_hdr->cmd, resp_hdr->length);

    u32                      tmp_cmd;
    u16                      tmp_length;
    u16                      tmp_num_args;
    u32                      tmp_buffer_length;
    u32                      tmp_buffer_size;

    warp_ip_udp_buffer     * buffer_ptr;
    u32                      resp_length;

    // Cast the buffer pointer so it is easier to use
    buffer_ptr               = (warp_ip_udp_buffer *) buffer;

    // Get the current values in the buffer so we can restore them after transmission
    tmp_cmd                  = resp_hdr->cmd;
    tmp_length               = resp_hdr->length;
    tmp_num_args             = resp_hdr->num_args;
    tmp_buffer_length        = buffer_ptr->length;
    tmp_buffer_size          = buffer_ptr->size;

    // Adjust the length of the buffer
    resp_length              = resp_hdr->length + sizeof(wl_cmd_resp_hdr);
    buffer_ptr->length      += resp_length;
    buffer_ptr->size        += resp_length;

    // Endian swap the response header before before transport sends it
    resp_hdr->cmd            = Xil_Ntohl(tmp_cmd);
    resp_hdr->length         = Xil_Ntohs(tmp_length);
    resp_hdr->num_args       = Xil_Ntohs(tmp_num_args);

    // Send the packet
    transport_send(socket_index, (struct sockaddr *)to, (warp_ip_udp_buffer **)&buffer, 0x1);

    // Restore the values in the buffer
    resp_hdr->cmd      = tmp_cmd;
    resp_hdr->length   = tmp_length;
    resp_hdr->num_args = tmp_num_args;
    buffer_ptr->length = tmp_buffer_length;
    buffer_ptr->size   = tmp_buffer_size;
}



/*****************************************************************************/
/**
 * Global initialization function
 *
 * Global_initialize is the subset of initialization commands that are safe
 * to execute multiple times when a user simply wants to reset stats on the board
 *
 * @param   None.
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int global_initialize(){
    int status = XST_SUCCESS;

    status = ifc_init();
    if(status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "Interface initialization error! Exiting\n");
        return XST_FAILURE;
    }

    status = baseband_init(dram_present, configure_buffers);
    if(status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "Baseband initialization error! Exiting\n");
        return XST_FAILURE;
    } else {
        configure_buffers = 0;                   // Only need to configure the buffers once
    }

    status = user_init();
    if(status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "User initialization error! Exiting\n");
        return XST_FAILURE;
    }

    status = trigmngr_init();
    if(status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "Trigger Manager initialization error! Exiting\n");
        return XST_FAILURE;
    }
    return status;
}




/**********************************************************************************************************************/
/**
 * @brief WARP v3 Specific Functions
 *
 **********************************************************************************************************************/

#ifdef WARP_HW_VER_v3

/***************************** Include Files *********************************/

#include <w3_userio.h>
#include <w3_clock_controller.h>
#include <w3_iic_eeprom.h>
#include <xil_cache.h>

#ifdef XPAR_XSYSMON_NUM_INSTANCES
    #include <xsysmon_hw.h>
#endif


/*************************** Constant Definitions ****************************/

/*********************** Global Variable Definitions *************************/

/*************************** Variable Definitions ****************************/

// Hardware LED state
u8                           use_leds;
u8                           red_led_state;
u8                           green_led_state;


/*************************** Functions Prototypes ****************************/

/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * Process Node Commands
 *
 * This function is part of the Ethernet processing system and will process the
 * various node related commands.
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
 * @note    See on-line documentation for more information about the Ethernet
 *          packet structure for WARPLab:  www.warpproject.org
 *
 *****************************************************************************/
int node_process_cmd(int socket_index, void * from, wl_cmd_resp * command, wl_cmd_resp * response) {

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
    u32                 eth_dev_num;
    int                 status;
    int                 numblinks;
    u8                  node_ip_addr[IP_ADDR_LEN];
    u8                  node_hw_addr[ETH_MAC_ADDR_LEN];

    u32                 msg_cmd;
    u32                 default_response;

    u32                 mem_addr;
    u32                 mem_length;
    u32                 mem_index;

    // Set up the response header
    resp_hdr->cmd       = cmd_hdr->cmd;
    resp_hdr->length    = 0;
    resp_hdr->num_args  = 0;

    // Get the Ethernet device number of the socket
    eth_dev_num         = socket_get_eth_dev_num(socket_index);

    // Populate the IP / MAC addresses of the Ethernet device
    eth_get_ip_addr(eth_dev_num, (u8 *)&node_ip_addr);
    eth_get_hw_addr(eth_dev_num, (u8 *)&node_hw_addr);

    // Process the command
    switch(cmd_id){

        //---------------------------------------------------------------------
        case CMDID_NODE_INITIALIZE:

            // Set the decimal point of the rigth hex display
            userio_write_hexdisp_right(USERIO_BASEADDR, (userio_read_hexdisp_right( USERIO_BASEADDR ) | W3_USERIO_HEXDISP_DP ) );

            // Initialize all the sub-systems of the node.
            //     NOTE:  The error condition is the same as during main()
            //
            status = global_initialize();

            if(status != XST_SUCCESS) {
                wl_printf(WL_PRINT_ERROR, print_type_node, "Error in global_initialize()! Exiting...\n");
                set_node_error_status(0x2);                     // Set user IO
                blink_node(0, 250000);                          // Infinite blink
            }
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_INFO:

            // Send all node information
            //     NOTE:  This must match the expectations of the host

            resp_args_32[resp_index++] = Xil_Htonl(w3_eeprom_readSerialNum(EEPROM_BASEADDR));
            resp_args_32[resp_index++] = Xil_Htonl(userio_read_fpga_dna_msb(USERIO_BASEADDR));
            resp_args_32[resp_index++] = Xil_Htonl(userio_read_fpga_dna_lsb(USERIO_BASEADDR));
            resp_args_32[resp_index++] = Xil_Htonl( (node_hw_addr[0] <<  8) |  node_hw_addr[1] );
            resp_args_32[resp_index++] = Xil_Htonl( (node_hw_addr[2] << 24) | (node_hw_addr[3] << 16) | (node_hw_addr[4] << 8) | node_hw_addr[5] );
            resp_args_32[resp_index++] = Xil_Htonl((3 << 24)|(WARPLAB_VER_MAJOR << 16)|(WARPLAB_VER_MINOR << 8)|(WARPLAB_VER_REV));
            resp_args_32[resp_index++] = Xil_Htonl((wl_bb_get_supported_tx_length() + 1));
            resp_args_32[resp_index++] = Xil_Htonl((wl_bb_get_supported_rx_length() + 1));
            resp_args_32[resp_index++] = Xil_Htonl((wl_bb_get_tx_length() + 1));
            resp_args_32[resp_index++] = Xil_Htonl((wl_bb_get_rx_length() + 1));
            resp_args_32[resp_index++] = Xil_Htonl(trigger_proc_get_core_info());
            resp_args_32[resp_index++] = Xil_Htonl(1);                  // num_interfaceGroups

            // Set the number of interfaces
            #if WARPLAB_CONFIG_4RF
                resp_args_32[resp_index++] = Xil_Htonl(4);
            #else
                resp_args_32[resp_index++] = Xil_Htonl(2);
            #endif

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_IDENTIFY:

            // Send the response early so that M-Code does not hang waiting for the node to stop blinking
            //
            //     NOTE:  The host must wait the appropriate amount of time (see below) to send another
            //       command.  Otherwise, the node will possibly cause a transport timeout given that it
            //       is still busy with this command and will ignore subsequent commands until it is done
            //       with this command.
            //
            node_send_early_resp(socket_index, from, response->header, response->buffer);

            // Set LED initial state:  Green = "ALL OFF"; Red = "ALL ON"
            userio_write_leds_green(USERIO_BASEADDR, 0x0);
            userio_write_leds_red(USERIO_BASEADDR, 0xF);

            // Currently, this code will toggle the Red and Green LEDs for "numblinks" times and pause for 0.1
            // seconds between each loop.  We have chosen the magic number of 10 "blinks", which results in this
            // command taking roughly 1 second.  The reason for this choice is:  1) a second is long enough to
            // easily see which node is blinking but not overly long; 2) currently the transport timeout is 1
            // second.  By making this command roughly equivalent to the transport timeout, we limit the
            // probability that the host will error out if it doesn't wait after the 'identify' command to send
            // the next command due to the fact that the transport retransmits commands once after a timeout.
            //
            for (numblinks = 0; numblinks < 10; numblinks++) {
                userio_toggle_leds_red(USERIO_BASEADDR, 0xF);
                userio_toggle_leds_green(USERIO_BASEADDR, 0xF);
                usleep(100000);
            }

            // Set LEDs back to "ALL OFF"
            userio_write_leds_red(USERIO_BASEADDR, 0x0);
            userio_write_leds_green(USERIO_BASEADDR, 0x0);

            // Tell the transport that the command has already sent a response
            resp_sent = RESP_SENT;
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_TEMPERATURE:

            #ifdef XPAR_XSYSMON_NUM_INSTANCES
                resp_args_32[resp_index++] = Xil_Htonl(XSysMon_ReadReg(SYSMON_BASEADDR, XSM_TEMP_OFFSET));
                resp_args_32[resp_index++] = Xil_Htonl(XSysMon_ReadReg(SYSMON_BASEADDR, XSM_MIN_TEMP_OFFSET));
                resp_args_32[resp_index++] = Xil_Htonl(XSysMon_ReadReg(SYSMON_BASEADDR, XSM_MAX_TEMP_OFFSET));
            #else
                resp_args_32[resp_index++] = 0;
                resp_args_32[resp_index++] = 0;
                resp_args_32[resp_index++] = 0;
            #endif

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_CONFIG_SETUP:
            // NODE_CONFIG_SETUP Packet Format:
            //   - Note:  All u32 parameters in cmd_args_32 are byte swapped so use Xil_Ntohl()
            //
            //   - cmd_args_32[0] - Serial Number
            //   - cmd_args_32[1] - Node ID
            //   - cmd_args_32[2] - IP Address
            //
            // NOTE:  This command will only execute on the node if it is in the "Network Reset" state
            //   (ie the node is set to 0xFFFF).
            //
            if (node == 0xFFFF) {

                // Only update the parameters if the serial numbers match
                if (w3_eeprom_readSerialNum(EEPROM_BASEADDR) ==  Xil_Ntohl(cmd_args_32[0])) {

                    node = Xil_Ntohl(cmd_args_32[1]) & 0xFFFF;

                    wl_printf(WL_PRINT_NONE, NULL, "  New Node ID   : %d \n", node);

                    // Set the hex display to the new Node ID
                    userio_write_control(USERIO_BASEADDR, (userio_read_control(USERIO_BASEADDR) | (W3_USERIO_HEXDISP_L_MAPMODE | W3_USERIO_HEXDISP_R_MAPMODE)));
                    userio_write_hexdisp_left(USERIO_BASEADDR, (node / 10));
                    userio_write_hexdisp_right(USERIO_BASEADDR, (node % 10));

                    // Get the new IP address
                    node_ip_addr[0] = (Xil_Ntohl(cmd_args_32[2]) >> 24) & 0xFF;
                    node_ip_addr[1] = (Xil_Ntohl(cmd_args_32[2]) >> 16) & 0xFF;
                    node_ip_addr[2] = (Xil_Ntohl(cmd_args_32[2]) >>  8) & 0xFF;
                    node_ip_addr[3] = (Xil_Ntohl(cmd_args_32[2])      ) & 0xFF;

                    wl_printf(WL_PRINT_NONE, NULL, "  New IP Address: %d.%d.%d.%d \n", node_ip_addr[0], node_ip_addr[1],node_ip_addr[2],node_ip_addr[3]);

                    // Configure the transport with the node ID and IP address
                    eth_set_ip_addr(eth_dev_num, node_ip_addr);

                    status = transport_config_sockets(eth_dev_num, (NODE_UDP_UNICAST_PORT_BASE + node), NODE_UDP_MCAST_BASE);

                    if(status != XST_SUCCESS) {
                        wl_printf(WL_PRINT_ERROR, print_type_node, "Error binding transport...\n");
                    }

                } else {
                    wl_printf(WL_PRINT_INFO, print_type_node, "NODE_IP_SETUP Packet with Serial Number %d ignored.  My serial number is %d \n", Xil_Ntohl(cmd_args_32[0]), w3_eeprom_readSerialNum(EEPROM_BASEADDR));
                }
            }
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_CONFIG_RESET:
            // NODE_CONFIG_RESET Packet Format:
            //   - Note:  All u32 parameters in cmd_args_32 are byte swapped so use Xil_Ntohl()
            //
            //   - cmd_args_32[0] - Serial Number
            //

            // Send the response early so that M-Code does not hang when IP address changes
            node_send_early_resp(socket_index, from, response->header, response->buffer);

            // Only update the parameters if the serial numbers match
            if (w3_eeprom_readSerialNum(EEPROM_BASEADDR) ==  Xil_Ntohl(cmd_args_32[0])) {

                // Reset node to 0xFFFF
                node = 0xFFFF;

                wl_printf(WL_PRINT_NONE, NULL, "\n!!! Reseting Network Configuration !!! \n\n");

                // Reset transport;  This will update the IP Address back to default and rebind the sockets
                transport_get_hw_info(eth_dev_num, node_ip_addr, node_hw_addr);

                eth_set_ip_addr(eth_dev_num, node_ip_addr);

                status = transport_config_sockets(eth_dev_num, NODE_UDP_UNICAST_PORT_BASE, NODE_UDP_MCAST_BASE);

                if(status != XST_SUCCESS) {
                    wl_printf(WL_PRINT_ERROR, print_type_node, "Error binding transport...\n");
                }

                // Update User IO
                wl_printf(WL_PRINT_NONE, NULL, "\n!!! Waiting for Network Configuration via Matlab !!! \n\n");

                // Turn off hex mapping; set the center LED: "--"
                //     NOTE:  The hex mapping will be re-enabled when the broadcast packet is processed to set the node ID
                //
                userio_write_control(USERIO_BASEADDR, (userio_read_control(USERIO_BASEADDR) & (~(W3_USERIO_HEXDISP_L_MAPMODE | W3_USERIO_HEXDISP_R_MAPMODE))));
                userio_write_hexdisp_left(USERIO_BASEADDR, 0x40);
                userio_write_hexdisp_right(USERIO_BASEADDR, 0x40);

            } else {
                wl_printf(WL_PRINT_INFO, print_type_node, "NODE_IP_RESET Packet with Serial Number %d ignored.  My serial number is %d \n", Xil_Ntohl(cmd_args_32[0]), w3_eeprom_readSerialNum(EEPROM_BASEADDR));
            }

            // Tell the transport that the command has already sent a response
            resp_sent = RESP_SENT;
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_MEM_RW:
            // Read / write arbitrary memory location
            //
            // Write Message format:
            //     cmd_args_32[0]      Command == CMD_PARAM_WRITE_VAL
            //     cmd_args_32[1]      Address
            //     cmd_args_32[2]      Length (number of u32 words to write)
            //     cmd_args_32[3:]     Values to write (integral number of u32 words)
            // Response format:
            //     resp_args_32[0]     Status
            //
            // Read Message format:
            //     cmd_args_32[0]      Command == CMD_PARAM_READ_VAL
            //     cmd_args_32[1]      Address
            //     cmd_args_32[2]      Length (number of u32 words to read)
            // Response format:
            //     resp_args_32[0]     Status
            //     resp_args_32[1]     Length (number of u32 values)
            //     resp_args_32[2:]    Memory values (length u32 values)
            //
            msg_cmd          = Xil_Ntohl(cmd_args_32[0]);
            mem_addr         = Xil_Ntohl(cmd_args_32[1]);
            mem_length       = Xil_Ntohl(cmd_args_32[2]);
            status           = CMD_PARAM_SUCCESS;
            default_response = WL_TRUE;

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    wl_printf(WL_PRINT_INFO, print_type_node, "Write CPU High Mem\n");
                    wl_printf(WL_PRINT_INFO, print_type_node, "  Addr: 0x%08x\n", mem_addr);
                    wl_printf(WL_PRINT_INFO, print_type_node, "  Len:  %d\n", mem_length);

                    // Don't bother if length is clearly bogus
                    if (mem_length < CMD_PARAM_NODE_MEM_RW_MAX_BYTES) {
                        for (mem_index = 0; mem_index < mem_length; mem_index++) {
                            wl_printf(WL_PRINT_INFO, print_type_node, "  W[%2d]: 0x%08x\n", mem_index, Xil_Ntohl(cmd_args_32[3 + mem_index]));
                            Xil_Out32((mem_addr + (mem_index * sizeof(u32))), Xil_Ntohl(cmd_args_32[3 + mem_index]));
                        }
                    } else {
                        wl_printf(WL_PRINT_ERROR, print_type_node, "NODE_MEM_RW write longer than %d bytes\n", CMD_PARAM_NODE_MEM_RW_MAX_BYTES);
                        status = CMD_PARAM_ERROR;
                    }
                break;

                case CMD_PARAM_READ_VAL:
                    wl_printf(WL_PRINT_INFO, print_type_node, "Read CPU High Mem:\n");
                    wl_printf(WL_PRINT_INFO, print_type_node, "  Addr: 0x%08x\n", mem_addr);
                    wl_printf(WL_PRINT_INFO, print_type_node, "  Len:  %d\n", mem_length);

                    // Add payload to response
                    if(mem_length < CMD_PARAM_NODE_MEM_RW_MAX_BYTES) {

                        // Don't set the default response / Don't send any response
                        default_response = WL_FALSE;

                        // Add length argument to response
                        resp_args_32[resp_index++] = Xil_Htonl(status);
                        resp_args_32[resp_index++] = Xil_Htonl(mem_length);

                        resp_hdr->length   += (resp_index * sizeof(resp_args_32));
                        resp_hdr->num_args  = resp_index;

                        for (mem_index = 0; mem_index < mem_length; mem_index++) {
                            resp_args_32[resp_index + mem_index] = Xil_Ntohl(Xil_In32((void*)(mem_addr) + (mem_index * sizeof(u32))));
                        }

                        // Update response header with payload length
                        resp_hdr->length   += (mem_length * sizeof(u32));
                        resp_hdr->num_args += mem_length;
                    } else {
                        wl_printf(WL_PRINT_ERROR, print_type_node, "NODE_MEM_RW read longer than %d bytes\n", CMD_PARAM_NODE_MEM_RW_MAX_BYTES);
                        status = CMD_PARAM_ERROR;
                    }
                break;

                default:
                    wl_printf(WL_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            if (default_response == WL_TRUE) {
                // Send default response
                resp_args_32[resp_index++] = Xil_Htonl(status);

                resp_hdr->length  += (resp_index * sizeof(resp_args_32));
                resp_hdr->num_args = resp_index;
            }
        break;


        //---------------------------------------------------------------------
        default:
            wl_printf(WL_PRINT_ERROR, print_type_node, "Unknown node command: %d\n", cmd_id);
        break;
    }

    return resp_sent;
}



/*****************************************************************************/
/**
 * Node Clock Initialization Function
 *
 * This function will initialize the clock module
 *
 * @param   None
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 * @note     This function works with w3_clock_controller v4.00.a
 *
 *****************************************************************************/
int node_clk_initialize() {
    int status               = XST_SUCCESS;
    u32 clkmod_status;

    // Initialize w3_clock_controller hardware and AD9512 buffers
    //   NOTE:  The clock initialization will set the clock divider to 2 (for 40MHz clock) to RF A/B AD9963's
    status = clk_init(CLK_BASEADDR, 2);
    if(status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "Clock initialization failed with error code: %d\n", status);
        return XST_FAILURE;
    }

    // Check for a clock module and configure clock inputs, outputs and dividers as needed
    clkmod_status = clk_config_read_clkmod_status(CLK_BASEADDR);

    switch(clkmod_status & CM_STATUS_SW) {
        case CM_STATUS_DET_NOCM:
        case CM_STATUS_DET_CMPLL_BYPASS:
            // No clock module - default config from HDL/driver is good as-is
            wl_printf(WL_PRINT_NONE, NULL, "No clock module detected - selecting on-board clocks\n\n");
        break;

        case CM_STATUS_DET_CMMMCX_CFG_A:
            // CM-MMCX config A:
            //     Samp clk: on-board, RF clk: on-board
            //     Samp MMCX output: 80MHz, RF MMCX output: 80MHz
            wl_printf(WL_PRINT_NONE, NULL, "CM-MMCX Config A Detected:\n");
            wl_printf(WL_PRINT_NONE, NULL, "  RF: On-board\n  Samp: On-board\n  MMCX Outputs: Enabled\n\n");

            clk_config_outputs(CLK_BASEADDR, CLK_OUTPUT_ON, (CLK_SAMP_OUTSEL_CLKMODHDR | CLK_RFREF_OUTSEL_CLKMODHDR));
            clk_config_dividers(CLK_BASEADDR, 1, CLK_SAMP_OUTSEL_CLKMODHDR | CLK_RFREF_OUTSEL_CLKMODHDR);
        break;

        case CM_STATUS_DET_CMMMCX_CFG_B:
            // CM-MMCX config B:
            //     Samp clk: off-board, RF clk: off-board
            //     Samp MMCX output: 80MHz, RF MMCX output: 80MHz
            wl_printf(WL_PRINT_NONE, NULL, "CM-MMCX Config B Detected:\n");
            wl_printf(WL_PRINT_NONE, NULL, "  RF: Off-board\n  Samp: Off-board\n  MMCX Outputs: Enabled\n\n");

            clk_config_input_rf_ref(CLK_BASEADDR, CLK_INSEL_CLKMOD);
            clk_config_outputs(CLK_BASEADDR, CLK_OUTPUT_ON, (CLK_SAMP_OUTSEL_CLKMODHDR | CLK_RFREF_OUTSEL_CLKMODHDR));
            clk_config_dividers(CLK_BASEADDR, 1, (CLK_SAMP_OUTSEL_CLKMODHDR | CLK_RFREF_OUTSEL_CLKMODHDR));
        break;

        case CM_STATUS_DET_CMMMCX_CFG_C:
            // CM-MMCX config C:
            //     Samp clk: off-board, RF clk: off-board
            //     Samp MMCX output: Off, RF MMCX output: Off
            wl_printf(WL_PRINT_NONE, NULL, "CM-MMCX Config C Detected:\n");
            wl_printf(WL_PRINT_NONE, NULL, "  RF: Off-board\n  Samp: Off-board\n  MMCX Outputs: Disabled\n\n");

            clk_config_input_rf_ref(CLK_BASEADDR, CLK_INSEL_CLKMOD);
            clk_config_outputs(CLK_BASEADDR, CLK_OUTPUT_OFF, (CLK_SAMP_OUTSEL_CLKMODHDR | CLK_RFREF_OUTSEL_CLKMODHDR));
        break;

        case CM_STATUS_DET_CMPLL_CFG_A:
            // CM-PLL config A:
            //     Samp clk: clock module PLL
            //     RF clk: on-board
            wl_printf(WL_PRINT_NONE, NULL, "CM-PLL Config A Detected:\n");
            wl_printf(WL_PRINT_NONE, NULL, "  RF: On-board\n  Samp: clock module PLL\n");

            // No changes from configuration applied by HDL and clk_init()
        break;

        case CM_STATUS_DET_CMPLL_CFG_B:
            // CM-PLL config B:
            //     Samp clk: clock module PLL
            //     RF clk: clock module PLL
            wl_printf(WL_PRINT_NONE, NULL, "CM-PLL Config B Detected:\n");
            wl_printf(WL_PRINT_NONE, NULL, "  RF: clock module PLL\n  Samp: clock module PLL\n");

            clk_config_input_rf_ref(CLK_BASEADDR, CLK_INSEL_CLKMOD);
        break;

        case CM_STATUS_DET_CMPLL_CFG_C:
            // CM-PLL config C:
            //     Samp clk: clock module PLL
            //     RF clk: clock module PLL
            wl_printf(WL_PRINT_NONE, NULL, "CM-PLL Config C Detected:\n");
            wl_printf(WL_PRINT_NONE, NULL, "  RF: clock module PLL\n  Samp: clock module PLL\n");

            clk_config_input_rf_ref(CLK_BASEADDR, CLK_INSEL_CLKMOD);
        break;

        default:
            // Should be impossible
            wl_printf(WL_PRINT_ERROR, print_type_node, "ERROR: Invalid clock module switch settings! (0x%08x)\n", clkmod_status);
            return XST_FAILURE;
        break;
    }

#if WARPLAB_CONFIG_4RF
    // Turn on clocks to FMC
    clk_config_outputs(CLK_BASEADDR, CLK_OUTPUT_ON, (CLK_SAMP_OUTSEL_FMC | CLK_RFREF_OUTSEL_FMC));

    // FMC samp clock divider = 2 (40MHz sampling reference, same as on-board AD9963 ref clk)
    clk_config_dividers(CLK_BASEADDR, 2, CLK_SAMP_OUTSEL_FMC);

    // FMC RF ref clock divider = 2 (40MHz RF reference, same as on-board MAX2829 ref clk)
    clk_config_dividers(CLK_BASEADDR, 2, CLK_RFREF_OUTSEL_FMC);
#endif

    return status;
}



/*****************************************************************************/
/**
 * Node Initialization Function
 *
 * This function will initialize many aspects of the node.
 *
 * @param   None.
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int node_init(){
    int status               = XST_SUCCESS;
    u64 timestamp;

    // Configure Microblaze
    microblaze_enable_exceptions();
    Xil_DCacheDisable();
    Xil_ICacheDisable();

    // Initialize hardware components
    wl_timer_initialize();
    wl_gpio_debug_initialize();
    wl_sysmon_initialize();
    wl_uart_initialize();
    iic_eeprom_init(EEPROM_BASEADDR, 0x64);

    // Initialize LED global variables
    use_leds        = 1;
    red_led_state   = 0;
    green_led_state = 0;

    // Initialize the central DMA (CDMA) driver
    //     NOTE:  Need to die if not successful
    status = wl_cdma_initialize();
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Initialize the hardware clocking
    status = node_clk_initialize();
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Populate the Node ID
    node = userio_read_inputs(USERIO_BASEADDR) & W3_USERIO_DIPSW;
        
    // If the node has dip switch value of 0xF, then set the node to 0xFFFF
    if ( node == 0xF ) {
        node = 0xFFFF;
    } else {
        userio_write_hexdisp_left(USERIO_BASEADDR, ((node+1)/10) );
        userio_write_hexdisp_right(USERIO_BASEADDR, (node+1)%10);
    }

    // Check the WARPLab version
    if( (wl_get_design_ver()&0xFFFF00) != ((REQ_WARPLAB_HW_VER)&0xFFFF00) ){
        wl_printf(WL_PRINT_ERROR, print_type_node, "HW/SW Version Mismatch! Expected HW Ver: 0x%x -- Actual HW Ver: 0x%x\n\n", (REQ_WARPLAB_HW_VER), wl_get_design_ver());
        return XST_FAILURE;
    }

    // Print node information
    wl_printf(WL_PRINT_NONE, print_type_node, "W3-a-%05d using Node ID: %d\n", w3_eeprom_readSerialNum(EEPROM_BASEADDR), node);

    // Test to see if DRAM SODIMM is connected to board
    timestamp    = get_usec_timestamp();

    while((get_usec_timestamp() - timestamp) < 100000){
        if(wl_get_dram_init_done() == 1){
            wl_printf(WL_PRINT_NONE, NULL, "DRAM SODIMM detected ... \n");

            if(ddr_sodim_memory_test() == XST_SUCCESS){
                dram_present = 1;

                if (CLEAR_DDR_ON_BOOT) {
                    clear_ddr(WL_VERBOSE);
                } else {
                    wl_printf(WL_PRINT_NONE, NULL, "  Contents not cleared\n");
                }
            } else {
                dram_present = 0;
                wl_printf(WL_PRINT_NONE, NULL, "  Memory test failed; Will not use DRAM\n");
            }
            break;
        }
    }

    if (wl_get_dram_init_done() != 1) {
        wl_printf(WL_PRINT_NONE, NULL, "DRAM SODIMM not detected.\n");
    }

    return status;
}



/*****************************************************************************/
/**
 * Set Node Error Status
 *
 * This function will set the LEDs to be 0x5 and then set the hex display
 * to Ex, where x is the value of the status error
 *
 * @param   status           - Number from 0 - 0xF to indicate status error
 *
 * @return  None
 *
 *****************************************************************************/
void set_node_error_status(int status) {

    userio_write_leds_red(USERIO_BASEADDR, 0x5);
    userio_write_hexdisp_left(USERIO_BASEADDR, 0xE);
    userio_write_hexdisp_right(USERIO_BASEADDR, status);
}



/*****************************************************************************/
/**
 * LED Functions
 *
 * Blink Node:
 *     For WARP v3 Hardware, this function will toggle LEDs.  The pattern depends
 *         on the value of the LEDs before the function was called.
 *
 * @param   num_blinks       - Number of blinks (0 means blink forever)
 *          blink_time       - Time in us between blinks
 *
 * @return  None
 *
 *****************************************************************************/
void blink_node(int num_blinks, int blink_time) {
    int i;
    use_leds = 0;

    if ( num_blinks > 0 ) {
        // Perform standard blink
        for( i = 0; i < num_blinks; i++ ) {
            userio_toggle_leds_green(USERIO_BASEADDR, 0xF);
            usleep( blink_time );
        }
    } else {
        // Perform an infinite blink
        while(1){
            userio_toggle_leds_red(USERIO_BASEADDR, 0xF);
            usleep( blink_time );
        }
    }

    use_leds = 1;
}


// Increment the green LEDs in a one-hot manner (ie 0x1 -> 0x2 -> 0x4 -> 0x8 -> 0x1 ...)
void increment_green_leds_one_hot() {
    if (use_leds) {
        userio_write_leds_green(USERIO_BASEADDR, (1 << green_led_state));
        green_led_state = (green_led_state + 1) % 4;
    }
}


// Increment the red LEDs in a one-hot manner (ie 0x1 -> 0x2 -> 0x4 -> 0x8 -> 0x1 ...)
void increment_red_leds_one_hot() {
    if (use_leds) {
        userio_write_leds_red(USERIO_BASEADDR, (1 << red_led_state));
        red_led_state = (red_led_state + 1) % 4;
    }
}



/*****************************************************************************/
/**
 * Process Received UART characters
 *
 * Due to the need to use interrupts for IQ processing, we decided to also connect and
 * initialize the UART interrupt.  This allows the user to communicate with the board
 * via a terminal program, such as Putty.  Each character typed in the terminal program
 * will cause the function below to be called with the ACSII value of the character.
 *
 * Currently, a few characters have been allocated for various debug modes, but no
 * characters have been officially allocated for particular functions.  Therefore, any
 * user can feel free to extend this method if needed for their experiment.  By default,
 * the node will echo all characters back to the terminal.
 *
 * @param   rx_byte          - Character received from the UART via the terminal program on the host
 *
 * @return  None
 *
 * @note    Some characters have been allocated for various debug functions that are enabled
 *     through defines found in wl_node.c and wl_common.h:
 *
 *     _DEBUG_STORAGE_
 *         - 'c'   - Clear the debug storage (ie reset the storage)
 *         - 'p'   - Print the contents of the debug storage
 *
 *     ALLOW_ETHERNET_PAUSE
 *         - 's'   - Toggles the state of Ethernet receptions; Will either pause or un-pause
 *                   the reception of Ethernet packets based on the current state.  By default,
 *                   Ethernet receptions are un-paused.
 *
 *****************************************************************************/
void uart_rx(u8 rx_byte){
    char character = rx_byte;

    // Process the received character
    switch(character) {

#if _DEBUG_STORAGE_
        case 'c':
            reset_debug_storage();
        break;

        case 'p':
            print_debug_storage();
        break;
#endif

#if ALLOW_ETHERNET_PAUSE
        case 's':
            if (ethernet_pause == 1) {
                ethernet_pause = 0;
            } else {
                ethernet_pause = 1;
            }
        break;
#endif

        // Echo any unknown characters back to the terminal
        default:
            wl_printf(WL_PRINT_NONE, NULL, "%c", rx_byte);
        break;
    }
}



/*****************************************************************************/
/**
 * @brief Node initialization
 *
 * This is the main function of the embedded C code.  It will initialize the
 * board and then begin a infinite polling loop on the Ethernet peripheral to
 * process any commands that are sent to the board.
 *
 * @param   None
 *
 * @return  None.  Implements an infinite while loop
 *
 * @note    The hex display values during boot should be as follows:
 *              OFF      - Bit stream is being downloaded to the board
 *              00       - Initial power up of the downloaded bit stream
 *              01 to 99 - ID value of the node.
 *              --       - Node is ready to recieve network configuration
 *              Ex       - Error condition where x is the value of the status error
 *                         Please plug in a USB cable for further debug messages
 *
 *          A value of 01 to 99 or -- indicates a successful boot and is ready to
 *          receive commands.
 *
 ******************************************************************************/
int main() {
    int            status;
    u32            tmp_eth_dev_num;
    volatile u32   link_status         = LINK_NOT_READY;
    u32            init_transport      = 1;                // Does the transport need to be initialized

    // ------------------------------------------
    // Initialize global variables
    //
    dram_present             = 0;
    configure_buffers        = 1;

#if ALLOW_ETHERNET_PAUSE
    ethernet_pause           = 0;
#endif

    // Set the print level
    wl_set_print_level(DEFAULT_DEBUG_PRINT_LEVEL);


    // ------------------------------------------
    // Print initial message to UART
    //
    wl_printf(WL_PRINT_NONE, NULL, "\fWARPLab v%d.%d.%d (compiled %s %s)\n", WARPLAB_VER_MAJOR, WARPLAB_VER_MINOR, WARPLAB_VER_REV, __DATE__, __TIME__);

    if(WARPLAB_CONFIG_4RF) {
        wl_printf(WL_PRINT_NONE, NULL, "Configured for 4 RF Interfaces - FMC-RF-2X245 FMC module must be installed\n");
    } else {
        wl_printf(WL_PRINT_NONE, NULL, "Configured for 2 RF Interfaces - Using WARP v3 on-board RF interfaces\n");
    }


    // ------------------------------------------
    // Check that right shift works correctly
    //   Issue with -Os in Xilinx SDK 14.7
    if (microblaze_right_shift_test() != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "Node right shift error! Exiting...\n");
        set_node_error_status(0x0);                        // Set user IO
        blink_node(0, 250000);                             // Infinite blink
    }


    // ------------------------------------------
    // Node initialization
    //   NOTE:  These errors are fatal and status error will be displayed
    //       on the hex display.  Also, please attach a USB cable for
    //       terminal debug messages.
    //
    status = node_init();

    if(status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "Node initialization error! Exiting...\n");
        set_node_error_status(0x1);                        // Set user IO
        blink_node(0, 250000);                             // Infinite blink
    }


    // ------------------------------------------
    // Global initialization
    //   NOTE:  These errors are fatal and status error will be displayed
    //       on the hex display.  Also, please attach a USB cable for
    //       terminal debug messages.
    //
    status = global_initialize();

    if(status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "Global initialization error! Exiting...\n");
        set_node_error_status(0x2);                        // Set user IO
        blink_node(0, 250000);                             // Infinite blink
    }


    // ------------------------------------------
    // Transport initialization
    //   NOTE:  These errors are fatal and status error will be displayed
    //       on the hex display.  Also, please attach a USB cable for
    //       terminal debug messages.
    //
    if (WL_USE_ETH_A) {
        status = transport_init(WL_ETH_A, init_transport);
        init_transport = 0;
    }

    if ((status == XST_SUCCESS) && WL_USE_ETH_B) {
        status = transport_init(WL_ETH_B, init_transport);
        init_transport = 0;
    }

    if(status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "Transport initialization error! Exiting...\n");
        set_node_error_status(0x3);                        // Set user IO
        blink_node(0, 250000);                             // Infinite blink
    }

    // A common error that can occur when regenerating the linker script, is that the linker
    // script puts the global data structures for the WARP IP/UDP transport in to a memory
    // that is not accessible by the Ethernet DMA. Unfortunately, this is an extremely fatal
    // error, so this check was added to catch this.
    //
    if (WL_USE_ETH_A) {
        status = eth_not_in_memory_range(WL_ETH_A, XPAR_MICROBLAZE_0_D_BRAM_CTRL_HIGHADDR, XPAR_MICROBLAZE_0_D_BRAM_CTRL_BASEADDR);
        tmp_eth_dev_num = WL_ETH_A;
    }

    if ((status == WARP_IP_UDP_SUCCESS) && WL_USE_ETH_B) {
        status = eth_not_in_memory_range(WL_ETH_B, XPAR_MICROBLAZE_0_D_BRAM_CTRL_HIGHADDR, XPAR_MICROBLAZE_0_D_BRAM_CTRL_BASEADDR);
        tmp_eth_dev_num = WL_ETH_B;
    }

    if(status != WARP_IP_UDP_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "Ethernet device %c: \n", warp_conv_eth_dev_num(tmp_eth_dev_num));
        wl_printf(WL_PRINT_ERROR, print_type_node, "    Global data structures not accessible by DMA.\n\n");
        wl_printf(WL_PRINT_ERROR, print_type_node, "Please update your linker command file to put buffers in shared BRAM.  Exiting...\n");
        set_node_error_status(0x4);                        // Set user IO
        blink_node(0, 250000);                             // Infinite blink
    }


    // ------------------------------------------
    // Interrupt initialization
    //   NOTE:  These errors are fatal and status error will be displayed
    //       on the hex display.  Also, please attach a USB cable for
    //       terminal debug messages.
    status = wl_interrupt_init();

    if(status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "Interrupt initialization error! Exiting...\n");
        set_node_error_status(0x5);                        // Set user IO
        blink_node(0, 250000);                             // Infinite blink
    }


    // ------------------------------------------
    // Wait for Ethernet to finish initializing the link
    //
    if (WL_WAIT_FOR_ETH) {
        wl_printf(WL_PRINT_NONE, NULL, "\nWaiting for Ethernet link ...\n");

        while(link_status == LINK_NOT_READY) {

            // Check the link status of each Ethernet device in use
            if (((transport_link_status(WL_ETH_A) == LINK_NOT_READY) && WL_USE_ETH_A) ||
                ((transport_link_status(WL_ETH_B) == LINK_NOT_READY) && WL_USE_ETH_B)) {

                link_status = LINK_NOT_READY;
            } else {
                link_status = LINK_READY;
            }

            // Update LEDs for a visual cue that we are waiting on the Ethernet device
            userio_toggle_leds_green(USERIO_BASEADDR, 0x1);
            usleep(100000);
        }

    } else {
        xil_printf("  Not waiting for Ethernet link.  Current status:\n");

        if ((transport_link_status(WL_ETH_A) == LINK_READY) && WL_USE_ETH_A) {
            xil_printf("    ETH A ready\n");
        } else {
            xil_printf("    ETH A not ready\n");
        }

        if ((transport_link_status(WL_ETH_B) == LINK_READY) && WL_USE_ETH_B) {
            xil_printf("    ETH B ready\n");
        } else {
            xil_printf("    ETH B not ready\n");
        }

        xil_printf("\n    Make sure link is ready before using WARPLab.\n");
    }

    wl_printf(WL_PRINT_NONE, NULL, "\nInitialization Successful - Waiting for Commands from MATLAB\n\n");


    // ------------------------------------------
    // Assign the transport receive callback (how to process received Ethernet packets)
    //     IMPORTANT: Must be called after transport_init()
    transport_set_process_hton_msg_callback((void *)node_rx_from_transport);


    // ------------------------------------------
    // Assign the uart receive callback (how to process received uart characters)
    //     IMPORTANT: Must be called after node_init()
    wl_set_uart_rx_callback((void*)uart_rx);


    // ------------------------------------------
    // Enable all interrupts
    status = wl_interrupt_restore_state(INTERRUPTS_ENABLED);

    if(status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "Cannot enable interrupts! Exiting...\n");
        set_node_error_status(0x6);                        // Set user IO
        blink_node(0, 250000);                             // Infinite blink
    }


    // ------------------------------------------
    // Blink LEDs to show we are done
    //
    userio_write_leds_green(USERIO_BASEADDR, 0x5);
    blink_node(10, 100000);                                // Blink 10 times
    userio_write_leds_red(USERIO_BASEADDR, 0x0);
    userio_write_leds_green(USERIO_BASEADDR, 0x0);

    // If you are in configure over network mode, then indicate that to the user
    if ( node == 0xFFFF ) {
        wl_printf(WL_PRINT_NONE, NULL, "!!! Waiting for Network Configuration via Matlab !!! \n\n");

        // Turn off hex mapping; set the center LED
        //     NOTE:  hex mapping will be re-enabled when the bcast packet is processed to set the node ID
        userio_write_control(USERIO_BASEADDR, (userio_read_control(USERIO_BASEADDR) & (~(W3_USERIO_HEXDISP_L_MAPMODE | W3_USERIO_HEXDISP_R_MAPMODE))));
        userio_write_hexdisp_left(USERIO_BASEADDR, 0x40);
        userio_write_hexdisp_right(USERIO_BASEADDR, 0x40);
    }


    // ------------------------------------------
    // Go into infinite while loop
    //
    while(1){

#if ALLOW_ETHERNET_PAUSE
        if (ethernet_pause) {
            // Indicate visually to the user that the node is not accepting Ethernet packets
            increment_red_leds_one_hot();
            usleep(100000);
        } else {
            // Process Ethernet packets.
            //     NOTE:  This is polling based and not interrupt based.
            //     NOTE:  #if is used in this case vs standard if() statement since this section of
            //            code is performance critical.
            //
#if WL_USE_ETH_A
            transport_poll(WL_ETH_A);
#endif
#if WL_USE_ETH_B
            transport_poll(WL_ETH_B);
#endif
        }
#else
        // Process Ethernet packets.
        //     NOTE:  This is polling based and not interrupt based.
        //     NOTE:  #if is used in this case vs standard if() statement since this section of
        //            code is performance critical.
        //
#if WL_USE_ETH_A
        transport_poll(WL_ETH_A);
#endif
#if WL_USE_ETH_B
        transport_poll(WL_ETH_B);
#endif

#endif

    }

    return XST_SUCCESS;
}

#endif

//
//
// NOTE:  As of WARPLab 7.6.0, the code associated with WARP v2 has been removed.  If you are using WARP v2, please use
//   WARPLab 7.5.1 or earlier.
//
//
