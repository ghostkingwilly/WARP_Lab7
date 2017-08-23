/** @file wl_transport.c
 *  @brief WARPLab Framework (Transport)
 *
 *  This contains the code for WARPLab Framework transport.
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
#include <xparameters.h>
#include <xil_io.h>
#include <xstatus.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// WARPLab includes
#include "wl_common.h"
#include "wl_transport.h"
#include "wl_trigger_manager.h"
#include "wl_node.h"


/*************************** Constant Definitions ****************************/

/*********************** Global Variable Definitions *************************/

extern u16                   node;                         // Node ID (defined in wl_node.c)


/*************************** Variable Definitions ****************************/

// NOTE:  This structure has different member types depending on the WARP version
wl_eth_dev_info              eth_devices[WL_NUM_ETH_DEVICES];

// Callbacks
volatile wl_function_ptr_t   process_hton_msg_callback;


/*************************** Function Prototypes *****************************/

void transport_wl_eth_dev_info_init(u32 eth_dev_num);

int  transport_check_device(u32 eth_dev_num);

void transport_receive(u32 eth_dev_num, int socket_index, struct sockaddr * from, warp_ip_udp_buffer * recv_buffer, warp_ip_udp_buffer * send_buffer);

void transport_set_eth_phy_speed(u32 eth_dev_num, u32 speed);
void transport_set_eth_phy_auto_negotiation(u32 eth_dev_num, u32 enable);


/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * This function will poll the given Ethernet device
 *
 * @param   eth_dev_num      - Ethernet device number
 *
 * @return  None
 *
 * @note    Buffers are managed by the WARP UDP transport driver
 *
 *****************************************************************************/
void transport_poll(u32 eth_dev_num) {

    int                     recv_bytes;
    int                     socket_index;
    warp_ip_udp_buffer      recv_buffer;
    warp_ip_udp_buffer    * send_buffer;
    struct sockaddr         from;

    // Check the socket to see if there is data
    recv_bytes = socket_recvfrom_eth(eth_dev_num, &socket_index, &from, &recv_buffer);

    // If we have received data, then we need to process it
    if (recv_bytes > 0) {
        // Allocate a send buffer from the transport driver
        send_buffer = socket_alloc_send_buffer();

        // Process the received packet
        transport_receive(eth_dev_num, socket_index, &from, &recv_buffer, send_buffer);

        // Need to communicate to the transport driver that the buffers can now be reused
        socket_free_recv_buffer(socket_index, &recv_buffer);
        socket_free_send_buffer(send_buffer);
    }
}



/*****************************************************************************/
/**
 * Process the received UDP packet by the transport
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   socket_index     - Index of the socket on which message was received
 * @param   from             - Pointer to socket address structure from which message was received
 * @param   recv_buffer      - Pointer to transport buffer with received message
 * @param   send_buffer      - Pointer to transport buffer for a node response to the message
 *
 * @return  None
 *
 * @note    If this packet is a host to node message, then the  process_hton_msg_callback
 *          is used to further process the packet.  This method will strip off the
 *          WL transport header for future packet processing.
 *
 *****************************************************************************/
void transport_receive(u32 eth_dev_num, int socket_index, struct sockaddr * from, warp_ip_udp_buffer * recv_buffer, warp_ip_udp_buffer * send_buffer) {

    int                           status;
    u32                           trigger_ethernet_id;
    u16                           dest_id;
    u16                           src_id;
    u16                           seq_num;
    u16                           flags;
    u32                           group_id;

    wl_transport_header         * wl_header_rx    = (wl_transport_header*)(recv_buffer->offset);   // Contains entire Ethernet frame; offset points to UDP payload
    wl_transport_header         * wl_header_tx    = (wl_transport_header*)(send_buffer->offset);   // New buffer for UDP payload

    // Get the transport headers for the send / receive buffers
    //     NOTE:  For the receive buffer, offset points to UDP payload of the Ethernet frame
    //     NOTE:  For the send buffer, the offset points to the start of the buffer but since we will use the
    //            UDP header of the socket to transmit the frame, this is effectively the start of the UDP payload
    //
    wl_header_rx             = (wl_transport_header*)(recv_buffer->offset);
    wl_header_tx             = (wl_transport_header*)(send_buffer->offset);

    // Update the buffers to account for the transport headers
    recv_buffer->offset     += sizeof(wl_transport_header);
    recv_buffer->length     -= sizeof(wl_transport_header);                    // Remaining bytes in receive buffer

    send_buffer->offset     += sizeof(wl_transport_header);
    send_buffer->length     += sizeof(wl_transport_header);                    // Adding bytes to the send buffer
    send_buffer->size       += sizeof(wl_transport_header);                    // Keep size in sync

    // Update the green LEDs for every received packet
    increment_green_leds_one_hot();

    // Process the data based on the packet type
    //     NOTE:  The pkt_type does not need to be endian swapped because it is a u8
    //
    switch(wl_header_rx->pkt_type){

        //-------------------------------
        // Trigger Manager Trigger packet
        //
        case PKTTYPE_TRIGGER:

            // Get the Trigger Ethernet ID from the receive packet
            trigger_ethernet_id  = Xil_Ntohl(*((u32 *)(recv_buffer->offset)));

            // Process the Trigger Ethernet ID
            trigmngr_trigger_in(trigger_ethernet_id, eth_dev_num);
        break;

        //-------------------------------
        // Message from the Host to the Node
        //
        case PKTTYPE_HTON_MSG:
            // Extract values from the received transport header
            //
            dest_id  = Xil_Ntohs(wl_header_rx->dest_id);
            src_id   = Xil_Ntohs(wl_header_rx->src_id);
            seq_num  = Xil_Ntohs(wl_header_rx->seq_num);
            flags    = Xil_Ntohs(wl_header_rx->flags);

            group_id = eth_devices[eth_dev_num].group_id;

            // If this message is not for the given node, then ignore it
            if((dest_id != node) && (dest_id != BROADCAST_DEST_ID) && ((dest_id & (0xFF00 | group_id)) == 0)) { return; }

            // Form outgoing WARPLab header for any outgoing packet in response to this message
            //     NOTE:  The u16/u32 fields here will be endian swapped in transport_send
            //     NOTE:  The length field of the header will be set in transport_send
            //
            wl_header_tx->dest_id  = src_id;
            wl_header_tx->src_id   = node;
            wl_header_tx->pkt_type = PKTTYPE_NTOH_MSG;
            wl_header_tx->seq_num  = seq_num;
            wl_header_tx->flags    = 0;
            wl_header_tx->reserved = 0;

            // Call the callback to further process the recv_buffer
            status = process_hton_msg_callback(socket_index, from, recv_buffer, send_buffer);

            if (send_buffer->size != send_buffer->length) {
                wl_printf(WL_PRINT_WARNING, print_type_transport, "Send buffer length (%d) does not match size (%d)\n", send_buffer->length, send_buffer->size);
            }

            // Based on the status, return a message to the host
            switch(status) {

                //-------------------------------
                // No response has been sent by the node
                //
                case NO_RESP_SENT:
                    // Check if the host requires a response from the node
                    if (flags & TRANSPORT_HDR_ROBUST_FLAG) {

                        // Check that the node has something to send to the host
                        if ((send_buffer->length) > sizeof(wl_transport_header)) {
                            transport_send(socket_index, from, &send_buffer, 1);                        // Send the buffer of data
                        } else {
                            wl_printf(WL_PRINT_WARNING, print_type_transport, "Host requires response but node has nothing to send.\n");
                        }
                    }
                break;

                //-------------------------------
                // A response has already been sent by the node
                //
                case RESP_SENT:
                    // The transport does not need to do anything else
                break;

                //-------------------------------
                // The node is not ready to process the message; host needs to wait and send it again
                //
                case NODE_NOT_READY:
                    wl_printf(WL_PRINT_NONE, NULL, "\nWARNING:  Node not ready for command.\n    Please add a pause() with the appropriate time to your Matlab code.\n\n");

                    // Set the flag to indicate the node is not ready
                    wl_header_tx->flags  = TRANSPORT_HDR_NODE_NOT_READY_FLAG;

                    // Send a packet to the host
                    transport_send(socket_index, from, &send_buffer, 1);                        // Send the buffer of data
                break;

                default:
                    wl_printf(WL_PRINT_ERROR, print_type_transport, "Received unknown status for message: %d\n", status);
                break;
            }
        break;

        default:
            wl_printf(WL_PRINT_ERROR, print_type_transport, "Received packet with unknown packet type: %d\n", (wl_header_rx->pkt_type));
        break;
    }
}



/*****************************************************************************/
/**
 * This function is used to send a message over Ethernet
 *
 * @param   socket_index     - Index of the socket on which to send message
 * @param   to               - Pointer to socket address structure to send message
 * @param   buffers          - Array of transport buffers to send
 * @param   num_buffers      - Number of transport buffers in 'buffers' array
 *
 * @return  None
 *
 * @note    This function requires that the first transport buffer in the 'buffers'
 *          array contain the WARPLab transport header.
 *
 *****************************************************************************/
void transport_send(int socket_index, struct sockaddr * to, warp_ip_udp_buffer ** buffers, u32 num_buffers) {

    u32                   i;
    int                   status;
    wl_transport_header * wl_header_tx;
    u16                   buffer_length = 0;

    // Check that we have a valid socket to send a message on
    if (socket_index == SOCKET_INVALID_SOCKET) {
        wl_printf(WL_PRINT_ERROR, print_type_transport, "Invalid socket.\n");
        return;
    }

    // Initialize the header
    //     NOTE:  We require that the first warp_ip_udp_buffer always contain the wl_transport_header
    //
    wl_header_tx = (wl_transport_header *)(buffers[0]->data);

    // Compute the length
    for (i = 0; i < num_buffers; i++) {
        buffer_length += buffers[i]->size;
    }

    //
    // NOTE:  Through performance testing, we found that it was faster to just manipulate the header
    //   in place vs creating a copy, updating the header and then restoring the copy.
    //

    // Make the outgoing transport header endian safe for sending on the network
    //     NOTE:  Set the 'length' to the computed value above
    //
    wl_header_tx->dest_id = Xil_Htons(wl_header_tx->dest_id);
    wl_header_tx->src_id  = Xil_Htons(wl_header_tx->src_id);
    wl_header_tx->length  = Xil_Htons(buffer_length + WARP_IP_UDP_DELIM_LEN);
    wl_header_tx->seq_num = Xil_Htons(wl_header_tx->seq_num);
    wl_header_tx->flags   = Xil_Htons(wl_header_tx->flags);

    // Update the green LEDs for every packet sent
    increment_green_leds_one_hot();

    // Send the Ethernet packet
    status = socket_sendto(socket_index, to, buffers, num_buffers);

    // Restore wl_header_tx
    wl_header_tx->dest_id = Xil_Ntohs(wl_header_tx->dest_id);
    wl_header_tx->src_id  = Xil_Ntohs(wl_header_tx->src_id);
    wl_header_tx->length  = 0;
    wl_header_tx->seq_num = Xil_Ntohs(wl_header_tx->seq_num);
    wl_header_tx->flags   = Xil_Ntohs(wl_header_tx->flags);

    // Check that the packet was sent correctly
    if (status == WARP_IP_UDP_FAILURE) {
        wl_printf(WL_PRINT_WARNING, print_type_transport, "Issue sending packet %d to host.\n", wl_header_tx->seq_num);
    }
}




/*****************************************************************************/
/**
 * Process Transport Commands
 *
 * This function is part of the Ethernet processing system and will process the
 * various transport related commands.
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
int transport_process_cmd(int socket_index, void * from, wl_cmd_resp * command, wl_cmd_resp * response) {

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
    u32                 temp;
    u32                 eth_dev_num;
    u32                 size_index;
    u32                 payload_size;
    u32                 header_size;

    // Set up the response header
    resp_hdr->cmd       = cmd_hdr->cmd;
    resp_hdr->length    = 0;
    resp_hdr->num_args  = 0;

    // Process the command
    switch(cmd_id){

        //---------------------------------------------------------------------
        case CMDID_TRANSPORT_PING:
            //
            // Nothing actually needs to be done when receiving the ping command. The framework is going
            // to respond regardless, which is all the host wants.
            //
        break;

        //---------------------------------------------------------------------
        case CMDID_TRANSPORT_PAYLOAD_SIZE_TEST:
            //
            // Due to packet fragmentation, it is not safe to just return the packet length.  We have seen
            // an issue where the host will send a 1514 byte fragment which results in a payload size of
            // 1472 and causes the transport to not behave correctly.  Therefore, we need to find the
            // last valid command argument and check that against the packet length.
            //
            header_size  = (sizeof(wl_transport_header) + sizeof(wl_cmd_resp_hdr));                            // Transport / Command headers
            size_index   = (((warp_ip_udp_buffer *)(command->buffer))->length - sizeof(wl_cmd_resp_hdr)) / 4;  // Final index into command args (/4 truncates)

            // Check the value in the command args to make sure it matches the size_index
            //   NOTE:  This indexing step works because the payload is filled with incrementing number starting at 1.
            //
            payload_size = (Xil_Htonl(cmd_args_32[size_index - 1]) * 4) + header_size;
            temp         = ((size_index * 4) + header_size);

            if (payload_size != temp) {
                wl_printf(WL_PRINT_WARNING, print_type_transport, "Payload size mismatch.  Value in command args does not match index:  %d != %d\n", payload_size, temp);
            }

            resp_args_32[resp_index++] = Xil_Ntohl(payload_size);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = 1;
        break;

        //---------------------------------------------------------------------
        case CMDID_TRANSPORT_NODE_GROUP_ID_ADD:
            eth_dev_num = socket_get_eth_dev_num(socket_index);

            if (eth_dev_num != WARP_IP_UDP_INVALID_ETH_DEVICE) {
                eth_devices[eth_dev_num].group_id = (eth_devices[eth_dev_num].group_id | Xil_Htonl(cmd_args_32[0]));
            } else {
                wl_printf(WL_PRINT_ERROR, print_type_transport, "Add Group ID - Invalid socket index: %d\n", socket_index);
            }
        break;

        //---------------------------------------------------------------------
        case CMDID_TRANSPORT_NODE_GROUP_ID_CLEAR:
            eth_dev_num = socket_get_eth_dev_num(socket_index);

            if (eth_dev_num != WARP_IP_UDP_INVALID_ETH_DEVICE) {
                eth_devices[eth_dev_num].group_id = (eth_devices[eth_dev_num].group_id & ~Xil_Htonl(cmd_args_32[0]));
            } else {
                wl_printf(WL_PRINT_ERROR, print_type_transport, "Clear Group ID - Invalid socket index: %d\n", socket_index);
            }
        break;

        //---------------------------------------------------------------------
        default:
            wl_printf(WL_PRINT_ERROR, print_type_transport, "Unknown user command ID: %d\n", cmd_id);
        break;
    }

    return resp_sent;
}



/*****************************************************************************/
/**
 * Close the unicast and broadcast sockets associated with a given Ethernet device
 *
 * @param   eth_dev_num      - Ethernet device number
 *
 * @return  None
 *
 *****************************************************************************/
void transport_close(u32 eth_dev_num) {

    if (transport_check_device(eth_dev_num) == XST_SUCCESS) {
        socket_close(eth_devices[eth_dev_num].unicast_socket);
        socket_close(eth_devices[eth_dev_num].broadcast_socket);
    }
}



/*****************************************************************************/
/**
 * Create and bind a socket for the Ethernet device
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   socket_index     - Socket index (return value)
 * @param   udp_port         - UDP port number
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int transport_config_socket(u32 eth_dev_num, int * socket_index, u32 udp_port) {

    int status;
    int tmp_socket  = *socket_index;

    // Release socket if it is already bound
    if (tmp_socket != SOCKET_INVALID_SOCKET) {
        socket_close(tmp_socket);
    }

    // Create a new socket
    tmp_socket = socket_socket(AF_INET, SOCK_DGRAM, 0);

    if (tmp_socket == SOCKET_INVALID_SOCKET) {
        wl_printf(WL_PRINT_ERROR, print_type_transport, "Could not create socket\n");

        * socket_index = SOCKET_INVALID_SOCKET;

        return XST_FAILURE;
    }

    // Bind the socket
    status = socket_bind_eth(tmp_socket, eth_dev_num, udp_port);

    if (status == WARP_IP_UDP_FAILURE) {
        wl_printf(WL_PRINT_ERROR, print_type_transport, "Unable to bind socket on port: %d\n", udp_port);

        socket_close(tmp_socket);

        * socket_index = SOCKET_INVALID_SOCKET;

        return XST_FAILURE;
    }

    * socket_index = tmp_socket;

    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * This function will configure the unicast and broadcast sockets to be used
 * by the transport using the default transport_receive_callback() function.
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   unicast_port     - Unicast port for the node
 * @param   bcast_port       - Broadcast port for the node
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int transport_config_sockets(u32 eth_dev_num, u32 unicast_port, u32 bcast_port) {
    int status = XST_SUCCESS;

    status = transport_config_socket(eth_dev_num, &(eth_devices[eth_dev_num].unicast_socket), unicast_port);
    if (status == XST_FAILURE) { return status; }

    status = transport_config_socket(eth_dev_num, &(eth_devices[eth_dev_num].broadcast_socket), bcast_port);
    if (status == XST_FAILURE) { return status; }

    wl_printf(WL_PRINT_NONE, NULL, "  Listening on UDP ports %d (unicast) and %d (broadcast)\n", unicast_port, bcast_port);

    return status;
}



/*****************************************************************************/
/**
 * This function is the Transport callback that allows WARPLab to process
 * a received Ethernet packet
 *
 * @param   hander           - Pointer to the transport receive callback function
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *
 *****************************************************************************/
int transport_set_process_hton_msg_callback(void(*handler)) {
    process_hton_msg_callback = handler;

    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * This function will check the link status of a Ethernet controller
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   unicast_port     - Unicast port for the node
 * @param   bcast_port       - Broadcast port for the node
 *
 * @return  int              - Status of the command:
 *                                 LINK_READY     - Ethernet controller is ready to be used
 *                                 LINK_NOT_READY - Ethernet controller is not ready to be used
 *
 *****************************************************************************/
int transport_link_status(u32 eth_dev_num) {

    int status  = LINK_READY;
    u16 reg_val = transport_get_ethernet_status(eth_dev_num);

    if(reg_val & ETH_PHY_REG_17_0_LINKUP) {
        status = LINK_READY;
    } else {
        status = LINK_NOT_READY;
    }

    return status;
}



/*****************************************************************************/
/**
 * This function will update the link speed of a Ethernet controller
 *
 * @param   eth_dev_num           - Ethernet device number
 * @param   wait_for_negotiation  - Flag to wait for auto-negotiation of Ethernet link speed
 *
 * @return  speed            - Ethernet link speed that was chosen
 *
 *****************************************************************************/
u32 transport_update_link_speed(u32 eth_dev_num, u32 wait_for_negotiation) {

    volatile u16 reg_val          = 0;
    u32          negotiated       = 1;
    u16          speed            = 0;

    u32          start_time       = get_usec_timestamp();
    u32          end_time         = start_time;

    // Make sure the Ethernet device is initialized
    if (eth_devices[eth_dev_num].initialized == WL_ETH_DEV_INITIALIZED) {

        xil_printf("  ETH %c speed ", warp_conv_eth_dev_num(eth_dev_num));

        reg_val = transport_get_ethernet_status(eth_dev_num);

        if (wait_for_negotiation == ETH_WAIT_FOR_AUTO_NEGOTIATION) {

            while((reg_val & ETH_PHY_REG_17_0_SPEED_RESOLVED) == 0) {
                usleep(1000);
                reg_val = transport_get_ethernet_status(eth_dev_num);
            }

            speed = ETH_PHY_SPEED_TO_MBPS((reg_val & ETH_PHY_REG_17_0_SPEED));
            end_time = get_usec_timestamp();

        } else {
            // Check to see if the Ethernet controller has auto-negotiated a speed
            if (reg_val & ETH_PHY_REG_17_0_SPEED_RESOLVED) {
                speed      = ETH_PHY_SPEED_TO_MBPS((reg_val & ETH_PHY_REG_17_0_SPEED));
            } else {
                speed      = eth_devices[eth_dev_num].default_speed;
                negotiated = 0;
            }
        }

        // Set the operating speed of the Ethernet controller
        eth_set_operating_speed(eth_dev_num, speed);

        // Set the operating speed of the Ethernet PHY
        transport_set_eth_phy_speed(eth_dev_num, speed);

        // Sleep for a short period of time to let everything settle
        usleep(1 * 10000);

    } else {
        wl_printf(WL_PRINT_NONE, NULL, "  ETH %c not initialized.  Link speed not updated.\n", warp_conv_eth_dev_num(eth_dev_num));
    }

    if (negotiated) {
        xil_printf("%d Mbps (auto-negotiated", speed);

        if (start_time != end_time) {
            xil_printf(" in %d usec)\n", (end_time - start_time));
        } else {
            xil_printf(")\n");
        }
    } else {
        xil_printf("%d Mbps (default)\n", speed);
    }

    return speed;
}



/*****************************************************************************/
/**
 * This function set the speed of a Ethernet PHY
 *
 * @param   eth_dev_num           - Ethernet device number
 * @param   speed                 - Speed
 *
 * @return  None
 *
 *****************************************************************************/
void transport_set_eth_phy_speed(u32 eth_dev_num, u32 speed) {

    // See Ethernet PHY specification for documentation on the values used for PHY commands
    u16          phy_ctrl_reg_val;

    // Read the PHY Control register
    eth_read_phy_reg(eth_dev_num, eth_devices[eth_dev_num].phy_addr, ETH_PHY_CONTROL_REG, &phy_ctrl_reg_val);

    // Based on the argument configure the appropriate bits to set the desired speed
    switch (speed) {
        case ETH_PHY_SPEED_1000_MBPS:
            // Set speed to 1000 Mbps (MSB = 1; LSB = 0)
            phy_ctrl_reg_val = (phy_ctrl_reg_val & ~ETH_PHY_REG_0_SPEED_LSB) | ETH_PHY_REG_0_SPEED_MSB;
        break;
        case ETH_PHY_SPEED_100_MBPS:
            // Set speed to 100 Mbps (MSB = 0; LSB = 1)
            phy_ctrl_reg_val = (phy_ctrl_reg_val & ~ETH_PHY_REG_0_SPEED_MSB) | ETH_PHY_REG_0_SPEED_LSB;
        break;
        case ETH_PHY_SPEED_10_MBPS:
            // Set speed to 10 Mbps (MSB = 0; LSB = 0)
            phy_ctrl_reg_val = phy_ctrl_reg_val & ~(ETH_PHY_REG_0_SPEED_MSB | ETH_PHY_REG_0_SPEED_LSB);
        break;
        default:
            wl_printf(WL_PRINT_ERROR, print_type_transport, "Ethernet %c invalid speed: %d.\n", warp_conv_eth_dev_num(eth_dev_num), speed);
        break;
    }

    // Write the value to the PHY and trigger a reset to update PHY internal state
    eth_write_phy_reg(eth_dev_num, eth_devices[eth_dev_num].phy_addr, ETH_PHY_CONTROL_REG, phy_ctrl_reg_val);
    eth_write_phy_reg(eth_dev_num, eth_devices[eth_dev_num].phy_addr, ETH_PHY_CONTROL_REG, (ETH_PHY_REG_0_RESET | phy_ctrl_reg_val));
}



/*****************************************************************************/
/**
 * This function set the auto-negotiation state of a Ethernet controller
 *
 * @param   eth_dev_num           - Ethernet device number
 * @param   enable                - Enable / Disable auto-negotiation
 *
 * @return  None
 *
 *****************************************************************************/
void transport_set_eth_phy_auto_negotiation(u32 eth_dev_num, u32 enable) {

    // See Ethernet PHY specification for documentation on the values used for PHY commands
    u16          phy_ctrl_reg_val;

    // Read the PHY Control register
    eth_read_phy_reg(eth_dev_num, eth_devices[eth_dev_num].phy_addr, ETH_PHY_CONTROL_REG, &phy_ctrl_reg_val);

    // Based on the argument enable or disable auto-negotiation
    if (enable) {
        // Enable auto-negotiation
        phy_ctrl_reg_val = phy_ctrl_reg_val | ETH_PHY_REG_0_AUTO_NEGOTIATION;
    } else {
        // Disable auto-negotiation
        phy_ctrl_reg_val = phy_ctrl_reg_val & ~ETH_PHY_REG_0_AUTO_NEGOTIATION;
    }

    // Write the value to the PHY and trigger a reset to update PHY internal state
    eth_write_phy_reg(eth_dev_num, eth_devices[eth_dev_num].phy_addr, ETH_PHY_CONTROL_REG, phy_ctrl_reg_val);
    eth_write_phy_reg(eth_dev_num, eth_devices[eth_dev_num].phy_addr, ETH_PHY_CONTROL_REG, (ETH_PHY_REG_0_RESET | phy_ctrl_reg_val));
}



/*****************************************************************************/
/**
 * Check the Ethernet device of the transport
 *
 * @param   eth_dev_num      - Ethernet device number
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/

int transport_check_device(u32 eth_dev_num) {

    // Check that we have a valid Ethernet device for the transport
    if (eth_dev_num >= WL_NUM_ETH_DEVICES) {
        wl_printf(WL_PRINT_ERROR, print_type_transport, "Ethernet %c is not available on WARP HW.\n", warp_conv_eth_dev_num(eth_dev_num));
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}



/**********************************************************************************************************************/
/**
 * @brief WARP v3 Specific Functions
 *
 **********************************************************************************************************************/


/***************************** Include Files *********************************/

#include <w3_iic_eeprom.h>
#include <w3_userio.h>


/*************************** Constant Definitions ****************************/

/*********************** Global Variable Definitions *************************/

/*************************** Variable Definitions ****************************/

/*************************** Functions Prototypes ****************************/

int transport_read_ip_addr(u32 eth_dev_num, u8 * ip_addr);


/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * Read the IP / MAC address from the node for the Ethernet device
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   hw_addr          - Pointer to an u8 array to return the MAC address
 * @param   ip_addr          - Pointer to an u8 array to return the IP address
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/
int transport_get_hw_info(u32 eth_dev_num, u8 * hw_addr, u8 * ip_addr) {

    int status = XST_SUCCESS;

    // Read the MAC address from the node
    w3_eeprom_readEthAddr(EEPROM_BASEADDR, eth_dev_num, hw_addr);

    wl_printf(WL_PRINT_NONE, NULL, "  ETH %c MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
            warp_conv_eth_dev_num(eth_dev_num), hw_addr[0], hw_addr[1], hw_addr[2], hw_addr[3], hw_addr[4], hw_addr[5]);

    // Read the IP address from the node
    transport_read_ip_addr(eth_dev_num, ip_addr);

    wl_printf(WL_PRINT_NONE, NULL, "  ETH %c IP  Address: %d.%d.%d.%d\n",
            warp_conv_eth_dev_num(eth_dev_num), ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);

    return status;
}



/*****************************************************************************/
/**
 * Read the IP address from the node
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   ip_addr          - Pointer to an u8 array to return the IP address
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/
int transport_read_ip_addr(u32 eth_dev_num, u8 * ip_addr) {

    u32 tmp_ip_address       = 0;

    switch (eth_dev_num) {
        case WL_ETH_A:
            tmp_ip_address = WL_ETH_A_IP_ADDR_BASE;
        break;
        case WL_ETH_B:
            tmp_ip_address = WL_ETH_B_IP_ADDR_BASE;
        break;
    }

    if (tmp_ip_address == 0) {
    	return XST_FAILURE;
    }

    // Populate the output array
    ip_addr[0] = (tmp_ip_address >> 24) & 0xFF;
    ip_addr[1] = (tmp_ip_address >> 16) & 0xFF;
    ip_addr[2] = (tmp_ip_address >>  8) & 0xFF;
    ip_addr[3] = (node + 1);                         // IP ADDR = x.y.z.( node + 1 )

    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * This function will read the status of a Ethernet controller
 *
 * @param   eth_dev_num      - Ethernet device number
 *
 * @return  u16              - Bits are defined for the WARP Ethernet chip.  See wl_transport.h
 *                             for defines for relevant fields
 *
 *****************************************************************************/
u16 transport_get_ethernet_status(u32 eth_dev_num) {

    u16 reg_val = 0;

    // Check that we are initializing a valid Ethernet device for the transport
    if (transport_check_device(eth_dev_num) != XST_SUCCESS) {
        return LINK_NOT_READY;
    }

    if (eth_devices[eth_dev_num].initialized == WL_ETH_DEV_INITIALIZED) {

        // Check if the Ethernet PHY reports a valid link
        eth_read_phy_reg(eth_dev_num, eth_devices[eth_dev_num].phy_addr, ETH_PHY_STATUS_REG, &reg_val);
    }

    return reg_val;
}



/*****************************************************************************/
/**
 * Initialize the information about the Ethernet device
 *
 * @param   eth_dev_num      - Ethernet device number
 *
 * @return  None
 *
 ******************************************************************************/
void transport_wl_eth_dev_info_init(u32 eth_dev_num) {

    // Initialize Ethernet device
    switch (eth_dev_num) {
        case WL_ETH_A:
            eth_devices[eth_dev_num].default_speed         = WL_ETH_A_DEFAULT_SPEED;
            eth_devices[eth_dev_num].phy_addr              = WL_ETH_A_MDIO_PHYADDR;
        break;

        case WL_ETH_B:
            eth_devices[eth_dev_num].default_speed         = WL_ETH_B_DEFAULT_SPEED;
            eth_devices[eth_dev_num].phy_addr              = WL_ETH_B_MDIO_PHYADDR;
        break;

        default:
            xil_printf("  **** ERROR:  Ethernet device %d not configured in hardware.", (eth_dev_num + 1));
        break;
    }

    // Common initialization
    eth_devices[eth_dev_num].type                  = WL_IP_UDP_TRANSPORT;
    eth_devices[eth_dev_num].hw_addr[0]            = 0;
    eth_devices[eth_dev_num].hw_addr[1]            = 0;
    eth_devices[eth_dev_num].ip_addr               = 0;
    eth_devices[eth_dev_num].unicast_socket        = SOCKET_INVALID_SOCKET;
    eth_devices[eth_dev_num].broadcast_socket      = SOCKET_INVALID_SOCKET;
    eth_devices[eth_dev_num].group_id              = 0;
    eth_devices[eth_dev_num].initialized           = WL_ETH_DEV_INITIALIZED;
}



/*****************************************************************************/
/**
 * @brief Transport subsystem initialization
 *
 * Initializes the transport subsystem
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   init_driver      - Initialize the WARP IP/UDP driver (should only be done once)
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 ******************************************************************************/
int transport_init(u32 eth_dev_num, u8 init_driver) {

    int       status = XST_SUCCESS;

    u8        node_ip_addr[IP_ADDR_LEN];
    u8        node_hw_addr[ETH_MAC_ADDR_LEN];

    // Print initialization message
    wl_printf(WL_PRINT_NONE, NULL, "Configuring transport ...\n");

    // Initialize the User callback for processing a packet
    process_hton_msg_callback = wl_null_callback;

    // Check that we are initializing a valid Ethernet device for the transport
    if (transport_check_device(eth_dev_num) != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Initialize the WARP UDP transport global variables
    if (init_driver) {
        warp_ip_udp_init();
    }

    // Get device specific MAC / IP address from the EEPROM
    status = transport_get_hw_info(eth_dev_num, (u8 *)&node_hw_addr, (u8 *)&node_ip_addr);

    if (status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_transport, "Error retrieving node specific HW info from EEPROM:  %d \n", status);
    }

    // Initialize the Ethernet device (use verbose mode)
    status = eth_init(eth_dev_num, node_hw_addr, node_ip_addr, 0x1);

    if (status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_transport, "Ethernet %c initialization error\n", warp_conv_eth_dev_num(eth_dev_num));
    }

    // Initialize the wl_eth_devices structure
    transport_wl_eth_dev_info_init(eth_dev_num);

    // Set the Ethernet link speed
    if (WL_NEGOTIATE_ETH_LINK_SPEED) {
        // Enable auto-negotiation in the Ethernet PHY
        transport_set_eth_phy_auto_negotiation(eth_dev_num, WL_ENABLE);

        // Update the link speed
        transport_update_link_speed(eth_dev_num, ETH_WAIT_FOR_AUTO_NEGOTIATION);

    } else {
        // Disable auto-negotiation in the Ethernet PHY
        transport_set_eth_phy_auto_negotiation(eth_dev_num, WL_DISABLE);

        // Update the link speed
        transport_update_link_speed(eth_dev_num, ETH_DO_NOT_WAIT_FOR_AUTO_NEGOTIATION);
    }

    // Start Ethernet device
    status = eth_start_device(eth_dev_num);

    if (status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_transport, "Cannot start Ethernet %c\n", warp_conv_eth_dev_num(eth_dev_num));
    }

    // Configure the Sockets for each Ethernet Interface
    status = transport_config_sockets(eth_dev_num, NODE_UDP_UNICAST_PORT_BASE, NODE_UDP_MCAST_BASE);

    if (status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_transport, "Cannot configure sockets for Ethernet %c\n", warp_conv_eth_dev_num(eth_dev_num));
    }

    return status;
}

