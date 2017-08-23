/** @file wl_baseband.h
 *  @brief WARPLab Framework (Baseband)
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

// Xilinx / Standard library includes
#include <xparameters.h>

// WARPLab includes
#include "wl_common.h"


/*************************** Constant Definitions ****************************/
#ifndef WL_BASEBAND_H_
#define WL_BASEBAND_H_

// **********************************************************************
// Command IDs (must match the CMD_ properties in wl_baseband_buffers.m)
//
#define CMDID_BASEBAND_TX_DELAY                            0x000001
#define CMDID_BASEBAND_TX_LENGTH                           0x000002
#define CMDID_BASEBAND_TX_MODE                             0x000003
#define CMDID_BASEBAND_TX_BUFF_EN                          0x000004
#define CMDID_BASEBAND_RX_BUFF_EN                          0x000005
#define CMDID_BASEBAND_TXRX_BUFF_DIS                       0x000006
#define CMDID_BASEBAND_TXRX_BUFF_STATE                     0x000007
#define CMDID_BASEBAND_WRITE_IQ                            0x000008
#define CMDID_BASEBAND_READ_IQ                             0x000009
#define CMDID_BASEBAND_READ_RSSI                           0x00000A
#define CMDID_BASEBAND_RX_LENGTH                           0x00000B
#define CMDID_BASEBAND_WRITE_IQ_CHECKSUM                   0x00000C
#define CMDID_BASEBAND_MAX_NUM_SAMPLES                     0x00000D

#define CMDID_BASEBAND_TXRX_COUNT_RESET                    0x000010
#define CMDID_BASEBAND_TXRX_COUNT_GET                      0x000011

#define CMDID_BASEBAND_AGC_STATE                           0x000100
#define CMDID_BASEBAND_AGC_DONE_ADDR                       0x000101
#define CMDID_BASEBAND_AGC_RESET                           0x000102
#define CMDID_BASEBAND_AGC_RESET_MODE                      0x000103

#define CMDID_BASEBAND_AGC_TARGET                          0x000110
#define CMDID_BASEBAND_AGC_DCO_EN_DIS                      0x000111

#define CMDID_BASEBAND_AGC_CONFIG                          0x000120
#define CMDID_BASEBAND_AGC_IIR_HPF                         0x000121
#define CMDID_BASEBAND_AGC_RF_GAIN_THRESHOLD               0x000122
#define CMDID_BASEBAND_AGC_TIMING                          0x000123
#define CMDID_BASEBAND_AGC_DCO_TIMING                      0x000124



// #define CMDID_BASEBAND_DEBUG_TX_OUTPUT_CONFIGURE           0x000080


// **********************************************************************
// WARPLab Buffers core debug parameters
//
#define USE_GENERATED_RX_DATA                              0
#define USE_TX_RX_LOOPBACK                                 0


// **********************************************************************
// Samples Constants
//
#define BYTES_PER_SAMP                                     4



// **********************************************************************
// Misc Constants
//
#define INIT_TX_DELAY                                      0
#define WL_BUF_DEBUG_4RF_ON_2RF                            0



// **********************************************************************
// Command Parameter Constants
//
#define CMD_PARAM_BASEBAND_TXRX_COUNT_GET_TX               0
#define CMD_PARAM_BASEBAND_TXRX_COUNT_GET_RX               1

#define CMD_PARAM_BASEBAND_TXRX_COUNT_GET_COUNT_RSVD       0xFFFFFFFF




// **********************************************************************
// Common memory defines for BRAM sample buffers
//   - ASSUME: all BRAM memories are the same size
//
#define WARPLAB_IQ_RX_BUF_SIZE                             WARPLAB_IQ_RX_BUF_A_SIZE
#define WARPLAB_IQ_TX_BUF_SIZE                             WARPLAB_IQ_TX_BUF_A_SIZE
#define WARPLAB_RSSI_BUF_SIZE                              WARPLAB_RSSI_BUF_A_SIZE

#define WL_BUF_DEFAULT_RX_NUM_SAMPLES                    ((WARPLAB_IQ_RX_BUF_A_SIZE >> 2) - 1)
#define WL_BUF_DEFAULT_TX_NUM_SAMPLES                    ((WARPLAB_IQ_TX_BUF_A_SIZE >> 2) - 1)



// **********************************************************************
// Defines for WARPLab Buffers Core
//     - Renamed from XPAR* here for easier maintenance
//

// Buffers Register definitions
#define WL_BUF_REG_DESIGN_VER                              XPAR_WARPLAB_BUFFERS_MEMMAP_DESIGN_VER
#define WL_BUF_REG_BUF_SIZES                               XPAR_WARPLAB_BUFFERS_MEMMAP_BUFF_SIZES
#define WL_BUF_REG_CONFIG                                  XPAR_WARPLAB_BUFFERS_MEMMAP_CONFIG
#define WL_BUF_REG_STATUS                                  XPAR_WARPLAB_BUFFERS_MEMMAP_STATUS

#define WL_BUF_REG_TX_DELAY                                XPAR_WARPLAB_BUFFERS_MEMMAP_TX_DELAY
#define WL_BUF_REG_RX_LENGTH                               XPAR_WARPLAB_BUFFERS_MEMMAP_RX_LENGTH
#define WL_BUF_REG_TX_LENGTH                               XPAR_WARPLAB_BUFFERS_MEMMAP_TX_LENGTH

#define WL_BUF_REG_RF_BUFFER_SEL                           XPAR_WARPLAB_BUFFERS_MEMMAP_RF_BUFFER_SEL
#define WL_BUF_REG_RX_BUF_EN                               XPAR_WARPLAB_BUFFERS_MEMMAP_RX_BUF_EN
#define WL_BUF_REG_TX_BUF_EN                               XPAR_WARPLAB_BUFFERS_MEMMAP_TX_BUF_EN

#define WL_BUF_REG_AGC_DONE_ADDR                           XPAR_WARPLAB_BUFFERS_MEMMAP_AGC_DONE_ADDR
#define WL_BUF_REG_RF_AB_AGC_DONE_RSSI                     XPAR_WARPLAB_BUFFERS_MEMMAP_RFAB_AGC_DONE_RSSI
#define WL_BUF_REG_RF_CD_AGC_DONE_RSSI                     XPAR_WARPLAB_BUFFERS_MEMMAP_RFCD_AGC_DONE_RSSI

#define WL_BUF_REG_RF_RX_IQ_BUF_RD_BYTE_OFFSET             XPAR_WARPLAB_BUFFERS_MEMMAP_RF_RX_IQ_BUF_RD_BYTE_OFFSET
#define WL_BUF_REG_RF_RX_IQ_BUF_WR_BYTE_OFFSET             XPAR_WARPLAB_BUFFERS_MEMMAP_RF_RX_IQ_BUF_WR_BYTE_OFFSET
#define WL_BUF_REG_RF_RX_IQ_BUF_WR_BYTE_OFFSET_UPDATE      XPAR_WARPLAB_BUFFERS_MEMMAP_RF_RX_IQ_BUF_WR_BYTE_OFFSET_UPDATE
#define WL_BUF_REG_RF_RX_IQ_THRESHOLD                      XPAR_WARPLAB_BUFFERS_MEMMAP_RF_RX_IQ_THRESHOLD
#define WL_BUF_REG_RF_RX_IQ_BUF_OCCUPANCY                  XPAR_WARPLAB_BUFFERS_MEMMAP_RF_RX_IQ_BUF_OCCUPANCY

#define WL_BUF_REG_RF_TX_IQ_BUF_RD_BYTE_OFFSET             XPAR_WARPLAB_BUFFERS_MEMMAP_RF_TX_IQ_BUF_RD_BYTE_OFFSET
#define WL_BUF_REG_RF_TX_IQ_BUF_WR_BYTE_OFFSET             XPAR_WARPLAB_BUFFERS_MEMMAP_RF_TX_IQ_BUF_WR_BYTE_OFFSET
#define WL_BUF_REG_RF_TX_IQ_THRESHOLD                      XPAR_WARPLAB_BUFFERS_MEMMAP_RF_TX_IQ_THRESHOLD
#define WL_BUF_REG_RF_TX_IQ_BUF_OCCUPANCY                  XPAR_WARPLAB_BUFFERS_MEMMAP_RF_TX_IQ_BUF_OCCUPANCY
#define WL_BUF_REG_RF_TX_IQ_STATUS                         XPAR_WARPLAB_BUFFERS_MEMMAP_RF_TX_IQ_STATUS

#define WL_BUF_REG_RF_ERROR_CLR                            XPAR_WARPLAB_BUFFERS_MEMMAP_RF_ERROR_CLR
#define WL_BUF_REG_INT_STATUS                              XPAR_WARPLAB_BUFFERS_MEMMAP_INT_STATUS

#define WL_BUF_REG_TXRX_COUNTER_RESET                      XPAR_WARPLAB_BUFFERS_MEMMAP_TXRX_COUNTER_RESET

#define WL_BUF_REG_RFA_TX_COUNTER                          XPAR_WARPLAB_BUFFERS_MEMMAP_RFA_TX_COUNTER
#define WL_BUF_REG_RFB_TX_COUNTER                          XPAR_WARPLAB_BUFFERS_MEMMAP_RFB_TX_COUNTER
#define WL_BUF_REG_RFC_TX_COUNTER                          XPAR_WARPLAB_BUFFERS_MEMMAP_RFC_TX_COUNTER
#define WL_BUF_REG_RFD_TX_COUNTER                          XPAR_WARPLAB_BUFFERS_MEMMAP_RFD_TX_COUNTER
#define WL_BUF_REG_RFA_RX_COUNTER                          XPAR_WARPLAB_BUFFERS_MEMMAP_RFA_RX_COUNTER
#define WL_BUF_REG_RFB_RX_COUNTER                          XPAR_WARPLAB_BUFFERS_MEMMAP_RFB_RX_COUNTER
#define WL_BUF_REG_RFC_RX_COUNTER                          XPAR_WARPLAB_BUFFERS_MEMMAP_RFC_RX_COUNTER
#define WL_BUF_REG_RFD_RX_COUNTER                          XPAR_WARPLAB_BUFFERS_MEMMAP_RFD_RX_COUNTER

#define WL_LOAD_TIMER_64_LSB                               XPAR_WARPLAB_BUFFERS_MEMMAP_LOAD_TIMER_64_LSB
#define WL_LOAD_TIMER_64_MSB                               XPAR_WARPLAB_BUFFERS_MEMMAP_LOAD_TIMER_64_MSB
#define WL_TIMER_64_LSB                                    XPAR_WARPLAB_BUFFERS_MEMMAP_TIMER_64_LSB
#define WL_TIMER_64_MSB                                    XPAR_WARPLAB_BUFFERS_MEMMAP_TIMER_64_MSB


// Masks for CONFIG register
#define WL_BUF_REG_CONFIG_CONT_TX                          0x00000001
#define WL_BUF_REG_CONFIG_STOP_TX                          0x00000002
#define WL_BUF_REG_CONFIG_PROC_ALL_TRIGGERS                0x00000004
#define WL_BUF_REG_CONFIG_AGC_IQ_SEL_RFA                   0x00000010
#define WL_BUF_REG_CONFIG_AGC_IQ_SEL_RFB                   0x00000020
#define WL_BUF_REG_CONFIG_AGC_IQ_SEL_RFC                   0x00000040
#define WL_BUF_REG_CONFIG_AGC_IQ_SEL_RFD                   0x00000080
#define WL_BUF_REG_CONFIG_RSSI_CLK_SEL                     0x00000300
#define WL_BUF_REG_CONFIG_LOAD_TIMER_64                    0x00001000
#define WL_BUF_REG_CONFIG_RX_WORD_ORDER                    0x00010000
#define WL_BUF_REG_CONFIG_RX_BYTE_ORDER                    0x00020000
#define WL_BUF_REG_CONFIG_TX_WORD_ORDER                    0x00040000
#define WL_BUF_REG_CONFIG_TX_BYTE_ORDER                    0x00080000
#define WL_BUF_REG_CONFIG_COUNTER_DATA_SEL                 0x00100000
#define WL_BUF_REG_CONFIG_TX_RX_LOOPBACK_SEL               0x00200000
#define WL_BUF_REG_CONFIG_DEBUG_TX_OUTPUT_SEL              0x10000000
#define WL_BUF_REG_CONFIG_DEBUG_TX_BUF_SEL                 0xE0000000


#define WL_BUF_REG_CONFIG_AGC_IQ_SEL_RF_ALL                0x000000F0


// Masks for Status register
#define WL_BUF_REG_STATUS_TX_RUNNING                       0x0000000F
#define WL_BUF_REG_STATUS_TX_RUNNING_RF_A                  0x00000001
#define WL_BUF_REG_STATUS_TX_RUNNING_RF_B                  0x00000002
#define WL_BUF_REG_STATUS_TX_RUNNING_RF_C                  0x00000004
#define WL_BUF_REG_STATUS_TX_RUNNING_RF_D                  0x00000008
#define WL_BUF_REG_STATUS_RX_RUNNING                       0x00000F00
#define WL_BUF_REG_STATUS_RX_RUNNING_RF_A                  0x00000100
#define WL_BUF_REG_STATUS_RX_RUNNING_RF_B                  0x00000200
#define WL_BUF_REG_STATUS_RX_RUNNING_RF_C                  0x00000400
#define WL_BUF_REG_STATUS_RX_RUNNING_RF_D                  0x00000800
#define WL_BUF_REG_STATUS_DRAM_INIT_DONE                   0x00010000


// Mask for RF output selection register
//   NOTE:  The defines for ANT_* in wl_interface.h should be used as values for the antenna arguments
#define RFA_BUF_SEL                                        0x00000003
#define RFB_BUF_SEL                                        0x00000300
#define RFC_BUF_SEL                                        0x00030000
#define RFD_BUF_SEL                                        0x03000000


// Masks for RF enable registers
#define RF_SEL_A                                           0x00000001
#define RF_SEL_B                                           0x00000002
#define RF_SEL_C                                           0x00000004
#define RF_SEL_D                                           0x00000008

#if WARPLAB_CONFIG_4RF
    #define NUM_RF_INF                                     4
    #define RF_SEL_ALL                                     0x0000000F

#else
    #define NUM_RF_INF                                     2
    #define RF_SEL_ALL                                     0x00000003
#endif

// Buffer state variables
#define BUF_STATE_STANDBY                                  0
#define BUF_STATE_RX                                       1
#define BUF_STATE_TX                                       2

// Masks for interrupt status register
#define WL_BUF_INT_ALL                                     0x00000003
#define RF_RX_IQ_RSSI_ERROR                                0x01000000
#define RF_TX_IQ_ERROR                                     0x01000000

#define RF_RX_IQ_RSSI_ERROR_CLR                            0x00000001
#define RF_TX_IQ_ERROR_CLR                                 0x00000100


// Masks for transfer calculations
//   Currently, these are defined as the BRAM size / 2 (ie we have a "ping" and "pong" buffer for storage)
//
//   NOTE:  We looked at
//
//
#define WL_BUF_RX_TRANSFER_THRESHOLD_SAMPLES               0x00004000
#define WL_BUF_RX_TRANSFER_THRESHOLD_BYTES                 0x00010000
#define WL_BUF_RX_TRANSFER_BYTE_ALIGNMENT_MASK             0xFFFF0000

#define WL_BUF_TX_TRANSFER_THRESHOLD_SAMPLES               0x00004000
#define WL_BUF_TX_TRANSFER_THRESHOLD_BYTES                 0x00010000
#define WL_BUF_TX_TRANSFER_BYTE_ALIGNMENT_MASK             0xFFFF0000


// Masks for RX / TX sample length calculations
//    NOTE:  This is based on the TX/RX_TRANSFER_THRESHOLD
//
#define WL_BUF_RX_SAMPLE_ALIGNMENT_MASK                    0xFFFFC000
#define WL_BUF_TX_SAMPLE_ALIGNMENT_MASK                    0xFFFFC000


// Defines for TX IQ status register
#define WL_BUF_TX_IQ_STATUS_WR_DONE                        0x00000001

// Defines for TX/RX counter reset
#define WL_BUF_TXRX_COUNTER_RESET_TX_RFA                   0x00000001
#define WL_BUF_TXRX_COUNTER_RESET_TX_RFB                   0x00000002
#define WL_BUF_TXRX_COUNTER_RESET_TX_RFC                   0x00000004
#define WL_BUF_TXRX_COUNTER_RESET_TX_RFD                   0x00000008
#define WL_BUF_TXRX_COUNTER_RESET_RX_RFA                   0x00000100
#define WL_BUF_TXRX_COUNTER_RESET_RX_RFB                   0x00000200
#define WL_BUF_TXRX_COUNTER_RESET_RX_RFC                   0x00000400
#define WL_BUF_TXRX_COUNTER_RESET_RX_RFD                   0x00000800

#define WL_BUF_TXRX_COUNTER_RESET_TXRX_ALL                 0x00000F0F



// Baseband Macros
#define wl_get_design_ver()                                XIo_In32(WL_BUF_REG_DESIGN_VER)

#define wl_bb_get_buffer_sizes()                           XIo_In32(WL_BUF_REG_BUF_SIZES)
#define wl_bb_get_rx_buffer_size()                        (XIo_In32(WL_BUF_REG_BUF_SIZES) & 0x0000FFFF)
#define wl_bb_get_tx_buffer_size()                       ((XIo_In32(WL_BUF_REG_BUF_SIZES) & 0xFFFF0000) >> 16)

#define wl_bb_get_raw_status()                            (XIo_In32(WL_BUF_REG_STATUS))
#define wl_bb_get_tx_status()                             (XIo_In32(WL_BUF_REG_STATUS) & WL_BUF_REG_STATUS_TX_RUNNING)
#define wl_bb_get_rx_status()                            ((XIo_In32(WL_BUF_REG_STATUS) & WL_BUF_REG_STATUS_RX_RUNNING) >> 8)

#define wl_bb_get_config()                                 XIo_In32(WL_BUF_REG_CONFIG)
#define wl_bb_set_config(mask)                             XIo_Out32(WL_BUF_REG_CONFIG, (XIo_In32(WL_BUF_REG_CONFIG) |  (mask)))
#define wl_bb_clear_config(mask)                           XIo_Out32(WL_BUF_REG_CONFIG, (XIo_In32(WL_BUF_REG_CONFIG) & ~(mask)))

#define wl_bb_set_rssi_clk(value)                          XIo_Out32(WL_BUF_REG_CONFIG, ((XIo_In32(WL_BUF_REG_CONFIG) & ~WL_BUF_REG_CONFIG_RSSI_CLK_SEL) | ((value << 8) & WL_BUF_REG_CONFIG_RSSI_CLK_SEL)))

#define wl_bb_get_tx_delay()                               XIo_In32(WL_BUF_REG_TX_DELAY)
#define wl_bb_set_tx_delay(delay)                          XIo_Out32(WL_BUF_REG_TX_DELAY, delay)

#define wl_bb_get_rx_length()                              XIo_In32(WL_BUF_REG_RX_LENGTH)
#define wl_bb_set_rx_length(length)                        XIo_Out32(WL_BUF_REG_RX_LENGTH, length)

#define wl_bb_get_tx_length()                              XIo_In32(WL_BUF_REG_TX_LENGTH)
#define wl_bb_set_tx_length(length)                        XIo_Out32(WL_BUF_REG_TX_LENGTH, length)

#define wl_bb_get_rf_buffer_sel()                          XIo_In32(WL_BUF_REG_RF_BUFFER_SEL)
#define wl_bb_set_rf_buffer_sel_rfa(ant)                   XIo_Out32(WL_BUF_REG_RF_BUFFER_SEL, ((XIo_In32(WL_BUF_REG_RF_BUFFER_SEL) & ~RFA_BUF_SEL) | ((ant      ) & RFA_BUF_SEL)))
#define wl_bb_set_rf_buffer_sel_rfb(ant)                   XIo_Out32(WL_BUF_REG_RF_BUFFER_SEL, ((XIo_In32(WL_BUF_REG_RF_BUFFER_SEL) & ~RFB_BUF_SEL) | ((ant <<  8) & RFB_BUF_SEL)))
#define wl_bb_set_rf_buffer_sel_rfc(ant)                   XIo_Out32(WL_BUF_REG_RF_BUFFER_SEL, ((XIo_In32(WL_BUF_REG_RF_BUFFER_SEL) & ~RFC_BUF_SEL) | ((ant << 16) & RFC_BUF_SEL)))
#define wl_bb_set_rf_buffer_sel_rfd(ant)                   XIo_Out32(WL_BUF_REG_RF_BUFFER_SEL, ((XIo_In32(WL_BUF_REG_RF_BUFFER_SEL) & ~RFD_BUF_SEL) | ((ant << 24) & RFD_BUF_SEL)))
#define wl_bb_set_rf_buffer_sel(rfa, rfb, rfc, rfd)        XIo_Out32(WL_BUF_REG_RF_BUFFER_SEL, (((rfa) & RFA_BUF_SEL) | ((rfb <<  8) & RFB_BUF_SEL) | ((rfc << 16) & RFC_BUF_SEL) | ((rfd << 24) & RFD_BUF_SEL)))

#define wl_bb_get_rx_buffer_en()                           XIo_In32(WL_BUF_REG_RX_BUF_EN)
#define wl_bb_set_rx_buffer_en(rf_sel)                     XIo_Out32(WL_BUF_REG_RX_BUF_EN, (XIo_In32(WL_BUF_REG_RX_BUF_EN) | rf_sel))
#define wl_bb_clear_rx_buffer_en(rf_sel)                   XIo_Out32(WL_BUF_REG_RX_BUF_EN, (XIo_In32(WL_BUF_REG_RX_BUF_EN) & ~(rf_sel)))

#define wl_bb_get_tx_buffer_en()                           XIo_In32(WL_BUF_REG_TX_BUF_EN)
#define wl_bb_set_tx_buffer_en(rf_sel)                     XIo_Out32(WL_BUF_REG_TX_BUF_EN, (XIo_In32(WL_BUF_REG_TX_BUF_EN) | rf_sel))
#define wl_bb_clear_tx_buffer_en(rf_sel)                   XIo_Out32(WL_BUF_REG_TX_BUF_EN, (XIo_In32(WL_BUF_REG_TX_BUF_EN) & ~(rf_sel)))

#define wl_bb_get_agc_done_addr()                          XIo_In32(WL_BUF_REG_AGC_DONE_ADDR)
#define wl_bb_get_rfa_agc_done_rssi()                     (XIo_In32(WL_BUF_REG_RF_AB_AGC_DONE_RSSI) & 0x000003FF)
#define wl_bb_get_rfb_agc_done_rssi()                    ((XIo_In32(WL_BUF_REG_RF_AB_AGC_DONE_RSSI) & 0x03FF0000) >> 16)
#define wl_bb_get_rfc_agc_done_rssi()                     (XIo_In32(WL_BUF_REG_RF_CD_AGC_DONE_RSSI) & 0x000003FF)
#define wl_bb_get_rfd_agc_done_rssi()                    ((XIo_In32(WL_BUF_REG_RF_CD_AGC_DONE_RSSI) & 0x03FF0000) >> 16)

#define wl_bb_get_rf_rx_iq_buf_rd_byte_offset()            XIo_In32(WL_BUF_REG_RF_RX_IQ_BUF_RD_BYTE_OFFSET)
#define wl_bb_set_rf_rx_iq_buf_rd_byte_offset(offset)      XIo_Out32(WL_BUF_REG_RF_RX_IQ_BUF_RD_BYTE_OFFSET, offset)

#define wl_bb_get_rf_rx_iq_buf_wr_byte_offset()            XIo_In32(WL_BUF_REG_RF_RX_IQ_BUF_WR_BYTE_OFFSET_UPDATE)
#define wl_bb_set_rf_rx_iq_buf_wr_byte_offset(offset)      XIo_Out32(WL_BUF_REG_RF_RX_IQ_BUF_WR_BYTE_OFFSET, offset)

#define wl_bb_get_rf_rx_iq_threshold()                     XIo_In32(WL_BUF_REG_RF_RX_IQ_THRESHOLD)
#define wl_bb_set_rf_rx_iq_threshold(num_samples)          XIo_Out32(WL_BUF_REG_RF_RX_IQ_THRESHOLD, num_samples)

#define wl_bb_get_rf_rx_iq_buf_occupancy()                 XIo_In32(WL_BUF_REG_RF_RX_IQ_BUF_OCCUPANCY)

#define wl_bb_get_rf_tx_iq_buf_rd_byte_offset()            XIo_In32(WL_BUF_REG_RF_TX_IQ_BUF_RD_BYTE_OFFSET)

#define wl_bb_get_rf_tx_iq_buf_wr_byte_offset()            XIo_In32(WL_BUF_REG_RF_TX_IQ_BUF_WR_BYTE_OFFSET)
#define wl_bb_set_rf_tx_iq_buf_wr_byte_offset(offset)      XIo_Out32(WL_BUF_REG_RF_TX_IQ_BUF_WR_BYTE_OFFSET, offset)

#define wl_bb_get_rf_tx_iq_threshold()                     XIo_In32(WL_BUF_REG_RF_TX_IQ_THRESHOLD)
#define wl_bb_set_rf_tx_iq_threshold(num_samples)          XIo_Out32(WL_BUF_REG_RF_TX_IQ_THRESHOLD, num_samples)

#define wl_bb_get_rf_tx_iq_buf_occupancy()                 XIo_In32(WL_BUF_REG_RF_TX_IQ_BUF_OCCUPANCY)
#define wl_bb_get_rf_tx_iq_status()                        XIo_In32(WL_BUF_REG_RF_TX_IQ_STATUS)

#define wl_bb_get_rf_rx_iq_rssi_error()                  ((XIo_In32(WL_BUF_REG_INT_STATUS) & RF_RX_IQ_RSSI_ERROR) >> 24)
#define wl_bb_clear_rf_rx_iq_rssi_error()                  XIo_Out32(WL_BUF_REG_RF_ERROR_CLR, RF_RX_IQ_RSSI_ERROR_CLR)

#define wl_bb_get_rf_tx_iq_error()                       ((XIo_In32(WL_BUF_REG_INT_STATUS) & RF_TX_IQ_ERROR) >> 25)
#define wl_bb_clear_rf_tx_iq_error()                       XIo_Out32(WL_BUF_REG_RF_ERROR_CLR, RF_TX_IQ_ERROR_CLR)

#define wl_bb_get_int_status()                            (XIo_In32(WL_BUF_REG_INT_STATUS) & WL_BUF_INT_ALL)

#define wl_bb_get_rfa_tx_count()                           XIo_In32(WL_BUF_REG_RFA_TX_COUNTER)
#define wl_bb_get_rfb_tx_count()                           XIo_In32(WL_BUF_REG_RFB_TX_COUNTER)
#define wl_bb_get_rfc_tx_count()                           XIo_In32(WL_BUF_REG_RFC_TX_COUNTER)
#define wl_bb_get_rfd_tx_count()                           XIo_In32(WL_BUF_REG_RFD_TX_COUNTER)
#define wl_bb_get_rfa_rx_count()                           XIo_In32(WL_BUF_REG_RFA_RX_COUNTER)
#define wl_bb_get_rfb_rx_count()                           XIo_In32(WL_BUF_REG_RFB_RX_COUNTER)
#define wl_bb_get_rfc_rx_count()                           XIo_In32(WL_BUF_REG_RFC_RX_COUNTER)
#define wl_bb_get_rfd_rx_count()                           XIo_In32(WL_BUF_REG_RFD_RX_COUNTER)

#define wl_bb_set_txrx_counter_reset(rf)                   XIo_Out32(WL_BUF_REG_TXRX_COUNTER_RESET, (rf & WL_BUF_TXRX_COUNTER_RESET_TXRX_ALL))
#define wl_bb_clear_txrx_counter_reset()                   XIo_Out32(WL_BUF_REG_TXRX_COUNTER_RESET, 0)



// Macros for other values from the buffers core
#define wl_get_dram_init_done()                          ((XIo_In32(WL_BUF_REG_STATUS) & WL_BUF_REG_STATUS_DRAM_INIT_DONE) >> 16)

#define wl_get_timer_64_MSB()                              XIo_In32(WL_TIMER_64_MSB)
#define wl_get_timer_64_LSB()                              XIo_In32(WL_TIMER_64_LSB)



// ****************************************************************************
// AGC Defines
//
#define AGC_A                                         0x10000000
#define AGC_B                                         0x20000000
#define AGC_C                                         0x40000000
#define AGC_D                                         0x80000000


// AGC Register definitions
#define WL_AGC_REG_RESET                              XPAR_WARPLAB_AGC_MEMMAP_RESET
#define WL_AGC_REG_TIMING_AGC                         XPAR_WARPLAB_AGC_MEMMAP_TIMING_AGC
#define WL_AGC_REG_TIMING_DCO                         XPAR_WARPLAB_AGC_MEMMAP_TIMING_DCO
#define WL_AGC_REG_TARGET                             XPAR_WARPLAB_AGC_MEMMAP_TARGET
#define WL_AGC_REG_CONFIG                             XPAR_WARPLAB_AGC_MEMMAP_CONFIG
#define WL_AGC_REG_RSSI_PWR_CALIB                     XPAR_WARPLAB_AGC_MEMMAP_RSSI_PWR_CALIB
#define WL_AGC_REG_IIR_COEF_B0                        XPAR_WARPLAB_AGC_MEMMAP_IIR_COEF_B0
#define WL_AGC_REG_IIR_COEF_A1                        XPAR_WARPLAB_AGC_MEMMAP_IIR_COEF_A1
#define WL_AGC_TIMING_RESET                           XPAR_WARPLAB_AGC_MEMMAP_TIMING_RESET
#define WL_AGC_SW_RESET                               XPAR_WARPLAB_AGC_MEMMAP_SW_RESET
#define WL_AGC_RESET_MODE                             XPAR_WARPLAB_AGC_MEMMAP_RESET_MODE
#define WL_AGC_RX_LENGTH                              XPAR_WARPLAB_AGC_MEMMAP_RX_LENGTH
#define WL_AGC_OVERRIDE                               XPAR_WARPLAB_AGC_MEMMAP_AGC_OVERRIDE

#define WL_AGC_GAINS                                  XPAR_WARPLAB_BUFFERS_MEMMAP_AGC_GAINS


#define WL_AGC_RESET_MODE_RESET_PER_RX_MASK           0x00000001

#define WL_AGC_RX_LENGTH_VALUE_MASK                   0xFFFFFFFF


// AGC gains reg:
//     [ 4: 0]: RF A BBG
//     [ 6: 5]: RF A RFG
//         [7]: RF A RXHP
//     [12: 8]: RF B BBG
//     [14:13]: RF B RFG
//        [15]: RF B RXHP
//     [20:16]: RF C BBG
//     [22:21]: RF C RFG
//        [23]: RF C RXHP
//     [28:24]: RF D BBG
//     [30:29]: RF D RFG
//        [31]: RF D RXHP
//
#define wl_get_agc_gains_raw()                        XIo_In32(WL_AGC_GAINS)

#define wl_get_agc_RFG(ant)                           (((ant==0) ? (Xil_In32(WL_AGC_GAINS) >>  5) : \
                                                        (ant==1) ? (Xil_In32(WL_AGC_GAINS) >> 13) : \
                                                        (ant==2) ? (Xil_In32(WL_AGC_GAINS) >> 21) : \
                                                                   (Xil_In32(WL_AGC_GAINS) >> 29)) & 0x3)

#define wl_get_agc_BBG(ant)                           (((ant==0) ? (Xil_In32(WL_AGC_GAINS) >>  0) : \
                                                        (ant==1) ? (Xil_In32(WL_AGC_GAINS) >>  8) : \
                                                        (ant==2) ? (Xil_In32(WL_AGC_GAINS) >> 16) : \
                                                                   (Xil_In32(WL_AGC_GAINS) >> 24)) & 0x1F)

#define wl_get_agc_RXHP(ant)                          (((ant==0) ? (Xil_In32(WL_AGC_GAINS) >>  7) : \
                                                        (ant==1) ? (Xil_In32(WL_AGC_GAINS) >> 15) : \
                                                        (ant==2) ? (Xil_In32(WL_AGC_GAINS) >> 23) : \
                                                                   (Xil_In32(WL_AGC_GAINS) >> 31)) & 0x1)


// AGC Macros
#define wl_agc_get_reset()                            XIo_In32(WL_AGC_REG_RESET)
#define wl_agc_set_reset(data)                        XIo_Out32(WL_AGC_REG_RESET, (data & 0x1))

#define wl_agc_get_AGC_timing()                       XIo_In32(WL_AGC_REG_TIMING_AGC)
#define wl_agc_set_AGC_timing(capt_rssi_1, capt_rssi_2, capt_v_db, agc_done) \
    Xil_Out32(WL_AGC_REG_TIMING_AGC, ((capt_rssi_1 & 0xFF)        | ((capt_rssi_2 & 0xFF) <<  8) | \
                                     ((capt_v_db   & 0xFF) << 16) | ((agc_done    & 0xFF) << 24)))

#define wl_agc_get_DCO_timing()                       XIo_In32(WL_AGC_REG_TIMING_DCO)
#define wl_agc_set_DCO_timing(start_dco, en_iir_filt) \
    Xil_Out32(WL_AGC_REG_TIMING_DCO, ((start_dco & 0xFF) | ((en_iir_filt & 0xFF) << 8)))

#define wl_agc_get_target()                           XIo_In32(WL_AGC_REG_TARGET)
#define wl_agc_set_target(target_pwr)                 Xil_Out32(WL_AGC_REG_TARGET, (target_pwr & 0x3F))

#define wl_agc_get_config()                           XIo_In32(WL_AGC_REG_CONFIG)
#define wl_agc_set_config_all(thresh32, thresh21, avg_len, v_db_adj, init_g_bb) \
    Xil_Out32(WL_AGC_REG_CONFIG, (((thresh32  & 0xFF) <<  0) | \
                                  ((thresh21  & 0xFF) <<  8) | \
                                  ((avg_len   & 0x03) << 16) | \
                                  ((v_db_adj  & 0x3F) << 18) | \
                                  ((init_g_bb & 0x1F) << 24)))

#define wl_agc_set_config(avg_len, v_db_adj, init_g_bb) \
    Xil_Out32(WL_AGC_REG_CONFIG, ((XIo_In32(WL_AGC_REG_CONFIG) & 0x0000FFFF) | \
                                  ((avg_len   & 0x03) << 16) | \
                                  ((v_db_adj  & 0x3F) << 18) | \
                                  ((init_g_bb & 0x1F) << 24)))

#define wl_agc_set_config_thresh(thresh32, thresh21) \
    Xil_Out32(WL_AGC_REG_CONFIG, ((XIo_In32(WL_AGC_REG_CONFIG) & 0xFFFF0000) | \
                                  ((thresh32  & 0xFF) <<  0) | \
                                  ((thresh21  & 0xFF) <<  8)))

#define wl_agc_get_RSSI_pwr_calib()                   XIo_In32(WL_AGC_REG_RSSI_PWR_CALIB)
#define wl_agc_set_RSSI_pwr_calib(g3, g2, g1)         Xil_Out32(WL_AGC_REG_RSSI_PWR_CALIB, ((g3 & 0xFF) | ((g2 & 0xFF)<<8) | ((g1 & 0xFF)<<16)))

#define wl_agc_get_reset_timing()                     XIo_In32(WL_AGC_TIMING_RESET)
#define wl_agc_set_reset_timing(rxhp, g_rf, g_bb)     Xil_Out32(WL_AGC_TIMING_RESET, ((rxhp & 0xFF) | ((g_rf & 0xFF)<<8) | ( (g_bb & 0xFF)<<16)))

#define wl_agc_get_rx_length()                        XIo_In32(WL_AGC_RX_LENGTH)
#define wl_agc_set_rx_length(data)                    XIo_Out32(WL_AGC_RX_LENGTH, data)

#define wl_agc_get_reset_mode()                       XIo_In32(WL_AGC_RESET_MODE)
#define wl_agc_enable_reset_per_rx()                  XIo_Out32(WL_AGC_RESET_MODE, (XIo_In32(WL_AGC_RESET_MODE) |  WL_AGC_RESET_MODE_RESET_PER_RX_MASK))
#define wl_agc_disable_reset_per_rx()                 XIo_Out32(WL_AGC_RESET_MODE, (XIo_In32(WL_AGC_RESET_MODE) & ~WL_AGC_RESET_MODE_RESET_PER_RX_MASK))

#define wl_agc_get_override()                         XIo_In32(WL_AGC_OVERRIDE)
#define wl_agc_set_override(data)                     Xil_Out32(WL_AGC_OVERRIDE, data)

#define wl_agc_get_iir_coef_a1()                      XIo_In32(WL_AGC_REG_IIR_COEF_A1)
#define wl_agc_set_iir_coef_a1(data)                  Xil_Out32(WL_AGC_REG_IIR_COEF_A1, data)

#define wl_agc_get_iir_coef_b0()                      XIo_In32(WL_AGC_REG_IIR_COEF_B0)
#define wl_agc_set_iir_coef_b0(data)                  Xil_Out32(WL_AGC_REG_IIR_COEF_B0, data)



/*********************** Global Structure Definitions ************************/

typedef u32 wl_samp;

// Common sample header flags between Read IQ / Write IQ
#define SAMPLE_HDR_FLAG_IQ_ERROR                           0x01
#define SAMPLE_HDR_FLAG_IQ_NOT_READY                       0x02


// Write IQ sample header flags
#define SAMPLE_HDR_FLAG_CHKSUM_RESET                       0x10
#define SAMPLE_HDR_FLAG_LAST_WRITE                         0x20


// Sample header
typedef struct{
    u16 buff_sel;
    u8  flags;
    u8  sample_iq_id;
    u32 start_samp;
    u32 num_samp;
} wl_bb_samp_hdr;




/******************************** Functions **********************************/

int          baseband_init(u8 dram_present, u8 configure_buffers);
int          baseband_process_cmd(int socket_index, void * from, wl_cmd_resp * command, wl_cmd_resp * response);

u32          wl_bb_get_supported_tx_length();
u32          wl_bb_get_supported_rx_length();

u32          baseband_get_checksum();
u32          baseband_update_checksum(u16 newdata, u8 reset );

// AGC Functions
void         warplab_agc_init();
void         warplab_agc_enable_DCO(u32 enable);
void         warplab_agc_reset();
inline void  warplab_agc_setNoiseEstimate(short int noiseEst);
void         warplab_set_agc_rx_length(u32 num_samples);



/**********************************************************************************************************************/
/**
 * @brief WARP v3 Specific Functions
 *
 **********************************************************************************************************************/

#ifdef WARP_HW_VER_v3

/***************************** Include Files *********************************/
#include <xintc.h>

/*************************** Constant Definitions ****************************/

// **********************************************************************
// Memory defines for DDR sample buffers
//
//     Currently, the RX buffer must be 8x the size of the RSSI buffer.  In order to
// minimize the unused space, we are allocating the buffers in the ratio:
//
//                         2RF / 4RF (2GB DDR)
//     RX   - 8x    -->    512 / 256 MB
//     TX   - 7x    -->    448 / 224 MB
//     RSSI - 1x    -->     64 /  32 MB
//
// NOTE:  Buffers must be allocated on temporary buffer size boundaries (ie WARPLAB_IQ_TX_BUF_SIZE)
// NOTE:  These values will not be use if DRAM is not available.  Instead, the buffers will default
//     back to WARPLab 7.4.0 sizes
//

#if 1

// To make it easier to define the buffers, we should allocate the space for the buffers in chunks
//     For 4 RF interfaces:  32 MB / increment (ie 2^23 samples)
//     For 2 RF interfaces:  64 MB / increment (ie 2^24 samples)
//
#if WARPLAB_CONFIG_4RF
    #define WL_BUF_DEFAULT_CHUNK_SIZE                      (DDR_SIZE / 64)
#else
    #define WL_BUF_DEFAULT_CHUNK_SIZE                      (DDR_SIZE / 32)
#endif

// Define the maximum number of sample supported base on the "chunk" size
#define WL_BUF_DEFAULT_RX_MAX_SAMPLES                      (((8 * WL_BUF_DEFAULT_CHUNK_SIZE) >> 2) - 1)
#define WL_BUF_DEFAULT_TX_MAX_SAMPLES                      (((7 * WL_BUF_DEFAULT_CHUNK_SIZE) >> 2) - 1)

// Define RF A Buffer addresses / sizes
#define WL_BUF_DEFAULT_IQ_RX_BUF_A_ADDR                    (DRAM_BASEADDR + ( 0 * WL_BUF_DEFAULT_CHUNK_SIZE))
#define WL_BUF_DEFAULT_IQ_TX_BUF_A_ADDR                    (DRAM_BASEADDR + ( 8 * WL_BUF_DEFAULT_CHUNK_SIZE))
#define WL_BUF_DEFAULT_RSSI_BUF_A_ADDR                     (DRAM_BASEADDR + (15 * WL_BUF_DEFAULT_CHUNK_SIZE))
#define WL_BUF_DEFAULT_IQ_RX_BUF_A_SIZE                    (8 * WL_BUF_DEFAULT_CHUNK_SIZE)
#define WL_BUF_DEFAULT_IQ_TX_BUF_A_SIZE                    (7 * WL_BUF_DEFAULT_CHUNK_SIZE)
#define WL_BUF_DEFAULT_RSSI_BUF_A_SIZE                     (1 * WL_BUF_DEFAULT_CHUNK_SIZE)

// Define RF B Buffer addresses / sizes
#define WL_BUF_DEFAULT_IQ_RX_BUF_B_ADDR                    (DRAM_BASEADDR + (16 * WL_BUF_DEFAULT_CHUNK_SIZE))
#define WL_BUF_DEFAULT_IQ_TX_BUF_B_ADDR                    (DRAM_BASEADDR + (24 * WL_BUF_DEFAULT_CHUNK_SIZE))
#define WL_BUF_DEFAULT_RSSI_BUF_B_ADDR                     (DRAM_BASEADDR + (31 * WL_BUF_DEFAULT_CHUNK_SIZE))
#define WL_BUF_DEFAULT_IQ_RX_BUF_B_SIZE                    (8 * WL_BUF_DEFAULT_CHUNK_SIZE)
#define WL_BUF_DEFAULT_IQ_TX_BUF_B_SIZE                    (7 * WL_BUF_DEFAULT_CHUNK_SIZE)
#define WL_BUF_DEFAULT_RSSI_BUF_B_SIZE                     (1 * WL_BUF_DEFAULT_CHUNK_SIZE)

#if WARPLAB_CONFIG_4RF
    // Define RF C Buffer addresses / sizes
    #define WL_BUF_DEFAULT_IQ_RX_BUF_C_ADDR                (DRAM_BASEADDR + (32 * WL_BUF_DEFAULT_CHUNK_SIZE))
    #define WL_BUF_DEFAULT_IQ_TX_BUF_C_ADDR                (DRAM_BASEADDR + (40 * WL_BUF_DEFAULT_CHUNK_SIZE))
    #define WL_BUF_DEFAULT_RSSI_BUF_C_ADDR                 (DRAM_BASEADDR + (47 * WL_BUF_DEFAULT_CHUNK_SIZE))
    #define WL_BUF_DEFAULT_IQ_RX_BUF_C_SIZE                (8 * WL_BUF_DEFAULT_CHUNK_SIZE)
    #define WL_BUF_DEFAULT_IQ_TX_BUF_C_SIZE                (7 * WL_BUF_DEFAULT_CHUNK_SIZE)
    #define WL_BUF_DEFAULT_RSSI_BUF_C_SIZE                 (1 * WL_BUF_DEFAULT_CHUNK_SIZE)

    // Define RF D Buffer addresses / sizes
    #define WL_BUF_DEFAULT_IQ_RX_BUF_D_ADDR                (DRAM_BASEADDR + (48 * WL_BUF_DEFAULT_CHUNK_SIZE))
    #define WL_BUF_DEFAULT_IQ_TX_BUF_D_ADDR                (DRAM_BASEADDR + (56 * WL_BUF_DEFAULT_CHUNK_SIZE))
    #define WL_BUF_DEFAULT_RSSI_BUF_D_ADDR                 (DRAM_BASEADDR + (63 * WL_BUF_DEFAULT_CHUNK_SIZE))
    #define WL_BUF_DEFAULT_IQ_RX_BUF_D_SIZE                (8 * WL_BUF_DEFAULT_CHUNK_SIZE)
    #define WL_BUF_DEFAULT_IQ_TX_BUF_D_SIZE                (7 * WL_BUF_DEFAULT_CHUNK_SIZE)
    #define WL_BUF_DEFAULT_RSSI_BUF_D_SIZE                 (1 * WL_BUF_DEFAULT_CHUNK_SIZE)
#else
    #if WL_BUF_DEBUG_4RF_ON_2RF
        // In the case we want to debug the 4RF buffers on a 2RF design,
        //     map RFC -> RFA and RFD -> RFB

        // Define RF C Buffer addresses / sizes
        #define WL_BUF_DEFAULT_IQ_RX_BUF_C_ADDR            (DRAM_BASEADDR + ( 0 * WL_BUF_DEFAULT_CHUNK_SIZE))
        #define WL_BUF_DEFAULT_IQ_TX_BUF_C_ADDR            (DRAM_BASEADDR + ( 8 * WL_BUF_DEFAULT_CHUNK_SIZE))
        #define WL_BUF_DEFAULT_RSSI_BUF_C_ADDR             (DRAM_BASEADDR + (15 * WL_BUF_DEFAULT_CHUNK_SIZE))
        #define WL_BUF_DEFAULT_IQ_RX_BUF_C_SIZE            (8 * WL_BUF_DEFAULT_CHUNK_SIZE)
        #define WL_BUF_DEFAULT_IQ_TX_BUF_C_SIZE            (7 * WL_BUF_DEFAULT_CHUNK_SIZE)
        #define WL_BUF_DEFAULT_RSSI_BUF_C_SIZE             (1 * WL_BUF_DEFAULT_CHUNK_SIZE)

        // Define RF D Buffer addresses / sizes
        #define WL_BUF_DEFAULT_IQ_RX_BUF_D_ADDR            (DRAM_BASEADDR + (16 * WL_BUF_DEFAULT_CHUNK_SIZE))
        #define WL_BUF_DEFAULT_IQ_TX_BUF_D_ADDR            (DRAM_BASEADDR + (24 * WL_BUF_DEFAULT_CHUNK_SIZE))
        #define WL_BUF_DEFAULT_RSSI_BUF_D_ADDR             (DRAM_BASEADDR + (31 * WL_BUF_DEFAULT_CHUNK_SIZE))
        #define WL_BUF_DEFAULT_IQ_RX_BUF_D_SIZE            (8 * WL_BUF_DEFAULT_CHUNK_SIZE)
        #define WL_BUF_DEFAULT_IQ_TX_BUF_D_SIZE            (7 * WL_BUF_DEFAULT_CHUNK_SIZE)
        #define WL_BUF_DEFAULT_RSSI_BUF_D_SIZE             (1 * WL_BUF_DEFAULT_CHUNK_SIZE)
    #else
        // Define RF C Buffer addresses / sizes
        #define WL_BUF_DEFAULT_IQ_RX_BUF_C_ADDR            0
        #define WL_BUF_DEFAULT_IQ_TX_BUF_C_ADDR            0
        #define WL_BUF_DEFAULT_RSSI_BUF_C_ADDR             0
        #define WL_BUF_DEFAULT_IQ_RX_BUF_C_SIZE            0
        #define WL_BUF_DEFAULT_IQ_TX_BUF_C_SIZE            0
        #define WL_BUF_DEFAULT_RSSI_BUF_C_SIZE             0

        // Define RF D Buffer addresses / sizes
        #define WL_BUF_DEFAULT_IQ_RX_BUF_D_ADDR            0
        #define WL_BUF_DEFAULT_IQ_TX_BUF_D_ADDR            0
        #define WL_BUF_DEFAULT_RSSI_BUF_D_ADDR             0
        #define WL_BUF_DEFAULT_IQ_RX_BUF_D_SIZE            0
        #define WL_BUF_DEFAULT_IQ_TX_BUF_D_SIZE            0
        #define WL_BUF_DEFAULT_RSSI_BUF_D_SIZE             0
    #endif
#endif

#endif  // #if 1


// **********************************************************************
// Alternate Example Memory defines for DDR sample buffers
//
//     Currently, the RX buffer must be 8x the size of the RSSI buffer.  In order to
// minimize the unused space, we are allocating the buffers in the ratio:
//
//     (2GB DDR)           Max Tx  / Max RX  /   1RF   /   2RF   /   4RF
//     RX   - 8x    -->     128 kB / 1820 MB / 1024 MB /  512 MB /  256 MB
//     TX   - 7x    -->    2048 MB /  128 kB /  896 MB /  448 MB /  224 MB
//     RSSI - 1x    -->      16 kB /  228 MB /  128 MB /   64 MB /   32 MB
//
// NOTE:  Buffers must be allocated on temporary buffer size boundaries (ie WARPLAB_IQ_TX_BUF_SIZE)
// NOTE:  These values will not be use if DRAM is not available.  Instead, the buffers will default
//     back to WARPLab 7.4.0 sizes
//

#if 0

//------------------------------------------------------------------------
// 1RF Case
//------------------------------------------------------------------------

#define WL_BUF_DEFAULT_CHUNK_SIZE                          (DDR_SIZE / 16)

// Define the maximum number of sample supported base on the "chunk" size
#define WL_BUF_DEFAULT_RX_MAX_SAMPLES                      (((8 * WL_BUF_DEFAULT_CHUNK_SIZE) >> 2) - 1)
#define WL_BUF_DEFAULT_TX_MAX_SAMPLES                      (((7 * WL_BUF_DEFAULT_CHUNK_SIZE) >> 2) - 1)

// Define RF A Buffer addresses / sizes
#define WL_BUF_DEFAULT_IQ_RX_BUF_A_ADDR                    (DRAM_BASEADDR + ( 0 * WL_BUF_DEFAULT_CHUNK_SIZE))
#define WL_BUF_DEFAULT_IQ_TX_BUF_A_ADDR                    (DRAM_BASEADDR + ( 8 * WL_BUF_DEFAULT_CHUNK_SIZE))
#define WL_BUF_DEFAULT_RSSI_BUF_A_ADDR                     (DRAM_BASEADDR + (15 * WL_BUF_DEFAULT_CHUNK_SIZE))
#define WL_BUF_DEFAULT_IQ_RX_BUF_A_SIZE                    (8 * WL_BUF_DEFAULT_CHUNK_SIZE)
#define WL_BUF_DEFAULT_IQ_TX_BUF_A_SIZE                    (7 * WL_BUF_DEFAULT_CHUNK_SIZE)
#define WL_BUF_DEFAULT_RSSI_BUF_A_SIZE                     (1 * WL_BUF_DEFAULT_CHUNK_SIZE)


#if WL_BUF_DEBUG_4RF_ON_2RF
    // In the case we want to debug the 4RF buffers on a 1RF design,
    //     map RFB -> RFA, RFC -> RFA and RFD -> RFA

    // Define RF B Buffer addresses / sizes
    #define WL_BUF_DEFAULT_IQ_RX_BUF_B_ADDR                WL_BUF_DEFAULT_IQ_RX_BUF_A_ADDR
    #define WL_BUF_DEFAULT_IQ_TX_BUF_B_ADDR                WL_BUF_DEFAULT_IQ_TX_BUF_A_ADDR
    #define WL_BUF_DEFAULT_RSSI_BUF_B_ADDR                 WL_BUF_DEFAULT_RSSI_BUF_A_ADDR
    #define WL_BUF_DEFAULT_IQ_RX_BUF_B_SIZE                WL_BUF_DEFAULT_IQ_RX_BUF_A_SIZE
    #define WL_BUF_DEFAULT_IQ_TX_BUF_B_SIZE                WL_BUF_DEFAULT_IQ_TX_BUF_A_SIZE
    #define WL_BUF_DEFAULT_RSSI_BUF_B_SIZE                 WL_BUF_DEFAULT_RSSI_BUF_A_SIZE

    // Define RF C Buffer addresses / sizes
    #define WL_BUF_DEFAULT_IQ_RX_BUF_C_ADDR                WL_BUF_DEFAULT_IQ_RX_BUF_A_ADDR
    #define WL_BUF_DEFAULT_IQ_TX_BUF_C_ADDR                WL_BUF_DEFAULT_IQ_TX_BUF_A_ADDR
    #define WL_BUF_DEFAULT_RSSI_BUF_C_ADDR                 WL_BUF_DEFAULT_RSSI_BUF_A_ADDR
    #define WL_BUF_DEFAULT_IQ_RX_BUF_C_SIZE                WL_BUF_DEFAULT_IQ_RX_BUF_A_SIZE
    #define WL_BUF_DEFAULT_IQ_TX_BUF_C_SIZE                WL_BUF_DEFAULT_IQ_TX_BUF_A_SIZE
    #define WL_BUF_DEFAULT_RSSI_BUF_C_SIZE                 WL_BUF_DEFAULT_RSSI_BUF_A_SIZE

    // Define RF D Buffer addresses / sizes
    #define WL_BUF_DEFAULT_IQ_RX_BUF_D_ADDR                WL_BUF_DEFAULT_IQ_RX_BUF_A_ADDR
    #define WL_BUF_DEFAULT_IQ_TX_BUF_D_ADDR                WL_BUF_DEFAULT_IQ_TX_BUF_A_ADDR
    #define WL_BUF_DEFAULT_RSSI_BUF_D_ADDR                 WL_BUF_DEFAULT_RSSI_BUF_A_ADDR
    #define WL_BUF_DEFAULT_IQ_RX_BUF_D_SIZE                WL_BUF_DEFAULT_IQ_RX_BUF_A_SIZE
    #define WL_BUF_DEFAULT_IQ_TX_BUF_D_SIZE                WL_BUF_DEFAULT_IQ_TX_BUF_A_SIZE
    #define WL_BUF_DEFAULT_RSSI_BUF_D_SIZE                 WL_BUF_DEFAULT_RSSI_BUF_A_SIZE
#else
    // Define RF B Buffer addresses / sizes
    #define WL_BUF_DEFAULT_IQ_RX_BUF_B_ADDR                0
    #define WL_BUF_DEFAULT_IQ_TX_BUF_B_ADDR                0
    #define WL_BUF_DEFAULT_RSSI_BUF_B_ADDR                 0
    #define WL_BUF_DEFAULT_IQ_RX_BUF_B_SIZE                0
    #define WL_BUF_DEFAULT_IQ_TX_BUF_B_SIZE                0
    #define WL_BUF_DEFAULT_RSSI_BUF_B_SIZE                 0

    // Define RF C Buffer addresses / sizes
    #define WL_BUF_DEFAULT_IQ_RX_BUF_C_ADDR                0
    #define WL_BUF_DEFAULT_IQ_TX_BUF_C_ADDR                0
    #define WL_BUF_DEFAULT_RSSI_BUF_C_ADDR                 0
    #define WL_BUF_DEFAULT_IQ_RX_BUF_C_SIZE                0
    #define WL_BUF_DEFAULT_IQ_TX_BUF_C_SIZE                0
    #define WL_BUF_DEFAULT_RSSI_BUF_C_SIZE                 0

    // Define RF D Buffer addresses / sizes
    #define WL_BUF_DEFAULT_IQ_RX_BUF_D_ADDR                0
    #define WL_BUF_DEFAULT_IQ_TX_BUF_D_ADDR                0
    #define WL_BUF_DEFAULT_RSSI_BUF_D_ADDR                 0
    #define WL_BUF_DEFAULT_IQ_RX_BUF_D_SIZE                0
    #define WL_BUF_DEFAULT_IQ_TX_BUF_D_SIZE                0
    #define WL_BUF_DEFAULT_RSSI_BUF_D_SIZE                 0
#endif

#endif  // #if 0



// **********************************************************************
// Memory defines for BRAM sample buffers
//     - Renamed from XPAR* here for easier maintenance
//
#define WARPLAB_IQ_RX_BUF_A                                XPAR_RFA_IQ_RX_BUFFER_CTRL_S_AXI_BASEADDR
#define WARPLAB_IQ_TX_BUF_A                                XPAR_RFA_IQ_TX_BUFFER_CTRL_S_AXI_BASEADDR
#define WARPLAB_RSSI_BUF_A                                 XPAR_RFA_RSSI_BUFFER_CTRL_S_AXI_BASEADDR

#define WARPLAB_IQ_RX_BUF_A_SIZE                          (XPAR_RFA_IQ_RX_BUFFER_CTRL_S_AXI_HIGHADDR - XPAR_RFA_IQ_RX_BUFFER_CTRL_S_AXI_BASEADDR + 1)
#define WARPLAB_IQ_TX_BUF_A_SIZE                          (XPAR_RFA_IQ_TX_BUFFER_CTRL_S_AXI_HIGHADDR - XPAR_RFA_IQ_TX_BUFFER_CTRL_S_AXI_BASEADDR + 1)
#define WARPLAB_RSSI_BUF_A_SIZE                           (XPAR_RFA_RSSI_BUFFER_CTRL_S_AXI_HIGHADDR  - XPAR_RFA_RSSI_BUFFER_CTRL_S_AXI_BASEADDR + 1)

#define WARPLAB_IQ_RX_BUF_B                                XPAR_RFB_IQ_RX_BUFFER_CTRL_S_AXI_BASEADDR
#define WARPLAB_IQ_TX_BUF_B                                XPAR_RFB_IQ_TX_BUFFER_CTRL_S_AXI_BASEADDR
#define WARPLAB_RSSI_BUF_B                                 XPAR_RFB_RSSI_BUFFER_CTRL_S_AXI_BASEADDR

#define WARPLAB_IQ_RX_BUF_B_SIZE                          (XPAR_RFB_IQ_RX_BUFFER_CTRL_S_AXI_HIGHADDR - XPAR_RFB_IQ_RX_BUFFER_CTRL_S_AXI_BASEADDR + 1)
#define WARPLAB_IQ_TX_BUF_B_SIZE                          (XPAR_RFB_IQ_TX_BUFFER_CTRL_S_AXI_HIGHADDR - XPAR_RFB_IQ_TX_BUFFER_CTRL_S_AXI_BASEADDR + 1)
#define WARPLAB_RSSI_BUF_B_SIZE                           (XPAR_RFB_RSSI_BUFFER_CTRL_S_AXI_HIGHADDR  - XPAR_RFB_RSSI_BUFFER_CTRL_S_AXI_BASEADDR + 1)


// NOTE:  Since the 2RF design does not contain memories for the RFC and RFD buffers
//   we will map RFC and RFD to 0 and set the buffer size to 0 so there are no issues
//   since there is no physical memory allocated (unlike previous revisions
//   where the memory was still allocated even though it wasn't used).
//
#if WARPLAB_CONFIG_4RF
    #define WARPLAB_IQ_RX_BUF_C                            XPAR_RFC_IQ_RX_BUFFER_CTRL_S_AXI_BASEADDR
    #define WARPLAB_IQ_TX_BUF_C                            XPAR_RFC_IQ_TX_BUFFER_CTRL_S_AXI_BASEADDR
    #define WARPLAB_RSSI_BUF_C                             XPAR_RFC_RSSI_BUFFER_CTRL_S_AXI_BASEADDR

    #define WARPLAB_IQ_RX_BUF_C_SIZE                      (XPAR_RFC_IQ_RX_BUFFER_CTRL_S_AXI_HIGHADDR - XPAR_RFC_IQ_RX_BUFFER_CTRL_S_AXI_BASEADDR + 1)
    #define WARPLAB_IQ_TX_BUF_C_SIZE                      (XPAR_RFC_IQ_TX_BUFFER_CTRL_S_AXI_HIGHADDR - XPAR_RFC_IQ_TX_BUFFER_CTRL_S_AXI_BASEADDR + 1)
    #define WARPLAB_RSSI_BUF_C_SIZE                       (XPAR_RFC_RSSI_BUFFER_CTRL_S_AXI_HIGHADDR  - XPAR_RFC_RSSI_BUFFER_CTRL_S_AXI_BASEADDR + 1)

    #define WARPLAB_IQ_RX_BUF_D                            XPAR_RFD_IQ_RX_BUFFER_CTRL_S_AXI_BASEADDR
    #define WARPLAB_IQ_TX_BUF_D                            XPAR_RFD_IQ_TX_BUFFER_CTRL_S_AXI_BASEADDR
    #define WARPLAB_RSSI_BUF_D                             XPAR_RFD_RSSI_BUFFER_CTRL_S_AXI_BASEADDR

    #define WARPLAB_IQ_RX_BUF_D_SIZE                      (XPAR_RFD_IQ_RX_BUFFER_CTRL_S_AXI_HIGHADDR - XPAR_RFD_IQ_RX_BUFFER_CTRL_S_AXI_BASEADDR + 1)
    #define WARPLAB_IQ_TX_BUF_D_SIZE                      (XPAR_RFD_IQ_TX_BUFFER_CTRL_S_AXI_HIGHADDR - XPAR_RFD_IQ_TX_BUFFER_CTRL_S_AXI_BASEADDR + 1)
    #define WARPLAB_RSSI_BUF_D_SIZE                       (XPAR_RFD_RSSI_BUFFER_CTRL_S_AXI_HIGHADDR  - XPAR_RFD_RSSI_BUFFER_CTRL_S_AXI_BASEADDR + 1)
#else
    #if WL_BUF_DEBUG_4RF_ON_2RF
        // In the case we want to debug the 4RF buffers on a 2RF design,
        //     map RFC -> RFA and RFD -> RFB
        #define WARPLAB_IQ_RX_BUF_C                        WARPLAB_IQ_RX_BUF_A
        #define WARPLAB_IQ_TX_BUF_C                        WARPLAB_IQ_TX_BUF_A
        #define WARPLAB_RSSI_BUF_C                         WARPLAB_RSSI_BUF_A

        #define WARPLAB_IQ_RX_BUF_C_SIZE                   WARPLAB_IQ_RX_BUF_A_SIZE
        #define WARPLAB_IQ_TX_BUF_C_SIZE                   WARPLAB_IQ_TX_BUF_A_SIZE
        #define WARPLAB_RSSI_BUF_C_SIZE                    WARPLAB_RSSI_BUF_A_SIZE

        #define WARPLAB_IQ_RX_BUF_D                        WARPLAB_IQ_RX_BUF_B
        #define WARPLAB_IQ_TX_BUF_D                        WARPLAB_IQ_TX_BUF_B
        #define WARPLAB_RSSI_BUF_D                         WARPLAB_RSSI_BUF_B

        #define WARPLAB_IQ_RX_BUF_D_SIZE                   WARPLAB_IQ_RX_BUF_B_SIZE
        #define WARPLAB_IQ_TX_BUF_D_SIZE                   WARPLAB_IQ_TX_BUF_B_SIZE
        #define WARPLAB_RSSI_BUF_D_SIZE                    WARPLAB_RSSI_BUF_B_SIZE
    #else
        #define WARPLAB_IQ_RX_BUF_C                        0
        #define WARPLAB_IQ_TX_BUF_C                        0
        #define WARPLAB_RSSI_BUF_C                         0

        #define WARPLAB_IQ_RX_BUF_C_SIZE                   0
        #define WARPLAB_IQ_TX_BUF_C_SIZE                   0
        #define WARPLAB_RSSI_BUF_C_SIZE                    0

        #define WARPLAB_IQ_RX_BUF_D                        0
        #define WARPLAB_IQ_TX_BUF_D                        0
        #define WARPLAB_RSSI_BUF_D                         0

        #define WARPLAB_IQ_RX_BUF_D_SIZE                   0
        #define WARPLAB_IQ_TX_BUF_D_SIZE                   0
        #define WARPLAB_RSSI_BUF_D_SIZE                    0
    #endif
#endif



// **********************************************************************
// Defines for WARPLab Buffers Core
//     - Renamed from XPAR* here for easier maintenance
//

// Interupt ID
#define WL_BUF_RX_INTERRUPT_ID                             XPAR_INTC_0_W3_WARPLAB_BUFFERS_AXIW_0_RF_RX_IQ_RSSI_INT_VEC_ID   ///< XParameters rename of buffers core rx interrupt
#define WL_BUF_TX_INTERRUPT_ID                             XPAR_INTC_0_W3_WARPLAB_BUFFERS_AXIW_0_RF_TX_IQ_INT_VEC_ID        ///< XParameters rename of buffers core tx interrupt




/*********************** Global Structure Definitions ************************/

/******************************** Functions **********************************/

int          wl_baseband_setup_interrupt(XIntc* intc);

#endif

#endif /* WL_BASEBAND_H_ */
