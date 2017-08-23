/** @file wl_trigger_manager.h
 *  @brief WARPLab Framework (Trigger Manager)
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
#ifndef TRIGCONF_H_
#define TRIGCONF_H_


// **********************************************************************
// Command IDs (must match the CMD_ properties in wl_transport_*.m)
//
#define CMDID_TRIG_MNGR_ADD_ETHERNET_TRIG                  0x000001
#define CMDID_TRIG_MNGR_DEL_ETHERNET_TRIG                  0x000002
#define CMDID_TRIG_MNGR_CLR_ETHERNET_TRIGS                 0x000003
#define CMDID_TRIG_MNGR_HW_SW_ETHERNET_TRIG                0x000004

#define CMDID_TRIG_MNGR_INPUT_SEL                          0x000010
#define CMDID_TRIG_MNGR_OUTPUT_DELAY                       0x000011
#define CMDID_TRIG_MNGR_OUTPUT_HOLD                        0x000012
#define CMDID_TRIG_MNGR_OUTPUT_READ                        0x000013
#define CMDID_TRIG_MNGR_OUTPUT_CLEAR                       0x000014

#define CMDID_TRIG_MNGR_INPUT_ENABLE                       0x000020
#define CMDID_TRIG_MNGR_INPUT_DEBOUNCE                     0x000021
#define CMDID_TRIG_MNGR_INPUT_DELAY                        0x000022
#define CMDID_TRIG_MNGR_IDELAY                             0x000023
#define CMDID_TRIG_MNGR_ODELAY                             0x000024

#define CMDID_TRIG_MNGR_ENERGY_BUSY_THRESHOLD              0x000030
#define CMDID_TRIG_MNGR_ENERGY_RSSI_AVG_LEN                0x000031
#define CMDID_TRIG_MNGR_ENERGY_BUSY_MIN_LEN                0x000032
#define CMDID_TRIG_MNGR_ENERGY_IFC_SEL                     0x000033

#define CMDID_TRIG_MNGR_TEST_TRIGGER                       0x000080



// ----------------------------------------------------------------------------
// Misc Defines
//
#define NUM_INPUT_TRIGGERS                                 9
#define NUM_OUTPUT_TRIGGERS                                6

#define ETH_TRIG_HW                                        0
#define ETH_TRIG_SW                                        1
#define ETH_TRIG_INVALID                                   0xFFFFFFFF



// ----------------------------------------------------------------------------
// Register Name Mapping
//
#define TRIG_MNGR_REG_CORE_INFO                            XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_CORE_INFO

#define TRIG_MNGR_REG_TRIG_OUTPUT                          XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_OUT

#define TRIG_MNGR_REG_TRIG_IN_CONF_0                       XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_IN_CONF_0
#define TRIG_MNGR_REG_TRIG_IN_CONF_1                       XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_IN_CONF_1
#define TRIG_MNGR_REG_TRIG_IN_CONF_2                       XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_IN_CONF_2
#define TRIG_MNGR_REG_TRIG_IN_CONF_3                       XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_IN_CONF_3
#define TRIG_MNGR_REG_TRIG_IN_CONF_4                       XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_IN_CONF_4
#define TRIG_MNGR_REG_TRIG_IN_CONF_5                       XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_IN_CONF_5
#define TRIG_MNGR_REG_TRIG_IN_CONF_6                       XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_IN_CONF_6
#define TRIG_MNGR_REG_TRIG_IN_CONF_7                       XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_IN_CONF_7
#define TRIG_MNGR_REG_TRIG_IN_CONF_8                       XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_IN_CONF_8

#define TRIG_MNGR_REG_TRIG_OUT_0_CONF_0                    XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_OUT_0_CONF_0
#define TRIG_MNGR_REG_TRIG_OUT_0_CONF_1                    XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_OUT_0_CONF_1
#define TRIG_MNGR_REG_TRIG_OUT_1_CONF_0                    XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_OUT_1_CONF_0
#define TRIG_MNGR_REG_TRIG_OUT_1_CONF_1                    XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_OUT_1_CONF_1
#define TRIG_MNGR_REG_TRIG_OUT_2_CONF_0                    XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_OUT_2_CONF_0
#define TRIG_MNGR_REG_TRIG_OUT_2_CONF_1                    XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_OUT_2_CONF_1
#define TRIG_MNGR_REG_TRIG_OUT_3_CONF_0                    XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_OUT_3_CONF_0
#define TRIG_MNGR_REG_TRIG_OUT_3_CONF_1                    XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_OUT_3_CONF_1
#define TRIG_MNGR_REG_TRIG_OUT_4_CONF_0                    XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_OUT_4_CONF_0
#define TRIG_MNGR_REG_TRIG_OUT_4_CONF_1                    XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_OUT_4_CONF_1
#define TRIG_MNGR_REG_TRIG_OUT_5_CONF_0                    XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_OUT_5_CONF_0
#define TRIG_MNGR_REG_TRIG_OUT_5_CONF_1                    XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_OUT_5_CONF_1

#define TRIG_MNGR_REG_TRIG_IODELAYS_CONTROL                XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_IODELAYS_CONTROL
#define TRIG_MNGR_REG_TRIG_ODELAY_CFG_CMPLL                XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_ODELAY_CFG_CMPLL
#define TRIG_MNGR_REG_TRIG_ODELAY_CFG_PIN                  XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_ODELAY_CFG_DEBUG_HDR
#define TRIG_MNGR_REG_TRIG_IDELAY_CFG_CMPLL                XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_IDELAY_CFG_CMPLL
#define TRIG_MNGR_REG_TRIG_IDELAY_CFG_PIN                  XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_TRIG_IDELAY_CFG_DEBUG_HDR

#define TRIG_MNGR_REG_RSSI_PKT_DET_CONFIG                  XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_RSSI_PKT_DET_CONFIG
#define TRIG_MNGR_REG_RSSI_PKT_DET_DURATIONS               XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_RSSI_PKT_DET_DURATIONS
#define TRIG_MNGR_REG_RSSI_PKT_DET_THRESHOLDS              XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_RSSI_PKT_DET_THRESHOLDS

#define TRIG_MNGR_REG_PKT_OPS_0                            XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_PKTOPS0
#define TRIG_MNGR_REG_PKT_OPS_1                            XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_PKTOPS1
#define TRIG_MNGR_REG_PKT_TEMPLATE_0                       XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_PKTTEMPLATE0
#define TRIG_MNGR_REG_PKT_TEMPLATE_1                       XPAR_WARPLAB_TRIGGER_PROC_MEMMAP_PKTTEMPLATE1



// ----------------------------------------------------------------------------
// INPUT TRIGGER CONFIGURATION
//
//
// --------------------------------------------------------
// Configuration Register 0
//     [ 4: 0] - Input Delay
//        [29] - Use SW / HW for trigger (Ethernet triggers only)
//        [30] - Raise Trigger (Ethernet and software triggers only)
//        [30] - Debounce (External pin input triggers only)
//        [31] - Reset
//
#define INPUT_DELAY_MASK                                   0x0000001F

#define INPUT_ETH_TRIGGER_SW_HW_MASK                       0x20000000

#define INPUT_RAISE_TRIGGER_MASK                           0x40000000
#define INPUT_EXT_TRIGGER_DEBOUNCE_MASK                    0x40000000

#define INPUT_DISABLE_MASK                                 0x80000000


// --------------------------------------------------------
// Macros
//
#define trigger_proc_in_eth_A_set_delay(val)               XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_0) & (~INPUT_DELAY_MASK)) | (val & INPUT_DELAY_MASK))
#define trigger_proc_in_eth_A_get_delay()                  (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_0) & INPUT_DELAY_MASK)
#define trigger_proc_in_eth_A_use_sw_trig()                XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_0) | (INPUT_ETH_TRIGGER_SW_HW_MASK)))
#define trigger_proc_in_eth_A_use_hw_trig()                XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_0) & (~INPUT_ETH_TRIGGER_SW_HW_MASK)))
#define trigger_proc_in_eth_A_raise_trigger()              XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_0) | (INPUT_RAISE_TRIGGER_MASK)))
#define trigger_proc_in_eth_A_lower_trigger()              XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_0) & (~INPUT_RAISE_TRIGGER_MASK)))
#define trigger_proc_in_eth_A_trig_disable()               XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_0) | (INPUT_DISABLE_MASK)))
#define trigger_proc_in_eth_A_trig_enable()                XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_0) & (~INPUT_DISABLE_MASK)))

#define trigger_proc_in_energy_trig_disable()              XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_1, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_1) | (INPUT_DISABLE_MASK)))
#define trigger_proc_in_energy_trig_enable()               XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_1, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_1) & (~INPUT_DISABLE_MASK)))

#define trigger_proc_in_agc_done_set_delay(val)            XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_2, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_2) & (~INPUT_DELAY_MASK)) | (val & INPUT_DELAY_MASK))
#define trigger_proc_in_agc_done_get_delay()               (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_2) & INPUT_DELAY_MASK)
#define trigger_proc_in_agc_done_trig_disable()            XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_2, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_2) | (INPUT_DISABLE_MASK)))
#define trigger_proc_in_agc_done_trig_enable()             XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_2, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_2) & (~INPUT_DISABLE_MASK)))

#define trigger_proc_in_software_raise_trigger()           XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_3, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_3) | (INPUT_RAISE_TRIGGER_MASK)))
#define trigger_proc_in_software_lower_trigger()           XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_3, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_3) & (~INPUT_RAISE_TRIGGER_MASK)))
#define trigger_proc_in_software_trig_disable()            XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_3, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_3) | (INPUT_DISABLE_MASK)))
#define trigger_proc_in_software_trig_enable()             XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_3, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_3) & (~INPUT_DISABLE_MASK)))

#define trigger_proc_in_ext_P0_set_delay(val)              XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_4, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_4) & (~INPUT_DELAY_MASK)) | (val & INPUT_DELAY_MASK))
#define trigger_proc_in_ext_P0_get_delay()                 (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_4) & INPUT_DELAY_MASK)
#define trigger_proc_in_ext_P0_debounce_mode(val)          XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_4, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_4) & (~INPUT_EXT_TRIGGER_DEBOUNCE_MASK)) | ((val & 1) << 30))
#define trigger_proc_in_ext_P0_trig_disable()              XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_4, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_4) | (INPUT_DISABLE_MASK)))
#define trigger_proc_in_ext_P0_trig_enable()               XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_4, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_4) & (~INPUT_DISABLE_MASK)))

#define trigger_proc_in_ext_P1_set_delay(val)              XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_5, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_5) & (~INPUT_DELAY_MASK)) | (val & INPUT_DELAY_MASK))
#define trigger_proc_in_ext_P1_get_delay()                (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_5) & INPUT_DELAY_MASK)
#define trigger_proc_in_ext_P1_debounce_mode(val)          XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_5, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_5) & (~INPUT_EXT_TRIGGER_DEBOUNCE_MASK)) | ((val & 1) << 30))
#define trigger_proc_in_ext_P1_trig_disable()              XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_5, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_5) | (INPUT_DISABLE_MASK)))
#define trigger_proc_in_ext_P1_trig_enable()               XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_5, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_5) & (~INPUT_DISABLE_MASK)))

#define trigger_proc_in_ext_P2_set_delay(val)              XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_6, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_6) & (~INPUT_DELAY_MASK)) | (val & INPUT_DELAY_MASK))
#define trigger_proc_in_ext_P2_get_delay()                 (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_6) & INPUT_DELAY_MASK)
#define trigger_proc_in_ext_P2_debounce_mode(val)          XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_6, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_6) & (~INPUT_EXT_TRIGGER_DEBOUNCE_MASK)) | ((val & 1) << 30))
#define trigger_proc_in_ext_P2_trig_disable()              XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_6, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_6) | (INPUT_DISABLE_MASK)))
#define trigger_proc_in_ext_P2_trig_enable()               XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_6, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_6) & (~INPUT_DISABLE_MASK)))

#define trigger_proc_in_ext_P3_set_delay(val)              XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_7, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_7) & (~INPUT_DELAY_MASK)) | (val & INPUT_DELAY_MASK))
#define trigger_proc_in_ext_P3_get_delay()                 (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_7) & INPUT_DELAY_MASK)
#define trigger_proc_in_ext_P3_debounce_mode(val)          XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_7, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_7) & (~INPUT_EXT_TRIGGER_DEBOUNCE_MASK)) | ((val & 1) << 30))
#define trigger_proc_in_ext_P3_trig_disable()              XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_7, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_7) | (INPUT_DISABLE_MASK)))
#define trigger_proc_in_ext_P3_trig_enable()               XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_7, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_7) & (~INPUT_DISABLE_MASK)))

#define trigger_proc_in_eth_B_set_delay(val)               XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_8, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_8) & (~INPUT_DELAY_MASK)) | (val & INPUT_DELAY_MASK))
#define trigger_proc_in_eth_B_get_delay()                  (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_8) & INPUT_DELAY_MASK)
#define trigger_proc_in_eth_B_use_sw_trig()                XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_8, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_8) | (INPUT_ETH_TRIGGER_SW_HW_MASK)))
#define trigger_proc_in_eth_B_use_hw_trig()                XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_8, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_8) & (~INPUT_ETH_TRIGGER_SW_HW_MASK)))
#define trigger_proc_in_eth_B_raise_trigger()              XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_8, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_8) | (INPUT_RAISE_TRIGGER_MASK)))
#define trigger_proc_in_eth_B_lower_trigger()              XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_8, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_8) & (~INPUT_RAISE_TRIGGER_MASK)))
#define trigger_proc_in_eth_B_trig_disable()               XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_8, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_8) | (INPUT_DISABLE_MASK)))
#define trigger_proc_in_eth_B_trig_enable()                XIo_Out32(TRIG_MNGR_REG_TRIG_IN_CONF_8, (XIo_In32(TRIG_MNGR_REG_TRIG_IN_CONF_8) & (~INPUT_DISABLE_MASK)))



// ----------------------------------------------------------------------------
// IDELAY / ODELAY CONFIGURATION
//
//
#define IO_DELAY_MASK                                      0x0000001F

#define IO_DELAY_TYPE_PIN                                  0x00000000
#define IO_DELAY_TYPE_CM_PLL                               0x00000001


// --------------------------------------------------------
// IDELAY / ODELAY Config register (CM-PLL and PIN registers)
//     [ 4: 0] - External Pin 0 IDELAY / ODELAY value
//     [12: 8] - External Pin 1 IDELAY / ODELAY value
//     [20:16] - External Pin 2 IDELAY / ODELAY value
//     [28:24] - External Pin 3 IDELAY / ODELAY value
//
#define EXT_P0_IO_DELAY_MASK                               0x0000001F
#define EXT_P1_IO_DELAY_MASK                               0x00001F00
#define EXT_P2_IO_DELAY_MASK                               0x001F0000
#define EXT_P3_IO_DELAY_MASK                               0x1F000000

#define EXT_P0_IO_DELAY_BIT_SHIFT                          0
#define EXT_P1_IO_DELAY_BIT_SHIFT                          8
#define EXT_P2_IO_DELAY_BIT_SHIFT                          16
#define EXT_P3_IO_DELAY_BIT_SHIFT                          24


// --------------------------------------------------------
// IDELAY / ODELAY Control register
//         [0] - IDELAY update
//         [1] - ODELAY update
//
#define IDELAY_UPDATE_MASK                                 0x00000001
#define ODELAY_UPDATE_MASK                                 0x00000002


// --------------------------------------------------------
// Macros
//
#define trigger_proc_in_ext_P0_set_idelay_pin(val)         XIo_Out32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_PIN,   ((XIo_In32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_PIN)   & (~EXT_P0_IO_DELAY_MASK)) | ((val & IO_DELAY_MASK) << EXT_P0_IO_DELAY_BIT_SHIFT)))
#define trigger_proc_in_ext_P0_set_idelay_cm_pll(val)      XIo_Out32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_CMPLL, ((XIo_In32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_CMPLL) & (~EXT_P0_IO_DELAY_MASK)) | ((val & IO_DELAY_MASK) << EXT_P0_IO_DELAY_BIT_SHIFT)))
#define trigger_proc_in_ext_P0_set_odelay_pin(val)         XIo_Out32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_PIN,   ((XIo_In32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_PIN)   & (~EXT_P0_IO_DELAY_MASK)) | ((val & IO_DELAY_MASK) << EXT_P0_IO_DELAY_BIT_SHIFT)))
#define trigger_proc_in_ext_P0_set_odelay_cm_pll(val)      XIo_Out32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_CMPLL, ((XIo_In32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_CMPLL) & (~EXT_P0_IO_DELAY_MASK)) | ((val & IO_DELAY_MASK) << EXT_P0_IO_DELAY_BIT_SHIFT)))
#define trigger_proc_in_ext_P0_get_idelay_pin()            ((XIo_In32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_PIN)   & EXT_P0_IO_DELAY_MASK) >> EXT_P0_IO_DELAY_BIT_SHIFT)
#define trigger_proc_in_ext_P0_get_idelay_cm_pll()         ((XIo_In32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_CMPLL) & EXT_P0_IO_DELAY_MASK) >> EXT_P0_IO_DELAY_BIT_SHIFT)
#define trigger_proc_in_ext_P0_get_odelay_pin()            ((XIo_In32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_PIN)   & EXT_P0_IO_DELAY_MASK) >> EXT_P0_IO_DELAY_BIT_SHIFT)
#define trigger_proc_in_ext_P0_get_odelay_cm_pll()         ((XIo_In32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_CMPLL) & EXT_P0_IO_DELAY_MASK) >> EXT_P0_IO_DELAY_BIT_SHIFT)

#define trigger_proc_in_ext_P1_set_idelay_pin(val)         XIo_Out32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_PIN,   ((XIo_In32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_PIN)   & (~EXT_P1_IO_DELAY_MASK)) | ((val & IO_DELAY_MASK) << EXT_P1_IO_DELAY_BIT_SHIFT)))
#define trigger_proc_in_ext_P1_set_idelay_cm_pll(val)      XIo_Out32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_CMPLL, ((XIo_In32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_CMPLL) & (~EXT_P1_IO_DELAY_MASK)) | ((val & IO_DELAY_MASK) << EXT_P1_IO_DELAY_BIT_SHIFT)))
#define trigger_proc_in_ext_P1_set_odelay_pin(val)         XIo_Out32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_PIN,   ((XIo_In32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_PIN)   & (~EXT_P1_IO_DELAY_MASK)) | ((val & IO_DELAY_MASK) << EXT_P1_IO_DELAY_BIT_SHIFT)))
#define trigger_proc_in_ext_P1_set_odelay_cm_pll(val)      XIo_Out32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_CMPLL, ((XIo_In32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_CMPLL) & (~EXT_P1_IO_DELAY_MASK)) | ((val & IO_DELAY_MASK) << EXT_P1_IO_DELAY_BIT_SHIFT)))
#define trigger_proc_in_ext_P1_get_idelay_pin()            ((XIo_In32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_PIN)   & EXT_P1_IO_DELAY_MASK) >> EXT_P1_IO_DELAY_BIT_SHIFT)
#define trigger_proc_in_ext_P1_get_idelay_cm_pll()         ((XIo_In32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_CMPLL) & EXT_P1_IO_DELAY_MASK) >> EXT_P1_IO_DELAY_BIT_SHIFT)
#define trigger_proc_in_ext_P1_get_odelay_pin()            ((XIo_In32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_PIN)   & EXT_P1_IO_DELAY_MASK) >> EXT_P1_IO_DELAY_BIT_SHIFT)
#define trigger_proc_in_ext_P1_get_odelay_cm_pll()         ((XIo_In32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_CMPLL) & EXT_P1_IO_DELAY_MASK) >> EXT_P1_IO_DELAY_BIT_SHIFT)

#define trigger_proc_in_ext_P2_set_idelay_pin(val)         XIo_Out32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_PIN,   ((XIo_In32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_PIN)   & (~EXT_P2_IO_DELAY_MASK)) | ((val & IO_DELAY_MASK) << EXT_P2_IO_DELAY_BIT_SHIFT)))
#define trigger_proc_in_ext_P2_set_idelay_cm_pll(val)      XIo_Out32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_CMPLL, ((XIo_In32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_CMPLL) & (~EXT_P2_IO_DELAY_MASK)) | ((val & IO_DELAY_MASK) << EXT_P2_IO_DELAY_BIT_SHIFT)))
#define trigger_proc_in_ext_P2_set_odelay_pin(val)         XIo_Out32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_PIN,   ((XIo_In32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_PIN)   & (~EXT_P2_IO_DELAY_MASK)) | ((val & IO_DELAY_MASK) << EXT_P2_IO_DELAY_BIT_SHIFT)))
#define trigger_proc_in_ext_P2_set_odelay_cm_pll(val)      XIo_Out32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_CMPLL, ((XIo_In32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_CMPLL) & (~EXT_P2_IO_DELAY_MASK)) | ((val & IO_DELAY_MASK) << EXT_P2_IO_DELAY_BIT_SHIFT)))
#define trigger_proc_in_ext_P2_get_idelay_pin()            ((XIo_In32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_PIN)   & EXT_P2_IO_DELAY_MASK) >> EXT_P2_IO_DELAY_BIT_SHIFT)
#define trigger_proc_in_ext_P2_get_idelay_cm_pll()         ((XIo_In32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_CMPLL) & EXT_P2_IO_DELAY_MASK) >> EXT_P2_IO_DELAY_BIT_SHIFT)
#define trigger_proc_in_ext_P2_get_odelay_pin()            ((XIo_In32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_PIN)   & EXT_P2_IO_DELAY_MASK) >> EXT_P2_IO_DELAY_BIT_SHIFT)
#define trigger_proc_in_ext_P2_get_odelay_cm_pll()         ((XIo_In32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_CMPLL) & EXT_P2_IO_DELAY_MASK) >> EXT_P2_IO_DELAY_BIT_SHIFT)

#define trigger_proc_in_ext_P3_set_idelay_pin(val)         XIo_Out32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_PIN,   ((XIo_In32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_PIN)   & (~EXT_P3_IO_DELAY_MASK)) | ((val & IO_DELAY_MASK) << EXT_P3_IO_DELAY_BIT_SHIFT)))
#define trigger_proc_in_ext_P3_set_idelay_cm_pll(val)      XIo_Out32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_CMPLL, ((XIo_In32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_CMPLL) & (~EXT_P3_IO_DELAY_MASK)) | ((val & IO_DELAY_MASK) << EXT_P3_IO_DELAY_BIT_SHIFT)))
#define trigger_proc_in_ext_P3_set_odelay_pin(val)         XIo_Out32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_PIN,   ((XIo_In32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_PIN)   & (~EXT_P3_IO_DELAY_MASK)) | ((val & IO_DELAY_MASK) << EXT_P3_IO_DELAY_BIT_SHIFT)))
#define trigger_proc_in_ext_P3_set_odelay_cm_pll(val)      XIo_Out32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_CMPLL, ((XIo_In32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_CMPLL) & (~EXT_P3_IO_DELAY_MASK)) | ((val & IO_DELAY_MASK) << EXT_P3_IO_DELAY_BIT_SHIFT)))
#define trigger_proc_in_ext_P3_get_idelay_pin()            ((XIo_In32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_PIN)   & EXT_P3_IO_DELAY_MASK) >> EXT_P3_IO_DELAY_BIT_SHIFT)
#define trigger_proc_in_ext_P3_get_idelay_cm_pll()         ((XIo_In32(TRIG_MNGR_REG_TRIG_IDELAY_CFG_CMPLL) & EXT_P3_IO_DELAY_MASK) >> EXT_P3_IO_DELAY_BIT_SHIFT)
#define trigger_proc_in_ext_P3_get_odelay_pin()            ((XIo_In32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_PIN)   & EXT_P3_IO_DELAY_MASK) >> EXT_P3_IO_DELAY_BIT_SHIFT)
#define trigger_proc_in_ext_P3_get_odelay_cm_pll()         ((XIo_In32(TRIG_MNGR_REG_TRIG_ODELAY_CFG_CMPLL) & EXT_P3_IO_DELAY_MASK) >> EXT_P3_IO_DELAY_BIT_SHIFT)


#define trigger_proc_idelay_update_set()                   XIo_Out32(TRIG_MNGR_REG_TRIG_IODELAYS_CONTROL, ((XIo_In32(TRIG_MNGR_REG_TRIG_IODELAYS_CONTROL) & (~IDELAY_UPDATE_MASK)) | IDELAY_UPDATE_MASK))
#define trigger_proc_idelay_update_clear()                 XIo_Out32(TRIG_MNGR_REG_TRIG_IODELAYS_CONTROL,  (XIo_In32(TRIG_MNGR_REG_TRIG_IODELAYS_CONTROL) & (~IDELAY_UPDATE_MASK)))

#define trigger_proc_odelay_update_set()                   XIo_Out32(TRIG_MNGR_REG_TRIG_IODELAYS_CONTROL, ((XIo_In32(TRIG_MNGR_REG_TRIG_IODELAYS_CONTROL) & (~ODELAY_UPDATE_MASK)) | ODELAY_UPDATE_MASK))
#define trigger_proc_odelay_update_clear()                 XIo_Out32(TRIG_MNGR_REG_TRIG_IODELAYS_CONTROL,  (XIo_In32(TRIG_MNGR_REG_TRIG_IODELAYS_CONTROL) & (~ODELAY_UPDATE_MASK)))



// ----------------------------------------------------------------------------
// OUTPUT TRIGGER CONFIGURATION
//
// The output triggers are:
//   OUT0 - Baseband
//   OUT1 - AGC
//   OUT2 - External output pin 0
//   OUT3 - External output pin 1
//   OUT4 - External output pin 2
//   OUT5 - External output pin 3
//
// --------------------------------------------------------
// Configuration Register 0
//     [ 8: 0] - AND terms used to trigger output
//     [15: 9] - Reserved
//     [24:16] - OR terms used to trigger output
//     [31:25] - Reserved
//
#define AND_OFFSET_BITS                                    0

#define AND_ETH_A                                          0x00000001
#define AND_ENERGY                                         0x00000002
#define AND_AGC_DONE                                       0x00000004
#define AND_SOFTWARE                                       0x00000008
#define AND_DEBUG0                                         0x00000010
#define AND_DEBUG1                                         0x00000020
#define AND_DEBUG2                                         0x00000040
#define AND_DEBUG3                                         0x00000080
#define AND_ETH_B                                          0x00000100
#define AND_ALL                                            0x000001FF

#define OR_OFFSET_BITS                                     16

#define OR_ETH_A                                           0x00010000
#define OR_ENERGY                                          0x00020000
#define OR_AGC_DONE                                        0x00040000
#define OR_SOFTWARE                                        0x00080000
#define OR_DEBUG0                                          0x00100000
#define OR_DEBUG1                                          0x00200000
#define OR_DEBUG2                                          0x00400000
#define OR_DEBUG3                                          0x00800000
#define OR_ETH_B                                           0x01000000
#define OR_ALL                                             0x01FF0000


// --------------------------------------------------------
// Configuration Register 1
//     [15: 0] - Output Trigger Delay
//     [29:16] - Reserved
//        [30] - Output Trigger Pulse Extender Bypass
//        [31] - Reset
//
// NOTE:  The AGC output trigger has an extended delay to allow for more flexibility about when to
//     start the AGC.  The Delay value for the AGC is 16 bits.
//
//
#define OUT_DELAY_MASK                                     0x0000FFFF

#define OUT_PULSE_EXTENDER_BYPASS_MASK                     0x40000000
#define OUT_HOLD_MODE_MASK                                 0x80000000

#define OUT_HOLD_MODE_ENABLE                               0
#define OUT_HOLD_MODE_DISABLE                              1


// --------------------------------------------------------
// Register Macros

#define trigger_proc_out0_set_config(mask)                 XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_0_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_0_CONF_0) | (mask)))
#define trigger_proc_out0_clear_config(mask)               XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_0_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_0_CONF_0) & (~(mask))))
#define trigger_proc_out0_get_hold_mode()                ((XIo_In32(TRIG_MNGR_REG_TRIG_OUT_0_CONF_1) & OUT_HOLD_MODE_MASK) >> 31)
#define trigger_proc_out0_set_hold_mode(val)               XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_0_CONF_1, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_0_CONF_1) & (~(OUT_HOLD_MODE_MASK))) | ((val << 31) & OUT_HOLD_MODE_MASK))
#define trigger_proc_out0_set_delay(val)                   XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_0_CONF_1, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_0_CONF_1) & (~(OUT_DELAY_MASK    ))) |  (val        & OUT_DELAY_MASK    ))
#define trigger_proc_out0_get_reg_0()                      XIo_In32(TRIG_MNGR_REG_TRIG_OUT_0_CONF_0)
#define trigger_proc_out0_get_reg_1()                      XIo_In32(TRIG_MNGR_REG_TRIG_OUT_0_CONF_1)

#define trigger_proc_out1_set_config(mask)                 XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_1_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_1_CONF_0) | (mask)))
#define trigger_proc_out1_clear_config(mask)               XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_1_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_1_CONF_0) & (~(mask))))
#define trigger_proc_out1_get_hold_mode()                ((XIo_In32(TRIG_MNGR_REG_TRIG_OUT_1_CONF_1) & OUT_HOLD_MODE_MASK) >> 31)
#define trigger_proc_out1_set_hold_mode(val)               XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_1_CONF_1, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_1_CONF_1) & (~(OUT_HOLD_MODE_MASK))) | ((val << 31) & OUT_HOLD_MODE_MASK))
#define trigger_proc_out1_set_delay(val)                   XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_1_CONF_1, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_1_CONF_1) & (~(OUT_DELAY_MASK    ))) |  (val        & OUT_DELAY_MASK    ))
#define trigger_proc_out1_get_reg_0()                      XIo_In32(TRIG_MNGR_REG_TRIG_OUT_1_CONF_0)
#define trigger_proc_out1_get_reg_1()                      XIo_In32(TRIG_MNGR_REG_TRIG_OUT_1_CONF_1)

#define trigger_proc_out2_set_config(mask)                 XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_2_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_2_CONF_0) | (mask)))
#define trigger_proc_out2_clear_config(mask)               XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_2_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_2_CONF_0) & (~(mask))))
#define trigger_proc_out2_get_hold_mode()                ((XIo_In32(TRIG_MNGR_REG_TRIG_OUT_2_CONF_1) & OUT_HOLD_MODE_MASK) >> 31)
#define trigger_proc_out2_set_hold_mode(val)               XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_2_CONF_1, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_2_CONF_1) & (~(OUT_HOLD_MODE_MASK))) | ((val << 31) & OUT_HOLD_MODE_MASK))
#define trigger_proc_out2_set_delay(val)                   XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_2_CONF_1, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_2_CONF_1) & (~(OUT_DELAY_MASK    ))) |  (val        & OUT_DELAY_MASK    ))
#define trigger_proc_out2_get_reg_0()                      XIo_In32(TRIG_MNGR_REG_TRIG_OUT_2_CONF_0)
#define trigger_proc_out2_get_reg_1()                      XIo_In32(TRIG_MNGR_REG_TRIG_OUT_2_CONF_1)

#define trigger_proc_out3_set_config(mask)                 XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_3_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_3_CONF_0) | (mask)))
#define trigger_proc_out3_clear_config(mask)               XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_3_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_3_CONF_0) & (~(mask))))
#define trigger_proc_out3_get_hold_mode()                ((XIo_In32(TRIG_MNGR_REG_TRIG_OUT_3_CONF_1) & OUT_HOLD_MODE_MASK) >> 31)
#define trigger_proc_out3_set_hold_mode(val)               XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_3_CONF_1, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_3_CONF_1) & (~(OUT_HOLD_MODE_MASK))) | ((val << 31) & OUT_HOLD_MODE_MASK))
#define trigger_proc_out3_set_delay(val)                   XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_3_CONF_1, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_3_CONF_1) & (~(OUT_DELAY_MASK    ))) |  (val        & OUT_DELAY_MASK    ))
#define trigger_proc_out3_get_reg_0()                      XIo_In32(TRIG_MNGR_REG_TRIG_OUT_3_CONF_0)
#define trigger_proc_out3_get_reg_1()                      XIo_In32(TRIG_MNGR_REG_TRIG_OUT_3_CONF_1)

#define trigger_proc_out4_set_config(mask)                 XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_4_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_4_CONF_0) | (mask)))
#define trigger_proc_out4_clear_config(mask)               XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_4_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_4_CONF_0) & (~(mask))))
#define trigger_proc_out4_get_hold_mode()                ((XIo_In32(TRIG_MNGR_REG_TRIG_OUT_4_CONF_1) & OUT_HOLD_MODE_MASK) >> 31)
#define trigger_proc_out4_set_hold_mode(val)               XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_4_CONF_1, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_4_CONF_1) & (~(OUT_HOLD_MODE_MASK))) | ((val << 31) & OUT_HOLD_MODE_MASK))
#define trigger_proc_out4_set_delay(val)                   XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_4_CONF_1, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_4_CONF_1) & (~(OUT_DELAY_MASK    ))) |  (val        & OUT_DELAY_MASK    ))
#define trigger_proc_out4_get_reg_0()                      XIo_In32(TRIG_MNGR_REG_TRIG_OUT_4_CONF_0)
#define trigger_proc_out4_get_reg_1()                      XIo_In32(TRIG_MNGR_REG_TRIG_OUT_4_CONF_1)

#define trigger_proc_out5_set_config(mask)                 XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_5_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_5_CONF_0) | (mask)))
#define trigger_proc_out5_clear_config(mask)               XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_5_CONF_0, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_5_CONF_0) & (~(mask))))
#define trigger_proc_out5_get_hold_mode()                ((XIo_In32(TRIG_MNGR_REG_TRIG_OUT_5_CONF_1) & OUT_HOLD_MODE_MASK) >> 31)
#define trigger_proc_out5_set_hold_mode(val)               XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_5_CONF_1, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_5_CONF_1) & (~(OUT_HOLD_MODE_MASK))) | ((val << 31) & OUT_HOLD_MODE_MASK))
#define trigger_proc_out5_set_delay(val)                   XIo_Out32(TRIG_MNGR_REG_TRIG_OUT_5_CONF_1, (XIo_In32(TRIG_MNGR_REG_TRIG_OUT_5_CONF_1) & (~(OUT_DELAY_MASK    ))) |  (val        & OUT_DELAY_MASK    ))
#define trigger_proc_out5_get_reg_0()                      XIo_In32(TRIG_MNGR_REG_TRIG_OUT_5_CONF_0)
#define trigger_proc_out5_get_reg_1()                      XIo_In32(TRIG_MNGR_REG_TRIG_OUT_5_CONF_1)



// ----------------------------------------------------------------------------
// Misc Registers
//
// --------------------------------------------------------
// Trigger Output Value Register:
//     [ 7: 0] - Number of Trigger Inputs
//     [15: 8] - Number of Trigger Outputs
//     [23:16] - Core ID
//     [31:24] - Reserved
//
#define trigger_proc_get_core_info()                       (XIo_In32(TRIG_MNGR_REG_CORE_INFO))



// --------------------------------------------------------
// Trigger Output Value Register:
//        [ 0] - Value of Output Trigger 0
//        [ 1] - Value of Output Trigger 1
//        [ 2] - Value of Output Trigger 2
//        [ 3] - Value of Output Trigger 3
//        [ 4] - Value of Output Trigger 4
//        [ 5] - Value of Output Trigger 5
//     [31: 6] - Reserved
//
#define OUT0                                               0x00000001
#define OUT1                                               0x00000002
#define OUT2                                               0x00000004
#define OUT3                                               0x00000008
#define OUT4                                               0x00000010
#define OUT5                                               0x00000020


// --------------------------------------------------------
// Macros
//
#define trigger_proc_get_output_values()                   (XIo_In32(TRIG_MNGR_REG_TRIG_OUTPUT))



// ----------------------------------------------------------------------------
// Energy Detection Registers
//
// --------------------------------------------------------
// Configuration Register:
//        [ 0] - Detect energy on RFA
//        [ 1] - Detect energy on RFB
//        [ 2] - Detect energy on RFC
//        [ 3] - Detect energy on RFD
//     [30: 4] - Reserved
//        [31] - Reset
//
#define WL_PACKET_DETECT_CONFIG_REG_RESET                  0x80000000
#define WL_PACKET_DETECT_CONFIG_REG_MASK_A                 0x00000001
#define WL_PACKET_DETECT_CONFIG_REG_MASK_B                 0x00000002
#define WL_PACKET_DETECT_CONFIG_REG_MASK_C                 0x00000004
#define WL_PACKET_DETECT_CONFIG_REG_MASK_D                 0x00000008
#define WL_PACKET_DETECT_CONFIG_REG_MASK_ALL               0x0000000F


// --------------------------------------------------------
// Macros
//

// Convert interface masks to packet detect mask
#define IFC_TO_PACKET_DETECT_MASK(val)                     (val >> 28)

#define wl_packet_detect_set_config(mask)                  XIo_Out32(TRIG_MNGR_REG_RSSI_PKT_DET_CONFIG, (XIo_In32(TRIG_MNGR_REG_RSSI_PKT_DET_CONFIG) | (mask)))
#define wl_packet_detect_clear_config(mask)                XIo_Out32(TRIG_MNGR_REG_RSSI_PKT_DET_CONFIG, (XIo_In32(TRIG_MNGR_REG_RSSI_PKT_DET_CONFIG) & (~(mask))))

#define wl_packet_detect_set_idle_threshold(idle)          XIo_Out32(TRIG_MNGR_REG_RSSI_PKT_DET_THRESHOLDS, (XIo_In32(TRIG_MNGR_REG_RSSI_PKT_DET_THRESHOLDS) & (~0x0000FFFF)) |  (idle       & 0x0000FFFF))
#define wl_packet_detect_set_busy_threshold(busy)          XIo_Out32(TRIG_MNGR_REG_RSSI_PKT_DET_THRESHOLDS, (XIo_In32(TRIG_MNGR_REG_RSSI_PKT_DET_THRESHOLDS) & (~0xFFFF0000)) | ((busy << 16)& 0xFFFF0000))

#define wl_packet_detect_set_RSSI_duration(rssi)           XIo_Out32(TRIG_MNGR_REG_RSSI_PKT_DET_DURATIONS, (XIo_In32(TRIG_MNGR_REG_RSSI_PKT_DET_DURATIONS) & (~0x1F0000)) | ((rssi <<16)& 0x1F0000))
#define wl_packet_detect_set_idle_duration(idle)           XIo_Out32(TRIG_MNGR_REG_RSSI_PKT_DET_DURATIONS, (XIo_In32(TRIG_MNGR_REG_RSSI_PKT_DET_DURATIONS) & (~0x0000FF)) |  (idle      & 0x0000FF))
#define wl_packet_detect_set_busy_duration(busy)           XIo_Out32(TRIG_MNGR_REG_RSSI_PKT_DET_DURATIONS, (XIo_In32(TRIG_MNGR_REG_RSSI_PKT_DET_DURATIONS) & (~0x00FF00)) | ((busy << 8)& 0x00FF00))



// ----------------------------------------------------------------------------
// Defines for warplab_trigger_proc core operators

// Equals
#define U8_OP_EQ                                           0x01
#define U16_OP_EQ                                          ((U8_OP_EQ <<  8) | U8_OP_EQ)
#define U32_OP_EQ                                          ((U8_OP_EQ << 24) | (U8_OP_EQ << 16) | (U8_OP_EQ <<  8) | U8_OP_EQ)

// Not-equals
#define U8_OP_NEQ                                          0x02
#define U16_OP_NEQ                                         ((U8_OP_NEQ <<  8) | U8_OP_NEQ)
#define U32_OP_NEQ                                         ((U8_OP_NEQ << 24) | (U8_OP_NEQ << 16) | (U8_OP_NEQ <<  8) | U8_OP_NEQ)

// No care (byte is ignored)
#define U8_OP_NC                                          0x00
#define U16_OP_NC                                         ((U8_OP_NC <<  8) | U8_OP_NC)
#define U32_OP_NC                                         ((U8_OP_NC << 24) | (U8_OP_NC << 16) | (U8_OP_NC <<  8) | U8_OP_NC)

// Any-of-and (and(x,y) > 0)
#define U8_OP_AA                                          0x03
#define U16_OP_AA                                         ((U8_OP_AA <<  8) | U8_OP_AA)
#define U32_OP_AA                                         ((U8_OP_AA << 24) | (U8_OP_AA << 16) | (U8_OP_AA <<  8) | U8_OP_AA)



/*********************** Global Structure Definitions ************************/



/*************************** Function Prototypes *****************************/
int  trigmngr_init();

int  trigmngr_process_cmd(int socket_index, void * from, wl_cmd_resp * command, wl_cmd_resp * response);

void trigmngr_trigger_in(u32 trig_id, u32 eth_dev_num);


#endif /* TRIGCONF_H_ */
