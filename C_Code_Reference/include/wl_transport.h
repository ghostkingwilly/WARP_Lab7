/** @file wl_transport.h
 *  @brief WARPLab Framework (Transport)
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

/**********************************************************************************************************************/
/**
 * @brief Common Functions (WARP v2 and WARP v3)
 *
 **********************************************************************************************************************/

/***************************** Include Files *********************************/

// WARPLab includes
#include "wl_common.h"


// WARP UDP transport includes
#include <WARP_ip_udp.h>
#include <WARP_ip_udp_device.h>


/*************************** Constant Definitions ****************************/
#ifndef WL_TRANSPORT_H_
#define WL_TRANSPORT_H_

// **********************************************************************
// Command IDs (must match the CMD_ properties in wl_transport_*.m)
//
#define CMDID_TRANSPORT_PING                               0x000001
#define CMDID_TRANSPORT_PAYLOAD_SIZE_TEST                  0x000002

#define CMDID_TRANSPORT_NODE_GROUP_ID_ADD                  0x000010
#define CMDID_TRANSPORT_NODE_GROUP_ID_CLEAR                0x000011


// ***********************************************************************
// Define WARPLab Transport Ethernet Information
//
#define WL_NUM_ETH_DEVICES                                 WARP_IP_UDP_NUM_ETH_DEVICES
#define WL_ETH_DEV_INITIALIZED                             1

#define WL_IP_UDP_TRANSPORT                                1


// Ethernet constants
#define ETH_DO_NOT_WAIT_FOR_AUTO_NEGOTIATION               0
#define ETH_WAIT_FOR_AUTO_NEGOTIATION                      1

// WARPLab message types
#define PKTTYPE_TRIGGER                                    0
#define PKTTYPE_HTON_MSG                                   1
#define PKTTYPE_NTOH_MSG                                   2

// Transport status types
#define LINK_READY                                         0
#define LINK_NOT_READY                                    -1

// Transport header flags (16 bits)
#define TRANSPORT_HDR_ROBUST_FLAG                          0x0001
#define TRANSPORT_HDR_NODE_NOT_READY_FLAG                  0x8000


/*********************** Global Structure Definitions ************************/

// WARPLab Transport header
//     NOTE:  This conforms to the WARPLab Transport Header Wire Format:
//            http://warpproject.org/trac/wiki/WARPLab/Reference/Architecture/WireFormat
//
typedef struct {
	u16                      dest_id;
	u16                      src_id;
	u8                       reserved;
	u8                       pkt_type;
	u16                      length;
	u16                      seq_num;
	u16                      flags;
} wl_transport_header;


// WARPLab Ethernet device information
//     NOTE:  This is so that differences between different Ethernet devices can be consolidated
//
typedef struct {
	u32                      initialized;                  // Etherent device initialized
	u32                      default_speed;                // Default Ethernet speed
	u32                      phy_addr;                     // Address of the Ethernt PHY

	u32                      type;                         // Transport Type
	u32                      hw_addr[2];                   // HW Address (big endian as 2 u32 values with 16 bit padding)
	u32                      ip_addr;                      // IP Address (big endian)
	int                      unicast_socket;               // Unicast socket index
	int                      broadcast_socket;             // Broadcast socket index
	u32                      group_id;                     // Group ID
} wl_eth_dev_info;



/*************************** Function Prototypes *****************************/

int  transport_init(u32 eth_dev_num, u8 init_driver);

int  transport_set_process_hton_msg_callback(void(*handler));
int  transport_process_cmd(int socket_index, void * from, wl_cmd_resp * command, wl_cmd_resp * response);

void transport_poll(u32 eth_dev_num);
void transport_send(int socket_index, struct sockaddr * to, warp_ip_udp_buffer ** buffers, u32 num_buffers);
void transport_close(u32 eth_dev_num);

int  transport_get_hw_info(u32 eth_dev_num, u8 * ip_addr, u8 * hw_addr);
int  transport_config_sockets(u32 eth_dev_num, u32 unicast_port, u32 bcast_port);
int  transport_config_socket(u32 eth_dev_num, int * socket_index, u32 udp_port);

int  transport_link_status(u32 eth_dev_num);
u32  transport_update_link_speed(u32 eth_dev_num, u32 wait_for_negotiation);
u16  transport_get_ethernet_status(u32 eth_dev_num);





/**********************************************************************************************************************/
/**
 * @brief WARP v3 Specific Functions
 *
 **********************************************************************************************************************/

#ifdef WARP_HW_VER_v3

/***************************** Include Files *********************************/


/*************************** Constant Definitions ****************************/

// Ethernet A constants
#define WL_ETH_A                                           ETH_A_MAC
#define WL_ETH_A_MDIO_PHYADDR                              0x6

// Ethernt B constants
#define WL_ETH_B                                           ETH_B_MAC
#define WL_ETH_B_MDIO_PHYADDR                              0x7

// Ethernet PHY constants
#define ETH_PHY_CONTROL_REG                                0
#define ETH_PHY_STATUS_REG                                 17

#define ETH_PHY_REG_0_RESET                                0x8000
#define ETH_PHY_REG_0_SPEED_LSB                            0x2000
#define ETH_PHY_REG_0_AUTO_NEGOTIATION                     0x1000
#define ETH_PHY_REG_0_SPEED_MSB                            0x0040

#define ETH_PHY_REG_17_0_SPEED                             0xC000
#define ETH_PHY_REG_17_0_SPEED_RESOLVED                    0x0800
#define ETH_PHY_REG_17_0_LINKUP                            0x0400


// NOTE:  These constants are for in place computation based on ETH_PHY_REG_17_0_SPEED (ie no bit shifting required)
#define ETH_PHY_REG_17_0_SPEED_10_MBPS                     0x0000
#define ETH_PHY_REG_17_0_SPEED_100_MBPS                    0x4000
#define ETH_PHY_REG_17_0_SPEED_1000_MBPS                   0x8000
#define ETH_PHY_REG_17_0_SPEED_RSVD                        0xC000


// Ethernet PHY macros
//     NOTE:  Speed is defined by the Ethernet hardware (xaxiethernet_hw.h and equivalent for WARP v2):
//         #define XAE_SPEED_10_MBPS        10      /**< Speed of 10 Mbps */
//         #define XAE_SPEED_100_MBPS       100     /**< Speed of 100 Mbps */
//         #define XAE_SPEED_1000_MBPS      1000    /**< Speed of 1000 Mbps */
//
#define ETH_PHY_SPEED_10_MBPS                              10
#define ETH_PHY_SPEED_100_MBPS                             100
#define ETH_PHY_SPEED_1000_MBPS                            1000

#define ETH_PHY_SPEED_TO_MBPS(speed)                       ((speed == ETH_PHY_REG_17_0_SPEED_1000_MBPS) ? ETH_PHY_SPEED_1000_MBPS : \
                                                            (speed == ETH_PHY_REG_17_0_SPEED_100_MBPS)  ? ETH_PHY_SPEED_100_MBPS  : \
                                                            (speed == ETH_PHY_REG_17_0_SPEED_10_MBPS)   ? ETH_PHY_SPEED_10_MBPS   : 0 )



/*********************** Global Structure Definitions ************************/


/******************************** Functions **********************************/

#endif

#endif /* WL_TRANSPORT_H_ */
