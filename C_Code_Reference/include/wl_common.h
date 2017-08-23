/** @file wl_common.h
 *  @brief WARPLab Framework (Common)
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

// Xilinx / Standard library includes
#include <xil_types.h>

// WARPLab includes
#include "warp_hw_ver.h"


/*************************** Constant Definitions ****************************/
#ifndef WL_COMMON_H_
#define WL_COMMON_H_


// **********************************************************************
// WARPLab Controls
//
//   NOTE:  These are the most common parameters that would be modified by a user:
//       1) Debug print level
//       2) DDR initialization
//       3) Ethernet controls
//


// 1) Choose the default debug print level
//
//    Values (see wl_common.h for more information):
//        WL_PRINT_NONE      - Print WL_PRINT_NONE messages
//        WL_PRINT_ERROR     - Print WL_PRINT_ERROR and WL_PRINT_NONE messages
//        WL_PRINT_WARNING   - Print WL_PRINT_WARNING, WL_PRINT_ERROR and WL_PRINT_NONE messages
//        WL_PRINT_INFO      - Print WL_PRINT_INFO, WL_PRINT_WARNING, WL_PRINT_ERROR and WL_PRINT_NONE messages
//        WL_PRINT_DEBUG     - Print WL_PRINT_DEBUG, WL_PRINT_INFO, WL_PRINT_WARNING, WL_PRINT_ERROR and WL_PRINT_NONE messages
//
#define DEFAULT_DEBUG_PRINT_LEVEL                          WL_PRINT_WARNING


// 2) Initialize the DDR to zeros (ie clear DDR) at boot
//
//    Values:
//        1                  - Clear DDR on boot
//        0                  - Do not clear DDR on boot
//
//     NOTE:  Based on initial testing, clearing DDR on boot will add approximately 1 second to
//            the boot time of the node.  See clear_ddr() in wl_common.c for more information.
//
#define CLEAR_DDR_ON_BOOT                                  0


// 3) Ethernet controls
//
//    a) Choose the Ethernet device and set the base address for the subnet and speed of the device:
//
//       Values for WL_USE_ETH_A and WL_USE_ETH_B:
//           1               - Use the Ethernet device
//           0               - Do not use the Ethernet device
//
//       Values for *_IP_ADDR_BASE:
//           0xAABBCC00      - Hexadecimal representation of an IP subnet:  AA.BB.CC.00
//                             where AA, BB, and CC are hexadecimal numbers.
//
//           NOTE:  IP subnet should match the host networking setup defined in wl_setup
//           NOTE:  Ethernet devices can not be on the same subnet.  The transport_read_ip_addr()
//                  function in wl_transport.c assigns the same final IP address octet to both
//                  Ethernet devices.
//
//       Values for *_DEFAULT_SPEED:
//           1000            - 1000 Mbps Ethernet (ie 1Gbps)
//           100             - 100 Mbps Ethernet
//           10              - 10 Mbps Ethernet
//
#define WL_USE_ETH_A                                       1
#define WL_ETH_A_IP_ADDR_BASE                              0x0a000000     // 10.0.0.x
#define WL_ETH_A_DEFAULT_SPEED                             1000

#define WL_USE_ETH_B                                       0
#define WL_ETH_B_IP_ADDR_BASE                              0x0a000100     // 10.0.1.x
#define WL_ETH_B_DEFAULT_SPEED                             1000


//    b) Wait for WARPNet Ethernet interface to be ready before continuing boot
//
//       Values:
//           1               - Wait for Ethernet device to be ready
//           0               - Do not wait for Ethernet device to be ready
//
//    NOTE:  During boot, this parameter will cause the node to wait for all Ethernet
//        devices with WL_USE_ETH_* = 1 to be ready.  This means if the node is configured
//        to use both ETH A and ETH B, then the node will wait until the link is ready for
//        both ETH A and ETH B.
//
#define WL_WAIT_FOR_ETH                                    1


//    c) Allow Ethernet Link speed to be negotiated
//
//       Values of WL_NEGOTIATE_ETH_LINK_SPEED:
//           1               - Auto-negotiate the Ethernet link speed
//           0               - Do not auto-negotiate the Ethernet link speed.  Speed is set
//                             by the *_DEFAULT_SPEED defined above.
//
//     NOTE:  Based on initial testing, auto-negotiation of the link speed will add around 3
//            seconds to the boot time of the node.  To bypass auto-negotiation but use a
//            different default link speed, please adjust the WL_ETH_*_DEFAULT_SPEED define
//            above.
//
#define WL_NEGOTIATE_ETH_LINK_SPEED                        0


//    d) Allow Ethernet reception of packets to be paused via the UART terminal
//
//       Values:
//           1               - Allow Ethernet receptions to be paused via UART terminal
//           0               - Do not allow Ethernet receptions to be paused via UART terminal
//
//    NOTE:  Use the character 's' in the UART terminal to 's'tart and 's'top reception of
//        Ethernet packets.
//
#define ALLOW_ETHERNET_PAUSE                               0



// **********************************************************************
// WARPLab Version Information
//

// Version info (MAJOR.MINOR.REV, all must be ints)
//     MAJOR and MINOR are both u8, while REV is u16
//     m-code requires C code MAJOR.MINOR match values in wl_version.ini
#define WARPLAB_VER_MAJOR	                               7
#define WARPLAB_VER_MINOR	                               7
#define WARPLAB_VER_REV		                               1

#define REQ_WARPLAB_HW_VER                               ((WARPLAB_VER_MAJOR << 16) | (WARPLAB_VER_MINOR << 8) | (WARPLAB_VER_REV))



// **********************************************************************
// Interface Configuration Information
//

// Use 4 Antennas (default is 0, ie 2 Antennas)
#define WARPLAB_CONFIG_4RF                                 0



// **********************************************************************
// Network Configuration Information
//     NOTE:  The values below must match the corresponding values in wl_config.ini
//

// Default network info
//     - The base IP address should be a u32 (big endian) with (at least) the last octet 0x00
//
#define BROADCAST_DEST_ID                                  0xFFFF

// Default ports
//     - unicast ports are used for host-to-node
//     - multicast for triggers and host-to-multinode
//
#define NODE_UDP_UNICAST_PORT_BASE	                        9000
#define NODE_UDP_MCAST_BASE			                       10000



// **********************************************************************
// WARPLab Common Defines
//
#define PAYLOAD_PAD_NBYTES                                 2

#define NO_RESP_SENT                                       0
#define RESP_SENT                                          1
#define NODE_NOT_READY                                     2

#define SUCCESS                                            0
#define FAILURE                                           -1

#define WL_CMD_TO_GRP(x)                                  ((x) >> 24)
#define WL_CMD_TO_CMDID(x)                                ((x) & 0x00FFFFFF)

#define FPGA_DNA_LEN                                       2
#define IP_VERSION                                         4
#define ETH_ADDR_LEN	                                   6

#define WL_TRUE                                            1
#define WL_FALSE                                           0

#define WL_NO_TRANSMIT                                     0
#define WL_TRANSMIT                                        1

#define WL_ENABLE                                          1
#define WL_DISABLE                                         0

#define WL_SILENT                                          0
#define WL_VERBOSE                                         1


// **********************************************************************
// WARPLab Command Defines
//
#define CMD_PARAM_WRITE_VAL                                0x00000000
#define CMD_PARAM_READ_VAL                                 0x00000001
#define CMD_PARAM_RSVD                                     0xFFFFFFFF

#define CMD_PARAM_SUCCESS                                  0x00000000
#define CMD_PARAM_ERROR                                    0xFF000000



// **********************************************************************
// Defines for non-invasive debug
//
#define _DEBUG_STORAGE_                                    0
#define _DEBUG_STORAGE_SIZE_                               400

#define _MEASUREMENT_PRINT_                                0
#define _MEASUREMENT_PRINT_WIDTH_                          4



/*********************** Global Structure Definitions ************************/

// **********************************************************************
// WARPLab Message Structures
//

// Command / Response Header
//     NOTE:  This conforms to the WARPLab Command / Response Wire Format:
//            http://warpproject.org/trac/wiki/WARPLab/Reference/Architecture/WireFormat
//
typedef struct{
    u32                      cmd;
	u16                      length;
	u16                      num_args;
} wl_cmd_resp_hdr;


// Command / Response data structure
//     This structure is used to keep track of pointers when decoding WARPLab commands.
//
typedef struct {
    void                   * buffer;                       // In general, assumed to be a (warp_ip_udp_buffer *)
    wl_cmd_resp_hdr        * header;
    u32                    * args;
} wl_cmd_resp;



// **********************************************************************
// WARPLab Function pointer
//
typedef int (*wl_function_ptr_t)();



// **********************************************************************
// WARPLab Tag Parameter Structure
//
typedef struct {
	u8    reserved;
	u8    group;
	u16   length;
	u32   command;
	u32  *value;
} wl_tag_parameter;



// **********************************************************************
// WARPLab Print Levels
//
#define WL_PRINT_NONE                                      0
#define WL_PRINT_ERROR                                     1
#define WL_PRINT_WARNING                                   2
#define WL_PRINT_INFO                                      3
#define WL_PRINT_DEBUG                                     4


#define wl_printf(level, type, format, args...) \
	                    do {  \
                            if (level <= wl_print_level) { \
                            	wl_print_header(level, type, __FILE__, __LINE__); \
                                xil_printf(format, ##args); \
                            } \
                        } while (0)


extern u8       wl_print_level;
extern char   * print_type_node;
extern char   * print_type_transport;
extern char   * print_type_interface;
extern char   * print_type_baseband;
extern char   * print_type_trigger;
extern char   * print_type_user;



/*************************** Function Prototypes *****************************/

// Peripheral Init Functions
int           wl_timer_initialize();
void          wl_gpio_debug_initialize();

// Callbacks
int           wl_null_callback(void* param);

// Printing Functions
void          wl_print_header(u8 level, char * type, char * filename, u32 line);
void          wl_print_mac_address(u8 level, u8 * mac_address);

void          wl_set_print_level(u8 level);

void          print_array_u8(u8 *buf, u32 size);
void          print_array_u32(u32 *buf, u32 size);

// GPIO pins on debug header
inline void   wl_setDebugGPIO(u8 mask);
inline void   wl_clearDebugGPIO(u8 mask);

// 7 segment display
u8            sevenSegmentMap(u8 hex_value);

// Micro-second timestamp counter
u64           get_usec_timestamp();

// Non-invasive debug functions
void          add_to_debug_storage(u32 value, u32 enable);
void          remove_from_debug_storage(u32 num_elements, u32 enable);
void          reset_debug_storage();
void          print_debug_storage();



/**********************************************************************************************************************/
/**
 * @brief WARP v3 Specific Functions
 *
 **********************************************************************************************************************/

#ifdef WARP_HW_VER_v3

/***************************** Include Files *********************************/

/*************************** Constant Definitions ****************************/

// **********************************************************************
// WARPLab Peripheral Defines
//
#define USERIO_BASEADDR	                                   XPAR_W3_USERIO_BASEADDR
#define EEPROM_BASEADDR                                    XPAR_W3_IIC_EEPROM_ONBOARD_BASEADDR
#define RC_BASEADDR                                        XPAR_RADIO_CONTROLLER_0_BASEADDR
#define CLK_BASEADDR                                       XPAR_W3_CLOCK_CONTROLLER_0_BASEADDR
#define DRAM_BASEADDR                                      XPAR_DDR3_SODIMM_S_AXI_BASEADDR
#define AD_BASEADDR                                        XPAR_W3_AD_CONTROLLER_0_BASEADDR
#define SYSMON_BASEADDR                                    XPAR_SYSMON_0_BASEADDR

#define DDR_SIZE                                          (XPAR_DDR3_SODIMM_S_AXI_HIGHADDR - XPAR_DDR3_SODIMM_S_AXI_BASEADDR + 1)


/*********************** Global Structure Definitions ************************/

// **********************************************************************
// WARPLab Interrupt State
//
typedef enum {INTERRUPTS_DISABLED, INTERRUPTS_ENABLED} interrupt_state_t;


/******************************** Functions **********************************/

// Peripheral Init Functions
void                         wl_sysmon_initialize();
int                          wl_cdma_initialize();
int                          wl_uart_initialize();

// DMA Functions
void                         wl_cdma_transfer(u32 src_address, u32 dest_address, u32 length);
int                          wl_cdma_busy();

// Callbacks
void                         wl_set_uart_rx_callback(wl_function_ptr_t callback);

// Interrupt Functions
int                          wl_interrupt_init();
inline int                   wl_interrupt_restore_state(interrupt_state_t new_interrupt_state);
inline interrupt_state_t     wl_interrupt_stop();

// HW specific functions
void                        usleep(u32 duration);
int                         microblaze_right_shift_test();
int                         ddr_sodim_memory_test();
void                        clear_ddr(u32 verbose);

#endif


#endif /* WL_COMMON_H_ */
