/** @file wl_user.h
 *  @brief WARPLab Framework (User Extensions)
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
#ifndef WL_USER_H_
#define WL_USER_H_


// **********************************************************************
// Command IDs (must match the CMD_ properties in wl_user_*.m)
//
#define CMDID_USER_EEPROM_WRITE_STRING                     0x000001
#define CMDID_USER_EEPROM_READ_STRING                      0x000002



/*********************** Global Structure Definitions ************************/



/*************************** Function Prototypes *****************************/
int  user_init();

int  user_process_cmd(int socket_index, void * from, wl_cmd_resp * command, wl_cmd_resp * response);


#endif /* WL_USER_H_ */
