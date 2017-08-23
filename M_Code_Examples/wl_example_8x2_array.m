%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% wl_example_8x2_array.m
%
% Description:
%     See warpproject.org/trac/wiki/WARPLab/Examples/8x2Array
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
clear;
figure(1);clf;

USE_AGC = true;
RUN_CONTINOUSLY = false;

MAX_TX_LEN = 32768;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Set up the WARPLab experiment
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Create a vector of node objects
%     
% This experiment uses 3 nodes: 2 will act as a transmitter and 1 will act as a receiver:
%     nodes(1): Primary transmitter
%     nodes(2): Secondary transmitter (receives clocks and triggers from primary transmitter)
%     nodes(3): Receiver
%
nodes    = wl_initNodes(3);

node_tx1 = nodes(1);
node_tx2 = nodes(2);
node_rx  = nodes(3);


% Create a UDP broadcast trigger and tell each node to be ready for it
eth_trig = wl_trigger_eth_udp_broadcast;
wl_triggerManagerCmd(nodes, 'add_ethernet_trigger', [eth_trig]);

% Read Trigger IDs into workspace
trig_in_ids  = wl_getTriggerInputIDs(node_tx1);
trig_out_ids = wl_getTriggerOutputIDs(node_tx1);

% For the primary transmit node, we will allow Ethernet to trigger the baseband buffers, 
% the AGC, and external output pin 0 (which is mapped to pin 8 on the debug header). We 
% also will allow Ethernet to trigger the same signals for the receiving node.
%
wl_triggerManagerCmd([node_tx1, node_rx], 'output_config_input_selection', [trig_out_ids.BASEBAND, trig_out_ids.AGC, trig_out_ids.EXT_OUT_P0], [trig_in_ids.ETH_A]);

% For the secondary transmit node, we will allow external input pin 3 (mapped to pin 15 
% on the debug header) to trigger the baseband buffers, and the AGC
%
% Note that the below line selects both P0 and P3. This will allow the
% script to work with either the CM-PLL (where output P0 directly
% connects to input P0) or the CM-MMCX (where output P0 is usually
% connected to input P3 since both neighbor ground pins).
wl_triggerManagerCmd(node_tx2, 'output_config_input_selection', [trig_out_ids.BASEBAND, trig_out_ids.AGC], [trig_in_ids.EXT_IN_P0, trig_in_ids.EXT_IN_P3]);

% For the secondary transmit node, we enable the debounce circuity on the external input
% pin 3 to guard against any noise in the trigger signal.
%
wl_triggerManagerCmd(node_tx2, 'input_config_debounce_mode', [trig_in_ids.EXT_IN_P0, trig_in_ids.EXT_IN_P3], false); 

% To better align the transmitters, we artificially delay the primary
% transmitter's trigger outputs that drive the baseband buffers. An
% external trigger output on the primary transmitter will appear as a
% trigger input at the secondary transmitter a minimum of 5 clock cycles
% later (31.25ns). It will appear a minimum of 9 clock cycles later
% (56.25ns) when the debounce circuitry is enabled.
%
wl_triggerManagerCmd(node_tx1, 'output_config_delay', [trig_out_ids.BASEBAND, trig_out_ids.AGC], 31.25);      % 31.25ns delay

% Also, we need to delay the AGC on the receiver so that the transmitted waveform has time 
% to begin and propagate to the receiver so it can be sampled appropriately.
% 
wl_triggerManagerCmd(node_rx, 'output_config_delay', [trig_out_ids.AGC], 3000);                               % 3000ns delay

% Get IDs for the interfaces on the boards
%     NOTE:  This example assumes each board has the same interface capabilities, we only 
%         need to get the IDs from one of the boards
%
ifc_ids_4RF = wl_getInterfaceIDs(node_tx1);
ifc_ids_2RF = wl_getInterfaceIDs(node_rx);

% Set up the interface for the experiment
wl_interfaceCmd([node_tx1, node_tx2], ifc_ids_4RF.RF_ALL, 'tx_gains', 3, 30);

% Set the channel
%     NOTE:  Due to the different number of interfaces, we need to issue
%     multiple commands to the differnt types of nodes
%
wl_interfaceCmd([node_tx1, node_tx2], ifc_ids_4RF.RF_ALL, 'channel', 2.4, 1);
wl_interfaceCmd(node_rx, ifc_ids_2RF.RF_ALL, 'channel', 2.4, 1);

if(USE_AGC)
    wl_interfaceCmd(node_rx, ifc_ids_2RF.RF_ALL, 'rx_gain_mode', 'automatic');
    wl_basebandCmd(node_rx,'agc_target',-8);
else
    RxGainRF = 1; % Rx RF Gain in [1:3]
    RxGainBB = 4; % Rx Baseband Gain in [0:31]
    
    wl_interfaceCmd(node_rx, ifc_ids_2RF.RF_ALL, 'rx_gain_mode', 'manual');
    wl_interfaceCmd(node_rx, ifc_ids_2RF.RF_ALL, 'rx_gains', RxGainRF, RxGainBB);
end

wl_interfaceCmd([node_tx1, node_tx2], ifc_ids_4RF.RF_ALL, 'tx_lpf_corn_freq', 2);  % Configure Tx for 36MHz of bandwidth
wl_interfaceCmd(node_rx, ifc_ids_2RF.RF_ALL, 'rx_lpf_corn_freq', 3);               % Configure Rx for 36MHz of bandwidth

% Read the transmitter's maximum I/Q buffer length
maximum_buffer_len = wl_basebandCmd(node_tx1, ifc_ids_4RF.RF_A, 'tx_buff_max_num_samples');

% Our transmission length for this example does not need to fill the entire transmit buffer, 
% so we will use the smaller of two values: the maximum buffer length the board can support 
% or an arbitrary value defined by this script.
% 
txLength = min(MAX_TX_LEN, maximum_buffer_len);

% Set up the baseband for the experiment
wl_basebandCmd(nodes, 'tx_delay', 0);
wl_basebandCmd(nodes, 'tx_length', txLength);
wl_basebandCmd(nodes, 'rx_length', txLength);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Signal processing to generate transmit signal
% 
% NOTE:  We can send any signal we want out of each of the 8 transmit antennas. 
%     For visualization, we'll send "pink" noise of 1MHz out of each, but centered 
%     at different parts of the 40MHz band.
% 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% First generate the preamble for AGC 
%     NOTE:  The preamble corresponds to the short symbols from the 802.11a PHY standard
% 
shortSymbol_freq = [0 0 0 0 0 0 0 0 1+i 0 0 0 -1+i 0 0 0 -1-i 0 0 0 1-i 0 0 0 -1-i 0 0 0 1-i 0 0 0 0 0 0 0 1-i 0 0 0 -1-i 0 0 0 1-i 0 0 0 -1-i 0 0 0 -1+i 0 0 0 1+i 0 0 0 0 0 0 0].';
shortSymbol_freq = [zeros(32,1);shortSymbol_freq;zeros(32,1)];
shortSymbol_time = ifft(fftshift(shortSymbol_freq));
shortSymbol_time = (shortSymbol_time(1:32).')./max(abs(shortSymbol_time));
shortsyms_rep    = repmat(shortSymbol_time,1,30);
preamble_single  = shortsyms_rep;
preamble_single  = preamble_single(:);

shifts = floor(linspace(0,31,8));
for k = 1:8
   % Shift preamble for each antenna so we don't have accidental beamforming
   preamble(:,k) = circshift(preamble_single,shifts(k));
end

% Constants for generating the payload
Ts                 = 1 / (wl_basebandCmd(node_tx1, 'tx_buff_clk_freq'));
BW                 = 1; %MHz 

% Generate the payload
payload            = complex(randn(txLength-length(preamble),8),randn(txLength-length(preamble),8));
payload_freq       = fftshift(fft(payload));
freqVec            = linspace(-((1/Ts)/2e6),((1/Ts)/2e6), txLength - length(preamble));
noise_centerFreqs  = linspace(-12,12,8);

for k = 1:8
    payload_freq((freqVec < (noise_centerFreqs(k) - BW/2)) | (freqVec > (noise_centerFreqs(k) + BW/2)),k)=0;
end

payload            = ifft(fftshift(payload_freq));
txData             = [preamble;payload];

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Transmit and receive signal using WARPLab
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

wl_basebandCmd(node_tx1, ifc_ids_4RF.RF_ALL_VEC, 'write_IQ', txData(:,1:4));  % First 4 columns of txData is for primary tx
wl_basebandCmd(node_tx2, ifc_ids_4RF.RF_ALL_VEC, 'write_IQ', txData(:,5:8));  % Second 4 columns of txData is for secondary tx

wl_basebandCmd([node_tx1, node_tx2], ifc_ids_4RF.RF_ALL, 'tx_buff_en');
wl_basebandCmd(node_rx, ifc_ids_2RF.RF_ALL, 'rx_buff_en');

wl_interfaceCmd([node_tx1, node_tx2], ifc_ids_4RF.RF_ALL, 'tx_en');
wl_interfaceCmd(node_rx, ifc_ids_2RF.RF_ALL, 'rx_en');


set(gcf, 'KeyPressFcn','RUN_CONTINOUSLY=0;');
fprintf('Press any key to halt experiment\n')

while(1)
    eth_trig.send();

    rx_IQ = wl_basebandCmd(node_rx, ifc_ids_2RF.RF_ALL_VEC, 'read_IQ', 0, txLength);

    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % Visualize results
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    t = [0:Ts:(txLength-1)*Ts].';
    figure(1);
    ax(1) = subplot(2,2,1);
    plot(t,real(rx_IQ(:,1)))
    title('Re\{rx\_IQ_{RFA}\}')
    xlabel('Time (s)')
    axis([0, max(t),-1,1])
    ax(2) = subplot(2,2,2);
    plot(t,real(rx_IQ(:,2)))
    title('Re\{rx\_IQ_{RFB}\}')
    xlabel('Time (s)')
    %linkaxes(ax,'x')
    axis([0, max(t),-1,1])

    FFTSIZE = 1024;

    ax(1) = subplot(2,2,3);
    rx_IQ_slice = rx_IQ(2049:end,1);
    rx_IQ_rs = reshape(rx_IQ_slice,FFTSIZE,length(rx_IQ_slice)/FFTSIZE);
    f = linspace(-20,20,FFTSIZE);
    fft_mag = abs(fftshift(fft(rx_IQ_rs)));
    plot(f,20*log10(mean(fft_mag,2)))
    title('FFT Magnitude of rx\_IQ_{RFA}')
    xlabel('Frequency (MHz)')
    axis([-20, 20,-20,40])
    
    ax(2) = subplot(2,2,4);
    rx_IQ_slice = rx_IQ(2049:end,2);
    rx_IQ_rs = reshape(rx_IQ_slice,FFTSIZE,length(rx_IQ_slice)/FFTSIZE);
    f = linspace(-20,20,FFTSIZE);
    fft_mag = abs(fftshift(fft(rx_IQ_rs)));
    plot(f,20*log10(mean(fft_mag,2)))
    title('FFT Magnitude of rx\_IQ_{RFB}')
    xlabel('Frequency (MHz)')
    %linkaxes(ax,'x')
    axis([-20, 20,-20,40])
    

    drawnow

    if (~RUN_CONTINOUSLY)
       break 
    end

end

% Disable the TX / RX on all nodes
wl_basebandCmd([node_tx1, node_tx2], ifc_ids_4RF.RF_ALL, 'tx_rx_buff_dis');
wl_basebandCmd(node_rx, ifc_ids_2RF.RF_ALL, 'tx_rx_buff_dis');

wl_interfaceCmd([node_tx1, node_tx2], ifc_ids_4RF.RF_ALL, 'tx_rx_dis');
wl_interfaceCmd(node_rx, ifc_ids_2RF.RF_ALL, 'tx_rx_dis');


