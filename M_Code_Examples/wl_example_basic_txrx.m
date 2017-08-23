%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% wl_example_basic_txrx.m
%
%   This example demonstrates basic transmission and reception of waveforms 
% between two WARP nodes.  One node will transmit a simple sinusoid and the 
% other node will receive the sinusoid.
%
% Requirements:
%     2 WARP nodes (same hardware generation); 2 RF interfaces each
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

clear;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Set up the WARPLab experiment
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Create a vector of node objects
nodes = wl_initNodes(2);

% Set up transmit and receive nodes
%     NOTE:  Transmit from nodes(1) to nodes(2)
%
node_tx = nodes(1);
node_rx = nodes(2);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Set up Trigger Manager
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Create a UDP broadcast trigger and tell each node to be ready for it
%     NOTE:  This will allow us to trigger both nodes to begin transmission / reception of IQ data
%
eth_trig = wl_trigger_eth_udp_broadcast;
wl_triggerManagerCmd(nodes, 'add_ethernet_trigger', [eth_trig]);

% Read Trigger IDs into workspace
trig_in_ids  = wl_getTriggerInputIDs(nodes(1));
trig_out_ids = wl_getTriggerOutputIDs(nodes(1));

% For both nodes, we will allow Ethernet A to trigger the baseband buffers
wl_triggerManagerCmd(nodes, 'output_config_input_selection', [trig_out_ids.BASEBAND], [trig_in_ids.ETH_A]);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Set up the Interface parameters
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Get IDs for the interfaces on the boards.  
%
% NOTE:  This example assumes each board has the same interface capabilities (ie 2 RF 
%   interfaces; RFA and RFB).  Therefore, we only need to get the IDs from one of the boards.
%
ifc_ids = wl_getInterfaceIDs(nodes(1));

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
wl_interfaceCmd(nodes, ifc_ids.RF_ALL, 'rx_gains', 1, 15);

% Set the TX gains on all interfaces
%     - Tx Baseband Gain:  Must be an integer in [0:3] for approx [-5, -3, -1.5, 0]dB baseband gain
%     - Tx RF Gain      :  Must be an integer in [0:63] for approx [0:31]dB RF gain
% 
% NOTE:  The gains may need to be modified depending on your experimental setup
% 
wl_interfaceCmd(nodes, ifc_ids.RF_ALL, 'tx_gains', 3, 30);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Set up the Baseband parameters
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Get the baseband sampling frequencies from the board
ts_tx   = 1 / (wl_basebandCmd(nodes(1), 'tx_buff_clk_freq'));
ts_rx   = 1 / (wl_basebandCmd(nodes(1), 'rx_buff_clk_freq'));
ts_rssi = 1 / (wl_basebandCmd(nodes(1), 'rx_rssi_clk_freq'));

% Get the maximum I/Q buffer length
%
% NOTE:  This example assumes that each board has the same baseband capabilities (ie both nodes are 
%   the same WARP hardware version, for example WARP v3).  This example also assumes that each RF 
%   interface has the same baseband capabilities (ie the max number of TX samples is the same as the 
%   max number of RF samples). Therefore, we only need to read the max I/Q buffer length of RF_A for
%   the transmitting node.
% 
maximum_buffer_len = wl_basebandCmd(node_tx, ifc_ids.RF_A, 'tx_buff_max_num_samples');

% Set the transmission / receptions lengths (in samples)
%     See WARPLab user guide for maximum length supported by WARP hardware 
%     versions and different WARPLab versions.
tx_length    = 2^12;
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

% Create the IQ data payload
t = [0:ts_tx:((tx_length - 1) * ts_tx)].';                      % Create time vector (Sample Frequency is ts_tx (Hz))

sinusoid_1    = 0.6 * exp(j*2*pi * 1e6 * t);                    % Create  1 MHz sinusoid
% sinusoid_2    = 0.6 * exp(j*2*pi * 5e4 * t);                    % Create 50 kHz sinusoid to transmit on RF_B 

tx_data       = [sinusoid_1];                                   % Create the data to transmit on RF_A
% tx_data       = [sinusoid_1, sinusoid_2];                       % Create the data to transmit on both RF_A and RF_B


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Transmit and receive signal using WARPLab
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Transmit IQ data to the TX node
wl_basebandCmd(node_tx, [ifc_ids.RF_A], 'write_IQ', tx_data);
% wl_basebandCmd(node_tx, [ifc_ids.RF_A, ifc_ids.RF_B], 'write_IQ', tx_data);                      % Write each sinusoid to a different interface

% Enabled the RF interfaces for TX / RX
wl_interfaceCmd(node_tx, ifc_ids.RF_A, 'tx_en');
wl_interfaceCmd(node_rx, ifc_ids.RF_A, 'rx_en');
% wl_interfaceCmd(node_tx, ifc_ids.RF_ON_BOARD, 'tx_en');       % Enable both RF_A and RF_B
% wl_interfaceCmd(node_rx, ifc_ids.RF_ON_BOARD, 'rx_en');       % Enable both RF_A and RF_B

% Enable the buffers for TX / RX
wl_basebandCmd(node_tx, ifc_ids.RF_A, 'tx_buff_en');
wl_basebandCmd(node_rx, ifc_ids.RF_A, 'rx_buff_en');
% wl_basebandCmd(node_tx, ifc_ids.RF_ON_BOARD, 'tx_buff_en');   % Enable both RF_A and RF_B
% wl_basebandCmd(node_rx, ifc_ids.RF_ON_BOARD, 'rx_buff_en');   % Enable both RF_A and RF_B

% Send the Ethernet trigger to start the TX / RX
eth_trig.send();

% Read the IQ and RSSI data from the RX node 
rx_iq    = wl_basebandCmd(node_rx, [ifc_ids.RF_A], 'read_IQ', 0, rx_length);
rx_rssi  = wl_basebandCmd(node_rx, [ifc_ids.RF_A], 'read_RSSI', 0, rssi_length);
% rx_iq    = wl_basebandCmd(node_rx, [ifc_ids.RF_A, ifc_ids.RF_B], 'read_IQ', 0, rx_length);      % Read IQ data from both RF_A and RF_B
% rx_rssi  = wl_basebandCmd(node_rx, [ifc_ids.RF_A, ifc_ids.RF_B], 'read_RSSI', 0, rssi_length);  % Read RSSI data from both RF_A and RF_B

% Disable the buffers and RF interfaces for TX / RX
wl_basebandCmd(nodes, ifc_ids.RF_A, 'tx_rx_buff_dis');
wl_interfaceCmd(nodes, ifc_ids.RF_A, 'tx_rx_dis');
% wl_basebandCmd(nodes, ifc_ids.RF_ON_BOARD, 'tx_rx_buff_dis'); % Disable both RF_A and RF_B
% wl_interfaceCmd(nodes, ifc_ids.RF_ON_BOARD, 'tx_rx_dis');     % Disable both RF_A and RF_B



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Visualize results
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

figure(1);clf;

% Plot IQ Data
%
ax(1) = subplot(3, 2, 1);
plot(0:(length(tx_data) - 1), real(tx_data(:, 1)))
xlabel('Sample Index')
title('Node 1 RFA: Transmitted I')
axis([1 tx_length -1 1])

ax(2) = subplot(3, 2, 2);
plot(0:(length(tx_data) - 1), imag(tx_data(:, 1)))
xlabel('Sample Index')
title('Node 1 RFA: Transmitted Q')
axis([1 tx_length -1 1])

bx(1) = subplot(3, 2, 3);
plot(0:(length(rx_iq) - 1), real(rx_iq(:, 1)))
xlabel('Sample Index')
title('Node 2 RFA: Received I')
axis([1 rx_length -1 1])

bx(2) = subplot(3, 2, 4);
plot(0:(length(rx_iq) - 1), imag(rx_iq(:, 1)))
xlabel('Sample Index')
title('Node 2 RFA: Received Q')
axis([1 rx_length -1 1])

linkaxes([ax, bx], 'x')


% Plot RSSI data
%
subplot(3, 1, 3)
plot(0:(length(rx_rssi) - 1), rx_rssi(:, 1))
xlabel('Sample Index')
title('Node 2 RFA: Received RSSI')
axis([0 rssi_length 0 1024])


%
% NOTE:  It is left to the user to plot the IQ and RSSI data for RF_B in rx_iq(:, 2)
%     and rx_rssi(:, 2), respectively, if receiving on both RF_A and RF_B.
%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% END
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%