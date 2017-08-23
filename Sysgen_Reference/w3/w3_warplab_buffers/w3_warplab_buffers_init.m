%IMPORTANT: if you change any of the NumSamps values, you must update the 
% EDK Processor block with the new shared memory sizes. We recommend
% deleting the existing EDK Processor block and re-adding it, to ensure
% all changes are reflected

NumSamps_Tx_IQ               = 2^15;
NumSamps_Rx_IQ               = 2^15;
NumSamps_Rx_RSSI             = 2^13;

simulation                   = 0;

% ------------------------------------------------------------------------
%  Initial Register Values
% ------------------------------------------------------------------------

if (simulation == 0)
    rx_length                    = 0;                          % Default 0
    rf_rx_iq_threshold           = 0;                          % Default 0
    rf_rx_iq_buf_rd_byte_offset  = 0;                          % Default 0
    rf_rx_iq_buf_wr_byte_offset  = 0;                          % Default 0
    
    tx_length                    = 0;                          % Default 0
    rf_tx_iq_threshold           = 0;                          % Default 0
    rf_tx_iq_buf_rd_byte_offset  = 0;                          % Default 0
    rf_tx_iq_buf_wr_byte_offset  = 0;                          % Default 0    

    rf_error_clr                 = 0;                          % Default 0
    
    rx_buf_en                    = 0;                          % Default 0
    tx_buf_en                    = 0;                          % Default 0
    
    txrx_counter_reset           = 0;                          % Default 0
else
    rx_length                    = hex2dec('0000FFFF');        % Default (NumSamps_Rx_IQ - 1)
    rf_rx_iq_threshold           = hex2dec('00002000');        % Default (NumSamps_Rx_IQ / 2)
    rf_rx_iq_buf_rd_byte_offset  = 0;                          % Default 0
    rf_rx_iq_buf_wr_byte_offset  = 0;                          % Default 0

    tx_length                    = hex2dec('0000FFFF');        % Default (NumSamps_Tx_IQ - 1)
    rf_tx_iq_threshold           = hex2dec('00004000');        % Default (NumSamps_Tx_IQ / 2)
    rf_tx_iq_buf_wr_byte_offset  = hex2dec('00020000');        % Default (NumSamps_Tx_IQ * 4)

    rf_error_clr                 = 0;                          % Default 0
        
    rx_buf_en                    = hex2dec('0000000F');        % Default hex2dec('0000000F')
    tx_buf_en                    = hex2dec('0000000F');        % Default hex2dec('0000000F')
end


% ------------------------------------------------------------------------
% Buffer Configuration Register:
%   Register format:
%       [31:26] - Reserved
%       [25:24] - Scope data select
%       [23:21] - Reserved
%          [20] - Counter data select
%          [17] - TX Byte order
%          [16] - TX Word order
%          [17] - RX Byte order
%          [16] - RX Word order
%       [15:10] - Reserved
%       [ 9: 8] - RSSI clock select
%           [7] - AGC IQ select RFD
%           [6] - AGC IQ select RFC
%           [5] - AGC IQ select RFB
%           [4] - AGC IQ select RFA
%       [ 3: 2] - Reserved
%           [1] - Stop Tx
%           [0] - Continous Tx
% 
if (simulation == 0)
    wl_buffers_config_init      = 0;                     % Initialize the Config register to zero
else
    wl_buffers_config_init      = hex2dec('001A0100');   % Initialize the Config register to 0x001A0100
end


% ------------------------------------------------------------------------
% Tx Delay Register:
%   Register format:
%       [31: 0] - Tx Delay
% 
wl_buffers_tx_delay_init    = 1000;                  % This will provide a Tx Delay of 1001 samples


% ------------------------------------------------------------------------
% RF Buffer Select Register:
%   Register format:
%       [31:26] - Reserved
%       [25:24] - RFD buffer select
%       [23:18] - Reserved
%       [17:16] - RFC buffer select
%       [15:10] - Reserved
%       [ 9: 8] - RFB buffer select
%       [ 7: 2] - Reserved
%       [ 1: 0] - RFA buffer select
%
%   RF* buffer select values:
%       0 - RFA Tx buffer
%       1 - RFB Tx buffer
%       2 - RFC Tx buffer
%       3 - RFD Tx buffer
% 
wl_buffers_rf_buffer_sel    = hex2dec('03020100');   % Select the buffer to use for the output
