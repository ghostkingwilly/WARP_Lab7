%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% wl_example_continuous_tx.m
%
%   This example demonstrates continuous transmit feature of WARP nodes.
%
% Requirements:
%     2 WARP nodes (same hardware generation); 1 RF interface each
%     WARPLab 7.6.0 and higher
%
%
% NOTE:  This example will not work with WARPLab 7.5.x because nodes have the 
%     restriction that Read IQ / Write IQ cannot occur while transmitting or 
%     receiving.
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

clear;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Top Level Control Variables
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

NUM_RECEPTIONS     = 4;                     % Number of times to capture and plot the Tx waveform

TX_LEN             = 400;                   % Transmission length (in samples).
                                            %     See documentation for 'continuous_tx' in the baseband buffers documentation for 
                                            %     more information on the restrictions on transmission length:
                                            %         http://warpproject.org/trac/wiki/WARPLab/Reference/Baseband/Buffers#continuous_tx

RX_LEN             = TX_LEN * 4;            % Reception length (in samples).  
                                            %     Setting this to a default of 4x the transmission length so that it is easy to see the 
                                            %     repetition of the TX waveform when using continuous tx mode.
                                            %
                                            %     NOTE:  Be careful to not exceed the maximum number of samples when using larger 
                                            %         transmission lengths.



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

% For both nodes, we will allow Ethernet to trigger the baseband buffers
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

% Set the Transmit and Receive interfaces
%     Transmit from RFA of one node to RFA of the other node
%
% NOTE:  Variables are used to make it easier to change interfaces.
% 
rf_tx         = ifc_ids.RF_A;                    % Transmit RF interface
rf_rx         = ifc_ids.RF_A;                    % Receive RF interface
rf_rx_name    = 'RFA';                           % RF interface string used for plots

rf_tx_vec     = ifc_ids.RF_A;                    % Vector version of transmit RF interface
rf_rx_vec     = ifc_ids.RF_A;                    % Vector version of receive RF interface

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

% Get the sample frequency from the board
ts      = 1 / (wl_basebandCmd(nodes(1), 'tx_buff_clk_freq'));

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
tx_length = TX_LEN;
rx_length = RX_LEN;

% Check the transmission length
if (tx_length > maximum_buffer_len) 
    error('Node supports max transmission length of %d samples.  Requested %d samples.', maximum_buffer_len, tx_length); 
end

% Check the reception length
if (rx_length > maximum_buffer_len) 
    error('Node supports max transmission length of %d samples.  Requested %d samples.', maximum_buffer_len, rx_length); 
end

% Set the length for the transmit and receive buffers based on the transmission length
wl_basebandCmd(nodes, 'tx_length', tx_length);
wl_basebandCmd(nodes, 'rx_length', rx_length);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Signal processing to generate transmit signal
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Create the payload
t = [0:ts:((tx_length - 1) * ts)].';                                      % Create time vector(Sample Frequency is ts (Hz))

envelope      = (((tx_length - 1) * ts) - t) ./ ((tx_length - 1) * ts);   % Create a linear envelope from 1 to 0 over the sample length

payload       = envelope .* exp(j*2*pi * 5e6 * t);                        % Waveform:  1 MHz sinusoid multiplied by the envelope

txData        = [payload];                                                % Create the transmit data



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Transmit and receive signal using WARPLab
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Set continuous transmit mode
%     NOTE:  Change this condition to 'false' to see the same example without continuous transmit mode
%
if (true)
    wl_basebandCmd(node_tx, 'continuous_tx', true);
else
    wl_basebandCmd(node_tx, 'continuous_tx', false);
end

% Transmit IQ data to the TX node
wl_basebandCmd(node_tx, rf_tx_vec, 'write_IQ', txData);

% Enabled the RF interfaces for TX / RX
wl_interfaceCmd(node_tx, rf_tx, 'tx_en');
wl_interfaceCmd(node_rx, rf_rx, 'rx_en');

% Enable the buffers for TX / RX
wl_basebandCmd(node_tx, rf_tx, 'tx_buff_en');
wl_basebandCmd(node_rx, rf_rx, 'rx_buff_en');

% Send the Ethernet trigger to start the TX / RX
eth_trig.send();

% Get the waveform NUM_RECEPTIONS times
for ii = 1:NUM_RECEPTIONS
    % Read the IQ data from the RX node 
    rx_iq(:, ii) = wl_basebandCmd(node_rx, rf_rx_vec, 'read_IQ', 0, rx_length);
    
    % Send the Ethernet trigger to re-start the RX
    eth_trig.send();
    
    % Pause for a moment to let the node continue to transmit
    pause(0.11);
end

% Disable the buffers and RF interfaces for TX / RX
wl_basebandCmd(nodes, ifc_ids.RF_ALL, 'tx_rx_buff_dis');
wl_interfaceCmd(nodes, ifc_ids.RF_ALL, 'tx_rx_dis');



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Visualize results
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Plotting variables
iq_range           = 1;                     % Plots IQ values in the range:  [-iq_range, iq_range]


figure(1);clf;

for ii = 1:NUM_RECEPTIONS
    sprintf('Run %d', ii);

    % Plot IQ Data
    %
    ax(2 * ii - 1) = subplot(NUM_RECEPTIONS, 2, (2 * ii - 1));
    plot(0:(length(rx_iq) - 1), real(rx_iq(:, ii)))
    xlabel('Sample Index')
    title(sprintf('Run %d - %s: Received I', ii, rf_rx_name))
    axis([1 rx_length -iq_range iq_range])
    grid on

    ax(2 * ii) = subplot(NUM_RECEPTIONS, 2, (2 * ii));
    plot(0:(length(rx_iq) - 1), imag(rx_iq(:, ii)))
    xlabel('Sample Index')
    title(sprintf('Run %d - %s: Received Q', ii, rf_rx_name))
    axis([1 rx_length -iq_range iq_range])
    grid on
end



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% END
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%