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

classdef wl_baseband_buffers < wl_baseband
% Baseband object for the warplab_buffers reference baseband
%     User code should not use this object directly-- the parent wl_node will
%     instantiate the appropriate baseband object for the hardware in use

    properties (SetAccess = protected, Hidden = true)
        description;                % Description of this baseband object

        sample_write_iq_id;         % IQ ID for Write IQ
        
        rx_iq_warning_needed;       % RX IQ warning:  Rx Length should be set explicitly before Rx buff is enabled
        tx_iq_warning_needed;       % TX IQ warning:  Tx Length should be set explicitly before Tx buff is enabled
        
        seq_num_tracker;            % Sequence number tracker
                                    %     Array of 8 entries:  [RFA_IQ, RFA_RSSI, RFB_IQ, RFB_RSSI, RFC_IQ, RFC_RSSI, RFD_IQ, RFD_RSSI]
    end

    % NOTE:  These properties will be removed in future releases
    properties (SetAccess = public)
        txIQLen;                    % Buffer length for transmit I/Q
        rxIQLen;                    % Buffer length for receive I/Q
        rxRSSILen;                  % Buffer length for receive RSSI
        
        txIQLen_warning;
        rxIQLen_warning;
        rxRSSILen_warning;
        
        seq_num_match_severity;     % Severity of the Read IQ sequence number error
    end
    
    properties (SetAccess = public)
        readTimeout;                % Maximum time spent waiting on board to send sample packets
        check_write_iq_clipping;    % Enable checking for input sample range
    end

    properties (SetAccess = public, Hidden = false, Constant = true)
        % Buffer State
        STANDBY                        = 0;
        RX                             = 1;
        TX                             = 2;
        
        % Read IQ sequence number error severity
        SEQ_NUM_MATCH_IGNORE           = 'ignore';
        SEQ_NUM_MATCH_WARNING          = 'warning';
        SEQ_NUM_MATCH_ERROR            = 'error';
    end
    
    properties (SetAccess = public, Hidden = true)
        % Due to performance limitation of the java transport, we are 
        %   imposing a soft maximum on the number of IQ samples that 
        %   can be requested using the java transport.  Based on the 
        %   performance data, 2^20 samples is about the max number of 
        %   samples before the performance degrades due to the way 
        %   samples are being tracked.  Similarly, we have a maximum
        %   sample limit for the MEX transport due to memory constraints
        %   within Matlab.
        %
        % THIS NEEDS TO BE REEVALUATED IN FUTURE RELEASES
        %
        JAVA_TRANSPORT_MAX_IQ          = 2^20;
        MEX_TRANSPORT_MAX_IQ           = 2^25;        
    end
    
    properties(Hidden = true, Constant = true)
        % These constants define specific command IDs used by this object.
        % Their C counterparts are found in wl_baseband.h
        GRP                            = 'baseband';
        CMD_TX_DELAY                   = 1;                % 0x000001
        CMD_TX_LENGTH                  = 2;                % 0x000002
        CMD_TX_MODE                    = 3;                % 0x000003
        CMD_TX_BUFF_EN                 = 4;                % 0x000004
        CMD_RX_BUFF_EN                 = 5;                % 0x000005
        CMD_TX_RX_BUFF_DIS             = 6;                % 0x000006
        CMD_TX_RX_BUFF_STATE           = 7;                % 0x000007
        CMD_WRITE_IQ                   = 8;                % 0x000008
        CMD_READ_IQ                    = 9;                % 0x000009
        CMD_READ_RSSI                  = 10;               % 0x00000A
        CMD_RX_LENGTH                  = 11;               % 0x00000B
        CMD_WRITE_IQ_CHKSUM            = 12;               % 0x00000C
        CMD_MAX_NUM_SAMPLES            = 13;               % 0x00000D

        CMD_TXRX_COUNT_RESET           = 16;               % 0x000010
        CMD_TXRX_COUNT_GET             = 17;               % 0x000011
        
        CMD_AGC_STATE                  = 256;              % 0x000100
        CMD_AGC_DONE_ADDR              = 257;              % 0x000101
        CMD_AGC_RESET                  = 258;              % 0x000102
        CMD_AGC_RESET_MODE             = 259;              % 0x000103
        
        CMD_AGC_TARGET                 = 272;              % 0x000110
        CMD_AGC_DCO_EN_DIS             = 273;              % 0x000111
        
        CMD_AGC_CONFIG                 = 288;              % 0x000120
        CMD_AGC_IIR_HPF                = 289;              % 0x000121
        CMD_RF_GAIN_THRESHOLD          = 290;              % 0x000122
        CMD_AGC_TIMING                 = 291;              % 0x000123
        CMD_AGC_DCO_TIMING             = 292;              % 0x000124
        
        % Sample defines
        SAMPLE_IQ_SUCCESS              = 0;
        SAMPLE_IQ_ERROR                = 1;
        SAMPLE_IQ_NOT_READY            = 2;
        SAMPLE_IQ_CHECKSUM_FAILED      = 3;
        
        % Baseband defines
        BB_SEL_RFA                     = 1;                % 0x00000001
        BB_SEL_RFB                     = 2;                % 0x00000002
        BB_SEL_RFC                     = 4;                % 0x00000004
        BB_SEL_RFD                     = 8;                % 0x00000008
    end
    
    methods
        function [varargout] = subsref(obj, S)
            % Display deprecation warnings for the baseband parameters that will be removed.
            %
            if((obj.txIQLen_warning) && (numel(S) == 1) && (S.type == '.') && strcmp(S.subs, 'txIQLen'))
                fprintf('\n\nWARNING:  Param txIQLen is deprecated and will be removed!\n\n\n')
                obj.txIQLen_warning = false;
            end
            
            if((obj.rxIQLen_warning) && (numel(S) == 1) && (S.type == '.') && strcmp(S.subs, 'rxIQLen'))
                fprintf('\n\nWARNING:  Param rxIQLen is deprecated and will be removed!\n\n\n')
                obj.rxIQLen_warning = false;
            end
            
            if((obj.rxRSSILen_warning) && (numel(S) == 1) && (S.type == '.') && strcmp(S.subs, 'rxRSSILen'))
                fprintf('\n\nWARNING:  Param rxRSSILen is deprecated and will be removed!\n\n\n')
                rxRSSILen_warning = false;
            end
            
            varargout{:} = builtin('subsref', obj, S);
        end        

        
        function obj = wl_baseband_buffers()
            obj.description             = 'WARPLab Baseband for wl_buffers';
            obj.readTimeout             = 0.1;                                      % Default read timeout (in seconds)
            
            obj.check_write_iq_clipping = 1;                                        % Default to check for write IQ clipping
            
            obj.rx_iq_warning_needed    = true;                                     % Default to issue a rx_iq warning
            obj.tx_iq_warning_needed    = true;                                     % Default to issue a tx_iq warning
            
            obj.txIQLen_warning         = true;
            obj.rxIQLen_warning         = true;
            obj.rxRSSILen_warning       = true;
            
            obj.sample_write_iq_id      = uint8(0);
            
            obj.seq_num_tracker         = uint32(zeros(1, 8));                      % Initialize all 8 trackers to zero
            obj.seq_num_match_severity  = obj.SEQ_NUM_MATCH_WARNING;
        end 
        
        function out = procCmd(obj, nodeInd, node, varargin)
            % wl_baseband_buffers procCmd(obj, nodeInd, node, varargin)
            %     obj:       Node object (when called using dot notation)
            %     nodeInd:   Index of the current node, when wl_node is iterating over nodes
            %     node:      Current node object (the owner of this baseband)
            %     cmdStr:    Command string of the interface command
            %     varargin:
            %         Two forms of arguments for baseband commands:
            %             (...,'theCmdString', someArgs) - for commands that affect all buffers
            %             (...,BUFF_SEL, 'theCmdString', someArgs) - for commands that affect specific buffers
            %        
            out       = [];
            transport = node.transport;
            
            % Process initial command varargin so commands can have consistent inputs
            if(strcmp(varargin{1}, 'RF_ALL'))
                buffSel     = rfSel_to_bbSel(sum(node.interfaceGroups{1}.ID));
                num_buffers = 1;
                cmdStr      = varargin{2};

                if(length(varargin) > 2)
                    varargin = varargin(3:end);
                else
                    varargin = {};
                end
                
            elseif(ischar(varargin{1}))
               % No buffers specified
               cmdStr      = varargin{1};

               if(length(varargin) > 1)
                   varargin = varargin(2:end);
               else
                   varargin = {};
               end
               
            else
               % Buffers specified
               %     Reference implementation uses the same RF_x values to identify RF interfaces
               %     and their associated buffers. A more sophisticated baseband (where interfaces:buffers 
               %     aren't 1:1) would need a different scheme for identifying buffers from user code
               %
               buffSel       = rfSel_to_bbSel(varargin{1});
               num_buffers   = length(buffSel);
               cmdStr        = varargin{2};
               
               if(length(varargin) > 2)
                   varargin = varargin(3:end);
               else
                   varargin = {};
               end
            end
            
            cmdStr = lower(cmdStr);
            switch(cmdStr)
            
                %---------------------------------------------------------
                case 'tx_delay'
                    % Transmit delay- gets or sets the number of sample periods the baseband 
                    % waits between the trigger and starting the transission
                    %
                    % Requires BUFF_SEL: No
                    % Arguments: none or (uint32 TX_DLY)
                    % Returns: (uint32 TX_DLY) or none
                    %
                    % If an argument is specified, this command enters a write mode where
                    % that argument is written to the board. If no argument is specified,
                    % the current value of TX_DLY is read from the board.
                    % 
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_TX_DELAY));
                    
                    if(isempty(varargin))                  % Read Mode
                        myCmd.addArgs(myCmd.CMD_PARAM_READ_VAL);
                        
                        resp = node.sendCmd(myCmd);
                        ret  = resp.getArgs();
                        out  = ret(2);
                    else                                   % Write Mode
                        myCmd.addArgs(myCmd.CMD_PARAM_WRITE_VAL);
                        
                        delay = varargin{1};
                        
                        % Check arguments
                        if(length(delay) ~= 1)
                           error('%s:  Requires scalar argument.  Delay is per-node, not per-interface or per-buffer.', cmdStr);
                        end
                        
                        if(delay < 0)
                            error('%s: Tx delay must be greater than or equal to zero.\n', cmdStr);
                        end

                        % Send command to the node
                        myCmd.addArgs(delay);
                        resp = node.sendCmd(myCmd);
                        
                        % Process response from the node.  Return arguments:
                        %     [1] - Status
                        %     [2] - Tx Delay
                        %
                        for i = 1:numel(resp)                   % Needed for unicast node_group support
                            ret   = resp(i).getArgs();

                            if (delay ~= ret(2))
                                msg = sprintf('%s: Tx delay error in node %d.\n    Requested delay of %d samples.\n    Max delay of %d samples.\n', ...
                                    cmdStr, nodeInd, delay, ret(2));
                                error(msg);
                            end
                        end
                    end

                %---------------------------------------------------------
                case 'tx_length'
                    % Transmit length- sets the duration of each transmit cycle, in sample periods 
                    %
                    % Requires BUFF_SEL: No
                    % Arguments: (uint32 TX_LEN)
                    % Returns:   none
                    % 
                    % NOTE:  This will error if the user tries to read tx_length from the board.
                    %   the 'tx_buff_max_num_samples' command should be used to determine the 
                    %   capabilities of the board.
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_TX_LENGTH));
                    
                    if(isempty(varargin))                  % Read Mode
                        error('%s: ''tx_length'' does not support read.  Use ''tx_buff_max_num_samples''.', cmdStr);
                    else                                   % Write Mode
                        myCmd.addArgs(myCmd.CMD_PARAM_WRITE_VAL);
                        
                        len = varargin{1};
                        
                        % Check arguments
                        if(length(len) ~= 1)
                           error('%s:  Requires scalar argument.  Length is per-node, not per-interface or per-buffer.', cmdStr);
                        end
                        
                        if(len < 0)
                            error('%s: Tx length must be greater than or equal to zero.\n', cmdStr);
                        end
                        
                        % Send command to the node
                        myCmd.addArgs(len);
                        resp = node.sendCmd(myCmd);
                        
                        % Process response from the node.  Return arguments:
                        %     [1] - Status
                        %     [2] - Tx Length
                        %
                        for i = 1:numel(resp)                   % Needed for unicast node_group support
                            ret   = resp(i).getArgs();
                            
                            if (len ~= ret(2))
                                msg = sprintf('%s: Tx length error in node %d.\n    Requested length of %d samples.\n    Max length of %d samples.\n', ...
                                    cmdStr, nodeInd, len, ret(2));
                                error(msg);
                            end
                        end
                        
                        % Update internal object values
                        obj.tx_iq_warning_needed = false;       % Since we have explicitly set the tx IQ length, we do not need a warning
                    end

                %---------------------------------------------------------
                case 'rx_length'
                    % Receive length- reads or sets the duration of each receive cycle, in sample periods 
                    %
                    % Requires BUFF_SEL: No
                    % Arguments: (uint32 RX_LEN)
                    % Returns: none
                    %
                    % NOTE:  This will error if the user tries to read tx_length from the board.
                    %   the 'tx_buff_max_num_samples' command should be used to determine the 
                    %   capabilities of the board.
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_RX_LENGTH));
                    
                    if(isempty(varargin))                  % Read Mode
                        error('%s: ''rx_length'' does not support read.  Use ''rx_buff_max_num_samples''.', cmdStr);
                    else                                   % Write Mode
                        myCmd.addArgs(myCmd.CMD_PARAM_WRITE_VAL);
                        
                        len = varargin{1};

                        % Check arguments
                        if(length(len) > 1)
                           error('%s:  Requires scalar argument.  Length is per-node, not per-interface or per-buffer.', cmdStr);
                        end

                        if(len  < 0)
                            error('%s: Rx length must be greater than or equal to zero.\n', cmdStr);
                        end
                        
                        % Send command to the node
                        myCmd.addArgs(len);
                        resp = node.sendCmd(myCmd);
                        
                        % Process response from the node.  Return arguments:
                        %     [1] - Status
                        %     [2] - Rx Length
                        %
                        for i = 1:numel(resp)                   % Needed for unicast node_group support
                            ret   = resp(i).getArgs();

                            if (len > ret(2))
                                msg = sprintf('%s: Rx length error in node %d.\n    Requested length of %d samples.\n    Max length of %d samples.\n', ...
                                    cmdStr, nodeInd, len, ret(2));
                                error(msg);
                            end
                        end
                        
                        % Update internal object values
                        obj.rx_iq_warning_needed = false;       % Since we have explicitly set the rx IQ length, we do not need a warning
                    end

                %---------------------------------------------------------
                case 'tx_buff_max_num_samples'
                    % Maximum number of TX samples
                    %
                    % Requires BUFF_SEL: Yes (Vector notation [RFA, RFB])
                    % Arguments: none
                    % Returns: (uint32 MAX_TX_LEN)
                    %
                    out = zeros(1, num_buffers);
                    
                    % Iterate over the provided interfaces
                    for n = 1:num_buffers
                    
                        % Check buffer selection
                        if(isSingleBuffer(buffSel(n)) == 0)
                            error('%s: buffer selection must be singular. Use vector notation for reading from multiple buffers e.g. [RFA, RFB]', cmdStr);
                        end
                    
                        myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_MAX_NUM_SAMPLES));
                        
                        if(isempty(varargin))                  % Read Mode
                            myCmd.addArgs(myCmd.CMD_PARAM_READ_VAL);
                            myCmd.addArgs(buffSel(n));
                        
                            resp = node.sendCmd(myCmd);
                            
                            % Process response from the node.  Return arguments:
                            %     [1] - Status
                            %     [2] - Max Tx Length
                            %     [3] - Max Rx Length
                            %
                            ret    = resp.getArgs();
                            out(n) = double(ret(2));
                            
                        else                                   % Write Mode
                            error('%s: ''tx_buff_max_num_samples'' does not support write.  Use ''tx_length''.', cmdStr);
                        end
                    end
                    
                %---------------------------------------------------------
                case 'rx_buff_max_num_samples'
                    % Maximum number of RX samples
                    %
                    % Requires BUFF_SEL: Yes (Vector notation [RFA, RFB])
                    % Arguments: none
                    % Returns: (uint32 MAX_RX_LEN)
                    %
                    out = zeros(1, num_buffers);
                    
                    % Iterate over the provided interfaces
                    for n = 1:num_buffers
                    
                        % Check buffer selection
                        if(isSingleBuffer(buffSel(n)) == 0)
                            error('%s: buffer selection must be singular. Use vector notation for reading from multiple buffers e.g. [RFA, RFB]', cmdStr);
                        end
                    
                        myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_MAX_NUM_SAMPLES));
                        
                        if(isempty(varargin))                  % Read Mode
                            myCmd.addArgs(myCmd.CMD_PARAM_READ_VAL);
                            myCmd.addArgs(buffSel(n));
                        
                            resp = node.sendCmd(myCmd);
                            
                            % Process response from the node.  Return arguments:
                            %     [1] - Status
                            %     [2] - Max Tx Length
                            %     [3] - Max Rx Length
                            %
                            ret    = resp.getArgs();
                            out(n) = double(ret(3));
                            
                        else                                   % Write Mode
                            error('%s: ''rx_buff_max_num_samples'' does not support write.  Use ''rx_length''.', cmdStr);
                        end
                    end
                    
                %---------------------------------------------------------
                case 'continuous_tx'
                    % Enable/disable continuous transmit mode
                    %
                    % Requires BUFF_SEL: No
                    % Arguments: (boolean CONT_TX)
                    %     CONT_TX:
                    %         true enables continuous transmit mode
                    %         false disable continuous transmit mode
                    % Returns: none
                    %
                    % Restrictions on continuous transmit waveform length:
                    %     WARPLab 7.6.0:
                    %         0 to 2^15   --> Waveform will be transmitted for the exact number of samples
                    %         > 2^15      --> Waveform must be a multiple of 2^14 samples for the waveform
                    %                         to be transmitted exactly.  Otherwise, waveform will be appended
                    %                         with whatever IQ data is in the transmit buffer to align the 
                    %                         waveform to be a multiple of 2^14 samples.
                    %
                    %     WARPLab 7.5.x:
                    %         Example not supported
                    %
                    %     WARPLab 7.4.0 and prior:
                    %         0 to 2^15   --> Waveform will be transmitted for the exact number of samples
                    %         > 2^15      --> Not supported
                    %
                    if(length(varargin) ~= 1)
                        error('%s: requires one boolean argument',cmdStr);
                    end
                    
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_TX_MODE), uint32(boolean(varargin{1})));
                    node.sendCmd(myCmd);

                %---------------------------------------------------------
                case 'tx_buff_en'
                    % Enable transmit buffer for one or more interfaces. When a buffer is enabled it will
                    %     drive samples into its associated interface when a trigger is received. The interface
                    %     itself must also be enabled (wl_interfaceCmd(...,'tx_en')) to actually transmit the samples
                    %
                    % Requires BUFF_SEL: Yes (Scalar notation [RFA + RFB])
                    % Arguments: none
                    % Returns: none
                    %

                    % Check buffer selection 
                    if(num_buffers ~= 1)
                        error('%s: Length of buffer selection vector must be 1', cmdStr);
                    end
                    
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_TX_BUFF_EN), buffSel);
                    node.sendCmd(myCmd);
                    
                    % Print warning if TX length was not set before the TX buffer was enabled                    
                    try
                        temp = evalin('base', 'wl_tx_iq_length_did_warn');
                    catch
                        if ( obj.tx_iq_warning_needed ) 
                            fprintf('WARNING:   Currently in WARPLab, the default value transmit length is %d.\n', obj.txIQLen);
                            fprintf('WARNING:   In the future, this may change.  Therefore, you need to explicitly\n');
                            fprintf('WARNING:   set the transmit IQ length:\n');
                            fprintf('WARNING:       wl_basebandCmd(nodes, ''tx_length'', my_tx_length);\n');
                        end
                        assignin('base', 'wl_tx_iq_length_did_warn', 1)
                    end

                %---------------------------------------------------------
                case 'rx_buff_en'
                    % Enable receive buffer for one or more interfaces. When a buffer is enabled it will
                    %     capture samples from its associated interface when a trigger is received. The interface
                    %     itself must also be enabled (wl_interfaceCmd(...,'rx_en'))
                    %
                    % Requires BUFF_SEL: Yes (Scalar notation [RFA + RFB])
                    % Arguments: none
                    % Returns: none
                    %
                    
                    % Check buffer selection 
                    if(num_buffers ~= 1)
                        error('%s: Length of buffer selection vector must be 1', cmdStr);
                    end
                    
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_RX_BUFF_EN), buffSel);
                    node.sendCmd(myCmd);

                    % Print warning if RX length was not set before the RX buffer was enabled                    
                    try
                        temp = evalin('base', 'wl_rx_iq_length_did_warn');
                    catch
                        if ( obj.rx_iq_warning_needed ) 
                            fprintf('WARNING:   Currently in WARPLab, the default value receive length is %d.\n', obj.rxIQLen);
                            fprintf('WARNING:   In the future, this may change.  Therefore, you need to explicitly\n');
                            fprintf('WARNING:   set the receive IQ length:\n');
                            fprintf('WARNING:       wl_basebandCmd(nodes, ''rx_length'', my_rx_length);\n');
                        end
                        assignin('base', 'wl_rx_iq_length_did_warn', 1)
                    end

                %---------------------------------------------------------
                case 'tx_rx_buff_dis'
                    % Disable the Tx and Rx buffers for one or more interfaces. When a buffer is disabled it will not
                    %     output/capture samples when a trigger is received, even if the associated interface is enabled
                    %
                    % Requires BUFF_SEL: Yes (Scalar notation [RFA + RFB])
                    % Arguments: none
                    % Returns: none
                    %
                    
                    % Check buffer selection 
                    if(num_buffers ~= 1)
                        error('%s: Length of buffer selection vector must be 1', cmdStr);
                    end
                    
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_TX_RX_BUFF_DIS), buffSel);
                    node.sendCmd(myCmd);

                %---------------------------------------------------------
                case 'read_buff_state'
                    % Read the current state of the buffer
                    %
                    % Requires BUFF_SEL: Yes (Vector notation [RFA, RFB])
                    % Arguments: none
                    % Returns: Current state of the buffer:  TX, RX or STANDBY
                    %
                    
                    for buff_index = 1:num_buffers
                    
                        if(isSingleBuffer(buffSel(buff_index)) == 0)
                            error('%s: Buffer selection must be singular. Use vector notation for reading from multiple buffers e.g. [RFA, RFB]', cmdStr);
                        end

                        myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_TX_RX_BUFF_STATE));
                        myCmd.addArgs(buffSel(buff_index));
                        
                        resp  = node.sendCmd(myCmd);
                        ret   = resp.getArgs();
                        
                        switch (ret(1))
                            case 0
                                out(buff_index) = obj.STANDBY;
                            case 1
                                out(buff_index) = obj.RX;
                            case 2
                                out(buff_index) = obj.TX;
                            otherwise
                                error('%s: Node returned an unknown buffer state.', cmdStr);
                        end
                    end
                
                %---------------------------------------------------------
                case 'tx_buff_clk_freq'
                    % Read the transmit sample clock frequency out of the buffer core.
                    %
                    % Requires BUFF_SEL: No
                    % Arguments: none
                    % Returns: (uint32 Fs_Tx)
                    %     Fs_Tx: Tx sample frequency of buffer core in Hz
                    %
                    out = 40e6;        % Currently, this value is hardcoded. It will eventually be read from the board.

                %---------------------------------------------------------
                case 'rx_buff_clk_freq'
                    % Read the receive sample clock frequency out of the buffer core.
                    %
                    % Requires BUFF_SEL: No
                    % Arguments: none
                    % Returns: (uint32 Fs_Rx)
                    %     Fs_Rx: Rx sample frequency of buffer core in Hz
                    %
                    out = 40e6;        % Currently, this value is hardcoded. It will eventually be read from the board.

                %---------------------------------------------------------
                case 'rx_rssi_clk_freq'
                    % Read the receive RSSI sample clock frequency out of the buffer core.
                    %
                    % Requires BUFF_SEL: No
                    % Arguments: none
                    % Returns: (uint32 Fs_RxRSSI)
                    %     Fs_RxRSSI: Rx RSSI sample frequency of buffer core in Hz
                    %
                    out = 10e6;        % Currently, this value is hardcoded. It will eventually be read from the board.                          

                %---------------------------------------------------------
                case 'write_iq'
                    % Write I/Q samples to the specified buffers. The dimensions of the buffer selection and samples matrix
                    %     must agree. The same samples can be written to multiple buffers by combining buffer IDs
                    %
                    % Requires BUFF_SEL: Yes (combined BUFF_SEL values ok)
                    % Arguments: (complex double TX_SAMPS, int OFFSET)
                    %     TX_SAMPS: matrix of complex samples. The number of columns must match the length of BUFF_SEL
                    %     OFFSET: buffer index of first sample to write (optional; defaults to 0)
                    %
                    % Examples:
                    %     TxLength = 2^14;
                    %     Ts = 1/(wl_basebandCmd(node0,'tx_buff_clk_freq'));
                    %     t  = [0:Ts:(TxLength-1)*Ts].';                       % column vector
                    %     X  = exp(t*1i*2*pi*3e6);                             % 3MHz sinusoid
                    %     Y  = exp(t*1i*2*pi*5e6);                             % 5MHz sinusoid
                    %
                    %     % Write X to RFA
                    %     wl_basebandCmd(node, RFA, 'write_IQ', X);
                    %
                    %     % Write X to RFA and RFB
                    %     wl_basebandCmd(node, (RFA + RFB), 'write_IQ', X);
                    %
                    %     % Write X to RFA, Y to RFB
                    %     wl_basebandCmd(node, [RFA RFB], 'write_IQ', [X Y]);
                    %
                    writeIQ(obj, node, transport, buffSel, cmdStr, varargin{:});

                %---------------------------------------------------------
                case 'write_iq_checksum'
                    % Write IQ checksum - gets the current Write IQ checksum from the node. 
                    %
                    % Requires BUFF_SEL: No
                    % Arguments: none
                    % Returns: (uint32 WRITE_IQ_CHECKSUM)
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_WRITE_IQ_CHKSUM));
                    resp  = node.sendCmd(myCmd);
                    ret   = resp.getArgs();
                    out   = ret(1);

                %---------------------------------------------------------
                case 'read_iq'
                    % Read I/Q samples from the specified buffers. The elements of the buffer selection must be scalers which
                    %     identify a single buffer. To read multiple buffers in one call, pass a vector of individual buffer IDs
                    %
                    % Requires BUFF_SEL: Yes (combined BUFF_SEL values not allowed)
                    % Arguments: (int OFFSET, int NUM_SAMPS)
                    %     OFFSET: buffer index of first sample to read (optional; defaults to 0)
                    %     NUM_SAMPS: number of complex samples to read (optional; defaults to length(OFFSET:rxIQLen-1))
                    %
                    % Examples:
                    %     % Read full buffer for RFA
                    %     %     NOTE:  size(X) will be [rxIQLen, 1]
                    %     X = wl_basebandCmd(node, RFA, 'read_IQ');
                    %
                    %     % Read partial buffer for RFA (samples 1000:4999)
                    %     %     NOTE:  size(X) will be [5000, 1]
                    %     X = wl_basebandCmd(node, RFA, 'read_IQ', 1000, 5000);
                    %
                    %     % Read full buffers for RFA and RFB
                    %     %     NOTE:  size(X) will be [rxIQLen, 2]
                    %     X = wl_basebandCmd(node, [RFA RFB], 'read_IQ');
                    %
                    out = readIQ(obj, node, buffSel, cmdStr, varargin{:});

                %---------------------------------------------------------
                case 'read_rssi'
                    % Read RSSI samples from the specified buffers. The elements of the buffer selection must be scalers which
                    %     identify a single buffer. To read multiple buffers in one call, pass a vector of individual buffer IDs.
                    %
                    % See 'read_iq' for arguments/returns
                    %
                    out = readRSSI(obj, node, buffSel, cmdStr, varargin{:});

                %---------------------------------------------------------
                case 'get_tx_count'
                    % For the given buffers, get the number of times the TX state machine has run
                    %
                    % Requires BUFF_SEL: Yes (Vector notation [RFA, RFB])
                    % Arguments: none
                    % Returns: [uint32 BUFFER_COUNTER]
                    %
                    out = zeros(1, num_buffers);
                    
                    % Iterate over the provided interfaces
                    for n = 1:num_buffers
                    
                        % Check buffer selection
                        if(isSingleBuffer(buffSel(n)) == 0)
                            error('%s: buffer selection must be singular. Use vector notation for reading from multiple buffers e.g. [RFA, RFB]', cmdStr);
                        end
                    
                        myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_TXRX_COUNT_GET));
                        
                        if(isempty(varargin))                  % Read Mode
                            myCmd.addArgs(myCmd.CMD_PARAM_READ_VAL);
                            myCmd.addArgs(buffSel(n));
                            myCmd.addArgs(0);                  % Select TX counter 
                        
                            resp = node.sendCmd(myCmd);
                            
                            % Process response from the node.  Return arguments:
                            %     [1] - Status
                            %     [2] - Counter value
                            %
                            ret    = resp.getArgs();
                            
                            if (ret(1) == myCmd.CMD_PARAM_SUCCESS)
                                out(n) = double(ret(2));
                            else 
                                msg = sprintf('%s: Get TX count error in node %d.\n', cmdStr, nodeInd);
                                error(msg);
                            end
                        else                                   % Write Mode
                            error('%s: ''get_tx_count'' does not support write.', cmdStr);
                        end
                    end
                    
                %---------------------------------------------------------
                case 'get_rx_count'
                    % For the given buffers, get the number of times the RX state machine has run 
                    %
                    % Requires BUFF_SEL: Yes (Vector notation [RFA, RFB])
                    % Arguments: none
                    % Returns: [uint32 BUFFER_COUNTER]
                    %
                    out = zeros(1, num_buffers);
                    
                    % Iterate over the provided interfaces
                    for n = 1:num_buffers
                    
                        % Check buffer selection
                        if(isSingleBuffer(buffSel(n)) == 0)
                            error('%s: buffer selection must be singular. Use vector notation for reading from multiple buffers e.g. [RFA, RFB]', cmdStr);
                        end
                    
                        myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_TXRX_COUNT_GET));
                        
                        if(isempty(varargin))                  % Read Mode
                            myCmd.addArgs(myCmd.CMD_PARAM_READ_VAL);
                            myCmd.addArgs(buffSel(n));
                            myCmd.addArgs(1);                  % Select RX counter 
                        
                            resp = node.sendCmd(myCmd);
                            
                            % Process response from the node.  Return arguments:
                            %     [1] - Status
                            %     [2] - Counter value
                            %
                            ret    = resp.getArgs();
                            
                            if (ret(1) == myCmd.CMD_PARAM_SUCCESS)
                                out(n) = double(ret(2));
                            else 
                                msg = sprintf('%s: Get RX count error in node %d.\n', cmdStr, nodeInd);
                                error(msg);
                            end
                        else                                   % Write Mode
                            error('%s: ''get_rx_count'' does not support write.', cmdStr);
                        end
                    end
                    
                %---------------------------------------------------------
                case 'reset_tx_count'
                    % For the given buffers, reset the counter that records the number of times the TX state machine has run 
                    %
                    % Requires BUFF_SEL: Yes (Vector notation [RFA, RFB])
                    % Arguments: none
                    % Returns: [uint32 BUFFER_COUNTER]
                    %
                    
                    % Iterate over the provided interfaces
                    for n = 1:num_buffers
                    
                        % Check buffer selection
                        if(isSingleBuffer(buffSel(n)) == 0)
                            error('%s: buffer selection must be singular. Use vector notation for reading from multiple buffers e.g. [RFA, RFB]', cmdStr);
                        end
                    
                        myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_TXRX_COUNT_RESET));
                        
                        if(isempty(varargin))
                            myCmd.addArgs(myCmd.CMD_PARAM_WRITE_VAL);
                            myCmd.addArgs(buffSel(n));
                            myCmd.addArgs(0);                  % Select TX counter 
                        
                            resp = node.sendCmd(myCmd);
                            
                            % Process response from the node.  Return arguments:
                            %     [1] - Status
                            %
                            ret    = resp.getArgs();
                            
                            if (ret(1) ~= myCmd.CMD_PARAM_SUCCESS)
                                msg = sprintf('%s: Get TX count reset error in node %d.\n', cmdStr, nodeInd);
                                error(msg);
                            end
                        else
                            error('%s: ''reset_tx_count'' does not support additional arguments.', cmdStr);
                        end
                    end
                    
                %---------------------------------------------------------
                case 'reset_rx_count'
                    % For the given buffers, reset the counter that records the number of times the RX state machine has run 
                    %
                    % Requires BUFF_SEL: Yes (Vector notation [RFA, RFB])
                    % Arguments: none
                    % Returns: [uint32 BUFFER_COUNTER]
                    %
                    
                    % Iterate over the provided interfaces
                    for n = 1:num_buffers
                    
                        % Check buffer selection
                        if(isSingleBuffer(buffSel(n)) == 0)
                            error('%s: buffer selection must be singular. Use vector notation for reading from multiple buffers e.g. [RFA, RFB]', cmdStr);
                        end
                    
                        myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_TXRX_COUNT_RESET));
                        
                        if(isempty(varargin))
                            myCmd.addArgs(myCmd.CMD_PARAM_WRITE_VAL);
                            myCmd.addArgs(buffSel(n));
                            myCmd.addArgs(1);                  % Select RX counter 
                        
                            resp = node.sendCmd(myCmd);
                            
                            % Process response from the node.  Return arguments:
                            %     [1] - Status
                            %
                            ret    = resp.getArgs();
                            
                            if (ret(1) ~= myCmd.CMD_PARAM_SUCCESS)
                                msg = sprintf('%s: Get RX count reset error in node %d.\n', cmdStr, nodeInd);
                                error(msg);
                            end
                        else
                            error('%s: ''reset_rx_count'' does not support additional arguments.', cmdStr);
                        end
                    end
                    
                %---------------------------------------------------------
                case 'agc_state'
                    % Read AGC state from the specified buffers. The elements of the buffer selection must be scalers which
                    %     identify a single buffer. To read multiple buffers in one call, pass a vector of individual buffer IDs
                    %
                    % Requires BUFF_SEL: Yes  (Vector notation [RFA, RFB])
                    % Arguments: none
                    %
                    % Returns: agc_state -- column vector per buffer BUFF_SEL
                    %   agc_state(1): RF gain chosen by AGC
                    %   agc_state(2): BB gain chosen by AGC
                    %   agc_state(3): RSSI observed by AGC at time of lock
                    %
                    for ifcIndex = length(buffSel):-1:1
                        currBuffSel = buffSel(ifcIndex);
                        
                        if(isSingleBuffer(currBuffSel) == 0)
                            error('%s: buffer selection must be singular. Use vector notation for reading from multiple buffers e.g. [RFA,RFB]',cmdStr);
                        end

                        myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_AGC_STATE));
                        myCmd.addArgs(bbSel_to_rfSel(currBuffSel));
                        
                        resp  = node.sendCmd(myCmd);
                        ret   = resp.getArgs();
                        k     = 1;
                        gains = uint16(bitand(ret(2*(k - 1) + 1), hex2dec('000000FF')));
                        
                        out(1, ifcIndex) = uint16(bitand(gains, hex2dec('03')));
                        out(2, ifcIndex) = uint16(bitshift(gains, -2));
                        out(3, ifcIndex) = uint16(ret(2*(k - 1) + 2));
                    end
                    
                %---------------------------------------------------------
                case 'agc_target'
                    % Set the AGC target
                    %
                    % Requires BUFF_SEL: No. Values apply to all RF paths
                    % Arguments: (int32 target) (Integer value in [-32, 31])
                    %     target: target receive power (in dBm)
                    %             default value: -13
                    % Returns: none
                    %
                    % This command is the best way to tweak AGC behavior
                    % to apply more or less gain. For example, a target of
                    % -5dBm will apply more gain than a target of -10dBm,
                    % so the waveform will be larger at the inputs of the I
                    % and Q ADCs.
                    %
                    myCmd  = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_AGC_TARGET));
                    
                    % Check arguments
                    if(length(varargin) ~= 1)
                       error('%s:  Requires one argument:  agc target', cmdStr);
                    end

                    target = varargin{1};
                    
                    % Check arguments
                    if(length(target) > 1)
                       error('%s:  Requires scalar argument.  AGC target is per-node, not per-interface or per-buffer.', cmdStr);
                    end
                                        
                    if ((target < -32) && (target > 31))
                        error('%s: AGC target must be in [-32, 31].\n', cmdStr);
                    end

                    % Convert to UFix_6_0
                    %   NOTE: this method of converting to two's compliment works because of the bounds checking above
                    %
                    target = uint32(mod(target, 2^6));
                    
                    myCmd.addArgs(target);
                    
                    node.sendCmd(myCmd);

                %---------------------------------------------------------
                case 'agc_dco'
                    % Enable/disable DC offset correction
                    %
                    % Requires BUFF_SEL: No
                    % Arguments: (boolean DCO)
                    %     DCO:
                    %         true enables DC offset correction
                    %         false disable DC offset correction
                    % Returns: none
                    %

                    % Check arguments                    
                    if(length(varargin) ~= 1)
                        error('%s: Requires one boolean argument',cmdStr);
                    end
                    
                    switch(varargin{1})
                        case true
                            myCmd = wl_cmd(node.calcCmd(obj.GRP,obj.CMD_AGC_DCO_EN_DIS), 1);
                        case false
                            myCmd = wl_cmd(node.calcCmd(obj.GRP,obj.CMD_AGC_DCO_EN_DIS), 0);
                    end
                    
                    node.sendCmd(myCmd);
                
                %---------------------------------------------------------
                case 'agc_done_addr'
                    % Sample index where AGC finished
                    %
                    % Value applies to all RF paths
                    %
                    % Requires BUFF_SEL: No. 
                    % Arguments: 
                    % Returns: (uint32) sample_index
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP,obj.CMD_AGC_DONE_ADDR));
                    resp  = node.sendCmd(myCmd);
                    ret   = resp.getArgs();
                    out   = ret(1);
                    
                %---------------------------------------------------------
                case 'agc_reset'
                    % Resets the AGC to its default state
                    %
                    % Requires BUFF_SEL: No. Values apply to all RF paths
                    % Arguments: none
                    % Returns: none
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP,obj.CMD_AGC_RESET));
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'agc_reset_per_rx'
                    % Get / Set whether the AGC will reset on per RX or hold gains across RX 
                    %
                    % Arguments: 'true' or 'false'; none on read
                    % Returns:   none on write; 'true' or 'false'
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP,obj.CMD_AGC_RESET_MODE));
                    
                    if(isempty(varargin))                  % Read Mode
                        myCmd.addArgs(myCmd.CMD_PARAM_READ_VAL);
                        
                    else                                   % Write Mode
                        % Check arguments
                        if(length(varargin) ~= 1)
                            error('%s: Requires one argument:  true/false', cmdStr);
                        end
                    
                        myCmd.addArgs(myCmd.CMD_PARAM_WRITE_VAL);

                        if (boolean(varargin{1}))
                           reset_per_rx = uint32(hex2dec('00000001'));;
                        else
                           reset_per_rx = uint32(hex2dec('00000000'));
                        end
                        
                        myCmd.addArgs(reset_per_rx);
                    end
                    
                    resp = node.sendCmd(myCmd);

                    % Process response from the node.  Return arguments:
                    %     [1] - Status
                    %     [2] - Value
                    %
                    for i = 1:numel(resp)                   % Needed for unicast node_group support
                        ret   = resp(i).getArgs();

                        if (ret(1) == myCmd.CMD_PARAM_SUCCESS)
                            if (ret(2) == hex2dec('00000001')) 
                                out = true;
                            else
                                out = false;
                            end
                        else 
                            msg = sprintf('%s: AGC reset per rx error in node %d.\n', cmdStr, nodeInd);
                            error(msg);                            
                        end
                    end

                %---------------------------------------------------------
                case 'agc_config'
                    % Set the configuration of the AGC
                    % 
                    % This function will set the following AGC configuration fields:
                    %   - RSSI averaging length
                    %   - Voltage DB Adjust
                    %   - Initial BB Gain
                    %
                    % Requires BUFF_SEL: No
                    % Arguments: RSSI Averaging length (Integer value in [0, 3])
                    %            Voltage DB Adjust (Integer value in [0, 63])
                    %            Initial BB Gain (RX) (Integer value in [0, 31])
                    % Returns  : None
                    % 
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_AGC_CONFIG));
                    
                    if(isempty(varargin))                  % Read Mode
                        error('%s: Read mode not supported', cmdStr);
                        
                    else                                   % Write Mode
                        % Check arguments
                        if(length(varargin) ~= 3)
                            error('%s: Requires three arguments:  RSSI_Avg, VDB_Adj, Init_BB_Gain', cmdStr);
                        end
                    
                        myCmd.addArgs(myCmd.CMD_PARAM_WRITE_VAL);

                        rssi_avg_length = varargin{1};
                        v_db_adjust     = varargin{2};
                        init_bb_gain    = varargin{3};
                        
                        % Check arguments
                        if ((rssi_avg_length < 0) || (rssi_avg_length > 3))
                            error('%s: RSSI Averaging length must be in [0, 3].\n', cmdStr);
                        end

                        if ((v_db_adjust < 0) || (v_db_adjust > 63))
                            error('%s: Voltage DB Adjust must be in [0, 63].\n', cmdStr);
                        end
                        
                        if ((init_bb_gain < 0) || (init_bb_gain > 31))
                            error('%s: Initial BB gain must be in [0, 31].\n', cmdStr);
                        end
                        
                        % Send command to the node
                        myCmd.addArgs(rssi_avg_length);
                        myCmd.addArgs(v_db_adjust);
                        myCmd.addArgs(init_bb_gain);
                        
                        resp = node.sendCmd(myCmd);
                        
                        % Process response from the node.  Return arguments:
                        %     [1] - Status
                        %
                        for i = 1:numel(resp)                   % Needed for unicast node_group support
                            ret   = resp(i).getArgs();

                            if (ret(1) == myCmd.CMD_PARAM_ERROR)
                                msg = sprintf('%s: AGC config error in node %d.\n', cmdStr, nodeInd);
                                error(msg);
                            end
                        end
                    end
                    
                %---------------------------------------------------------
                case 'agc_iir_hpf'
                    % Set the Infinite Impulse Response (IIR) High Pass Filter (HPF) coefficients
                    % 
                    % This function will set the following IIR HPF coefficients:
                    %   - A1
                    %   - B0
                    %
                    % NOTE:  By default the reference design uses a filter with a 3 dB cutoff at 
                    %     20 kHz with 40 MHz sampling.  This results in coefficients:
                    %         A1 = -0.996863331833438
                    %         B0 = 0.99843166591671906
                    %
                    % Requires BUFF_SEL: No
                    % Arguments: A1 coefficient (Value in [-1, 1]; range represented by Fix_18_17)
                    %            B0 coefficient (Value in [0, 2]; range represented by UFix_18_17)
                    % Returns  : None
                    % 
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_AGC_IIR_HPF));
                    
                    if(isempty(varargin))                  % Read Mode
                        error('%s: Read mode not supported', cmdStr);
                        
                    else                                   % Write Mode
                        myCmd.addArgs(myCmd.CMD_PARAM_WRITE_VAL);
                        
                        % Check arguments
                        if(length(varargin) ~= 2)
                            error('%s: Requires two arguments:  A1 coefficient, B0 coefficient', cmdStr);
                        end
                    
                        myCmd.addArgs(myCmd.CMD_PARAM_WRITE_VAL);

                        a1_coeff = varargin{1};
                        b0_coeff = varargin{2};
                        
                        % Check arguments
                        if ((a1_coeff < -1) || (a1_coeff > 1))
                            error('%s: A1 coefficient must be in [-1, 1].\n', cmdStr);
                        end

                        if ((b0_coeff < 0) || (b0_coeff > 2))
                            error('%s: B0 coefficient must be in [0, 2].\n', cmdStr);
                        end

                        % Convert A1 to Fix_18_17 
                        %   NOTE: this method of converting to two's compliment works because of the bounds checking above
                        %
                        a1_coeff = bitand(uint32(mod((a1_coeff * 2^17), 2^18)), hex2dec('0003FFFF'));
                        
                        % Convert B0 to UFix_18_17
                        b0_coeff = bitand(uint32(b0_coeff * 2^17), hex2dec('0003FFFF'));

                        % Send command to the node
                        myCmd.addArgs(a1_coeff);
                        myCmd.addArgs(a1_coeff);
                        
                        resp = node.sendCmd(myCmd);
                        
                        % Process response from the node.  Return arguments:
                        %     [1] - Status
                        %
                        for i = 1:numel(resp)                   % Needed for unicast node_group support
                            ret   = resp(i).getArgs();

                            if (ret(1) == myCmd.CMD_PARAM_ERROR)
                                msg = sprintf('%s: IIR HPF error in node %d.\n', cmdStr, nodeInd);
                                error(msg);
                            end
                        end
                    end
                    
                %---------------------------------------------------------
                case 'agc_rf_gain_threshold'
                    % Set the RF gain thresholds
                    % 
                    % This function will set the following fields:
                    %   - 3 -> 2 RF gain threshold
                    %   - 2 -> 1 RF gain threshold
                    %
                    % After the AGC has converted RSSI to power (dBm), this will select the 
                    % the thresholds used to set the RF (LNA) gain in the MAX2829.  
                    %
                    % Requires BUFF_SEL: No
                    % Arguments: 3 -> 2 RF gain threshold (Integer value in [-128, 127])
                    %            2 -> 1 RF gain threshold (Integer value in [-128, 127])
                    % Returns  : None
                    % 
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_RF_GAIN_THRESHOLD));
                    
                    if(isempty(varargin))                  % Read Mode
                        error('%s: Requires argument(s)', cmdStr);
                        
                    else                                   % Write Mode
                        myCmd.addArgs(myCmd.CMD_PARAM_WRITE_VAL);
                        
                        % Check arguments
                        if(length(varargin) ~= 2)
                            error('%s: Requires two arguments:  3 -> 2 threshold, 2 -> 1 threshold', cmdStr);
                        end                  

                        threshold_3_2 = varargin{1};
                        threshold_2_1 = varargin{2};
                        
                        % Check arguments
                        if ((threshold_3_2 < -128) || (threshold_3_2 > 127))
                            error('%s: 3 -> 2 threshold must be in [-128, 127].\n', cmdStr);
                        end

                        if ((threshold_2_1 < -128) || (threshold_2_1 > 127))
                            error('%s: 3 -> 2 threshold must be in [-128, 127].\n', cmdStr);
                        end

                        % Convert to UFix_8_0 
                        %   NOTE: this method of converting to two's compliment works because of the bounds checking above
                        %
                        threshold_3_2 = uint32(mod(threshold_3_2, 2^8));
                        threshold_2_1 = uint32(mod(threshold_2_1, 2^8));
                        
                        % Send command to the node
                        myCmd.addArgs(threshold_3_2);
                        myCmd.addArgs(threshold_2_1);
                        
                        resp = node.sendCmd(myCmd);
                        
                        % Process response from the node.  Return arguments:
                        %     [1] - Status
                        %
                        for i = 1:numel(resp)                   % Needed for unicast node_group support
                            ret   = resp(i).getArgs();

                            if (ret(1) == myCmd.CMD_PARAM_ERROR)
                                msg = sprintf('%s: IIR HPF error in node %d.\n', cmdStr, nodeInd);
                                error(msg);
                            end
                        end
                    end
                    
                %---------------------------------------------------------
                case 'agc_timing'
                    % Set the AGC timing
                    % 
                    % This function will set the following fields:
                    %   - Sample to take first RSSI capture
                    %   - Sample to take second RSSI capture
                    %   - Sample to take the Voltage DB capture
                    %   - Sample to complete the AGC
                    %
                    % Requires BUFF_SEL: No
                    % Arguments: Capture RSSI 1 (Integer value in [0, 255])
                    %            Capture RSSI 2 (Integer value in [0, 255])
                    %            Capture Voltage DB (Integer value in [0, 255])
                    %            AGC Done (Integer value in [0, 255])
                    % Returns  : None
                    % 
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_AGC_TIMING));
                    
                    if(isempty(varargin))                  % Read Mode
                        error('%s: Read mode not supported', cmdStr);
                        
                    else                                   % Write Mode
                        myCmd.addArgs(myCmd.CMD_PARAM_WRITE_VAL);
                        
                        % Check arguments
                        if(length(varargin) ~= 4)
                            error('%s: Requires four arguments:  capture rssi 1, capture rssi 2, capture voltage db, agc done', cmdStr);
                        end                   

                        capture_rssi_1 = varargin{1};
                        capture_rssi_2 = varargin{2};
                        capture_v_db   = varargin{3};
                        agc_done       = varargin{4};
                        
                        % Check arguments
                        if ((capture_rssi_1 < 0) || (capture_rssi_1 > 255))
                            error('%s: Capture RSSI 1 must be in [0, 255].\n', cmdStr);
                        end

                        if ((capture_rssi_2 < 0) || (capture_rssi_2 > 255))
                            error('%s: Capture RSSI 2 must be in [0, 255].\n', cmdStr);
                        end

                        if ((capture_v_db < 0) || (capture_v_db > 255))
                            error('%s: Capture Voltage DB must be in [0, 255].\n', cmdStr);
                        end

                        if ((agc_done < 0) || (agc_done > 255))
                            error('%s: AGC done must be in [0, 255].\n', cmdStr);
                        end
                        
                        % Send command to the node
                        myCmd.addArgs(capture_rssi_1);
                        myCmd.addArgs(capture_rssi_2);
                        myCmd.addArgs(capture_v_db);
                        myCmd.addArgs(agc_done);
                        
                        resp = node.sendCmd(myCmd);
                        
                        % Process response from the node.  Return arguments:
                        %     [1] - Status
                        %
                        for i = 1:numel(resp)                   % Needed for unicast node_group support
                            ret   = resp(i).getArgs();

                            if (ret(1) == myCmd.CMD_PARAM_ERROR)
                                msg = sprintf('%s: AGC timing error in node %d.\n', cmdStr, nodeInd);
                                error(msg);
                            end
                        end
                    end
                    
                %---------------------------------------------------------
                case 'agc_dco_timing'
                    % Set the AGC DC Offset (DCO) timing
                    % 
                    % This function will set the following fields:
                    %   - Sample to start the DCO
                    %   - Sample to start the IIR HPF
                    %
                    % Requires BUFF_SEL: No
                    % Arguments: Start DCO (Integer value in [0, 255])
                    %            Start IIR HPF (Integer value in [0, 255])
                    % Returns  : None
                    % 
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_AGC_DCO_TIMING));
                    
                    if(isempty(varargin))                  % Read Mode
                        error('%s: Read mode not supported', cmdStr);
                        
                    else                                   % Write Mode
                        myCmd.addArgs(myCmd.CMD_PARAM_WRITE_VAL);
                        
                        % Check arguments
                        if(length(varargin) ~= 2)
                            error('%s: Requires two arguments:  start DCO, start IIR HPF', cmdStr);
                        end                   

                        start_dco     = varargin{1};
                        start_iir_hpf = varargin{2};
                        
                        % Check arguments
                        if ((start_dco < 0) || (start_dco > 255))
                            error('%s: Start DCO must be in [0, 255].\n', cmdStr);
                        end

                        if ((threshold_2_1 < 0) || (threshold_2_1 > 255))
                            error('%s: Start IIR HPF must be in [0, 255].\n', cmdStr);
                        end
                        
                        if (start_dco < start_iir_hpf)
                            error('%s: Start DCO must be before start IIR HPF.\n', cmdStr);
                        end
                        
                        % Send command to the node
                        myCmd.addArgs(start_dco);
                        myCmd.addArgs(start_iir_hpf);
                        
                        resp = node.sendCmd(myCmd);
                        
                        % Process response from the node.  Return arguments:
                        %     [1] - Status
                        %
                        for i = 1:numel(resp)                   % Needed for unicast node_group support
                            ret   = resp(i).getArgs();

                            if (ret(1) == myCmd.CMD_PARAM_ERROR)
                                msg = sprintf('%s: AGC DCO timing error in node %d.\n', cmdStr, nodeInd);
                                error(msg);
                            end
                        end
                    end
                    
                %---------------------------------------------------------
                otherwise
                    error('unknown command ''%s''',cmdStr);
            end
            
            if(iscell(out)==0 && numel(out)~=1)
                out = {out}; 
            end
         end
    end
end


function out = rfSel_to_bbSel(sel)
    out = bitshift(uint32(sel), -28);
end


function out = bbSel_to_rfSel(sel)
    out = bitshift(uint32(sel), 28);
end


function out = isSingleBuffer(sel)
    out = (length(strfind(dec2bin(sel), '1')) == 1);
end


function out = updateChecksum(newdata, varargin)
    persistent sum1;
    persistent sum2;
    
    if(isempty(sum1))
        sum1 = uint32(0);
    end
    if(isempty(sum2))
        sum2 = uint32(0);
    end

    if(length(varargin) == 1)
       if(strcmpi(varargin{1}, 'reset'))
           sum1 = uint32(0);
           sum2 = uint32(0);
       end
    end

    newdata = uint32(newdata);

    sum1 = mod((sum1 + newdata), 65535);
    sum2 = mod(sum2 + sum1, 65535);
    
    out = bitshift(sum2, 16) + sum1;
end



function writeIQ(obj, node, transport, buffSel, cmdStr, varargin)
% writeIQ Helper function for baseband object to write IQ samples to node
% IMPORTANT: user code should never call this function; always use the 
%  'writeIQ' baesband command (which will call this function with the right arguments)
%
% Writing a full buffer of IQ samples requires many host-to-node packets
% This function uses the minimum number of packets possible, given the payload
% limitations of the node's transport object
%
% This write IQ implementation has two modes:
%  fast: node sends ACK only after first and last samples packet
%  slow: node sends ACK after every samples packet
%
% This implementation always attempts fast mode first. If the node fails to
%  receive any packet in fast mode, this funciton reverts to slow mode and
%  re-sends all samples.
% A failure in fast mode is detected using a simple checksum scheme. The node
%  returns its computed checksum over all received samples packets in its ACK
%  of the final packet. If the node's checksum does not match the one computed here,
%  this function reverts to slow mode and tries again. This function raises an
%  error if slow mode fails.

    command       = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_WRITE_IQ));

    samps         = varargin{1};
    num_samples   = size(samps, 1);

    num_interface = length(buffSel);
    
    if( num_interface ~= size(samps,2) )
        if (min(size(samps)) > num_interface)
            error_str = sprintf('%s: Length of buffer selection vector smaller than the number of columns in the sample matrix.', cmdStr);
            error_str = strcat(error_str, '  If trying to write to multiple interfaces, use vector notation vs bitwise addition:  [RFA, RFB] vs RFA + RFB');
        else
            error_str = sprintf('%s: Length of buffer selection vector greater than the number of columns in the sample matrix.', cmdStr);
            error_str = strcat(error_str, '  Make sure that the sample matrix has one IQ vector per specified interface.');
        end
        
        error(error_str);
    end
    
    % Check if user provided a first sample index for any interface
    if(length(varargin)==2)
        offset = varargin{2};
        if(length(offset)==1)
            offset = offset(:,ones(1,num_interface));
        end
    else
        % No offsets specified; write to index 0 for all interfaces
        offset = zeros(1, num_interface);
    end

    % If we have the WARPLab MEX transport, then call the transport function with the raw data
    if ( strcmp( class(transport), 'wl_transport_eth_udp_mex' ) )
    
        if ( num_samples > obj.MEX_TRANSPORT_MAX_IQ )
            msg0 = sprintf('%s: Requested %d samples.  Due to Matlab memory limitations, the mex transport only supports %d samples.', cmdStr, num_samples, obj.MEX_TRANSPORT_MAX_IQ);
            msg1 = sprintf('\n    If your computer has enough physical memory, you can adjust this limit using node.baseband.MEX_TRANSPORT_MAX_IQ.');
            msg2 = sprintf('\n\n');
            msg  = strcat(msg0, msg1, msg2);
            error(msg);
        end

        % write_buffers(obj, func, num_samples, samples, buffer_ids, start_sample, hw_ver, wl_command, check_chksum, input_type)
        %     NOTE:  Currently the only input type supported is 'double' which has a value of 0
        % 
        transport.write_buffers('IQ', num_samples, samps, buffSel, offset, node.hwVer, command, 1, 0);

    elseif( strcmp( class(transport), 'wl_transport_eth_udp_mex_bcast') )
    
        if ( num_samples > obj.MEX_TRANSPORT_MAX_IQ )
            msg0 = sprintf('%s: Requested %d samples.  Due to Matlab memory limitations, the mex transport only supports %d samples.', cmdStr, num_samples, obj.MEX_TRANSPORT_MAX_IQ);
            msg1 = sprintf('\n    If your computer has enough physical memory, you can adjust this limit using node.baseband.MEX_TRANSPORT_MAX_IQ.');
            msg2 = sprintf('\n\n');
            msg  = strcat(msg0, msg1, msg2);
            error(msg);
        end
        
        % write_buffers(obj, func, num_samples, samples, buffer_ids, start_sample, hw_ver, wl_command, check_chksum, input_type)
        %     NOTE:  Currently the only input type supported is 'double' which has a value of 0
        % 
        checksum = transport.write_buffers('IQ', num_samples, samps, buffSel, offset, node.hwVer, command, 0, 0);

        % Call the node to verify the checksum from the WriteIQ
        node.verify_writeIQ_checksum(checksum);
    else
    
        if ( num_samples > obj.JAVA_TRANSPORT_MAX_IQ )
            msg0 = sprintf('%s: Requested %d samples.  Due to performance reasons, the java transport only supports %d samples.', cmdStr, num_samples, obj.JAVA_TRANSPORT_MAX_IQ);
            msg1 = sprintf('\n    Please use the MEX transport for larger requests.');
            msg2 = sprintf('\n\n');
            msg  = strcat(msg0, msg1, msg2);
            error(msg);
        end

        % Determine if we need to check the Write IQ checksum
        if (strcmp(class(transport), 'wl_transport_eth_udp_java')) 
            check_chksum       = 1;
        else
            check_chksum       = 0;
        end
        
        % Set default value to warn when issuing a Write IQ and the node is not ready
        write_iq_ready_warn    = 1;
        
        % Define some constants (*_np variables do not have transport padding)
        TRANSPORT_PADDING_SIZE = 2;                                       % In bytes
        TRANSPORT_MAX_RETRY    = 2;                                       % In packet transmissions
        TRANSPORT_TIMEOUT      = 1;                                       % In seconds
        TRANSPORT_SEND_PKT_LEN = transport.getMaxPayload();               % In bytes

        tport_hdr_size_np      = sizeof( transport.hdr );
        cmd_hdr_size_np        = tport_hdr_size_np + sizeof( wl_cmd );
        all_hdr_size_np        = cmd_hdr_size_np + sizeof( wl_samples );
        
        % Check the bounds on the data
        if(obj.check_write_iq_clipping)
            if(any(any((real(samps).^2) > 1)) || any(any((imag(samps).^2) > 1)))                
                warning('Sample vector contains values outside the range of [-1,+1]');
            end
        end

        % Convert the user-supplied floating point I/Q values to Fix16_15
        samp_I_fi = int16(real(samps)*2^15);
        samp_Q_fi = int16(imag(samps)*2^15);
    
        % Combine the Fix16_15 I/Q values into one 32-bit word
        %     The typecast call preserves the bits of the Fix16_15 I/Q values, so they can be safely 
        %     handled as unsigned 32-bit integers until re-interpreted by the node's C code
        samp_fi = uint32(zeros(size(samps)));
        
        for col = 1:size(samps, 2)
            samp_fi(:, col) = (2^16 .* uint32(typecast(samp_I_fi(:, col), 'uint16'))) + uint32(typecast(samp_Q_fi(:, col), 'uint16'));
        end
    
        % Create a wl_samples object to help serialize chunks of samples for each Ethernet packet
        samples = wl_samples();

        % Compute the maximum number of samples in each Ethernet packet
        %     Starts with transport.maxPayload is the max number of bytes the node's transport can handle per packet (nominally the Ethernet MTU)
        %     Subtracts sizes of the transport header, command header and samples header
        %     Makes sure that it is 4 sample aligned (ie 16 byte aligned) for node DMA transfers
        % 
        max_samples = double(bitand(((floor(double(TRANSPORT_SEND_PKT_LEN)/4) - sizeof(transport.hdr)/4 - sizeof(wl_cmd)/4) - (sizeof(wl_samples)/4)), 4294967292));

        % Calculate the number of transport packets required to write all I/Q samples
        num_pkts = ceil(num_samples / max_samples);
        
        % User species sample offsets zero-indexed; adjust here for MATLAB indexing
        offset = offset + 1;

        % Initialize the checksum
        currChecksum = zeros(1, num_interface);

        % Initialize loop variables for transmission of all samples
        transmitted = 0;
        sendErrors  = 0;
        
        % Loop over the send command and use try/catch to make sure there are no issues in the send buffer        
        while (transmitted == 0)
            try
                for ifcIndex = 1:num_interface

                    % Set values that do not change within each transmission
                    %
                    % Command header
                    %     NOTE:  Since there is one sample packet per command, we set the number number of command arguments to be 1.
                    command.numArgs         = 1;

                    % Sample header
                    samples.buffSel         = buffSel(ifcIndex);
                    samples.sample_iq_id    = obj.sample_write_iq_id;
                    
                    % Increment write IQ ID
                    obj.sample_write_iq_id  = mod((obj.sample_write_iq_id + 1), 256);

                    
                    % Initialize loop variables for a transmission of samples for 1 interface 
                    startSampOffset       = offset(ifcIndex);
                    startSampOffsetMinus1 = offset(ifcIndex) - 1;    % Due to 0 vs 1 indexing, we use this often; so compute it once
                    done                  = 0;
                    pktIndex              = 1;
                    slow_write            = 0;                       % Try a fast write first (ACK only last packet)
                    
                    while(done == 0)
                        
                        % Determine the samples to transmit 
                        if((startSampOffset + max_samples) <= num_samples)
                            stopSampOffset = startSampOffsetMinus1 + max_samples;
                            xfer_samples   = max_samples;
                        else
                            % Last packet may not require max payload size
                            stopSampOffset = (num_samples);
                            xfer_samples   = stopSampOffset - startSampOffset + 1;
                        end
 
                        % Determine the total length of the transmission
                        pkt_length = all_hdr_size_np + (xfer_samples * 4);                     % 4 bytes / sample

                        % Request that the board responds:
                        %     - For slow_write == 1, all packets should be acked
                        %     - For the last packet, if we are checking the checksum, then it needs to be acked
                        if (slow_write == 1)
                            need_resp               = 1;
                            transport.hdr.flags     = bitset(transport.hdr.flags, 1, 1);      % We do need a response for the command
                        elseif ((pktIndex == num_pkts) && (check_chksum == 1))
                            need_resp               = 1;
                            transport.hdr.flags     = bitset(transport.hdr.flags, 1, 1);      % We do need a response for the command
                        else
                            need_resp               = 0;
                            transport.hdr.flags     = bitset(transport.hdr.flags, 1, 0);      % We do not need a response for the command
                        end

                        % Construct the WARPLab transport header that will be used used to write the samples
                        %
                        transport.hdr.msgLength = pkt_length - tport_hdr_size_np;             % Length of the packet without the transport header
                        transport.hdr.seqNum    = (transport.hdr.seqNum + 1) & 255;
                        % transport.hdr.increment;
                                
                        % Construct the WARPLab command that will be used used to write the samples
                        %
                        command.len             = pkt_length - cmd_hdr_size_np;               % Length of the packet without the transport and command headers  

                        % Construct the WARPLab sample structure that will be used to write the samples
                        samples.startSamp       = startSampOffsetMinus1;
                        samples.numSamp         = xfer_samples;

                        if (pktIndex == 1)
                            % First packet
                            if (num_pkts > 1)
                                % This is the first packet of a multi-packet transfer
                                samples.flags   = samples.FLAG_CHKSUM_RESET;
                            else
                                % There is only one packet so mark both flags
                                samples.flags   = samples.FLAG_CHKSUM_RESET + samples.FLAG_LAST_WRITE;
                            end
                        elseif (pktIndex == num_pkts)
                            samples.flags       = samples.FLAG_LAST_WRITE;
                        else
                            samples.flags       = 0;
                        end
                            
                        % Construct the data array to send
                        tport_header            = transport.hdr.serialize;
                        cmd_header              = command.serialize;
                        sample_header           = samples.serialize;
                        data                    = [tport_header, cmd_header, sample_header, samp_fi(startSampOffset:stopSampOffset, ifcIndex).'];
                        data_swap               = swapbytes(uint32(data));
                        data8                   = [zeros(1,2,'uint8') typecast(data_swap, 'uint8')];
                                                
                        % Send the packet
                        node.transport.send_raw(data8, (pkt_length + TRANSPORT_PADDING_SIZE));
                        
                        % Updated checksum
                        %     NOTE:  Due to a weakness in the Fletcher 32 checksum (ie it cannot distinguish between
                        %         blocks of all 0 bits and blocks of all 1 bits), we need to add additional information
                        %         to the checksum so that we will not miss errors on packets that contain data of all 
                        %         zero or all one.  Therefore, we add in the start sample for each packet since that 
                        %         is readily available on the node.
                        %
                        newchkinput_32 = uint32(samp_fi(stopSampOffset, ifcIndex));
                        newchkinput_16 = bitxor(bitshift(newchkinput_32, -16), bitand(newchkinput_32, 65535));
                        
                        % Reset the checksum on the first packet
                        if(pktIndex == 1)
                            currChecksum(ifcIndex) = updateChecksum(bitand(startSampOffsetMinus1, 65535), 'reset');
                        else
                            currChecksum(ifcIndex) = updateChecksum(bitand(startSampOffsetMinus1, 65535));
                        end
                        currChecksum(ifcIndex) = updateChecksum(newchkinput_16);

                        
                        % If we need a response, then wait for it
                        if (need_resp == 1)
                            num_retrys    = 1;
                            rcvd_response = 0;
                            curr_time     = tic;

                            while (rcvd_response == 0)
                            
                                % Have we timed out
                                if ((toc(curr_time) > TRANSPORT_TIMEOUT) && (rcvd_response == 0))
                                    if(num_retrys == TRANSPORT_MAX_RETRY)
                                        error('Error:  Reached maximum number of retrys without a response... aborting.'); 
                                    end
                                    
                                    % Roll everything back and retransmit the packet
                                    num_retrys            = num_retrys + 1;
                                    stopSampOffset        = startSampOffsetMinus1;
                                    pktIndex              = pktIndex - 1;
                                    break;
                                end

                                [recv_len, reply] = transport.receive_raw();
                                
                                % If we have a packet, then process the contents
                                if(recv_len > 0)
                                    reply = reply(((cmd_hdr_size_np / 4) + 1):end);       % Strip off transport and command headers
                                
                                    write_iq_response = process_write_iq_response(obj, reply, samples.sample_iq_id, currChecksum(ifcIndex), write_iq_ready_warn);

                                    % Transmission failed between host and the node
                                    if (write_iq_response == obj.SAMPLE_IQ_CHECKSUM_FAILED)
                                        if (slow_write == 0)
                                            warning('%s: Checksum mismatch on fast write ... reverting to ''slow write''', cmdStr);
                                        else
                                            error('Error:  Checksums do not match when in slow write... aborting.');
                                        end
                                        
                                        % Start over with a slow write
                                        slow_write      = 1;
                                        stopSampOffset  = offset(ifcIndex) - 1;
                                        pktIndex        = 0;
                                        break;
                                    end
                                    
                                    % Node was not ready for the Write IQ
                                    if (write_iq_response == obj.SAMPLE_IQ_NOT_READY)
                                        write_iq_ready_warn = 0;
                                        
                                        % Start over; Maintain "fast write"
                                        stopSampOffset  = offset(ifcIndex) - 1;
                                        pktIndex        = 0;
                                        break;
                                    end
                    
                                    curr_time     = tic;
                                    rcvd_response = 1;
                                end
                            end
                        else
                            % For performance reasons, only check the socket once every 32 packets
                            if (mod(pktIndex, 32) == 0)
                            
                                % Check if the node has sent us a packet that we were not expecting
                                [recv_len, reply] = transport.receive_raw();
                                
                                % If we have a packet, then process the contents
                                if(recv_len > 0)
                                    reply = reply(((cmd_hdr_size_np / 4) + 1):end);       % Strip off transport and command headers
                                    
                                    write_iq_response = process_write_iq_response(obj, reply, samples.sample_iq_id, currChecksum(ifcIndex), write_iq_ready_warn);

                                    % Node was not ready for the Write IQ
                                    if (write_iq_response == obj.SAMPLE_IQ_NOT_READY)
                                        write_iq_ready_warn = 0;
                                        
                                        % Start over; Maintain "fast write"
                                        stopSampOffset  = offset(ifcIndex) - 1;
                                        pktIndex        = 0;
                                    end
                                end
                            end
                            
                            % If this was the last packet and we did not need a response, then this must be a
                            % broadcast Write IQ.  Therefore, we need to use the node to verify the checksum
                            if (pktIndex == num_pkts)
                                node.verify_writeIQ_checksum(currChecksum(ifcIndex));
                            end
                        end
                        
                        if (pktIndex == num_pkts)
                            done = 1;
                        end
                        
                        % Update starting sample offset for next packet
                        startSampOffset       = stopSampOffset + 1;
                        startSampOffsetMinus1 = stopSampOffset;
                        pktIndex              = pktIndex + 1;
                        
                    end %end while !done sending packets
                end %end for ifcIndex
                
                % Exit out of the while loop
                transmitted = 1;
        
            catch sendError
                % If we have a socket exception, this could indicate that the send buffer was not large enough.
                % Therefore, we should use slow writes and see if that fixes the problem.
                if ~isempty(strfind(sendError.message, 'java.net.SocketException'))
                    slow_write  = 1;
                    
                    % If we have tried the slow write and still fail, then throw the error.
                    if ( sendErrors == 1 )
                        fprintf('%s.m--Failed to send writeIQ data after trying slow write.\n', mfilename);
                        throw( sendError );
                    end
                    
                    sendErrors = 1;
                else
                    throw( sendError );
                end
            end %end try
        end %end while( transmitted == 0 )      
    end %end if mex transport
end %end function writeIQ



function out = process_write_iq_response(obj, args, sample_iq_id, checksum, iq_ready_warn)
% process_write_iq_response
%   Helper function to parse write IQ responses
%

    % Initialize the response
    out = obj.SAMPLE_IQ_SUCCESS;

    % Get the IQ ID from the response
    node_sample_iq_id = args(2);
    
    % Only process packets for the current sample_iq_id
    if (node_sample_iq_id == sample_iq_id)
        node_status = args(1);
        
        switch(node_status)
            case obj.SAMPLE_IQ_ERROR
                fprintf('SAMPLE_IQ_ERROR:\n');
                fprintf('    Due to limitations on the node, it is not possible to do a Write IQ while the\n');
                fprintf('    node is transmitting in ''Continuous Tx'' mode.  Please stop the current transmission\n');
                fprintf('    and try the Write IQ again\n');
            
                error('ERROR:  Node returned ''SAMPLE_IQ_ERROR''.  See above for debug information.');
            
            case obj.SAMPLE_IQ_NOT_READY
                % If the node is not ready, then we need to wait until the node is ready and try again from the 
                % beginning of the Write IQ.
                %
                wait_time = compute_sample_wait_time(args(4:end));
            
                % Wait until the samples should be done
                if ( wait_time ~= 0 )
                    pause( wait_time + 0.001 );
                end
                
                % Print warning
                if (iq_ready_warn == 1)
                    fprintf('WARNING:  Node was not ready to process Write IQ request.  Waiting to request again.\n');
                    fprintf('    This warning can be removed by waiting until the node is not busy with a TX or RX\n');
                    fprintf('    operation.  To do this, please add ''pause(1.5 * NUM_SAMPLES * 1/(40e6));'' after\n');
                    fprintf('    any triggers and before the Write IQ request.\n\n');
                end
                
                out = obj.SAMPLE_IQ_NOT_READY;

            case obj.SAMPLE_IQ_SUCCESS
                % If the response was a success, then check the checksum
                %
                node_checksum  = args(3);

                % Compare the checksum values
                if ( node_checksum ~= checksum )
                
                    fprintf('Checksum mismatch:  0x%08x != 0x%08x\n', node_checksum, checksum);
                
                    % Reset the loop variables
                    out = obj.SAMPLE_IQ_CHECKSUM_FAILED;
                end
            
            otherwise
                error('ERROR:  Unknown write IQ response status = %d\n', node_status);
        end
    end
end



function out = compute_sample_wait_time(args)
% compute_sample_wait_time
%   Function to compute the wait time based on the args:
%      args[1]       - Tx status
%      args[2]       - Current Tx read pointer
%      args[3]       - Tx length
%      args[4]       - Rx status
%      args[5]       - Current Rx write pointer
%      args[6]       - Rx length
%

    node_tx_status    = args(1);
    node_tx_pointer   = args(2);
    node_tx_length    = args(3);
    node_rx_status    = args(4);
    node_rx_pointer   = args(5);
    node_rx_length    = args(6);
    
    % NOTE:  node_*_length and node_*_pointer are in bytes.  To convert the difference to microseconds,
    %    we need to divide by: 160e6  (ie 40e6 sample / sec * 4 bytes / sample => 160e6 bytes / sec)
    % 
    if (node_tx_status == 1)
        tx_wait_time = ((node_tx_length - node_tx_pointer) / 160e6);
    else
        tx_wait_time = 0;
    end
    
    if (node_rx_status == 1)
        rx_wait_time = ((node_rx_length - node_rx_pointer) / 160e6);
    else
        rx_wait_time = 0;
    end

    if (tx_wait_time > rx_wait_time)
        out = tx_wait_time;
    else
        out = rx_wait_time;
    end
end



function out = readIQ(obj, node, buffSel, cmdStr, varargin)
% readIQ Helper function for baseband object to read IQ samples from node
% IMPORTANT: user code should never call this function; always use the 
%  'readIQ' baesband command (which will call this function with proper arguments)
%
% Reading a full buffer of IQ samples requires many node-to-host packets
% This function uses the minimum number of packets possible, given the payload
% limitations of the node's transport object.

    myCmd = wl_cmd(node.calcCmd(obj.GRP,obj.CMD_READ_IQ));

    if(isempty(varargin))
        % User didn't specify a starting sample or num samples default to reading all samples (0:rxIQLen-1)
        offset   = 0;
        numSamps = obj.rxIQLen;
    elseif(length(varargin) == 2)
        offset   = varargin{1};
        numSamps = varargin{2};
    else
        error('%s: invalid arguments... user must provide an offset and a length',cmdStr);
    end
        
    % If we have the WARPLab MEX transport, then call the special function   
    if (strcmp(class(node.transport), 'wl_transport_eth_udp_mex'))
        if ( numSamps > obj.MEX_TRANSPORT_MAX_IQ )
            msg0 = sprintf('%s: Requested %d samples.  Due to Matlab memory limitations, the mex transport only supports %d samples.', cmdStr, numSamps, obj.MEX_TRANSPORT_MAX_IQ);
            msg1 = sprintf('\n    If your computer has enough physical memory, you can adjust this limit using node.baseband.MEX_TRANSPORT_MAX_IQ.');
            msg2 = sprintf('\n\n');
            msg  = strcat(msg0, msg1, msg2);
            error(msg);
        end

        % read_buffers(obj, func, num_samples, buffer_ids, start_sample, wl_command, input_type)
        %     NOTE:  Currently the only input type supported is 'double' which has a value of 0
        % 
        rxSamples_IQ = node.transport.read_buffers('IQ', numSamps, buffSel, offset, obj.seq_num_tracker, obj.seq_num_match_severity, node.repr(), myCmd, 0);

    else
        if (numSamps > obj.JAVA_TRANSPORT_MAX_IQ)
            msg0 = sprintf('%s: Requested %d samples.  Due to performance reasons, the java transport only supports %d samples.', cmdStr, numSamps, obj.JAVA_TRANSPORT_MAX_IQ);
            msg1 = sprintf('\n    Please use the MEX transport for larger requests.');
            msg2 = sprintf('\n\n');
            msg  = strcat(msg0, msg1, msg2);
            error(msg);
        end
        
        num_interface = length(buffSel);
    
        % Use the readbuffer helper to handle network I/O
        %     The helper avoids repeating code for reading I/Q and RSSI
        rxSamples = read_baseband_buffer(obj, node, buffSel, myCmd, numSamps, offset, cmdStr);

        rxSamples_IQ = double(zeros(numSamps, num_interface));
        
        for ifcIndex = 1:num_interface
            % Unpack the WARPLab sample
            %   NOTE:  This performs a conversion from an UFix_16_0 to a Fix_16_15 
            %      Process:
            %          1) Treat the 16 bit unsigned value as a 16 bit two's compliment signed value
            %          2) Divide by range / 2 to move the decimal point so resulting value is between +/- 1
            %          3) Cast as complex doubles
            %   NOTE:  This verbose implementation avoids using the fixed-point toolbox
            rxSamples_I = uint16(bitand(bitshift(rxSamples(:, ifcIndex), -16), 65535));       % 16 MSB (Right shift by 16 bits; Mask by 0xFFFF)
            rxSamples_I = double(typecast(rxSamples_I, 'int16'))./2^15;                       % Cast as 'int16'; Divide by 0x8000

            rxSamples_Q = uint16(bitand(rxSamples(:, ifcIndex), 65535));                      % 16 LSB (Mask by 0xFFFF)
            rxSamples_Q = double(typecast(rxSamples_Q, 'int16'))./2^15;                       % Cast as 'int16'; Divide by 0x8000
            
            rxSamples_IQ(:,ifcIndex) = complex(rxSamples_I, rxSamples_Q);
        end
    end

    out = rxSamples_IQ;
end



function out = readRSSI(obj, node, buffSel, cmdStr, varargin)
%readIQ Helper function for baseband object to read IQ samples from node
% IMPORTANT: user code should never call this function; always use the 
%  'readRSSI' baseband command (which will call this function with proper arguments)
%
% Reading a full buffer of RSSI samples requires many node-to-host packets
% This function uses the minimum number of packets possible, given the payload
% limitations of the node's transport object.

    myCmd = wl_cmd(node.calcCmd(obj.GRP,obj.CMD_READ_RSSI));
    num_interface = length(buffSel);

    if(isempty(varargin))
        offset   = 0;
        numSamps = obj.rxRSSILen;
    elseif(length(varargin)==2)
        offset   = varargin{1};
        numSamps = varargin{2};
    else
        error('%s: invalid arguments... user must provide an offset and a length',cmdStr);
    end
    
    %RSSI is a unique buffer in that it stores pairs of RSSI samples in a single 32-bit word. 
    % As such, from the board's perspective, the number of samples that we request is actually 
    % half what the end-user really specifies.
    
    % If we have the WARPLab MEX transport, then call the special function   
    if ( strcmp( class(node.transport), 'wl_transport_eth_udp_mex' ) )
        % The underlying MEX function does not like odd number of samples;  This will potentially request one more sample
        %     than necessary.  That sample will be discarded when the vector is returned.
        samples_to_req = (numSamps/2) + mod((numSamps/2), 2);

        % read_buffers(obj, func, num_samples, buffer_ids, start_sample, wl_command, input_type)
        %     NOTE:  Currently the only input type supported is 'double' which has a value of 0
        % 
        rxSamples_RSSI = node.transport.read_buffers('RSSI', samples_to_req, buffSel, floor(double(offset)/2), obj.seq_num_tracker, obj.seq_num_match_severity, node.repr(), myCmd, 0);

    else

        rxSamples = read_baseband_buffer(obj, node, buffSel, myCmd, ceil(double(numSamps)./2), floor(double(offset)/2), cmdStr);

        for ifcIndex = num_interface:-1:1
            rssi = [mod(bitshift(rxSamples(:, ifcIndex), -16), 1024), mod(rxSamples(:, ifcIndex), 1024)];
            rssi = rssi.';
            rxSamples_RSSI(:,ifcIndex) = rssi(:);
        end
    end

    % Return the appropriate samples depending on the offset
    if(mod(offset,2)==0)
        out = rxSamples_RSSI(1:numSamps, :);
    else
        out = rxSamples_RSSI(2:(numSamps + 1), :);
    end
end



function rx_samples = read_baseband_buffer(obj, node, buffSel, myCmd, num_samples, start_sample, cmdStr)
% read_baseband_buffer  Helper function to read a buffer from the node's baseband
% IMPORTANT: user code should never call this function; always use the 'readRSSI' or
%  'readIQ' baseband commands (which will call properly call this function)
%
% This function implements the process for reading large numbers of samples from a
%  baseband buffer, a transfer which requires multiple transport packets
% 
% This function retrieves samples in order, starting at the user-specified offset and
%  requesting the largest contiguous block of not-yet-retrieved samples. Each reqest
%  may generate many node-to-host packets. In case a packet is lost, this function 
%  will re-request only the missing samples. This function only returns successfully
%  if all requested samples are received from the node.

    transport              = node.transport;               % Get the transport that we will use to send/receive
    curr_samples           = wl_samples();                 % Create a wl_samples object to deserialize the received samples packets
    num_interface          = length(buffSel);              % Determine the number of interfaces to transfer
    USEFULBUFFERSIZE       = .8;                           % Assume (1 - USEFULBUFFERSIZE) is used for Ethernet packet overhead;
    TIMEOUT                = 100;                          % Timeout time (in seconds)
    TRANSPORT_SEND_PKT_LEN = transport.getMaxPayload();    % In bytes
    not_ready_warn         = true;
    
    % Compute the maximum number of samples in each Ethernet packet
    %     Starts with transport.maxPayload is the max number of bytes the node's transport can handle per packet (nominally the Ethernet MTU)
    %     Subtracts sizes of the transport header, command header and samples header
    %     Makes sure that it is 4 sample aligned (ie 16 byte aligned) for node DMA transfers
    % 
    max_samples = double(bitand(((floor(double(TRANSPORT_SEND_PKT_LEN)/4) - sizeof(transport.hdr)/4 - sizeof(wl_cmd)/4) - (sizeof(wl_samples)/4)), 4294967292));
    max_sample_length  = max_samples * 4;

    % Pre-allocate an array to hold retrieved samples
    %     NOTE:  Since Matlab passes function arguments by value, we need to use the wl_samples object as a
    %            container for the samples to pass to read_samples().  However, for performance reasons, we 
    %            are directly setting the internal properties vs using methods.  This could be remedied in 
    %            the future by creating 'raw' methods that will not modify the data or do so in a "smart" way.
    %
    curr_samples.samps = uint32(zeros(num_samples, num_interface, 'double'));
    
    % Get the buffer size
    buffer_size        = bitand(uint32(USEFULBUFFERSIZE * transport.rxBufferSize), uint32(4294967280));       % Mask with 0xFFFF_FFF0 so requests are 16 byte aligned
    
    % Get the samples for each interface
    for ifcIndex = 1:num_interface

        buffer_id = buffSel(ifcIndex);

        if(isSingleBuffer(buffer_id) == 0)
            error('%s: buffer selection must be singular. Use vector notation for reading from multiple buffers e.g. [RFA,RFB]', cmdStr);
        end

        % Initialize loop variables for timeout
        startTime   = tic;
        numloops    = 0;
        total_loops = 1;
        done        = 0;
        seq_num     = 0;
        
        start_sample_this_req = start_sample;
        rcvd_samples          = 0;

        while (done == 0)
            % Each iteration of this loop retrieves the first contiguous block of samples
            %     that has not yet been received from the node
            numloops = numloops + 1;

            % Due to limitations with the receive buffer, we need to chunk the read calls to 
            % the node so that we do not overflow the receive buffer and lose packets
            %
            num_samples_this_req = num_samples - rcvd_samples;
            
            if((num_samples_this_req * 4) > (buffer_size))
                % If the number of bytes we need from the board exceeds the
                %     receive buffer size of our transport, we are going to drop
                %     packets. We should reduce our request to minimize dropping.
                num_samples_this_req = floor(buffer_size / 4);
                total_loops          = total_loops + 1;
            end

            % Calculate how many transport packets are required for this request
            num_pkts = ceil(double(num_samples_this_req)/double(max_samples));

            % fprintf('Useful buffer size = %10d (of %10d) for %10d pkt request\n', buffer_size, transport.rxBufferSize, num_pkts );
            % fprintf('  Num samples      = 0x%08x     Useful buffer samples = 0x%08x\n', num_samples, num_samples_this_req);
            % fprintf('  Offset           = 0x%08x     Payload samples       = 0x%08x\n', start_sample_this_req, max_sample_length);
            
            % Construct and send the argument to the node
            myCmd.setArgs(buffer_id, start_sample_this_req, num_samples_this_req, max_sample_length, num_pkts);
            
            if (numloops == 1)
                node.sendCmd_noresp(myCmd);                            % Normal send
            else 
                node.transport.send(myCmd.serialize(), false, false);  % Do not increment the header for subsequent loops
            end
            
            % Wait for the node to send all requested samples
            seq_num = read_samples(obj, node, myCmd, curr_samples, ifcIndex, start_sample, num_samples_this_req, start_sample_this_req, buffer_id, num_pkts, max_samples);

            % Update loop variables
            start_sample_this_req = start_sample_this_req + num_samples_this_req;
            rcvd_samples          = rcvd_samples + num_samples_this_req;
            
            % Determine if we are done
            if (rcvd_samples >= num_samples)
                done = 1;
            end
            
            % Fail-safe timeout, in case indexing is broken (in m or C), to keep read_baseband_buffers from running forever
            if(toc(startTime) > TIMEOUT)
                  error('read_baseband_buffers took too long to retrieve samples; check for indexing errors in C and M code');
            end
        end %end while 

        % Check the sequence number
        check_seq_num(obj, node, cmdStr, buffer_id, seq_num);
        
        % Update the sequence number
        update_seq_num(obj, cmdStr, buffer_id, seq_num);
        
        if(numloops > total_loops)
            warning('%s: Dropped frames on fast read... took %d iterations', cmdStr, numloops); 
        end
    end% end for all interfaces
    
    rx_samples = curr_samples.samps;
end% end function read_baseband_buffer



function out = update_seq_num(obj, cmd_str, buffer_id, seq_num)
% Update the RX counters for the given buffer ID

    if (strcmp(cmd_str, 'read_iq'))
        if (buffer_id == obj.BB_SEL_RFA) obj.seq_num_tracker(1)  = seq_num; end
        if (buffer_id == obj.BB_SEL_RFB) obj.seq_num_tracker(3)  = seq_num; end
        if (buffer_id == obj.BB_SEL_RFC) obj.seq_num_tracker(5)  = seq_num; end
        if (buffer_id == obj.BB_SEL_RFD) obj.seq_num_tracker(7)  = seq_num; end
    end
    
    if (strcmp(cmd_str, 'read_rssi'))
        if (buffer_id == obj.BB_SEL_RFA) obj.seq_num_tracker(2)  = seq_num; end
        if (buffer_id == obj.BB_SEL_RFB) obj.seq_num_tracker(4)  = seq_num; end
        if (buffer_id == obj.BB_SEL_RFC) obj.seq_num_tracker(6)  = seq_num; end
        if (buffer_id == obj.BB_SEL_RFD) obj.seq_num_tracker(8)  = seq_num; end
    end
    
    % fprintf('Seq Num: %5d %5d %5d %5d %5d %5d %5d %5d\n', obj.seq_num_tracker(1), obj.seq_num_tracker(2), obj.seq_num_tracker(3), obj.seq_num_tracker(4), ...
    %                                                       obj.seq_num_tracker(5), obj.seq_num_tracker(6), obj.seq_num_tracker(7), obj.seq_num_tracker(8));
end



function check_seq_num(obj, node, cmd_str, buffer_id, seq_num)
% Check the sequence number and issue the appropriate response based on the severity

    seq_num_matches = false;

    if (strcmp(cmd_str, 'read_iq'))
        if ((buffer_id == obj.BB_SEL_RFA) && (obj.seq_num_tracker(1) == seq_num)) seq_num_matches = true; end 
        if ((buffer_id == obj.BB_SEL_RFB) && (obj.seq_num_tracker(3) == seq_num)) seq_num_matches = true; end 
        if ((buffer_id == obj.BB_SEL_RFC) && (obj.seq_num_tracker(5) == seq_num)) seq_num_matches = true; end 
        if ((buffer_id == obj.BB_SEL_RFD) && (obj.seq_num_tracker(7) == seq_num)) seq_num_matches = true; end 
    end 

    if (strcmp(cmd_str, 'read_rssi'))
        if ((buffer_id == obj.BB_SEL_RFA) && (obj.seq_num_tracker(2) == seq_num)) seq_num_matches = true; end 
        if ((buffer_id == obj.BB_SEL_RFB) && (obj.seq_num_tracker(4) == seq_num)) seq_num_matches = true; end 
        if ((buffer_id == obj.BB_SEL_RFC) && (obj.seq_num_tracker(6) == seq_num)) seq_num_matches = true; end 
        if ((buffer_id == obj.BB_SEL_RFD) && (obj.seq_num_tracker(8) == seq_num)) seq_num_matches = true; end 
    end
    
    % fprintf('%10s:  Buffer %d:  Seq Num = %d matches %d\n', cmd_str, buffer_id, seq_num, seq_num_matches);
    
    % If the current sequence number matches the recorded sequence number, this means
    % that the buffer has already been read and the appropriate message should be sent
    if (seq_num_matches)
        switch(obj.seq_num_match_severity)
            case obj.SEQ_NUM_MATCH_IGNORE
                % Do nothing
            case obj.SEQ_NUM_MATCH_WARNING
                % Issue a warning
                warning('%s Detected multiple reads of same %s waveform.  If this is unintentional, ensure Rx node triggers are configured correctly.', node.repr(), cmd_str);
            case obj.SEQ_NUM_MATCH_ERROR
                % Issue an error
                error('ERROR:  %s Detected multiple reads of same %s waveform.', node.repr(), cmd_str);
            otherwise
                error('ERROR:  %s Unknown sequence number error severity = %s', node.repr(), obj.seq_num_match_severity);
        end
    end
end



function seq_num = read_samples(obj, node, command, samples, interface, initial_offset, num_samples, start_sample, buffer_id, num_pkts, max_samples)
% read_samples
%   Read the given number of samples from the node
%

    sample_start_tracker = zeros(1, num_pkts);
    sample_num_tracker   = zeros(1, num_pkts);
    
    max_retries          = 2;                               % FIXME - Need to centralize
    max_iq_retries       = 10;
    
    iq_busy_warn         = 1;
    curr_time            = tic;
    num_retries          = 0;
    num_iq_retries       = 0;     
    rcvd_pkts            = 1;
    done                 = 0;
    
    seq_num              = 0;

    while (done == 0)

        if (toc(curr_time) > obj.readTimeout)
        
            if (num_retries >= max_retries)
                fprintf('ERROR:  Exceeded %d retrys for current Read IQ / Read RSSI request \n', max_retries);
                fprintf('    Requested %d samples from buffer %d starting from sample number %d \n', num_samples, buffer_id, start_sample);
                fprintf('    Received %d out of %d packets from node before timeout.\n', rcvd_pkts, num_pkts);
                fprintf('    Please check the node and look at the ethernet traffic to isolate the issue. \n');
            
                error('Error:  Reached maximum number of retrys without a response... aborting.');
            else
                warning('Read IQ / Read RSSI request timed out.  Re-requesting samples.\n');
                
                % Find the first packet error and request the remaining samples
                %   - TBD - For now just request all the packets again 
                %         - See MEX C code for template on how to do this
                
                % Send command
                node.transport.send(command.serialize(), false, false);  % Do not increment the header for subsequent loops
                
                num_retries = num_retries + 1;
                curr_time   = tic;
            end
        end

        % Receive packet        
        resp = node.receiveResp();

        % Process the packet        
        if(~isempty(resp))
        
            % Get the packet data
            args = resp.getArgs;
            
            % Deserialize the sample header
            %     NOTE:  For performance reasons, we are only grabbing values we need directly out of the 
            %            packet data.  Any adjustments to wl_samples will need to be reflected here.
            %
            sample_flags = bitshift(bitand(args(1), 65280), -8);

            % Check the sample header
            if ((sample_flags & samples.FLAG_IQ_ERROR) == samples.FLAG_IQ_ERROR )
                error('ERROR:  Node returned ''SAMPLE_IQ_ERROR''.  Check that node is not currently transmitting in continuous TX mode.');
            
            elseif ((sample_flags & samples.FLAG_IQ_NOT_READY) == samples.FLAG_IQ_NOT_READY )
            
                if (iq_busy_warn == 1)
                    fprintf('WARNING:  Node was not ready to process Read IQ request.  Waiting to request again.\n');
                    fprintf('    This warning can be removed by waiting until the node is not busy with a TX or RX\n');
                    fprintf('    operation.  To do this, please add ''pause(1.5 * NUM_SAMPLES * 1/(40e6));'' after\n');
                    fprintf('    any triggers and before the Read IQ request.\n\n');
                    iq_busy_warn = 0;
                end
                
                % If the node is not ready, then we need to wait until the node is ready and try again from the 
                % beginning of the Write IQ.
                %
                wait_time = compute_sample_wait_time(args(4:end));
            
                % Wait until the samples should be done
                if ( wait_time ~= 0 )
                    pause( wait_time + 0.001 );
                end
                
                num_iq_retries = num_iq_retries + 1;

                % Start over at the beginning
                node.transport.send(command.serialize(), false, false);  % Do not increment the header for subsequent loops
                rcvd_pkts = 1;
                
                % Check that we have not spent a "long time" waiting for samples to be ready
                if (num_iq_retries > max_iq_retries)
                    error('ERROR:  Timeout waiting for node to return samples.  Please check the node operation.');
                end
                
            else
                % Normal IQ data                
                sample_num  = args(2) - initial_offset;
                sample_size = args(3);
                
                % If we are tracking packets, record which samples have been received
                sample_start_tracker(rcvd_pkts) = args(2);
                sample_num_tracker(rcvd_pkts)   = sample_size;

                % Fill in the output arrays - output arrays are (num_requested_samples x num_interfaces)
                start_index = sample_num + 1;
                end_index   = start_index + sample_size - 1;

                % fprintf('  Start index      = 0x%08x     End index             = 0x%08x\n', start_index, end_index);
                
                samples.samps((start_index:end_index), interface) = args(4:end);
                
                % Update loop variables
                rcvd_pkts     = rcvd_pkts + 1;
                num_iq_retrys = 0;
            
                % Exit the loop when we have enough packets
                if (rcvd_pkts > num_pkts) 
                    
                    % Check for errors
                    if (read_iq_sample_error(sample_num_tracker, sample_start_tracker, num_samples, start_sample, num_pkts, max_samples) == 1)
                    
                        if (num_retries >= max_retries)
                            fprintf('ERROR:  Exceeded %d retrys for current Read IQ / Read RSSI request \n', max_retries);
                            fprintf('    Requested %d samples from buffer %d starting from sample number %d \n', num_samples, buffer_id, start_sample);
                            fprintf('    Received %d out of %d packets from node before timeout.\n', rcvd_pkts, num_pkts);
                            fprintf('    Please check the node and look at the ethernet traffic to isolate the issue. \n');
                        
                            error('Error:  Reached maximum number of retrys without a response... aborting.');
                        else
                            warning('Read IQ / Read RSSI IQ Error.  Re-requesting samples.\n');
                            
                            % Find the first packet error and request the remaining samples
                            %   - TBD - For now just request all the packets again 
                            %         - See MEX C code for template on how to do this
                            
                            % Start over at the beginning
                            node.transport.send(command.serialize(), false, false);  % Do not increment the header for subsequent loops
                            
                            sample_start_tracker = zeros(1, num_pkts);
                            sample_num_tracker   = zeros(1, num_pkts);
                            rcvd_pkts            = 1;
                            
                            num_retries = num_retries + 1;
                        end
                    else 
                        % No errors
                        %     Record sequence number and exit the function
                        seq_num = bitand(args(1), 255);
                        done    = 1;
                    end
                end
            end
            
            % Since we received a packet, reset the timeout
            curr_time = tic;
        end
    end
end



function out = read_iq_sample_error(sample_num_tracker, sample_start_tracker, num_samples, start_sample, num_pkts, max_sample_size)
%  Function:  Read IQ sample check
%
%  Function to check if we received all the samples at the correct indexes
%
%  Returns:  0 if no errors
%            1 if if there is an error and prints debug information
%
    try
        num_samples_sum  = uint64(0);
        start_sample_sum = uint64(0);

        % Compute the value of the start samples:
        %   We know that the start samples should follow the pattern:
        %       [ x, (x + y), (x + 2y), (x + 3y), ... , (x + (N - 1)y) ]
        %   where x = start_sample, y = max_sample_size, and N = num_pkts.  This is due
        %   to the fact that the node will fill all packets completely except the last packet.
        %   Therefore, the sum of all element in that array is:
        %       (N * x) + ((N * (N - 1) * Y) / 2
        %
        start_sample_total = uint64(uint64(num_pkts) * uint64(start_sample)) + uint64((uint64(num_pkts * (num_pkts - 1)) * uint64(max_sample_size)) / 2); 
        
        % Compute the totals using the sample tracker
        for idx = 1:num_pkts    
            num_samples_sum  = num_samples_sum + sample_num_tracker(idx);
            start_sample_sum = start_sample_sum + sample_start_tracker(idx);
        end

        % Check the totals
        if ((num_samples_sum ~= num_samples) || (start_sample_sum ~= start_sample_total))
        
            % Debug prints
            %
            % fprintf('Num sample sum   = %16d    Num samples        = %16d\n', num_samples_sum, num_samples);
            % fprintf('Start sample sum = 0x%016x  Start sample total = 0x%016x\n', start_sample_sum, start_sample_total);
            % fprintf('num_pkts         = %16d    Max sample size    = %16d\n', num_pkts, max_sample_size);
            % fprintf('start sample     = 0x%016x  \n', start_sample);
        
            out = 1;
        else
            out = 0;
        end
    catch
        % Print warning that this syntax will be deprecated
        try
            temp = evalin('base', 'wl_uint64_did_warn');
        catch
            fprintf('WARNING:   Matlab version does not support uint64 arithmetic.  Please use Matlab R2011a or later.\n');
            fprintf('WARNING:   The transport will not detect sample transfer errors during readIQ operations.\n');

            warning('Matlab version does not support uint64 arithmetic.  Please use Matlab R2011a or later.');
            
            assignin('base', 'wl_uint64_did_warn', 1)
        end
    
        out = 1;
    end
end



