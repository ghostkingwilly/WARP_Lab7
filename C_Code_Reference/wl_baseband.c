/** @file wl_baseband.c
 *  @brief WARPLab Framework (Baseband)
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

/******************************* IMPORTANT ***********************************/
//
// We are going to assume that all TX, RX and RSSI buffers are the same size.
// In the future, this might not be true and the code will need to be modified
// to accommodate this.
//
/******************************* IMPORTANT ***********************************/



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

#include <xio.h>
#include <xparameters.h>
#include <xil_io.h>
#include <xil_types.h>
#include <xstatus.h>

// WARPLab includes
#include "wl_common.h"
#include "wl_baseband.h"
#include "wl_interface.h"
#include "wl_transport.h"
#include "wl_node.h"
#include "wl_trigger_manager.h"

// WARP IP/UPD Library includes
#include <WARP_ip_udp.h>

/*************************** Constant Definitions ****************************/

/*********************** Global Variable Definitions *************************/

/*************************** Variable Definitions ****************************/

// Fletcher-32 Checksum variables
static u32         write_iq_checksum_lsb = 0;
static u32         write_iq_checksum_msb = 0;

// Buffer variables
static u32         rx_buffer_size;
static u32         use_dram_for_buffers  = 0;

static void      * wl_iq_rx_buff_a;
static void      * wl_iq_tx_buff_a;
static void      * wl_rssi_buff_a;

static u32         wl_iq_rx_buff_a_size;
static u32         wl_iq_tx_buff_a_size;
static u32         wl_rssi_buff_a_size;

static void      * wl_iq_rx_buff_b;
static void      * wl_iq_tx_buff_b;
static void      * wl_rssi_buff_b;

static u32         wl_iq_rx_buff_b_size;
static u32         wl_iq_tx_buff_b_size;
static u32         wl_rssi_buff_b_size;

static void      * wl_iq_rx_buff_c;
static void      * wl_iq_tx_buff_c;
static void      * wl_rssi_buff_c;

static u32         wl_iq_rx_buff_c_size;
static u32         wl_iq_tx_buff_c_size;
static u32         wl_rssi_buff_c_size;

static void      * wl_iq_rx_buff_d;
static void      * wl_iq_tx_buff_d;
static void      * wl_rssi_buff_d;

static u32         wl_iq_rx_buff_d_size;
static u32         wl_iq_tx_buff_d_size;
static u32         wl_rssi_buff_d_size;

static u32         supported_tx_length = 0xFFFFFFFF;
static u32         supported_rx_length = 0xFFFFFFFF;

// Bit counting vector
const u8           one_bits[] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};


// Read IQ Packet Buffering:
//
// In WARPLab 7.5.x, the baseband explicitly did not allow Read IQ transfers while the node was transmitting
// or receiving.  This restriction was imposed because the new larger WARPLab buffers, see
// http://warpproject.org/trac/wiki/WARPLab/BufferSizes, required CPU processing to perform intermediate
// transfers of IQ data between BRAM and DDR.  This restriction also allowed certain design decisions to be
// kept from previous versions of WARPLab for both the transport and the baseband.  First, the WARPxilnet
// transport would only process one Ethernet transaction at a time, ie it was not possible to queue up multiple
// transactions within the transport (NOTE:  While the transport would only process one transaction at a time,
// there was some internal buffering within the Ethernet sub-system that allowed the node to achieve almost a
// continuous 1 Gbps, see http://warpproject.org/trac/wiki/WARPLab/Benchmarks).  Second, given the behavior of
// the transport, the Read IQ command would perform an in place modification of the Ethernet packet buffer
// provided by the transport, which meant that no additional memory was needed for the Read IQ command.
//
// However, with WARPLab 7.6.0, this restriction on Read IQ transfers was removed.  This was done to 1) increase
// overall performance of the node by allowing Read IQ transfers to be pipelined with the node transmitting or
// receiving; and 2) make the new version of WARPLab compatible with older versions of WARPLab (ie WARPLab 7.4.0
// and older) which did not have this restriction due to the smaller buffer sizes not requiring CPU processing.
// Removing this restriction, required moving to the WARP IP/UDP transport which was able to process multiple
// Ethernet transactions at a time, ie it was now possible to queue up multiple transactions within the
// transport.  This also required updating the Read IQ command to use additional buffering for Ethernet packets
// and not just perform in place modification of the Ethernet packet buffer provided by the transport.
//
// In order to maintain the same performance of Read IQ commands while the node is transmitting or receiving
// (i.e. to not have Read IQ transfers paused by the transfer of IQ data between BRAM and DDR as part of the
// transmit and receive processes), the Read IQ command has to issue as multiple transfers back to back so that
// it can have a buffer for when the CPU is busy transferring IQ data.  Since it is costly to continually query
// the state of the transport to understand when to issue the next transaction, the easiest course of action is
// to issue as many transactions as the transport supports and allow the transport to naturally provide the back
// pressure that will limit the issuing of new transactions.  For the WARP IP/UDP transport, the number of
// transfers is limited by the number of Transmit Buffer Descriptors (TX BDs).  Based on some high level
// measurements, when using jumbo frames the transport provides approximately ~460 us of buffer in steady state
// when using the default configuration of 10 TX BDs (NOTE:  TX BDs are a BSP configuration setting.  Also, each
// Ethernet packet generally requires 2 or 3 buffer descriptors).  This buffer time will increase or decrease
// if a user increases or decreases the number of TX BDs, respectively.
//
// The issuing of multiple transactions to the transport for the Read IQ command requires the baseband to buffer
// Ethernet headers (ie the non-IQ data that changes between packets) before they are processed by the transport.
// Based on experiments, the number of Ethernet header buffers should be more than half the number of TX BDs.
// For example, in a node with 10 TX BDs, the minimum number of Ethernet header buffers required is 5.  If there
// are fewer buffers than that, for example 4 Ethernet header buffers in a node with 10 TX BDs, then there will
// be errors in the Read IQ command as the command overwrites Ethernet header information before it is able to be
// processed by the transport.  Therefore, we need to define and allocate buffers for Ethernet headers in the
// baseband:
//
// Define Read IQ Ethernet Header Buffer Constants
//   NOTE:  Given we are not extremely memory constrained in WARPLab 7.6.0, the constants will be defined such
//          that the buffers are easy to understand and debug:
//              1)  Each buffer has 128 bytes which is more than needed for an Ethernet header for standard node
//                  to host communication in WARP reference designs (max 76 bytes as of WARPLab 7.6.0).
//              2)  5 buffers are allocated which is the minimum number of buffers needed for the default transport
//                  setting of 10 TX BDs.
//              3)  Use 64 byte alignment for the buffers which is the same as the WARP IP/UDP transport
//                  (ie it is the same as WARP_IP_UDP_BUFFER_ALIGNMENT in WARP_ip_udp_config.h)
//
#define WL_BASEBAND_ETH_BUFFER_SIZE                        0x80                // Number of bytes per buffer
#define WL_BASEBAND_ETH_NUM_BUFFER                         0x05                // Number of buffers allocated
#define WL_BASEBAND_ETH_BUFFER_ALIGNMENT                   0x40                // Buffer alignment (64 byte boundary)


// Allocate Read IQ Ethernet Header buffer
//     NOTE:  The buffer memory must be placed in DMA accessible BRAM such that it can be fetched by the AXI DMA
//            attached to the Ethernet module.  Therefore, we will use the same section as other buffers for
//            Ethernet data, ie section ".eth_data".
//
u8                 ETH_IQ_buffer[WL_BASEBAND_ETH_NUM_BUFFER * WL_BASEBAND_ETH_BUFFER_SIZE] __attribute__ ((aligned(WL_BASEBAND_ETH_BUFFER_ALIGNMENT))) __attribute__ ((section (".eth_data")));


/*************************** Functions Prototypes ****************************/

void read_rx_buffers(u32 cmd_id, u32 buffer_sel, u32 offset, u32 length, u32 dest_addr, warp_ip_udp_buffer * buffer);
void write_tx_buffers(u32 buffer_sel, u32 src_addr, u32 offset, u32 length);

// Functions implemented in HW specific sections of the file
void baseband_hw_specific_reset();
void baseband_transfer_data(u32 src_addr, u32 dest_addr, u32 length);
void populate_tmp_tx_buffers(u32 buffer_sel, u32 offset, u32 length);
void baseband_buffers_config(u8 dram_present);
int  baseband_check_parameters();

// Misc functions
u32  get_buffer_counter(u32 txrx_sel, u32 buffer_sel);

// Read IQ transport functions
void send_read_iq_packet(int socket_index, void * to, wl_cmd_resp_hdr * resp_hdr, void ** buffers, u32 num_buffers);

// Debug functions
#ifdef _DEBUG_
void print_buffer_info();
void print_buffer_core_registers();
void print_agc_registers();
#endif


/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * Process Baseband Commands
 *
 * This function is part of the Ethernet processing system and will process the
 * various baseband related commands.
 *
 * @param   socket_index     - Index of the socket on which to send message
 * @param   from             - Pointer to socket address structure (struct sockaddr *) where command is from
 * @param   command          - Pointer to WARPLab Command
 * @param   response         - Pointer to WARPLab Response
 *
 * @return  int              - Status of the command:
 *                                 NO_RESP_SENT       - No response has been sent
 *                                 RESP_SENT          - A response has been sent
 *                                 NODE_NOT_READY     - Node is not ready to respond to the command
 *
 * @note    See on-line documentation for more information about the Ethernet
 *          packet structure for WARPLab:  www.warpproject.org
 *
 ******************************************************************************/
int baseband_process_cmd(int socket_index, void * from, wl_cmd_resp * command, wl_cmd_resp * response) {

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
    u32                 status;
    u32                 msg_cmd;

    u32                 i;
    u32                 tx_delay;
    u32                 sample_length;
    u32                 byte_length;
    u32                 mode;
    u32                 rf_sel, buff_sel, txrx_sel;
    u32                 buff_enable;
    u32                 buff_counter;
    u32                 start_samp, curr_samp, start_byte, num_samp, samp_len, num_pkts, offset;
    u32                 total_samp, max_samp_len_per_pkt, max_samp_per_pkt, next_start_samp;
    u8                  sample_iq_id;

    warp_ip_udp_buffer  header_buffer;
    warp_ip_udp_buffer  sample_buffer;
    void              * read_iq_resp[2];
    u32                 header_length;
    u16                 ip_length;
    u16                 udp_length;
    u16                 data_length;
    u32                 total_hdr_length;
    u8                  dest_hw_addr[ETH_MAC_ADDR_LEN];
    u32                 dest_ip_addr;
    u16                 dest_port;
    u32                 eth_dev_num;
    u8                * header_base_addr;
    u32                 header_offset;
    u8                * header_addr;
    u32                 header_buffer_size;
    u8                  tmp_header[80];                         // Temporary header (80 bytes)

    u32                 temp;
    u32                 temp_offset;
    u32                 temp_status;
    u32                 temp_threshold;
    u32                 threshold;
    u32                 check_status;
    u32                 raw_status;

    u32                 curr_checksum;
    u32                 checksum_input_32;
    u16                 checksum_input_16;

    u8                  flags;
    void              * samp_addr;
    u32               * samp_addr_32;
    wl_bb_samp_hdr    * samp_hdr;

    u16                 agc_target;
    u32                 agc_dco_enable;

    u32                 rssi_avg_length;
    u32                 v_db_adjust;
    u32                 init_bb_gain;

    u32                 a1_coeff;
    u32                 b0_coeff;

    u32                 thresh_3_2;
    u32                 thresh_2_1;

    u32                 capture_rssi_1;
    u32                 capture_rssi_2;
    u32                 capture_v_db;
    u32                 agc_done;

    u32                 start_dco;
    u32                 start_iir_filter;

    u32                 dest_addr;

    warp_ip_udp_header     * eth_ip_udp_header;
    wl_transport_header    * wl_header_tx;


    // Set up the response header
    resp_hdr->cmd       = cmd_hdr->cmd;
    resp_hdr->length    = 0;
    resp_hdr->num_args  = 0;

    // Process the command
    switch(cmd_id){

        //---------------------------------------------------------------------
        case CMDID_BASEBAND_TX_DELAY:
            // Get / Set the TX Delay
            //
            // Message format:
            //     cmd_args_32[0]      Command:
            //                             - Write       (NODE_WRITE_VAL)
            //                             - Read        (NODE_READ_VAL)
            //
            // Response format:
            //     resp_args_32[0]     Status
            //     resp_args_32[1]     Current TX Delay
            //
            status  = CMD_PARAM_SUCCESS;
            msg_cmd = Xil_Ntohl(cmd_args_32[0]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    tx_delay = Xil_Ntohl(cmd_args_32[1]);
                    wl_bb_set_tx_delay(tx_delay);
                break;

                case CMD_PARAM_READ_VAL:
                break;

                default:
                    wl_printf(WL_PRINT_ERROR, print_type_baseband, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(wl_bb_get_tx_delay());

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_TX_LENGTH:
            // Get / Set the TX Length
            //
            // Message format:
            //     cmd_args_32[0]      Command:
            //                             - Write       (NODE_WRITE_VAL)
            //                             - Read        (NODE_READ_VAL)
            //
            // Response format:
            //     resp_args_32[0]     Status
            //     resp_args_32[1]     Current TX Length
            //
            status  = CMD_PARAM_SUCCESS;
            msg_cmd = Xil_Ntohl(cmd_args_32[0]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    sample_length = Xil_Ntohl(cmd_args_32[1]);

                    // Check that the length is supported
                    if (sample_length != 0) {
                        sample_length -= 1;                // Adjust sample length for buffers core

                        if (sample_length > supported_tx_length) {
                            wl_printf(WL_PRINT_WARNING, print_type_baseband,
                                      "Tx length greater than max supported length.  Setting to %d\n", supported_tx_length);

                            sample_length = supported_tx_length;
                        }
                    }

                    wl_bb_set_tx_length(sample_length);
                break;

                case CMD_PARAM_READ_VAL:
                break;

                default:
                    wl_printf(WL_PRINT_ERROR, print_type_baseband, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(wl_bb_get_tx_length() + 1);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_RX_LENGTH:
            // Get / Set the RX Length
            //
            // Message format:
            //     cmd_args_32[0]      Command:
            //                             - Write       (NODE_WRITE_VAL)
            //                             - Read        (NODE_READ_VAL)
            //
            // Response format:
            //     resp_args_32[0]     Status
            //     resp_args_32[1]     Current RX Length
            //
            status  = CMD_PARAM_SUCCESS;
            msg_cmd = Xil_Ntohl(cmd_args_32[0]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    sample_length = Xil_Ntohl(cmd_args_32[1]);
                    byte_length   = (sample_length << 2);

                    if (sample_length != 0) {
                        // Check that the length is less than the supported maximum number of samples
                        if ((sample_length - 1) > supported_rx_length) {
                            wl_printf(WL_PRINT_WARNING, print_type_baseband,
                                      "Rx length greater than max supported length.  Setting to %d\n", supported_rx_length);

                            sample_length = supported_rx_length;
                        }

                        // Set the global variable of the RX buffer size (in bytes) aligned to the RX transfer boundary
                        rx_buffer_size = byte_length & WL_BUF_RX_TRANSFER_BYTE_ALIGNMENT_MASK;

                        // Adjust the rx_buffer_size so that it is greater than the requested RX length
                        //     NOTE:  This is necessary so that we run through the ISR the correct number of times
                        //
                        if (byte_length > rx_buffer_size) {
                            rx_buffer_size += WL_BUF_RX_TRANSFER_THRESHOLD_BYTES;
                        }

                        //
                        //
                        // NOTE:  Due to the buffering scheme, if the length is greater than the RX IQ threshold,
                        //     then we need to make sure the RX length is aligned to the Rx transfer boundary so that
                        //     we do not run into any interrupt timing issues.  This means that for length values that
                        //     are not already aligned, we will capture some extra samples.  This will cause those
                        //     samples to be overwritten in the DDR buffer but that should be fine.
                        //
                        //

                        // Get the current RX IQ threshold value
                        threshold = wl_bb_get_rf_rx_iq_threshold();

                        if (sample_length > threshold) {
                            // Align the length the transfer threshold
                            temp = sample_length & WL_BUF_RX_SAMPLE_ALIGNMENT_MASK;

                            // Check to see if the length was actually longer than the aligned value
                            if (sample_length > temp) {
                                temp = temp + WL_BUF_RX_TRANSFER_THRESHOLD_SAMPLES;
                            }
                        } else {
                            temp = sample_length;
                        }

                        // Set the Rx length in the buffers core to new value
                        wl_bb_set_rx_length(temp - 1);

                        // Update the AGC rx length
                        warplab_set_agc_rx_length(temp + 100);

                    } else {
                        // Set the Rx length in the buffers core to new value
                        wl_bb_set_rx_length(sample_length);
                    }
                break;

                case CMD_PARAM_READ_VAL:
                break;

                default:
                    wl_printf(WL_PRINT_ERROR, print_type_baseband, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(wl_bb_get_rx_length() + 1);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_MAX_NUM_SAMPLES:
            // Get the maximum number of samples for a given RF interface
            //
            // Message format:
            //     cmd_args_32[0]      Command:
            //                             - Write       (NODE_WRITE_VAL)
            //                             - Read        (NODE_READ_VAL)
            //     cmd_args_32[1]      RF selection
            //
            // Response format:
            //     resp_args_32[0]     Status
            //     resp_args_32[1]     Max TX samples
            //     resp_args_32[2]     Max RX samples
            //
            // NOTE:  Since we have assumed that all buffers have the same capabilities, we ignore the
            //     RF selection parameter since that does not affect the return value.  In the future,
            //     if we violate this assumption, this function will need to be updated.
            //
            status  = CMD_PARAM_SUCCESS;
            msg_cmd = Xil_Ntohl(cmd_args_32[0]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    status = CMD_PARAM_ERROR;
                    wl_printf(WL_PRINT_ERROR, print_type_baseband, "Write for max num samples not supported\n");
                break;

                case CMD_PARAM_READ_VAL:
                break;

                default:
                    wl_printf(WL_PRINT_ERROR, print_type_baseband, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(wl_bb_get_supported_tx_length() + 1);
            resp_args_32[resp_index++] = Xil_Htonl(wl_bb_get_supported_rx_length() + 1);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_TX_MODE:
            // Set TX mode to "continuous tx" or "normal"
            //
            // Message format:
            //     cmd_args_32[0]      Mode:
            //                             - 1        - Set Continuous TX mode
            //                             - 0        - Clear Continuous TX mode (normal TX mode)
            //
            mode = Xil_Ntohl(cmd_args_32[0]);

            if(mode) {
                sample_length = wl_bb_get_tx_length() + 1;

                if ((sample_length > WL_BUF_DEFAULT_TX_NUM_SAMPLES) && ((sample_length % WL_BUF_TX_TRANSFER_THRESHOLD_SAMPLES) != 0) ) {
                    wl_printf(WL_PRINT_WARNING, print_type_baseband,
                              "Tx length not a multiple of %d.\n    Tx waveform not fully defined.\n", WL_BUF_TX_TRANSFER_THRESHOLD_SAMPLES);
                }

                wl_bb_set_config(WL_BUF_REG_CONFIG_CONT_TX);
            } else {
                wl_bb_clear_config(WL_BUF_REG_CONFIG_CONT_TX);
            }
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_TX_BUFF_EN:
            // Enable TX buffers
            //
            // Message format:
            //     cmd_args_32[0]      Buffer Select (bit selects):
            //                             - [0]      - Select RFA buffer
            //                             - [1]      - Select RFB buffer
            //                             - [2]      - Select RFC buffer
            //                             - [3]      - Select RFD buffer
            //
            // NOTE:  While it is technically feasible to enable the RFC and RFD buffers of the
            //     WARPLab Buffers core in a 2RF design, that ability is restricted in the default
            //     reference design.  First, in a 2RF design, the default DDR configuration allocates
            //     all DDR memory to RFA and RFB, so there is no DDR available for RFC and RFD. Second,
            //     the BRAM blocks that the core uses for "temporary local storage" are not present
            //     for RFC and RFD in the 2RF design.  This makes the design easier to build within
            //     XPS but means that there is no ability to actually transmit or receive data on
            //     RFC or RFD.
            //
            // NOTE:  This will enable the buffers set in the buff_sel input argument.  However, it
            //     will not affect the state of the other buffers.  To disable a buffer, you must use
            //     the BB_TXRX_BUFF_DIS command.
            //
            if (WARPLAB_CONFIG_4RF) {
                buff_sel = Xil_Ntohl(cmd_args_32[0]) & 0x0000000F;
            } else {
                buff_sel = Xil_Ntohl(cmd_args_32[0]) & 0x00000003;
            }

            // Enable the selected buffers in the WARPLab Buffers core
            wl_bb_set_tx_buffer_en(buff_sel);

            // Since the node cannot transmit and receive on the same interface, we explicitly disable
            // the RX buffers for the enabled TX buffers.
            //
            // NOTE:  This might not be true in future revisions of WARPLab.
            //
            buff_enable = wl_bb_get_tx_buffer_en();

            wl_bb_clear_rx_buffer_en(buff_enable);

            // Pre-load data into the enabled buffers so they are ready to go.
            populate_tmp_tx_buffers(buff_sel, 0x0, WARPLAB_IQ_TX_BUF_SIZE);
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_RX_BUFF_EN:
            // Enable RX buffers
            //
            // Message format:
            //     cmd_args_32[0]      Buffer Select (bit selects):
            //                             - [0]      - Select RFA buffer
            //                             - [1]      - Select RFB buffer
            //                             - [2]      - Select RFC buffer
            //                             - [3]      - Select RFD buffer
            //
            // NOTE:  While it is technically feasible to enable the RFC and RFD buffers of the
            //     WARPLab Buffers core in a 2RF design, that ability is restricted in the default
            //     reference design.  First, in a 2RF design, the default DDR configuration allocates
            //     all DDR memory to RFA and RFB, so there is no DDR available for RFC and RFD. Second,
            //     the BRAM blocks that the core uses for "temporary local storage" are not present
            //     for RFC and RFD in the 2RF design.  This makes the design easier to build within
            //     XPS but means that there is no ability to actually transmit or receive data on
            //     RFC or RFD.
            //
            // NOTE:  This will enable the buffers set in the buff_sel input argument.  However, it
            //     will not affect the state of the other buffers.  To disable a buffer, you must use
            //     the BB_TXRX_BUFF_DIS command.
            //
            if (WARPLAB_CONFIG_4RF) {
                buff_sel = Xil_Ntohl(cmd_args_32[0]) & 0x0000000F;
            } else {
                buff_sel = Xil_Ntohl(cmd_args_32[0]) & 0x00000003;
            }

            // Enable the selected buffers in the WARPLab Buffers core
            wl_bb_set_rx_buffer_en(buff_sel);

            // Since the node cannot transmit and receive on the same interface, we explicitly disable
            // the RX buffers for the enabled TX buffers.
            //
            // NOTE:  This might not be true in future revisions of WARPLab.
            //
            buff_enable = wl_bb_get_rx_buffer_en();

            wl_bb_clear_tx_buffer_en(buff_enable);
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_TXRX_BUFF_DIS:
            // Disable TX and RX buffers
            //
            // Message format:
            //     cmd_args_32[0]      Buffer Select (bit selects):
            //                             - [0]      - Select RFA buffer
            //                             - [1]      - Select RFB buffer
            //                             - [2]      - Select RFC buffer
            //                             - [3]      - Select RFD buffer
            //
            // NOTE:  While it is technically feasible to enable the RFC and RFD buffers of the
            //     WARPLab Buffers core in a 2RF design, that ability is restricted in the default
            //     reference design.  First, in a 2RF design, the default DDR configuration allocates
            //     all DDR memory to RFA and RFB, so there is no DDR available for RFC and RFD. Second,
            //     the BRAM blocks that the core uses for "temporary local storage" are not present
            //     for RFC and RFD in the 2RF design.  This makes the design easier to build within
            //     XPS but means that there is no ability to actually transmit or receive data on
            //     RFC or RFD.
            //
            if (WARPLAB_CONFIG_4RF) {
                buff_sel = Xil_Ntohl(cmd_args_32[0]) & 0x0000000F;
            } else {
                buff_sel = Xil_Ntohl(cmd_args_32[0]) & 0x00000003;
            }

            wl_bb_clear_tx_buffer_en(buff_sel);
            wl_bb_clear_rx_buffer_en(buff_sel);

            // Return all disabled buffers to their default state
            populate_tmp_tx_buffers(buff_sel, 0x0, WARPLAB_IQ_TX_BUF_SIZE);
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_TXRX_BUFF_STATE:
            // Return the state of the TX and RX buffers
            //
            // Message format:
            //     cmd_args_32[0]      Buffer Select (bit selects):
            //                             - [0]      - Select RFA buffer
            //                             - [1]      - Select RFB buffer
            //                             - [2]      - Select RFC buffer
            //                             - [3]      - Select RFD buffer
            //
            // Response format:
            //     resp_args_32[0]     RFA state (optional)
            //     resp_args_32[1]     RFB state (optional)
            //     resp_args_32[2]     RFC state (optional)
            //     resp_args_32[3]     RFD state (optional)
            //
            // NOTE:  The return value of the RF state will only get added to the return
            //     arguments if the given RF interface is selected.
            //
            buff_sel    = Xil_Ntohl(cmd_args_32[0]);

            if (buff_sel & RF_SEL_A) {
                buff_enable = BUF_STATE_STANDBY;

                if (wl_bb_get_rx_buffer_en() & RF_SEL_A) { buff_enable = BUF_STATE_RX; }
                if (wl_bb_get_tx_buffer_en() & RF_SEL_A) { buff_enable = BUF_STATE_TX; }

                resp_args_32[resp_index++] = Xil_Htonl(buff_enable);
            }

            if (buff_sel & RF_SEL_B) {
                buff_enable = BUF_STATE_STANDBY;

                if (wl_bb_get_rx_buffer_en() & RF_SEL_B) { buff_enable = BUF_STATE_RX; }
                if (wl_bb_get_tx_buffer_en() & RF_SEL_B) { buff_enable = BUF_STATE_TX; }

                resp_args_32[resp_index++] = Xil_Htonl(buff_enable);
            }

            if (buff_sel & RF_SEL_C) {
                buff_enable = BUF_STATE_STANDBY;

                if (wl_bb_get_rx_buffer_en() & RF_SEL_C) { buff_enable = BUF_STATE_RX; }
                if (wl_bb_get_tx_buffer_en() & RF_SEL_C) { buff_enable = BUF_STATE_TX; }

                resp_args_32[resp_index++] = Xil_Htonl(buff_enable);
            }

            if (buff_sel & RF_SEL_D) {
                buff_enable = BUF_STATE_STANDBY;

                if (wl_bb_get_rx_buffer_en() & RF_SEL_D) { buff_enable = BUF_STATE_RX; }
                if (wl_bb_get_tx_buffer_en() & RF_SEL_D) { buff_enable = BUF_STATE_TX; }

                resp_args_32[resp_index++] = Xil_Htonl(buff_enable);
            }

            // Send response
            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_WRITE_IQ:
            // BB_WRITE_IQ Packet Format:
            //
            //   - cmd_args            - Samples:  wl_bb_samp_hdr followed by the
            //                           defined number of samples.
            //
            //   - resp_args_32[0]     - Status
            //                           - CMD_PARAM_SUCCESS
            //                           - SAMPLE_HDR_FLAG_IQ_NOT_READY
            //                           - SAMPLE_HDR_FLAG_IQ_ERROR
            //   - resp_args_32[1]     - IQ ID
            //   - resp_args_32[2]     - Current checksum            (ignore if Status != CMD_PARAM_SUCCESS)
            //   - resp_args_32[3]     - Tx status                   (ignore if Status != SAMPLE_HDR_FLAG_IQ_NOT_READY)
            //   - resp_args_32[4]     - Current Tx read pointer     (ignore if Status != SAMPLE_HDR_FLAG_IQ_NOT_READY)
            //   - resp_args_32[5]     - Tx length                   (ignore if Status != SAMPLE_HDR_FLAG_IQ_NOT_READY)
            //   - resp_args_32[6]     - Rx status                   (ignore if Status != SAMPLE_HDR_FLAG_IQ_NOT_READY)
            //   - resp_args_32[7]     - Current Rx write pointer    (ignore if Status != SAMPLE_HDR_FLAG_IQ_NOT_READY)
            //   - resp_args_32[8]     - Rx length                   (ignore if Status != SAMPLE_HDR_FLAG_IQ_NOT_READY)
            //
            //   NOTE:  When SAMPLE_HDR_FLAG_IQ_NOT_READY is returned, then the host
            //       can compute the wait time by the following calculation:
            //           max(((tx_status) ? (tx_length - tx_read_pointer) : 0), ((rx_status) ? (rx_length - rx_write_pointer) : 0));
            //
            //   NOTE:  Node will return SAMPLE_HDR_FLAG_IQ_ERROR if in continuous Tx
            //       mode since the node will never be ready.
            //
            samp_hdr         = (wl_bb_samp_hdr *)cmd_args_32;

            // Parse the command arguments
            buff_sel         = (u32) Xil_Ntohs(samp_hdr->buff_sel);
            start_samp       = Xil_Ntohl(samp_hdr->start_samp);
            offset           = start_samp * sizeof(wl_samp);
            flags            = samp_hdr->flags;
            sample_iq_id     = samp_hdr->sample_iq_id;
            num_samp         = Xil_Ntohl(samp_hdr->num_samp);

            //
            // NOTE: Based on experimental results, jumbo Ethernet frames from Write IQ command can interfere
            //     with the operation of the Read IQ and Write IQ processes.  This is due to the processing
            //     of the frames interfering with the latency requirements of the transfers to / from DDR
            //     (Basically, since there is only one DMA to service both the Write IQ commands and the
            //     Read IQ / Write IQ processes, the DMA transfers for jumbo frames can cause latencies that
            //     result in overflow / underflow conditions in the IQ processes.  This does not occur in
            //     the Read IQ command because the Ethernet DMA is used to transfer data from DDR.  One option
            //     to address this in the future would be to have a separate DMA for only the Read IQ / Write
            //     IQ processes.)  However, with non-jumbo frames, there is no issue.  Therefore, we can
            //     allow the Write IQ to proceed if we are using non-jumbo frames but need to request the
            //     host wait if we are using jumbo frames.
            //
            check_status     = 0;
            status           = 0;
            raw_status       = wl_bb_get_raw_status();                         // Get raw status so there is only one register read from the baseband peripheral
            temp_status      = (raw_status & WL_BUF_REG_STATUS_TX_RUNNING);    // Current TX status

            // Check size of packet
            if (num_samp < 400) {
                // Number of samples is for a non-jumbo frame (approx 400 samples); process the Write IQ command
                check_status = 1;

            } else {
                //
                // NOTE:  For jumbo frames, if the Write IQ process is running for 2 buffers or less, then it is
                //     ok to process the Write IQ command.  Otherwise, we need to tell the host to wait.
                //
                // NOTE:  There is a special case that we are ignoring:  If the node is in continuous transmit
                //     mode and the waveform is less than WL_BUF_DEFAULT_TX_NUM_SAMPLES, then the DMA is not
                //     in use and it would be ok to process the Write IQ command so long as the Write IQ command
                //     is not to the buffer that is transmitting.  However, this additional checking would require
                //     a read from the baseband buffers peripheral and many additional operations.  Given that
                //     the Write IQ command is performance critical, we leave out this use case.  If this use case
                //     is critical for your experiment, then you can update the code to perform the check.
                //
                if (raw_status & WL_BUF_REG_STATUS_RX_RUNNING) {
                    // Read IQ process is running, tell host to wait
                    status = 1;

                } else {
                    if (one_bits[temp_status] > 2) {
                        // Write IQ process is running for more than two buffers, tell host to wait
                        status = 1;

                    } else {
                        // Write IQ process is running on two or less buffers so it is ok to process the Write IQ command
                        check_status = 1;

                    }
                }
            }

            // Check Write IQ command parameters
            if (check_status) {
                //
                // NOTE:  We will only allow a write of an IQ buffer that is currently transmitting data if the
                //     requested write is at least 16 kSamples (64 kB) behind the current write pointer (ie the
                //     current read byte offset of the baseband transmit process is 16 kSamples greater than that
                //     of the requested write).  This serves multiple purposes:
                //         1) It will ensure that the data is does not have any issues with the "chunking" that occurs
                //            in the transmit process (ie the Write IQ process moves 16 kSample "chunks" from DDR to
                //            BRAM to be used by the baseband buffers module.
                //         2) It will ensure that the read of IQ data over Ethernet does not exceed the writing of
                //            IQ data into the buffer from the baseband core.  While this could not happen when
                //            capturing 40 MHz of bandwidth, at higher decimation levels (ie capturing less bandwidth)
                //            this could be an issue.
                //
                temp_threshold = ((start_samp + (WL_BUF_TX_TRANSFER_THRESHOLD_SAMPLES)) << 2);     // Threshold in bytes
                temp_status    = (temp_status) & buff_sel;                                         // Are we trying to write to the currently transmitting buffer
                temp_offset    = (wl_bb_get_rf_tx_iq_buf_rd_byte_offset() + 4);
                status         = (temp_status) && (temp_offset < temp_threshold);
            }

            // Check if there is a transmission in progress to a buffer that is currently transmitting
            //     If yes, then tell the host to wait and request again.
            if (status) {

                if (wl_bb_get_config() & WL_BUF_REG_CONFIG_CONT_TX) {
                    // If we are in 'continuous tx' mode, then return 'error'
                    resp_args_32[resp_index++] = Xil_Htonl(SAMPLE_HDR_FLAG_IQ_ERROR);                        // Status
                    resp_args_32[resp_index++] = Xil_Htonl(sample_iq_id);                                    // ID

                } else {
                    // If we are not in 'continuous tx' mode, then return 'not ready'
                    resp_args_32[resp_index++] = Xil_Htonl(SAMPLE_HDR_FLAG_IQ_NOT_READY);                    // Status
                    resp_args_32[resp_index++] = Xil_Htonl(sample_iq_id);                                    // ID
                    resp_args_32[resp_index++] = 0x00000000;                                                 // Checksum
                    resp_args_32[resp_index++] = Xil_Htonl(wl_bb_get_tx_status());                           // Tx status
                    resp_args_32[resp_index++] = Xil_Htonl((wl_bb_get_rf_tx_iq_buf_rd_byte_offset() + 4));   // Tx pointer
                    resp_args_32[resp_index++] = Xil_Htonl(((wl_bb_get_tx_length() + 1) << 2));              // Tx length
                    resp_args_32[resp_index++] = Xil_Htonl(wl_bb_get_rx_status());                           // Rx status
                    resp_args_32[resp_index++] = Xil_Htonl((wl_bb_get_rf_rx_iq_buf_wr_byte_offset() + 4));   // Rx pointer
                    resp_args_32[resp_index++] = Xil_Htonl(((wl_bb_get_rx_length() + 1) << 2));              // Rx length
                }

                resp_hdr->length      += (resp_index * sizeof(resp_args_32));
                resp_hdr->num_args     = resp_index;

                resp_sent              = NODE_NOT_READY;
            } else {
                // Samples start after the sample header
                samp_addr              = (void *)samp_hdr + sizeof(wl_bb_samp_hdr);
                samp_addr_32           = (u32 *)samp_addr;
                samp_len               = num_samp * sizeof(wl_samp);
                checksum_input_32      = Xil_Ntohl(samp_addr_32[num_samp-1]);
                checksum_input_16      = (checksum_input_32 >> 16) ^ (0xFFFF & checksum_input_32);

                // Update the write checksum
                if(flags & SAMPLE_HDR_FLAG_CHKSUM_RESET){
                    curr_checksum = baseband_update_checksum( (start_samp & 0xFFFF), 1);
                }else{
                    curr_checksum = baseband_update_checksum( (start_samp & 0xFFFF), 0);
                }
                curr_checksum = baseband_update_checksum(checksum_input_16, 0);

                // Populate response header
                resp_args_32[resp_index++]  = Xil_Htonl(CMD_PARAM_SUCCESS);                        // Status
                resp_args_32[resp_index++]  = Xil_Htonl(sample_iq_id);                             // ID
                resp_args_32[resp_index++]  = Xil_Htonl(curr_checksum);                            // Checksum
                resp_hdr->length           += (resp_index * sizeof(resp_args_32));
                resp_hdr->num_args          = resp_index;

                write_tx_buffers(buff_sel, (u32)(samp_addr), offset, samp_len);

                // If this is the last transfer for a WRITE IQ, then we need to populate the temporary buffers
                // that have been written
                //
                // NOTE:  If there is currently a write to a buffer, then do not populate that TX buffer when
                //     the write IQ ends since the buffers will be populated by the final interrupt in the
                //     Write IQ process.
                //
                if (flags & SAMPLE_HDR_FLAG_LAST_WRITE) {

                    populate_tmp_tx_buffers(((~wl_bb_get_tx_status()) & buff_sel), 0x0, WARPLAB_IQ_TX_BUF_SIZE);
                }
            }
        break;

        
        //---------------------------------------------------------------------
        case CMDID_BASEBAND_WRITE_IQ_CHECKSUM:
            // wl_printf(WL_PRINT_DEBUG, print_type_baseband, "Get Write IQ checksum\n");

            resp_args_32[resp_index++] = Xil_Htonl(baseband_get_checksum());

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;

        
        //---------------------------------------------------------------------
        case CMDID_BASEBAND_READ_IQ:
        case CMDID_BASEBAND_READ_RSSI:
            // BB_READ_IQ / BB_READ_RSSI Packet Format:
            //
            //   - cmd_args_32[0]      - Buffer selection (guaranteed to be singular) (uint16)
            //   - cmd_args_32[1]      - Start sample
            //   - cmd_args_32[2]      - Total samples in transfer
            //   - cmd_args_32[3]      - Maximum number of samples per packet
            //   - cmd_args_32[4]      - Number of packets in transfer
            //
            //   - resp_args           - Samples:  wl_bb_samp_hdr followed by appropriate samples
            //
            //   NOTE:  If the sample header flags == SAMPLE_HDR_FLAG_IQ_NOT_READY, then the "samples"
            //       after the sample header need to be interpreted in the following manner:
            //
            //       - sample[0]       - Tx status
            //       - sample[1]       - Current Tx read pointer
            //       - sample[2]       - Tx length
            //       - sample[3]       - Rx status
            //       - sample[4]       - Current Rx write pointer
            //       - sample[5]       - Rx length
            //
            //   NOTE:  When SAMPLE_HDR_FLAG_IQ_NOT_READY is returned, then the host
            //       can compute the maximum wait time by the following calculation:
            //           max(((tx_status) ? (tx_length - tx_read_pointer) : 0), ((rx_status) ? (rx_length - rx_write_pointer) : 0));
            //
            //   NOTE:  The response buffer (ie the response->buffer in the wl_cmd_resp) starts at the transport header.  Therefore,
            //       we need to increase the size of the buffer by:  sizeof(wl_cmd_resp_hdr) + sizeof(wl_bb_samp_hdr);  The size of
            //       the sample buffer will be (num_samples * 4);
            //

            // wl_printf(WL_PRINT_DEBUG, print_type_baseband, "Read IQ\n");

            // Process command arguments
            buff_sel              = Xil_Ntohl(cmd_args_32[0]);
            start_samp            = Xil_Ntohl(cmd_args_32[1]);
            total_samp            = Xil_Ntohl(cmd_args_32[2]);
            max_samp_len_per_pkt  = Xil_Ntohl(cmd_args_32[3]);
            num_pkts              = Xil_Ntohl(cmd_args_32[4]);

            // Set the sample_iq_id
            //   NOTE:  This is the lower 8 bits of the RX counter for the given buffer.  Since buff_sel is
            //       guaranteed to be a single value (ie one of RFA, RFB, RFC, or RFD), this will always
            //       select the appropriate RX counter.
            //
            sample_iq_id          = 0;

            if      (buff_sel & RF_SEL_A) { sample_iq_id = (wl_bb_get_rfa_rx_count() & 0x000000FF); }
            else if (buff_sel & RF_SEL_B) { sample_iq_id = (wl_bb_get_rfb_rx_count() & 0x000000FF); }
            else if (buff_sel & RF_SEL_C) { sample_iq_id = (wl_bb_get_rfc_rx_count() & 0x000000FF); }
            else if (buff_sel & RF_SEL_D) { sample_iq_id = (wl_bb_get_rfd_rx_count() & 0x000000FF); }

            // Calculate the maximum samples per packet
            max_samp_per_pkt      = max_samp_len_per_pkt / sizeof(wl_samp);      // This is an constant integer division that is optimized away by the compiler

            // Initialize loop variables
            num_samp              = 0;
            curr_samp             = start_samp;
            dest_addr             = (u32)((void*)resp_args_32 + sizeof(wl_bb_samp_hdr));

            //
            // NOTE:  We will only allow a read of an IQ buffer that is currently receiving data if the
            //     requested read has been completely received (ie the current write byte offset of the
            //     baseband receive process is greater than the last sample of the requested read).  This
            //     serves multiple purposes:
            //         1) It will ensure that the data is not known garbage data.  (A user could still read a
            //            buffer that has never been populated with real data or mistakenly read a buffer.
            //            However, there is no way to protect against that.)
            //         2) It will ensure that the read of IQ data over Ethernet does not exceed the writing of
            //            IQ data into the buffer from the baseband core.  While this could not happen when
            //            capturing 40 MHz of bandwidth, at higher decimation levels (ie capturing less bandwidth)
            //            this could be an issue.
            //
            // NOTE:  The threshold is based on the starting sample and then the Read IQ "chunk" size (ie the
            //     Read IQ process will transfer 16 kSample "chunks" from BRAM to DDR, which is where is it
            //     read for the Read IQ command).
            //
            temp_threshold = ((start_samp + (WL_BUF_RX_TRANSFER_THRESHOLD_SAMPLES)) << 2);
            temp_status    = (wl_bb_get_rx_status()) & buff_sel;
            temp_offset    = (wl_bb_get_rf_rx_iq_buf_wr_byte_offset() + 4);
            status         = (temp_status) && (temp_offset < temp_threshold);


            // Check if we need to defer the read request due to an ongoing reception
            //     If yes, then tell the host to wait and request again
            //
            if (status) {

                // Fill in parts of sample header that do not change between Write IQ packets
                samp_hdr                   = (wl_bb_samp_hdr *)resp_args_32;
                samp_hdr->buff_sel         = (u16)buff_sel;
                samp_hdr->buff_sel         = Xil_Htons(samp_hdr->buff_sel);

                samp_hdr->flags            = SAMPLE_HDR_FLAG_IQ_NOT_READY;

                resp_args_32               = (u32 *)dest_addr;

                resp_args_32[resp_index++] = Xil_Htonl(wl_bb_get_tx_status());                           // Tx status
                resp_args_32[resp_index++] = Xil_Htonl((wl_bb_get_rf_tx_iq_buf_rd_byte_offset() + 4));   // Tx pointer
                resp_args_32[resp_index++] = Xil_Htonl(((wl_bb_get_tx_length() + 1) << 2));              // Tx length
                resp_args_32[resp_index++] = Xil_Htonl(wl_bb_get_rx_status());                           // Rx status
                resp_args_32[resp_index++] = Xil_Htonl((wl_bb_get_rf_rx_iq_buf_wr_byte_offset() + 4));   // Rx pointer
                resp_args_32[resp_index++] = Xil_Htonl(((wl_bb_get_rx_length() + 1) << 2));              // Rx length

                samp_hdr->sample_iq_id     = sample_iq_id;
                samp_hdr->start_samp       = 0;
                samp_hdr->num_samp         = 0;

                resp_hdr->length           = sizeof(wl_bb_samp_hdr) + (resp_index * sizeof(resp_args_32));
                resp_hdr->num_args         = 1;

                resp_sent                  = NODE_NOT_READY;

            } else {
                //
                // NOTE:  Currently, the Ethernet send function only blocks when it runs out of transmit
                //   buffer descriptors.  If we perform all header modifications in place, this will cause
                //   problems with Read IQ when WARP_IP_UDP_TXBD_CNT (ie the number of TX BDs) is greater
                //   than 5 because the Ethernet DMA will not have transfered the header before the next
                //   round of processing that modifies the header.  In order to fix this, we are going to
                //   create multiple copies of the packet header in the buffer we allocated above.  The
                //   contents of the packet header are:
                //
                //   Read IQ Packet Header (76 bytes total):
                //       Eth header       = 14 bytes
                //       IP header        = 20 bytes
                //       UDP header       =  8 bytes
                //       Delimiter        =  2 bytes
                //       Transport header = 12 bytes
                //       Command header   =  8 bytes
                //       Sample header    = 12 bytes;
                //
                //   In order to minimize the impact to performance, we are going to pull the header from
                //   AXI BRAM into LMB memory for processing and then copy the completed header back to
                //   AXI BRAM so the Ethernet DMA can fetch it.  This way we can maintain the header for
                //   the current packet and then copy it to multiple locations to avoid overwriting the
                //   header before the DMA can transfer its contents.
                //

                // Initialize Read IQ response buffer array
                //     NOTE:  header_buffer fields will be set here; sample_buffer fields will be set in read_rx_buffers
                //
                read_iq_resp[0]   = (void *) &header_buffer;
                read_iq_resp[1]   = (void *) &sample_buffer;

                // Set up temporary pointers to the header data
                eth_ip_udp_header = (warp_ip_udp_header  *)(&tmp_header[0]);
                wl_header_tx      = (wl_transport_header *)(&tmp_header[sizeof(warp_ip_udp_header)]);
                resp_hdr          = (wl_cmd_resp_hdr     *)(&tmp_header[sizeof(warp_ip_udp_header) + sizeof(wl_transport_header)]);
                samp_hdr          = (wl_bb_samp_hdr      *)(&tmp_header[sizeof(warp_ip_udp_header) + sizeof(wl_transport_header) + sizeof(wl_cmd_resp_hdr)]);

                // Set up temporary variables with the length values of the header
                ip_length         = WARP_IP_UDP_DELIM_LEN + UDP_HEADER_LEN + IP_HEADER_LEN_BYTES;
                udp_length        = WARP_IP_UDP_DELIM_LEN + UDP_HEADER_LEN;
                header_length     = sizeof(wl_transport_header) + sizeof(wl_cmd_resp_hdr) + sizeof(wl_bb_samp_hdr);
                total_hdr_length  = sizeof(warp_ip_udp_header) + header_length;

                // Get values out of the socket address structure
                dest_ip_addr      = ((struct sockaddr_in*)from)->sin_addr.s_addr;    // NOTE:  Value big endian
                dest_port         = ((struct sockaddr_in*)from)->sin_port;

                // Get hardware address of the destination
                eth_dev_num       = socket_get_eth_dev_num(socket_index);
                arp_get_hw_addr(eth_dev_num, dest_hw_addr, (u8 *)(&dest_ip_addr));

                // Pull in header information into local LMB memory:
                //   - Copy the header information from the socket
                //   - Copy the information from the response
                //
                memcpy((void *)eth_ip_udp_header, (void *)socket_get_warp_ip_udp_header(socket_index), sizeof(warp_ip_udp_header));
                memcpy((void *)wl_header_tx, (void *)(((warp_ip_udp_buffer *)(response->buffer))->data), header_length);

                // Initialize header buffer size/length (see above for description)
                header_buffer.length = total_hdr_length;
                header_buffer.size   = total_hdr_length;

                //
                // NOTE:  In order to make large Read IQ transfers more efficient, we can pre-process most of
                //   of the response packet such that the transport has to do only the minimal amount of
                //   processing per packet.  This should not cause any additional overhead for a single packet
                //   but will have have reduced overhead for all other packets.
                //

                // Fill in parts of sample header that do not change between Read IQ packets
                samp_hdr->buff_sel     = (u16)buff_sel;
                samp_hdr->buff_sel     = Xil_Htons(samp_hdr->buff_sel);
                samp_hdr->sample_iq_id = sample_iq_id;
                samp_hdr->flags        = 0;

                // Populate response header fields with static data
                resp_hdr->cmd          = Xil_Ntohl(resp_hdr->cmd);
                resp_hdr->num_args     = Xil_Ntohs(1);

                // Populate transport header fields with static data
                wl_header_tx->dest_id  = Xil_Htons(wl_header_tx->dest_id);
                wl_header_tx->src_id   = Xil_Htons(wl_header_tx->src_id);
                wl_header_tx->seq_num  = Xil_Htons(wl_header_tx->seq_num);
                wl_header_tx->flags    = Xil_Htons(wl_header_tx->flags);

                // Update the Ethernet header
                //     NOTE:  dest_hw_addr must be big-endian; ethertype must be little-endian
                //     NOTE:  Adapted from the function:
                //                eth_update_header(&(eth_ip_udp_header->eth_hdr), dest_hw_addr, ETHERTYPE_IP_V4);
                //
                memcpy((void *)eth_ip_udp_header->eth_hdr.dest_mac_addr, (void *)dest_hw_addr, ETH_MAC_ADDR_LEN);
                eth_ip_udp_header->eth_hdr.ethertype  = Xil_Htons(ETHERTYPE_IP_V4);

                // Update the UDP header
                //     NOTE:  Requires dest_port to be big-endian; udp_length to be little-endian
                //     NOTE:  Adapted from the function:
                //                udp_update_header(&(eth_ip_udp_header->udp_hdr), dest_port, (udp_length + data_length));
                //
                eth_ip_udp_header->udp_hdr.dest_port  = dest_port;
                eth_ip_udp_header->udp_hdr.checksum   = UDP_NO_CHECKSUM;

                // Set AXI BRAM address for the header
                header_base_addr       = ETH_IQ_buffer;             // Use the buffer allocated above
                header_offset          = 0;
                header_buffer_size     = WL_BASEBAND_ETH_BUFFER_SIZE * WL_BASEBAND_ETH_NUM_BUFFER;

                // Process the Read IQ / Read RSSI packets
                for(i = 0; i < num_pkts; i++){

                    // Update loop variables
                    header_addr     = (u8 *)(((u32)header_base_addr) + header_offset);
                    next_start_samp = curr_samp + max_samp_per_pkt;

                    if(next_start_samp > (start_samp + total_samp)){
                        num_samp = (start_samp + total_samp) - curr_samp;
                    } else {
                        num_samp = max_samp_per_pkt;
                    }

                    samp_len             = num_samp * sizeof(wl_samp);
                    start_byte           = curr_samp * sizeof(wl_samp);
                    data_length          = samp_len + header_length;

                    // Populate sample header fields with per packet data
                    samp_hdr->start_samp = Xil_Htonl(curr_samp);
                    samp_hdr->num_samp   = Xil_Htonl(num_samp);

                    // Populate response header fields with per packet data
                    resp_hdr->length     = Xil_Ntohs(samp_len + sizeof(wl_bb_samp_hdr));

                    // Populate transport header fields with per packet data
                    wl_header_tx->length = Xil_Htons(data_length + WARP_IP_UDP_DELIM_LEN);

                    // Update the UDP header
                    //     NOTE:  Requires dest_port to be big-endian; udp_length to be little-endian
                    //     NOTE:  Adapted from the function:
                    //                udp_update_header(&(eth_ip_udp_header->udp_hdr), dest_port, (udp_length + data_length));
                    //
                    eth_ip_udp_header->udp_hdr.length = Xil_Htons(udp_length + data_length);

                    // Update the IPv4 header
                    //     NOTE:  Requires dest_ip_addr to be big-endian; ip_length to be little-endian
                    //     NOTE:  We did not break this function apart like the other header updates b/c the IP ID counter is
                    //            maintained in the library and we did not want to violate that.
                    //
                    ipv4_update_header(&(eth_ip_udp_header->ip_hdr), dest_ip_addr, (ip_length + data_length), IP_PROTOCOL_UDP);

                    // Copy the completed header to DMA accessible BRAM
                    memcpy((void *)header_addr, (void *)tmp_header, total_hdr_length);

                    // Set the header buffer data / offset
                    header_buffer.data   = (u8 *)header_addr;
                    header_buffer.offset = (u8 *)header_addr;

                    // Set up the IQ data for the Ethernet packet buffer
                    read_rx_buffers(cmd_id, buff_sel, start_byte, samp_len, dest_addr, &sample_buffer);

                    // Update the green LEDs for every packet sent
                    increment_green_leds_one_hot();

                    // Send the Ethernet packet
                    //   NOTE:  In an effort to reduce overhead (ie improve performance) for Read IQ, we are using the "raw"
                    //       socket_sendto method which transmits the provided buffers "as is" (ie there are no header updates
                    //       or other modifications to the buffer data).  Also, we have consolidated all the headers into a
                    //       single buffer so that a Read IQ Ethernet packet only requires two Transmit Buffer Descriptors
                    //       (TX BDs).
                    //
                    status = socket_sendto_raw(socket_index, (warp_ip_udp_buffer **)read_iq_resp, 0x2);

                    // Check that the packet was sent correctly
                    if (status == WARP_IP_UDP_FAILURE) {
                        wl_printf(WL_PRINT_WARNING, print_type_baseband, "Issue sending read IQ packet to host.\n");
                    }

                    // Update loop variables
                    curr_samp          = next_start_samp;
                    header_offset      = (header_offset + WL_BASEBAND_ETH_BUFFER_SIZE) % header_buffer_size;
                }

                //
                // NOTE:  With the current processing, the state of the transmit buffer has not been modified
                //   from what was passed in to this function.  All modifications occurred on temporary headers
                //   that were buffered in either LMB or BRAM memory.
                //

                resp_sent = RESP_SENT;
            }
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_TXRX_COUNT_RESET:
            // Reset the TX / RX counters
            //
            // Message format:
            //     cmd_args_32[0]      Command:
            //                             - Write       (NODE_WRITE_VAL)
            //                             - Read        (NODE_READ_VAL)
            //     cmd_args_32[1]      Buffer Select
            //     cmd_args_32[2]      TX = 0 / RX = 1
            //
            // Response format:
            //     resp_args_32[0]     Status
            //
            status      = CMD_PARAM_SUCCESS;
            msg_cmd     = Xil_Ntohl(cmd_args_32[0]);
            buff_sel    = Xil_Ntohl(cmd_args_32[1]);
            txrx_sel    = Xil_Ntohl(cmd_args_32[2]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    if (txrx_sel) {
                        wl_bb_clear_txrx_counter_reset();
                        wl_bb_set_txrx_counter_reset(buff_sel << 8);
                        wl_bb_clear_txrx_counter_reset();
                    } else {
                        wl_bb_clear_txrx_counter_reset();
                        wl_bb_set_txrx_counter_reset(buff_sel);
                        wl_bb_clear_txrx_counter_reset();
                    }
                break;

                case CMD_PARAM_READ_VAL:
                    status = CMD_PARAM_ERROR;
                break;

                default:
                    wl_printf(WL_PRINT_ERROR, print_type_baseband, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_TXRX_COUNT_GET:
            // Get the TX / RX counter value
            //
            // Message format:
            //     cmd_args_32[0]      Command:
            //                             - Write       (NODE_WRITE_VAL)
            //                             - Read        (NODE_READ_VAL)
            //     cmd_args_32[1]      Buffer Select
            //     cmd_args_32[2]      TX / RX Select
            //                             - TX          (CMD_PARAM_BASEBAND_TXRX_COUNT_GET_TX)
            //                             - RX          (CMD_PARAM_BASEBAND_TXRX_COUNT_GET_RX)
            //
            // Response format:
            //     resp_args_32[0]     Status
            //     resp_args_32[1]     Selected ounter value
            //
            status           = CMD_PARAM_SUCCESS;
            msg_cmd          = Xil_Ntohl(cmd_args_32[0]);
            buff_sel         = Xil_Ntohl(cmd_args_32[1]);
            txrx_sel         = Xil_Ntohl(cmd_args_32[2]);
            buff_counter     = CMD_PARAM_BASEBAND_TXRX_COUNT_GET_COUNT_RSVD;

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    status = CMD_PARAM_ERROR;
                break;

                case CMD_PARAM_READ_VAL:
                    buff_counter = get_buffer_counter(txrx_sel, buff_sel);
                break;

                default:
                    wl_printf(WL_PRINT_ERROR, print_type_baseband, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(buff_counter);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_AGC_STATE:
            // Get the AGC State
            //
            // Response format:
            //     resp_args_32[N]     Gains ([1:0] RF gain; [6:2] BB gain)
            //     resp_args_32[N + 1] RSSI observed when gains were locked
            //
            // NOTE:  Host side processing of the return value needs to be careful that it understands
            //     which RF interfaces were requested since the below implementation will collapse the
            //     return value to transmit the minimal amount of data.
            //
            rf_sel =  Xil_Ntohl(cmd_args_32[0]);

            if(rf_sel & AGC_A){
                resp_args_32[resp_index++] = Xil_Htonl(wl_get_agc_RFG(ANT_A) + (wl_get_agc_BBG(ANT_A) << 2));
                resp_args_32[resp_index++] = Xil_Htonl(wl_bb_get_rfa_agc_done_rssi());
            }
            if(rf_sel & AGC_B){
                resp_args_32[resp_index++] = Xil_Htonl(wl_get_agc_RFG(ANT_B) + (wl_get_agc_BBG(ANT_B) << 2));
                resp_args_32[resp_index++] = Xil_Htonl(wl_bb_get_rfb_agc_done_rssi());
            }
            if(rf_sel & AGC_C){
                resp_args_32[resp_index++] = Xil_Htonl(wl_get_agc_RFG(ANT_C) + (wl_get_agc_BBG(ANT_C) << 2));
                resp_args_32[resp_index++] = Xil_Htonl(wl_bb_get_rfc_agc_done_rssi());
            }
            if(rf_sel & AGC_D){
                resp_args_32[resp_index++] = Xil_Htonl(wl_get_agc_RFG(ANT_D) + (wl_get_agc_BBG(ANT_D) << 2));
                resp_args_32[resp_index++] = Xil_Htonl(wl_bb_get_rfd_agc_done_rssi());
            }

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_AGC_DONE_ADDR:
            // Get the address the AGC finished
            //
            // Response format:
            //     resp_args_32[0]     Sample address AGC finished
            //
            resp_args_32[resp_index++] = Xil_Htonl(wl_bb_get_agc_done_addr());

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_AGC_RESET:
            // Reset the AGC
            //
            // NOTE:  No arguments are expected and nothing is returned
            //
            warplab_agc_reset();
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_AGC_RESET_MODE:
            // Get / Set the AGC reset mode
            //
            // Message format:
            //     cmd_args_32[0]      Command:
            //                             - Write       (NODE_WRITE_VAL)
            //                             - Read        (NODE_READ_VAL)
            //     cmd_args_32[1]      Enable / Disable reset per rx  (Write only)
            //
            // Response format:
            //     resp_args_32[0]     Status
            //     resp_args_32[1]     Value of the reset mode
            //
            status  = CMD_PARAM_SUCCESS;
            msg_cmd = Xil_Ntohl(cmd_args_32[0]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    if (Xil_Ntohl(cmd_args_32[1]) == WL_AGC_RESET_MODE_RESET_PER_RX_MASK) {
                        wl_agc_enable_reset_per_rx();
                    } else {
                        wl_agc_disable_reset_per_rx();
                    }
                break;

                case CMD_PARAM_READ_VAL:
                break;

                default:
                    wl_printf(WL_PRINT_ERROR, print_type_baseband, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(wl_agc_get_reset_mode());

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_AGC_TARGET:
            // Set the AGC target
            //
            // Message format:
            //     cmd_args_32[0]      AGC target
            //
            agc_target = Xil_Ntohl(cmd_args_32[0]);

            wl_agc_set_target(agc_target);       // Note:  AGC target is a Fix_6_0 value
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_AGC_DCO_EN_DIS:
            // Enable / Disable AGC DC Offset (DCO) correction
            //
            // Message format:
            //     cmd_args_32[0]      Enable = 1 / Disable = 0
            //
            agc_dco_enable = Xil_Ntohl(cmd_args_32[0]);

            warplab_agc_enable_DCO(agc_dco_enable);
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_AGC_CONFIG:
            // Get / Set AGC configuration
            //
            // Message format:
            //     cmd_args_32[0]      Command:
            //                             - Write       (NODE_WRITE_VAL)
            //                             - Read        (NODE_READ_VAL)
            //     cmd_args_32[1]      RSSI Averaging length
            //     cmd_args_32[2]      Voltage DB Adjust
            //     cmd_args_32[3]      Initial BB Gain
            //
            // Response format:
            //     resp_args_32[0]     Status
            //
            status  = CMD_PARAM_SUCCESS;
            msg_cmd = Xil_Ntohl(cmd_args_32[0]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    rssi_avg_length = Xil_Ntohl(cmd_args_32[1]) & 0x00000003;
                    v_db_adjust     = Xil_Ntohl(cmd_args_32[2]) & 0x0000003F;
                    init_bb_gain    = Xil_Ntohl(cmd_args_32[3]) & 0x0000001F;

                    wl_agc_set_config(rssi_avg_length, v_db_adjust, init_bb_gain);
                break;

                case CMD_PARAM_READ_VAL:
                    // Read not supported
                    status = CMD_PARAM_ERROR;
                break;

                default:
                    wl_printf(WL_PRINT_ERROR, print_type_baseband, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_AGC_IIR_HPF:
            // Get / Set Infinite Impulse Response (IIR) High Pass Filter (HPF) coefficients
            //
            // Message format:
            //     cmd_args_32[0]      Command:
            //                             - Write       (NODE_WRITE_VAL)
            //                             - Read        (NODE_READ_VAL)
            //     cmd_args_32[1]      A1 coefficient (value is already converted Fix_18_17)
            //     cmd_args_32[2]      B0 coefficient (value is already converted UFix_18_17)
            //
            // Response format:
            //     resp_args_32[0]     Status
            //
            // NOTE:  The values are assumed to already be in the correct format
            //
            status  = CMD_PARAM_SUCCESS;
            msg_cmd = Xil_Ntohl(cmd_args_32[0]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    a1_coeff = Xil_Ntohl(cmd_args_32[1]) & 0x0003FFFF;
                    b0_coeff = Xil_Ntohl(cmd_args_32[2]) & 0x0003FFFF;

                    wl_agc_set_iir_coef_a1(a1_coeff);               // This value is  Fix_18_17
                    wl_agc_set_iir_coef_b0(b0_coeff);               // This value is UFix_18_17
                break;

                case CMD_PARAM_READ_VAL:
                    // Read not supported
                    status = CMD_PARAM_ERROR;
                break;

                default:
                    wl_printf(WL_PRINT_ERROR, print_type_baseband, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_AGC_RF_GAIN_THRESHOLD:
            // Get / Set RF gain (LNA) thresholds
            //
            // Message format:
            //     cmd_args_32[0]      Command:
            //                             - Write       (NODE_WRITE_VAL)
            //                             - Read        (NODE_READ_VAL)
            //     cmd_args_32[1]      3 -> 2 RF gain power threshold
            //     cmd_args_32[2]      2 -> 1 RF gain power threshold
            //
            // Response format:
            //     resp_args_32[0]     Status
            //
            // NOTE:  The thresholds are assumed to be in UFix_8_0 two's compliment format
            //
            status  = CMD_PARAM_SUCCESS;
            msg_cmd = Xil_Ntohl(cmd_args_32[0]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    thresh_3_2 = Xil_Ntohl(cmd_args_32[1]) & 0x000000FF;
                    thresh_2_1 = Xil_Ntohl(cmd_args_32[2]) & 0x000000FF;

                    wl_agc_set_config_thresh(thresh_3_2, thresh_2_1);
                break;

                case CMD_PARAM_READ_VAL:
                    // Read not supported
                    status = CMD_PARAM_ERROR;
                break;

                default:
                    wl_printf(WL_PRINT_ERROR, print_type_baseband, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_AGC_TIMING:
            // Get / Set
            //
            // Message format:
            //     cmd_args_32[0]      Command:
            //                             - Write       (NODE_WRITE_VAL)
            //                             - Read        (NODE_READ_VAL)
            //     cmd_args_32[1]      Capture RSSI 1
            //     cmd_args_32[2]      Capture RSSI 2
            //     cmd_args_32[3]      Capture Voltage DB
            //     cmd_args_32[4]      AGC Done
            //
            // Response format:
            //     resp_args_32[0]     Status
            //
            status  = CMD_PARAM_SUCCESS;
            msg_cmd = Xil_Ntohl(cmd_args_32[0]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    capture_rssi_1 = Xil_Ntohl(cmd_args_32[1]) & 0x000000FF;
                    capture_rssi_2 = Xil_Ntohl(cmd_args_32[2]) & 0x000000FF;
                    capture_v_db   = Xil_Ntohl(cmd_args_32[3]) & 0x000000FF;
                    agc_done       = Xil_Ntohl(cmd_args_32[4]) & 0x000000FF;

                    wl_agc_set_AGC_timing(capture_rssi_1, capture_rssi_2, capture_v_db, agc_done);
                break;

                case CMD_PARAM_READ_VAL:
                    // Read not supported
                    status = CMD_PARAM_ERROR;
                break;

                default:
                    wl_printf(WL_PRINT_ERROR, print_type_baseband, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_BASEBAND_AGC_DCO_TIMING:
            // Get / Set
            //
            // Message format:
            //     cmd_args_32[0]      Command:
            //                             - Write       (NODE_WRITE_VAL)
            //                             - Read        (NODE_READ_VAL)
            //     cmd_args_32[1]      Start DCO
            //     cmd_args_32[2]      Start IIR HPF
            //
            // Response format:
            //     resp_args_32[0]     Status
            //
            status  = CMD_PARAM_SUCCESS;
            msg_cmd = Xil_Ntohl(cmd_args_32[0]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    start_dco        = Xil_Ntohl(cmd_args_32[1]) & 0x000000FF;
                    start_iir_filter = Xil_Ntohl(cmd_args_32[2]) & 0x000000FF;

                    wl_agc_set_DCO_timing(start_dco, start_iir_filter);
                break;

                case CMD_PARAM_READ_VAL:
                    // Read not supported
                    status = CMD_PARAM_ERROR;
                break;

                default:
                    wl_printf(WL_PRINT_ERROR, print_type_baseband, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        // case BB_DEBUG_TX_OUTPUT_CONFIGURE:
        //     wl_printf(WL_PRINT_DEBUG, print_type_baseband, "Tx debug output configure\n");
        //
        //     // Clear current Tx debug output configuration
        //     temp = (WL_BUF_REG_CONFIG_DEBUG_TX_OUTPUT_SEL | WL_BUF_REG_CONFIG_DEBUG_TX_BUF_SEL);
        //     wl_bb_clear_config(temp);
        //
        //     // Set new Tx debug output configuration
        //     temp = (Xil_Ntohl(cmd_args_32[0]) & temp);
        //     wl_bb_set_config(temp);
        // break;


        //---------------------------------------------------------------------
        default:
            wl_printf(WL_PRINT_ERROR, print_type_baseband, "Unknown command ID: %d\n", cmd_id);
        break;
    }
    return resp_sent;
}



/*****************************************************************************/
/**
 * Read RX buffers
 *
 *   Sets up the warp_ip_udp_buffer with data from the buffer indicated by the buffer_sel
 * parameter, using the priority RFA -> RFB -> RFC -> RFD.  If there is an addressing
 * issue, then the function copies zeros into the given destination address and uses that
 * for the warp_ip_udp_buffer.
 *
 * @param   cmd_id           - Command ID (Differentiates between READ_IQ and READ_RSSI commands
 * @param   buffer_sel       - Buffer select (Indicates which buffer should be read)
 * @param   offset           - Starting byte offset into RX buffer (in bytes; must be 16 byte
 *                             aligned due to CDMA usage)
 * @param   length           - Length of the transfer (in bytes)
 * @param   dest_addr        - Destination address of the transfer (in bytes; must be 16 byte
 *                             aligned due to CDMA usage)
 *
 * @return  None
 *
 *****************************************************************************/
void read_rx_buffers(u32 cmd_id, u32 buffer_sel, u32 offset, u32 length, u32 dest_addr, warp_ip_udp_buffer * buffer) {

    u32 src_addr       = 0;
    u32 buffer_size    = 0;
    u32 end_byte       = offset + length - 1;

    // Process Read IQ
    if(cmd_id == CMDID_BASEBAND_READ_IQ) {
        if(buffer_sel & RF_SEL_A) {
            buffer_size = wl_iq_rx_buff_a_size;
            src_addr    = (u32)(wl_iq_rx_buff_a + offset);

        } else if(buffer_sel & RF_SEL_B) {
            buffer_size = wl_iq_rx_buff_b_size;
            src_addr    = (u32)(wl_iq_rx_buff_b + offset);

        } else if(buffer_sel & RF_SEL_C) {
            if (WARPLAB_CONFIG_4RF) {
                buffer_size = wl_iq_rx_buff_c_size;
                src_addr    = (u32)(wl_iq_rx_buff_c + offset);
            } else {
                wl_printf(WL_PRINT_ERROR, print_type_baseband, "Trying to read to RF C buffer on a 2RF design.\n");
            }

        } else if(buffer_sel & RF_SEL_D) {
            if (WARPLAB_CONFIG_4RF) {
                buffer_size = wl_iq_rx_buff_d_size;
                src_addr    = (u32)(wl_iq_rx_buff_d + offset);
            } else {
                wl_printf(WL_PRINT_ERROR, print_type_baseband, "Trying to read to RF D buffer on a 2RF design.\n");
            }
        }
    }

    // Process Read RSSI
    if(cmd_id == CMDID_BASEBAND_READ_RSSI) {
        if(buffer_sel & RF_SEL_A) {
            buffer_size = wl_rssi_buff_a_size;
            src_addr    = (u32)(wl_rssi_buff_a + offset);

        } else if(buffer_sel & RF_SEL_B) {
            buffer_size = wl_rssi_buff_b_size;
            src_addr    = (u32)(wl_rssi_buff_b + offset);

        } else if(buffer_sel & RF_SEL_C) {
            if (WARPLAB_CONFIG_4RF) {
                buffer_size = wl_rssi_buff_c_size;
                src_addr    = (u32)(wl_rssi_buff_c + offset);
            } else {
                wl_printf(WL_PRINT_ERROR, print_type_baseband, "Trying to read to RF C RSSI buffer on a 2RF design.\n");
            }

        } else if(buffer_sel & RF_SEL_D) {
            if (WARPLAB_CONFIG_4RF) {
                buffer_size = wl_rssi_buff_d_size;
                src_addr    = (u32)(wl_rssi_buff_d + offset);
            } else {
                wl_printf(WL_PRINT_ERROR, print_type_baseband, "Trying to read to RF D RSSI buffer on a 2RF design.\n");
            }
        }
    }

    // Transfer data or zeros
    //   NOTE:  buffer_size is initialized to zero so it will return zeros when RFC / RFD errors occur
    if (end_byte <= buffer_size) {
        buffer->data   = (u8 *)src_addr;
        buffer->offset = (u8 *)src_addr;
        buffer->length = length;
        buffer->size   = length;
    } else {
        wl_printf(WL_PRINT_ERROR, print_type_baseband, "Too many bytes read from buffer - Size = %d;  Read end = %d\n", buffer_size, end_byte);
        bzero((void *)(dest_addr), length);

        // Create an error buffer w/ zero data
        buffer->data   = (u8 *)dest_addr;
        buffer->offset = (u8 *)dest_addr;
        buffer->length = length;
        buffer->size   = length;
    }
}



/*****************************************************************************/
/**
 * Write TX buffers
 *
 *     Writes from the source address to all of the buffers indicated by the buffer_sel
 * parameter.
 *
 * @param   buffer_sel       - Buffer select (Indicates which buffer(s) should be written)
 * @param   src_addr         - Source address of the transfer (in bytes; must be 16 byte
 *                             aligned due to CDMA usage)
 * @param   offset           - Starting byte offset into TX buffer (in bytes; must be 16 byte
 *                             aligned due to CDMA usage)
 * @param   length           - Length of the transfer (in bytes)
 *
 * @return  None
 *
 *****************************************************************************/
void write_tx_buffers(u32 buffer_sel, u32 src_addr, u32 offset, u32 length) {

    u32 dest_addr      = 0;
    u32 end_byte       = offset + length - 1;

    // Process RFA
    if(buffer_sel & RF_SEL_A) {
        dest_addr = (u32)(wl_iq_tx_buff_a) + offset;

        if ( end_byte <= wl_iq_tx_buff_a_size ) {
            baseband_transfer_data(src_addr, dest_addr, length);
        } else {
            wl_printf(WL_PRINT_ERROR, print_type_baseband, "Too many bytes written to buffer RFA - Size = %d;  Write end = %d\n", wl_iq_tx_buff_a_size, end_byte);
            bzero((void *)(dest_addr), length);
        }
    }

    // Process RFB
    if(buffer_sel & RF_SEL_B) {
        dest_addr = (u32)(wl_iq_tx_buff_b) + offset;

        if ( end_byte <= wl_iq_tx_buff_b_size ) {
            baseband_transfer_data(src_addr, dest_addr, length);
        } else {
            wl_printf(WL_PRINT_ERROR, print_type_baseband, "Too many bytes written to buffer RFB - Size = %d;  Write end = %d\n", wl_iq_tx_buff_b_size, end_byte);
            bzero((void *)(dest_addr), length);
        }
    }

    // Process RFC
    if(buffer_sel & RF_SEL_C) {
        if (WARPLAB_CONFIG_4RF) {
            dest_addr = (u32)(wl_iq_tx_buff_c) + offset;

            if ( end_byte <= wl_iq_tx_buff_c_size ) {
                baseband_transfer_data(src_addr, dest_addr, length);
            } else {
                wl_printf(WL_PRINT_ERROR, print_type_baseband, "Too many bytes written to buffer RFC - Size = %d;  Write end = %d\n", wl_iq_tx_buff_c_size, end_byte);
                bzero((void *)(dest_addr), length);
            }
        } else {
            wl_printf(WL_PRINT_ERROR, print_type_baseband, "Trying to write to RF C buffer on a 2RF design.\n");
        }
    }

    // Process RFD
    if(buffer_sel & RF_SEL_D) {
        if (WARPLAB_CONFIG_4RF) {
            dest_addr = (u32)(wl_iq_tx_buff_d) + offset;

            if ( end_byte <= wl_iq_tx_buff_d_size ) {
                baseband_transfer_data(src_addr, dest_addr, length);
            } else {
                wl_printf(WL_PRINT_ERROR, print_type_baseband, "Too many bytes written to buffer RFD - Size = %d;  Write end = %d\n", wl_iq_tx_buff_d_size, end_byte);
                bzero((void *)(dest_addr), length);
            }
        } else {
            wl_printf(WL_PRINT_ERROR, print_type_baseband, "Trying to write to RF D buffer on a 2RF design.\n");
        }
    }

    //
    // TBD - Could rework this function to reduce the code overhead due to printing
    //

}



/*****************************************************************************/
/**
 * @brief Baseband reset
 *
 * @param   None
 *
 * @return  None
 *
 ******************************************************************************/
void baseband_reset(){
    // ------------------------------------------
    // Reset the global variables
    //
    write_iq_checksum_lsb = 0;
    write_iq_checksum_msb = 0;


    // ------------------------------------------
    // Reset the buffers core
    //
    
    // Perform any HW specific resets
    baseband_hw_specific_reset();

    // Set default config register values
    wl_bb_clear_config(
            WL_BUF_REG_CONFIG_CONT_TX |               // Disable continuous Tx
            WL_BUF_REG_CONFIG_STOP_TX |               // Stop any TX that are currently active
            WL_BUF_REG_CONFIG_AGC_IQ_SEL_RF_ALL );    // Select non-AGC IQ for all RF interfaces


    // Set RSSI clock to be 1/4 of the IQ sample clock
    wl_bb_set_rssi_clk(1);

    // Set the TX delay to INIT_TX_DELAY
    wl_bb_set_tx_delay(INIT_TX_DELAY);

    // Turn off all RX and TX buffers
    wl_bb_clear_rx_buffer_en(RF_SEL_ALL);
    wl_bb_clear_tx_buffer_en(RF_SEL_ALL);

    // Set the Buffer to RF mapping:
    //     Buffer A - RFA
    //     Buffer B - RFB
    //     Buffer C - RFC
    //     Buffer D - RFD
    wl_bb_set_rf_buffer_sel(ANT_A, ANT_B, ANT_C, ANT_D);

    // Initialize the RD/WR byte offsets in the core
    //   NOTE:  We initialize the TX write offset to the buffer size because we assume
    //     that the TX buffer will be populated by the time the transmission starts.
    wl_bb_set_rf_rx_iq_buf_rd_byte_offset(0);
    wl_bb_set_rf_rx_iq_buf_wr_byte_offset(0);

    wl_bb_set_rf_tx_iq_buf_wr_byte_offset(WARPLAB_IQ_TX_BUF_SIZE);

    // Reset the TX / RX counters
    wl_bb_clear_txrx_counter_reset();
    wl_bb_set_txrx_counter_reset(WL_BUF_TXRX_COUNTER_RESET_TXRX_ALL);
    wl_bb_clear_txrx_counter_reset();
}



/*****************************************************************************/
/**
 * @brief Accessor methods
 *
 * @param   None
 *
 * @return  u32              - Value of parameter
 *
 ******************************************************************************/
u32 wl_bb_get_supported_tx_length() {
    if (supported_tx_length != 0xFFFFFFFF) {
        return supported_tx_length;
    } else {
        wl_printf(WL_PRINT_ERROR, print_type_baseband, "TX baseband buffers not configured.\n");
        return 0;
    }
}

u32 wl_bb_get_supported_rx_length() {
    if (supported_rx_length != 0xFFFFFFFF) {
        return supported_rx_length;
    } else {
        wl_printf(WL_PRINT_ERROR, print_type_baseband, "RX baseband buffers not configured.\n");
        return 0;
    }
}



/*****************************************************************************/
/**
 * @brief Baseband subsystem initialization
 *
 * @param   dram_present          - Status flag to indicate if DRAM is present
 * @param   configure_buffers     - Flag to indicate if the buffer sizes / locations should be configured
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/
int baseband_init(u8 dram_present, u8 configure_buffers){
    u32  status         = XST_SUCCESS;

    // Configure the sample buffers (if necessary)
    if (configure_buffers) {
        baseband_buffers_config(dram_present);
    }

    // Initialize the baseband
    baseband_reset();

    // Initialize the AGC core
    warplab_agc_init();
    warplab_agc_reset();
    trigger_proc_out1_set_delay(2000);          // Configure initial delay for 12.5 us

    // Check baseband parameters
    status = baseband_check_parameters();

    return status;
}



/*****************************************************************************/
/**
 * @brief IQ Checksums
 *
 * WARPLab uses the Fletcher-32 checksum algorithm
 *
 ******************************************************************************/
u32 baseband_get_checksum() {
    return ((write_iq_checksum_msb << 16) + write_iq_checksum_lsb);
}


u32 baseband_update_checksum(u16 newdata, u8 reset ){
    // xil_printf("Checksum input: %d, reset: %d\n", newdata, reset);

    if(reset){
        write_iq_checksum_lsb = 0;
        write_iq_checksum_msb = 0;
    }

    write_iq_checksum_lsb = (write_iq_checksum_lsb + newdata) % 65535;
    write_iq_checksum_msb = (write_iq_checksum_msb + write_iq_checksum_lsb) % 65535;

    // xil_printf("sum1: %d sum2: %d\n", write_iq_checksum_lsb, write_iq_checksum_msb);

    return baseband_get_checksum();
}



/*****************************************************************************/
/**
 * @brief Get the selected buffer's buffer size
 *
 * @param   cmd_id           - Command ID is used as a proxy to get the TX / RX or RSSI buffer sizes
 * @param   buffer_sel       - Which of the buffers to get the size
 *
 * @return  u32              - Buffer Size
 *
 ******************************************************************************/
u32  get_buffer_size(u32 cmd_id, u32 buffer_sel) {

    u32 buffer_size = 0;

    switch(cmd_id) {
        case CMDID_BASEBAND_WRITE_IQ:
            if(buffer_sel & RF_SEL_A) { buffer_size = wl_iq_tx_buff_a_size; }
            if(buffer_sel & RF_SEL_B) { buffer_size = wl_iq_tx_buff_b_size; }
            if(buffer_sel & RF_SEL_C) { buffer_size = wl_iq_tx_buff_c_size; }
            if(buffer_sel & RF_SEL_D) { buffer_size = wl_iq_tx_buff_d_size; }
        break;

        case CMDID_BASEBAND_READ_IQ:
            if(buffer_sel & RF_SEL_A) { buffer_size = wl_iq_rx_buff_a_size; }
            if(buffer_sel & RF_SEL_B) { buffer_size = wl_iq_rx_buff_b_size; }
            if(buffer_sel & RF_SEL_C) { buffer_size = wl_iq_rx_buff_c_size; }
            if(buffer_sel & RF_SEL_D) { buffer_size = wl_iq_rx_buff_d_size; }
        break;

        case CMDID_BASEBAND_READ_RSSI:
            if(buffer_sel & RF_SEL_A) { buffer_size = wl_rssi_buff_a_size; }
            if(buffer_sel & RF_SEL_B) { buffer_size = wl_rssi_buff_b_size; }
            if(buffer_sel & RF_SEL_C) { buffer_size = wl_rssi_buff_c_size; }
            if(buffer_sel & RF_SEL_D) { buffer_size = wl_rssi_buff_d_size; }
        break;
    }

    return buffer_size;
}



/*****************************************************************************/
/**
 * @brief Get the selected buffer's counter
 *
 * @param   txrx_sel         - Get TX / RX buffer counter value
 *                                 TX = CMD_PARAM_BASEBAND_TXRX_COUNT_GET_TX
 *                                 RX = CMD_PARAM_BASEBAND_TXRX_COUNT_GET_RX
 * @param   buffer_sel       - Which of the buffers to get the counter value
 *
 * @return  u32              - Counter value
 *
 ******************************************************************************/
u32  get_buffer_counter(u32 txrx_sel, u32 buffer_sel) {

    u32 buffer_count = 0;

    if (txrx_sel == CMD_PARAM_BASEBAND_TXRX_COUNT_GET_RX) {
        if(buffer_sel & RF_SEL_A) { buffer_count = wl_bb_get_rfa_rx_count(); }
        if(buffer_sel & RF_SEL_B) { buffer_count = wl_bb_get_rfb_rx_count(); }
        if(buffer_sel & RF_SEL_C) { buffer_count = wl_bb_get_rfc_rx_count(); }
        if(buffer_sel & RF_SEL_D) { buffer_count = wl_bb_get_rfd_rx_count(); }
    } else {
        if(buffer_sel & RF_SEL_A) { buffer_count = wl_bb_get_rfa_tx_count(); }
        if(buffer_sel & RF_SEL_B) { buffer_count = wl_bb_get_rfb_tx_count(); }
        if(buffer_sel & RF_SEL_C) { buffer_count = wl_bb_get_rfc_tx_count(); }
        if(buffer_sel & RF_SEL_D) { buffer_count = wl_bb_get_rfd_tx_count(); }
    }

    return buffer_count;
}



/*****************************************************************************/
/**
 * @brief AGC Commands
 *
 * The WARPLab AGC is based on the 802.11 Reference design AGC.  The initialization
 * is much the same between the two, except where noted.
 *
 ******************************************************************************/
void warplab_agc_init(){
    // wl_printf(WL_PRINT_DEBUG, print_type_baseband, "AGC init\n");

    // ant_mode argument allows per-antenna AGC settings, in case FMC module has different
    //     response than on-board RF interfaces. Testing so far indicates the settings below
    //     work fine for all RF interfaces

    // Post Rx_done reset delays for [rxhp, g_rf, g_bb]
    wl_agc_set_reset_timing(4, 250, 250);

    // AGC config:
    //     RFG Thresh 3->2, 2->1, Avg_len_sel, V_DB_Adj, Init G_BB
    //       NOTE:  V_DB_Adj has a different value from the 802.11 reference design (4 vs 6)
    //              so that the agc_target command has roughly the same behavior between WARPLab
    //              7.4.0 and WARPLab 7.5.0.
    //
    wl_agc_set_config_all((256 - 56), (256 - 37), 2, 4, 24);

    //     AGC RSSI->Rx power offsets
    wl_agc_set_RSSI_pwr_calib(100, 79, 70);

    //     AGC timing: capt_rssi_1, capt_rssi_2, capt_v_db, agc_done
    wl_agc_set_AGC_timing(1, 60, 180, 192);

    //     AGC timing: start_dco, en_iir_filt
    wl_agc_set_DCO_timing(100, (100 + 34));

    //     AGC target output power (log scale)
    wl_agc_set_target((64 - 13));

    //     Set IIR coefficients
    //         IIR HPF filter with 3dB cutoff at 20kHz with 40MHz sampling
    //             DCO_IIR_Coef_A1 = -0.996863331833438  ( Fix_18_17) => -130661  => 0x0002019B
    //             DCO_IIR_Coef_B0 = 0.99843166591671906 (UFix_18_17) =>  130866  => 0x0001FF32
    //
    //         To convert to a fixed point number, we first multiple by 2^17, then round
    //             and then represent the number as a 2's complement value (18 bits).
    //
    wl_agc_set_iir_coef_a1(0x0002019B);               // This value is  Fix_18_17
    wl_agc_set_iir_coef_b0(0x0001FF32);               // This value is UFix_18_17

    // Enable the "reset per rx" mode
    wl_agc_enable_reset_per_rx();

    // Initialize the AGC rx length to a sane default (ie 100 samples more than the current baseband rx length)
    warplab_set_agc_rx_length(wl_bb_get_rx_length() + 100);

    // AGC is now reset and enabled, ready to go!

    // Override the AGC functionality
    // wl_agc_set_override(0x00008080);
}


void warplab_agc_enable_DCO(u32 enable){
    // wl_printf(WL_PRINT_DEBUG, print_type_baseband, "AGC set DCO\n");

    // Enables DCO and DCO subtraction (correction scheme and butterworth hipass are active)
    if(enable)
        wl_agc_set_DCO_timing(100, (100+34));
    else
        wl_agc_set_DCO_timing(255, 255);
}


void warplab_agc_reset(){
    // wl_printf(WL_PRINT_DEBUG, print_type_baseband, "AGC reset\n");

    // Cycle the AGC software reset port
    wl_agc_set_reset(1);
    usleep(10);
    wl_agc_set_reset(0);
    usleep(100);
}


void warplab_set_agc_rx_length(u32 num_samples) {

	// The AGC core implements a 32-bit sample counter that increments at 40MHz
	//  This counter is compared to the rx_length when the reset-per-Rx mode is enabled
	//  The AGC rx_length value must be non-zero
	if(num_samples == 0) {
		wl_agc_set_rx_length(1);
	} else {
		wl_agc_set_rx_length(num_samples);
	}

	return;
}







/**********************************************************************************************************************/
/**
 * @brief WARP v3 Specific Functions
 *
 **********************************************************************************************************************/

/***************************** Include Files *********************************/

#include <xintc.h>

/*************************** Constant Definitions ****************************/

/*********************** Global Variable Definitions *************************/

/*************************** Variable Definitions ****************************/

/*************************** Functions Prototypes ****************************/

void wl_buffers_core_rx_int_handler(void *InstancePtr);
void wl_buffers_core_tx_int_handler(void *InstancePtr);


/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * @brief Transfer Baseband Data
 *
 *     Uses CDMA to transfer baseband data.
 *
 * @param   src_addr         - Source address of the data
 * @param   dest_addr        - Destination address of the data
 * @param   length           - Length of the transfer (in bytes)
 *
 * @return  None
 *
 *****************************************************************************/
void baseband_transfer_data( u32 src_addr, u32 dest_addr, u32 length ) {
    wl_cdma_transfer(src_addr, dest_addr, length);
}



/*****************************************************************************/
/**
 * @brief Hardware Specific Baseband Reset
 *
 *     Performs any hardware specific initialization required during a baseband
 * reset.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void baseband_hw_specific_reset() {
    // Enable byte swapping
    wl_bb_set_config(WL_BUF_REG_CONFIG_RX_BYTE_ORDER | WL_BUF_REG_CONFIG_TX_BYTE_ORDER);
}



/*****************************************************************************/
/**
 * @brief Populate Temporary TX buffers
 *
 *     Copies data from the RF buffer in DDR to the temporary BRAM buffer based
 * on the buffer_sel parameter.  This method will not transfer to RFC and RFD if
 * they do not exist.
 *
 * @param   buffer_sel       - Buffer select (Indicates which buffer(s) should be copied)
 * @param   length           - Length of the transfer (in bytes)
 *
 * @return  None
 *
 *****************************************************************************/
void populate_tmp_tx_buffers(u32 buffer_sel, u32 offset, u32 length) {

    u32 src_addr;
    u32 dest_addr;

    // Only transfer if DDR is being used for buffers
    if (use_dram_for_buffers) {

        // Process RFA
        if(buffer_sel & RF_SEL_A) {
            src_addr  = (u32)(wl_iq_tx_buff_a) + offset;
            dest_addr = WARPLAB_IQ_TX_BUF_A + (offset % WARPLAB_IQ_TX_BUF_SIZE);
            wl_cdma_transfer(src_addr, dest_addr, length);
        }

        // Process RFB
        if(buffer_sel & RF_SEL_B) {
            src_addr = (u32)(wl_iq_tx_buff_b) + offset;
            dest_addr = WARPLAB_IQ_TX_BUF_B + (offset % WARPLAB_IQ_TX_BUF_SIZE);
            wl_cdma_transfer(src_addr, dest_addr, length);
        }

        // Process RFC
        if ((buffer_sel & RF_SEL_C) && (WARPLAB_CONFIG_4RF)) {
            src_addr = (u32)(wl_iq_tx_buff_c) + offset;
            dest_addr = WARPLAB_IQ_TX_BUF_C + (offset % WARPLAB_IQ_TX_BUF_SIZE);
            wl_cdma_transfer(src_addr, dest_addr, length);
        }

        // Process RFD
        if ((buffer_sel & RF_SEL_D) && (WARPLAB_CONFIG_4RF)) {
            src_addr = (u32)(wl_iq_tx_buff_d) + offset;
            dest_addr = WARPLAB_IQ_TX_BUF_D + (offset % WARPLAB_IQ_TX_BUF_SIZE);
            wl_cdma_transfer(src_addr, dest_addr, length);
        }
    }
}



/*****************************************************************************/
/**
 * @brief Set up the Baseband interrupts
 *
 * @param   intc             - Pointer to instance of interrupt controller
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *
 *****************************************************************************/
int wl_baseband_setup_interrupt(XIntc* intc){
    int status;

    // Connect buffers core RX interrupt to Interrupt Controller
    status =  XIntc_Connect(intc, WL_BUF_RX_INTERRUPT_ID, (XInterruptHandler)wl_buffers_core_rx_int_handler, NULL);
    XIntc_Enable(intc, WL_BUF_RX_INTERRUPT_ID);

    // Connect buffers core TX interrupt to Interrupt Controller
    status =  XIntc_Connect(intc, WL_BUF_TX_INTERRUPT_ID, (XInterruptHandler)wl_buffers_core_tx_int_handler, NULL);
    XIntc_Enable(intc, WL_BUF_TX_INTERRUPT_ID);

    return status;
}



/*****************************************************************************/
/**
 * @brief Buffers Core RX Interrupt Handler
 *
 * Handles interrupts that occur from the Buffers core.
 *
 * @param   InstancePtr      - Pointer to the Buffers core instance
 *
 * @return  None
 *
 ******************************************************************************/
void wl_buffers_core_rx_int_handler(void *InstancePtr){

    u32 src_addr;
    u32 dest_addr;
    u32 iq_buf_size;
    u32 rssi_buf_size;
    u32 iq_xfer_length;
    u32 rssi_xfer_length;

    u32 buff_en;
    u32 iq_read_offset;
    u32 iq_read_offset_mod_buf_size;
    u32 iq_write_offset;
    u32 iq_write_offset_mod_buf_size;
    u32 rssi_read_offset;
    u32 rssi_read_offset_mod_buf_size;

    if (use_dram_for_buffers) {
        // Check if there was a RX error
        if (wl_bb_get_rf_rx_iq_rssi_error()) {
            wl_printf(WL_PRINT_ERROR, print_type_baseband, "RX temp buffer overflowed.\n");
            return;
        }

        // Get buffers core register values
        //   NOTE:  Since we are transferring both IQ and RSSI data, we need to align the write offset
        //          To transfer the correct number of RSSI bytes.  Note that IQ data is 8x RSSI data (in bytes).
        buff_en          = wl_bb_get_rx_buffer_en();
        iq_read_offset   = wl_bb_get_rf_rx_iq_buf_rd_byte_offset();
        rssi_read_offset = (iq_read_offset >> 3);
        iq_write_offset  = (wl_bb_get_rf_rx_iq_buf_wr_byte_offset() + 4) & WL_BUF_RX_TRANSFER_BYTE_ALIGNMENT_MASK;

        // ASSUMPTION:  We are going to assume that all RX and RSSI buffers are the same size.
        //   In the future, this might not be true and the *_xfer_length calculations will need to be
        //   done per buffer.  Note that IQ data is 8x RSSI data (in bytes).
        iq_buf_size                   = (u32)(WARPLAB_IQ_RX_BUF_SIZE);
        iq_read_offset_mod_buf_size   = (u32)(iq_read_offset % iq_buf_size);
        iq_write_offset_mod_buf_size  = (u32)(iq_write_offset % iq_buf_size);

        rssi_buf_size                 = (u32)(WARPLAB_RSSI_BUF_SIZE);
        rssi_read_offset_mod_buf_size = (u32)(rssi_read_offset % rssi_buf_size);

        // Compute transfer lengths based on the buffer sizes
        if (iq_write_offset_mod_buf_size > iq_read_offset_mod_buf_size) {
            iq_xfer_length           = (u32)(iq_write_offset - iq_read_offset);
        } else {
            iq_xfer_length           = (u32)(iq_buf_size - iq_read_offset_mod_buf_size);
        }
        rssi_xfer_length             = (u32)(iq_xfer_length >> 3);

        // NOTE:  All steps below are conditioned on at least one of the buffers being enabled.  Otherwise,
        //   this function will do nothing to the state of the buffers core.

        // For each buffer, transfer relevant data
        if (buff_en & RF_SEL_A) {
            // Transfer IQ data
            src_addr    = (u32)(WARPLAB_IQ_RX_BUF_A + iq_read_offset_mod_buf_size);
            dest_addr   = (u32)(wl_iq_rx_buff_a + iq_read_offset);

            wl_cdma_transfer(src_addr, dest_addr, iq_xfer_length);

            // Transfer RSSI data (RSSI data is 8x less bytes than IQ data)
            src_addr    = (u32)(WARPLAB_RSSI_BUF_A + rssi_read_offset_mod_buf_size);
            dest_addr   = (u32)(wl_rssi_buff_a + rssi_read_offset);

            wl_cdma_transfer(src_addr, dest_addr, rssi_xfer_length);
        }

        if (buff_en & RF_SEL_B) {
            // Transfer IQ data
            src_addr    = (u32)(WARPLAB_IQ_RX_BUF_B + iq_read_offset_mod_buf_size);
            dest_addr   = (u32)(wl_iq_rx_buff_b + iq_read_offset);

            wl_cdma_transfer(src_addr, dest_addr, iq_xfer_length);

            // Transfer RSSI data (RSSI data is 8x less bytes than IQ data)
            src_addr    = (u32)(WARPLAB_RSSI_BUF_B + rssi_read_offset_mod_buf_size);
            dest_addr   = (u32)(wl_rssi_buff_b + rssi_read_offset);

            wl_cdma_transfer(src_addr, dest_addr, rssi_xfer_length);
        }

        if ((buff_en & RF_SEL_C) && (WARPLAB_CONFIG_4RF)) {
            // Transfer IQ data
            src_addr    = (u32)(WARPLAB_IQ_RX_BUF_C + iq_read_offset_mod_buf_size);
            dest_addr   = (u32)(wl_iq_rx_buff_c + iq_read_offset);

            wl_cdma_transfer(src_addr, dest_addr, iq_xfer_length);

            // Transfer RSSI data (RSSI data is 8x less bytes than IQ data)
            src_addr    = (u32)(WARPLAB_RSSI_BUF_C + rssi_read_offset_mod_buf_size);
            dest_addr   = (u32)(wl_rssi_buff_c + rssi_read_offset);

            wl_cdma_transfer(src_addr, dest_addr, rssi_xfer_length);
        }

        if ((buff_en & RF_SEL_D) && (WARPLAB_CONFIG_4RF)) {
            // Transfer IQ data
            src_addr    = (u32)(WARPLAB_IQ_RX_BUF_D + iq_read_offset_mod_buf_size);
            dest_addr   = (u32)(wl_iq_rx_buff_d + iq_read_offset);

            wl_cdma_transfer(src_addr, dest_addr, iq_xfer_length);

            // Transfer RSSI data (RSSI data is 8x less bytes than IQ data)
            src_addr    = (u32)(WARPLAB_RSSI_BUF_D + rssi_read_offset_mod_buf_size);
            dest_addr   = (u32)(wl_rssi_buff_d + rssi_read_offset);

            wl_cdma_transfer(src_addr, dest_addr, rssi_xfer_length);
        }

        // If we are done, then reset the read / write offsets.  Otherwise, update
        //     the read offset to reflect the bytes read only if at least one of
        //     the buffers is enabled.
        if (buff_en) {
            if (iq_write_offset == rx_buffer_size) {
                wl_bb_set_rf_rx_iq_buf_rd_byte_offset(0);
                wl_bb_set_rf_rx_iq_buf_wr_byte_offset(0);
            } else {
                wl_bb_set_rf_rx_iq_buf_rd_byte_offset(iq_write_offset);
            }
        }

    } else {
        // Since we should not use the interrupt since DDR is not present; reset the
        //     read / write offsets so we don't hit any weird conditions
        wl_bb_set_rf_rx_iq_buf_rd_byte_offset(0);
        wl_bb_set_rf_rx_iq_buf_wr_byte_offset(0);
    }
}



/*****************************************************************************/
/**
 * @brief Buffers Core TX Interrupt Handler
 *
 * Handles interrupts that occur from the Buffers core.
 *
 * @param   InstancePtr      - Pointer to the Buffers core instance
 *
 * @return  None
 *
 ******************************************************************************/
void wl_buffers_core_tx_int_handler(void *InstancePtr){

    u32 buff_en;
    u32 tx_iq_status;
    u32 continuous_tx;
    u32 iq_xfer_length;
    u32 iq_write_offset;

    // Only perform a transfer if the node is using DDR for buffers
    if (use_dram_for_buffers) {

        // Check if there was a RX error
        if (wl_bb_get_rf_tx_iq_error()) {
            wl_printf(WL_PRINT_ERROR, print_type_baseband, "TX temp buffer underflowed.\n");
            return;
        }

        // Get buffers core register values
        buff_en          = wl_bb_get_tx_buffer_en();
        tx_iq_status     = wl_bb_get_rf_tx_iq_status();
        continuous_tx    = ((wl_bb_get_config() & WL_BUF_REG_CONFIG_CONT_TX) == WL_BUF_REG_CONFIG_CONT_TX);

        // For the last write we need to prep the buffer for the next write IQ.  This means
        //   that we fill the temporary buffer completely.  For every other write, we need to
        //   transfer a threshold number of bytes to the buffer.
        //
        // ASSUMPTION:  We are going to assume that all TX buffers are the same size.
        //   In the future, this might not be true and the *_xfer_length calculations will need to be
        //   done per buffer.
        //
        // NOTE:  All steps below are conditioned on at least one of the buffers being enabled.  Otherwise,
        //   this function will do nothing to the state of the buffers core.
        //
        if (buff_en) {
            if (tx_iq_status & WL_BUF_TX_IQ_STATUS_WR_DONE) {
                iq_write_offset  = 0x00000000;

                // If we are in continuous TX mode, the "done" means we need to start the TX waveform over,
                // but can't over-write what we are currently transmitting (ie only write half the buffer).
                if (continuous_tx) {
                    iq_xfer_length   = (u32)(WARPLAB_IQ_TX_BUF_SIZE >> 1);
                } else {
                    iq_xfer_length   = (u32)(WARPLAB_IQ_TX_BUF_SIZE);
                }
            } else {
                iq_write_offset  = (wl_bb_get_rf_tx_iq_buf_wr_byte_offset() & WL_BUF_TX_TRANSFER_BYTE_ALIGNMENT_MASK);
                iq_xfer_length   = (u32)(WARPLAB_IQ_TX_BUF_SIZE >> 1);
            }

            // Transfer the data
            populate_tmp_tx_buffers(buff_en, iq_write_offset, iq_xfer_length);

            // Update the write_offset in the buffers core
            wl_bb_set_rf_tx_iq_buf_wr_byte_offset(iq_write_offset + iq_xfer_length);
        }

    } else {
        // Since we should not use the interrupt since DDR is not present; reset the
        //     read / write offsets so we don't hit any weird conditions
        wl_bb_set_rf_tx_iq_buf_wr_byte_offset(WARPLAB_IQ_TX_BUF_SIZE);
    }
}



/*****************************************************************************/
/**
 * @brief Configure Baseband Buffers
 *
 *   This function will configure the size of the buffers used in this sub-system
 * and configure the WARPLab Buffers core with appropriate settings based on the
 * WARP hardware that is present (ie is the DDR available).
 *
 * @param   dram_present     - Flag to indicate if DRAM is present
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/
void baseband_buffers_config(u8 dram_present){

    //
    //
    // NOTE:  For WARPLab 7.5.0, for compatibility reasons, the default tx/rx_num_samples
    //     is not tx/rx_max_samples.  For WARPLab 7.5.0, tx/rx_num_samples will be the
    //     same as WARPLab 7.4.0 (ie WL_BUF_DEFAULT_RX_NUM_SAMPLES / WL_BUF_DEFAULT_TX_NUM_SAMPLES)
    //
    //

    u32  rx_max_samples;
    u32  tx_max_samples;

    u32  tx_num_samples;
    u32  rx_num_samples;

    wl_printf(WL_PRINT_NONE, NULL, "Configuring baseband ...\n");

    // Set the number of samples and sample buffers based on if the DRAM is present
    if (dram_present) {
        wl_printf(WL_PRINT_NONE, NULL, "  Using DDR for buffers\n");

        use_dram_for_buffers = 1;

        // DDR Buffer Allocation
        //     See defines in wl_baseband.h for DDR buffer allocations
        //
        rx_max_samples       = WL_BUF_DEFAULT_RX_MAX_SAMPLES;
        tx_max_samples       = WL_BUF_DEFAULT_TX_MAX_SAMPLES;

        rx_num_samples       = WL_BUF_DEFAULT_RX_NUM_SAMPLES;
        tx_num_samples       = WL_BUF_DEFAULT_TX_NUM_SAMPLES;

        wl_bb_set_rf_rx_iq_threshold(WL_BUF_RX_TRANSFER_THRESHOLD_SAMPLES);
        wl_bb_set_rf_tx_iq_threshold(WL_BUF_TX_TRANSFER_THRESHOLD_SAMPLES);

        wl_iq_rx_buff_a      = (void *) WL_BUF_DEFAULT_IQ_RX_BUF_A_ADDR;
        wl_iq_tx_buff_a      = (void *) WL_BUF_DEFAULT_IQ_TX_BUF_A_ADDR;
        wl_rssi_buff_a       = (void *) WL_BUF_DEFAULT_RSSI_BUF_A_ADDR;
        wl_iq_rx_buff_a_size = (u32)    WL_BUF_DEFAULT_IQ_RX_BUF_A_SIZE;
        wl_iq_tx_buff_a_size = (u32)    WL_BUF_DEFAULT_IQ_TX_BUF_A_SIZE;
        wl_rssi_buff_a_size  = (u32)    WL_BUF_DEFAULT_RSSI_BUF_A_SIZE;

        wl_iq_rx_buff_b      = (void *) WL_BUF_DEFAULT_IQ_RX_BUF_B_ADDR;
        wl_iq_tx_buff_b      = (void *) WL_BUF_DEFAULT_IQ_TX_BUF_B_ADDR;
        wl_rssi_buff_b       = (void *) WL_BUF_DEFAULT_RSSI_BUF_B_ADDR;
        wl_iq_rx_buff_b_size = (u32)    WL_BUF_DEFAULT_IQ_RX_BUF_B_SIZE;
        wl_iq_tx_buff_b_size = (u32)    WL_BUF_DEFAULT_IQ_TX_BUF_B_SIZE;
        wl_rssi_buff_b_size  = (u32)    WL_BUF_DEFAULT_RSSI_BUF_B_SIZE;

        wl_iq_rx_buff_c      = (void *) WL_BUF_DEFAULT_IQ_RX_BUF_C_ADDR;
        wl_iq_tx_buff_c      = (void *) WL_BUF_DEFAULT_IQ_TX_BUF_C_ADDR;
        wl_rssi_buff_c       = (void *) WL_BUF_DEFAULT_RSSI_BUF_C_ADDR;
        wl_iq_rx_buff_c_size = (u32)    WL_BUF_DEFAULT_IQ_RX_BUF_C_SIZE;
        wl_iq_tx_buff_c_size = (u32)    WL_BUF_DEFAULT_IQ_TX_BUF_C_SIZE;
        wl_rssi_buff_c_size  = (u32)    WL_BUF_DEFAULT_RSSI_BUF_C_SIZE;

        wl_iq_rx_buff_d      = (void *) WL_BUF_DEFAULT_IQ_RX_BUF_D_ADDR;
        wl_iq_tx_buff_d      = (void *) WL_BUF_DEFAULT_IQ_TX_BUF_D_ADDR;
        wl_rssi_buff_d       = (void *) WL_BUF_DEFAULT_RSSI_BUF_D_ADDR;
        wl_iq_rx_buff_d_size = (u32)    WL_BUF_DEFAULT_IQ_RX_BUF_D_SIZE;
        wl_iq_tx_buff_d_size = (u32)    WL_BUF_DEFAULT_IQ_TX_BUF_D_SIZE;
        wl_rssi_buff_d_size  = (u32)    WL_BUF_DEFAULT_RSSI_BUF_D_SIZE;

    } else {
        wl_printf(WL_PRINT_NONE, NULL, "  Using BRAM for buffers\n");

        use_dram_for_buffers = 0;

        // BRAM Buffer Allocation
        //     See defines in wl_baseband.h for BRAM buffer allocations
        //
        rx_max_samples       = WL_BUF_DEFAULT_RX_NUM_SAMPLES;
        tx_max_samples       = WL_BUF_DEFAULT_TX_NUM_SAMPLES;

        rx_num_samples       = WL_BUF_DEFAULT_RX_NUM_SAMPLES;
        tx_num_samples       = WL_BUF_DEFAULT_TX_NUM_SAMPLES;

        // Set the threshold after the end of the buffer
        wl_bb_set_rf_rx_iq_threshold(rx_num_samples + 1);
        wl_bb_set_rf_tx_iq_threshold(tx_num_samples + 1);

        wl_iq_rx_buff_a      = (void *) WARPLAB_IQ_RX_BUF_A;
        wl_iq_tx_buff_a      = (void *) WARPLAB_IQ_TX_BUF_A;
        wl_rssi_buff_a       = (void *) WARPLAB_RSSI_BUF_A;
        wl_iq_rx_buff_a_size = (u32)    WARPLAB_IQ_RX_BUF_SIZE;
        wl_iq_tx_buff_a_size = (u32)    WARPLAB_IQ_TX_BUF_SIZE;
        wl_rssi_buff_a_size  = (u32)    WARPLAB_RSSI_BUF_SIZE;

        wl_iq_rx_buff_b      = (void *) WARPLAB_IQ_RX_BUF_B;
        wl_iq_tx_buff_b      = (void *) WARPLAB_IQ_TX_BUF_B;
        wl_rssi_buff_b       = (void *) WARPLAB_RSSI_BUF_B;
        wl_iq_rx_buff_b_size = (u32)    WARPLAB_IQ_RX_BUF_SIZE;
        wl_iq_tx_buff_b_size = (u32)    WARPLAB_IQ_TX_BUF_SIZE;
        wl_rssi_buff_b_size  = (u32)    WARPLAB_RSSI_BUF_SIZE;

        wl_iq_rx_buff_c      = (void *) WARPLAB_IQ_RX_BUF_C;
        wl_iq_tx_buff_c      = (void *) WARPLAB_IQ_TX_BUF_C;
        wl_rssi_buff_c       = (void *) WARPLAB_RSSI_BUF_C;
        wl_iq_rx_buff_c_size = (u32)    WARPLAB_IQ_RX_BUF_SIZE;
        wl_iq_tx_buff_c_size = (u32)    WARPLAB_IQ_TX_BUF_SIZE;
        wl_rssi_buff_c_size  = (u32)    WARPLAB_RSSI_BUF_SIZE;

        wl_iq_rx_buff_d      = (void *) WARPLAB_IQ_RX_BUF_D;
        wl_iq_tx_buff_d      = (void *) WARPLAB_IQ_TX_BUF_D;
        wl_rssi_buff_d       = (void *) WARPLAB_RSSI_BUF_D;
        wl_iq_rx_buff_d_size = (u32)    WARPLAB_IQ_RX_BUF_SIZE;
        wl_iq_tx_buff_d_size = (u32)    WARPLAB_IQ_TX_BUF_SIZE;
        wl_rssi_buff_d_size  = (u32)    WARPLAB_RSSI_BUF_SIZE;
    }

    wl_printf(WL_PRINT_NONE, NULL, "  Rx samples:  %10d (%10d max)\n", (rx_num_samples + 1), (rx_max_samples + 1));
    wl_printf(WL_PRINT_NONE, NULL, "  Tx samples:  %10d (%10d max)\n", (tx_num_samples + 1), (tx_max_samples + 1));

    // Set the TX/RX supported length global variables based on the maximum number of samples
    supported_tx_length = tx_max_samples;
    supported_rx_length = rx_max_samples;

    // Set the TX/RX length based on the number of samples
    //    NOTE:  This is for backward compatibility with WARPLab 7.4.0
    //
    wl_bb_set_tx_length(tx_num_samples);
    wl_bb_set_rx_length(rx_num_samples);

    // Set the global variable of the RX buffer size (in bytes) based on the number of samples
    rx_buffer_size = (rx_num_samples + 1) << 2;

    // NOTE:  This can be used to generate a known data stream in the RX buffers that
    //    can be used to debug higher level SW issues with the RX buffers.
    //
    if (USE_GENERATED_RX_DATA) {
        wl_bb_set_config(WL_BUF_REG_CONFIG_COUNTER_DATA_SEL);
        wl_printf(WL_PRINT_NONE, NULL, "  Using fake counter data\n");
    }

    // NOTE:  This can be used to directly loopback the TX data to the RX buffer to
    //    debug higher level software issues with the TX buffers.  This only affects
    //    IQ data and not RSSI data (ie IQ samples that are actually received are
    //    ignored but received RSSI data is captured normally).  Also, the loopback
    //    mode will only connect RFA -> RFB and RFC -> RFD.  This means that data
    //    transmitted on RFA will be received on RFB and vice versa.  Similarly, RFC
    //    and RFD are connected but due to timing considerations, we did not allow the
    //    full crossbar of RF interfaces for loopback data.
    //
    // NOTE:  USE_TX_RX_LOOPBACK mode has higher precedence than USE_GENERATED_RX_DATA
    //
    if (USE_TX_RX_LOOPBACK) {
        wl_bb_set_config(WL_BUF_REG_CONFIG_TX_RX_LOOPBACK_SEL);
        wl_printf(WL_PRINT_NONE, NULL, "  Using TX -> RX Loopback\n");
    }

#ifdef _DEBUG_
    print_buffer_info();
#endif
}



/*****************************************************************************/
/**
 * @brief Check Baseband Parameters
 *
 * @param   None
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/
int baseband_check_parameters() {
    u32  status         = XST_SUCCESS;
    u32  bd_count;

    // Check that the number of Transmit Buffer Descriptors (TX BDs) is less that or
    // equal to 2x the number of Ethernet Header buffers.
    //
    bd_count = eth_get_num_tx_descriptors();

    if (bd_count > (2 * WL_BASEBAND_ETH_NUM_BUFFER)){
        wl_printf(WL_PRINT_ERROR, print_type_baseband, "ERROR: Not enough Ethernet Buffers to support %d TX BDs for Read IQ command!\n", bd_count);

        status = XST_FAILURE;
    }

    return status;
}


