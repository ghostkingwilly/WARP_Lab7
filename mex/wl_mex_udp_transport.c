/******************************************************************************
*
* File   :	wl_mex_udp_transport.c
* Authors:	Chris Hunter (chunter [at] mangocomm.com)
*			Patrick Murphy (murphpo [at] mangocomm.com)
*           Erik Welsh (welsh [at] mangocomm.com)
* License:	Copyright 2013, Mango Communications. All rights reserved.
*			Distributed under the WARP license  (http://warpproject.org/license)
*
******************************************************************************/
/**
*
* @file wl_mex_udp_transport.c
*
* Implements the basic socket layer in MEX for the WARPLab Transport protocol 
*
* Compile command:
*
*   Windows (Visual C++):
*       mex -g -O wl_mex_udp_transport.c -lwsock32 -lKernel32 -DWIN32
*
*   MAC / Unix:
*       mex -g -O wl_mex_udp_transport.c
*
*
* MODIFICATION HISTORY:
*
* Ver   Who  Date     Changes
* ----- ---- -------- -------------------------------------------------------
* 1.00a ejw  7/15/13  Initial release
* 1.01a ejw  8/28/13  Updated to support MAC / Unix as well as Windows
*
*
******************************************************************************/



/***************************** Include Files *********************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include <matrix.h>
#include <mex.h>   


#ifdef WIN32

#include <Windows.h>
#include <WinBase.h>
#include <winsock.h>

#else

#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#endif



/*************************** Constant Definitions ****************************/

// Use to print debug message to the console
// #define _DEBUG_


// Version WARPLab MEX Transport Driver
//
// NOTE:  This version must match the required version in:
//     - wl_setup.m
//     - classes/wl_transport_eth_udp_mex.m
//     - classes/wl_transport_eth_udp_mex_bcast.m
//
#define WL_MEX_UDP_TRANSPORT_VERSION                       "1.0.4a"


// Windows / Unix compatibility
#ifdef WIN32

// Define printf for future compatibility
#define printf                                             mexPrintf
#define malloc(x)                                          mxMalloc(x)
#define free(x)                                            mxFree(x)
#define make_persistent(x)                                 mexMakeMemoryPersistent(x)
#define usleep(x)                                          Sleep((x)/1000)
#define close(x)                                           closesocket(x)
#define non_blocking_socket(x)                           { unsigned long optval = 1; ioctlsocket( x, FIONBIO, &optval ); }
#define wl_usleep(x)                                       wl_mex_udp_transport_usleep(x)
#define wl_timestamp                                       wl_mex_udp_transport_usec_timestamp()
#define SOCKET                                             SOCKET
#define get_last_error                                     WSAGetLastError()
#define EWOULDBLOCK                                        WSAEWOULDBLOCK
#define socklen_t                                          int

#else

// Define printf for future compatibility
#define printf                                             mexPrintf
#define malloc(x)                                          mxMalloc(x)
#define free(x)                                            mxFree(x)
#define make_persistent(x)                                 mexMakeMemoryPersistent(x)
#define usleep(x)                                          usleep(x)
#define close(x)                                           close(x)
#define non_blocking_socket(x)                             fcntl( x, F_SETFL, O_NONBLOCK )
#define wl_usleep(x)                                       usleep(x)
#define wl_timestamp                                       0
#define SOCKET                                             int
#define get_last_error                                     errno
#define INVALID_SOCKET                                     0xFFFFFFFF
#define SOCKET_ERROR                                       -1

#endif


// WL_MEX_UDP_TRANSPORT Commands
#define TRANSPORT_REVISION                                 0
#define TRANSPORT_INIT_SOCKET                              1
#define TRANSPORT_SET_SO_TIMEOUT                           2
#define TRANSPORT_SET_SEND_BUF_SIZE                        3
#define TRANSPORT_GET_SEND_BUF_SIZE                        4
#define TRANSPORT_SET_RCVD_BUF_SIZE                        5
#define TRANSPORT_GET_RCVD_BUF_SIZE                        6
#define TRANSPORT_CLOSE                                    7
#define TRANSPORT_SEND                                     8
#define TRANSPORT_RECEIVE                                  9
#define TRANSPORT_READ_IQ                                  10
#define TRANSPORT_READ_RSSI                                11
#define TRANSPORT_WRITE_IQ                                 12
#define TRANSPORT_WRITE_IQ_SET_PKT_WAIT_TIME               13
#define TRANSPORT_READ_IQ_SET_MAX_REQUEST_SIZE             14
#define TRANSPORT_SUPPRESS_IQ_WARNINGS                     15


// Maximum number of sockets that can be allocated
#define TRANSPORT_MAX_SOCKETS                              65

// Maximum size of a packet
#define TRANSPORT_MAX_PKT_LENGTH                           9050

// Socket state
#define TRANSPORT_SOCKET_FREE                              0
#define TRANSPORT_SOCKET_IN_USE                            1

// Transport defines
#define TRANSPORT_NUM_PENDING                              20
#define TRANSPORT_MIN_SEND_SIZE                            1000
#define TRANSPORT_SLEEP_TIME                               10000
#define TRANSPORT_FLAG_ROBUST                              0x0001
#define TRANSPORT_PADDING_SIZE                             2
#define TRANSPORT_TIMEOUT                                  10000000
#define TRANSPORT_MAX_RETRY                                50
#define TRANSPORT_NOT_READY_WAIT_TIME                      100000
#define TRANSPORT_NOT_READY_MAX_RETRY                      50
#define TRANSPORT_HDR_NODE_NOT_READY_FLAG                  0x8000

// Command defines
#define CMD_PARAM_SUCCESS                                  0x00000000
#define CMD_PARAM_ERROR                                    0xFF000000

// Sample defines
#define SAMPLE_RESPONSE_SUCCESS                            0x00000000
#define SAMPLE_RESPONSE_ERROR                              0xFFFFFFFF
#define SAMPLE_IQ_ERROR                                    0x01
#define SAMPLE_IQ_NOT_READY                                0x02
#define SAMPLE_CHECKSUM_FAILED                             0x03

#define SAMPLE_IQ_WAIT_TIME                                TRANSPORT_NOT_READY_WAIT_TIME
#define SAMPLE_IQ_MAX_RETRY                                TRANSPORT_NOT_READY_MAX_RETRY

#define SAMPLE_CHKSUM_RESET                                0x10
#define SAMPLE_CHKSUM_NOT_RESET                            0x00

#define SAMPLE_LAST_WRITE                                  0x20

// WARP HW version defines
#define TRANSPORT_WARP_HW_v2                               2
#define TRANSPORT_WARP_HW_v3                               3

// WARP Buffers defines
#define TRANSPORT_WARP_RF_BUFFER_MAX                       4

// IQ defines
#define IQ_DATA_TYPE_DOUBLE                                0
#define IQ_DATA_TYPE_SINGLE                                1
#define IQ_DATA_TYPE_INT16                                 2
#define IQ_DATA_TYPE_RAW                                   3

// RF defines
#define BUFFER_ID_RFA                                      0x00000001
#define BUFFER_ID_RFB                                      0x00000002
#define BUFFER_ID_RFC                                      0x00000004
#define BUFFER_ID_RFD                                      0x00000008

// Sequence number defines
#define SEQ_NUM_MATCH_IGNORE                               "ignore"
#define SEQ_NUM_MATCH_WARNING                              "warning"
#define SEQ_NUM_MATCH_ERROR                                "error"


/*************************** Variable Definitions ****************************/

// Define types for different size data
typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned int    uint32;

typedef char            int8;
typedef short           int16;
typedef int             int32;


// Data packet structure
typedef struct
{
    char              *buf;                 // Pointer to the data buffer
    int                length;              // Length of the buffer (buffer must be pre-allocated)
    int                offset;              // Offset of data to be sent or received
    struct sockaddr_in address;             // Address information of data to be sent / recevied    
} wl_trans_data_pkt;

// Socket structure
typedef struct
{
    SOCKET              handle;             // Handle to the socket
    int                 timeout;            // Timeout value
    int                 status;             // Status of the socket
    wl_trans_data_pkt  *packet;             // Pointer to a data_packet
    uint32              rx_buffer_size;     // Rx buffer size of the socket
    uint32              tx_buffer_size;     // Tx buffer size of the socket
} wl_trans_socket;

// WARPLAB Transport Header
typedef struct
{
    uint16             padding;        // Padding for memory alignment
    uint16             dest_id;        // Destination ID
    uint16             src_id;         // Source ID
    uint8              rsvd;           // Reserved
    uint8              pkt_type;       // Packet type
    uint16             length;         // Length
    uint16             seq_num;        // Sequence Number
    uint16             flags;          // Flags
} wl_transport_header;

// WARPLAB Command Header
typedef struct
{
    uint32             command_id;     // Command ID
    uint16             length;         // Length
    uint16             num_args;       // Number of Arguments
} wl_command_header;

// WARPLAB Sample Header
typedef struct
{
    uint16             buffer_id;      // Buffer ID
    uint8              flags;          // Flags
    uint8              sample_iq_id;   // IQ ID
    uint32             start;          // Starting sample
    uint32             num_samples;    // Number of samples
} wl_sample_header;

// WARPLAB Sample Tracket
typedef struct
{
    uint32             start_sample;   // Starting sample
    uint32             num_samples;    // Number of samples
} wl_sample_tracker;


typedef int (*wl_function_ptr_t)();


/*********************** Global Variable Definitions *************************/

static int       initialized    = 0;   // Global variable to initialize the driver

// Global structure of socket connections
wl_trans_socket  sockets[TRANSPORT_MAX_SOCKETS];

// Global variables to allow M control of write IQ inter-packet wait time
static uint32    use_user_write_iq_wait_time     = 0;
static uint32    user_write_iq_wait_time         = 0;

// Global variables to allow M control of read IQ max request size
static uint32    use_user_read_iq_max_req_size   = 0;
static uint32    user_read_iq_max_req_size       = 0;

// Global variable to suppress Read IQ / Write IQ warnings
static uint32    suppress_iq_warnings            = 0;

// Global variables for Read / Write IQ IDs
static uint8     sample_read_iq_id               = 0;
static uint8     sample_write_iq_id              = 0;


#ifdef WIN32
WSADATA          wsaData;              // Structure for WinSock setup communication 
#endif



/*************************** Function Prototypes *****************************/

// Main function
void         mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);

// Socket functions
void         print_version( void );
void         init_wl_mex_udp_transport( void );
int          init_socket( void );
void         set_so_timeout( int index, int value );
void         set_reuse_address( int index, int value );
void         set_broadcast( int index, int value );
void         set_send_buffer_size( int index, int size );
int          get_send_buffer_size( int index );
void         set_receive_buffer_size( int index, int size );
int          get_receive_buffer_size( int index );
void         close_socket( int index );
int          send_socket( int index, char *buffer, int length, char *ip_addr, int port );
int          receive_socket( int index, int length, char * buffer );

// Debug / Error functions
void         print_usage( void );
void         print_sockets( void );
void         print_buffer(unsigned char *buf, int size);
void         die( void );
void         die_with_error( char *errorMessage );
void         cleanup( void );


// Helper functions
void         convert_to_uppercase( char *input, char *output, unsigned int len );
unsigned int find_transport_function( char *input, unsigned int len );

uint16       endian_swap_16(uint16 value);
uint32       endian_swap_32(uint32 value);

unsigned int wl_update_checksum(unsigned short int newdata, unsigned char reset );
uint32       wl_compute_sample_wait_time(uint32 * command_args);

int          wl_read_iq_sample_error( wl_sample_tracker *tracker, uint32 num_samples, uint32 start_sample, uint32 num_pkts, uint32 max_sample_size );
int          wl_read_iq_find_error( wl_sample_tracker *tracker, uint32 num_samples, uint32 start_sample, uint32 num_pkts, uint32 max_sample_size,
                                    uint32 *ret_num_samples, uint32 *ret_start_sample, uint32 *ret_num_pkts );

uint32       wl_compute_write_wait_time(uint32 hw_ver, uint32 buffer_id, uint32 max_samples);
uint32       wl_process_write_iq_response(uint32 * command_args, uint32 sample_iq_id, uint32 checksum, uint32 iq_ready_warn);

void         wl_update_seq_num(uint32 function, uint32 buffer_id, uint32 seq_num, uint32 *seq_num_tracker);
void         wl_check_seq_num(uint32 function, char * node_id_str, uint32 buffer_id, uint32 seq_num, uint32 *seq_num_tracker, char *seq_num_severity);

#ifdef WIN32
// Windows specific helper functions
void         wl_mex_udp_transport_usleep( int wait_time );
uint32       wl_mex_udp_transport_usec_timestamp();

#endif


// WARPLab Functions
int          wl_read_baseband_buffer( int index, char *buffer, int length, char *ip_addr, int port,
                                      uint32 initial_offset, uint32 num_samples, uint32 start_sample, uint32 buffer_id, 
                                      uint32 function, uint32 data_type,
                                      void **output_array, uint32 *num_cmds, uint32 *seq_num );

int          wl_write_baseband_buffer( int index, char *buffer, int max_length, char *ip_addr, int port,
                                       uint32 num_samples, uint32 start_sample, const void *samples, uint32 buffer_id, uint32 num_pkts, 
                                       uint32 max_samples, uint32 hw_ver, uint32 check_chksum, uint32 data_type, uint32 iteration,
                                       uint32 *num_cmds, uint32 *checksum );

/******************************** Functions **********************************/


/*****************************************************************************/
/**
*  Function:  init_wl_mex_udp_transport
* 
*  Initialize the driver
*
******************************************************************************/

void init_wl_mex_udp_transport( void ) {
    int i;

    // Print initalization information
    printf("Loaded wl_mex_udp_transport version %s \n", WL_MEX_UDP_TRANSPORT_VERSION );

    // Initialize global variables
    sample_read_iq_id  = 0;
    sample_write_iq_id = 0;
    
    // Initialize Socket datastructure
    for ( i = 0; i < TRANSPORT_MAX_SOCKETS; i++ ) {
        memset( &sockets[i], 0, sizeof(wl_trans_socket) );
        
        sockets[i].handle         = INVALID_SOCKET;
        sockets[i].status         = TRANSPORT_SOCKET_FREE;
        sockets[i].timeout        = 0;
        sockets[i].packet         = NULL;
        sockets[i].rx_buffer_size = 0;
        sockets[i].tx_buffer_size = 0;
    }

#ifdef WIN32
    // Load the Winsock 2.0 DLL
    if ( WSAStartup(MAKEWORD(2, 0), &wsaData) != 0 ) {
        die_with_error("WSAStartup() failed");
    }
#endif
        
    // Set cleanup function to remove persistent data    
    mexAtExit(cleanup);
    
    // Set initialized flag to '1' so this is not run again
    initialized = 1;
}


/*****************************************************************************/
/**
*  Function:  init_socket
*
*  Initializes a socket and returns the index in to the sockets array
*
******************************************************************************/

int init_socket( void ) {
    int i;
    
    // Allocate a socket in the datastructure
    for ( i = 0; i < TRANSPORT_MAX_SOCKETS; i++ ) {
        if ( sockets[i].status == TRANSPORT_SOCKET_FREE ) {  break; }    
    }

    // Check to see if we cannot allocate a socket
    if ( i == TRANSPORT_MAX_SOCKETS) {
        die_with_error("Error:  Cannot allocate a socket");
    }
        
    // Create a best-effort datagram socket using UDP
    if ( ( sockets[i].handle = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0) {
        die_with_error("socket() failed");
    }
    
    // Update the status field of the socket
    sockets[i].status = TRANSPORT_SOCKET_IN_USE;
    
    // Set the reuse_address and broadcast flags for all sockets
    set_reuse_address( i, 1 );
    set_broadcast( i, 1 );
    
    // Set the buffer sizes for all sockets
    get_send_buffer_size(i);
    get_receive_buffer_size(i);
    
    // Listen on the socket; Make sure we have a non-blocking socket
    listen( sockets[i].handle, TRANSPORT_NUM_PENDING );
    non_blocking_socket( sockets[i].handle );

    return i;    
}


/*****************************************************************************/
/**
*  Function:  set_so_timeout
*
*  Sets the Socket Timeout to the value (in ms)
*
******************************************************************************/
void set_so_timeout( int index, int value ) {

    sockets[index].timeout = value;
}


/*****************************************************************************/
/**
*  Function:  set_reuse_address
*
*  Sets the Reuse Address option on the socket
*
******************************************************************************/
void set_reuse_address( int index, int value ) {
    int optval;

    if ( value ) {
        optval = 1;
        setsockopt( sockets[index].handle, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval) );
    } else {
        optval = 0;
        setsockopt( sockets[index].handle, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval) );
    }    
}

/*****************************************************************************/
/**
*  Function:  set_broadcast
*
*  Sets the Broadcast option on the socket
*
******************************************************************************/
void set_broadcast( int index, int value ) {
    int optval;

    if ( value ) {
        optval = 1;
        setsockopt( sockets[index].handle, SOL_SOCKET, SO_BROADCAST, (const char *)&optval, sizeof(optval) );
    } else {
        optval = 0;
        setsockopt( sockets[index].handle, SOL_SOCKET, SO_BROADCAST, (const char *)&optval, sizeof(optval) );
    }    
}


/*****************************************************************************/
/**
*  Function:  set_send_buffer_size
*
*  Sets the send buffer size on the socket
*
******************************************************************************/
void set_send_buffer_size( int index, int size ) {
    int optval = size;
    int optlen = sizeof(int);
    int retval = 0;

    // Request size bytes for the socket buffer size
    setsockopt( sockets[index].handle, SOL_SOCKET, SO_SNDBUF, (const char *)&optval, sizeof(optval) );
    
    // Get the socket buffer size and set it in the global data structure
    if ( (retval = getsockopt( sockets[index].handle, SOL_SOCKET, SO_SNDBUF, (char *)&optval, (socklen_t *)&optlen )) != 0 ) {
        die_with_error("Error:  Could not get socket option - send buffer size"); 
    } else {
        sockets[index].tx_buffer_size = optval;
    }
}


/*****************************************************************************/
/**
*  Function:  get_send_buffer_size
*
*  Gets the send buffer size on the socket
*
******************************************************************************/
int get_send_buffer_size( int index ) {
    int optval = 0;
    int optlen = sizeof(int);
    int retval = 0;
    
    // Get the socket buffer size and set it in the global data structure
    if ( (retval = getsockopt( sockets[index].handle, SOL_SOCKET, SO_SNDBUF, (char *)&optval, (socklen_t *)&optlen )) != 0 ) {
        die_with_error("Error:  Could not get socket option - send buffer size"); 
    } else {
        sockets[index].tx_buffer_size = optval;
    }
    
#ifdef _DEBUG_    
    printf("Send Buffer Size:  %d \n", optval );
#endif
    
    return optval;
}


/*****************************************************************************/
/**
*  Function:  set_receive_buffer_size
*
*  Sets the receive buffer size on the socket
*
******************************************************************************/
void set_receive_buffer_size( int index, int size ) {
    int optval = size;
    int optlen = sizeof(int);
    int retval = 0;

    // Request size bytes for the socket buffer size
    setsockopt( sockets[index].handle, SOL_SOCKET, SO_RCVBUF, (const char *)&optval, sizeof(optval) );

    // Get the socket buffer size and set it in the global data structure
    if ( (retval = getsockopt( sockets[index].handle, SOL_SOCKET, SO_RCVBUF, (char *)&optval, (socklen_t *)&optlen )) != 0 ) {
        die_with_error("Error:  Could not get socket option - send buffer size"); 
    } else {
        sockets[index].rx_buffer_size = optval;
    }
}


/*****************************************************************************/
/**
*  Function:  get_receive_buffer_size
*
*  Gets the receive buffer size on the socket
*
******************************************************************************/
int get_receive_buffer_size( int index ) {
    int optval = 0;
    int optlen = sizeof(int);
    int retval = 0;
    
    // Get the socket buffer size and set it in the global data structure
    if ( (retval = getsockopt( sockets[index].handle, SOL_SOCKET, SO_RCVBUF, (char *)&optval, (socklen_t *)&optlen )) != 0 ) {
        die_with_error("Error:  Could not get socket option - send buffer size"); 
    } else {
        sockets[index].rx_buffer_size = optval;
    }
    
#ifdef _DEBUG_    
    printf("Rcvd Buffer Size:  %d \n", optval );
#endif
    
    return optval;
}


/*****************************************************************************/
/**
*  Function:  close_socket
*
*  Closes the socket based on the index
*
******************************************************************************/
void close_socket( int index ) {

#ifdef _DEBUG_
    printf("Close Socket: %d\n", index);
#endif    

    if ( sockets[index].handle != INVALID_SOCKET ) {
        close( sockets[index].handle );
        
        if ( sockets[index].packet != NULL ) {
            free( sockets[index].packet );
        }
    } else {
        printf( "WARNING:  Connection %d already closed.\n", index );
    }

    sockets[index].handle         = INVALID_SOCKET;
    sockets[index].status         = TRANSPORT_SOCKET_FREE;
    sockets[index].timeout        = 0;
    sockets[index].packet         = NULL;
    sockets[index].rx_buffer_size = 0;
    sockets[index].tx_buffer_size = 0;
}


/*****************************************************************************/
/**
*  Function:  send_socket
*
*  Sends the buffer to the IP address / Port that is passed in
*
******************************************************************************/
int send_socket( int index, char *buffer, int length, char *ip_addr, int port ) {

    struct sockaddr_in socket_addr;  // Socket address
    int                length_sent;
    int                size;

    // Construct the address structure
    memset( &socket_addr, 0, sizeof(socket_addr) );        // Zero out structure 
    socket_addr.sin_family      = AF_INET;                 // Internet address family
    socket_addr.sin_addr.s_addr = inet_addr(ip_addr);      // IP address 
    socket_addr.sin_port        = htons(port);             // Port 

    // If we are sending a large amount of data, we need to make sure the entire 
    // buffer has been sent.
    
    length_sent = 0;
    size        = 0xFFFF;
    
    if ( sockets[index].status != TRANSPORT_SOCKET_IN_USE ) {
        return length_sent;
    }

    while ( length_sent < length ) {
    
        // If we did not send more than MIN_SEND_SIZE, then wait a bit
        if ( size < TRANSPORT_MIN_SEND_SIZE ) {
            wl_usleep( TRANSPORT_SLEEP_TIME );
        }

        // Send as much data as possible to the address
        size = sendto( sockets[index].handle, &buffer[length_sent], (length - length_sent), 0, 
                      (struct sockaddr *) &socket_addr, sizeof(socket_addr) );

        // Check the return value    
        if ( size == SOCKET_ERROR )  {
            if ( get_last_error != EWOULDBLOCK ) {
                die_with_error("Error:  Socket Error.");
            } else {
                // If the socket is not ready, then we did not send any bytes
                length_sent += 0;
            }
        } else {
            // Update how many bytes we sent
            length_sent += size;        
        }

        // TODO:  IMPLEMENT A TIMEOUT SO WE DONT GET STUCK HERE FOREVER
        //        FOR WARPLab 7.3.0, this is not implemented and has not 
        //        been an issue during testing.
    }

    // Error checking
    if ( length_sent != length ) {
        printf("Size of packet sent = %d\n", length_sent);
        printf("Size of packet      = %d\n", length);
        die_with_error("Error:  Size of packet sent does not match size of packet.  See above.");
    }
    
    return length_sent;
}


/*****************************************************************************/
/**
*  Function:  receive_socket
*
*  Reads data from the socket; will return 0 if no data is available
*
******************************************************************************/
int receive_socket( int index, int length, char * buffer ) {

    wl_trans_data_pkt  *pkt;           
    int                 size;
    int                 socket_addr_size = sizeof(struct sockaddr_in);
    
    // Allocate a packet in memory if necessary
    if ( sockets[index].packet == NULL ) {
        sockets[index].packet = (wl_trans_data_pkt *) malloc( sizeof(wl_trans_data_pkt) );
        
        make_persistent( sockets[index].packet );

        if ( sockets[index].packet == NULL ) {
            die_with_error("Error:  Cannot allocate memory for packet.");        
        }

        memset( sockets[index].packet, 0, sizeof(wl_trans_data_pkt));        
    }

    // Get the packet associcated with the index
    pkt = sockets[index].packet;

    // If we have a packet from the last recevie call, then zero out the address structure    
    if ( pkt->length != 0 ) {
        memset( &(pkt->address), 0, sizeof(socket_addr_size));
    }

    // Receive a response 
    size = recvfrom( sockets[index].handle, buffer, length, 0, 
                    (struct sockaddr *) &(pkt->address), (socklen_t *) &socket_addr_size );


    // Check on error conditions
    if ( size == SOCKET_ERROR )  {
        if ( get_last_error != EWOULDBLOCK ) {
            die_with_error("Error:  Socket Error.");
        } else {
            // If the socket is not ready, then just return a size of 0 so the function can be 
            // called again
            size = 0;
        }
    } else {
                    
        // Update the packet associated with the socket
        //   NOTE:  pkt.address was updated via the function call
        pkt->buf     = buffer;
        pkt->offset  = 0;
    }

    // Update the packet length so we can determine when we need to zero out pkt.address
    pkt->length  = size;

    return size;
}


/*****************************************************************************/
/**
*  Function:  cleanup
*
*  Function called by atMexExit to close everything
*
******************************************************************************/
void cleanup( void ) {
    int i;

    printf("MEX-file is terminating\n");

    // Close all sockets
    for ( i = 0; i < TRANSPORT_MAX_SOCKETS; i++ ) {
        if ( sockets[i].handle != INVALID_SOCKET ) {  close_socket( i ); }    
    }

#ifdef WIN32
    WSACleanup();  // Cleanup Winsock 
#endif

}


/*****************************************************************************/
/**
*  Function:  print_version
*
* This function will print the version of the wl_mex_udp_transport driver
*
******************************************************************************/
void print_version( ) {

    printf("WARPLab MEX UDP Transport v%s (compiled %s %s)\n", WL_MEX_UDP_TRANSPORT_VERSION, __DATE__, __TIME__);
    printf("Copyright 2013-2015, Mango Communications. All rights reserved.\n");
    printf("Distributed under the WARP license:  http://warpproject.org/license  \n");
}


/*****************************************************************************/
/**
*  Function:  print_usage
*
* This function will print the usage of the wl_mex_udp_transport driver
*
******************************************************************************/
void print_usage( ) {

    printf("Usage:  WARPLab MEX Transport v%s \n", WL_MEX_UDP_TRANSPORT_VERSION );
    printf("Standard WARPLab transport functions: \n");
    printf("    1.                  wl_mex_udp_transport('version') \n");
    printf("    2. index          = wl_mex_udp_transport('init_socket') \n");
    printf("    3.                  wl_mex_udp_transport('set_so_timeout', index, timeout) \n");
    printf("    4.                  wl_mex_udp_transport('set_send_buf_size', index, size) \n");
    printf("    5. size           = wl_mex_udp_transport('get_send_buf_size', index) \n");
    printf("    6.                  wl_mex_udp_transport('set_rcvd_buf_size', index, size) \n");
    printf("    7. size           = wl_mex_udp_transport('get_rcvd_buf_size', index) \n");
    printf("    8.                  wl_mex_udp_transport('close', index) \n");
    printf("    9. size           = wl_mex_udp_transport('send', index, buffer, length, ip_addr, port) \n");
    printf("   10. [size, buffer] = wl_mex_udp_transport('receive', index, length ) \n");
    printf("\n");
    printf("Additional WARPLab MEX UDP transport functions: \n");
    printf("    1. [num_samples, cmds_used, samples]  = wl_mex_udp_transport('read_rssi' / 'read_iq', \n");
    printf("                                                index, buffer, length, ip_addr, port, \n");
    printf("                                                number_samples, buffer_id, start_sample, \n");
    printf("                                                max_length, num_pkts) \n");
    printf("    2. [cmds_used, checksum]              = wl_mex_udp_transport('write_iq', \n");
    printf("                                                index, cmd_buffer, max_length, ip_addr, port, \n");
    printf("                                                number_samples, sample_buffer, buffer_id, \n");
    printf("                                                start_sample, num_pkts, max_samples, hw_ver, \n");
    printf("                                                check_chksum) \n");
    printf("    3.                = wl_mex_udp_transport('write_iq_set_pkt_wait_time', wait_time) \n");
    printf("    4.                = wl_mex_udp_transport('read_iq_set_max_request_size', size) \n");
    printf("    5.                = wl_mex_udp_transport('suppress_iq_warnings') \n");
    printf("\n");
    printf("See documentation for further details.\n");
    printf("\n");
}


/*****************************************************************************/
/**
*  Function:  die
*
*  Generates an error message and cause the program to halt
*
******************************************************************************/
void die( ) {
    mexErrMsgTxt("Error:  See description above.");
}


/*****************************************************************************/
/**
*  Function:  die_with_error
*
* This function will error out of the wl_mex_udp_transport function call
*
******************************************************************************/
void die_with_error(char *errorMessage) {
    printf("%s \n   Socket Error Code: %d\n", errorMessage, get_last_error );
    die();
}


//#if _DEBUG_

/*****************************************************************************/
/**
*  Function:  print_sockets
*
*  Debug function to print socket table
*
******************************************************************************/
void print_sockets( void ) {
    int i;
    
    printf("Sockets: \n");    
    
    for ( i = 0; i < TRANSPORT_MAX_SOCKETS; i++ ) {
        printf("    socket[%d]:  handle = 0x%4x,   timeout = 0x%4x,  status = 0x%4x,  packet = 0x%8x,  rx_buf_size = %8d,  tx_buf_size = %8d\n", 
               i, sockets[i].handle, sockets[i].timeout, sockets[i].status, sockets[i].packet, sockets[i].rx_buffer_size, sockets[i].tx_buffer_size );
    }

    printf("\n");
}


/*****************************************************************************/
/**
*  Function:  print_buffer
*
*  Debug function to print a buffer
*
******************************************************************************/
void print_buffer(unsigned char *buf, int size) {
	int i;

	printf("Buffer: (0x%x bytes)\n", size);

	for (i=0; i<size; i++) {
        printf("%2x ", buf[i] );
        if ( (((i + 1) % 16) == 0) && ((i + 1) != size) ) {
            printf("\n");
        }
	}
	printf("\n\n");
}


/*****************************************************************************/
/**
*  Function:  print_buffer_16
*
*  Debug function to print a buffer
*
******************************************************************************/
void print_buffer_16(uint16 *buf, int size) {
	int i;

	printf("Buffer: (0x%x bytes)\n", (2*size));

	for (i=0; i<size; i++) {
        printf("%4x ", buf[i] );
        if ( (((i + 1) % 16) == 0) && ((i + 1) != size) ) {
            printf("\n");
        }
	}
	printf("\n\n");
}


/*****************************************************************************/
/**
*  Function:  print_buffer_32
*
*  Debug function to print a buffer
*
******************************************************************************/
void print_buffer_32(uint32 *buf, int size) {
	int i;

	printf("Buffer: (0x%x bytes)\n", (4*size));

	for (i=0; i<size; i++) {
        printf("%8x ", buf[i] );
        if ( (((i + 1) % 8) == 0) && ((i + 1) != size) ) {
            printf("\n");
        }
	}
	printf("\n\n");
}

//#endif


/*****************************************************************************/
/**
*  Function:  endian_swap_16
*
* This function will perform an byte endian swap on a 16-bit value 
*
******************************************************************************/
uint16 endian_swap_16(uint16 value) {

	return (((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8));
}


/*****************************************************************************/
/**
*  Function:  endian_swap_32
*
* This function will perform an byte endian swap on a 32-bit value 
*
******************************************************************************/
uint32 endian_swap_32(uint32 value) {
    uint8  byte_0 = (value & 0x000000FF);
    uint8  byte_1 = (value & 0x0000FF00) >>  8;
    uint8  byte_2 = (value & 0x00FF0000) >> 16;
    uint8  byte_3 = (value & 0xFF000000) >> 24;
    
    return (uint32)((byte_0 << 24) | (byte_1 << 16) | (byte_2 << 8) | byte_3);
}


/*****************************************************************************/
/**
*  Function:  convert_to_uppercase
*
* This function converts the input string to all uppercase letters
*
******************************************************************************/
void convert_to_uppercase(char *input, char *output, unsigned int len){
  unsigned int i;
  
  for ( i = 0; i < (len - 1); i++ ) { 
      output[i] = toupper( input[i] ); 
  }

  output[(len - 1)] = '\0';  
}


/*****************************************************************************/
/**
*  Function:  find_transport_function
*
* This function will return the wl_mex_udp_transport function number based on 
* the input string
*
******************************************************************************/
unsigned int find_transport_function( char *input, unsigned int len ) {
    unsigned int function = 0xFFFF;
    char * uppercase;
    
    uppercase = (char *) mxCalloc(len, sizeof(char));
    convert_to_uppercase( input, uppercase, len );

#ifdef _DEBUG_
    printf("Function :  %s\n", uppercase);
#endif    

    if ( !strcmp( uppercase, "VERSION"                      ) && ( function == 0xFFFF ) ) { function = TRANSPORT_REVISION;                     }
    if ( !strcmp( uppercase, "INIT_SOCKET"                  ) && ( function == 0xFFFF ) ) { function = TRANSPORT_INIT_SOCKET;                  }
    if ( !strcmp( uppercase, "SET_SO_TIMEOUT"               ) && ( function == 0xFFFF ) ) { function = TRANSPORT_SET_SO_TIMEOUT;               }
    if ( !strcmp( uppercase, "SET_SEND_BUF_SIZE"            ) && ( function == 0xFFFF ) ) { function = TRANSPORT_SET_SEND_BUF_SIZE;            }
    if ( !strcmp( uppercase, "GET_SEND_BUF_SIZE"            ) && ( function == 0xFFFF ) ) { function = TRANSPORT_GET_SEND_BUF_SIZE;            }
    if ( !strcmp( uppercase, "SET_RCVD_BUF_SIZE"            ) && ( function == 0xFFFF ) ) { function = TRANSPORT_SET_RCVD_BUF_SIZE;            }
    if ( !strcmp( uppercase, "GET_RCVD_BUF_SIZE"            ) && ( function == 0xFFFF ) ) { function = TRANSPORT_GET_RCVD_BUF_SIZE;            }
    if ( !strcmp( uppercase, "CLOSE"                        ) && ( function == 0xFFFF ) ) { function = TRANSPORT_CLOSE;                        }
    if ( !strcmp( uppercase, "SEND"                         ) && ( function == 0xFFFF ) ) { function = TRANSPORT_SEND;                         }
    if ( !strcmp( uppercase, "RECEIVE"                      ) && ( function == 0xFFFF ) ) { function = TRANSPORT_RECEIVE;                      }
    if ( !strcmp( uppercase, "READ_IQ"                      ) && ( function == 0xFFFF ) ) { function = TRANSPORT_READ_IQ;                      }
    if ( !strcmp( uppercase, "READ_RSSI"                    ) && ( function == 0xFFFF ) ) { function = TRANSPORT_READ_RSSI;                    }
    if ( !strcmp( uppercase, "WRITE_IQ"                     ) && ( function == 0xFFFF ) ) { function = TRANSPORT_WRITE_IQ;                     }
    if ( !strcmp( uppercase, "WRITE_IQ_SET_PKT_WAIT_TIME"   ) && ( function == 0xFFFF ) ) { function = TRANSPORT_WRITE_IQ_SET_PKT_WAIT_TIME;   }
    if ( !strcmp( uppercase, "READ_IQ_SET_MAX_REQUEST_SIZE" ) && ( function == 0xFFFF ) ) { function = TRANSPORT_READ_IQ_SET_MAX_REQUEST_SIZE; }
    if ( !strcmp( uppercase, "SUPPRESS_IQ_WARNINGS"         ) && ( function == 0xFFFF ) ) { function = TRANSPORT_SUPPRESS_IQ_WARNINGS;         }

    mxFree( uppercase );
    return function;
}


/*****************************************************************************/
/**
*
* This function is the mex function to implement the data transfer over sockets
*
* @param	nlhs - Number of left hand side (return) arguments
* @param	plhs - Pointers to left hand side (return) arguments
* @param	nrhs - Number of right hand side (function) arguments
* @param    prhs - Pointers to right hand side (function) arguments
*
* @return	None
*
* @note		This function requires the following pointers:
*   Input:
*       - Function Name
*       - Necessary function arguments (see individual function)
*
*   Output:
*       - Necessary function outputs (see individual function)
*
******************************************************************************/
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{

    //--------------------------------------------------------------------
    // Declare variables

    int            i, j, k;    
    char          *func                     = NULL;
    mwSize         func_len                 = 0;
    int            function                 = 0xFFFF;
    int            handle                   = 0xFFFF;
    char          *buffer                   = NULL;
    const void    *sample_buffer            = NULL;
    char          *ip_addr                  = NULL;
    int            size                     = 0;
    int            timeout                  = 0;
    int            wait_time                = 0;
    int            length                   = 0;
    int            port                     = 0;
    uint32         num_samples              = 0;
    uint32         max_samples              = 0;
    uint32         start_sample             = 0;
    uint32         num_pkts                 = 0;
    uint32         max_length               = 0;
    uint32         num_cmds                 = 0;
    int            check_chksum             = 0;
    uint32         checksum                 = 0;
    double        *checksum_array           = NULL;
    int            hw_ver                   = 0;
    uint32        *buffer_ids               = NULL;
    int            num_buffers              = 0;
    uint32         buffer_id                = 0;
    uint32         start_sample_to_request  = 0;
    uint32         num_samples_to_request   = 0;
    uint32         num_pkts_to_request      = 0;
    uint32         useful_rx_buffer_size    = 0;
    uint32        *command_args             = NULL;

    uint32         data_type                = 0;
    uint32         data_size                = 0;
    uint32         mex_data_type            = 0;

    uint32         seq_num                  = 0;
    uint32        *seq_num_tracker          = NULL;
    char          *seq_num_severity         = NULL;
    
    char          *node_id_str              = NULL;
    
    void          *output_array[2]          = {NULL, NULL};
    uint32         num_output_arrays        = 0;
    uint32         num_output_data          = 0;
    mwSize         ndim                     = (mwSize) 2;
    mwSize         dims[2];
    
    
    
    //--------------------------------------------------------------------
    // Initialization    
    
    if (!initialized) {
#ifdef _DEBUG_
            printf("Initialization \n");
#endif
    
       // Initialize driver
       init_wl_mex_udp_transport();           

#ifdef _DEBUG_
            printf("END Initialization \n");
#endif
    }


    //--------------------------------------------------------------------
    // Process input
    
    // Check for proper number of arguments 
    if( nrhs < 1 ) { print_usage(); die(); }

    // Input must be a string 
    if ( mxIsChar( prhs[0] ) != 1 ) {
        mexErrMsgTxt("Error: Input must be a string.");
    }

    // Input must be a row vector
    if ( mxGetM( prhs[0] ) != 1 ) {
        mexErrMsgTxt("Error: Input must be a row vector.");
    }
    
    // get the length of the input string
    func_len = (mwSize) ( mxGetM( prhs[0] ) * mxGetN( prhs[0] ) ) + 1;

    // copy the string data from prhs[0] into a C string func
    func = mxArrayToString( prhs[0] );
    
    if( func == NULL ) {
        mexErrMsgTxt("Error:  Could not convert input to string.");
    }


    //--------------------------------------------------------------------
    // Process commands
    
    function = find_transport_function( func, func_len );

    switch ( function ) {
    
        //------------------------------------------------------
        // wl_mex_udp_transport('version')
        //   - Arguments:
        //     - none
        //   - Returns:
        //     - none
        case TRANSPORT_REVISION :
#ifdef _DEBUG_
            printf("Function TRANSPORT_REVISION: \n");
#endif
            // Validate arguments
            if( nrhs != 1 ) { print_usage(); die(); }

            // Call function
            if( nlhs == 0 ) { 
                print_version(); 
            } else if( nlhs == 1 ) { 
                plhs[0] = mxCreateString( WL_MEX_UDP_TRANSPORT_VERSION );
            } else { 
                print_usage(); die(); 
            }

#ifdef _DEBUG_
            printf("END TRANSPORT_REVISION \n");
#endif
        break;

        //------------------------------------------------------
        // handle = wl_mex_udp_transport('init_socket')
        //   - Arguments:
        //     - none
        //   - Returns:
        //     - handle (int)     - index to the socket
        case TRANSPORT_INIT_SOCKET :
#ifdef _DEBUG_
            printf("Function TRANSPORT_INIT_SOCKET: \n");
#endif
            // Validate arguments
            if( nrhs != 1 ) { print_usage(); die(); }
            if( nlhs != 1 ) { print_usage(); die(); }

            // Call function
            handle = init_socket();
            
            // Return value to MABLAB
            plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
            *mxGetPr(plhs[0]) = handle;

#ifdef _DEBUG_
            print_sockets();
            printf("END TRANSPORT_INIT_SOCKET \n");
#endif
        break;

        //------------------------------------------------------
        // wl_mex_udp_transport('set_so_timeout', handle, timeout)
        //   - Arguments:
        //     - handle (int)     - index to the requested socket
        //     - timeout (int)    - timeout value (in ms); 0 => infinite timeout
        //   - Returns:
        //     - none
        case TRANSPORT_SET_SO_TIMEOUT :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_SET_SO_TIMEOUT\n");
#endif
            // Validate arguments
            if( nrhs != 3 ) { print_usage(); die(); }
            if( nlhs != 0 ) { print_usage(); die(); }

            // Get input arguments
            handle  = (int) mxGetScalar(prhs[1]);
            timeout = (int) mxGetScalar(prhs[2]);

            // Call function
            set_so_timeout( handle, timeout );
        
#ifdef _DEBUG_
            print_sockets();
            printf("END TRANSPORT_SET_SO_TIMEOUT \n");
#endif
        break;

        //------------------------------------------------------
        // wl_mex_udp_transport('set_send_buf_size', handle, size)
        //   - Arguments:
        //     - handle (int)     - index to the requested socket
        //     - size (int)       - size of buffer (in bytes)
        //   - Returns:
        //     - none
        case TRANSPORT_SET_SEND_BUF_SIZE :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_SET_SEND_BUF_SIZE\n");
#endif        
            // Validate arguments
            if( nrhs != 3 ) { print_usage(); die(); }
            if( nlhs != 0 ) { print_usage(); die(); }

            // Get input arguments
            handle  = (int) mxGetScalar(prhs[1]);
            size    = (int) mxGetScalar(prhs[2]);

            // Call function
            set_send_buffer_size( handle, size );

#ifdef _DEBUG_
            printf("END TRANSPORT_SET_SEND_BUF_SIZE \n");
#endif
        break;

        //------------------------------------------------------
        // size = wl_mex_udp_transport('get_send_buf_size', handle)
        //   - Arguments:
        //     - handle (int)     - index to the requested socket
        //   - Returns:
        //     - size (int)       - size of buffer (in bytes)
        case TRANSPORT_GET_SEND_BUF_SIZE :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_GET_SEND_BUF_SIZE\n");
#endif
            // Validate arguments
            if( nrhs != 2 ) { print_usage(); die(); }
            if( nlhs != 1 ) { print_usage(); die(); }
            
            // Get input arguments
            handle  = (int) mxGetScalar(prhs[1]);

            // Call function
            size = get_send_buffer_size( handle );
            
            // Return value to MABLAB
            plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
            *mxGetPr(plhs[0]) = size;
        
#ifdef _DEBUG_
            printf("END TRANSPORT_GET_SEND_BUF_SIZE \n");
#endif
        break;
        
        //------------------------------------------------------
        // wl_mex_udp_transport('set_rcvd_buf_size', handle, size)
        //   - Arguments:
        //     - handle (int)     - index to the requested socket
        //     - size (int)       - size of buffer (in bytes)
        //   - Returns:
        //     - none
        case TRANSPORT_SET_RCVD_BUF_SIZE :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_SET_RCVD_BUF_SIZE\n");
#endif
            // Validate arguments
            if( nrhs != 3 ) { print_usage(); die(); }
            if( nlhs != 0 ) { print_usage(); die(); }

            // Get input arguments
            handle  = (int) mxGetScalar(prhs[1]);
            size    = (int) mxGetScalar(prhs[2]);

            // Call function
            set_receive_buffer_size( handle, size );
            
#ifdef _DEBUG_
            printf("END TRANSPORT_SET_RCVD_BUF_SIZE \n");
#endif
        break;
        
        //------------------------------------------------------
        // size = wl_mex_udp_transport('get_rcvd_buf_size', handle)
        //   - Arguments:
        //     - handle (int)     - index to the requested socket
        //   - Returns:
        //     - size (int)       - size of buffer (in bytes)
        case TRANSPORT_GET_RCVD_BUF_SIZE :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_GET_RCVD_BUF_SIZE\n");
#endif
            // Validate arguments
            if( nrhs != 2 ) { print_usage(); die(); }
            if( nlhs != 1 ) { print_usage(); die(); }
            
            // Get input arguments
            handle  = (int) mxGetScalar(prhs[1]);

            // Call function
            size = get_receive_buffer_size( handle );
            
            // Return value to MABLAB
            plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
            *mxGetPr(plhs[0]) = size;
        
#ifdef _DEBUG_
            printf("END TRANSPORT_GET_RCVD_BUF_SIZE \n");
#endif
        break;

        //------------------------------------------------------
        // wl_mex_udp_transport('close', handle)
        //   - Arguments:
        //     - handle (int)     - index to the requested socket
        //   - Returns:
        //     - none
        case TRANSPORT_CLOSE :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_CLOSE\n");
#endif
            // Validate arguments
            if( nrhs != 2 ) { print_usage(); die(); }
            if( nlhs != 0 ) { print_usage(); die(); }

            // Get input arguments
            handle  = (int) mxGetScalar(prhs[1]);

            // Call function if socket not already closed
            //   NOTE:  When 'clear all' is executed in Matlab, it will
            //     call both the cleanup() function in the Mex that closes
            //     all sockets as well as the destroy function on the transport
            //     object that will call the wl_mex_udp_transport('close') function
            //     so we need to check if the socket is already closed to not
            //     print out an erroneous warning.
            if ( sockets[handle].handle != INVALID_SOCKET ) {
                close_socket( handle );
            }
        
#ifdef _DEBUG_
            printf("END TRANSPORT_CLOSE \n");
#endif
        break;

        //------------------------------------------------------
        // size = wl_mex_udp_transport('send', handle, buffer, length, ip_addr, port)
        //   - Arguments:
        //     - handle (int)     - index to the requested socket
        //     - buffer (char *)  - Buffer of data to be sent
        //     - length (int)     - Length of data to be sent
        //     - ip_addr          - IP Address to send data to
        //     - port             - Port to send data to
        //   - Returns:
        //     - size (int)       - size of data sent (in bytes)
        case TRANSPORT_SEND :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_SEND\n");
#endif
            // Validate arguments
            if( nrhs != 6 ) { print_usage(); die(); }
            if( nlhs != 1 ) { print_usage(); die(); }
            
            // Get input arguments
            handle  = (int) mxGetScalar(prhs[1]);
            length  = (int) mxGetScalar(prhs[3]);
            port    = (int) mxGetScalar(prhs[5]);
            
            // Input must be a string 
            if ( mxIsChar( prhs[4] ) != 1 ) { mexErrMsgTxt("Error: Input must be a string."); }
            if ( mxGetM( prhs[4] ) != 1 ) { mexErrMsgTxt("Error: Input must be a row vector."); }
            ip_addr = mxArrayToString( prhs[4] );
            if( ip_addr == NULL ) { mexErrMsgTxt("Error:  Could not convert input to string."); }

#ifdef _DEBUG_
            printf("index = %d, length = %d, port = %d, ip_addr = %s \n", handle, length, port, ip_addr);
#endif
           
            // Input must be an array of uint8
            if ( mxIsUint8( prhs[2] ) != 1 ) { mexErrMsgTxt("Error: Input must be an array of uint8"); }
            if ( mxGetM( prhs[2] ) != 1 ) { mexErrMsgTxt("Error: Input must be a row vector."); }
            buffer = (char *) mxGetData( prhs[2] );
            if( buffer == NULL ) { mexErrMsgTxt("Error:  Could not convert input to array of char."); }
            
            // Call function
            size = send_socket( handle, buffer, length, ip_addr, port );
            
            // Return value to MABLAB
            plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
            *mxGetPr(plhs[0]) = size;

            mxFree( ip_addr );

#ifdef _DEBUG_
            print_buffer( buffer, length );
            printf("END TRANSPORT_SEND \n");
#endif
        break;
        
        //------------------------------------------------------
        // [size, buffer] = wl_mex_udp_transport('receive', handle, length )
        //   - Arguments:
        //     - handle (int)     - index to the requested socket
        //     - length (int)     - length of data to be received (in bytes)
        //   - Returns:
        //     - buffer (char *)  - Buffer of data received
        case TRANSPORT_RECEIVE :
                        
#ifdef _DEBUG_
            printf("Function : TRANSPORT_RECEIVE\n");
#endif
            // Validate arguments
            if( nrhs != 3 ) { print_usage(); die(); }
            if( nlhs != 2 ) { print_usage(); die(); }

            // Get input arguments
            handle  = (int) mxGetScalar(prhs[1]);
            length  = (int) mxGetScalar(prhs[2]);

#ifdef _DEBUG_
            printf("index = %d, length = %d \n", handle, length );
#endif

            // Check the length input argument
            if(length == 0) { mexErrMsgTxt("Error:  Receive length of 0 not allowed."); }

            // Allocate memory for the buffer
            buffer = (char *) malloc(sizeof(char) * length);
            if(buffer == NULL) { mexErrMsgTxt("Error:  Could not allocate receive buffer"); }
            for (i = 0; i < length; i++) { buffer[i] = 0; }

            // Call function
            size = receive_socket(handle, length, buffer);

            // Return value to MABLAB
            plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);
            *mxGetPr(plhs[0]) = size;
            
            // Return value to MABLAB
            if (size != 0) {
                plhs[1] = mxCreateNumericMatrix(size, 1, mxUINT8_CLASS, mxREAL);
                if(plhs[1] == NULL) { mexErrMsgTxt("Error:  Could not allocate return buffer"); }            
                memcpy((char *) mxGetData(plhs[1]), buffer, size*sizeof(char));
            } else {
                plhs[1] = mxCreateNumericMatrix(0, 0, mxUINT8_CLASS, mxREAL);            
            }

            // Free allocated memory
            free(buffer);
            
#ifdef _DEBUG_
            if (size != 0) {
                printf("Buffer size = %d \n", size);
                print_buffer((char *) mxGetData(plhs[1]), size);
            }
            printf("END TRANSPORT_RECEIVE \n");
#endif
        break;


        //------------------------------------------------------
        //[num_samples, cmds_used, samples]  = wl_mex_udp_transport('read_rssi' / 'read_iq', 
        //                                        handle, buffer, length, ip_addr, port,
        //                                        number_samples, buffer_ids, start_sample,
        //                                        max_length, num_pkts);
        //   - Arguments:
        //     - handle           (int)          - index to the requested socket
        //     - buffer           (char *)       - Buffer of data to be sent
        //     - length           (int)          - Length of data to be sent
        //     - ip_addr          (char *)       - IP Address to send data to
        //     - port             (int)          - Port to send data to
        //     - num_samples      (int)          - Number of samples requested
        //     - buffer_ids       (int  *)       - List of WARP RF buffer to obtain samples from
        //     - start_sample     (int)          - Starting address in the array for the samples
        //     - max_length       (int)          - Max number of bytes of samples received per packet
        //                                         (last packet may not receive max_length bytes)
        //     - num_pkts         (int)          - Number of Ethernet packets it should take to receive the samples
        //     - data_type        (int)          - Type of the output array:
        //                                             IQ_DATA_TYPE_DOUBLE ==> double / mxDOUBLE_CLASS
        //                                             IQ_DATA_TYPE_SINGLE ==> float  / mxSINGLE_CLASS
        //                                             IQ_DATA_TYPE_INT16  ==> int16  / mxINT16_CLASS
        //                                             IQ_DATA_TYPE_RAW    ==> uint32 / mxUINT32_CLASS
        //     - seq_num_tracker  (int  *)       - Sequence number tracker
        //     - seq_num_severity (char *)       - Severity of response to sequence number match
        //     - node_str_id      (char *)       - Node string ID
        //   - Returns:
        //     - num_samples      (int)          - Number of samples received
        //     - cmds_used        (int)          - Number of transport commands used to obtain samples
        //     - samples          (double *)     - Array of samples received
        case TRANSPORT_READ_IQ:
        case TRANSPORT_READ_RSSI:

#ifdef _DEBUG_
            printf("Function : TRANSPORT_READ_IQ / TRANSPORT_READ_RSSI\n");
#endif

            // Example to profile the performance of Read IQ
            // printf("times = [%12d, ", wl_timestamp);

            // Validate arguments
            if( nrhs != 15 ) { print_usage(); die(); }
            if( nlhs !=  3 ) { print_usage(); die(); }

            // Get input arguments
            handle       = (int) mxGetScalar(prhs[1]);
            length       = (int) mxGetScalar(prhs[3]);
            port         = (int) mxGetScalar(prhs[5]);
            num_samples  = (int) mxGetScalar(prhs[6]);
            start_sample = (int) mxGetScalar(prhs[8]);
            max_length   = (int) mxGetScalar(prhs[9]);
            num_pkts     = (int) mxGetScalar(prhs[10]);
            data_type    = (int) mxGetScalar(prhs[11]);
            
            // Packet data must be an array of uint8
            if ( mxIsUint8( prhs[2] ) != 1 ) { mexErrMsgTxt("Error: Input buffer must be an array of uint8"); }
            if ( mxGetM( prhs[2] ) != 1 ) { mexErrMsgTxt("Error: Input buffer must be a row vector."); }
            buffer = (char *) mxGetData( prhs[2] );
            if( buffer == NULL ) { mexErrMsgTxt("Error:  Could not convert input buffer to array of char."); }

            // IP address input must be a string 
            if ( mxIsChar( prhs[4] ) != 1 ) { mexErrMsgTxt("Error: Input IP address must be a string."); }
            if ( mxGetM( prhs[4] ) != 1 ) { mexErrMsgTxt("Error: Input IP address must be a row vector."); }
            ip_addr = mxArrayToString( prhs[4] );
            if( ip_addr == NULL ) { mexErrMsgTxt("Error:  Could not convert input IP address to string."); }

            // Sequence tracker must be an array of integers
            if ( mxIsUint32( prhs[12] ) != 1 ) { mexErrMsgTxt("Error: Sequence number tracker must be an array of uint32"); }
            if ( mxGetM( prhs[12] ) != 1 ) { mexErrMsgTxt("Error: Sequence number tracker must be a row vector."); }
            seq_num_tracker = (int *) mxGetData( prhs[12] );
            if( seq_num_tracker == NULL ) { mexErrMsgTxt("Error:  Could not convert sequence number tracker to array of uint32."); }

            // Sequence number severity must be a string
            if ( mxIsChar( prhs[13] ) != 1 ) { mexErrMsgTxt("Error: Sequence number severity must be a string."); }
            if ( mxGetM( prhs[13] ) != 1 ) { mexErrMsgTxt("Error: Sequence number severity must be a row vector."); }
            seq_num_severity = mxArrayToString( prhs[13] );
            if( seq_num_severity == NULL ) { mexErrMsgTxt("Error:  Could not convert sequence number severity to string."); }

            // Node ID string must be a string
            if ( mxIsChar( prhs[14] ) != 1 ) { mexErrMsgTxt("Error: Node ID string must be a string."); }
            if ( mxGetM( prhs[14] ) != 1 ) { mexErrMsgTxt("Error: Node ID string must be a row vector."); }
            node_id_str = mxArrayToString( prhs[14] );
            if( node_id_str == NULL ) { mexErrMsgTxt("Error:  Could not convert node ID string to string."); }

            // Buffer IDs must be an array of singular buffer IDs
            if ( mxIsUint32( prhs[7] ) != 1 ) { mexErrMsgTxt("Error: Input buffer IDs must be an array of uint32"); }
            if ( mxGetM( prhs[7] ) != 1 ) { mexErrMsgTxt("Error: Input buffer IDs must be a row vector."); }            
            buffer_ids    = (int *) mxGetData( prhs[7] );
            if( buffer_ids == NULL ) { mexErrMsgTxt("Error:  Could not convert input buffer IDs to array of uint32."); }
            
            num_buffers = (int) mxGetN( prhs[7] );

            for ( i = 0; i < num_buffers; i++ ) {
                buffer_id = buffer_ids[i];
                
                if ( !((buffer_id == BUFFER_ID_RFA) || (buffer_id == BUFFER_ID_RFB) || (buffer_id == BUFFER_ID_RFC) || (buffer_id == BUFFER_ID_RFD))) {
                    mexErrMsgTxt("Error:  Buffer selection must be singular.  Use vector notation for reading from multiple buffers e.g. [RFA,RFB]");
                }
            }
            
            // Determine data types and sizes based on input data_type
            switch (data_type) {
                case IQ_DATA_TYPE_DOUBLE:
                    mex_data_type     = mxDOUBLE_CLASS;
                    data_size         = sizeof(double);                    
                break;
                
                case IQ_DATA_TYPE_SINGLE:
                    mex_data_type     = mxSINGLE_CLASS;
                    data_size         = sizeof(float);
                break;
                
                case IQ_DATA_TYPE_INT16:
                    mex_data_type     = mxINT16_CLASS;
                    data_size         = sizeof(int16);
                break;
                
                case IQ_DATA_TYPE_RAW:
                    mex_data_type     = mxUINT32_CLASS;
                    data_size         = sizeof(uint32);
                break;
                
                default:
                    mexErrMsgTxt("Error:  Unsupported output data type");
                break;
            }
            
            // Allocate output variables based on the data type
            switch (data_type) {
                case IQ_DATA_TYPE_DOUBLE:
                    // Data allocation scheme uses mxGetPr / mxGetPi vs mxGetData / mxGetImagData
                    switch ( function ) {
                        case TRANSPORT_READ_IQ:
                            plhs[2] = mxCreateDoubleMatrix(num_samples, num_buffers, mxCOMPLEX);
                            if( plhs[2] == NULL ) { mexErrMsgTxt("Error:  Could not allocate return buffer"); }

                            output_array[0]   = (void *) mxGetPr(plhs[2]);
                            output_array[1]   = (void *) mxGetPi(plhs[2]);
                            num_output_arrays = 2;
                            num_output_data   = num_samples;
                        break;
                        
                        case TRANSPORT_READ_RSSI:
                            plhs[2] = mxCreateDoubleMatrix((2 * num_samples), num_buffers, mxREAL);
                            if( plhs[2] == NULL ) { mexErrMsgTxt("Error:  Could not allocate return buffer"); }            

                            output_array[0]   = (void *) mxGetPr(plhs[2]);
                            num_output_arrays = 1;
                            num_output_data   = 2 * num_samples;
                        break;
                    }
                break;
                
                case IQ_DATA_TYPE_SINGLE:
                case IQ_DATA_TYPE_INT16:
                    // Data allocation scheme is the same for 'single', and 'int16'
                    switch ( function ) {
                        case TRANSPORT_READ_IQ:
                            dims[0] = (mwSize) num_samples;
                            dims[1] = (mwSize) num_buffers;

                            plhs[2] = mxCreateNumericArray(ndim, dims, mex_data_type, mxCOMPLEX);
                            if( plhs[2] == NULL ) { mexErrMsgTxt("Error:  Could not allocate return buffer"); }

                            output_array[0]   = mxGetData(plhs[2]);
                            output_array[1]   = mxGetImagData(plhs[2]);
                            num_output_arrays = 2;
                            num_output_data   = num_samples;
                        break;
                        
                        case TRANSPORT_READ_RSSI:
                            dims[0] = (mwSize) (2 * num_samples);
                            dims[1] = (mwSize) num_buffers;

                            plhs[2] = mxCreateNumericArray(ndim, dims, mex_data_type, mxREAL);
                            if( plhs[2] == NULL ) { mexErrMsgTxt("Error:  Could not allocate return buffer"); }            

                            output_array[0]   = mxGetData(plhs[2]);
                            num_output_arrays = 1;
                            num_output_data   = 2 * num_samples;
                        break;
                    }
                break;
                
                case IQ_DATA_TYPE_RAW:
                    // Allocate a uint32 array for raw data
                    dims[0] = (mwSize) num_samples;
                    dims[1] = (mwSize) num_buffers;

                    plhs[2] = mxCreateNumericArray(ndim, dims, mex_data_type, mxREAL);
                    if( plhs[2] == NULL ) { mexErrMsgTxt("Error:  Could not allocate return buffer"); }            

                    output_array[0]   = mxGetData(plhs[2]);
                    num_output_arrays = 1;
                    num_output_data   = num_samples;
                break;
                
                default:
                    mexErrMsgTxt("Error:  Unsupported output data type");
                break;
            }
            
            plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);
            plhs[1] = mxCreateDoubleMatrix(1, 1, mxREAL);
            
            // If the default implementation to limit Read IQ request size is not sufficient, then 
            // the user can override the Read IQ max request size.
            //   NOTE:  The request size must be at least max_length so that we don't run in to a corner
            //          case of spinning forever requesting zero samples.  
            if ( use_user_read_iq_max_req_size == 1 ) {
                        
                if ( user_read_iq_max_req_size < max_length ) {
                    useful_rx_buffer_size = max_length;
                } else {
                    useful_rx_buffer_size = user_read_iq_max_req_size;
                }
            } else {

                // Set the useful RX buffer size to 80% of the RX buffer
                //     NOTE:  This is integer division so the rx_buffer_size will be truncated by the divide
                //
                useful_rx_buffer_size  = 8 * ( sockets[handle].rx_buffer_size / 10 );
            }

#ifdef _DEBUG_
            printf("Useful buffer size = %d (of %d) for %d pkt request\n", useful_rx_buffer_size, sockets[handle].rx_buffer_size, num_pkts);
            printf("  Num samples  = %d     Useful buffer samples = %d\n", num_samples, (useful_rx_buffer_size >> 2));
#endif

            // Iterate thru all the buffers that have been requested
            for (k = 0; k < num_buffers; k++) {
            
                // Set the buffer ID for this Read IQ
                buffer_id = buffer_ids[k];
                
                // Update the buffer with the correct command arguments since it is too expensive to do in MATLAB
                command_args    = (uint32 *) ( buffer + sizeof( wl_transport_header ) + sizeof( wl_command_header ) );
                command_args[0] = endian_swap_32( buffer_id );
                command_args[3] = endian_swap_32( max_length );

                // Check to see if we have enough receive buffer space for the requested packets.
                // If not, then break the request up in to multiple requests.            
                if( num_samples < ( useful_rx_buffer_size >> 2 ) ) {

                    // Call receive function normally
                    command_args[1] = endian_swap_32( start_sample );
                    command_args[2] = endian_swap_32( num_samples );
                    command_args[4] = endian_swap_32( num_pkts );

                    // Call function
                    size = wl_read_baseband_buffer( handle, buffer, length, ip_addr, port,
                                                    start_sample, num_samples, start_sample, buffer_id, function, data_type,
                                                    output_array, &num_cmds, &seq_num );

                } else {

                    // Since we are requesting more data than can fit in to the receive buffer, break this 
                    // request in to multiple function calls, so we do not hit the timeout functions

                    // Number of packets that can fit in the receive buffer
                    num_pkts_to_request     = useful_rx_buffer_size / max_length;            // RX buffer size in bytes / Max packet size in bytes
                    
                    // Number of samples in a request (number of samples in a packet * number of packets in a request)
                    num_samples_to_request  = (max_length >> 2) * num_pkts_to_request;
                    
                    // Starting sample
                    start_sample_to_request = start_sample;

#ifdef _DEBUG_
                    printf("  Num pkts per req = %d     Num samples per req = %d\n", num_pkts_to_request, num_samples_to_request);
#endif                
                
                    // Error checking to make sure something bad did not happen
                    if ( num_pkts_to_request > num_pkts ) {
                        printf("ERROR:  Read IQ / Read RSSI - Parameter mismatch \n");
                        printf("    Requested %d packet(s) and %d sample(s) in function call.  \n", num_pkts, num_samples);
                        printf("    Receive buffer can hold %d samples (ie %d packets).  \n", num_samples_to_request, num_pkts_to_request);
                        printf("    Since, the number of samples requested is greater than what the receive buffer can hold, \n");
                        printf("    the number of packets requested should be greater than what the receive buffer can hold. \n");
                        die_with_error("Error:  Read IQ / Read RSSI - Parameter mismatch.  See above for debug information.");
                    }

                    for( i = num_pkts; i > 0; i -= num_pkts_to_request ) {

                        j = i - num_pkts_to_request;

                        // If we are requesting the last set of packets, then just request the remaining samples
                        if ( j < 0 ) {
                            num_samples_to_request = num_samples - ((num_pkts - i) * (max_length >> 2));
                            num_pkts_to_request    = i;
                        }

                        // Need to set all args here b/c if there is a timeout then the command args in the buffer will
                        // be changed to request the missing data.
                        command_args[1] = endian_swap_32( start_sample_to_request );
                        command_args[2] = endian_swap_32( num_samples_to_request );
                        command_args[4] = endian_swap_32( num_pkts_to_request );

#ifdef _DEBUG_
                        printf("    Req Num pkts = %5d    Num samples = %10d    Start sample = %10d \n", num_pkts_to_request, num_samples_to_request, start_sample_to_request);
#endif

                        // Call function
                        size = wl_read_baseband_buffer( handle, buffer, length, ip_addr, port,
                                                        start_sample, num_samples_to_request, start_sample_to_request, buffer_id, function, data_type,
                                                        output_array, &num_cmds, &seq_num );

                        start_sample_to_request += num_samples_to_request;
                    }
                    
                    size = num_samples;
                }

                // Do not update the pointers on the last iteration thru the loop
                if (k < (num_buffers - 1)) {
                    // Update the output array pointers to the next section of samples
                    for (i = 0; i < num_output_arrays; i++) {
                        output_array[i] = (void *)(((long long)(output_array[i])) + (long long)(num_output_data * data_size));
                    }
                }
                
                // Check the sequence number
                wl_check_seq_num(function, node_id_str, buffer_id, seq_num, seq_num_tracker, seq_num_severity);
                
                // Update the sequence number
                wl_update_seq_num(function, buffer_id, seq_num, seq_num_tracker);
                
            }  // END for each buffer_id
            
            // Return values to MABLAB
            *mxGetPr(plhs[0]) = size;            
            *mxGetPr(plhs[1]) = num_cmds;

            // Process output array
            if ( size == 0 ) {
                // Free any allocated arrays
                if ( plhs[2] != NULL ) {
                    mxDestroyArray( plhs[2] );
                }
                
                // Return an empty array
                plhs[2] = mxCreateDoubleMatrix(0, 0, mxCOMPLEX);
            }
            
            // Free allocated memory
            free( ip_addr );

#ifdef _DEBUG_
            printf("END TRANSPORT_READ_IQ \ TRANSPORT_READ_RSSI\n");
#endif        
        break;        

        //------------------------------------------------------
        // [cmds_used, checksum] = wl_mex_udp_transport('write_iq', handle, cmd_buffer, max_length, ip_addr, port, 
        //                                              number_samples, samples, buffer_ids, start_sample, 
        //                                              num_pkts, max_samples, hw_ver, check_chksum, data_type);
        //
        //   - Arguments:
        //     - handle          (int)      - Index to the requested socket
        //     - cmd_buffer      (char *)   - Buffer of data to be used as the header for Write IQ command
        //     - max_length      (int)      - Length of max data packet (in bytes)
        //     - ip_addr         (char *)   - IP Address to send samples to
        //     - port            (int)      - Port to send samples to
        //     - num_samples     (int)      - Number of samples to send
        //     - samples         (void *)   - Array of IQ samples to be sent (type determined by data_type)
        //     - buffer_ids      (int  *)   - List of WARP RF buffers to write samples to
        //     - start_sample    (int)      - Starting address in the array of samples
        //     - num_pkts        (int)      - Number of Ethernet packets it should take to send the samples
        //     - max_samples     (int)      - Max number of samples transmitted per packet
        //                                    (last packet may not send max_sample samples)
        //     - hw_ver          (int)      - Hardware version;  Since different HW versions have different 
        //                                    processing capabilities, we need to know the HW version for timing
        //     - check_chksum    (int)      - Perform the robustness check of the WriteIQ transmission in 
        //                                    this function or assume the WriteIQ was successful.
        //     - data_type       (int)      - Type of the output array:
        //                                        IQ_DATA_TYPE_DOUBLE ==> double / mxDOUBLE_CLASS
        //                                        IQ_DATA_TYPE_SINGLE ==> float  / mxSINGLE_CLASS
        //                                        IQ_DATA_TYPE_INT16  ==> int16  / mxINT16_CLASS
        //                                        IQ_DATA_TYPE_RAW    ==> uint32 / mxUINT32_CLASS
        //
        //   - Returns:
        //     - cmds_used   (int)  - number of transport commands used to send samples
        //     - checksum    (int)  - WriteIQ checksum calculated by Mex
        //
        case TRANSPORT_WRITE_IQ:

#ifdef _DEBUG_
            printf("Function : TRANSPORT_WRITE_IQ\n");
#endif
            // Validate arguments
            if( nrhs != 15 ) { print_usage(); die(); }
            if( nlhs !=  2 ) { print_usage(); die(); }

            // Get input arguments
            handle       = (int) mxGetScalar(prhs[1]);
            max_length   = (int) mxGetScalar(prhs[3]);
            port         = (int) mxGetScalar(prhs[5]);
            num_samples  = (int) mxGetScalar(prhs[6]);
            start_sample = (int) mxGetScalar(prhs[9]);
            num_pkts     = (int) mxGetScalar(prhs[10]);
            max_samples  = (int) mxGetScalar(prhs[11]);
            hw_ver       = (int) mxGetScalar(prhs[12]);
            check_chksum = (int) mxGetScalar(prhs[13]);
            data_type    = (int) mxGetScalar(prhs[14]);
            
            // Packet data must be an array of uint8
            if ( mxIsUint8( prhs[2] ) != 1 ) { mexErrMsgTxt("Error: Command Buffer input must be an array of uint8"); }
            if ( mxGetM( prhs[2] ) != 1 ) { mexErrMsgTxt("Error: Command Buffer input must be a row vector."); }
            buffer = (char *) mxGetData( prhs[2] );
            if( buffer == NULL ) { mexErrMsgTxt("Error:  Could not convert command buffer input to array of char."); }

            // IP address input must be a string 
            if ( mxIsChar( prhs[4] ) != 1 ) { mexErrMsgTxt("Error: IP Address input must be a string."); }
            if ( mxGetM( prhs[4] ) != 1 ) { mexErrMsgTxt("Error: IP Address input must be a row vector."); }
            ip_addr = mxArrayToString( prhs[4] );
            if( ip_addr == NULL ) { mexErrMsgTxt("Error:  Could not convert ip address input to string."); }

            // Buffer IDs must be an array of singular buffer IDs
            if ( mxIsUint32( prhs[8] ) != 1 ) { mexErrMsgTxt("Error: Input buffer IDs must be an array of uint32"); }
            if ( mxGetM( prhs[8] ) != 1 ) { mexErrMsgTxt("Error: Input buffer IDs must be a row vector."); }            
            buffer_ids    = (int *) mxGetData( prhs[8] );
            if( buffer_ids == NULL ) { mexErrMsgTxt("Error:  Could not convert input buffer IDs to array of uint32."); }
            
            num_buffers = (int) mxGetN( prhs[8] );
            
            // Sample IQ Buffer
            if ( mxGetN( prhs[7] ) != num_buffers ) { mexErrMsgTxt("Error: Sample buffer input must be a column vector."); }
            sample_buffer = prhs[7];
            if( sample_buffer == NULL ) { mexErrMsgTxt("Error:  Could not convert sample buffer input to array"); }

#if 0
            // Print command arguments    
            printf("index = %d, length = %d, port = %d, ip_addr = %s \n", handle, max_length, port, ip_addr);
            printf("num_sample = %d, start_sample = %d\n", num_samples, start_sample);
            printf("num_pkts = %d, max_samples = %d \n", num_pkts, max_samples);
            printf("hw_ver = %d, data_type = %d \n", hw_ver, data_type);
#endif
            
            // Allocate return variables
            plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
            
            plhs[1] = mxCreateDoubleMatrix (1, num_buffers, mxREAL);
            if( plhs[1] == NULL ) { mexErrMsgTxt("Error:  Could not allocate return buffer"); }            
            checksum_array = mxGetPr(plhs[1]);

            
            // Iterate thru all the buffers that have been requested
            for (k = 0; k < num_buffers; k++) {
            
                // Set the buffer ID for this Write IQ
                buffer_id = buffer_ids[k];

                // Call function
                //     NOTE:  Since we cannot decode the sample buffer in this part of the function,
                //        we need to pass down the iteration we are on so that the lower function can
                //        know the index into the sample buffer:  (k * num_samples * data_size)
                //
                size = wl_write_baseband_buffer( handle, buffer, max_length, ip_addr, port,
                                                 num_samples, start_sample, sample_buffer, buffer_id, 
                                                 num_pkts, max_samples, hw_ver, check_chksum, data_type, k,
                                                 &num_cmds, &checksum );

                // Check that we actually sent some samples
                if ( size == 0 ) {
                    mexErrMsgTxt("Error:  Did not send any samples");
                }
                                                 
                // Update the output checksum array
                checksum_array[k] = checksum;
            }

            // Return value to MABLAB
            *mxGetPr(plhs[0]) = num_cmds;
            
            // Free allocated memory
            free( ip_addr );
            
#ifdef _DEBUG_
            printf("END TRANSPORT_WRITE_IQ\n");
#endif        
        break;

        
        //------------------------------------------------------
        // wl_mex_udp_transport('write_iq_set_pkt_wait_time', wait_time)
        //   - Arguments:
        //     - wait_time (int) - wait_time value (in us)
        //   - Returns:
        //     - none
        case TRANSPORT_WRITE_IQ_SET_PKT_WAIT_TIME :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_WRITE_IQ_SET_PKT_WAIT_TIME\n");
#endif
            // Validate arguments
            if( nrhs != 2 ) { print_usage(); die(); }
            if( nlhs != 0 ) { print_usage(); die(); }

            // Get input arguments
            wait_time = (int) mxGetScalar(prhs[1]);

            // Set the global variables
            use_user_write_iq_wait_time = 1;
            user_write_iq_wait_time = wait_time;
        
#ifdef _DEBUG_
            printf("END TRANSPORT_WRITE_IQ_SET_PKT_WAIT_TIME \n");
#endif
        break;


        //------------------------------------------------------
        // wl_mex_udp_transport('read_iq_set_max_request_size', size)
        //   - Arguments:
        //     - size (int) - Request size value (in bytes)
        //   - Returns:
        //     - none
        case TRANSPORT_READ_IQ_SET_MAX_REQUEST_SIZE :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_READ_IQ_SET_MAX_REQUEST_SIZE\n");
#endif
            // Validate arguments
            if( nrhs != 2 ) { print_usage(); die(); }
            if( nlhs != 0 ) { print_usage(); die(); }

            // Get input arguments
            size = (int) mxGetScalar(prhs[1]);

            // Set the global variables
            use_user_read_iq_max_req_size = 1;
            user_read_iq_max_req_size = size;
        
#ifdef _DEBUG_
            printf("END TRANSPORT_READ_IQ_SET_MAX_REQUEST_SIZE \n");
#endif
        break;


        //------------------------------------------------------
        // wl_mex_udp_transport('suppress_iq_warnings')
        //   - Arguments:
        //     - none
        //   - Returns:
        //     - none
        case TRANSPORT_SUPPRESS_IQ_WARNINGS :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_SUPPRESS_IQ_WARNINGS\n");
#endif
            // Validate arguments
            if( nrhs != 1 ) { print_usage(); die(); }
            if( nlhs != 0 ) { print_usage(); die(); }

            // Set the global variables
            suppress_iq_warnings = 1;
        
#ifdef _DEBUG_
            printf("END TRANSPORT_SUPPRESS_IQ_WARNINGS \n");
#endif
        break;


        //------------------------------------------------------
        //  Default
        //
        //  Print error message
        //
        default:
            printf("Error:  Function %s not supported.", func);
            mexErrMsgTxt("Error:  Function not supported.");
        break; 
    }

    // Free allocated memory
    mxFree( func );
    
#ifdef _DEBUG_
    printf("DONE \n");
#endif

    // if ( function == TRANSPORT_READ_IQ) {
    //     wl_usleep(5000000);
    // }

    // Return back to MATLAB
    return;
}




/*****************************************************************************/
//           Additional WL Mex UDP Transport functionality
/*****************************************************************************/


/*****************************************************************************/
/**
*
* This function will receive a packet
*
* @param	index          - Index in to socket structure which will receive samples
* @param	buffer         - WARPLab command to request samples
* @param	length         - Length (in bytes) of buffer
* @param    ip_addr        - IP Address of node to retrieve samples
* @param    port           - Port of node to retrieve samples
* @param    num_samples    - Number of samples to process (should be the same as the argument in the WARPLab command)
* @param    start_sample   - Index of starting sample (should be the same as the agrument in the WARPLab command)
* @param    buffer_id      - Which buffer(s) do we need to retrieve samples from
* @param    output_array   - Return parameter - array of samples to return
* @param    num_cmds       - Return parameter - number of ethernet send commands used to request packets 
*                                (could be > 1 if there are transmission errors)
*
* @return	size           - Number of samples processed (also size of output_array)
*
* @note		This function requires the following pointers:
*   Input:
*       - Buffer containing command to request samples (char *)
*       - IP Address (char *)
*       - Array of buffer IDs (uint32 *)
*       - Allocated memory to return samples (uint32 *)
*       - Location to return the number of send commands used to generate the samples (uint32 *)
*
*   Output:
*       - Number of samples processed 
*
******************************************************************************/
int wl_receive_response( int index, 
                         char *resp_buffer, int resp_buffer_size, uint32 *resp_time, 
                         char *cmd_buffer,  int cmd_buffer_size, char *ip_addr, int port, wl_function_ptr_t cmd_update_callback
                       ) {

    int                      done                = 0;
    uint32                   timeout             = 0;
    uint32                   num_retries         = 0;
    uint32                   num_wait_retries    = 0;
    int                      sent_size           = 0;
    int                      rcvd_size           = 0;
    uint32                   total_cmds          = 0;

    wl_transport_header     *rcvd_transport_hdr;
    

    // Try to process the return packet
    while ( !done ) {

        // If we hit the timeout, then resend the command
        if ( timeout >= TRANSPORT_TIMEOUT ) {
        
            // If we hit the max number of retrys, then abort
            if ( num_retries >= TRANSPORT_MAX_RETRY ) {

                printf("ERROR:  Exceeded %d retries for current command request\n", TRANSPORT_MAX_RETRY);
                die_with_error("Error:  Reached maximum number of retries without a response... aborting.");
                
            } else {            
                // Retransmit the command packet

                // See if there are any updates required                
                cmd_update_callback( cmd_buffer, cmd_buffer_size, ip_addr, port );
                
                // Send the packet
                sent_size    = send_socket( index, cmd_buffer, cmd_buffer_size, ip_addr, port );
                total_cmds  += 1;
                
                // Update control variables
                timeout      = 0;
                num_retries += 1;
            }
        }
        
        // Receive packet
        rcvd_size = receive_socket( index, resp_buffer_size, resp_buffer );

        // receive_socket() handles all socket related errors and will only return:
        //   - zero if no packet is available
        //   - non-zero if packet is available
        if ( rcvd_size > 0 ) {
            // Decode the ethernet packet
            rcvd_transport_hdr  = (wl_transport_header *) resp_buffer;
            
            // Record the response timeout value
            *resp_time   = timeout;

            // Reset the timeout regardless of the contents of the packet
            timeout      = 0;

            // Check the transport header to see if we should wait
            if ( (rcvd_transport_hdr->flags & TRANSPORT_HDR_NODE_NOT_READY_FLAG) == TRANSPORT_HDR_NODE_NOT_READY_FLAG ) {
            
                // Node is not ready; Wait and try again
                wl_usleep( TRANSPORT_NOT_READY_WAIT_TIME );                
                num_wait_retries += 1;

                // Send packet to request samples
                sent_size   = send_socket( index, cmd_buffer, cmd_buffer_size, ip_addr, port );
                total_cmds += 1;

                // Check that we have not spent a "long time" waiting for samples to be ready                
                if ( num_wait_retries > TRANSPORT_NOT_READY_MAX_RETRY ) {
                    die_with_error("Error:  Timeout waiting for node to be ready.  Please check the node operation.");
                }            
            } else {
                done = 1;
            }
        }
    }
    
    return total_cmds;
}



/*****************************************************************************/
/**
*
* This function will read the baseband buffers and construct the sample array
*
* @param	index          - Index in to socket structure which will receive samples
* @param	buffer         - WARPLab command to request samples
* @param	length         - Length (in bytes) of buffer
* @param    ip_addr        - IP Address of node to retrieve samples
* @param    port           - Port of node to retrieve samples
* @param    initial_offset - Initial offset of the Read IQ request
* @param    num_samples    - Number of samples to process (should be the same as the argument in the WARPLab command)
* @param    start_sample   - Index of starting sample (this changes when "chunking")
* @param    buffer_id      - Which buffer(s) do we need to retrieve samples from
* @param    function       - Function that we are reading data for:
*                                Values = [TRANSPORT_READ_IQ, TRANSPORT_READ_RSSI]
* @param    data_type      - Type of the output array:
*                                IQ_DATA_TYPE_DOUBLE ==> double / mxDOUBLE_CLASS
                                 IQ_DATA_TYPE_SINGLE ==> float  / mxSINGLE_CLASS
                                 IQ_DATA_TYPE_INT16  ==> short  / mxINT16_CLASS
                                 IQ_DATA_TYPE_RAW    ==> uint32 / mxUINT32_CLASS
* @param    output_array   - Return parameter - array of samples to return
* @param    num_cmds       - Return parameter - number of ethernet send commands used to request packets 
*                                (could be > 1 if there are transmission errors)
*
* @return	size           - Number of samples processed (also size of output_array)
*
* @note		This function requires the following pointers:
*   Input:
*       - Buffer containing command to request samples (char *)
*       - IP Address (char *)
*       - Array of buffer IDs (uint32 *)
*       - Allocated memory to return samples (uint32 *)
*       - Location to return the number of send commands used to generate the samples (uint32 *)
*
*   Output:
*       - Number of samples processed 
*
*
* @note    BB_READ_IQ / BB_READ_RSSI Packet Format:
*
* Command Packet
*    - cmd_args_32[0]      - Buffer selection (guaranteed to be singular) (uint16)
*    - cmd_args_32[1]      - Start sample
*    - cmd_args_32[2]      - Total samples in transfer
*    - cmd_args_32[3]      - Maximum number of samples per packet
*    - cmd_args_32[4]      - Number of packets in transfer
*    - cmd_args_32[5]      - IQ ID (uint 8) - Populated at the lower level
*
* Response Packet
*    - resp_args           - Samples:  wl_bb_samp_hdr followed by appropriate samples
*
*    NOTE:  If the sample header flags == SAMPLE_HDR_FLAG_IQ_NOT_READY, then the "samples"
*        after the sample header need to be interpreted in the following manner:
*
*        - sample[0]       - Tx status
*        - sample[1]       - Current Tx read pointer
*        - sample[2]       - Tx length
*        - sample[3]       - Rx status
*        - sample[4]       - Current Rx write pointer
*        - sample[5]       - Rx length
*
******************************************************************************/
int wl_read_baseband_buffer( int index, char *buffer, int length, char *ip_addr, int port,
                             uint32 initial_offset, uint32 num_samples, uint32 start_sample, uint32 buffer_id, 
                             uint32 function, uint32 data_type,
                             void **output_array, uint32 *num_cmds, uint32 *seq_num) {

    // Variable declaration
    uint32                   i, tmp;
    int                      done                = 0;
    
    uint32                   buffer_id_cmd       = 0;
    uint32                   start_sample_cmd    = 0;
    uint32                   total_sample_cmd    = 0;
    uint32                   bytes_per_pkt       = 0;
    uint32                   num_pkts            = 0;
    
    uint32                   tmp_eth_buffer_size = 0;
    uint32                   samples_per_pkt     = 0;

    int                      sent_size           = 0;

    uint32                   rcvd_pkts           = 0;
    int                      rcvd_size           = 0;
    uint32                   num_rcvd_samples    = 0;
    uint32                   sample_num          = 0;
    uint32                   sample_size         = 0;
    uint8                    sample_flags        = 0;
    uint8                    sample_iq_id        = 0;
    
    uint32                   timeout             = 0;
    uint32                   num_retrys          = 0;
    uint32                   num_iq_retrys       = 0;

    uint32                   total_cmds          = 0;
    
    uint32                   err_start_sample    = 0;
    uint32                   err_num_samples     = 0;
    uint32                   err_num_pkts        = 0;
    
    uint32                   iq_busy_warn        = 1;
    uint32                   wait_time           = 0;
    
    char                    *tmp_eth_buffer;
    uint8                   *samples;

    // Variables for the different output types    
    double                  *tmp_double_array_0;
    double                  *tmp_double_array_1;
    double                   tmp_double_val;

    float                   *tmp_single_array_0;
    float                   *tmp_single_array_1;
    float                    tmp_single_val;
    
    int16                   *tmp_int16_array_0;
    int16                   *tmp_int16_array_1;
    
    uint32                  *tmp_uint32_array_0;

    wl_transport_header     *transport_hdr;
    wl_transport_header     *rcvd_transport_hdr;
    wl_command_header       *command_hdr;
    uint32                  *command_args;
    wl_sample_header        *sample_hdr;
    wl_sample_tracker       *sample_tracker;

    // Read statistics
    uint32                   total_timeout     = 0;
    uint32                   init_timeout      = 0;
    uint32                   avg_timeout       = 0;
    
    // Compute some constants to be used later
    uint32                   tport_hdr_size    = sizeof( wl_transport_header );
    uint32                   cmd_hdr_size      = sizeof( wl_transport_header ) + sizeof( wl_command_header );
    uint32                   all_hdr_size      = sizeof( wl_transport_header ) + sizeof( wl_command_header ) + sizeof( wl_sample_header );

    // Initialization
    transport_hdr       = (wl_transport_header *) buffer;
    command_hdr         = (wl_command_header   *) ( buffer + tport_hdr_size );
    command_args        = (uint32              *) ( buffer + cmd_hdr_size );
    
    buffer_id_cmd       = endian_swap_32( command_args[0] );
    start_sample_cmd    = endian_swap_32( command_args[1] );
    total_sample_cmd    = endian_swap_32( command_args[2] );
    bytes_per_pkt       = endian_swap_32( command_args[3] );         // Command contains payload size; add room for header
    num_pkts            = endian_swap_32( command_args[4] );
    
    tmp_eth_buffer_size = bytes_per_pkt + 100;                       // Command contains payload size; add room for header
    samples_per_pkt     = ( bytes_per_pkt >> 2 );                    // Each WARPLab sample is 4 bytes
    
    // Replace IQ ID with value maintained by the transport
    sample_iq_id        = (sample_read_iq_id & 0xFF);
    command_args[5]     = endian_swap_32( sample_iq_id );
    
    // Increment read IQ ID (explicitly maintain as a uint8)
    sample_read_iq_id   = (sample_read_iq_id + 1) % 0x100;
    
#ifdef _DEBUG_
    // Print command arguments    
    printf("Read IQ / Read RSSI command\n");
    printf("    index = %d, length = %d, port = %d, ip_addr = %s \n", index, length, port, ip_addr);
    printf("    num_sample = %d, start_sample = %d, buffer_id = %d \n", num_samples, start_sample, buffer_id);
    printf("    bytes_per_pkt = %d;  num_pkts = %d \n", bytes_per_pkt, num_pkts );
    // print_buffer( buffer, length );
#endif

    // Perform a consistency check to make sure parameters are correct
    if ( buffer_id_cmd != buffer_id ) {
        printf("WARNING:  Buffer ID in command (%d) does not match function parameter (%d)\n", buffer_id_cmd, buffer_id);
    }
    if ( start_sample_cmd != start_sample ) {
        printf("WARNING:  Starting sample in command (%d) does not match function parameter (%d)\n", start_sample_cmd, start_sample);
    }
    if ( total_sample_cmd != num_samples ) {
        printf("WARNING:  Number of samples requested in command (%d) does not match function parameter (%d)\n", total_sample_cmd, num_samples);
    }
    
    // Malloc temporary buffer to process ethernet packets
    tmp_eth_buffer  = (char *) malloc( sizeof( char ) * tmp_eth_buffer_size );
    if( tmp_eth_buffer == NULL ) { die_with_error("Error:  Could not allocate temporary Ethernet packet buffer"); }
    
    // Malloc temporary array to track samples that have been received and initialize
    sample_tracker = (wl_sample_tracker *) malloc( sizeof( wl_sample_tracker ) * num_pkts );
    if( sample_tracker == NULL ) { die_with_error("Error:  Could not allocate sample tracker buffer"); }
    for ( i = 0; i < num_pkts; i++ ) { sample_tracker[i].start_sample = 0;  sample_tracker[i].num_samples = 0; }
    
    // Send packet to request samples
    sent_size   = send_socket( index, buffer, length, ip_addr, port );
    total_cmds += 1;

    // Initialize loop variables
    rcvd_pkts = 0;
    timeout   = 0;
    
    // Process each return packet
    while ( !done ) {
        
        // If we hit the timeout, then try to re-request the remaining samples
        if ( timeout >= TRANSPORT_TIMEOUT ) {
        
            // If we hit the max number of retrys, then abort
            if ( num_retrys >= TRANSPORT_MAX_RETRY ) {

                printf("ERROR:  Exceeded %d retrys for current Read IQ / Read RSSI request \n", TRANSPORT_MAX_RETRY);
                printf("    Requested %d samples from buffer %d starting from sample number %d \n", num_samples, buffer_id, start_sample);
                printf("    Received %d out of %d packets from node before timeout.\n", rcvd_pkts, num_pkts);
                printf("    Please check the node and look at the ethernet traffic to isolate the issue. \n");                
            
                die_with_error("Error:  Reached maximum number of retrys without a response... aborting.");
                
            } else {

                // NOTE:  We print a warning here because the Read IQ / Read RSSI case in the mex function above
                //        will split Read IQ / Read RSSI requests based on the receive buffer size.  Therefore,
                //        any timeouts we receive here should be legitimate issues that should be explored.
                //
                if ( suppress_iq_warnings == 0 ) {
                    printf("WARNING:  Read IQ / Read RSSI request timed out.  Retrying remaining samples. \n");
                    printf("          If this message occurs frequently, please adjust the Read IQ \n");
                    printf("          maximum request size (in bytes) for the transport using the \n");
                    printf("          M code function:  \n");
                    printf("              wl_mex_udp_transport('read_iq_set_max_request_size', size)  \n");
                    printf("          Defaults to 80 percent of the receive buffer allocated by the OS. \n");
                    printf("\n          To suppress all IQ warnings for the transport use the M code function: \n");
                    printf("              wl_mex_udp_transport('suppress_iq_warnings')\n");
                }
            
                // Find the first packet error and request the remaining samples
                if ( wl_read_iq_find_error( sample_tracker, num_samples, start_sample, rcvd_pkts, samples_per_pkt,
                                            &err_num_samples, &err_start_sample, &err_num_pkts ) ) {
                    
                    command_args[1] = endian_swap_32( err_start_sample );
                    command_args[2] = endian_swap_32( err_num_samples );
                    command_args[4] = endian_swap_32( num_pkts - ( rcvd_pkts - err_num_pkts ) );

                    // Since there was an error in the packets we have already received, then we need to adjust rcvd_pkts
                    rcvd_pkts        -= err_num_pkts;
                    num_rcvd_samples  = num_samples - err_num_samples;

                } else {
                    // If we did not find an error, then the first rcvd_pkts are correct and we should request
                    //   the remaining packets
                    command_args[1] = endian_swap_32( err_start_sample );
                    command_args[2] = endian_swap_32( err_num_samples );
                    command_args[4] = endian_swap_32( num_pkts - rcvd_pkts );
                }

                // Retransmit the read IQ request packet
                sent_size   = send_socket( index, buffer, length, ip_addr, port );
                total_cmds += 1;
                
                // Update control variables
                timeout     = 0;
                num_retrys += 1;
            }
        }
        
        // Receive packet
        rcvd_size = receive_socket( index, tmp_eth_buffer_size, tmp_eth_buffer );

        // receive_socket() handles all socket related errors and will only return:
        //   - zero if no packet is available
        //   - non-zero if packet is available
        if ( rcvd_size > 0 ) {
            // Decode the Ethernet packet
            rcvd_transport_hdr  = (wl_transport_header *) tmp_eth_buffer;
            sample_hdr          = (wl_sample_header    *) (tmp_eth_buffer + cmd_hdr_size);

            // Decode the sample header
            sample_num          = endian_swap_32( sample_hdr->start ) - initial_offset;
            sample_size         = endian_swap_32( sample_hdr->num_samples );
            sample_flags        = sample_hdr->flags;
            
#ifdef _DEBUG_
            // Record the timeout value for statistics
            if ( rcvd_pkts == 0 ) {
                init_timeout   = timeout;
            } else {
                total_timeout += timeout;
            }
            
            // Print information about the last 10 packets
            if ((rcvd_pkts > (num_pkts - 10)) || (num_pkts < 10 )) {
                printf("num_sample = %d, start_sample = %d   %08x  %08x %08x %08x\n", sample_size, sample_num, sample_hdr->num_samples, sample_size, sample_hdr->start, sample_num);
            }
#endif

            // Reset the timeout regardless of the contents of the packet
            timeout = 0;

            // Check the sample header flags
            if ((sample_flags & SAMPLE_IQ_ERROR) == SAMPLE_IQ_ERROR) {
                // Error in samples print error message and return
                die_with_error("Error:  Node returned 'SAMPLE_IQ_ERROR'.  Check that node is not currently transmitting in continuous TX mode.");
            
            } else if ((sample_flags & SAMPLE_IQ_NOT_READY) == SAMPLE_IQ_NOT_READY) {
                if ( iq_busy_warn ) {
                    printf("WARNING:  Node was not ready to process Read IQ request.  Waiting to request again.\n");
                    printf("    This warning can be removed by waiting until the node is not busy with a TX or RX \n");
                    printf("    operation.  To do this, please add 'pause(1.5 * NUM_SAMPLES * 1/(40e6));' after\n");
                    printf("    any triggers and before the Read IQ request.\n\n");
                    iq_busy_warn = 0;
                }
                
                wait_time = wl_compute_sample_wait_time((uint32 *)(tmp_eth_buffer + all_hdr_size));
                
                // Wait until the samples should be done
                if ( wait_time != 0 ) {
                    wl_usleep( wait_time + 100 );
                }
                
                num_iq_retrys += 1;
                
                // Send packet to request samples
                sent_size   = send_socket( index, buffer, length, ip_addr, port );
                total_cmds += 1;

                if ( sent_size != length ) {
                    die_with_error("Error:  Size of packet sent to request samples does not match length of packet.");
                }

                // Check that we have not spent a "long time" waiting for samples to be ready                
                if ( num_iq_retrys > SAMPLE_IQ_MAX_RETRY ) {
                    die_with_error("Error:  Timeout waiting for node to return samples.  Please check the node operation.");
                }
            } else {
                // Normal IQ data
                
                // Set a pointer to the sample data
                samples      = (uint8 *) ( tmp_eth_buffer + all_hdr_size );
                
                // If we are tracking packets, record which samples have been received
                sample_tracker[rcvd_pkts].start_sample = sample_num + initial_offset;
                sample_tracker[rcvd_pkts].num_samples  = sample_size;
                
                // Place samples in the array (Ethernet packet is uint8 big endian, output array is various types little endian) 
                //   NOTE: Need to process samples in the correct order
                //   NOTE: Need to process differently based on the data type
                
                switch (data_type) {
                    // ------------------------------------------------------------------------------------------------
                    case IQ_DATA_TYPE_DOUBLE:
                        // Need to process the Ethernet packet into a double precision floating point array
                        //    NOTE:  This performs an endian swap (big to little) on both IQ and RSSI data
                        //    NOTE:  This will convert IQ data from a UFix_16_0 to a Fix_16_15
                        //    NOTE:  This will unpack the RSSI sample
                        // 
                        switch ( function ) {
                            case TRANSPORT_READ_IQ:
                                tmp_double_array_0 = (double *) output_array[0];
                                tmp_double_array_1 = (double *) output_array[1];
                                
                                for( i = 0; i < (4 * sample_size); i += 4 ) {
                                    tmp = sample_num + (i / 4);
                                    
                                    // Unpack the WARPLab IQ sample
                                    //   NOTE:  This performs a conversion from an UFix_16_0 to a Fix_16_15
                                    //      Process:
                                    //          1) Treat the 16 bit unsigned value as a 16 bit two's compliment signed value
                                    //          2) Divide by range / 2 to move the decimal point so resulting value is between +/- 1
                                    
                                    // I samples
                                    tmp_double_val = (double) ((int16)((samples[i    ] << 8) | (samples[i + 1])));
                                    tmp_double_array_0[tmp] = ( tmp_double_val / 0x8000 );
                                    
                                    // Q samples
                                    tmp_double_val = (double) ((int16)((samples[i + 2] << 8) | (samples[i + 3])));
                                    tmp_double_array_1[tmp] = ( tmp_double_val / 0x8000 );
                                }
                            break;
                            
                            case TRANSPORT_READ_RSSI:
                                tmp_double_array_0 = (double *) output_array[0];
                                
                                for( i = 0; i < (4 * sample_size); i += 4 ) {
                                    tmp = (sample_num + (i / 4)) * 2;
                                    
                                    // Unpack the WARPLab RSSI sample
                                    //   NOTE:  This will place the packed 12 bit RSSI samples in the output array
                                    tmp_double_array_0[tmp    ] = (double)(((samples[i    ] << 8) | (samples[i + 1])) & 0x03FF);
                                    tmp_double_array_0[tmp + 1] = (double)(((samples[i + 2] << 8) | (samples[i + 3])) & 0x03FF);
                                }
                            break;
                            
                            default:
                                printf("ERROR:  Unsupported function for read_buffers in MEX transport\n");
                            break;
                        }
                    break;
                    
                    // ------------------------------------------------------------------------------------------------
                    case IQ_DATA_TYPE_SINGLE:
                        // Need to process the Ethernet packet into a single precision floating point array
                        //     NOTE:  This is exactly the same processing as the 'double' case but using float 
                        //            instead of double for the output type
                        //
                        switch ( function ) {
                            case TRANSPORT_READ_IQ:
                                tmp_single_array_0 = (float *) output_array[0];
                                tmp_single_array_1 = (float *) output_array[1];
                                
                                for( i = 0; i < (4 * sample_size); i += 4 ) {
                                    tmp = sample_num + (i / 4);
                                    
                                    // Unpack the WARPLab IQ sample
                                    //   NOTE:  This performs a conversion from an UFix_16_0 to a Fix_16_15 
                                    //      Process:
                                    //          1) Treat the 16 bit unsigned value as a 16 bit two's compliment signed value
                                    //          2) Divide by range / 2 to move the decimal point so resulting value is between +/- 1
                                    
                                    // I samples
                                    tmp_single_val = (float) ((int16)((samples[i    ] << 8) | (samples[i + 1])));
                                    tmp_single_array_0[tmp] = ( tmp_single_val / 0x8000 );
                                    
                                    // Q samples
                                    tmp_single_val = (float) ((int16)((samples[i + 2] << 8) | (samples[i + 3])));
                                    tmp_single_array_1[tmp] = ( tmp_single_val / 0x8000 );
                                }
                            break;
                            
                            case TRANSPORT_READ_RSSI:
                                tmp_single_array_0 = (float *) output_array[0];
                                
                                for( i = 0; i < (4 * sample_size); i += 4 ) {
                                    tmp = (sample_num + (i / 4)) * 2;
                                    
                                    // Unpack the WARPLab RSSI sample
                                    //   NOTE:  This will place the packed 12 bit RSSI samples in the output array
                                    tmp_single_array_0[tmp    ] = (float)(((samples[i    ] << 8) | (samples[i + 1])) & 0x03FF);
                                    tmp_single_array_0[tmp + 1] = (float)(((samples[i + 2] << 8) | (samples[i + 3])) & 0x03FF);
                                }
                            break;
                            
                            default:
                                printf("ERROR:  Unsupported function for read_buffers in MEX transport\n");
                            break;
                        }
                    break;
                    
                    // ------------------------------------------------------------------------------------------------
                    case IQ_DATA_TYPE_INT16:
                        // Need to process the Ethernet packet into a int16 array
                        //    NOTE:  This performs an endian swap (big to little) on both IQ and RSSI data
                        //    NOTE:  This will convert IQ data from a UFix_16_0 to a Fix_16_0
                        //    NOTE:  This will unpack the RSSI sample
                        //
                        switch ( function ) {
                            case TRANSPORT_READ_IQ:
                                tmp_int16_array_0 = (int16 *) output_array[0];
                                tmp_int16_array_1 = (int16 *) output_array[1];
                                
                                for( i = 0; i < (4 * sample_size); i += 4 ) {
                                    tmp = sample_num + (i / 4);
                                    
                                    // Unpack the WARPLab IQ sample
                                    //   NOTE:  This performs a conversion from an UFix_16_0 to a Fix_16_0 
                                    //      Process:
                                    //          1) Treat the 16 bit unsigned value as a 16 bit two's compliment signed value
                                    
                                    // I samples
                                    tmp_int16_array_0[tmp] = (int16)((samples[i    ] << 8) | (samples[i + 1]));
                                    
                                    // Q samples
                                    tmp_int16_array_1[tmp] = (int16)((samples[i + 2] << 8) | (samples[i + 3]));
                                }
                            break;
                            
                            case TRANSPORT_READ_RSSI:
                                tmp_int16_array_0 = (int16 *) output_array[0];
                                
                                for( i = 0; i < (4 * sample_size); i += 4 ) {
                                    tmp = (sample_num + (i / 4)) * 2;
                                    
                                    // Unpack the WARPLab RSSI sample
                                    //   NOTE:  This will place the packed 12 bit RSSI samples in the output array
                                    tmp_int16_array_0[tmp    ] = (int16)(((samples[i    ] << 8) | (samples[i + 1])) & 0x03FF);
                                    tmp_int16_array_0[tmp + 1] = (int16)(((samples[i + 2] << 8) | (samples[i + 3])) & 0x03FF);
                                }
                            break;
                            
                            default:
                                printf("ERROR:  Unsupported function for read_buffers in MEX transport\n");
                            break;
                        }
                    break;
                    
                    // ------------------------------------------------------------------------------------------------
                    case IQ_DATA_TYPE_RAW:
                        // Need to process the Ethernet packet into a uint32 array
                        //    NOTE:  This performs an endian swap (big to little) on both IQ and RSSI data
                        //    NOTE:  No other processing is done on the data
                        //
                        switch ( function ) {
                            case TRANSPORT_READ_IQ:
                            case TRANSPORT_READ_RSSI:
                                tmp_uint32_array_0 = (uint32 *) output_array[0];
                                
                                for( i = 0; i < (4 * sample_size); i += 4 ) {
                                    tmp_uint32_array_0[ sample_num + (i / 4) ] = (uint32) ( (samples[i] << 24) | (samples[i + 1] << 16) | (samples[i + 2] << 8) | (samples[i + 3]) );
                                }
                            break;
                            
                            default:
                                printf("ERROR:  Unsupported function for read_buffers in MEX transport\n");
                            break;
                        }
                    break;
                    
                    // ------------------------------------------------------------------------------------------------
                    default:
                        mexErrMsgTxt("Error:  Unsupported output data type");
                    break;
                }
    
                num_rcvd_samples += sample_size;
                rcvd_pkts        += 1;
                num_iq_retrys     = 0;
                
                // Exit the loop when we have enough packets
                if ( rcvd_pkts == num_pkts ) {
                
#ifdef _DEBUG_
                    // Calculate statistics
                    if (num_pkts > 1) {
                        avg_timeout = total_timeout / (num_pkts - 1);
                    } else {
                        avg_timeout = init_timeout;
                    }
                    
                    // Print statistics
                    printf("Initial Timeout = %10d\n", init_timeout);
                    printf("Avg Timeout     = %10d  (%10d packets)\n", avg_timeout, (num_pkts - 1));
#endif
                
                    // Check to see if we have any packet errors
                    //     NOTE:  This check will detect duplicate packets or sample indexing errors
                    if ( wl_read_iq_sample_error( sample_tracker, num_samples, start_sample, rcvd_pkts, samples_per_pkt ) ) {

                        // In this case, there is probably some issue in the transmitting node not getting the
                        // correct number of samples or messing up the indexing of the transmit packets.  
                        // The wl_read_iq_sample_error() printed debug information, so we need to retry the packets
                        
                        if ( num_retrys >= TRANSPORT_MAX_RETRY ) {
                        
                            die_with_error("Error:  Errors in sample request from board.  Max number of re-transmissions reached.  See above for debug information.");
                            
                        } else {

                            // Find the first packet error and request the remaining samples
                            if ( wl_read_iq_find_error( sample_tracker, num_samples, start_sample, rcvd_pkts, samples_per_pkt,
                                                        &err_num_samples, &err_start_sample, &err_num_pkts ) ) {

                                command_args[1] = endian_swap_32( err_start_sample );
                                command_args[2] = endian_swap_32( err_num_samples );
                                command_args[4] = endian_swap_32( num_pkts - ( rcvd_pkts - err_num_pkts ) );
                                // command_args[4] = endian_swap_32( err_num_pkts );

                                // Retransmit the read IQ request packet
                                sent_size   = send_socket( index, buffer, length, ip_addr, port );
                                
                                if ( sent_size != length ) {
                                    die_with_error("Error:  Size of packet sent to request samples does not match length of packet.");
                                }
                                
                                // We are re-requesting err_num_pkts, so we need to subtract err_num_pkts from what we have already recieved
                                rcvd_pkts        -= err_num_pkts;
                                num_rcvd_samples  = num_samples - err_num_samples;

                                // Update control variables
                                timeout     = 0;
                                total_cmds += 1;
                                num_retrys += 1;
                                
                            } else {
                                // Die since we could not find the error
                                die_with_error("Error:  Encountered error in sample packets but could not determine the error.  See above for debug information.");
                            }
                        }
                    } else {
                        // There are no errors, so we are done
                        //     Record sequence number and exit the function
                        *seq_num = sample_hdr->sample_iq_id;
                        
                        done = 1;
                    }
                }
            }  // END if (sample_flags)
        } else {       
            // Increment the timeout counter; Note this counter does not reflect real-time
            timeout += 1;            
            
        }  // END if ( rcvd_size > 0 )
        
    }  // END while( !done )

    
    // Free locally allocated memory    
    free( tmp_eth_buffer );    
    free( sample_tracker ); 

    // Finalize outputs   
    *num_cmds  += total_cmds;
    
    return num_rcvd_samples;
}



/*****************************************************************************/
/**
*  Function:  Read IQ sample check
*
*  Function to check if we received all the samples at the correct indexes
*
*  Returns:  0 if no errors
*            1 if if there is an error and prints debug information
*
******************************************************************************/
int wl_read_iq_sample_error( wl_sample_tracker *tracker, uint32 num_samples, uint32 start_sample, uint32 num_pkts, uint32 max_sample_size ) {

    uint32 i;
    uint32 start_sample_total;

    uint32 num_samples_sum  = 0;
    uint32 start_sample_sum = 0;

    // Compute the value of the start samples:
    //   We know that the start samples should follow the pattern:
    //       [ x, (x + y), (x + 2y), (x + 3y), ... , (x + (N - 1)y) ]
    //   where x = start_sample, y = max_sample_size, and N = num_pkts.  This is due
    //   to the fact that the node will fill all packets completely except the last packet.
    //   Therefore, the sum of all element in that array is:
    //       (N * x) + ((N * (N - 1) * Y) / 2
    //
    start_sample_total = (num_pkts * start_sample) + (((num_pkts * (num_pkts - 1)) * max_sample_size) >> 1); 
    
    // Compute the totals using the sample tracker
    for( i = 0; i < num_pkts; i++ ) {
    
        num_samples_sum  += tracker[i].num_samples;
        start_sample_sum += tracker[i].start_sample;
    }

    // Check the totals
    if ( ( num_samples_sum != num_samples ) || ( start_sample_sum != start_sample_total ) ) {
    
        if ( suppress_iq_warnings == 0 ) {
            // Print warning debug information if there is an error
            if ( num_samples_sum != num_samples ) { 
                printf("WARNING:  Number of samples received (%d) does not equal number of samples requested (%d).  \n", num_samples_sum, num_samples);
            } else {
                printf("WARNING:  Sample packet indexes not correct.  Expected the sum of sample indexes to be \n");
                printf("          (%d) but received a sum of (%d).  Retrying ...\n", start_sample_total, start_sample_sum);
            }
            // printf("\n          To suppress all IQ warnings for the transport use the M code function: \n");
            // printf("              wl_mex_udp_transport('suppress_iq_warnings')\n");
            
            // Print debug information
            printf("Packet Tracking Information: \n");
            printf("    Requested Samples:  Number: %8d    Start Sample: %8d  \n", num_samples, start_sample);
            printf("    Received  Samples:  Number: %8d  \n", num_samples_sum);
            
            // for ( i = 0; i < num_pkts; i++ ) {
            //     printf("         Packet[%4d]:   Number: %8d    Start Sample: %8d  \n", i, tracker[i].num_samples, tracker[i].start_sample );
            // }
        }

        return 1;
    } else {
        return 0;
    }
}



/*****************************************************************************/
/**
*  Function:  Read IQ find first error
*
*  Function to check if we received all the samples at the correct indecies.
*  This will also update the ret_* parameters to indicate values to use when 
*  requesting a new set of packets.
*
*  Returns:  0 if no error was found
*            1 if an error was found
*
******************************************************************************/
int wl_read_iq_find_error( wl_sample_tracker *tracker, uint32 num_samples, uint32 start_sample, uint32 num_pkts, uint32 max_sample_size,
                           uint32 *ret_num_samples, uint32 *ret_start_sample, uint32 *ret_num_pkts ) {

    uint32 i, j;

    uint32 value_found;
    uint32 return_value            = 1;
    uint32 start_sample_to_request = start_sample;
    uint32 num_samples_left        = num_samples;
    uint32 num_pkts_left           = num_pkts;

    // Iterate thru the array to find the first index that does not match the expected value
    //     NOTE:  This is performing a naive search and could be optimized.  Since we are 
    //         already in an error condition, we chose simplicity over performance.
    
    for( i = 0; i < num_pkts; i++ ) {
    
        value_found = 0;
    
        // Find element in the array   
        for ( j = 0; j < num_pkts; j ++ ) {
            if ( start_sample_to_request == tracker[i].start_sample ) {
                value_found = 1;
            }            
        }

        // If we did not find the value, then exit the loop
        if ( !value_found ) {
            break;
        }
        
        // Update element to search for
        start_sample_to_request += max_sample_size;
        num_samples_left        -= max_sample_size;
        num_pkts_left           -= 1;
    }

    // Set the return value
    if ( num_pkts_left == 0 ) {
        return_value  = 0;
    } 
        
    // Return parameters
    *ret_start_sample = start_sample_to_request;
    *ret_num_samples  = num_samples_left;
    *ret_num_pkts     = num_pkts_left;
    
    return return_value;
}



/*****************************************************************************/
/**
*  Function:  Sequence Number tracker
*
*  Functions to update and check the sequence numbers.
*
*  Returns:   None
*
******************************************************************************/
void wl_update_seq_num(uint32 function, uint32 buffer_id, uint32 seq_num, uint32 *seq_num_tracker) {

    switch ( function ) {
        case TRANSPORT_READ_IQ:
            if (buffer_id == BUFFER_ID_RFA) { seq_num_tracker[0] = seq_num; }
            if (buffer_id == BUFFER_ID_RFB) { seq_num_tracker[2] = seq_num; }
            if (buffer_id == BUFFER_ID_RFC) { seq_num_tracker[4] = seq_num; }
            if (buffer_id == BUFFER_ID_RFD) { seq_num_tracker[6] = seq_num; }
        break;
        
        case TRANSPORT_READ_RSSI:
            if (buffer_id == BUFFER_ID_RFA) { seq_num_tracker[1] = seq_num; }
            if (buffer_id == BUFFER_ID_RFB) { seq_num_tracker[3] = seq_num; }
            if (buffer_id == BUFFER_ID_RFC) { seq_num_tracker[5] = seq_num; }
            if (buffer_id == BUFFER_ID_RFD) { seq_num_tracker[7] = seq_num; }
        break;
        
        default:
            printf("ERROR:  Unsupported function for wl_update_seq_num in MEX transport\n");
        break;
    }
    
    // printf("Seq Num: %5d %5d %5d %5d %5d %5d %5d %5d\n", seq_num_tracker[0], seq_num_tracker[1], seq_num_tracker[2], seq_num_tracker[3], 
    //                                                      seq_num_tracker[4], seq_num_tracker[5], seq_num_tracker[6], seq_num_tracker[7]);
    
}



void wl_check_seq_num(uint32 function, char * node_id_str, uint32 buffer_id, uint32 seq_num, uint32 *seq_num_tracker, char *seq_num_severity) {
    uint32 seq_num_matches   = 0;
    uint32 severity          = 0xFFFF;
    char  *function_name     = NULL;
    char   iq_function[10]   = "read_iq\0";
    char   rssi_function[10] = "read_rssi\0";
    
    
    switch (function) {
        case TRANSPORT_READ_IQ:
            if ((buffer_id == BUFFER_ID_RFA) && (seq_num_tracker[0] == seq_num)) { seq_num_matches = 1; }
            if ((buffer_id == BUFFER_ID_RFB) && (seq_num_tracker[2] == seq_num)) { seq_num_matches = 1; }
            if ((buffer_id == BUFFER_ID_RFC) && (seq_num_tracker[4] == seq_num)) { seq_num_matches = 1; }
            if ((buffer_id == BUFFER_ID_RFD) && (seq_num_tracker[6] == seq_num)) { seq_num_matches = 1; }
            
            function_name = iq_function;
        break;
        
        case TRANSPORT_READ_RSSI:
            if ((buffer_id == BUFFER_ID_RFA) && (seq_num_tracker[1] == seq_num)) { seq_num_matches = 1; }
            if ((buffer_id == BUFFER_ID_RFB) && (seq_num_tracker[3] == seq_num)) { seq_num_matches = 1; }
            if ((buffer_id == BUFFER_ID_RFC) && (seq_num_tracker[5] == seq_num)) { seq_num_matches = 1; }
            if ((buffer_id == BUFFER_ID_RFD) && (seq_num_tracker[7] == seq_num)) { seq_num_matches = 1; }
            
            function_name = rssi_function;
        break;
        
        default:
            printf("ERROR:  Unsupported function for wl_check_seq_num in MEX transport\n");
        break;
    }
    
    // printf("%d:  Buffer %d:  Seq Num = %d matches %d\n", function, buffer_id, seq_num, seq_num_matches);
    
    // If the current sequence number matches the recorded sequence number, this means
    // that the buffer has already been read and the appropriate message should be sent
    if (seq_num_matches) {
        if (!strcmp(seq_num_severity, SEQ_NUM_MATCH_IGNORE ) && (severity == 0xFFFF)) { severity = 0; }
        if (!strcmp(seq_num_severity, SEQ_NUM_MATCH_WARNING) && (severity == 0xFFFF)) { severity = 1; }
        if (!strcmp(seq_num_severity, SEQ_NUM_MATCH_ERROR  ) && (severity == 0xFFFF)) { severity = 2; }
        
        switch (severity) {
            case 0:    // Do nothing
            break;
            
            case 1:    // Warning
                mexWarnMsgIdAndTxt("WARPLab:MEX_UDP_TRANSPORT", "%s Detected multiple reads of same %s waveform.  If this is unintentional, ensure Rx node triggers are configured correctly.", node_id_str, function_name);
            break;
            
            case 2:    // Error
                mexErrMsgIdAndTxt("WARPLab:MEX_UDP_TRANSPORT", "ERROR:  %s Detected multiple reads of same %s waveform.", node_id_str, function_name);
            break;
            
            default:
                mexErrMsgIdAndTxt("WARPLab:MEX_UDP_TRANSPORT", "ERROR:  %s Unknown sequence number error severity = %s", node_id_str, seq_num_severity);
            break;
        }
    }
}



/*****************************************************************************/
/**
* This function will write the baseband buffers 
*
* @param	index          - Index in to socket structure which will receive samples
* @param	buffer         - WARPLab command (includes transport header and command header)
* @param	length         - Length (in bytes) max data packet to send (Ethernet MTU size - Ethernet header)
* @param    ip_addr        - IP Address of node to retrieve samples
* @param    port           - Port of node to retrieve samples
* @param    num_samples    - Number of samples to process (should be the same as the argument in the WARPLab command)
* @param    start_sample   - Index of starting sample (should be the same as the agrument in the WARPLab command)
* @param    samples        - Array of IQ samples to be sent
* @param    buffer_id      - Which buffer(s) do we need to send samples to (all dimensionality of buffer_ids is handled by Matlab)
* @param    num_pkts       - Number of packets to transfer (precomputed by calling SW)
* @param    max_samples    - Max samples to send per packet (precomputed by calling SW)
* @param    hw_ver         - Hardware version of node
* @param    check_chksum   - Perform the robustness check of transmission in this function or assume the WriteIQ was successful.
* @param    data_type      - Data type of IQ samples to be sent
* @param    iteration      - Variable to help with indexing into samples array
* @param    num_cmds       - Return parameter - number of ethernet send commands used to request packets 
*                                (could be > 1 if there are transmission errors)
* @param    checksum       - Return parameter - WriteIQ checksum that was calculated
*
* @return	samples_sent   - Number of samples processed 
*
* @note		This function requires the following pointers:
*   Input:
*       - Buffer containing command header to send samples (char *)
*       - IP Address (char *)
*       - Buffer containing the samples to be sent (uint32 *)
*       - Location to return the number of send commands used to generate the samples (uint32 *)
*
*   Output:
*       - Number of samples processed 
*
* @note    BB_WRITE_IQ Packet Format:
*
*       - cmd_args            - Samples:  wl_bb_samp_hdr followed by the
*                                   defined number of samples.
*
*       - resp_args_32[0]     - Status
*                                 - CMD_PARAM_SUCCESS
*                                 - SAMPLE_HDR_FLAG_IQ_NOT_READY
*                                 - SAMPLE_HDR_FLAG_IQ_ERROR
*       - resp_args_32[1]     - Current checksum            (ignore if Status != CMD_PARAM_SUCCESS)
*       - resp_args_32[2]     - IQ ID                       (ignore if Status != SAMPLE_HDR_FLAG_IQ_NOT_READY)
*       - resp_args_32[2]     - Tx status                   (ignore if Status != SAMPLE_HDR_FLAG_IQ_NOT_READY)
*       - resp_args_32[3]     - Current Tx read pointer     (ignore if Status != SAMPLE_HDR_FLAG_IQ_NOT_READY)
*       - resp_args_32[4]     - Tx length                   (ignore if Status != SAMPLE_HDR_FLAG_IQ_NOT_READY)
*       - resp_args_32[5]     - Rx status                   (ignore if Status != SAMPLE_HDR_FLAG_IQ_NOT_READY)
*       - resp_args_32[6]     - Current Rx write pointer    (ignore if Status != SAMPLE_HDR_FLAG_IQ_NOT_READY)
*       - resp_args_32[7]     - Rx length                   (ignore if Status != SAMPLE_HDR_FLAG_IQ_NOT_READY)
*
*       NOTE:  When SAMPLE_HDR_FLAG_IQ_NOT_READY is returned, then the host
*              can compute the wait time by the following calculation:
*                  max(((tx_status) ? (tx_length - tx_read_pointer) : 0), ((rx_status) ? (rx_length - rx_write_pointer) : 0));
*
*       NOTE:  Node will return SAMPLE_HDR_FLAG_IQ_ERROR if in continuous Tx
*              mode since the node will never be ready.
*
******************************************************************************/
int wl_write_baseband_buffer( int index, char *buffer, int max_length, char *ip_addr, int port,
                              uint32 num_samples, uint32 start_sample, const void *samples, uint32 buffer_id, uint32 num_pkts, 
                              uint32 max_samples, uint32 hw_ver, uint32 check_chksum, uint32 data_type, uint32 iteration,
                              uint32 *num_cmds, uint32 *checksum ) {

    // Variable declaration
    uint32                i, j;
    int                   done                   = 0;
    int                   length                 = 0;
    int                   sent_size              = 0;
    uint32                sample_num             = 0;
    int                   offset                 = 0;
    int                   need_resp              = 0;
    int                   slow_write             = 0;
    uint16                transport_flags        = 0;
    uint32                timeout                = 0;
    int                   buffer_count           = 0;
    uint32                mex_data_type          = 0;
    uint32                data_size              = 0;
    uint8                 sample_iq_id           = 0;
    uint32                write_iq_ready_warn    = 1;
    
    // Variables to handle different input types
    double               *tmp_double_array_real;
    double               *tmp_double_array_imag;
    double                tmp_double_real        = 0.0;
    double                tmp_double_imag        = 0.0;

    float                *tmp_single_array_real;
    float                *tmp_single_array_imag;
    float                 tmp_single_real        = 0.0;
    float                 tmp_single_imag        = 0.0;
    
    int16                *tmp_int16_array_real;
    int16                *tmp_int16_array_imag;
    int16                 tmp_int16_real         = 0;
    int16                 tmp_int16_imag         = 0;
    
    uint32               *tmp_uint32_array;
    
    uint32                process_imag           = 1;
    
    // Response variables
    uint32                write_iq_response      = 0;
    
    // Packet checksum tracking
    uint32                local_checksum         = 0;

    // Keep track of packet sequence number
    //   NOTE: these are uint32 even though the actual seq_num is uint16 so we can keep track of 
    //       sending greater than 2^16 commands in one write IQ.
    uint32                seq_num                = 0;
    uint32                seq_start_num          = 0;

    // Receive Buffer variables
    int                   rcvd_size              = 0;
    int                   rcvd_max_size          = 100;
    int                   num_retrys             = 0;
    unsigned char        *rcvd_buffer;
    uint32               *resp_args;

    // Buffer of data for ethernet packet and pointers to components of the packet
    unsigned char        *send_buffer;
    wl_transport_header  *transport_hdr;
    wl_command_header    *command_hdr;
    wl_sample_header     *sample_hdr;
    uint32               *sample_payload;

    // Sleep timer
    uint32                wait_time;
        
    // Compute some constants to be used later
    uint32                tport_hdr_size         = sizeof( wl_transport_header );
    uint32                tport_hdr_size_np      = sizeof( wl_transport_header ) - TRANSPORT_PADDING_SIZE;
    uint32                cmd_hdr_size           = sizeof( wl_transport_header ) + sizeof( wl_command_header );
    uint32                cmd_hdr_size_np        = sizeof( wl_transport_header ) + sizeof( wl_command_header ) - TRANSPORT_PADDING_SIZE;
    uint32                all_hdr_size           = sizeof( wl_transport_header ) + sizeof( wl_command_header ) + sizeof( wl_sample_header );
    uint32                all_hdr_size_np        = sizeof( wl_transport_header ) + sizeof( wl_command_header ) + sizeof( wl_sample_header ) - TRANSPORT_PADDING_SIZE;


#ifdef _DEBUG_
    // Print command arguments    
    printf("index = %d, length = %d, port = %d, ip_addr = %s \n", index, length, port, ip_addr);
    printf("num_sample = %d, start_sample = %d, buffer_id = %d \n", num_samples, start_sample, buffer_id);
    printf("num_pkts = %d, max_samples = %d \n", num_pkts, max_samples);
#endif

    // Initialization

    // Malloc temporary buffer to receive ethernet packets    
    rcvd_buffer  = (unsigned char *) malloc( sizeof( char ) * rcvd_max_size );
    if( rcvd_buffer == NULL ) { die_with_error("Error:  Could not allocate temp receive buffer"); }

    // Malloc temporary buffer to process ethernet packets    
    send_buffer  = (unsigned char *) malloc( sizeof( char ) * max_length );
    if( send_buffer == NULL ) { die_with_error("Error:  Could not allocate temp send buffer"); }
    for( i = 0; i < cmd_hdr_size; i++ ) { send_buffer[i] = buffer[i]; }     // Copy current header to send buffer 

    // Set up pointers to all the pieces of the ethernet packet    
    transport_hdr  = (wl_transport_header *) send_buffer;
    command_hdr    = (wl_command_header   *) ( send_buffer + tport_hdr_size );
    sample_hdr     = (wl_sample_header    *) ( send_buffer + cmd_hdr_size   );
    sample_payload = (uint32              *) ( send_buffer + all_hdr_size   );

    // Get necessary values from the packet buffer so we can send multiple packets
    seq_num         = endian_swap_16( transport_hdr->seq_num ) + 1;    // Current sequence number is from the last packet
    transport_flags = endian_swap_16( transport_hdr->flags );

    // Determine data types and sizes based on input data_type
    switch (data_type) {
        case IQ_DATA_TYPE_DOUBLE:
            mex_data_type     = mxDOUBLE_CLASS;
            data_size         = sizeof(double);

            // Check that we have complex doubles
            if ( mxIsDouble(samples) != 1 ) { mexErrMsgTxt("Error: Data type of samples does not match input type 'double'"); }
            if ( mxIsComplex(samples) != 1 ) { process_imag = 0; }  // Do not process the imaginary part of the array
            
            // Get the real pointer and update based on the iteration
            tmp_double_array_real = mxGetPr(samples);
            tmp_double_array_real = &(tmp_double_array_real[iteration * num_samples]);

            // Get the imag pointer and update based on the iteration
            if (process_imag) {
                tmp_double_array_imag = mxGetPi(samples);            
                tmp_double_array_imag = &(tmp_double_array_imag[iteration * num_samples]);
            }
        break;
        
        case IQ_DATA_TYPE_SINGLE:
            mex_data_type     = mxSINGLE_CLASS;
            data_size         = sizeof(float);
            
            // Check that we have complex singles
            if ( mxIsSingle(samples) != 1 ) { mexErrMsgTxt("Error: Data type of samples does not match input type 'single'"); }
            if ( mxIsComplex(samples) != 1 ) { process_imag = 0; }  // Do not process the imaginary part of the array
            
            // Get the real pointer and update based on the iteration
            tmp_single_array_real = (float *) mxGetData(samples);
            tmp_single_array_real = &(tmp_single_array_real[iteration * num_samples]);

            // Get the imag pointer and update based on the iteration
            if (process_imag) {
                tmp_single_array_imag = (float *) mxGetImagData(samples);
                tmp_single_array_imag = &(tmp_single_array_imag[iteration * num_samples]);
            }
        break;
        
        case IQ_DATA_TYPE_INT16:
            mex_data_type     = mxINT16_CLASS;
            data_size         = sizeof(int16);
            
            // Check that we have complex int16
            if ( mxIsInt16(samples) != 1 ) { mexErrMsgTxt("Error: Data type of samples does not match input type 'int16'"); }
            if ( mxIsComplex(samples) != 1 ) { process_imag = 0; }  // Do not process the imaginary part of the array
            
            // Get the real pointer and update based on the iteration
            tmp_int16_array_real = (int16 *) mxGetData(samples);
            tmp_int16_array_real = &(tmp_int16_array_real[iteration * num_samples]);
                        
            // Get the imag pointer and update based on the iteration
            if (process_imag) {
                tmp_int16_array_imag = (int16 *) mxGetImagData(samples);
                tmp_int16_array_imag = &(tmp_int16_array_imag[iteration * num_samples]);
            }
        break;
        
        case IQ_DATA_TYPE_RAW:
            mex_data_type     = mxUINT32_CLASS;
            data_size         = sizeof(uint32);
            
            // Check that we have real uint32
            if ( mxIsUint32(samples) != 1 ) { mexErrMsgTxt("Error: Data type of samples does not match input type 'raw'"); }
            if ( mxIsComplex(samples) != 0 ) { mexErrMsgTxt("Error: Sample data of type 'raw' must be real"); }
            
            // Get the real and imag pointers
            tmp_uint32_array = (uint32 *) mxGetData(samples);
            
            // Update the pointers based on the iteration
            tmp_uint32_array = &(tmp_uint32_array[iteration * num_samples]);
        break;
        
        default:
            mexErrMsgTxt("Error:  Unsupported output data type");
        break;
    }

    
    // Computer the intra-packet wait time
    wait_time = wl_compute_write_wait_time(hw_ver, buffer_id, max_samples);

    
    // Set up the one-time packet values

    // Command header
    //   NOTE:  Since there is one sample packet per command, we set the number number of command arguments to be 1.
    command_hdr->num_args    = endian_swap_16( 0x0001 );

    // Sample header
    //   NOTE:  Iteration over buffer_ids occurs above this function
    sample_hdr->buffer_id    = endian_swap_16( buffer_id );    
    sample_iq_id             = (sample_write_iq_id & 0xFF);
    sample_hdr->sample_iq_id = sample_iq_id;
    
    // Increment write IQ ID (explicitly maintain as a uint8)
    sample_write_iq_id = (sample_write_iq_id + 1) % 0x100;
    
    // Initialize loop variables
    slow_write    = 0;
    need_resp     = 0;
    offset        = start_sample;
    seq_start_num = seq_num;
        
    // Based on some analysis, the loop to transmit packets rarely takes longer than 72us
    // (ie the time that it takes to transmit a jumbo frame on a 1 Gbps Ethernet link)
    // on our test setup:  See http://warpproject.org/trac/wiki/WARPLab/Benchmarks/WARPLAB_7_5_0 
    // for a description of the hardware and software.
    // 
    // Therefore, while further optimizations do exist within the code loop below, we are going 
    // to leave it 'as is' for now.
    //
    
    // For each packet
    for( i = 0; i < num_pkts; i++ ) {
    
        // Determine how many samples we need to send in the packet
        if ( ( offset + max_samples ) <= num_samples ) {
            sample_num = max_samples;
        } else {
            sample_num = num_samples - offset;
        }

        // Determine the length of the packet (All WARPLab payload minus the padding for word alignment)
        length = all_hdr_size_np + (sample_num * sizeof( uint32 ));

        // Request that the board responds:
        //    - For slow_write == 1, all packets should be acked
        //    - For the last packet, if we are checking the checksum, then it needs to be acked
        //
        if ( slow_write == 1 ) {
            need_resp       = 1;
            transport_flags = transport_flags | TRANSPORT_FLAG_ROBUST;        
        } else if ( ( i == ( num_pkts - 1 ) ) && ( check_chksum == 1 ) ) {
            need_resp       = 1;
            transport_flags = transport_flags | TRANSPORT_FLAG_ROBUST;
        } else {
            need_resp       = 0;
            transport_flags = transport_flags & ~TRANSPORT_FLAG_ROBUST;        
        }

        // Prepare transport
        //   NOTE:  The length of the packet is the maximum payload size supported by the transport.  However, the 
        //       maximum payload size returned by the node has the two byte Ethernet padding already subtracted out, so the 
        //       length of the transport command is the length minus the transport header plus the padding size, since
        //       we do not want to double count the padding.
        //
        transport_hdr->length   = endian_swap_16( length - tport_hdr_size_np );
        transport_hdr->seq_num  = endian_swap_16( (seq_num & 0xFFFF) );
        transport_hdr->flags    = endian_swap_16( transport_flags );
        
        // Prepare command
        //   NOTE:  See above comment about length.  Since there is one sample packet per command, we set the number
        //       number of command arguments to be 1.
        //
        command_hdr->length     = endian_swap_16( length - cmd_hdr_size_np );
        
        // Prepare sample packet
        if ( i == 0 ) {
            // First packet
            if ( num_pkts > 1 ) {
                sample_hdr->flags   = SAMPLE_CHKSUM_RESET;
            } else {
                // Only one packet so mark both flag bits
                sample_hdr->flags   = SAMPLE_CHKSUM_RESET | SAMPLE_LAST_WRITE;
            }
        } else if ( i == (num_pkts - 1) ) {
            // Last packet
            sample_hdr->flags       = SAMPLE_LAST_WRITE;
        } else {
            // All other packets
            sample_hdr->flags       = 0x0;
        }
        
        sample_hdr->start       = endian_swap_32( offset );
        sample_hdr->num_samples = endian_swap_32( sample_num );
        
        
        // NOTE:  In C when converting from double to Fix_16_15, the naive implementation:
        //
        //       output = (int16)(double_value * (1 << 15));
        //
        //   will have conversion errors when the double input exceeds the range of the Fix_16_15 variable.  For
        //   example, a Fix_16_15 can only represent a value of [32767 .. -32768] or in terms of doubles
        //   [0.999969482421875 .. -1].  However, in Write IQ, we allow a value of [1 .. -1] which unfortunately
        //   exceeds the range of the Fix_16_15 representation.  Therefore, if we have a Write IQ value of 1, then 
        //   this will result in an output value of 0x8000 which is -32768 and is not correct.
        //
        //   Interestingly, the naive implementation in M:
        //
        //       output = int16(double_value * 2^15);
        //
        //   actually handles the conversion correctly and will cause the value of 1 (and presumably any values 
        //   greater than 1) to have a value of 32767 (ie it caps the value at the largest positive integer 
        //   representable by a Fix_16_15).
        //
        //   Hence, we need to adjust our implementation of the double to Fix_16_15 conversion so that it behaves
        //   like the naive Matlab implementation (ie correctly):
        //
        //       tmp_int32 = (double_value) * (1 << 15);                   // Convert but store in a larger variable
        //       tmp_int32 = ( tmp_int32 >= 0x8000 ) ? 0x7FFF : tmp_int32; // Adjust any values that greater than Fix_16_15
        //       tmp_int32 = ( tmp_int32 < -0x8000 ) ? 0x8000 : tmp_int32; // Adjust any values that are less than Fix_16_15
        //       output    = (int16)(tmp_int32);                           // Truncate to a 16-bit value
        //
        //   or 
        //
        //       tmp_int16 = (int16)(double_value * (1 << 15));            // Convert naively from double to Fix_16_15
        //       if (double_value >= 1.0) { tmp_int16 = (int16)(0x7FFF); } // Adjust any values that greater than Fix_16_15
        //       if (double_value < -1.0) { tmp_int16 = (int16)(0x8000); } // Adjust any values that are less than Fix_16_15
        //       output = tmp_int16;
        //
        //   This will correctly handle the error condition described above.
        //
        switch (data_type) {
            // ------------------------------------------------------------------------------------------------
            case IQ_DATA_TYPE_DOUBLE:
                // Need to process double precision floating point array
                //    NOTE:  This performs an endian swap (little big) on IQ data
                //    NOTE:  This will convert IQ data from a complex double to two Fix_16_15 packed into a UFix_32_0
                // 
                for( j = 0; j < sample_num; j++ ) {
                    tmp_double_real = tmp_double_array_real[j + offset];
                    
                    // Convert the real value
                    tmp_int16_real = (int16)(tmp_double_real * (1 << 15));            // Convert naively from double to Fix_16_15
                    if (tmp_double_real >= 1.0) { tmp_int16_real = (int16)(0x7FFF); } // Adjust any values that greater than Fix_16_15
                    if (tmp_double_real < -1.0) { tmp_int16_real = (int16)(0x8000); } // Adjust any values that are less than Fix_16_15
                     
                    if (process_imag) {
                        tmp_double_imag = tmp_double_array_imag[j + offset];
                                    
                        // Convert the imag value
                        tmp_int16_imag = (int16)(tmp_double_imag * (1 << 15));            // Convert naively from double to Fix_16_15
                        if (tmp_double_imag >= 1.0) { tmp_int16_imag = (int16)(0x7FFF); } // Adjust any values that greater than Fix_16_15
                        if (tmp_double_imag < -1.0) { tmp_int16_imag = (int16)(0x8000); } // Adjust any values that are less than Fix_16_15
                    
                        // Populate the sample payload
                        sample_payload[j] = endian_swap_32((tmp_int16_real << 16) | (tmp_int16_imag & 0xFFFF));
                    } else {
                        // Populate the sample payload
                        sample_payload[j] = endian_swap_32((tmp_int16_real << 16));
                    }
                }                
            break;
            
            // ------------------------------------------------------------------------------------------------
            case IQ_DATA_TYPE_SINGLE:
                // Need to process single precision floating point array
                //     NOTE:  This is exactly the same processing as the 'double' case but using float 
                //            instead of double for the output type
                //
                for( j = 0; j < sample_num; j++ ) {
                    tmp_single_real = tmp_single_array_real[j + offset];
                                    
                    // Convert the real value
                    tmp_int16_real = (int16)(tmp_single_real * (1 << 15));            // Convert naively from double to Fix_16_15
                    if (tmp_single_real >= 1.0) { tmp_int16_real = (int16)(0x7FFF); } // Adjust any values that greater than Fix_16_15
                    if (tmp_single_real < -1.0) { tmp_int16_real = (int16)(0x8000); } // Adjust any values that are less than Fix_16_15
 
                    if (process_imag) {
                        tmp_single_imag = tmp_single_array_imag[j + offset];
                        
                        // Convert the imag value
                        tmp_int16_imag = (int16)(tmp_single_imag * (1 << 15));            // Convert naively from double to Fix_16_15
                        if (tmp_single_imag >= 1.0) { tmp_int16_imag = (int16)(0x7FFF); } // Adjust any values that greater than Fix_16_15
                        if (tmp_single_imag < -1.0) { tmp_int16_imag = (int16)(0x8000); } // Adjust any values that are less than Fix_16_15
                        
                        // Populate the sample payload
                        sample_payload[j] = endian_swap_32((tmp_int16_real << 16) | (tmp_int16_imag & 0xFFFF));
                    } else {
                        // Populate the sample payload
                        sample_payload[j] = endian_swap_32((tmp_int16_real << 16));                    
                    }
                }
            break;
            
            // ------------------------------------------------------------------------------------------------
            case IQ_DATA_TYPE_INT16:
                // Need to process int16 array
                //    NOTE:  This performs an endian swap (little big) on IQ data
                //    NOTE:  This will convert IQ data from a complex Fix_16_15 to two Fix_16_15 packed into a UFix_32_0
                //
                for( j = 0; j < sample_num; j++ ) {
                    tmp_int16_real = tmp_int16_array_real[j + offset];

                    if (process_imag) {
                        tmp_int16_imag = tmp_int16_array_imag[j + offset];
                    
                        // Populate the sample payload
                        sample_payload[j] = endian_swap_32((tmp_int16_real << 16) | (tmp_int16_imag & 0xFFFF));
                    } else {
                        // Populate the sample payload
                        sample_payload[j] = endian_swap_32((tmp_int16_real << 16));
                    }
                }
            break;
            
            // ------------------------------------------------------------------------------------------------
            case IQ_DATA_TYPE_RAW:
                // Need to process the Ethernet packet into a uint32 array
                //    NOTE:  This performs an endian swap (little big) on IQ data
                //    NOTE:  No other processing is done on the data
                //
                for( j = 0; j < sample_num; j++ ) {
                    sample_payload[j] = endian_swap_32(tmp_uint32_array[j + offset]);
                    
                    // Populate variables for the checksum
                    if (j == (sample_num - 1)) {
                        tmp_int16_real = (tmp_uint32_array[j + offset] >> 16);
                        tmp_int16_imag = (tmp_uint32_array[j + offset] & 0xFFFF);
                    }
                }
            break;
            
            // ------------------------------------------------------------------------------------------------
            default:
                mexErrMsgTxt("Error:  Unsupported output data type");
            break;
        }
        
        // Add back in the padding so we can send the packet
        length += TRANSPORT_PADDING_SIZE;

        // Send packet 
        sent_size = send_socket( index, (char *) send_buffer, length, ip_addr, port );

        if ( sent_size != length ) {
            die_with_error("Error:  Size of packet sent to with samples does not match length of packet.");
        }
        
        // Update loop variables
        offset   += sample_num;
        seq_num  += 1;

        // Compute checksum
        // NOTE:  Due to a weakness in the Fletcher 32 checksum (ie it cannot distinguish between
        //     blocks of all 0 bits and blocks of all 1 bits), we need to add additional information
        //     to the checksum so that we will not miss errors on packets that contain data of all 
        //     zero or all one.  Therefore, we add in the start sample for each packet since that 
        //     is readily available on the node.
        //
        if ( i == 0 ) {
            local_checksum = wl_update_checksum(((offset - sample_num) & 0xFFFF), SAMPLE_CHKSUM_RESET); 
        } else {
            local_checksum = wl_update_checksum(((offset - sample_num) & 0xFFFF), SAMPLE_CHKSUM_NOT_RESET); 
        }
        
        // NOTE:  Based on the packet processing above, the temporary variables:  tmp_int16_real and tmp_int16_imag 
        //     will contain the value of the last entry in the packet which we will use for the checksum.
        //
        local_checksum = wl_update_checksum(((tmp_int16_real & 0xFFFF) ^ (tmp_int16_imag & 0xFFFF)), SAMPLE_CHKSUM_NOT_RESET);
        
        // If we need a response, then wait for it
        //
        if ( need_resp == 1 ) {

            // Initialize loop variables
            timeout   = 0;
            done      = 0;
            rcvd_size = 0;
            
            // Process each return packet
            while ( !done ) {

                // If we hit the timeout, then try to re-transmit the packet
                if ( timeout >= TRANSPORT_TIMEOUT ) {
                
                    // If we hit the max number of retrys, then abort
                    if ( num_retrys >= TRANSPORT_MAX_RETRY ) {
                        die_with_error("Error:  Reached maximum number of retrys without a response... aborting.");                    
                    } else {
                        // Roll everything back and retransmit the packet
                        num_retrys += 1;
                        offset     -= sample_num;
                        i          -= 1;
                        break;
                    }
                }

                // Receive packet (socket error checking done by function)
                rcvd_size = receive_socket( index, rcvd_max_size, (char *) rcvd_buffer );

                if ( rcvd_size > 0 ) {
                    resp_args   = (uint32 *) ( rcvd_buffer + cmd_hdr_size );
                    
                    write_iq_response = wl_process_write_iq_response(resp_args, sample_iq_id, local_checksum, write_iq_ready_warn);

                    // If we have a checksum error in fast write mode, then switch to slow write and start over
                    if (write_iq_response == SAMPLE_CHECKSUM_FAILED) {
                        if ( slow_write == 0 ) {
                            if ( suppress_iq_warnings == 0 ) {
                                printf("WARNING:  Checksums do not match on pkt %d.\n", i);
                                printf("    Expected = %08x  Received = %08x.  Restarting Write IQ using 'slow write'.\n\n", local_checksum, endian_swap_32(resp_args[1]));
                                printf("    This message generally occurs when the node is not able to keep up with the\n");
                                printf("    Write IQ data transfer rate from the host.  If this message occurs frequently\n");
                                printf("    please do one of the following:\n");
                                printf("        1) If the node is transmitting or receiving while trying to perform the \n");
                                printf("           Write IQ, then add a delay until the node is finished.  For example,\n");
                                printf("               'pause(1.5 * NUM_SAMPLES * 1/(40e6));'\n");
                                printf("           after any triggers and before the Write IQ request.\n");
                                printf("        2) Adjust the inter-packet Write IQ wait time for the transport:\n");
                                printf("               wl_mex_udp_transport('write_iq_set_pkt_wait_time', wait_time)\n");
                                printf("           where 'wait_time' is in microseconds and is larger than the current\n");
                                printf("           wait time of %d microseconds.\n", wait_time);
                                printf("        3) Suppress all IQ warnings for the transport:\n");
                                printf("               wl_mex_udp_transport('suppress_iq_warnings')\n\n");
                            }
                            
                            slow_write = 1;
                            offset     = start_sample;
                            i          = -1;
                            break;
                        } else {
                            die_with_error("Error:  Checksums do not match when in slow write... aborting.");
                        }
                    }

                    // If the node is not ready, then start over (any required waiting has already been completed)                    
                    if (write_iq_response == SAMPLE_IQ_NOT_READY) {
                        write_iq_ready_warn = 0;
                    
                        offset     = start_sample;
                        i          = -1;
                        break;
                    }
                    
                    timeout = 0;
                    done    = 1;
                } else {
                    // If we do not have a packet, increment the timeout counter
                    timeout++;
                }
            }  // END while( !done )
        } else {
            // Check if the node has sent us a packet that we were not expecting
            rcvd_size = receive_socket( index, rcvd_max_size, (char *) rcvd_buffer );
            
            if ( rcvd_size > 0 ) {
                resp_args = (uint32 *) ( rcvd_buffer + cmd_hdr_size );
                
                write_iq_response = wl_process_write_iq_response(resp_args, sample_iq_id, 0, write_iq_ready_warn);

                // If the node is not ready, then start over (any required waiting has already been completed)
                if (write_iq_response == SAMPLE_IQ_NOT_READY) {
                    write_iq_ready_warn = 0;
                    
                    offset     = start_sample;
                    i          = -1;
                }
            }
        }  // END if need_resp

        
        // Wait for the requisite time between packets
        if ( wait_time != 0 ) {
            wl_usleep( wait_time );
        }
    }  // END for num_pkts


    if ( offset != num_samples ) {
        printf("ERROR:  Issue with calling function.  \n");
        printf("    Requested %d samples, sent %d sample based on other packet information: \n", num_samples, offset);
        printf("    Number of packets to send %d, Max samples per packet %d \n", num_pkts, max_samples);
    }
    
    // Free locally allocated memory    
    free( send_buffer );
    free( rcvd_buffer );
    
    // Finalize outputs
    //   NOTE:  seq_num is a uint32 so that it can capture values > 2^16 which could potentially 
    //          occur with the new larger buffer sizes.
    //
    if ( seq_num > seq_start_num ) {
        *num_cmds += seq_num - seq_start_num;
    } else {
        *num_cmds += (0xFFFF - seq_start_num) + seq_num;
    }
    
    *checksum = local_checksum;
    
    return offset;
}



/*****************************************************************************/
/**
*  Function:  wl_compute_write_wait_time
*
*  Function to compute the wait time for the write function in micro-seconds
*
******************************************************************************/
uint32 wl_compute_write_wait_time(uint32 hw_ver, uint32 buffer_id, uint32 max_samples) {
    uint32         j;
    uint32         buffer_count      = 0;
    uint32         wait_time         = 0;


    // Write baseband buffers can saturate the Ethernet wire.  However, for small packets the 
    // node cannot keep up and therefore we need to delay the next transmission based on:
    // 1) packet size and 2) number of buffers being written
    
    // In case the default implementation based on experimental data does not work as expected for your 
    // setup, you can optionally specify a user Write IQ packet wait time that will override the default
    // implementation.
    
    if ( use_user_write_iq_wait_time == 1 ) {
        
        wait_time = user_write_iq_wait_time;

    } else {
    
        // NOTE:  This is a simplified implementation based on experimental data.  It is by no means optimized for 
        //     all cases.  Since WARP v2 and WARP v3 hardware have drastically different internal architectures,
        //     we first have to understand which HW the Write IQ is being performed and scale the wait_times
        //     accordingly.  Also, since we can have one transmission of IQ data be distributed to multiple buffers,
        //     we need to increase the timeout if we are transmitting to more buffers on the node in order to 
        //     compensate for the extended processing time.  One other thing to note is that if you view the traffic that
        //     this generates on an oscilloscope (at least on Window 7 Professional 64-bit), it is not necessarily regular 
        //     but will burst 2 or 3 packets followed by an extended break.  Fortunately there is enough buffering in 
        //     the data flow that this does not cause a problem, but it makes it more difficult to optimize the data flow 
        //     since we cannot guarentee the timing of the packets at the node.  
        //
        //     If you start receiving checksum failures and need to adjust timing, please do so in the code below.
        //
        switch ( hw_ver ) {
            case TRANSPORT_WARP_HW_v2:
                // WARP v2 Hardware only supports small ethernet packets
 
                buffer_count = 0;

                // Count the number of buffers in the buffer_id
                for( j = 0; j < TRANSPORT_WARP_RF_BUFFER_MAX; j++ ) {
                    if ( ( ( buffer_id >> j ) & 0x1 ) == 1 ) {
                        buffer_count++;
                    }
                }
            
                // Note:  Performance drops dramatically if this number is smaller than the processing time on
                //     the node.  This is due to the fact that you perform a slow write when the checksum fails.  
                //     For example, if you change this from 160 to 140, the Avg Write IQ per second goes from
                //     ~130 to ~30.  If this number gets too large, then you will also degrade performance
                //     given you are waiting longer than necessary.
                //
                // Currently, wait times are set at:
                //     1 buffer  = 160 us
                //     2 buffers = 240 us
                //     3 buffers = 320 us
                //     4 buffers = 400 us
                // 
                // This is due to the fact that processing on the node is done thru a memcpy and takes 
                // a fixed amount of time longer for each additonal buffer that is transferred.
                //                
                wait_time = 80 + ( buffer_count * 80 );
            break;
            
            case TRANSPORT_WARP_HW_v3:
                // In WARP v3 hardware, we need to account for both small packets as well as jumbo frames.  Also,
                // since the WARP v3 hardware uses a DMA to transfer packet data, the processing overhead is much
                // less than on v2 (hence the smaller wait times).  Through experimental testing, we found that 
                // for jumbo frames, the processing overhead was smaller than the length of the ethernet transfer
                // and therefore we do not need to wait at all.  We have not done exhaustive testing on ethernet 
                // packet size vs wait time.  So in this simplified implementation, if your Ethernet MTU size is 
                // less than 9000 bytes (ie approximately 0x8B8 samples) then we will insert a 40 us, or 50 us, 
                // delay between transmissions to give the board time to keep up with the flow of packets.
                // 
                if ( max_samples < 0x800 ) {
                    if ( buffer_id == 0xF ) {
                        wait_time = 50;
                    } else {
                        wait_time = 40;
                    }
                }
             break;
             
             default:
                printf("WARNING:  WARP HW version of node is not recognized:  Received:  %d   Expected:  %d or %d\n", hw_ver, TRANSPORT_WARP_HW_v2, TRANSPORT_WARP_HW_v3);
                printf("WARNING:      This could be an issue with the version of the MEX you are trying to use.\n");
                printf("WARNING:      Please check that WARPLab MEX UDP Transport v%s is the required version \n", WL_MEX_UDP_TRANSPORT_VERSION);
                printf("WARNING:      for your WARPLab release.\n");
             break;
        }
    }  // END if ( use_user_write_iq_wait_time == 1 )
    
    return wait_time;
}



/*****************************************************************************/
/**
*  Function:  wl_process_write_iq_response
*
*  Function to process the write IQ response
*
******************************************************************************/
uint32 wl_process_write_iq_response(uint32 * args, uint32 sample_iq_id, uint32 checksum, uint32 iq_ready_warn) {

    // Response variables
    uint32                node_status       = 0;
    uint32                node_checksum     = 0;
    uint32                node_sample_iq_id = 0;
    uint32                wait_time         = 0;

    // Return variable
    uint32                return_val        = SAMPLE_RESPONSE_SUCCESS;
    
    // Get the IQ ID from the response
    node_sample_iq_id = endian_swap_32( args[1] );

    // Only process packets for the current sample_iq_id
    if (node_sample_iq_id == sample_iq_id) {
        node_status       = endian_swap_32( args[0] );
        
        switch (node_status) {
            // ------------------------------------------------------------------------------------        
            case SAMPLE_IQ_ERROR:
                printf("SAMPLE_IQ_ERROR:\n");
                printf("    Due to limitations on the node, it is not possible to do a Write IQ while the\n");
                printf("    node is transmitting in 'Continuous Tx' mode.  Please stop the current transmission\n");
                printf("    and try the Write IQ again\n");
            
                die_with_error("ERROR:  Node returned 'SAMPLE_IQ_ERROR'.  See above for debug information.");
            break;
            
            // ------------------------------------------------------------------------------------        
            case SAMPLE_IQ_NOT_READY:
                // If the node is not ready, then we need to wait until the node is ready and try again from the 
                // beginning of the Write IQ.
                //
                wait_time = wl_compute_sample_wait_time( &args[3] );
                                
                // Wait until the samples should be done
                if ( wait_time != 0 ) {
                    wl_usleep( wait_time + 100 );
                }
                
                // Print warning
                if (iq_ready_warn == 1) {
                    printf("WARNING:  Node was not ready to process Write IQ request.  Waiting to request again.\n");
                    printf("    This warning can be removed by waiting until the node is not busy with a TX or RX \n");
                    printf("    operation.  To do this, please add 'pause(1.5 * NUM_SAMPLES * 1/(40e6));' after\n");
                    printf("    any triggers and before the Write IQ request.\n\n");
                }
                
                // Reset the loop variables
                return_val = SAMPLE_IQ_NOT_READY;
            break;
            
            // ------------------------------------------------------------------------------------        
            case CMD_PARAM_SUCCESS:
                // If the response was a success, then check the checksum
                //
                node_checksum  = endian_swap_32( args[2] );

                // Compare the checksum values
                if ( node_checksum != checksum ) {
                
                    // Reset the loop variables
                    return_val = SAMPLE_CHECKSUM_FAILED;
                }
            break;
        
            // ------------------------------------------------------------------------------------        
            default:
                // Print that there was an error; Reset the loop and continue
                printf("ERROR:  Unknown write IQ response status = %d\n", node_status);
                return_val = SAMPLE_RESPONSE_ERROR;
            break;
        }
    }
    
    return return_val;
}



/*****************************************************************************/
/**
*  Function:  wl_update_checksum
*
*  Function to calculate a Fletcher-32 checksum to detect packet loss
*
******************************************************************************/
unsigned int wl_update_checksum(unsigned short int newdata, unsigned char reset ){
	// Fletcher-32 Checksum
	static unsigned int sum1 = 0;
	static unsigned int sum2 = 0;

	if( reset == SAMPLE_CHKSUM_RESET ){ sum1 = 0; sum2 = 0; }

	sum1 = (sum1 + newdata) % 0xFFFF;
	sum2 = (sum2 + sum1   ) % 0xFFFF;

	return ( ( sum2 << 16 ) + sum1);
}



/*****************************************************************************/
/**
*  Function:  wl_compute_sample_wait_time
*
*  Function to compute the wait time based on the args:
*      args[0]       - Tx status
*      args[1]       - Current Tx read pointer
*      args[2]       - Tx length
*      args[3]       - Rx status
*      args[4]       - Current Rx write pointer
*      args[5]       - Rx length
*
******************************************************************************/
uint32 wl_compute_sample_wait_time(uint32 * args) {

    uint32 node_tx_status    = endian_swap_32( args[0] );
    uint32 node_tx_pointer   = endian_swap_32( args[1] );
    uint32 node_tx_length    = endian_swap_32( args[2] );
    uint32 node_rx_status    = endian_swap_32( args[3] );
    uint32 node_rx_pointer   = endian_swap_32( args[4] );
    uint32 node_rx_length    = endian_swap_32( args[5] );
    
    // NOTE:  node_*_length and node_*_pointer are in bytes.  To convert the difference to microseconds,
    //    we need to divide by: 160  (ie 40e6 sample / sec * 4 bytes / sample * 1 usec => 160 bytes / usec)
    // 
    uint32 tx_wait_time      = (node_tx_status) ? ((node_tx_length - node_tx_pointer) / 160) :0;
    uint32 rx_wait_time      = (node_rx_status) ? ((node_rx_length - node_rx_pointer) / 160) :0;;

    // Return the greater of the TX or RX wait time    
    return ((tx_wait_time > rx_wait_time) ? tx_wait_time : rx_wait_time);
    
}


#ifdef WIN32

/*****************************************************************************/
/**
*  Function:  uSleep
*
*  Since the windows Sleep() function only has a resolution of 1 ms, we need
*  to implement a usleep() function that will allow for finer timing granularity
*
******************************************************************************/
void wl_mex_udp_transport_usleep( int wait_time ) {
    static bool     init = false;
    static LONGLONG ticks_per_usecond;

    LARGE_INTEGER   ticks_per_second;    
    LONGLONG        wait_ticks;
    LARGE_INTEGER   start_time;
    LONGLONG        stop_time;
    LARGE_INTEGER   counter_val;

    // Initialize the function
    if ( !init ) {
    
        if ( QueryPerformanceFrequency( &ticks_per_second ) ) {
            ticks_per_usecond = ticks_per_second.QuadPart / 1000000;
            init = true;
        } else {
            printf("QPF() failed with error %d\n", GetLastError());
        }
    }

    if ( ticks_per_usecond ) {
    
        // Calculate how many ticks we have to wait
        wait_ticks = wait_time * ticks_per_usecond;
    
        // Save the performance counter value
        if ( !QueryPerformanceCounter( &start_time ) )
            printf("QPC() failed with error %d\n", GetLastError());

        // Calculate the stop time
        stop_time = start_time.QuadPart + wait_ticks;

        // Wait until the time has expired
        while (1) {        

            if ( !QueryPerformanceCounter( &counter_val ) ) {
                printf("QPC() failed with error %d\n", GetLastError());
                break;
            }
            
            if ( counter_val.QuadPart >= stop_time ) {
                break;
            }
        }
    }
}



/*****************************************************************************/
/**
*  Function:  usec timestamp
*
******************************************************************************/
uint32 wl_mex_udp_transport_usec_timestamp() {
    static bool     init = false;
    static LONGLONG ticks_per_usecond;

    LARGE_INTEGER   ticks_per_second;    
    LARGE_INTEGER   counter_val;

    // Initialize the function
    if ( !init ) {
    
        if ( QueryPerformanceFrequency( &ticks_per_second ) ) {
            ticks_per_usecond = ticks_per_second.QuadPart / 1000000;
            init = true;
        } else {
            printf("QPF() failed with error %d\n", GetLastError());
        }
    }

    if ( ticks_per_usecond ) {
    
        // Save the performance counter value
        if ( !QueryPerformanceCounter( &counter_val ) )
            printf("QPC() failed with error %d\n", GetLastError());

        return (uint32)((counter_val.QuadPart / ticks_per_usecond) & 0x7FFFFFFF);
            
    }
    
    return 0;
}


#endif

