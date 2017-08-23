/** @file wl_common.c
 *  @brief WARPLab Framework (Common)
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
#include <xil_io.h>
#include <xio.h>

//#include "stdlib.h"
//#include "ctype.h"
#include "string.h"
//#include "stdarg.h"
#include "stdio.h"

// Xilinx Peripheral includes
#include <xtmrctr.h>
#include <xgpio.h>

// WARPLab includes
#include "wl_common.h"
#include "wl_node.h"
#include "wl_baseband.h"


/*************************** Constant Definitions ****************************/

#define TMRCTR_DEVICE_ID                                   XPAR_TMRCTR_0_DEVICE_ID
#define TIMER_FREQ                                         XPAR_TMRCTR_0_CLOCK_FREQ_HZ
#define TIMER_COUNTER_0                                    0



/*********************** Global Variable Definitions *************************/

/*************************** Variable Definitions ****************************/

// Peripheral Instances
static XTmrCtr                    TimerCounter;                 ///< Instance of the Tmrctr device
static XGpio                      GPIO_debugpin;                ///< Instance of the GPIO device


#if _DEBUG_STORAGE_
u32                debug_storage[_DEBUG_STORAGE_SIZE_];
u32                storage_index = 0;
#endif



/*************************** Functions Prototypes ****************************/

/******************************** Functions **********************************/

/*****************************************************************************
 *
 * Debug Printing Functions
 *
 *****************************************************************************/
u8       wl_print_level           = DEFAULT_DEBUG_PRINT_LEVEL;
char   * print_type_node          = "NODE";
char   * print_type_transport     = "TRANSPORT";
char   * print_type_interface     = "IFC";
char   * print_type_baseband      = "BB";
char   * print_type_trigger       = "TRIG";
char   * print_type_user          = "USER";


void wl_print_header(u8 level, char * type, char* filename, u32 line) {
    char * basename = NULL;

    if (type != NULL) {
        xil_printf("%s", type);

        if ((level <= WL_PRINT_WARNING) || (wl_print_level == WL_PRINT_DEBUG)) {
            basename =  strrchr(filename, '/') ? strrchr(filename, '/') + 1 : filename;
        }

        if (wl_print_level == WL_PRINT_DEBUG) {
            xil_printf(" (%s:%d): ", basename, line);
        } else {
            xil_printf(": ");
        }

        switch(level) {
            case WL_PRINT_ERROR:
                xil_printf("ERROR (%s:%d): ", basename, line);
                increment_red_leds_one_hot();
            break;

            case WL_PRINT_WARNING:
                xil_printf("WARNING (%s:%d): ", basename, line);
            break;
        }
    }
}


void wl_print_mac_address(u8 level, u8 * mac_address) {
    u32 i;

    if (level <= wl_print_level) {
        xil_printf("%02x", mac_address[0]);

        for ( i = 1; i < ETH_ADDR_LEN; i++ ) {
            xil_printf(":%02x", mac_address[i]);
        }
    }
}


void wl_set_print_level(u8 level) {

    switch(level) {
        case WL_PRINT_NONE:
        case WL_PRINT_ERROR:
        case WL_PRINT_WARNING:
        case WL_PRINT_INFO:
        case WL_PRINT_DEBUG:
            wl_print_level = level;
        break;

        default:
            xil_printf("Unsupported print level.  Setting to WLAN_EXP_PRINT_ERROR.\n");
            wl_print_level = WL_PRINT_ERROR;
        break;
    }
}



/*****************************************************************************
 *
 * Hardware initialization functions
 *
 *****************************************************************************/

/********************************************************************
 * @brief Timer Initialization
 *
 * @param  None
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS             - Initialization was successful
 *                                 XST_DEVICE_IS_STARTED   - The device has already been started
 *                                 XST_DEVICE_NOT_FOUND    - The device doesn't exist
 *
 ********************************************************************/
int wl_timer_initialize(){
    int             status            = XST_SUCCESS;
    XTmrCtr        *TmrCtrInstancePtr = &TimerCounter;
    XTmrCtr_Config *TmrCtrConfigPtr;

    // Initialize the timer counter
    status = XTmrCtr_Initialize(TmrCtrInstancePtr, TMRCTR_DEVICE_ID);

    if (status == XST_DEVICE_IS_STARTED) {
        wl_printf(WL_PRINT_INFO, print_type_node, "Timer was already running; clear/init manually\n");

        TmrCtrConfigPtr = XTmrCtr_LookupConfig(TMRCTR_DEVICE_ID);

        TmrCtrInstancePtr->BaseAddress = TmrCtrConfigPtr->BaseAddress;
        TmrCtrInstancePtr->IsReady     = XIL_COMPONENT_IS_READY;

        XTmrCtr_Stop(TmrCtrInstancePtr, 0);
        XTmrCtr_Reset(TmrCtrInstancePtr, 0);

        status = XTmrCtr_Initialize(TmrCtrInstancePtr, TMRCTR_DEVICE_ID);
    }

    if (status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "XtmrCtr_Initialize failed with status %d\n", status);
    }

    // Set timer 0 to into a "count down" mode
    XTmrCtr_SetOptions(TmrCtrInstancePtr, 0, (XTC_DOWN_COUNT_OPTION));
    XTmrCtr_SetResetValue(TmrCtrInstancePtr, 1, 0);                       // Sets timer so issuing a "start" command begins at counter = 0

    return status;
}



/*****************************************************************************
 *
 * Common functions
 *
 *****************************************************************************/

/********************************************************************
 * Node Null Callbacks
 *
 * This function is part of the callback system for processing node commands.
 * If there are no additional node commands, then this will return appropriate values.
 *
 * @param   void * param     - Parameters for the callback
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS  - Command successful
 *
 ********************************************************************/
int wl_null_callback(void* param){
    wl_printf(WL_PRINT_INFO, print_type_node, "WL NULL callback\n");
    return XST_SUCCESS;
}



/********************************************************************
 * @brief Set/Clear debug GPIO pins
 *
 * @param   mask             - u8 Bit mask to set / clear GPIO pins on the debug header
 *
 * @return  None
 *
 ********************************************************************/
inline void wl_setDebugGPIO(u8 mask){
    XGpio_DiscreteSet(&GPIO_debugpin, 1, mask);
}

inline void wl_clearDebugGPIO(u8 mask){
    XGpio_DiscreteClear(&GPIO_debugpin, 1, mask);
}



/********************************************************************
 * @brief Mapping of hexadecimal values to the 7-segment display
 *
 * @param   hex_value        - u8 Hexadecimal value to be converted (between 0 and 15)
 *
 * @return  u8               - LED map value of the 7-segment display
 *
 ********************************************************************/
u8   sevenSegmentMap(u8 hex_value) {
    switch(hex_value) {
        case(0x0) : return 0x3F;
        case(0x1) : return 0x06;
        case(0x2) : return 0x5B;
        case(0x3) : return 0x4F;
        case(0x4) : return 0x66;
        case(0x5) : return 0x6D;
        case(0x6) : return 0x7D;
        case(0x7) : return 0x07;
        case(0x8) : return 0x7F;
        case(0x9) : return 0x6F;

        case(0xA) : return 0x77;
        case(0xB) : return 0x7C;
        case(0xC) : return 0x39;
        case(0xD) : return 0x5E;
        case(0xE) : return 0x79;
        case(0xF) : return 0x71;
        default   : return 0x00;
    }
}



/********************************************************************
 * @brief Pretty print a buffer of u8
 *
 * @param   buf              - Pointer to u8 buffer to be printed
 * @param   size             - Number of bytes to be printed
 *
 * @return  None
 *
 ********************************************************************/
void print_array_u8(u8 *buf, u32 size) {
    u32 i;
    for (i = 0; i < size; i++) {
        xil_printf("%2x ", buf[i]);
        if ( (((i + 1) % 16) == 0) && ((i + 1) != size) ) {
            xil_printf("\n");
        }
    }
    xil_printf("\n\n");
}


#if _MEASUREMENT_PRINT_
/********************************************************************
 * @brief Pretty print a buffer of u32 formatted for measurement purposes
 *
 * @param   buf              - Pointer to u32 buffer to be printed
 * @param   size             - Number of u32 words to be printed
 *
 * @return  None
 *
 ********************************************************************/
void print_array_u32(u32 *buf, u32 size) {
    u32 i;
    xil_printf("[");
    for (i = 0; i < size; i++) {
        xil_printf("0x%08x, ", buf[i]);
        if ( (((i + 1) % _MEASUREMENT_PRINT_WIDTH_ ) == 0) && ((i + 1) != size) ) {
            xil_printf("],\n[");
        }
    }
    xil_printf("]\n\n");
}


#else


/********************************************************************
 * @brief Pretty print a buffer of u32
 *
 * @param   buf              - Pointer to u32 buffer to be printed
 * @param   size             - Number of u32 words to be printed
 *
 * @return  None
 *
 ********************************************************************/
void print_array_u32(u32 *buf, u32 size) {
    u32 i;
    for (i = 0; i < size; i++) {
        xil_printf("0x%08x ", buf[i]);
        if ( (((i + 1) % 4) == 0) && ((i + 1) != size) ) {
            xil_printf("\n");
        }
    }
    xil_printf("\n\n");
}


#endif


/********************************************************************
 * @brief Get Microsecond Counter Timestamp
 *
 * The Reference Design includes a 64-bit counter that increments with
 * every microsecond. This function returns this value.
 *
 * @param   None
 *
 * @return  u64              - Current number of microseconds that have elapsed since the hardware has booted.
 *
 ********************************************************************/
u64 get_usec_timestamp(){
    u32 timestamp_high_u32;
    u32 timestamp_low_u32;
    u64 timestamp_u64;

    timestamp_high_u32 = wl_get_timer_64_MSB();
    timestamp_low_u32  = wl_get_timer_64_LSB();

    // Catch very rare race when 32-LSB of 64-bit value wraps between the two 32-bit reads
    if( (timestamp_high_u32 & 0x1) != (wl_get_timer_64_MSB() & 0x1) ) {
        // 32-LSB wrapped - start over
        timestamp_high_u32 = wl_get_timer_64_MSB();
        timestamp_low_u32  = wl_get_timer_64_LSB();
    }

    timestamp_u64      = (((u64)timestamp_high_u32) << 32) + ((u64)timestamp_low_u32);

    return timestamp_u64;
}



/**********************************************************************************************************************/
/**
 * @brief WARP v3 Specific Functions
 *
 **********************************************************************************************************************/

#ifdef WARP_HW_VER_v3

/***************************** Include Files *********************************/

#include "stdlib.h"
#include "ctype.h"
#include "stdarg.h"

#include <xil_exception.h>

#include <xintc.h>
#include <xaxicdma.h>
#include <xuartlite.h>

#ifdef XPAR_XSYSMON_NUM_INSTANCES
    #include <xsysmon_hw.h>
#endif


/*************************** Constant Definitions ****************************/

// Peripheral defines
#define DEBUG_GPIO_DEVICE_ID                               XPAR_AXI_GPIO_0_DEVICE_ID
#define CDMA_DEVICE_ID                                     XPAR_AXI_CDMA_0_DEVICE_ID
#define UARTLITE_DEVICE_ID                                 XPAR_UARTLITE_0_DEVICE_ID
#define INTC_DEVICE_ID                                     XPAR_INTC_0_DEVICE_ID

// UART defines
#define UARTLITE_INTERRUPT_ID                              XPAR_INTC_0_UARTLITE_0_VEC_ID
#define UART_BUFFER_SIZE                                   1                                 ///< UART is configured to read 1 byte at a time

// CDMA defines
#define CDMA_ALIGNMENT                                     0x10
#define CDMA_ALIGNMENT_MASK                                0xFFFFFFF0


/*********************** Global Variable Definitions *************************/

/*************************** Variable Definitions ****************************/

// Peripherals
XAxiCdma                          cdma_inst;                    ///< Instance of the CDMA device
static XIntc                      InterruptController;          ///< Interrupt Controller instance
static XUartLite                  UartLite;                     ///< UART Device instance

// UART interface
u8                                uart_rx_buffer[UART_BUFFER_SIZE];  ///< Buffer for received byte from UART
volatile wl_function_ptr_t        uart_callback;                     ///< User callback for UART reception

// Interrupt State
volatile static interrupt_state_t interrupt_state;


/*************************** Functions Prototypes ****************************/

void wl_uart_rx_handler(void* CallBackRef, unsigned int EventData);



/******************************** Functions **********************************/


/********************************************************************
 * @brief Debug GPIO Initialization
 *
 * @param  None
 *
 * @return None
 *
 ********************************************************************/
void wl_gpio_debug_initialize(){
    XGpio_Initialize(&GPIO_debugpin, DEBUG_GPIO_DEVICE_ID);
    XGpio_DiscreteClear(&GPIO_debugpin, 1, 0xFF);
}


/********************************************************************
 * @brief System Monitor Initialization
 *
 * @param  None
 *
 * @return None
 *
 ********************************************************************/
void wl_sysmon_initialize(){

#ifdef XPAR_XSYSMON_NUM_INSTANCES
    u32 RegValue;

    // Reset the device.
    XSysMon_WriteReg(SYSMON_BASEADDR, XSM_SRR_OFFSET, XSM_SRR_IPRST_MASK);

    // Disable the Channel Sequencer before configuring the Sequence registers.
    RegValue = XSysMon_ReadReg(SYSMON_BASEADDR, XSM_CFR1_OFFSET) & (~ XSM_CFR1_SEQ_VALID_MASK);
    XSysMon_WriteReg(SYSMON_BASEADDR, XSM_CFR1_OFFSET,    RegValue | XSM_CFR1_SEQ_SINGCHAN_MASK);

    // Setup the Averaging to be done for the channels in the Configuration 0 register as 16 samples
    RegValue = XSysMon_ReadReg(SYSMON_BASEADDR, XSM_CFR0_OFFSET) & (~XSM_CFR0_AVG_VALID_MASK);
    XSysMon_WriteReg(SYSMON_BASEADDR, XSM_CFR0_OFFSET, RegValue | XSM_CFR0_AVG16_MASK);

    // Enable the averaging on the following channels in the Sequencer registers:
    //   - On-chip Temperature
    //   - On-chip VCCAUX supply sensor
    XSysMon_WriteReg(SYSMON_BASEADDR, XSM_SEQ02_OFFSET, XSM_SEQ_CH_TEMP | XSM_SEQ_CH_VCCAUX);

    // Enable the following channels in the Sequencer registers:
    //   - On-chip Temperature
    //   - On-chip VCCAUX supply sensor
    XSysMon_WriteReg(SYSMON_BASEADDR, XSM_SEQ00_OFFSET, XSM_SEQ_CH_TEMP | XSM_SEQ_CH_VCCAUX);

    // Set the ADCCLK frequency equal to 1/32 of System clock for the System
    // Monitor/ADC in the Configuration Register 2.
    XSysMon_WriteReg(SYSMON_BASEADDR, XSM_CFR2_OFFSET, 32 << XSM_CFR2_CD_SHIFT);

    // Enable the Channel Sequencer in continuous sequencer cycling mode.
    RegValue = XSysMon_ReadReg(SYSMON_BASEADDR, XSM_CFR1_OFFSET) & (~ XSM_CFR1_SEQ_VALID_MASK);
    XSysMon_WriteReg(SYSMON_BASEADDR, XSM_CFR1_OFFSET,    RegValue | XSM_CFR1_SEQ_CONTINPASS_MASK);

    // Wait till the End of Sequence occurs
    XSysMon_ReadReg(SYSMON_BASEADDR, XSM_SR_OFFSET); /* Clear the old status */
    while (((XSysMon_ReadReg(SYSMON_BASEADDR, XSM_SR_OFFSET)) &
            XSM_SR_EOS_MASK) != XSM_SR_EOS_MASK);

#endif

}


/********************************************************************
 * @brief Central DMA Initialization
 *
 * @param  None
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS             - Initialization was successful
 *                                 XST_DEVICE_IS_STARTED   - The device has already been started
 *                                 XST_DEVICE_NOT_FOUND    - The device doesn't exist
 *
 ********************************************************************/
int wl_cdma_initialize(){
    int              status            = XST_SUCCESS;
    XAxiCdma_Config *cdma_cfg_ptr;

    cdma_cfg_ptr = XAxiCdma_LookupConfig(CDMA_DEVICE_ID);

    status = XAxiCdma_CfgInitialize(&cdma_inst, cdma_cfg_ptr, cdma_cfg_ptr->BaseAddress);

    if (status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "CDMA initialization failed with status: %d\n", status );
        return XST_FAILURE;
    }

    XAxiCdma_IntrDisable(&cdma_inst, XAXICDMA_XR_IRQ_ALL_MASK);

    return status;
}



/********************************************************************
 * @brief UART Initialization
 *
 * @param  None
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS             - Initialization was successful
 *                                 XST_DEVICE_IS_STARTED   - The device has already been started
 *                                 XST_DEVICE_NOT_FOUND    - The device doesn't exist
 *
 ********************************************************************/
int wl_uart_initialize(){
    int              status            = XST_SUCCESS;

    status = XUartLite_Initialize(&UartLite, UARTLITE_DEVICE_ID);

    if (status != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "UART initialization failed with status: %d\n", status);
        return XST_FAILURE;
    }

    uart_callback            = (wl_function_ptr_t)wl_null_callback;

    return status;
}


/********************************************************************
 * @brief Use CDMA to transfer data from source address to destination address
 *
 * @param   src_address      - Source address (u32) of the transfer
 * @param   dest_address     - Destination address (u32) of the transfer
 * @param   length           - Length of the transfer in bytes
 *
 * @return  None
 *
 * @note  The CDMA is 128 bits and contains no data re-alignment engine
 *   (limitation of the IP).  Therefore, we can only perform 16 byte aligned
 *   transfers without issue.  If the transfer was unaligned, we will issue
 *   a warning since this call can be in timing critical loops.
 *
 * @note  This function does not wait for a DMA transfer to complete once it
 *   has been issued.  This allows the CPU to pipeline instructions while the
 *   DMA is transferring data.  However, this function will wait for the DMA
 *   to be ready if it is currently in use.
 *
 ********************************************************************/
void wl_cdma_transfer(u32 src_address, u32 dest_address, u32 length){

    // This code assumes all transfers are 16 byte aligned

    // Check if there was an error in the previous transfer and reset the DMA
    if ( XAxiCdma_GetError(&cdma_inst) != 0x0 ) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "DMA transfer prior to %d bytes from 0x%08x to 0x%08x failed.\nResetting DMA ... \n\n", length, src_address, dest_address);
        XAxiCdma_Reset(&cdma_inst);
        while(!XAxiCdma_ResetIsDone(&cdma_inst)) {}
    }

    // Wait for the DMA to be ready before issuing a new transfer
    while(XAxiCdma_IsBusy(&cdma_inst)) {}
    XAxiCdma_SimpleTransfer(&cdma_inst, src_address, dest_address, length, NULL, NULL);

    // Issue a warning if the transfer was unaligned
    if (((src_address  & CDMA_ALIGNMENT_MASK) != src_address ) ||
        ((dest_address & CDMA_ALIGNMENT_MASK) != dest_address)) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "DMA transfer not %d byte aligned: %d bytes from 0x%08x to 0x%08x.\n", CDMA_ALIGNMENT, length, src_address, dest_address);
    }
}

int  wl_cdma_busy() {
    return XAxiCdma_IsBusy(&cdma_inst);
}


/********************************************************************
 * @brief Initialize WARPLab Interrupts
 *
 * This function initializes sets up the interrupt subsystem of WARPLab.
 *
 * @param   None
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS             - Initialization was successful
 *                                 XST_DEVICE_IS_STARTED   - The device has already been started
 *                                 XST_DEVICE_NOT_FOUND    - The device doesn't exist
 *                                 XST_FAILURE             - Initialization of an interrupt was not successful
 *
 ********************************************************************/
int wl_interrupt_init(){
    int Result;

    // Set interrupt state
    interrupt_state = INTERRUPTS_DISABLED;


    // ***************************************************
    // Initialize XIntc
    // ***************************************************
    Result = XIntc_Initialize(&InterruptController, INTC_DEVICE_ID);
    if (Result != XST_SUCCESS) {
        return Result;
    }

    // ***************************************************
    // Connect interrupt devices
    // ***************************************************
    Result = XIntc_Connect(&InterruptController, UARTLITE_INTERRUPT_ID, (XInterruptHandler)XUartLite_InterruptHandler, &UartLite);
    if (Result != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "Failed to connect XUartLite to XIntc\n");
        return Result;
    }
    XIntc_Enable(&InterruptController, UARTLITE_INTERRUPT_ID);
    XUartLite_SetRecvHandler(&UartLite, wl_uart_rx_handler, &UartLite);
    XUartLite_EnableInterrupt(&UartLite);


    Result = wl_baseband_setup_interrupt(&InterruptController);
    if (Result != XST_SUCCESS) {
        wl_printf(WL_PRINT_ERROR, print_type_node, "Failed to set up baseband interrupt\n");
        return XST_FAILURE;
    }

    // ***************************************************
    // Enable MicroBlaze exceptions
    // ***************************************************
    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_ExceptionHandler)XIntc_InterruptHandler, &InterruptController);
    Xil_ExceptionEnable();

    // Finish setting up any subsystems that were waiting on interrupts to be configured
    //    - None at this time

    return XST_SUCCESS;
}


/********************************************************************
 * @brief Restore the state of the interrupt controller
 *
 * This function will restore the state of the interrupt controller to the value
 * specified by the "new_interrupt_state" argument.  This behavior allows this
 * function to be used with wl_interrupt_stop() to wrap code that is not interrupt
 * safe and not worry about the current state of the interrupt controller.
 *
 * @param  new_interrupt_state    - State to return interrupts. Typically, this argument
 *                                  is the output of a previous call to wl_interrupt_stop()
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS  - Command successful
 *                                 XST_FAILURE  - Command not successful
 *
 ********************************************************************/
inline int wl_interrupt_restore_state(interrupt_state_t new_interrupt_state){
    interrupt_state = new_interrupt_state;

    // Enable the interrupts based on the new interrupt state
    if(interrupt_state == INTERRUPTS_ENABLED){
        if(InterruptController.IsReady && InterruptController.IsStarted == 0){
            return XIntc_Start(&InterruptController, XIN_REAL_MODE);
        } else {
            return XST_FAILURE;
        }
    } else {
        return XST_SUCCESS;
    }
}


/********************************************************************
 * @brief Stop the interrupt controller
 *
 * This function stops the interrupt controller, effectively pausing interrupts and returns
 * the current state of the interrupts (ie whether the interrupts were currently enabled or
 * disabled).  This can then be used along with wl_interrupt_restore_state() to wrap code
 * that is not interrupt-safe.
 *
 * @param   None
 *
 * @return  interrupt_state_t     - Current state of interrupts (when function was called):
 *                                    INTERRUPTS_ENABLED   - Interrupts were enabled
 *                                    INTERRUPTS_DISABLED  - Interrupts were disabled
 *
 * @note Interrupts that occur while the interrupt controller is off will be executed once
 *   it is turned back on. They will not be "lost" as the interrupt inputs to the controller
 *   will remain high.
 *
 ********************************************************************/
inline interrupt_state_t wl_interrupt_stop(){
    interrupt_state_t curr_state = interrupt_state;        // Save current interrupt state

    if(InterruptController.IsReady && InterruptController.IsStarted) XIntc_Stop(&InterruptController);
    interrupt_state = INTERRUPTS_DISABLED;                 // Set current interrupt state to "Disabled"

    return curr_state;
}


/********************************************************************
 * @brief UART Receive Interrupt Handler
 *
 * This function is the interrupt handler for UART receptions. It, in turn,
 * will execute a callback that the user has previously registered.
 *
 * @param   CallBackRef      - Argument supplied by the XUartLite driver. Unused in this application.
 * @param   EventData        - Argument supplied by the XUartLite driver. Unused in this application.
 *
 * @return None
 *
 * @see wl_set_uart_rx_callback()
 *
 ********************************************************************/
void wl_uart_rx_handler(void* CallBackRef, unsigned int EventData){
    XUartLite_Recv(&UartLite, uart_rx_buffer, UART_BUFFER_SIZE);
    uart_callback(uart_rx_buffer[0]);
}


/********************************************************************
 * @brief Set UART Reception Callback
 *
 * Tells the framework which function should be called when a byte is received from UART.
 *
 * @param   callback         - Pointer to callback function
 *
 * @return  None
 *
 ********************************************************************/
void wl_set_uart_rx_callback(wl_function_ptr_t callback){
    uart_callback = callback;
}


/********************************************************************
 * @brief Microsecond sleep counter
 *
 * @param   duration         - Duration in microseconds to sleep
 *
 * @return None
 *
 * @note   For WARP v3, you cannot use usleep until wl_timer_initialize() has been called
 *
 ********************************************************************/
void usleep(u32 duration){
    XTmrCtr    *TmrCtrInstancePtr = &TimerCounter;
    volatile u8 isExpired         = 0;

    XTmrCtr_SetResetValue(TmrCtrInstancePtr, 0, (duration * (TIMER_FREQ/1000000)));
    XTmrCtr_Start(TmrCtrInstancePtr, 0);

    while(isExpired != 1){
        isExpired = XTmrCtr_IsExpired(TmrCtrInstancePtr, 0);
    }

    XTmrCtr_Reset(TmrCtrInstancePtr, 0);
}


/********************************************************************
 * @brief Test Right Shift Operator
 *
 * This function tests the compiler right shift operator.  This is due to a bug in
 * the Xilinx 14.7 toolchain when the '-Os' flag is used during compilation.  Please
 * see:  http://warpproject.org/forums/viewtopic.php?id=2472 for more information.
 *
 * @param  None
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS  - Command successful
 *                                 XST_FAILURE  - Command not successful
 *
 ********************************************************************/
u32 right_shift_test = 0xFEDCBA98;

int microblaze_right_shift_test() {
    u8 val_3, val_2, val_1, val_0;

    u32 test_val   = right_shift_test;
    u8 *test_array = (u8 *)&right_shift_test;

    val_3 = (u8)((test_val & 0xFF000000) >> 24);
    val_2 = (u8)((test_val & 0x00FF0000) >> 16);
    val_1 = (u8)((test_val & 0x0000FF00) >>  8);
    val_0 = (u8)((test_val & 0x000000FF) >>  0);

    if ((val_3 != test_array[3]) || (val_2 != test_array[2]) || (val_1 != test_array[1]) || (val_0 != test_array[0])) {
        xil_printf("Right shift operator is not operating correctly in this toolchain.\n");
        xil_printf("Please use Xilinx 14.4 or an optimization level other than '-Os'\n");
        xil_printf("See http://warpproject.org/forums/viewtopic.php?id=2472 for more info.\n");
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}


/********************************************************************
 * @brief Test DDR3 SODIMM Memory Module
 *
 * This function tests the integrity of the DDR3 SODIMM module attached to the hardware
 * by performing various write and read tests. Note, this function will destroy contents
 * in DRAM, so it should only be called immediately after booting.
 *
 * @param  None
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS  - Command successful
 *                                 XST_FAILURE  - Command not successful
 *
 ********************************************************************/
int ddr_sodim_memory_test(){
    u32            status              = XST_SUCCESS;

    // Test num_test_points of the DDR evenly over the DDR range
    u32            num_test_points     = 8;
    u32            ddr_test_step       = (DDR_SIZE / num_test_points);

    // Delay to make sure data is written completely to the DDR
    u32            readback_delay_usec = 10000;

    // Memory variables
    volatile u8    i,j;

    volatile u8    test_u8;
    volatile u16   test_u16;
    volatile u32   test_u32;
    volatile u64   test_u64;

    volatile u8    readback_u8;
    volatile u16   readback_u16;
    volatile u32   readback_u32;
    volatile u64   readback_u64;

    volatile void* memory_ptr;

    for(i = 0; i < num_test_points; i++){
        memory_ptr = (void*)((u8*)DRAM_BASEADDR + (i*ddr_test_step));

        for(j = 0; j < 3; j++){
            // Test 1 byte offsets to make sure byte enables are all working
            test_u8  = rand() & 0xFF;
            test_u16 = rand() & 0xFFFF;
            test_u32 = rand() & 0xFFFFFFFF;
            test_u64 = (((u64)rand() & 0xFFFFFFFF) << 32) + ((u64)rand() & 0xFFFFFFFF);

            // u8 Test
            *((u8*)memory_ptr) = test_u8;
            usleep(readback_delay_usec);
            readback_u8 = *((u8*)memory_ptr);

            if(readback_u8 != test_u8){
                wl_printf(WL_PRINT_ERROR, print_type_node, "0x%08x: %02x != %02x\n", memory_ptr, readback_u8, test_u8);
                wl_printf(WL_PRINT_ERROR, print_type_node, "DRAM Failure: Addr: 0x%08x -- Unable to verify write of u8\n", memory_ptr);
                return XST_FAILURE;
            }

            // u16 Test
            *((u16*)memory_ptr) = test_u16;
            usleep(readback_delay_usec);
            readback_u16 = *((u16*)memory_ptr);

            if(readback_u16 != test_u16){
                wl_printf(WL_PRINT_ERROR, print_type_node, "0x%08x: %04x != %04x\n", memory_ptr, readback_u16, test_u16);
                wl_printf(WL_PRINT_ERROR, print_type_node, "DRAM Failure: Addr: 0x%08x -- Unable to verify write of u16\n", memory_ptr);
                return XST_FAILURE;
            }

            // u32 Test
            *((u32*)memory_ptr) = test_u32;
            usleep(readback_delay_usec);
            readback_u32 = *((u32*)memory_ptr);

            if(readback_u32 != test_u32){
                wl_printf(WL_PRINT_ERROR, print_type_node, "0x%08x: %08x != %08x\n", memory_ptr, readback_u32, test_u32);
                wl_printf(WL_PRINT_ERROR, print_type_node, "DRAM Failure: Addr: 0x%08x -- Unable to verify write of u32\n", memory_ptr);
                return XST_FAILURE;
            }

            // u64 Test
            *((u64*)memory_ptr) = test_u64;
            usleep(readback_delay_usec);
            readback_u64 = *((u64*)memory_ptr);

            if(readback_u64 != test_u64){
                wl_printf(WL_PRINT_ERROR, print_type_node, "0x%08x: %08x%08x != %08x%08x\n", memory_ptr,
                                                               (u32)(readback_u64 >> 32), (u32)readback_u64, (u32)(test_u64 >> 32), (u32)test_u64);
                wl_printf(WL_PRINT_ERROR, print_type_node, "DRAM Failure: Addr: 0x%08x -- Unable to verify write of u64\n", memory_ptr);
                return XST_FAILURE;
            }

            memory_ptr++;
        }
    }

    return status;
}


/********************************************************************
 * @brief Clear DDR3 SODIMM Memory Module
 *
 * This function will clear the contents of the DDR
 *
 * @param  verbose           - Print information on time to clear the DDR
 *
 * @return None
 *
 ********************************************************************/
void clear_ddr(u32 verbose) {
    u32 i;
    u64 num_step;
    u64 step_size;

    u64 start_time;
    u64 end_time;
    u32 processing_time;

    u32 start_address = DRAM_BASEADDR;
    u64 size          = DDR_SIZE;

    start_time = get_usec_timestamp();

#if 0
    // Implementation 1:
    //     Use CPU to bzero the entire DDR  (approx 84769092 usec)
    bzero((void *)start_address, size);
#endif

#if 1
    // Implementation 2:
    //     Use CPU to bzero the first block of DDR
    //     Use the DMA to zero out the rest of the DDR
    //
    // For num_step (all times approx):
    //     2^10  --> 1149215 usec
    //     2^11  --> 1107146 usec
    //     2^12  --> 1089062 usec
    //     2^13  --> 1082326 usec
    //     2^14  --> 1080768 usec  <-- Minimum
    //     2^15  --> 1093902 usec
    //     2^16  --> 1150738 usec
    //     2^17  --> 1265897 usec
    //
    num_step  = 1 << 14;
    step_size = size / num_step;

    bzero((void *)start_address, step_size);

    for (i = 1; i < num_step; i++) {
        wl_cdma_transfer(start_address, (start_address + (step_size * i)), step_size);
    }
#endif

    end_time   = get_usec_timestamp();
    processing_time = (end_time - start_time) & 0xFFFFFFFF;

    if (verbose == WL_VERBOSE) {
        wl_printf(WL_PRINT_NONE, NULL, "  Contents cleared in %d (usec)\n", processing_time);
    }
}

#endif



/**********************************************************************************************************************/
/**
 * @brief Debug Functions
 *
 **********************************************************************************************************************/


/*****************************************************************************
 *
 * Non-Invasive Debug functions
 *
 *****************************************************************************/

/********************************************************************
 * @brief Add value to debug storage
 *
 * @param   value            - Value (u32) to add to debug storage
 * @param   enable           - Should the value be added to the debug storage
 *
 * @return  None
 *
 ********************************************************************/
void add_to_debug_storage(u32 value, u32 enable) {
#if _DEBUG_STORAGE_
    if ((enable) && (storage_index < _DEBUG_STORAGE_SIZE_)) {
        debug_storage[storage_index++] = value;
    }
#endif
}


/********************************************************************
 * @brief Remove elements from the debug storage
 *
 * @param   num_elements     - Number of elements to remove from debug storage
 *
 * @return  None
 *
 ********************************************************************/
void remove_from_debug_storage(u32 num_elements, u32 enable) {
#if _DEBUG_STORAGE_
    if (enable) {
        if (num_elements > storage_index) {
            storage_index = 0;
        } else {
            storage_index -= num_elements;
        }
    }
#endif
}


/********************************************************************
 * @brief Reset the debug storage
 *
 * @param  None
 *
 * @return None
 *
 ********************************************************************/
void reset_debug_storage() {
#if _DEBUG_STORAGE_
    storage_index = 0;
    wl_printf(WL_PRINT_NONE, NULL, "Cleared Debug Storage.\n");
#endif
}


/********************************************************************
 * @brief Print the debug storage
 *
 * @param  None
 *
 * @return None
 *
 ********************************************************************/
void print_debug_storage() {
#if _DEBUG_STORAGE_
    print_array_u32(debug_storage, storage_index);
#else
    wl_printf(WL_PRINT_NONE, NULL, "Debug storage not enabled.\n");
#endif
}


