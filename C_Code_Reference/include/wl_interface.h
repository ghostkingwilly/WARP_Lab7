/** @file wl_interface.h
 *  @brief WARPLab Interface
 *
 *  This contains the code for WARPLab Framework.
 *
 *  @copyright Copyright 2013, Mango Communications. All rights reserved.
 *          Distributed under the WARP license  (http://warpproject.org/license)
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */


/***************************** Include Files *********************************/
#include "wl_common.h"
#include <xparameters.h>


/*************************** Constant Definitions ****************************/
#ifndef WL_IFC_H_
#define WL_IFC_H_


// ****************************************************************************
// Define Commands
//
// NOTE:  All Command IDs (CMDID_*) must be a 24 bit unique number
//

//-----------------------------------------------
// WARPLab Interface Commands
//
#define CMDID_INTERFACE_TX_EN                         0x000001
#define CMDID_INTERFACE_RX_EN                         0x000002
#define CMDID_INTERFACE_TXRX_DIS                      0x000003
#define CMDID_INTERFACE_TXRX_STATE                    0x000004
#define CMDID_INTERFACE_CHANNEL                       0x000005
#define CMDID_INTERFACE_TX_GAINS                      0x000006
#define CMDID_INTERFACE_RX_GAINS                      0x000007
#define CMDID_INTERFACE_TX_LPF_CORN_FREQ              0x000008
#define CMDID_INTERFACE_RX_LPF_CORN_FREQ              0x000009
#define CMDID_INTERFACE_RX_HPF_CORN_FREQ              0x00000A
#define CMDID_INTERFACE_RX_GAIN_CTRL_SRC              0x00000B
#define CMDID_INTERFACE_RXHP_CTRL                     0x00000C
#define CMDID_INTERFACE_RX_LPF_CORN_FREQ_FINE         0x00000D



// ****************************************************************************
// Interface Defines
//
#define ANT_A                                         0
#define ANT_B                                         1
#define ANT_C                                         2
#define ANT_D                                         3

// Defines for the radio state
#define RF_STATE_STANDBY                              0
#define RF_STATE_RX                                   1
#define RF_STATE_TX                                   2

// Define macro not implemented in radio controller driver to get the radio controller state
#define wl_get_radio_controller_state()               Xil_In32(RC_BASEADDR + RC_SLV_REG0_OFFSET)

//
#define RF_RX_ANT_A                                  (RC_CTRLREGMASK_RFA & RC_REG0_RXEN)
#define RF_RX_ANT_B                                  (RC_CTRLREGMASK_RFB & RC_REG0_RXEN)
#define RF_RX_ANT_C                                  (RC_CTRLREGMASK_RFC & RC_REG0_RXEN)
#define RF_RX_ANT_D                                  (RC_CTRLREGMASK_RFD & RC_REG0_RXEN)

#define RF_TX_ANT_A                                  (RC_CTRLREGMASK_RFA & RC_REG0_TXEN)
#define RF_TX_ANT_B                                  (RC_CTRLREGMASK_RFB & RC_REG0_TXEN)
#define RF_TX_ANT_C                                  (RC_CTRLREGMASK_RFC & RC_REG0_TXEN)
#define RF_TX_ANT_D                                  (RC_CTRLREGMASK_RFD & RC_REG0_TXEN)


/*************************** Function Prototypes *****************************/

int ifc_init();

int ifc_process_cmd(int socket_index, void * from, wl_cmd_resp * command, wl_cmd_resp * response);

#endif /* WL_IFC_H_ */
