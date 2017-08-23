%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% wl_example_siso_txrx_energyTrigger.m
%
%   This example demonstrates energy trigger feature of WARP nodes.  It sends
% a waveform consisting of a simple sinusoidal payload.  The trigger manager
% uses the energy detector to trigger the receiving node instead of using an
% Ethernet trigger.
%
% NOTE:  It is a straight forward extension to use the automatic gain controller
%     with this example.
%
% Requirements:
%     2 WARP nodes (same hardware generation); 1 RF interface each
%     WARPLab 7.6.0 and higher
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

clear;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Top Level Control Variables
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Parameters for the energy trigger
rssi_sum_len                 = 15;
energy_detection_threshold   = rssi_sum_len * 200;
busy_minlength               = 10;

% Rx Gain
rx_gain_rf                   = 1;      % Value must be an integer in [1:3]
rx_gain_bb                   = 15;     % Value must be an integer in [0:31]

% Tx Gain
tx_gain_bb                   = 3;      % Value must be an integer in [0:3] 
tx_gain_rf                   = 30;     % Value must be an integer in [0:63]

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Set up the WARPLab experiment
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Create a vector of node objects
nodes = wl_initNodes(2);

% Set up transmit and receive nodes
%     Transmit from nodes(1) to nodes(2)
node_tx = nodes(1);
node_rx = nodes(2);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Set up Trigger Manager
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Create a UDP broadcast trigger and tell each node to be ready for it
eth_trig = wl_trigger_eth_udp_broadcast;
wl_triggerManagerCmd(nodes, 'add_ethernet_trigger', [eth_trig]);

% Read Trigger IDs into workspace
trig_in_ids  = wl_getTriggerInputIDs(nodes(1));
trig_out_ids = wl_getTriggerOutputIDs(nodes(1));

% For the transmit node, we will allow Ethernet to trigger the baseband buffers
wl_triggerManagerCmd(node_tx, 'output_config_input_selection', [trig_out_ids.BASEBAND], [trig_in_ids.ETH_A]);

% For the receive node, we will allow the energy detector to trigger the baseband buffers
wl_triggerManagerCmd(node_rx, 'output_config_input_selection', [trig_out_ids.BASEBAND], [trig_in_ids.ENERGY_DET]);
% wl_triggerManagerCmd(node_rx, 'output_config_input_selection', [trig_out_ids.BASEBAND, trig_out_ids.AGC], [trig_in_ids.ENERGY_DET]);

% Set the trigger output delays. 
%
% NOTE:  We are waiting 3000 ns before starting the AGC so that there is time for the inputs 
%   to settle before sampling the waveform to calculate the RX gains.
%
% wl_triggerManagerCmd(node_rx, 'output_config_delay', [trig_out_ids.AGC], 3000);     % 3000 ns delay before starting the AGC

% Enable the hold mode for the triggers driven by energy detection. 
%
%     NOTE:  This will prevent the buffer from being overwritten before we have a chance to read it.
%
wl_triggerManagerCmd(node_rx, 'output_config_hold_mode', [trig_out_ids.BASEBAND], true); 
% wl_triggerManagerCmd(node_rx, 'output_config_hold_mode', [trig_out_ids.BASEBAND, trig_out_ids.AGC], true);

% Set parameters for the energy trigger
wl_triggerManagerCmd(node_rx, 'energy_config_average_length', rssi_sum_len);
wl_triggerManagerCmd(node_rx, 'energy_config_busy_threshold', energy_detection_threshold);
wl_triggerManagerCmd(node_rx, 'energy_config_busy_minlength', busy_minlength);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Set up the Interface parameters
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Get IDs for the interfaces on the boards.  
%
% NOTE:  This example assumes each board has the same interface capabilities (ie 2 RF 
%   interfaces; RFA and RFB).  Therefore, we only need to get the IDs from one of the boards.
%
ifc_ids = wl_getInterfaceIDs(nodes(1));

% Set the Transmit and Receive interfaces
%     Transmit from RFA of one node to RFA of the other node
%
% NOTE:  Variables are used to make it easier to change interfaces.
% 
rf_tx         = ifc_ids.RF_A;                    % Transmit RF interface
rf_rx         = ifc_ids.RF_A;                    % Receive RF interface

rf_rx_vec     = ifc_ids.RF_A;                    % Vector version of transmit RF interface
rf_tx_vec     = ifc_ids.RF_A;                    % Vector version of receive RF interface

% Set the RF center frequency on all interfaces
%     - Frequency Band  :  Must be 2.4 or 5, to select 2.4GHz or 5GHz channels
%     - Channel         :  Must be an integer in [1,11] for BAND = 2.4; [1,23] for BAND = 5
%
wl_interfaceCmd(nodes, ifc_ids.RF_ALL, 'channel', 2.4, 11);

% Set the RX gains on all interfaces
%     - Rx RF Gain      :  Must be an integer in [1:3]
%     - Rx Baseband Gain:  Must be an integer in [0:31]
% 
% NOTE:  The gains may need to be modified depending on your experimental setup
%
wl_interfaceCmd(nodes, ifc_ids.RF_ALL, 'rx_gain_mode', 'manual');
wl_interfaceCmd(nodes, ifc_ids.RF_ALL, 'rx_gains', rx_gain_rf, rx_gain_bb);
% wl_interfaceCmd(nodes, ifc_ids.RF_ALL, 'rx_gain_mode', 'automatic');
% wl_basebandCmd(nodes, 'agc_target', -10);

% Set the TX gains on all interfaces
%     - Tx Baseband Gain:  Must be an integer in [0:3] for approx [-5, -3, -1.5, 0]dB baseband gain
%     - Tx RF Gain      :  Must be an integer in [0:63] for approx [0:31]dB RF gain
% 
% NOTE:  The gains may need to be modified depending on your experimental setup
% 
wl_interfaceCmd(nodes, ifc_ids.RF_ALL, 'tx_gains', tx_gain_bb, tx_gain_rf);

% Set interface parameters for the energy trigger
wl_triggerManagerCmd(node_rx, 'energy_config_interface_selection', ifc_ids.RF_ON_BOARD);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Set up the Baseband parameters
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Get the sample frequency from the board
ts      = 1 / (wl_basebandCmd(nodes(1), 'tx_buff_clk_freq'));
ts_rx   = 1 / (wl_basebandCmd(nodes(1), 'rx_buff_clk_freq'));
ts_rssi = 1 / (wl_basebandCmd(nodes(1), 'rx_rssi_clk_freq'));

% Read the maximum I/Q buffer length.  
%
% NOTE:  This example assumes that each board has the same baseband capabilities (ie both nodes are 
%   the same WARP hardware version, for example WARP v3).  This example also assumes that each RF 
%   interface has the same baseband capabilities (ie the max number of TX samples is the same as the 
%   max number of RF samples). Therefore, we only need to read the max I/Q buffer length of node_tx RFA.
% 
maximum_buffer_len = wl_basebandCmd(node_tx, rf_tx, 'tx_buff_max_num_samples');

% Set the transmission / receptions lengths (in samples)
%     See WARPLab user guide for maximum length supported by WARP hardware 
%     versions and different WARPLab versions.
%
tx_length    = 2^15;
rx_length    = tx_length;
rssi_length  = floor(rx_length / (ts_rssi / ts_rx));

% Check the transmission length
if (tx_length > maximum_buffer_len) 
    error('Node supports max transmission length of %d samples.  Requested %d samples.', maximum_buffer_len, tx_length); 
end

% Set the length for the transmit and receive buffers based on the transmission length
wl_basebandCmd(nodes, 'tx_length', tx_length);
wl_basebandCmd(nodes, 'rx_length', rx_length);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Signal processing to generate transmit signal
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% First generate the preamble for AGC. 
%     NOTE:  The preamble corresponds to the short symbols from the 802.11a PHY standard
%
% shortSymbol_freq = [0 0 0 0 0 0 0 0 1+i 0 0 0 -1+i 0 0 0 -1-i 0 0 0 1-i 0 0 0 -1-i 0 0 0 1-i 0 0 0 0 0 0 0 1-i 0 0 0 -1-i 0 0 0 1-i 0 0 0 -1-i 0 0 0 -1+i 0 0 0 1+i 0 0 0 0 0 0 0].';
% shortSymbol_freq = [zeros(32,1);shortSymbol_freq;zeros(32,1)];
% shortSymbol_time = ifft(fftshift(shortSymbol_freq));
% shortSymbol_time = (shortSymbol_time(1:32).')./max(abs(shortSymbol_time));
% shortsyms_rep    = repmat(shortSymbol_time,1,30);
% preamble         = shortsyms_rep;
% preamble         = preamble(:);

t         = [0:ts:((tx_length - 1))*ts].';                      % Create time vector(Sample Frequency is ts (Hz))
% t         = [0:ts:((tx_length - length(preamble) - 1))*ts].';   % Create time vector(Sample Frequency is ts (Hz))

sinusoid  = 0.6 * exp(j*2*pi * 5e6 * t);                        % Create  5 MHz sinusoid

tx_data   = [sinusoid];
% tx_data   = [preamble; sinusoid];


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Transmit and receive signal using WARPLab
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Transmit IQ data to the TX node
wl_basebandCmd(node_tx, rf_tx_vec, 'write_IQ', tx_data(:));

% Enabled the RF interfaces for TX / RX
wl_interfaceCmd(node_tx, rf_tx, 'tx_en');
wl_interfaceCmd(node_rx, rf_rx, 'rx_en');

% Enable the buffers for TX / RX
wl_basebandCmd(node_tx, rf_tx, 'tx_buff_en');
wl_basebandCmd(node_rx, rf_rx, 'rx_buff_en');

% Send the Ethernet trigger to start the TX
eth_trig.send();

% Check that the energy trigger asserted the BASEBAND output trigger to the WARPLab buffers core.  
%     NOTE:  This will prevent users from reading stale IQ data in the node.
%
trigger_asserted = node_rx.wl_triggerManagerCmd('output_state_read', [trig_out_ids.BASEBAND]);

if (trigger_asserted)
    % Read the IQ and RSSI data from the RX node 
    rx_iq    = wl_basebandCmd(node_rx, rf_rx_vec, 'read_IQ', 0, rx_length);
    rx_rssi  = wl_basebandCmd(node_rx, rf_rx_vec, 'read_RSSI', 0, rssi_length);

    % Disable the buffers and RF interfaces for TX / RX
    wl_basebandCmd(nodes, ifc_ids.RF_ALL, 'tx_rx_buff_dis');
    wl_interfaceCmd(nodes, ifc_ids.RF_ALL, 'tx_rx_dis');

    % Clear the held energy detection trigger at our receiver
    wl_triggerManagerCmd(node_rx, 'output_state_clear', [trig_out_ids.BASEBAND]);
    % wl_triggerManagerCmd(node_rx, 'output_state_clear', [trig_out_ids.BASEBAND, trig_out_ids.AGC]);
else
    % Disable the buffers and RF interfaces for TX / RX
    wl_basebandCmd(nodes, ifc_ids.RF_ALL, 'tx_rx_buff_dis');
    wl_interfaceCmd(nodes, ifc_ids.RF_ALL, 'tx_rx_dis');
    error('Energy Trigger did not assert.  Please try lowering the energy detection threshold or changing the Tx/Rx gains.');
end


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Visualize results
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

figure(1);clf;

% Plot IQ data
ax(1) = subplot(2,2,1);
plot(0:(length(rx_iq)-1),real(rx_iq))
xlabel('Sample Index')
title('Received I')
axis([1 rx_length -1 1])

ax(2) = subplot(2,2,2);
plot(0:(length(rx_iq)-1),imag(rx_iq))
xlabel('Sample Index')
title('Received Q')
axis([1 rx_length -1 1])

linkaxes(ax,'xy')

% Plot RSSI data
subplot(2,1,2)
plot(0:(length(rx_rssi)-1),rx_rssi)
xlabel('Sample Index')
title('Received RSSI')
axis([0 rssi_length 0 1024])


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% END
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%