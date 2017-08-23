/** @file wl_node.h
 *  @brief WARPLab Framework (Node)
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

// WARPLab includes
#include "wl_common.h"



/*************************** Constant Definitions ****************************/
#ifndef WL_NODE_H_
#define WL_NODE_H_


// **********************************************************************
// Command Groups (must match the CMD_ properties in wl_node_*.m)
//
#define GROUP_NODE                                         0x00
#define GROUP_TRANSPORT                                    0x10
#define GROUP_INTERFACE                                    0x20
#define GROUP_BASEBAND                                     0x30
#define GROUP_TRIGGER_MANAGER                              0x40
#define GROUP_USER                                         0x50



// **********************************************************************
// Command IDs (must match the CMD_ properties in wl_node_*.m)
//
#define CMDID_NODE_INITIALIZE                              0x000001
#define CMDID_NODE_INFO                                    0x000002
#define CMDID_NODE_IDENTIFY                                0x000003
#define CMDID_NODE_TEMPERATURE                             0x000004
#define CMDID_NODE_CONFIG_SETUP                            0x000005
#define CMDID_NODE_CONFIG_RESET                            0x000006

#define CMDID_NODE_MEM_RW                                  0x000010



// **********************************************************************
// MISC defines
//
#define CMD_PARAM_NODE_MEM_RW_MAX_BYTES                    1400



/*********************** Global Structure Definitions ************************/



/*************************** Function Prototypes *****************************/

int  node_process_cmd(int socket_index, void * from, wl_cmd_resp * command, wl_cmd_resp * response);

void node_send_early_resp(int socket_index, void * to, wl_cmd_resp_hdr * resp_hdr, void * buffer);

// Node LED commands
void blink_node( int num_blinks, int blink_time );
void increment_green_leds_one_hot();
void increment_red_leds_one_hot();

#endif /* WL_NODE_H_ */

