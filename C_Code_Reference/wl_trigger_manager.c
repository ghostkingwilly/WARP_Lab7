/** @file wl_trigger_manager.c
 *  @brief WARPLab Framework (Trigger Manager)
 *
 *  This contains the code for WARPLab Framework trigger manager.
 *
 *  @copyright Copyright 2013-2015, Mango Communications. All rights reserved.
 *          Distributed under the WARP license  (http://warpproject.org/license)
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

//
// NOTE:
//     In version 1.04.a of the trigger manager, we broke the dependence of the Ethernet 
//     trigger(s) on the software trigger.  In this version of the trigger manager, each 
//     Ethernet trigger also has a bit that can be used by software to cause the trigger.
//     This was done so that the software trigger was able to be used exclusively by user
//     software (ie the reference design does not use the software trigger). 
//  

//
// NOTE:
//     The trigger manager terminology can be a bit confusing, since the term "Trigger ID"
//     gets used interchangably for a number of distinct IDs.  In order to help clarify
//     things, we will try to use the following conventions:
//
//       1) Trigger Input IDs are used to identify the trigger input that is desired.  In
//          general, we will try to refer to these as input_ids.
//
//       2) Trigger Output IDs are used to identify the trigger output that is desired.  In
//          general, we will try to refer to these as output_ids.
//
//       3) Trigger Ethernet IDs are used to qualify Ethernet triggers.  The Ethernet trigger packet
//          contains a Trigger Ethernet ID that the Trigger Manager uses to filter Ethernet trigger
//          packets.  This allows a user to cause an Ethernet trigger event in a sub-set of nodes,
//          which are sensitized to a given Trigger Ethernet ID, by specifying that Trigger Ethernet
//          ID in the Ethernet trigger packet.  In general, we will try to refer to these as
//          ethernet_ids.
//

/**********************************************************************************************************************/
/**
 * @brief Common Functions
 *
 **********************************************************************************************************************/

/***************************** Include Files *********************************/

// Xilinx / Standard library includes
#include <stdio.h>
#include <xio.h>
#include <string.h>

// WARPLab includes
#include "wl_common.h"
#include "wl_trigger_manager.h"
#include "wl_baseband.h"
#include "wl_transport.h"

// WARP UDP transport (for Ethernet packet structures / definitions)
#include <WARP_ip_udp.h>


/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/

// Trigger Test Flag
u32      trigger_test_flag;

// Trigger Ethernet ID mask for Ethernet A and B triggers
u32      active_ethernet_id_mask;

// Flag to indicate that Ethernet triggers are done via software vs hardware "sniffing"
u32      eth_A_sw_ethernet_trigger_enable;
u32      eth_B_sw_ethernet_trigger_enable;


/*************************** Function Prototypes *****************************/

u32  trigmngr_input_id_to_AND_mask(u32 input_id);
u32  trigmngr_input_id_to_OR_mask(u32 input_id);

void trigmngr_disable_all_triggers();
void trigmngr_enable_all_triggers();

void eth_trigger_init();

u32  get_eth_trigger_type(u32 eth_dev_num);
void set_eth_trigger_type(u32 type, u32 eth_dev_num);

void update_eth_trigger_control(u32 ethernet_id, u32 eth_dev_num);


/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * Process Trigger Manager Commands
 *
 * This function is part of the Ethernet processing system and will process the
 * various trigger manager related commands.
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
 ******************************************************************************/
int trigmngr_process_cmd(int socket_index, void * from, wl_cmd_resp * command, wl_cmd_resp * response) {

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
    u32                 i, j;
    u32                 eth_dev_num;
    u32                 ethernet_id;
    u32                 num_output_ids;
    u32                 output_id;
    u32                 num_input_ids;
    u32                 input_id;
    u32                 num_or_input_ids, num_and_input_ids;
    u32                 output_start, or_start, and_start;
    u32                 or_inputs, and_inputs;
    u32                 trigger_type;
    u32                 delay;
    u32                 mode;
    u32                 type;

    // Set up the response header
    resp_hdr->cmd       = cmd_hdr->cmd;
    resp_hdr->length    = 0;
    resp_hdr->num_args  = 0;

    // Get the Ethernet device number of the socket
    eth_dev_num = socket_get_eth_dev_num(socket_index);

    // Process commands
    switch(cmd_id){

        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_ADD_ETHERNET_TRIG:
            // Add the new Trigger Ethernet ID to the internal mask of all enabled
            // Trigger Ethernet IDs
            //
            // NOTE:  Trigger Ethernet IDs must be one-hot encoded
            //
            ethernet_id = Xil_Ntohl(cmd_args_32[0]);

            active_ethernet_id_mask = active_ethernet_id_mask | ethernet_id;

            update_eth_trigger_control(active_ethernet_id_mask, eth_dev_num);

            resp_args_32[resp_index++] = Xil_Htonl(active_ethernet_id_mask);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_DEL_ETHERNET_TRIG:
            // Remove the new Trigger Ethernet ID from the internal mask of all enabled
            // Trigger Ethernet IDs
            //
            // NOTE:  Trigger Ethernet IDs must be one-hot encoded
            //
            ethernet_id = Xil_Ntohl(cmd_args_32[0]);

            active_ethernet_id_mask = active_ethernet_id_mask & ~ethernet_id;

            update_eth_trigger_control(active_ethernet_id_mask, eth_dev_num);

            resp_args_32[resp_index++] = Xil_Htonl(active_ethernet_id_mask);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_CLR_ETHERNET_TRIGS:
            // Remove all Ethernet triggers
            //
            active_ethernet_id_mask = 0;

            update_eth_trigger_control(active_ethernet_id_mask, eth_dev_num);

            resp_args_32[resp_index++] = Xil_Htonl(active_ethernet_id_mask);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_HW_SW_ETHERNET_TRIG:
            // Select whether to use HW or SW for Ethernet triggers
            //
            // NOTE:  This command allows for better timing synchronization between
            //        heterogeneous WARP v3 and WARP v2 networks since HW triggers
            //        are not supported for WARP v2
            //
            trigger_type = Xil_Ntohl(cmd_args_32[0]);

            set_eth_trigger_type(trigger_type, eth_dev_num);

            resp_args_32[resp_index++] = Xil_Htonl(get_eth_trigger_type(eth_dev_num));

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_INPUT_SEL:
            // TRIG_MNGR_INPUT_SEL Packet Format:
            //   - NOTE:  All u32 parameters in cmd_args_32 are byte swapped so use Xil_Ntohl()
            //   - NOTE:  This command contains three lists of parameters:
            //                1) output ids
            //                2) "OR" input ids
            //                3) "AND" input ids
            //            Since these lists can be arbitrarily long, they are each preceded by the number
            //            of elements in each list.
            //
            //   - cmd_args_32[0]                               - Number of output ids (N)
            //   - cmd_args_32[1 to N]                          - Output ID(s)
            //   - cmd_args_32[(N + 1)]                         - Number of "OR" input ids (M)
            //   - cmd_args_32[(N + 1) to (M + N + 1)]          - "OR" input ID(s)
            //   - cmd_args_32[(M + N + 2)]                     - Number of "AND" input ids (M)
            //   - cmd_args_32[(M + N + 2) to (P + M + N + 2)]  - "AND" input ID(s)
            //
            // NOTE:  This function will clear any previous state for the outputs specified in the argument list
            //     and will not affect any outputs not specified in the argument list.
            //

            // Get start positions for the argument lists
            output_start          = 0;
            or_start              = Xil_Ntohl(cmd_args_32[output_start]) + 1;
            and_start             = or_start + Xil_Ntohl(cmd_args_32[or_start]) + 1;

            // Get the number of each type of argument
            num_output_ids        = Xil_Ntohl(cmd_args_32[0]);
            num_or_input_ids      = Xil_Ntohl(cmd_args_32[or_start]);
            num_and_input_ids     = Xil_Ntohl(cmd_args_32[and_start]);

            // xil_printf("Num Outputs = %d\n", num_output_ids);
            // xil_printf("Num OR      = %d\n", num_or_input_ids);
            // xil_printf("Num AND     = %d\n", num_and_input_ids);

            // Loop Through Outputs
            for(i = 1; i < (num_output_ids + 1); i++) {
                or_inputs  = 0;
                and_inputs = 0;
                output_id  = Xil_Ntohl(cmd_args_32[i]);

                // xil_printf("    Output: %d\n", output_id);

                // Loop Through OR input ids
                for(j = 1; j < (num_or_input_ids + 1); j++) {
                    // xil_printf("        OR Input: %d\n", Xil_Ntohl(cmd_args_32[j + or_start]));
                    or_inputs = or_inputs + trigmngr_input_id_to_OR_mask(Xil_Ntohl(cmd_args_32[j + or_start]));
                }

                // Loop Through AND input ids
                for(j = 1; j < (num_and_input_ids + 1); j++){
                    // xil_printf("      AND Input: %d\n", Xil_Ntohl(cmd_args_32[j + and_start]));
                    and_inputs = and_inputs + trigmngr_input_id_to_AND_mask(Xil_Ntohl(cmd_args_32[j + and_start]));
                }

                // Configure the output
                switch(output_id){
                    case 1:
                        trigger_proc_out0_clear_config(AND_ALL | OR_ALL);
                        trigger_proc_out0_set_config(and_inputs | or_inputs);
                    break;
                    case 2:
                        trigger_proc_out1_clear_config(AND_ALL | OR_ALL);
                        trigger_proc_out1_set_config(and_inputs | or_inputs);
                    break;
                    case 3:
                        trigger_proc_out2_clear_config(AND_ALL | OR_ALL);
                        trigger_proc_out2_set_config(and_inputs | or_inputs);
                    break;
                    case 4:
                        trigger_proc_out3_clear_config(AND_ALL | OR_ALL);
                        trigger_proc_out3_set_config(and_inputs | or_inputs);
                    break;
                    case 5:
                        trigger_proc_out4_clear_config(AND_ALL | OR_ALL);
                        trigger_proc_out4_set_config(and_inputs | or_inputs);
                    break;
                    case 6:
                        trigger_proc_out5_clear_config(AND_ALL | OR_ALL);
                        trigger_proc_out5_set_config(and_inputs | or_inputs);
                    break;
                }
            }
            // xil_printf("\n");
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_OUTPUT_DELAY:
            // TRIG_MNGR_OUTPUT_DELAY Packet Format:
            //   - NOTE:  All u32 parameters in cmd_args_32 are byte swapped so use Xil_Ntohl()
            //   - NOTE:  This command contains a list of parameters:  output ids
            //            Since this list can be arbitrarily long, it is preceded by the number
            //            of elements in the list.
            //
            //   - cmd_args_32[0]                               - Number of output ids (N)
            //   - cmd_args_32[1 to N]                          - Output ID(s)
            //   - cmd_args_32[(N + 1)]                         - Delay value
            //
            // NOTE:  This function will clear any previous delay state for the output ids
            //        specified in the argument list.
            //
            num_output_ids   = Xil_Ntohl(cmd_args_32[0]);
            delay            = Xil_Ntohl(cmd_args_32[(num_output_ids + 1)]);

            // Loop Through Outputs
            for(i = 1; i < (num_output_ids + 1); i++){
                output_id = Xil_Ntohl(cmd_args_32[i]);

                switch(output_id){
                    case 1:  trigger_proc_out0_set_delay(delay);  break;
                    case 2:  trigger_proc_out1_set_delay(delay);  break;
                    case 3:  trigger_proc_out2_set_delay(delay);  break;
                    case 4:  trigger_proc_out3_set_delay(delay);  break;
                    case 5:  trigger_proc_out4_set_delay(delay);  break;
                    case 6:  trigger_proc_out5_set_delay(delay);  break;
                }
            }
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_OUTPUT_HOLD:
            // TRIG_MNGR_OUTPUT_HOLD Packet Format:
            //   - NOTE:  All u32 parameters in cmd_args_32 are byte swapped so use Xil_Ntohl()
            //   - NOTE:  This command contains a list of parameters:  output ids
            //            Since this list can be arbitrarily long, it is preceded by the number
            //            of elements in the list.
            //
            //   - cmd_args_32[0]                               - Number of output ids (N)
            //   - cmd_args_32[1 to N]                          - Output ID(s)
            //   - cmd_args_32[(N + 1)]                         - Hold mode value
            //
            // NOTE:  This function will clear any previous hold mode state for the output ids
            //        specified in the argument list.
            //
            num_output_ids   = Xil_Ntohl(cmd_args_32[0]);
            mode             = Xil_Ntohl(cmd_args_32[(num_output_ids + 1)]);

            // Loop Through Outputs
            for(i = 1; i < (num_output_ids + 1); i++){
                output_id = Xil_Ntohl(cmd_args_32[i]);

                switch(output_id){
                    case 1:  trigger_proc_out0_set_hold_mode(mode);  break;
                    case 2:  trigger_proc_out1_set_hold_mode(mode);  break;
                    case 3:  trigger_proc_out2_set_hold_mode(mode);  break;
                    case 4:  trigger_proc_out3_set_hold_mode(mode);  break;
                    case 5:  trigger_proc_out4_set_hold_mode(mode);  break;
                    case 6:  trigger_proc_out5_set_hold_mode(mode);  break;
                }
            }
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_OUTPUT_READ:
            // TRIG_MNGR_OUTPUT_READ Packet Format:
            //   - NOTE:  All u32 parameters in cmd_args_32 are byte swapped so use Xil_Ntohl()
            //   - NOTE:  This command contains a list of parameters:  output ids
            //            Since this list can be arbitrarily long, it is preceded by the number
            //            of elements in the list.
            //
            //   - cmd_args_32[0]                               - Number of output ids (N)
            //   - cmd_args_32[1 to N]                          - Output ID(s)
            //
            // NOTE:  This function will return a list of output values for the specified output ids
            //        in the same order as they are listed in the argument list.
            //
            num_output_ids = Xil_Ntohl(cmd_args_32[0]);

            // Loop Through Outputs
            for(i = 1; i < (num_output_ids + 1); i++){
                output_id = Xil_Ntohl(cmd_args_32[i]);

                switch(output_id){
                    case 1:  resp_args_32[resp_index++] = Xil_Htonl((trigger_proc_get_output_values() & OUT0) > 0);  break;
                    case 2:  resp_args_32[resp_index++] = Xil_Htonl((trigger_proc_get_output_values() & OUT1) > 0);  break;
                    case 3:  resp_args_32[resp_index++] = Xil_Htonl((trigger_proc_get_output_values() & OUT2) > 0);  break;
                    case 4:  resp_args_32[resp_index++] = Xil_Htonl((trigger_proc_get_output_values() & OUT3) > 0);  break;
                    case 5:  resp_args_32[resp_index++] = Xil_Htonl((trigger_proc_get_output_values() & OUT4) > 0);  break;
                    case 6:  resp_args_32[resp_index++] = Xil_Htonl((trigger_proc_get_output_values() & OUT5) > 0);  break;
                }
            }

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_OUTPUT_CLEAR:
            // TRIG_MNGR_OUTPUT_CLEAR Packet Format:
            //   - NOTE:  All u32 parameters in cmd_args_32 are byte swapped so use Xil_Ntohl()
            //   - NOTE:  This command contains a list of parameters:  output ids
            //            Since this list can be arbitrarily long, it is preceded by the number
            //            of elements in the list.
            //
            //   - cmd_args_32[0]                               - Number of output ids (N)
            //   - cmd_args_32[1 to N]                          - Output ID(s)
            //
            num_output_ids = Xil_Ntohl(cmd_args_32[0]);

            // Loop Through Outputs
            for(i = 1; i < (num_output_ids + 1); i++){
                output_id = Xil_Ntohl(cmd_args_32[i]);

                // NOTE:  The Hold Mode register is active-low.
                // NOTE:  If not in Hold Mode, the output is already clear
                switch(output_id){
                    case 1:
                        if(trigger_proc_out0_get_hold_mode() == OUT_HOLD_MODE_ENABLE){
                            trigger_proc_out0_set_hold_mode(OUT_HOLD_MODE_DISABLE);
                            trigger_proc_out0_set_hold_mode(OUT_HOLD_MODE_ENABLE);
                        }
                    break;
                    case 2:
                        if(trigger_proc_out1_get_hold_mode() == OUT_HOLD_MODE_ENABLE){
                            trigger_proc_out1_set_hold_mode(OUT_HOLD_MODE_DISABLE);
                            trigger_proc_out1_set_hold_mode(OUT_HOLD_MODE_ENABLE);
                        }
                    break;
                    case 3:
                        if(trigger_proc_out2_get_hold_mode() == OUT_HOLD_MODE_ENABLE){
                            trigger_proc_out2_set_hold_mode(OUT_HOLD_MODE_DISABLE);
                            trigger_proc_out2_set_hold_mode(OUT_HOLD_MODE_ENABLE);
                        }
                    break;
                    case 4:
                        if(trigger_proc_out3_get_hold_mode() == OUT_HOLD_MODE_ENABLE){
                            trigger_proc_out3_set_hold_mode(OUT_HOLD_MODE_DISABLE);
                            trigger_proc_out3_set_hold_mode(OUT_HOLD_MODE_ENABLE);
                        }
                    break;
                    case 5:
                        if(trigger_proc_out4_get_hold_mode() == OUT_HOLD_MODE_ENABLE){
                            trigger_proc_out4_set_hold_mode(OUT_HOLD_MODE_DISABLE);
                            trigger_proc_out4_set_hold_mode(OUT_HOLD_MODE_ENABLE);
                        }
                    break;
                    case 6:
                        if(trigger_proc_out5_get_hold_mode() == OUT_HOLD_MODE_ENABLE){
                            trigger_proc_out5_set_hold_mode(OUT_HOLD_MODE_DISABLE);
                            trigger_proc_out5_set_hold_mode(OUT_HOLD_MODE_ENABLE);
                        }
                    break;
                }
            }
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_INPUT_ENABLE:
        	// NOT SUPPORTED
        	xil_printf("TRIG_MNGR_INPUT_ENABLE is not supported\n");
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_INPUT_DEBOUNCE:
            // TRIG_MNGR_INPUT_DEBOUNCE Packet Format:
            //   - NOTE:  All u32 parameters in cmd_args_32 are byte swapped so use Xil_Ntohl()
            //   - NOTE:  This command contains a list of parameters:  input ids
            //            Since this list can be arbitrarily long, it is preceded by the number
            //            of elements in the list.
            //
            //   - cmd_args_32[0]                               - Number of input ids (N)
            //   - cmd_args_32[1 to N]                          - Input ID(s)
            //   - cmd_args_32[(N + 1)]                         - Debounce mode
            //
            num_input_ids    = Xil_Ntohl(cmd_args_32[0]);
            mode             = Xil_Ntohl(cmd_args_32[(num_input_ids + 1)]);

            // Loop Through Inputs
            for(i = 1; i < (num_input_ids + 1); i++){
                input_id = Xil_Ntohl(cmd_args_32[i]);

                switch(input_id){
                    case 5:  trigger_proc_in_ext_P0_debounce_mode(mode);  break;
                    case 6:  trigger_proc_in_ext_P1_debounce_mode(mode);  break;
                    case 7:  trigger_proc_in_ext_P2_debounce_mode(mode);  break;
                    case 8:  trigger_proc_in_ext_P3_debounce_mode(mode);  break;

                    // NOTE:  The following cases are invalid:
                    //   case 1:  // Invalid -- Ethernet A input has no debounce circuit
                    //   case 2:  // Invalid -- Energy detection input has no debounce circuit
                    //   case 3:  // Invalid -- AGC done input has no debounce circuit
                    //   case 4:  // Invalid -- Software trigger input has no debounce circuit
                    //   case 9:  // Invalid -- Ethernet B input has no debounce circuit
                    //
                }
            }
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_INPUT_DELAY:
            // TRIG_MNGR_INPUT_DELAY Packet Format:
            //   - NOTE:  All u32 parameters in cmd_args_32 are byte swapped so use Xil_Ntohl()
            //   - NOTE:  This command contains a list of parameters:  input ids
            //            Since this list can be arbitrarily long, it is preceded by the number
            //            of elements in the list.
            //
            //   - cmd_args_32[0]                               - Number of input ids (N)
            //   - cmd_args_32[1 to N]                          - Input ID(s)
            //   - cmd_args_32[(N + 1)]                         - Input delay
            //
            num_input_ids    = Xil_Ntohl(cmd_args_32[0]);
            delay            = Xil_Ntohl(cmd_args_32[(num_input_ids + 1)]);

            // Loop Through Inputs
            for(i = 1; i < (num_input_ids + 1); i++){
                input_id = Xil_Ntohl(cmd_args_32[i]);

                switch(input_id){
                    case 1:  trigger_proc_in_eth_A_set_delay(delay);    break;
                    case 3:  trigger_proc_in_agc_done_set_delay(delay);  break;
                    case 5:  trigger_proc_in_ext_P0_set_delay(delay);   break;
                    case 6:  trigger_proc_in_ext_P1_set_delay(delay);   break;
                    case 7:  trigger_proc_in_ext_P2_set_delay(delay);   break;
                    case 8:  trigger_proc_in_ext_P3_set_delay(delay);   break;
                    case 9:  trigger_proc_in_eth_B_set_delay(delay);    break;

                    // NOTE:  The following cases are invalid:
                    //   case 2:  // Invalid -- Energy input has no delay circuit
                    //   case 4:  // Invalid -- Software input has no delay circuit
                    //

                }
            }
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_IDELAY:
            // TRIG_MNGR_IDELAY Packet Format:
            //   - NOTE:  All u32 parameters in cmd_args_32 are byte swapped so use Xil_Ntohl()
            //   - NOTE:  This command contains a list of parameters:  input ids
            //            Since this list can be arbitrarily long, it is preceded by the number
            //            of elements in the list.
            //
            //   - cmd_args_32[0]                               - 'ext_pin' or 'cm_pll'
            //   - cmd_args_32[1]                               - Number of input ids (N)
            //   - cmd_args_32[2 to N]                          - Input ID(s)
            //   - cmd_args_32[(N + 2) to (N + N)]              - IDELAY value of corresponding input ID (UFix_5_0)
            //
            // NOTE:  The following Input IDs are invalid:
            //   - 1 - Invalid -- Ethernet A input has no IDELAY circuit
            //   - 2 - Invalid -- Energy detection input has no IDELAY circuit
            //   - 3 - Invalid -- AGC done input has no IDELAY circuit
            //   - 4 - Invalid -- Software trigger input has no IDELAY circuit
            //   - 9 - Invalid -- Ethernet B input has no IDELAY circuit
            //
            type             = Xil_Ntohl(cmd_args_32[0]);
            num_input_ids    = Xil_Ntohl(cmd_args_32[1]);

            // Loop Through Inputs
            if (type == IO_DELAY_TYPE_PIN) {
                for (i = 2; i < (num_input_ids + 2); i++) {
                    input_id = Xil_Ntohl(cmd_args_32[i]);
                    delay    = (Xil_Ntohl(cmd_args_32[(num_input_ids + i)]) & IO_DELAY_MASK);

                    switch (input_id) {
                        case 5: trigger_proc_in_ext_P0_set_idelay_pin(delay); break;
                        case 6: trigger_proc_in_ext_P1_set_idelay_pin(delay); break;
                        case 7: trigger_proc_in_ext_P2_set_idelay_pin(delay); break;
                        case 8: trigger_proc_in_ext_P3_set_idelay_pin(delay); break;
                    }
                }
            } else {
                for (i = 2; i < (num_input_ids + 2); i++) {
                    input_id = Xil_Ntohl(cmd_args_32[i]);
                    delay    = (Xil_Ntohl(cmd_args_32[(num_input_ids + i)]) & IO_DELAY_MASK);

                    switch (input_id) {
                        case 5: trigger_proc_in_ext_P0_set_idelay_cm_pll(delay); break;
                        case 6: trigger_proc_in_ext_P1_set_idelay_cm_pll(delay); break;
                        case 7: trigger_proc_in_ext_P2_set_idelay_cm_pll(delay); break;
                        case 8: trigger_proc_in_ext_P3_set_idelay_cm_pll(delay); break;
                    }
                }
            }

            // Toggle IDELAY set bit
            trigger_proc_idelay_update_clear();
            trigger_proc_idelay_update_set();
            trigger_proc_idelay_update_clear();
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_ODELAY:
            // TRIG_MNGR_ODELAY Packet Format:
            //   - NOTE:  All u32 parameters in cmd_args_32 are byte swapped so use Xil_Ntohl()
            //   - NOTE:  This command contains a list of parameters:  input ids
            //            Since this list can be arbitrarily long, it is preceded by the number
            //            of elements in the list.
            //
            //   - cmd_args_32[1]                               - 'ext_pin' or 'cm_pll'
            //   - cmd_args_32[0]                               - Number of output ids (N)
            //   - cmd_args_32[2 to N]                          - Output ID(s)
            //   - cmd_args_32[(N + 2) to (N + N)]              - ODELAY value of corresponding output ID (UFix_5_0)
            //
            // NOTE:  The following Output IDs are invalid:
            //   - 1 - Invalid -- Baseband output has no ODELAY circuit
            //   - 2 - Invalid -- AGC output has no ODELAY circuit
            //
            type             = Xil_Ntohl(cmd_args_32[0]);
            num_output_ids   = Xil_Ntohl(cmd_args_32[1]);

            // Loop Through Inputs
            if (type == IO_DELAY_TYPE_PIN) {
                for (i = 2; i < (num_output_ids + 2); i++) {
                    output_id = Xil_Ntohl(cmd_args_32[i]);
                    delay     = (Xil_Ntohl(cmd_args_32[(num_output_ids + i)]) & IO_DELAY_MASK);

                    switch (output_id) {
                        case 3: trigger_proc_in_ext_P0_set_odelay_pin(delay); break;
                        case 4: trigger_proc_in_ext_P1_set_odelay_pin(delay); break;
                        case 5: trigger_proc_in_ext_P2_set_odelay_pin(delay); break;
                        case 6: trigger_proc_in_ext_P3_set_odelay_pin(delay); break;
                    }
                }
            } else {
                for (i = 2; i < (num_output_ids + 2); i++) {
                    output_id = Xil_Ntohl(cmd_args_32[i]);
                    delay     = (Xil_Ntohl(cmd_args_32[(num_output_ids + i)]) & IO_DELAY_MASK);

                    switch (output_id) {
                        case 3: trigger_proc_in_ext_P0_set_odelay_cm_pll(delay); break;
                        case 4: trigger_proc_in_ext_P1_set_odelay_cm_pll(delay); break;
                        case 5: trigger_proc_in_ext_P2_set_odelay_cm_pll(delay); break;
                        case 6: trigger_proc_in_ext_P3_set_odelay_cm_pll(delay); break;
                    }
                }
            }

            // Toggle IDELAY set bit
            trigger_proc_odelay_update_clear();
            trigger_proc_odelay_update_set();
            trigger_proc_odelay_update_clear();
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_ENERGY_BUSY_THRESHOLD:
            wl_packet_detect_set_busy_threshold(Xil_Ntohl(cmd_args_32[0]));
            wl_packet_detect_set_idle_threshold(0xFFFF);                      // Set to make sure the idle condition is always met.
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_ENERGY_RSSI_AVG_LEN:
            wl_packet_detect_set_RSSI_duration(Xil_Ntohl(cmd_args_32[0]));
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_ENERGY_BUSY_MIN_LEN:
            wl_packet_detect_set_busy_duration(Xil_Ntohl(cmd_args_32[0]));
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_ENERGY_IFC_SEL:
            wl_packet_detect_clear_config(WL_PACKET_DETECT_CONFIG_REG_MASK_ALL);
            wl_packet_detect_set_config(IFC_TO_PACKET_DETECT_MASK(Xil_Ntohl(cmd_args_32[0])));
        break;


        //---------------------------------------------------------------------
        case CMDID_TRIG_MNGR_TEST_TRIGGER:
            // TRIG_MNGR_TEST_TRIGGER Packet Format:
            //   - NOTE:  All u32 parameters in cmd_args_32 are byte swapped so use Xil_Ntohl()
            //
            //   - cmd_args_32[0]                               - Trigger Test Flag (optional)
            //
            // NOTE:  Based on the number of arguments, the command will either set the trigger
            //     test flag or return and reset the trigger test flag.
            //
            if(cmd_hdr->num_args == 1){
                trigger_test_flag =  Xil_Ntohl(cmd_args_32[0]);

            // Requesting status of test trigger reception
            } else {
                resp_args_32[resp_index++] = Xil_Htonl(trigger_test_flag);

                resp_hdr->length  += (resp_index * sizeof(resp_args_32));
                resp_hdr->num_args = resp_index;

                trigger_test_flag  = 0;
            }
        break;

        //---------------------------------------------------------------------
        default:
            wl_printf(WL_PRINT_ERROR, print_type_trigger, "Unknown trigger manager command: 0x%x\n", cmd_id);
        break;
    }

    return resp_sent;
}



/*****************************************************************************/
/**
 * Trigger Manager Mask Functions
 *
 * These functions convert trigger input ids to mask values to be used in the trigger
 * manager.
 *
 * @param   input_id         - ID of the input to convert
 *
 * @return  u32              - Mask value of input ID
 *
 *****************************************************************************/
u32 trigmngr_input_id_to_AND_mask(u32 input_id){
    u32 mask = 0;

    if (input_id <= NUM_INPUT_TRIGGERS) {
        mask = 1 << ((input_id - 1) + AND_OFFSET_BITS);
    }

    return mask;
}



u32 trigmngr_input_id_to_OR_mask(u32 input_id){
    u32 mask = 0;

    if (input_id <= NUM_INPUT_TRIGGERS) {
        mask = 1 << ((input_id - 1) + OR_OFFSET_BITS);
    }

    return mask;
}



/*****************************************************************************/
/**
 * Trigger Processing
 *
 * This method is called when a trigger is received.
 *
 * @param   ethernet_id      - ID contained within the trigger Ethernet packet
 * @param   eth_dev_num      - Ethernet device on which trigger was received
 *
 * @return  None
 *
 *****************************************************************************/
void trigmngr_trigger_in(u32 ethernet_id, u32 eth_dev_num){

    // wl_printf(WL_PRINT_DEBUG, print_type_trigger, "Trigger received: 0x%08x\n", trig_id);

    // If we are not 'sniffing' Ethernet packets, then use the software Ethernet trigger
    if (ethernet_id & active_ethernet_id_mask){
        switch (eth_dev_num) {
            case WL_ETH_A:
                if (eth_A_sw_ethernet_trigger_enable) {
                    trigger_proc_in_eth_A_raise_trigger();
                    trigger_proc_in_eth_A_lower_trigger();
                }
            break;
            case WL_ETH_B:
                if (eth_B_sw_ethernet_trigger_enable) {
                    trigger_proc_in_eth_B_raise_trigger();
                    trigger_proc_in_eth_B_lower_trigger();
                }
            break;
        }
    }
}



/*****************************************************************************/
/**
 * Trigger Manager disable all triggers
 *
 * This method disables all triggers by setting the reset bit on all triggers
 *
 * @param    None
 *
 * @return   None
 *
 *****************************************************************************/
void trigmngr_disable_all_triggers() {
    trigger_proc_in_eth_A_trig_disable();
    trigger_proc_in_energy_trig_disable();
    trigger_proc_in_agc_done_trig_disable();
    trigger_proc_in_software_trig_disable();
    trigger_proc_in_ext_P0_trig_disable();
    trigger_proc_in_ext_P1_trig_disable();
    trigger_proc_in_ext_P2_trig_disable();
    trigger_proc_in_ext_P3_trig_disable();
    trigger_proc_in_eth_B_trig_disable();
}



/*****************************************************************************/
/**
 * Trigger Manager enable all triggers
 *
 * This method enables all triggers by clearing the reset bit on all triggers
 *
 * @param    None
 *
 * @return   None
 *
 *****************************************************************************/
void trigmngr_enable_all_triggers() {
    trigger_proc_in_eth_A_trig_enable();
    trigger_proc_in_energy_trig_enable();
    trigger_proc_in_agc_done_trig_enable();
    trigger_proc_in_software_trig_enable();
    trigger_proc_in_ext_P0_trig_enable();
    trigger_proc_in_ext_P1_trig_enable();
    trigger_proc_in_ext_P2_trig_enable();
    trigger_proc_in_ext_P3_trig_enable();
    trigger_proc_in_eth_B_trig_enable();
}



/*****************************************************************************/
/**
 * @brief Trigger Manager subsystem initialization
 *
 * Initializes the trigger manager subsystem
 *
 * @param   None
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS  - Command successful
 *                                 XST_FAILURE  - Command not successful
 *
 ******************************************************************************/
int trigmngr_init(){
    int status = XST_SUCCESS;

    // Initialize the global variables
    active_ethernet_id_mask  = 0;
    trigger_test_flag        = 0;

    // Initialize Trigger Processor

    // Set all reset bits (disable all triggers)
    trigmngr_disable_all_triggers();

    // Set all trigger delays to zero
    trigger_proc_in_eth_A_set_delay(0);
    trigger_proc_in_agc_done_set_delay(0);
    trigger_proc_in_ext_P0_set_delay(0);
    trigger_proc_in_ext_P1_set_delay(0);
    trigger_proc_in_ext_P2_set_delay(0);
    trigger_proc_in_ext_P3_set_delay(0);
    trigger_proc_in_eth_B_set_delay(0);

    trigger_proc_out0_set_delay(0);
    trigger_proc_out1_set_delay(0);
    trigger_proc_out2_set_delay(0);
    trigger_proc_out3_set_delay(0);
    trigger_proc_out4_set_delay(0);
    trigger_proc_out5_set_delay(0);
    
    // Set the debounce mode on all external trigger inputs
    trigger_proc_in_ext_P0_debounce_mode(1);
    trigger_proc_in_ext_P1_debounce_mode(1);
    trigger_proc_in_ext_P2_debounce_mode(1);
    trigger_proc_in_ext_P3_debounce_mode(1);

    // De-assert all software trigger bits
    trigger_proc_in_eth_A_lower_trigger();
    trigger_proc_in_software_lower_trigger();
    trigger_proc_in_eth_B_lower_trigger();

    // Clear all connections to output triggers
    trigger_proc_out0_clear_config(AND_ALL | OR_ALL);
    trigger_proc_out1_clear_config(AND_ALL | OR_ALL);
    trigger_proc_out2_clear_config(AND_ALL | OR_ALL);
    trigger_proc_out3_clear_config(AND_ALL | OR_ALL);
    trigger_proc_out4_clear_config(AND_ALL | OR_ALL);
    trigger_proc_out5_clear_config(AND_ALL | OR_ALL);

    // Set hold mode to disabled (ie disable "sticky" bits)
    trigger_proc_out0_set_hold_mode(1);
    trigger_proc_out1_set_hold_mode(1);
    trigger_proc_out2_set_hold_mode(1);
    trigger_proc_out3_set_hold_mode(1);
    trigger_proc_out4_set_hold_mode(1);
    trigger_proc_out5_set_hold_mode(1);

    // Set defaults for the Energy Trigger
    //   Disable the idle-checking functionality of the trigger manager core
    //       - Even a 1 cycle drop below the idle threshold will count as idle
    wl_packet_detect_set_idle_duration(10);

    wl_packet_detect_set_config(WL_PACKET_DETECT_CONFIG_REG_RESET);
    wl_packet_detect_clear_config(WL_PACKET_DETECT_CONFIG_REG_RESET);
    
    // Initialize the Ethernet triggers
    eth_trigger_init();

    // Configure Trigger Processor
    //   Default configuration:
    //       Inputs:
    //           Ethernet A       - Disabled
    //           Energy           - Enabled
    //           AGC Done         - Enabled
    //           Software         - Enabled
    //           Debug Input 0    - Enabled
    //           Debug Input 1    - Enabled
    //           Debug Input 2    - Enabled
    //           Debug Input 3    - Enabled
    //           Ethernet B       - Disabled
    //       Outputs:
    //           Baseband         - [ETH_A, ETH_B]
    //           ACG              - [ETH_A, ETH_B]
    //           Debug Output 0   - []
    //           Debug Output 1   - []
    //           Debug Output 2   - []
    //           Debug Output 3   - []
    //
    trigger_proc_out0_set_config(OR_ETH_A | OR_ETH_B);   // Out0: warplab_buffers baseband
    trigger_proc_out1_set_config(OR_ETH_A | OR_ETH_B);   // Out1: AGC start

    // Enable all triggers
    trigmngr_enable_all_triggers();

    // Disable the Ethernet triggers
    trigger_proc_in_eth_A_trig_disable();
    trigger_proc_in_eth_B_trig_disable();

    return status;
}





/**********************************************************************************************************************/
/**
 * @brief WARP v3 Specific Functions
 *
 **********************************************************************************************************************/

/***************************** Include Files *********************************/

/*************************** Constant Definitions ****************************/

/*********************** Global Variable Definitions *************************/

/*************************** Variable Definitions ****************************/

/*************************** Functions Prototypes ****************************/

void wl_pkt_proc_set_ethernet_id(u32 ethernet_id, u32 eth_dev_num);


/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * Ethernet Trigger Control
 *
 * These methods will control the the Ethernet triggers.
 *
 *****************************************************************************/
void eth_trigger_init() {
    // By default, WARP v3 uses HW triggers for both Eth A and Eth B
    set_eth_trigger_type(ETH_TRIG_HW, WL_ETH_A);
    set_eth_trigger_type(ETH_TRIG_HW, WL_ETH_B);
}


u32  get_eth_trigger_type(u32 eth_dev_num) {
    u32 ret_val = ETH_TRIG_INVALID;

    switch(eth_dev_num) {
        case WL_ETH_A:
            ret_val = eth_A_sw_ethernet_trigger_enable;
        break;
        case WL_ETH_B:
            ret_val = eth_B_sw_ethernet_trigger_enable;
        break;
    }

    return ret_val;
}


void set_eth_trigger_type(u32 type, u32 eth_dev_num) {
    switch(eth_dev_num) {
        case WL_ETH_A:
            switch(type) {
                case ETH_TRIG_HW:
                    trigger_proc_in_eth_A_use_hw_trig();
                    eth_A_sw_ethernet_trigger_enable = 0;
                break;
                case ETH_TRIG_SW:
                    trigger_proc_in_eth_A_use_sw_trig();
                    eth_A_sw_ethernet_trigger_enable = 1;
                break;
            }
        break;
        case WL_ETH_B:
            switch(type) {
                case ETH_TRIG_HW:
                    trigger_proc_in_eth_B_use_hw_trig();
                    eth_B_sw_ethernet_trigger_enable = 0;
                break;
                case ETH_TRIG_SW:
                    trigger_proc_in_eth_B_use_sw_trig();
                    eth_B_sw_ethernet_trigger_enable = 1;
                break;
            }
        break;
    }
}



void update_eth_trigger_control(u32 ethernet_id, u32 eth_dev_num) {

    switch(eth_dev_num) {
        case WL_ETH_A:
            if (eth_A_sw_ethernet_trigger_enable) {
                // If we are using SW triggers, clear the fast trigger detector
                wl_pkt_proc_set_ethernet_id(0, eth_dev_num);
            } else {
                // If we are using HW triggers, update the fast trigger detector
                wl_pkt_proc_set_ethernet_id(ethernet_id, eth_dev_num);
            }
            
            // If there are no active triggers, then disable the Ethernet trigger
            if (ethernet_id) {
                trigger_proc_in_eth_A_trig_enable();
            } else {
                trigger_proc_in_eth_A_trig_disable();
            }
        break;
        case WL_ETH_B:
            if (eth_B_sw_ethernet_trigger_enable) {
                // If we are using SW triggers, clear the fast trigger detector
                wl_pkt_proc_set_ethernet_id(0, eth_dev_num);
            } else {
                // If we are using HW triggers, update the fast trigger detector
                wl_pkt_proc_set_ethernet_id(ethernet_id, eth_dev_num);
            }
            
            // If there are no active triggers, then disable the Ethernet trigger
            if (ethernet_id) {
                trigger_proc_in_eth_B_trig_enable();
            } else {
                trigger_proc_in_eth_B_trig_disable();
            }
        break;
        default:
            wl_printf(WL_PRINT_ERROR, print_type_trigger, "Trigger Manager:  Unsupported Ethernet device\n");
            return;
        break;
    }
}



/*****************************************************************************/
/**
 * Trigger Manager Ethernet Packet Processor
 *
 * This method configures the fast trigger logic in the warplab_pkt_proc core so that
 * Ethernet triggers can be directly snooped from the incoming Ethernet packet reducing
 * delay / jitter when using Ethernet triggers.
 *
 * @param   ethernet_id      - Trigger Ethernet ID to match
 *          eth_dev_num      - Indicates which Ethernet device to use
 *
 * @return  None
 *
 *****************************************************************************/
void wl_pkt_proc_set_ethernet_id(u32 ethernet_id, u32 eth_dev_num) {

    // Ethernet device
    void                   * pkt_template;
    void                   * pkt_ops;
    u32                      tmp_ip_addr;

    // Helper pointers, for interpreting various packet parts
    ethernet_header        * hdr_eth;
    ipv4_header            * hdr_ip;
    udp_header             * hdr_udp;
    wl_transport_header    * hdr_xport;
    u32                    * trig_payload;

    // Packet length:
    //     Ethernet header
    //     IP header
    //     UDP header
    //     WL_TRANSPORT header
    //     Trigger Ethernet ID
    const int pkt_match_len =  WARP_IP_UDP_HEADER_LEN + sizeof(wl_transport_header) + 4;

    // Local arrays to construct packet template and template operators
    //     The template cannot be constructed directly in the pkt_proc BRAM, since Sysgen shared memories
    //     aren't byte-addressable (they lack byte enables, so any write access writes the full word)
    u8 template0[pkt_match_len];
    u8 ops0[pkt_match_len];

    // Set up Ethernet device
    switch(eth_dev_num) {
        case WL_ETH_A:
            pkt_template = (void *)TRIG_MNGR_REG_PKT_TEMPLATE_0;
            pkt_ops      = (void *)TRIG_MNGR_REG_PKT_OPS_0;
            tmp_ip_addr  = (u32) WL_ETH_A_IP_ADDR_BASE;
        break;
        case WL_ETH_B:
            pkt_template = (void *)TRIG_MNGR_REG_PKT_TEMPLATE_1;
            pkt_ops      = (void *)TRIG_MNGR_REG_PKT_OPS_1;
            tmp_ip_addr  = (u32) WL_ETH_B_IP_ADDR_BASE;
        break;
        default:
            wl_printf(WL_PRINT_ERROR, print_type_trigger, "Trigger Manager:  Unsupported Ethernet device\n");
            return;
        break;
    }

    // Ensure the local arrays are zero (makes it safe between soft boots)
    bzero(template0, pkt_match_len);
    bzero(ops0, pkt_match_len);

    // Assign helper pointers to their respective first bytes in the blank template buffer
    //     Address offsets here match received Ethernet frames (ETH-IP-UDP-WL)
    hdr_eth      = (ethernet_header *) template0;
    hdr_ip       = (ipv4_header *)((void*)hdr_eth + ETH_HEADER_LEN);
    hdr_udp      = (udp_header *)((void*)hdr_ip + IP_HEADER_LEN_BYTES);
    hdr_xport    = (wl_transport_header *)((void*)hdr_udp + UDP_HEADER_LEN + WARP_IP_UDP_DELIM_LEN);
    trig_payload = (u32 *)((void*)hdr_xport + sizeof(wl_transport_header));

    // Configure the packet template
    hdr_eth->dest_mac_addr[0] = 0xFF;
    hdr_eth->dest_mac_addr[1] = 0xFF;
    hdr_eth->dest_mac_addr[2] = 0xFF;
    hdr_eth->dest_mac_addr[3] = 0xFF;
    hdr_eth->dest_mac_addr[4] = 0xFF;
    hdr_eth->dest_mac_addr[5] = 0xFF;
    hdr_eth->ethertype        = Xil_Htons(ETHERTYPE_IP_V4);

    hdr_ip->protocol          = IP_PROTOCOL_UDP;
    hdr_ip->src_ip_addr       = 0;                                             // Any source IP address is fine
    hdr_ip->dest_ip_addr      = Xil_Htonl((u32)(tmp_ip_addr | 0xFF));          // X.X.X.255

    hdr_udp->src_port         = 0;
    hdr_udp->dest_port        = Xil_Htons((u16)NODE_UDP_MCAST_BASE);           // WARPLab broadcast port

    hdr_xport->src_id         = Xil_Htons((u16)0);
    hdr_xport->dest_id        = Xil_Htons((u16)BROADCAST_DEST_ID);
    hdr_xport->pkt_type       = PKTTYPE_TRIGGER;

    trig_payload[0]           = Xil_Htonl(ethernet_id);

    // Zero out full template, then write the part we need
    bzero(pkt_template, 64*4);
    memcpy(pkt_template, template0, pkt_match_len);

    // If necessary, this will print the template array:
    //
    // print_array_u8(template0, pkt_match_len);
    //

    // Configure the packet template operators
    hdr_eth      = (ethernet_header *)ops0;
    hdr_ip       = (ipv4_header *)((void*)hdr_eth + ETH_HEADER_LEN);
    hdr_udp      = (udp_header *)((void*)hdr_ip + IP_HEADER_LEN_BYTES);
    hdr_xport    = (wl_transport_header *)((void*)hdr_udp + UDP_HEADER_LEN + WARP_IP_UDP_DELIM_LEN);
    trig_payload = (u32 *)((void*)hdr_xport + sizeof(wl_transport_header));

    // Dest Ethernet address must match (all 0xFF for broadcast)
    hdr_eth->dest_mac_addr[0] = U8_OP_EQ;
    hdr_eth->dest_mac_addr[1] = U8_OP_EQ;
    hdr_eth->dest_mac_addr[2] = U8_OP_EQ;
    hdr_eth->dest_mac_addr[3] = U8_OP_EQ;
    hdr_eth->dest_mac_addr[4] = U8_OP_EQ;
    hdr_eth->dest_mac_addr[5] = U8_OP_EQ;
    hdr_eth->ethertype        = U16_OP_EQ;

    // IP protocol (UDP) and dest addr (.255) must match
    hdr_ip->protocol          = U8_OP_EQ;
    hdr_ip->src_ip_addr       = U32_OP_NC;
    hdr_ip->dest_ip_addr      = U32_OP_EQ;

    // UDP src/dest ports must match
    hdr_udp->src_port         = U16_OP_NC;
    hdr_udp->dest_port        = U16_OP_EQ;

    // WARPLab transport dest ID and packet type must match
    hdr_xport->src_id         = U16_OP_NC;
    hdr_xport->dest_id        = U16_OP_EQ;
    hdr_xport->pkt_type       = U8_OP_EQ;

    // Trigger Ethernet IDs are one-hot encoded; a trigger packet will have one or more
    //    bit-OR'd IDs in its payload.
    // If any Ethernet ID matches any previously-enabled Ethernet ID, the core should assert
    trig_payload[0]           = U32_OP_AA;

    // Zero out full template, then write the part we need
    bzero(pkt_ops, 64*4);
    memcpy(pkt_ops, ops0, pkt_match_len);

    // If necessary, this will print the ops array:
    //
    // print_array_u8(ops0, pkt_match_len);
    //
}


