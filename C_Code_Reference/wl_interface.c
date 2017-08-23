/** @file wl_interface.c
 *  @brief WARPLab Framework (Interface)
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
#include <xparameters.h>
#include <xio.h>

#include <stdlib.h>
#include <stdio.h>

// Xilinx Peripheral includes
#include <radio_controller.h>

// WARPLab includes
#include "wl_common.h"
#include "wl_interface.h"
#include "wl_baseband.h"



/*************************** Constant Definitions ****************************/

/*********************** Global Variable Definitions *************************/

/*************************** Variable Definitions ****************************/

/*************************** Functions Prototypes ****************************/

/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * Process Interface Commands
 *
 * This function is part of the Ethernet processing system and will process the
 * various interface related commands.
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
int ifc_process_cmd(int socket_index, void * from, wl_cmd_resp * command, wl_cmd_resp * response) {

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
    u32                 rf_sel;
    u32                 rf_enable;
    u32                 rf_state;
    u32                 band, chan;
    u32                 bb_gain, rf_gain;
    u32                 enable;
    u8                  corn_freq;

    u32                 rxhp_val;
    u32                 rxhp_en;

    // Set up the response header
    resp_hdr->cmd       = cmd_hdr->cmd;
    resp_hdr->length    = 0;
    resp_hdr->num_args  = 0;

    // Process the command
    switch(cmd_id){

        //---------------------------------------------------------------------
        case CMDID_INTERFACE_TX_EN:
            // wl_printf(WL_PRINT_DEBUG, print_type_interface, "Tx enable\n");

            rf_sel = Xil_Ntohl(cmd_args_32[0]);
            radio_controller_TxEnable(RC_BASEADDR, rf_sel);
        break;


        //---------------------------------------------------------------------
        case CMDID_INTERFACE_RX_EN:
            // wl_printf(WL_PRINT_DEBUG, print_type_interface, "Rx enable\n");

            rf_sel = Xil_Ntohl(cmd_args_32[0]);
            radio_controller_RxEnable(RC_BASEADDR, rf_sel);
        break;


        //---------------------------------------------------------------------
        case CMDID_INTERFACE_TXRX_DIS:
            // wl_printf(WL_PRINT_DEBUG, print_type_interface, "Tx/Rx disable\n");

            rf_sel =  Xil_Ntohl(cmd_args_32[0]);
            radio_controller_TxRxDisable(RC_BASEADDR, rf_sel);
        break;


        //---------------------------------------------------------------------
        case CMDID_INTERFACE_TXRX_STATE:
            // Return the state of the TX and RX interface
            //
            // Message format:
            //     cmd_args_32[0]      Interface Select (bit selects):
            //                             - [28]     - Select RF A interface
            //                             - [29]     - Select RF B interface
            //                             - [30]     - Select RF C interface
            //                             - [31]     - Select RF D interface
            //
            // Response format:
            //     resp_args_32[0]     RF A state (optional)
            //     resp_args_32[1]     RF B state (optional)
            //     resp_args_32[2]     RF C state (optional)
            //     resp_args_32[3]     RF D state (optional)
            //
            // NOTE:  The return value of the RF state will only get added to the return
            //     arguments if the given RF interface is selected.
            //
            rf_sel   = Xil_Ntohl(cmd_args_32[0]);
            rf_state = wl_get_radio_controller_state();

            if (rf_sel & RC_RFA) {
                rf_enable = RF_STATE_STANDBY;

                if (rf_state & RF_RX_ANT_A) { rf_enable = RF_STATE_RX; }
                if (rf_state & RF_TX_ANT_A) { rf_enable = RF_STATE_TX; }

                resp_args_32[resp_index++] = Xil_Htonl(rf_enable);
            }

            if (rf_sel & RC_RFB) {
                rf_enable = RF_STATE_STANDBY;

                if (rf_state & RF_RX_ANT_B) { rf_enable = RF_STATE_RX; }
                if (rf_state & RF_TX_ANT_B) { rf_enable = RF_STATE_TX; }

                resp_args_32[resp_index++] = Xil_Htonl(rf_enable);
            }

            if (rf_sel & RC_RFC) {
                rf_enable = RF_STATE_STANDBY;

                if (rf_state & RF_RX_ANT_C) { rf_enable = RF_STATE_RX; }
                if (rf_state & RF_TX_ANT_C) { rf_enable = RF_STATE_TX; }

                resp_args_32[resp_index++] = Xil_Htonl(rf_enable);
            }

            if (rf_sel & RC_RFD) {
                rf_enable = RF_STATE_STANDBY;

                if (rf_state & RF_RX_ANT_D) { rf_enable = RF_STATE_RX; }
                if (rf_state & RF_TX_ANT_D) { rf_enable = RF_STATE_TX; }

                resp_args_32[resp_index++] = Xil_Htonl(rf_enable);
            }

            // Send response
            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        break;


        //---------------------------------------------------------------------
        case CMDID_INTERFACE_CHANNEL:
            // wl_printf(WL_PRINT_DEBUG, print_type_interface, "Set channel\n");

            rf_sel = Xil_Ntohl(cmd_args_32[0]);
            band   = Xil_Ntohl(cmd_args_32[1]);
            chan   = Xil_Ntohl(cmd_args_32[2]);

            radio_controller_setCenterFrequency(RC_BASEADDR, rf_sel, band, chan);
        break;


        //---------------------------------------------------------------------
        case CMDID_INTERFACE_TX_GAINS:
            // wl_printf(WL_PRINT_DEBUG, print_type_interface, "Set Tx gains\n");

            rf_sel  = Xil_Ntohl(cmd_args_32[0]);
            bb_gain = Xil_Ntohl(cmd_args_32[1]);
            rf_gain = Xil_Ntohl(cmd_args_32[2]);

            radio_controller_setRadioParam(RC_BASEADDR, rf_sel, RC_PARAMID_TXGAIN_BB, bb_gain);
            radio_controller_setRadioParam(RC_BASEADDR, rf_sel, RC_PARAMID_TXGAIN_RF, rf_gain);
        break;


        //---------------------------------------------------------------------
        case CMDID_INTERFACE_RX_GAINS:
            // wl_printf(WL_PRINT_DEBUG, print_type_interface, "Set Rx gains\n");

            rf_sel  = Xil_Ntohl(cmd_args_32[0]);
            rf_gain = Xil_Ntohl(cmd_args_32[1]);
            bb_gain = Xil_Ntohl(cmd_args_32[2]);

            // Get the current RXHP value
            rxhp_val = (Xil_In32(RC_BASEADDR + RC_SLV_REG0_OFFSET) & RC_REG0_RXHP);

            // Force RXHP to one
            //   - This will minimize the impact of DC level changes during RX gain changes
            //
            radio_controller_setRxHP(RC_BASEADDR, rf_sel, RC_RXHP_ON);

            radio_controller_setRadioParam(RC_BASEADDR, rf_sel, RC_PARAMID_RXGAIN_RF, rf_gain);
            radio_controller_setRadioParam(RC_BASEADDR, rf_sel, RC_PARAMID_RXGAIN_BB, bb_gain);

            // Set RXHP back to the original value
            if(rf_sel & RC_RFA) {
                rxhp_en = (rxhp_val & RC_CTRLREGMASK_RFA) ? RC_RXHP_ON : RC_RXHP_OFF;
                radio_controller_setRxHP(RC_BASEADDR, RC_RFA, rxhp_en);
            }
            if(rf_sel & RC_RFB) {
                rxhp_en = (rxhp_val & RC_CTRLREGMASK_RFB) ? RC_RXHP_ON : RC_RXHP_OFF;
                radio_controller_setRxHP(RC_BASEADDR, RC_RFB, rxhp_en);
            }
            if(rf_sel & RC_RFC) {
                rxhp_en = (rxhp_val & RC_CTRLREGMASK_RFC) ? RC_RXHP_ON : RC_RXHP_OFF;
                radio_controller_setRxHP(RC_BASEADDR, RC_RFC, rxhp_en);
            }
            if(rf_sel & RC_RFD) {
                rxhp_en = (rxhp_val & RC_CTRLREGMASK_RFD) ? RC_RXHP_ON : RC_RXHP_OFF;
                radio_controller_setRxHP(RC_BASEADDR, RC_RFD, rxhp_en);
            }
        break;


        //---------------------------------------------------------------------
        case CMDID_INTERFACE_TX_LPF_CORN_FREQ:
            // wl_printf(WL_PRINT_DEBUG, print_type_interface, "Set Tx LPF corner freq\n");

            rf_sel      = Xil_Ntohl(cmd_args_32[0]);
            corn_freq   = Xil_Ntohl(cmd_args_32[1]);

            radio_controller_setRadioParam(RC_BASEADDR, rf_sel, RC_PARAMID_TXLPF_BW, corn_freq);
        break;


        //---------------------------------------------------------------------
        case CMDID_INTERFACE_RX_LPF_CORN_FREQ:
            // wl_printf(WL_PRINT_DEBUG, print_type_interface, "Set Rx LPF corner freq\n");

            rf_sel      = Xil_Ntohl(cmd_args_32[0]);
            corn_freq   = Xil_Ntohl(cmd_args_32[1]);

            radio_controller_setRadioParam(RC_BASEADDR, rf_sel, RC_PARAMID_RXLPF_BW, corn_freq);
        break;

        //---------------------------------------------------------------------
        case CMDID_INTERFACE_RX_LPF_CORN_FREQ_FINE:
            // wl_printf(WL_PRINT_DEBUG, print_type_interface, "Set Rx LPF corner freq\n");

            rf_sel      = Xil_Ntohl(cmd_args_32[0]);
            corn_freq   = Xil_Ntohl(cmd_args_32[1]);

            radio_controller_setRadioParam(RC_BASEADDR, rf_sel, RC_PARAMID_RXLPF_BW_FINE, corn_freq);
        break;

        //---------------------------------------------------------------------
        case CMDID_INTERFACE_RX_HPF_CORN_FREQ:
            // wl_printf(WL_PRINT_DEBUG, print_type_interface, "Set Rx HPF corner freq\n");

            rf_sel      = Xil_Ntohl(cmd_args_32[0]);
            corn_freq   = Xil_Ntohl(cmd_args_32[1]);


            xil_printf("HPF CORN FREQ:  0x%08x    0x%08x\n", rf_sel, corn_freq);


            //
            // NOTE:  The setting of the RX HPF cutoff frequency only take effect if RXHP is 0.  Unfortunately,
            //     we are not able to read the read the RXHP value to determine if this write actually takes
            //     effect when we perform the setRadioParam function.  Therefore it is left up to the user to
            //     make sure the state of the radio controller is correct to have this command take effect.
            //

            radio_controller_setRadioParam(RC_BASEADDR, rf_sel, RC_PARAMID_RXHPF_HIGH_CUTOFF_EN, corn_freq);
        break;


        //---------------------------------------------------------------------
        case CMDID_INTERFACE_RX_GAIN_CTRL_SRC:
            // wl_printf(WL_PRINT_DEBUG, print_type_interface, "Set gain control source\n");

            rf_sel = Xil_Ntohl(cmd_args_32[0]);
            enable = Xil_Ntohl(cmd_args_32[1]) & 0x1;

            if (enable == 0){
                // Manual gain control
                //   NOTE:  AGC will always run but values will be ignored
                radio_controller_setCtrlSource(RC_BASEADDR, rf_sel, RC_REG0_RXHP_CTRLSRC, RC_CTRLSRC_REG);
                radio_controller_setRxGainSource(RC_BASEADDR, rf_sel, RC_GAINSRC_SPI);

                // De-select AGC I/Q signals for Rx buffers input and disable buffers->AGC trigger
                wl_bb_clear_config(rf_sel >> 24);
            } else {
                // Automatic gain control
                radio_controller_setCtrlSource(RC_BASEADDR, rf_sel, RC_REG0_RXHP_CTRLSRC, RC_CTRLSRC_HW);
                radio_controller_setRxGainSource(RC_BASEADDR, rf_sel, RC_GAINSRC_HW);

                // Select AGC I/Q signals for Rx buffers input and enable buffers->AGC trigger
                wl_bb_set_config(rf_sel >> 24);
            }

        break;


        //---------------------------------------------------------------------
        case CMDID_INTERFACE_RXHP_CTRL:
            // wl_printf(WL_PRINT_DEBUG, print_type_interface, "Set rxhp value\n");

            rf_sel = Xil_Ntohl(cmd_args_32[0]);
            enable = Xil_Ntohl(cmd_args_32[1]) & 0x1;

            if(enable == 0){
                // Force RXHP to zero
                radio_controller_setRxHP(RC_BASEADDR, rf_sel, RC_RXHP_OFF);
            } else {
                // Force RXHP to one
                radio_controller_setRxHP(RC_BASEADDR, rf_sel, RC_RXHP_ON);
            }

        break;


        //---------------------------------------------------------------------
        default:
            wl_printf(WL_PRINT_ERROR, print_type_interface, "Unknown command ID: %d\n", cmd_id);
        break;
    }
    return resp_sent;
}



/*****************************************************************************/
/**
 * @brief Set Radio Controller defaults
 *
 * @param  None
 *
 * @return None
 *
 ******************************************************************************/
void set_radio_controller_defaults(u32 all_rf_sel) {

    // Set Tx bandwidth to nominal mode (Radios_Tx_LPF_Corn_Freq)
    radio_controller_setRadioParam(RC_BASEADDR, all_rf_sel, RC_PARAMID_TXLPF_BW, 1);

    // Set Rx bandwidth to nominal mode (Radios_Rx_LPF_Corn_Freq)
    radio_controller_setRadioParam(RC_BASEADDR, all_rf_sel, RC_PARAMID_RXLPF_BW, 1);
    radio_controller_setRadioParam(RC_BASEADDR, all_rf_sel, RC_PARAMID_RXLPF_BW_FINE, 2);

    // Set Radios to use 30KHz cutoff on HPF
    radio_controller_setRadioParam(RC_BASEADDR, all_rf_sel, RC_PARAMID_RXHPF_HIGH_CUTOFF_EN, 1);

    // Set Tx VGA Linearity to 2 (78% current) see pg 36 of MAX2829 datasheet for more information
    radio_controller_setRadioParam(RC_BASEADDR, all_rf_sel, RC_PARAMID_TXLINEARITY_VGA, 2);

    // Disable all Tx / Rx
    radio_controller_TxRxDisable(RC_BASEADDR, all_rf_sel);

}




/**********************************************************************************************************************/
/**
 * @brief WARP v3 Specific Functions
 *
 **********************************************************************************************************************/

/***************************** Include Files *********************************/

#include <w3_ad_controller.h>

/*************************** Constant Definitions ****************************/

/*********************** Global Variable Definitions *************************/

/*************************** Variable Definitions ****************************/

/*************************** Functions Prototypes ****************************/

/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * @brief RF Interface subsystem initialization
 *
 * Initializes the RF interface subsystem
 *
 * @param  None
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/
int ifc_init(){
    // wl_printf(WL_PRINT_DEBUG, print_type_interface, "Configuring interface ...\n\n");

    int status          = XST_SUCCESS;
    u32 all_rf_sel;
    u32 all_ad_sel;

    // Set the "ALL" variables based on the RF config
    if (WARPLAB_CONFIG_4RF) {
        all_rf_sel = (RC_RFA | RC_RFB | RC_RFC | RC_RFD);
        all_ad_sel = (RFA_AD_CS | RFB_AD_CS | RFC_AD_CS | RFD_AD_CS);
    } else {
        all_rf_sel = (RC_RFA | RC_RFB);
        all_ad_sel = (RFA_AD_CS | RFB_AD_CS);
    }

    // Initialize the AD9963 ADCs/DACs
    status = ad_init(AD_BASEADDR, all_ad_sel, 2);

    if(status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_interface, "AD initialization failed with status: %d\n", status);
        wl_printf(WL_PRINT_NONE, NULL, "\n************************************************************\n");
        wl_printf(WL_PRINT_NONE, NULL, " Check that software and hardware config match\n  (this error may indicate 4-radio code on 2-radio hadrware)\n");
        wl_printf(WL_PRINT_NONE, NULL, "************************************************************\n\n");
        return XST_FAILURE;
    }

    // Initialize the radio_controller core and MAX2829 transceivers
    status = radio_controller_init(RC_BASEADDR, (all_rf_sel), 1, 1);

    if(status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_interface, "Radio controller initialization failed with status: %d\n", status);
        return XST_FAILURE;
    }

    // Update the TX delays to match the 802.11 design
    //     NOTE:  These values considerably reduces any "pop" in the transmission following a TX enable (TX_EN) command
    //     NOTE:  Default value set in radio_controller_init() is:  radio_controller_setTxDelays(RC_BASEADDR, 150, 0, 0, 250);
    //
    radio_controller_setTxDelays(RC_BASEADDR, 40, 20, 0, 250);

    // Default the Tx/Rx gain control sources to SPI
    radio_controller_setTxGainSource(RC_BASEADDR, all_rf_sel, RC_GAINSRC_SPI);
    radio_controller_setRxGainSource(RC_BASEADDR, all_rf_sel, RC_GAINSRC_SPI);

    // Apply the TxDCO correction values stored in the on-board (and FMC, if present) EEPROMs
    radio_controller_apply_TxDCO_calibration(AD_BASEADDR, EEPROM_BASEADDR, all_rf_sel);

    // Set some sane defaults for the MAX2829 Tx/Rx paths; these can be changed by WARPLab commands later
    set_radio_controller_defaults(all_rf_sel);

    return status;
}



