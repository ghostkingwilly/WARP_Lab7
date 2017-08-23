% ------------------------------------------------------------------------
%  Initial Register Values
% ------------------------------------------------------------------------

% ------------------------------------------------------------------------
% Trigger Input Configuration Register:
%   Basic register format:
%       [31]   - Reset
%       [30]   - Debounce
%       [29:5] - Reserved
%       [4:0]  - Input Delay
%   This register is replicated for each of the trigger inputs.  There are
%   a number of reserved bits per trigger input in case the input delay 
%   needs to be increased in the future.
%
%   Input Trigger order:
%       0 - Ethernet A
%             NOTE:  Debounce bit is used as a SW trigger
%                    Bit 29 is reserved (used in WARP v3 version)
%       1 - Energy Detection
%             NOTE:  Debounce bit is not supported
%                    Delay is not supported
%       2 - AGC
%             NOTE:  Debounce bit is not supported
%       3 - Software Trigger
%             NOTE:  Debounce bit is used as a SW trigger
%                    Delay is not supported
%       4 - Debug Input Pin 0
%       5 - Debug Input Pin 1
%       6 - Debug Input Pin 2
%       7 - Debug Input Pin 3
%       8 - Ethernet B (Not supported)
%         
% 
TRIG_IN_CONF_0 = hex2dec('80000000');
TRIG_IN_CONF_1 = hex2dec('80000000');
TRIG_IN_CONF_2 = hex2dec('80000000');
TRIG_IN_CONF_3 = hex2dec('80000000');
TRIG_IN_CONF_4 = hex2dec('C0000000');    % Debounce enabled
TRIG_IN_CONF_5 = hex2dec('C0000000');    % Debounce enabled
TRIG_IN_CONF_6 = hex2dec('C0000000');    % Debounce enabled
TRIG_IN_CONF_7 = hex2dec('C0000000');    % Debounce enabled
TRIG_IN_CONF_8 = hex2dec('80000000');


% ------------------------------------------------------------------------
% Trigger Output Configuration Register:
%   Register format:
%       CONF_0:
%           [31:25] - Reserved
%           [24]    - Output OR use trigger input 8
%           [23]    - Output OR use trigger input 7
%           [22]    - Output OR use trigger input 6
%           [21]    - Output OR use trigger input 5
%           [20]    - Output OR use trigger input 4
%           [19]    - Output OR use trigger input 3
%           [18]    - Output OR use trigger input 2
%           [17]    - Output OR use trigger input 1
%           [16]    - Output OR use trigger input 0
%           [15: 9] - Reserved
%           [ 8]    - Output AND use trigger input 8
%           [ 7]    - Output AND use trigger input 7
%           [ 6]    - Output AND use trigger input 6
%           [ 5]    - Output AND use trigger input 5
%           [ 4]    - Output AND use trigger input 4
%           [ 3]    - Output AND use trigger input 3
%           [ 2]    - Output AND use trigger input 2
%           [ 1]    - Output AND use trigger input 1
%           [ 0]    - Output AND use trigger input 0
%       CONF_1:
%           [31]    - Reset
%           [30: 5] - Reserved
%           [ 4: 0] - Output Delay
%   These two registers are replicated for each of the trigger outputs.
%
%   Output Trigger order (connected in MHS file):
%       0 - Buffer trigger input
%       1 - AGC packet in
%       2 - Debug Output Pin 0
%       3 - Debug Output Pin 1
%       4 - Debug Output Pin 2
%       5 - Debug Output Pin 3
% 
TRIG_OUT_5_CONF_0 = hex2dec('0');
TRIG_OUT_5_CONF_1 = hex2dec('80000000');

TRIG_OUT_4_CONF_0 = hex2dec('0');
TRIG_OUT_4_CONF_1 = hex2dec('80000000');

TRIG_OUT_3_CONF_0 = hex2dec('0');
TRIG_OUT_3_CONF_1 = hex2dec('80000000');

TRIG_OUT_2_CONF_0 = hex2dec('0');
TRIG_OUT_2_CONF_1 = hex2dec('80000000');

TRIG_OUT_1_CONF_0 = hex2dec('0');
TRIG_OUT_1_CONF_1 = hex2dec('80000000');

TRIG_OUT_0_CONF_0 = hex2dec('0');
TRIG_OUT_0_CONF_1 = hex2dec('80000000');


% ------------------------------------------------------------------------
% RSSI Packet Detection Configuration Register
%   Register format:
%       [31]    - Packet detect reset
%       [30: 4] - Reserved
%       [ 3]    - Packet detect mask RF D
%       [ 2]    - Packet detect mask RF C
%       [ 1]    - Packet detect mask RF B
%       [ 0]    - Packet detect mask RF A
%
RSSI_PKT_DET_CONFIG = hex2dec('0');

% ------------------------------------------------------------------------
% RSSI Packet Detection Threshold Register
%   Register format:
%       [31:16] - Packet detect energy threshold busy
%       [15: 0] - Packet detect energy threshold idle
%
RSSI_PKT_DET_THRESHOLDS = hex2dec('0');

% ------------------------------------------------------------------------
% RSSI Packet Detection Duration Register
%   Register format:
%       [31:21] - Reserved
%       [20:16] - Packet detect RSSI average length
%       [15: 8] - Packet detect duration busy
%       [ 7: 0] - Packet detect duration idle
% 
RSSI_PKT_DET_DURATIONS = hex2dec('0');


% ------------------------------------------------------------------------
% Ethernet Trigger Memories
%     NOTE:  WARP v2 does not allow for Ethernet Triggers
%
% PKT_OPS_0      = hex2dec('0');
% PKT_TEMPLATE_0 = hex2dec('0');
% 
% PKT_OPS_1      = hex2dec('0');
% PKT_TEMPLATE_1 = hex2dec('0');
