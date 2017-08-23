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

classdef wl_transport_eth_udp_mex < wl_transport & handle_light
% Mex physical layer Ethernet UDP Transport for unicast traffic
% User code should not use this object directly-- the parent wl_node will
% instantiate the appropriate transport object for the hardware in use

%******************************** Properties **********************************

    properties (SetAccess = public)
        timeout;             % Maximum time spent waiting before retransmission
    end

    properties (SetAccess = protected, Hidden = true)
        sock;                % UDP socket
        status;              % Status of UDP socket
        maxSamples;          % Maximum number of samples able to be transmitted (based on maxPayload)
        maxPayload;          % Maximum payload size (e.g. MTU - ETH/IP/UDP headers)
    end

    properties (SetAccess = protected)
        address;             % IP address of destination
        port;                % Port of destination
    end
    
    properties (SetAccess = public)
        hdr;                 % Transport header object
        rxBufferSize;        % OS's receive buffer size in bytes
    end

    properties(Hidden = true, Constant = true)
        % These constants define specific command IDs used by this object.
        % Their C counterparts are found in wl_transport.h
        GRP                            = 'transport';
        CMD_PING                       = 1;                % 0x000001
        CMD_PAYLOADSIZETEST            = 2;                % 0x000002
        
        CMD_NODEGRPID_ADD              = 16;               % 0x000010
        CMD_NODEGRPID_CLEAR            = 17;               % 0x000011
        
        TRANSPORT_NOT_READY_MAX_RETRY  = 50;
        TRANSPORT_NOT_READY_WAIT_TIME  = 0.1;
        
        REQUIRED_MEX_VERSION           = '1.0.4a';         % Must match version in MEX transport
    end


%********************************* Methods ************************************

    methods

        function obj = wl_transport_eth_udp_mex()
            obj.hdr         = wl_transport_header;
            obj.hdr.pktType = obj.hdr.PKTTYPE_HTON_MSG;
            obj.status      = 0;
            obj.timeout     = 1;
            obj.maxPayload  = 1000;    % Sane default. This gets overwritten by CMD_PAYLOADSIZETEST command.
            obj.port        = 0;
            obj.address     = '10.0.0.0';
            
            obj.checkSetup();
        end
        
        function checkSetup(obj)
            % Check to make sure wl_mex_udp_transport exists and is is configured
            %
            temp = which('wl_mex_udp_transport');
            
            if(isempty(temp))
                error('wl_transport_eth_udp_mex:constructor', 'WARPLab Mex UDP transport not found in Matlab''s path');
            elseif(strcmp(temp((end-length(mexext)+1):end ), mexext) == 0)
                error('wl_transport_eth_udp_mex:constructor', 'WARPLab Mex UDP transport found, but it is not a compiled mex file');
            end
            
            version     = wl_mex_udp_transport('version');
            version     = sscanf(version, '%d.%d.%d%c');
            version_req = sscanf(obj.REQUIRED_MEX_VERSION, '%d.%d.%d%c');

            % Version must match required version    
            if(~(version(1) == version_req(1) && version(2) == version_req(2) && version(3) == version_req(3) && version(4) >= version_req(4)))
                version     = wl_mex_udp_transport('version');
                version_req = obj.REQUIRED_MEX_VERSION;
                
                error('wl_transport_eth_udp_mex:constructor', 'MEX transport version mismatch.\nRequires version %s, found version %s', version_req, version);
            end
        end
        
        function setMaxPayload(obj, value)
        
            if (isempty(value))
               error('wl_transport_eth_udp_mex:setMaxPayload', 'setMaxPayload requires a non-empty argument.');
            else
                obj.maxPayload = value;
            
                % Compute the maximum number of samples in each Ethernet packet
                %   - Start with maxPayload is the max number of bytes per packet (nominally the Ethernet MTU)
                %   - Subtract sizes of the transport header, command header and samples header
                %   - Due to DMA alignment issues in the node, the max samples must be 4 sample aligned.
                obj.maxSamples = double(bitand(((floor(double(obj.maxPayload)/4) - sizeof(obj.hdr)/4 - sizeof(wl_cmd)/4) - (sizeof(wl_samples)/4)), 4294967292));
                
                % fprintf('Max samples: %d\n', obj.maxSamples);
            end
        end
        
        function out = getMaxPayload(obj)
            out = double(obj.maxPayload);
        end

        function setAddress(obj, value)
            if(ischar(value))
                obj.address = value;
            else
                obj.address = obj.int2IP(value);
            end
        end
        
        function out = getAddress(obj)
            out = obj.address;
        end
        
        function setPort(obj, value)
            obj.port = value;
        end
        
        function out = getPort(obj)
            out = obj.port;
        end

        function open(obj,varargin)
            % varargin{1}: (optional) IP address
            % varargin{2}: (optional) port
            %
            if(nargin==3)
               obj.setAddress(varargin{1});
               obj.port = varargin{2};
            end

            REQUESTED_BUF_SIZE = 2^22;
             
            % Call to 'init_sockets' will internally call setReuseAddress and setBroadcast
            obj.sock = wl_mex_udp_transport('init_socket');
            wl_mex_udp_transport('set_so_timeout', obj.sock, 1);
            wl_mex_udp_transport('set_send_buf_size', obj.sock, REQUESTED_BUF_SIZE);
            wl_mex_udp_transport('set_rcvd_buf_size', obj.sock, REQUESTED_BUF_SIZE);

            x = wl_mex_udp_transport('get_rcvd_buf_size', obj.sock);            
            obj.rxBufferSize = x;

            if(x < REQUESTED_BUF_SIZE)
                fprintf('OS reduced recv buffer size to %d\n', x);
            end
            
            obj.status = 1;
        end
        
        function out = procCmd(obj,nodeInd,node,cmdStr,varargin)
            % wl_node procCmd(obj, nodeInd, node, varargin)
            %     obj:       Node object (when called using dot notation)
            %     nodeInd:   Index of the current node, when wl_node is iterating over nodes
            %     node:      Current node object
            %     cmdStr:    Command string of the interface command
            %     varargin:
            %         [1:N}  Command arguments
            %
            out    = [];
            
            cmdStr = lower(cmdStr);
            switch(cmdStr)
            
                %---------------------------------------------------------
                case 'ping'
                    % Test to make sure node can be accessed via this
                    % transport
                    %
                    % Arguments: none
                    % Returns: true if board responds; raises error otherwise
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP,obj.CMD_PING));
                    node.sendCmd(myCmd);
                    out = true;                            % sendCmd will timeout and raise error if board doesn't respond
                    
                %---------------------------------------------------------
                case 'payload_size_test'
                    % Determine objects maxPayload parameter
                    %
                    % Arguments: none
                    % Returns: none
                    %
                    configFile = which('wl_config.ini');
                    
                    if(isempty(configFile))
                       error('cannot find wl_config.ini. please run wl_setup.m'); 
                    end
                    
                    readKeys = {'network', '', 'max_transport_payload_size', ''};
                    max_transport_payload_size = inifile(configFile, 'read', readKeys);
                    max_transport_payload_size = str2num(max_transport_payload_size{1});
                    
                    % Determine the payloads to test
                    payloadTestSizes = [];

                    for i = [1000 1470 5000 8966]
                        if (i < max_transport_payload_size) 
                            payloadTestSizes = [payloadTestSizes, i];
                        end
                    end
                    
                    payloadTestSizes = [payloadTestSizes, max_transport_payload_size];
                                        
                    % WARPLab Header is (see http://warpproject.org/trac/wiki/WARPLab/Reference/Architecture/WireFormat ):
                    %   - 2 byte pad
                    %   - Transport header
                    %   - Command header
                    payloadTestSizes = floor((payloadTestSizes - (sizeof(node.transport.hdr) + sizeof(wl_cmd) + 2)) / 4);
                    
                    for index = 1:length(payloadTestSizes)
                        myCmd = wl_cmd(node.calcCmd(obj.GRP,obj.CMD_PAYLOADSIZETEST));
                        myCmd.addArgs(1:payloadTestSizes(index));
                        try                        
                            resp = node.sendCmd(myCmd);
                            obj.setMaxPayload(resp.getArgs);
                            if(obj.getMaxPayload() < (payloadTestSizes(index) * 4))
                               break; 
                            end
                        catch ME
                            break 
                        end
                    end
                    
                %---------------------------------------------------------
                case 'add_node_group_id'
                    % Adds a Node Group ID to the node so that it can
                    % process broadcast commands that are received from
                    % that node group.
                    %
                    % Arguments: (uint32 NODE_GRP_ID)
                    % Returns: none
                    %
                    % NODE_GRP_ID: ID provided by wl_node_grp
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_NODEGRPID_ADD));
                    myCmd.addArgs(varargin{1});
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                case 'clear_node_group_id'
                    % Clears a Node Group ID from the node so it can ignore
                    % broadcast commands that are received from that node
                    % group.
                    %
                    % Arguments: (uint32 NODE_GRP_ID)
                    % Returns: none
                    %
                    % NODE_GRP_ID: ID provided by wl_node_grp
                    %
                    myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_NODEGRPID_CLEAR));
                    myCmd.addArgs(varargin{1});
                    node.sendCmd(myCmd);
                    
                %---------------------------------------------------------
                otherwise
                    error('unknown command ''%s''',cmdStr);
            end
            
            if((iscell(out) == 0) && (numel(out) ~= 1))
                out = {out}; 
            end     
        end
        
        function close(obj)            
            if(~isempty(obj.sock))
                try
                    wl_mex_udp_transport('close', obj.sock);
                catch closeError
                    warning( 'Error closing socket; mex error was %s', closeError.message)
                end
            end
            obj.status=0;
        end
        
        function delete(obj)
            obj.close();
        end
        
        function flush(obj)
            % Currently not implemented
        end
    end % methods

    
    methods (Hidden = true)

        function reply = send(obj, send_data, response, varargin)
            % send_data  : Data to be sent to the node
            % response   : Does the transmission require a response from the node
            % varargin{1}: (optional)
            %     - increment the transport header; defaults to true if not specified
            %

            % Initialize variables
            maxAttempts       = 2;                                   % Maximum times the transport will re-try a packet
            payload           = uint32(send_data);                   % Change data to 32 bit unsigned integers for transmit
            obj.hdr.msgLength = ((length(payload)) * 4);             % Length in bytes (4 bytes / sample)
            reply             = [];                                  % Initialize array for response from the node
            robust            = response;

            % Process command line arguments
            increment_hdr     = true;

            if (nargin == 4)
               increment_hdr  = varargin{1};
            end
            
            % Set the appropriate flags in the header            
            if(robust)
               obj.hdr.flags = bitset(obj.hdr.flags, 1, 1);
            else
               obj.hdr.flags = bitset(obj.hdr.flags, 1, 0);
            end

            % Increment the header and serialize all the data into a uint32 array
            if (increment_hdr)
                obj.hdr.increment;
            end
                        
            % Format the data / address arguments for transmission
            data     = [obj.hdr.serialize, payload];                                      % Get a unified array with the header and data
            data8    = [zeros(1,2,'uint8') typecast(swapbytes(uint32(data)), 'uint8')];   % Change to uint8 and pad by 2 bytes so payload is 32 bit aligned w/ Eth header

            % Send the packet
            size = wl_mex_udp_transport('send', obj.sock, data8, length(data8), obj.address, obj.port);

            % Wait to receive reply from the board                
            if(robust == 1)
                MAX_PKT_LEN      = obj.getMaxPayload() + 100;
                hdr_length       = obj.hdr.length;
                currTx           = 1;
                numWaitRetries   = 0;
                receivedResponse = 0;
                currTime         = tic;

                while (receivedResponse == 0)
                    try
                        [recv_len, recv_data8] = wl_mex_udp_transport('receive', obj.sock, MAX_PKT_LEN);

                    catch receiveError
                        error('%s.m -- Failed to receive UDP packet.\nMEX transport error message follows:\n    %s\n', mfilename, receiveError.message);
                    end
                    
                    % If we have a packet, then process the contents
                    if(recv_len > 0)
                        reply8     = [recv_data8(3:recv_len) zeros(mod(-(recv_len - 2), 4), 1, 'uint8')];
                        reply      = swapbytes(typecast(reply8, 'uint32'));

                        % Check the header to see if this was a valid reply                        
                        if(obj.hdr.isReply(reply(1:hdr_length)))
                        
                            % Check the header to see if we need to wait for the node to be ready
                            if (obj.hdr.isNodeReady(reply(1:hdr_length)))
                            
                                % Strip off transport header to give response to caller
                                reply  = reply((hdr_length + 1):end);
                                
                                if(isempty(reply))
                                    reply = []; 
                                end
                                
                                receivedResponse = 1;
                                
                            else
                                % Node is not ready; Wait and try again
                                pause(obj.TRANSPORT_NOT_READY_WAIT_TIME);
                                numWaitRetries = numWaitRetries + 1;

                                % Send packet packet again
                                obj.sock.send(pkt_send);

                                % Check that we have not spent a "long time" waiting for samples to be ready                
                                if (numWaitRetries > obj.TRANSPORT_NOT_READY_MAX_RETRY)
                                    error('wl_transport_eth_mex:send:isReady', 'Error:  Timeout waiting for node to be ready.  Please check the node operation.');
                                end
                                
                                reply = [];
                            end
                        end
                    end
                    
                    % Look for timeout
                    if ((toc(currTime) > obj.timeout) && (receivedResponse == 0))
                        if(currTx == maxAttempts)
                            error('wl_transport_eth_mex:send:noReply', 'maximum number of retransmissions met without reply from node'); 
                        end
                        
                        % Retry the packet
                        size = wl_mex_udp_transport('send', obj.sock, data8, length(data8), obj.address, obj.port);
                        currTx   = currTx + 1;
                        currTime = tic;
                    end
                end
            end
        end
        
        function resp = receive(obj)
            % Receive all packets from the Ethernet interface and pass array
            %     of valid responses to the caller.
            %
            % NOTE:  This function will strip off the transport header from the responses
            % NOTE:  This function is non-blocking and will return an empty response if 
            %            there are no packets available.
            %

            % Initialize variables
            MAX_PKT_LEN = obj.getMaxPayload() + 100;            
            hdr_length  = obj.hdr.length;
            done        = false;
            resp        = [];
            
            while ~done
                try
                    [recv_len, recv_data8] = wl_mex_udp_transport('receive', obj.sock, MAX_PKT_LEN);

                catch receiveError
                    error('%s.m -- Failed to receive UDP packet.\nMEX transport error message follows:\n    %s\n', mfilename, receiveError.message);
                end
                
                if(recv_len > 0)
                    reply8 = [recv_data8(3:recv_len) zeros(mod(-(recv_len - 2), 4), 1, 'uint8')].';
                    reply  = swapbytes(typecast(reply8, 'uint32'));

                    if(obj.hdr.isReply(reply(1:hdr_length)))
                        % Strip off transport header to give response to caller
                        reply  = reply((hdr_length + 1):end);
                        
                        if(isempty(reply))
                            reply = [];
                        end
                        
                        resp = [resp, reply];                        

                        done = true;
                    end
                else
                    done = true;
                end
            end
        end
        
        function dottedIPout = int2IP(obj,intIn)
            addrChars(4) = mod(intIn, 2^8);
            addrChars(3) = mod(bitshift(intIn, -8), 2^8);
            addrChars(2) = mod(bitshift(intIn, -16), 2^8);
            addrChars(1) = mod(bitshift(intIn, -24), 2^8);
            dottedIPout = sprintf('%d.%d.%d.%d', addrChars);
        end
        
        function intOut = IP2int(obj,dottedIP)
            addrChars = sscanf(dottedIP, '%d.%d.%d.%d')';
            intOut = 2^0 * addrChars(4) + 2^8 * addrChars(3) + 2^16 * addrChars(2) + 2^24 * addrChars(1);
        end
        
        function reply = print_cmd(obj, type, num_samples, start_sample, buffer_ids, command)
            fprintf('Command:  %s \n', type);
            fprintf('    # samples  = %d    start sample = %d \n', num_samples, start_sample);
            fprintf('    buffer IDs = ');
            for index = 1:length(buffer_ids)
                fprintf('%d    ', buffer_ids(index));
            end
            fprintf('\n    Command (%d bytes): ', length(command) );
            for index = 1:length(command)
                switch(index)
                    case 1
                        fprintf('\n        Padding  : ');
                    case 3
                        fprintf('\n        Dest ID  : ');
                    case 5
                        fprintf('\n        Src ID   : ');
                    case 7
                        fprintf('\n        Rsvd     : ');
                    case 8
                        fprintf('\n        Pkt Type : ');
                    case 9
                        fprintf('\n        Length   : ');
                    case 11
                        fprintf('\n        Seq Num  : ');
                    case 13
                        fprintf('\n        Flags    : ');
                    case 15
                        fprintf('\n        Command  : ');
                    case 19
                        fprintf('\n        Length   : ');
                    case 21
                        fprintf('\n        # Args   : ');
                    case 23
                        fprintf('\n        Args     : ');
                    otherwise
                        if ( index > 23 ) 
                            if ( ( mod( index + 1, 4 ) == 0 ) && not( index == length(command) ) )
                                fprintf('\n                   ');
                            end
                        end
                end
               fprintf('%2x ', command(index) );
            end
            fprintf('\n\n');
            
            reply = 0;
        end

        %-----------------------------------------------------------------
        % read_buffers
        %     Command to utilize additional functionality in the wl_mex_udp_transport C code in order to 
        %     speed up processing of 'readIQ' and 'readRSSI' commands
        % 
        % Supports the following calling conventions:
        %    - start_sample -> must be a single value
        %    - num_samples  -> must be a single value
        %    - buffer_ids   -> Can be a vector of single RF interfaces
        % 
        function reply = read_buffers(obj, func, num_samples, buffer_ids, start_sample, seq_num_tracker, seq_num_match_severity, node_id_str, wl_command, input_type)
            % func                     : Function within read_buffers to call
            % number_samples           : Number of samples requested
            % buffer_ids               : Array of Buffer IDs
            % start_sample             : Start sample
            % seq_num_tracker          : Sequence number tracker
            % seq_num_match_severity   : Severity of message when sequence numbers match on reads
            % node_id_str              : String representation of Node ID
            % wl_command               : Ethernet WARPLab command
            % input_type               : Type of sample array:
            %                                0 ==> 'double'
            %                                1 ==> 'single'
            %                                2 ==> 'int16'
            %                                3 ==> 'raw'

            % Get the lowercase version of the function            
            func = lower(func);

            % Calculate how many transport packets are required
            numPktsRequired = ceil(double(num_samples)/double(obj.maxSamples));

            % Arguments for the command will be set in the MEX function since it is faster
            %     wl_command.setArgs(buffer_id, start_sample, num_samples, obj.maxSamples * 4, numPktsRequired);
            
            % Construct the minimal WARPLab command that will be used used to get the samples
            %   NOTE:  Since we did not add the arguments of the command thru setArgs, we need to pad the structure so that 
            %          the proper amount of memory is allocated to be available to the MEX
            payload           = uint32( wl_command.serialize() );        % Convert command to uint32
            cmd_args_pad      = uint32( zeros(1, 5) );                   % Padding for command args
            obj.hdr.flags     = bitset(obj.hdr.flags,1,0);               % We do not need a response for the sent command
            obj.hdr.msgLength = ( ( length( payload ) ) + 5) * 4;        % Length in bytes

            data  = [obj.hdr.serialize, payload, cmd_args_pad];
            data8 = [zeros(1,2,'uint8') typecast(swapbytes(uint32(data)), 'uint8')];

            % Pass all of the command arguments down to MEX
            switch(func)
                case 'iq'
                    % Calls the MEX read_iq command
                    %
                    % obj.print_cmd('READ_IQ', num_samples, start_sample, buffer_ids, data8);
                    
                    [num_rcvd_samples, cmds_used, rx_samples]  = wl_mex_udp_transport('read_iq', obj.sock, data8, length(data8), obj.address, obj.port, num_samples, buffer_ids, start_sample, obj.maxSamples * 4, numPktsRequired, input_type, seq_num_tracker, seq_num_match_severity, node_id_str);
                    
                    % Code to test higher level matlab code without invoking the MEX transport
                    % 
                    % rx_samples = zeros( num_samples, 1 );
                    % cmds_used  = 0;

                case 'rssi'
                    % Calls the MEX read_rssi command
                    %                        
                    % obj.print_cmd('READ_RSSI', num_samples, start_sample, buffer_ids, data8);

                    [num_rcvd_samples, cmds_used, rx_samples]  = wl_mex_udp_transport('read_rssi', obj.sock, data8, length(data8), obj.address, obj.port, num_samples, buffer_ids, start_sample, obj.maxSamples * 4, numPktsRequired, input_type, seq_num_tracker, seq_num_match_severity, node_id_str);

                    % Code to test higher level matlab code without invoking the MEX transport
                    %
                    % rx_samples = zeros( num_samples * 2, 1 );
                    % cmds_used  = 0;

                otherwise
                    error('unknown command ''%s''', cmdStr);
            end
            
            obj.hdr.increment(cmds_used);
            
            reply = rx_samples;
        end
 
 
        %-----------------------------------------------------------------
        % write_buffers
        %     Command to utilize additional functionality in the wl_mex_udp_transport C code in order to 
        %     speed up processing of 'writeIQ' commands
        % 
        function reply = write_buffers(obj, func, num_samples, samples, buffer_ids, start_sample, hw_ver, wl_command, check_chksum, input_type)
            % func           : Function within read_buffers to call
            % number_samples : Number of samples requested
            % samples        : Array of IQ samples
            % buffer_ids     : Array of Buffer IDs
            % start_sample   : Start sample
            % hw_ver         : Hardware version of the Node
            % wl_command     : Ethernet WARPLab command
            % check_chksum   : Perform the WriteIQ checksum check inside the function
            % input_type     : Type of sample array:
            %                      0 ==> 'double'
            %                      1 ==> 'single'
            %                      2 ==> 'int16'
            %                      3 ==> 'raw'
            
            % Calculate how many transport packets are required
            num_pkts_required = ceil(double(num_samples)/double(obj.maxSamples));

            % Construct the WARPLab command that will be used used to write the samples
            payload           = uint32( wl_command.serialize() );        % Convert command to uint32
            obj.hdr.flags     = bitset(obj.hdr.flags,1,0);               % We do not need a response for the sent command
            obj.hdr.msgLength = ( length( payload ) ) * 4;               % Length in bytes
            
            data              = [obj.hdr.serialize, payload];
            data8             = [zeros(1,2,'uint8') typecast(swapbytes(uint32(data)), 'uint8')];
            
            socket            = obj.sock;
            address           = obj.address;
            port              = obj.port;
            max_payload       = obj.getMaxPayload();
            max_samples       = obj.maxSamples;
            

            func = lower(func);
            switch(func)
                case 'iq'
                    % Calls the MEX read_iq command
                    %
                    [cmds_used, checksum] = wl_mex_udp_transport('write_iq', socket, data8, max_payload, address, port, num_samples, samples, buffer_ids, start_sample, num_pkts_required, max_samples, hw_ver, check_chksum, input_type);
                    
                    % Increment the transport header by cmds_used (ie number of commands used
                    obj.hdr.increment(cmds_used);
                    
                    % Record the checksum for that Write IQ
                    reply = checksum;
                    
                otherwise
                    error('unknown command ''%s''',cmdStr);
            end
            
            reply = checksum;
        end
    end
end % classdef




