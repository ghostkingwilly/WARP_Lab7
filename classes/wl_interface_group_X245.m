%-------------------------------------------------------------------------
% WARPLab Framework
%
% Copyright 2013, Mango Communications. All rights reserved.
%           Distributed under the WARP license  (http://warpproject.org/license)
%
% Chris Hunter (chunter [at] mangocomm.com)
% Patrick Murphy (murphpo [at] mangocomm.com)
% Erik Welsh (welsh [at] mangocomm.com)
%-------------------------------------------------------------------------

classdef wl_interface_group_X245 < wl_interface_group
% Interface group object for the X245 transceivers
%     User code should not use this object directly -- the parent wl_node will
%     instantiate the appropriate interface group object for the hardware in use
    properties (SetAccess = protected)
        num_interface;      % number of interfaces in this group
        ID;                 % vector of IDs for all interfaces in this group
        label;              % cell vector of labels for the interfaces
        description;        % description of this object
    end
    
    properties (SetAccess = public, Hidden = false, Constant = true)
        % Buffer State
        STANDBY                        = 0;
        RX                             = 1;
        TX                             = 2;
    end
    
    properties(Hidden = true,Constant = true)
        % These constants define specific command IDs used by this object.
        % Their C counterparts are found in wl_interface.h
        GRP                            = 'interface';
        CMD_TX_EN                      = 1;                % 0x000001
        CMD_RX_EN                      = 2;                % 0x000002
        CMD_TX_RX_DIS                  = 3;                % 0x000003
        CMD_TX_RX_STATE                = 4;                % 0x000004
        CMD_CHANNEL                    = 5;                % 0x000005
        CMD_TX_GAINS                   = 6;                % 0x000006
        CMD_RX_GAINS                   = 7;                % 0x000007
        CMD_TX_LPF_CORN_FREQ           = 8;                % 0x000008
        CMD_RX_LPF_CORN_FREQ           = 9;                % 0x000009
        CMD_RX_HPF_CORN_FREQ           = 10;               % 0x00000A
        CMD_RX_GAINCTRL_SRC            = 11;               % 0x00000B
        CMD_RX_RXHP_CTRL               = 12;               % 0x00000C
        CMD_RX_LPF_CORN_FREQ_FINE      = 13;               % 0x00000D
    end
    
    methods
        function obj = wl_interface_group_X245(varargin)
            obj.description = '2.4/5GHz RF Interface Group';

            radioSel = varargin{1};
            FMCMode  = 0;
            
            if (nargin == 2)
                FMCMode = strcmpi(varargin{2},'w3');
            end
            
            obj.ID    = [];
            obj.label = {};
            
            for n = radioSel
                obj.ID = [obj.ID, uint32(2.^((n - 1) + 28))];
                if (FMCMode == 0 || n < 3)
                    obj.label = [obj.label, {sprintf('RF%c',n+64)}];
                else
                    obj.label = [obj.label, {sprintf('RF%c (FMC RF%c)', (n + 64), (n + 62))}];
                end
            end            
        end
        
        function out = wl_ifcValid(obj,rfSel)
            rfSel = repmat(rfSel,size(obj.ID));
            out   = max(bitand(rfSel,obj.ID)>1);
        end
        
        function out = procCmd(obj, nodeInd, node, rfSel, cmdStr, varargin)
            % wl_interface procCmd(obj, nodeInd, node, rfSel, cmdStr, varargin)
            %     obj:       Node object (when called using dot notation)
            %     nodeInd:   Index of the current node, when wl_node is iterating over nodes
            %     node:      Current node object (the owner of this baseband)
            %     rfSel:     RF interface(s) to apply the interface command
            %     cmdStr:    Command string of the interface command
            %     varargin:
            %         [1:N}  Command arguments
            %        
            out = [];
            
            % Process the rfSel argument
            if(strcmp(rfSel, 'RF_ALL'))
            
                % If rfSel is the "magic string" 'RF_ALL', then select all RF interfaces
                rfSel          = sum(obj.ID);
                num_interfaces = 1;
                
            else
                
                % RF interfaces specified by RF interface IDs
                num_interfaces = length(rfSel);
            end
            
            % Check to make sure this rfSel actually applies to me
            if(bitand(rfSel, sum(obj.ID)) ~= rfSel)
               error('Invalid interface selection'); 
            end
            
            switch(lower(cmdStr))                
                %---------------------------------------------------------
                case 'tx_en'
                    % Enable transmit mode for selected interfaces
                    %
                    % Arguments: none
                    % Returns: none
                    %
                    
                    % Check interface selection 
                    if(num_interfaces ~= 1)
                        error('%s: Requires scalar interface selection.  If trying to enable multiple interfaces, use bitwise addition of interfaces:  RFA + RFB.', cmdStr);
                    end
                    
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_TX_EN), rfSel);
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'rx_en'
                    % Enable receive mode for selected interfaces
                    %
                    % Arguments: none
                    % Returns: none
                    %
                    
                    % Check interface selection 
                    if(num_interfaces ~= 1)
                        error('%s: Requires scalar interface selection.  If trying to enable multiple interfaces, use bitwise addition of interfaces:  RFA + RFB.', cmdStr);
                    end
                    
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_RX_EN), rfSel);
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'tx_rx_dis'
                    % Disable transmit and receive for selected interfaces (standby mode)
                    %
                    % Arguments: none
                    % Returns: none
                    %
                    
                    % Check interface selection 
                    if(num_interfaces ~= 1)
                        error('%s: Requires scalar interface selection.  If trying to disable multiple interfaces, use bitwise addition of interfaces:  RFA + RFB.', cmdStr);
                    end
                    
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_TX_RX_DIS), rfSel);
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'read_tx_rx_state'
                    % Read the current state of the interface
                    %
                    % Requires BUFF_SEL: Yes
                    % Arguments: none
                    % Returns: Current state of the buffer:  TX, RX or STANDBY
                    %
                    
                    for ifc_index = 1:num_interfaces
                    
                        if(isSingleBuffer(rfSel(ifc_index)) == 0)
                            error('%s: Buffer selection must be singular. Use vector notation for reading from multiple buffers e.g. [RFA, RFB]', cmdStr);
                        end

                        myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_TX_RX_STATE));
                        myCmd.addArgs(rfSel(ifc_index));
                        
                        resp  = node.sendCmd(myCmd);
                        ret   = resp.getArgs();
                        
                        if(ret(1) == 0)
                            out(ifc_index) = obj.STANDBY;
                        elseif(ret(1) == 1)
                            out(ifc_index) = obj.RX;
                        elseif(ret(1) == 2)
                            out(ifc_index) = obj.TX;
                        else
                            error('%s: Node returned an unknown buffer state.', cmdStr);
                        end                                              
                    end
                    
                %---------------------------------------------------------
                case 'channel'
                    % Tune selected interfaces to the specified band and channel
                    %
                    % Arguments: (float BAND, int CHAN)
                    %     BAND: Must be 2.4 or 5, to select 2.4GHz or 5GHz channels
                    %     CHAN: Must be integer in [1,14] for BAND=2.4, [1,44] for BAND=5
                    % Returns: none
                    %
                    % BAND and CHAN must be scalars (same values for all specified interfaces) or
                    %    1-D vectors (one value per interface) with length equal to the length of the interface ID vector
                    %
                    % Band/Channel - Center Frequency Map:
                    % ||||=  2.4GHz  =||||=  5GHz  =||
                    % || Chan || Freq || Chan || Freq ||
                    % ||  1 ||    2412 ||  1 ||    5180 ||
                    % ||  2 ||    2417 ||  2 ||    5190 ||
                    % ||  3 ||    2422 ||  3 ||    5200 ||
                    % ||  4 ||    2427 ||  4 ||    5220 ||
                    % ||  5 ||    2432 ||  5 ||    5230 ||
                    % ||  6 ||    2437 ||  6 ||    5240 ||
                    % ||  7 ||    2442 ||  7 ||    5260 ||
                    % ||  8 ||    2447 ||  8 ||    5270 ||
                    % ||  9 ||    2452 ||  9 ||    5280 ||
                    % ||  10 ||    2457 ||  10 ||    5300 ||
                    % ||  11 ||    2462 ||  11 ||    5310 ||
                    % ||  12 ||    2467 ||  12 ||    5320 ||
                    % ||  13 ||    2472 ||  13 ||    5500 ||
                    % ||  14 ||    2484 ||  14 ||    5510 ||
                    % || || ||  15 ||    5520 ||
                    % || || ||  16 ||    5540 ||
                    % || || ||  17 ||    5550 ||
                    % || || ||  18 ||    5560 ||
                    % || || ||  19 ||    5580 ||
                    % || || ||  20 ||    5590 ||
                    % || || ||  21 ||    5600 ||
                    % || || ||  22 ||    5620 ||
                    % || || ||  23 ||    5630 ||
                    % || || ||  24 ||    5640 ||
                    % || || ||  25 ||    5660 ||
                    % || || ||  26 ||    5670 ||
                    % || || ||  27 ||    5680 ||
                    % || || ||  28 ||    5700 ||
                    % || || ||  29 ||    5710 ||
                    % || || ||  30 ||    5720 ||
                    % || || ||  31 ||    5745 ||
                    % || || ||  32 ||    5755 ||
                    % || || ||  33 ||    5765 ||
                    % || || ||  34 ||    5785 ||
                    % || || ||  35 ||    5795 ||
                    % || || ||  36 ||    5805 ||
                    % || || ||  37 ||    5825 ||
                    % || || ||  38 ||    5860 ||
                    % || || ||  39 ||    5870 ||
                    % || || ||  40 ||    5875 ||
                    % || || ||  41 ||    5880 ||
                    % || || ||  42 ||    5885 ||
                    % || || ||  43 ||    5890 ||
                    % || || ||  44 ||    5865 ||
                    %
                    if(isempty(varargin))                  % read-back mode
                        error('%s: Channel read back not implemented.', cmdStr);
                        
                    elseif(length(varargin) == 2)          % write mode
                    
                        band    = varargin{1};
                        channel = varargin{2};
                        
                        for n = 1:num_interfaces
                    
                            myCmd   = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_CHANNEL), rfSel(n));
                        
                            % Check arguments                        
                            if(length(band) == num_interfaces)
                                node_band = band(n);
                            else
                                error('%s: Expects band selection to be a vector of length equal to the number of interfaces.', cmdStr);
                            end
                            
                            if(length(channel) == num_interfaces)
                                node_channel = channel(n);
                            else
                                error('%s: Expects channel selection to be a vector of length equal to the number of interfaces.', cmdStr);
                            end
                            
                            if(node_band == 2.4)
                                node_band = 0;
                                if((node_channel < 1) || (node_channel > 14))
                                    error('%s: Expects channel selection to be between [1,14] for the 2.4GHz band', cmdStr);
                                end
                            elseif(node_band == 5)
                                node_band = 1;
                                if((node_channel < 1) || (node_channel > 44))
                                    error('%s: Expects channel selection to be between [1,44] for the 5GHz band', cmdStr);
                                end
                            else
                                error('%s: Expects band selection 2.4 or 5', cmdStr);
                            end

                            myCmd.addArgs(node_band, node_channel);
                            node.sendCmd(myCmd);
                        end
                    else
                        error('%s:  Expects two arguments: a band and a channel', cmdStr);
                    end

                %---------------------------------------------------------
                case 'tx_gains'
                    % Sets the gains for the variable gain amplifiers in the MAX2829 Tx path
                    % Refer to MAX2829 datasheet for curves of gain value vs actual gain at 2.4 and 5GHz
                    %
                    % Arguments: (int BB_GAIN, int RF_GAIN)
                    %  BB_GAIN: Must be integer in [0,1,2,3] for approx [-5, -3, -1.5, 0]dB baseband gain
                    %  RF_GAIN: Must be integer in [0:63] for approx [0:31]dB RF gain
                    % Returns: none
                    %
                    % BB_GAIN and RF_GAIN must be scalars (same values for all specified interfaces) or
                    %  1-D vectors (one value per interface) with length equal to the length of the interface ID vector
                    %
                    % NOTE:  The parameters are in the order in which they are applied to the signal (ie baseband gain, then RF gain)
                    %
                    if(isempty(varargin))                  % read-back mode
                        error('%s: Tx gain read back not implemented.', cmdStr);
                        
                    elseif(length(varargin) == 2)          % write mode
                    
                        bbGain = varargin{1};
                        rfGain = varargin{2};
                        
                        for n = 1:num_interfaces
                    
                            myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_TX_GAINS), rfSel(n));
                        
                            % Check arguments                        
                            if(length(bbGain) == num_interfaces)
                                node_bb_gain = bbGain(n);
                            else
                                error('%s: Expects baseband gain selection to be a vector of length equal to the number of interfaces.', cmdStr);
                            end
                            
                            if(length(rfGain) == num_interfaces)
                                node_rf_gain = rfGain(n);
                            else
                                error('%s: Expects rf gain selection to be a vector of length equal to the number of interfaces.', cmdStr);
                            end
                                                        
                            if((node_bb_gain > 3) || (node_rf_gain > 63))
                                error('%s: Baseband gain must be in [0,3] and RF gain must be in [0,63]', cmdStr);
                            end

                            myCmd.addArgs(node_bb_gain, node_rf_gain);
                            node.sendCmd(myCmd);
                        end
                    else
                        error('%s:  Expects two arguments: a baseband gain and an RF gain', cmdStr);
                    end

                %---------------------------------------------------------
                case 'rx_gains'
                    % Sets the gains for the variable gain amplifiers in the MAX2829 Rx path
                    % Refer to MAX2829 datasheet for curves of gain value vs actual gain at 2.4 and 5GHz
                    %
                    % Arguments: (int RF_GAIN, int BB_GAIN)
                    %  RF_GAIN: Must be integer in [1,2,3] for approx [0,15,30]dB RF gain
                    %  BB_GAIN: Must be integer in [0:31] for approx [0:63]dB baseband gain
                    % Returns: none
                    %
                    % BB_GAIN and RF_GAIN must be scalars (same values for all specified interfaces) or
                    %  1-D vectors (one value per interface) with length equal to the length of the interface ID vector
                    %
                    % NOTE:  The parameters are in the order in which they are applied to the signal (ie RF gain, then baseband gain)
                    %
                    if(isempty(varargin))                  % read-back mode
                        error('%s: Rx gain read back not implemented.', cmdStr);
                        
                    elseif(length(varargin) == 2)          % write mode
                    
                        rfGain = varargin{1};
                        bbGain = varargin{2};
                        
                        for n = 1:num_interfaces
                    
                            myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_RX_GAINS), rfSel(n));
                        
                            % Check arguments                        
                            if(length(bbGain) == num_interfaces)
                                node_bb_gain = bbGain(n);
                            else
                                error('%s: Expects baseband gain selection to be a vector of length equal to the number of interfaces.', cmdStr);
                            end
                            
                            if(length(rfGain) == num_interfaces)
                                node_rf_gain = rfGain(n);
                            else
                                error('%s: Expects rf gain selection to be a vector of length equal to the number of interfaces.', cmdStr);
                            end
                                                        
                            if((node_bb_gain > 31) || (node_rf_gain > 3))
                                error('%s: RF gain must be in [0,3] and baseband gain must be in [0,31]', cmdStr);
                            end

                            myCmd.addArgs(node_rf_gain, node_bb_gain);
                            node.sendCmd(myCmd);
                        end
                    else
                        error('%s:  Expects two arguments: an RF gain and a baseband gain', cmdStr);
                    end
                    
                %---------------------------------------------------------
                case 'tx_lpf_corn_freq'
                    % Sets the corner frequency for the MAX2829 Tx path low pass filter
                    % Refer to MAX2829 datasheet for curves of the frequency response with each setting
                    %
                    % Arguments: (int FILT)
                    %     FILT: Must be integer in [1,2,3] for approx [12,18,24]MHz corner frequencies ([24,36,48]MHz bandwidths)
                    % Returns: none
                    %
                    % FILT must be scalar (same value for all specified interfaces) or 1-D vector (one value per interface) 
                    %     with length equal to the length of the interface ID vector
                    %
                    if(isempty(varargin))                  % read-back mode
                        error('%s: Tx LPF corner frequency read back not implemented.', cmdStr);
                        
                    elseif(length(varargin) == 1)          % write mode
                    
                        cornFreq = varargin{1};
                        
                        for n = 1:num_interfaces
                    
                            myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_TX_LPF_CORN_FREQ), rfSel(n));
                        
                            % Check arguments                        
                            if(length(cornFreq) == num_interfaces)
                                node_corn_freq = cornFreq(n);
                            else
                                error('%s: Expects corner frequency selection to be a vector of length equal to the number of interfaces.', cmdStr);
                            end
                            
                            if((node_corn_freq == 0) || (node_corn_freq > 3))
                                error('%s: Corner frequency filter must be in [1, 3]', cmdStr);
                            end

                            myCmd.addArgs(node_corn_freq);
                            node.sendCmd(myCmd);
                        end
                    else
                        error('%s:  Expects one argument', cmdStr);
                    end
                    
                %---------------------------------------------------------
                case 'rx_lpf_corn_freq_fine'
                    % Sets the fine corner frequency for the MAX2829 Rx path low pass filter
                    % Refer to MAX2829 datasheet for curves of the frequency response with each setting
                    %
                    % Arguments: (int FILT)
                    %     FILT: Must be integer in [0,1,2,3,4,5] for
                    %       [90,95,100,105,110]% scaling to LPF corner frequency
                    %
                    % Returns: none
                    %
                    % FILT must be scalar (same value for all specified interfaces) or 1-D vector (one value per interface) 
                    %     with length equal to the length of the interface ID vector
                    %
                    if(isempty(varargin))                  % read-back mode
                        error('%s: Rx LPF fine corner frequency read back not implemented.', cmdStr);
                        
                    elseif(length(varargin) == 1)          % write mode
                    
                        cornFreq = varargin{1};
                        
                        for n = 1:num_interfaces
                    
                            myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_RX_LPF_CORN_FREQ_FINE), rfSel(n));
                        
                            % Check arguments                        
                            if(length(cornFreq) == num_interfaces)
                                node_corn_freq = cornFreq(n);
                            else
                                error('%s: Expects corner frequency selection to be a vector of length equal to the number of interfaces.', cmdStr);
                            end
                            
                            if((node_corn_freq < 0) || (node_corn_freq > 5))
                                error('%s: Corner frequency filter must be in [0, 5]', cmdStr);
                            end

                            myCmd.addArgs(node_corn_freq);
                            node.sendCmd(myCmd);
                        end
                    else
                        error('%s:  Expects one argument', cmdStr);
                    end    
                    
                %---------------------------------------------------------
                case 'rx_lpf_corn_freq'
                    % Sets the corner frequency for the MAX2829 Rx path low pass filter
                    % Refer to MAX2829 datasheet for curves of the frequency response with each setting
                    %
                    % Arguments: (int FILT)
                    %     FILT: Must be integer in [0,1,2,3] for approx [7.5,9.5,14,18]MHz corner 
                    %           frequencies ([15,19,28,36]MHz bandwidths)
                    % Returns: none
                    %
                    % FILT must be scalar (same value for all specified interfaces) or 1-D vector (one value per interface) 
                    %     with length equal to the length of the interface ID vector
                    %
                    if(isempty(varargin))                  % read-back mode
                        error('%s: Rx LPF corner frequency read back not implemented.', cmdStr);
                        
                    elseif(length(varargin) == 1)          % write mode
                    
                        cornFreq = varargin{1};
                        
                        for n = 1:num_interfaces
                    
                            myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_RX_LPF_CORN_FREQ), rfSel(n));
                        
                            % Check arguments                        
                            if(length(cornFreq) == num_interfaces)
                                node_corn_freq = cornFreq(n);
                            else
                                error('%s: Expects corner frequency selection to be a vector of length equal to the number of interfaces.', cmdStr);
                            end
                            
                            if((node_corn_freq < 0) || (node_corn_freq > 3))
                                error('%s: Corner frequency filter must be in [0, 3]', cmdStr);
                            end

                            myCmd.addArgs(node_corn_freq);
                            node.sendCmd(myCmd);
                        end
                    else
                        error('%s:  Expects one argument', cmdStr);
                    end
                    
                %---------------------------------------------------------
                case 'rx_hpf_corn_freq'
                    % Sets the corner frequency for the MAX2829 Rx path high pass filter when RXHP = 'disable' (ie 0)
                    % 
                    % Arguments: (int MODE)
                    %     MODE: Must be 0 (HPF corner of 100 Hz) or 1 (default; HPF corner of 30 kHz)
                    % Returns: none
                    %
                    % MODE must be scalar (same value for all specified interfaces) or 1-D vector (one value per interface) 
                    %     with length equal to the length of the interface ID vector
                    %
                    % The MAX2829 default setting is 1 (30 kHz corner frequency). This is the better setting for most
                    %  applications. The lower cutoff frequency (100 Hz) should only be used when the Rx waveform  has
                    %  energy in the 60 kHz around DC.
                    %
                    % This filter setting is only used when RXHP is 'disable' (ie 0).
                    %                
                    if(isempty(varargin))                  % read-back mode
                        error('%s: Rx HPF corner frequency read back not implemented.', cmdStr);
                        
                    elseif(length(varargin) == 1)          % write mode
                    
                        cornFreq = varargin{1};
                        
                        for n = 1:num_interfaces
                    
                            myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_RX_HPF_CORN_FREQ), rfSel(n));
                        
                            % Check arguments                        
                            if(length(cornFreq) == num_interfaces)
                                node_corn_freq = cornFreq(n);
                            else
                                error('%s: Expects corner frequency selection to be a vector of length equal to the number of interfaces.', cmdStr);
                            end
                            
                            if((node_corn_freq < 0) || (node_corn_freq > 1))
                                error('%s: Corner frequency mode must be in [0, 1]', cmdStr);
                            end

                            myCmd.addArgs(node_corn_freq);
                            node.sendCmd(myCmd);
                        end
                    else
                        error('%s:  Expects one argument', cmdStr);
                    end

                %---------------------------------------------------------
                case 'rx_gain_mode'
                    % Sets the gain selection mode
                    %
                    % Arguments: (string MODE)
                    %     MODE: 'automatic' for AGC, or 'manual' for manual gain control
                    % Returns: none
                    %
                    
                    % Check interface selection 
                    if(num_interfaces ~= 1)
                        error('%s: Requires scalar interface selection.  If trying to set multiple interfaces, use bitwise addition of interfaces:  RFA + RFB.', cmdStr);
                    end

                    if(length(varargin) ~= 1)
                        error('%s: Requires one argument', cmdStr);
                    end
                    
                    if(strcmp(lower(varargin{1}) , 'automatic'))
                        myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_RX_GAINCTRL_SRC), rfSel, 1);
                    elseif(strcmp(lower(varargin{1}) , 'manual'))
                        myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_RX_GAINCTRL_SRC), rfSel, 0);
                    else
                        error( 'Invalid rx_gain_mode: %s (must be ''automatic'' or ''manual'')', varargin{1});
                    end

                    node.sendCmd(myCmd);

                %---------------------------------------------------------
                case 'rx_rxhp'
                    % Sets the RXHP mode on the node when in manual gain control
                    %
                    % Arguments: (boolean MODE)
                    %     MODE: true enables RXHP on the node when in manual gain control
                    %           false disables RXHP on the node when in manual gain control
                    %
                    % Returns: none
                    %
                    
                    % Check interface selection 
                    if(num_interfaces ~= 1)
                        error('%s: Requires scalar interface selection.  If trying to set multiple interfaces, use bitwise addition of interfaces:  RFA + RFB.', cmdStr);
                    end

                    if(length(varargin) ~= 1)
                        error('%s: Requires one argument', cmdStr);
                    end
                    
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_RX_RXHP_CTRL), rfSel);
                    mode  = varargin{1};
                    
                    if (ischar(mode)) 
                        % Print warning that this syntax will be deprecated
                        try
                            temp = evalin('base', 'wl_rx_rxhp_did_warn');
                        catch
                            fprintf('WARNING:   This syntax for rx_rxhp() is being deprecated.\n');
                            fprintf('WARNING:   Please use:  wl_interfaceCmd(node, RF_SEL, ''rx_rxhp'', <true, false>);\n');
                            fprintf('WARNING:   See WARPLab documentation for more information\n\n');
                            
                            assignin('base', 'wl_rx_rxhp_did_warn', 1)
                        end
                    
                        if(strcmp(lower(mode),'enable'))
                            mode = 1;
                        elseif(strcmp(lower(mode),'disable'))
                            mode = 0;
                        else
                            error('mode selection must be ''enable'' or ''disable''')
                        end
                    elseif (islogical(mode))
                        if(mode)
                            mode = 1;
                        else
                            mode = 0;
                        end
                    else 
                        error('mode selection must be true or false')
                    end
                    
                    myCmd.addArgs(mode);
                    node.sendCmd(myCmd);

                %---------------------------------------------------------
                otherwise
                    error('unknown command ''%s''', cmdStr);
            end
            
            if(iscell(out)==0 && numel(out)~=1)
                out = {out}; 
            end
        end 
    end % methods
end % classdef


function out = isSingleBuffer(sel)
    out = (length(strfind(dec2bin(sel), '1')) == 1);
end        
