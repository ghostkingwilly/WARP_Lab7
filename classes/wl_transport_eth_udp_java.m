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

classdef wl_transport_eth_udp_java < wl_transport & handle_light
% Java-based Ethernet UDP Transport for unicast traffic
% User code should not use this object directly-- the parent wl_node will
%  instantiate the appropriate transport object for the hardware in use

%******************************** Properties **********************************

    properties (SetAccess = public)
        timeout;        % Maximum time spent waiting before retransmission
    end

    properties (SetAccess = protected, Hidden = true)
        sock;           % UDP socket
        
        j_address;      % Java address   (statically allocated for performance reasons)
        send_pkt;       % DatagramPacket (statically allocated for performance reasons)
        recv_pkt;       % DatagramPacket (statically allocated for performance reasons)
        
        status;         % Status of UDP socket
        maxPayload;     % Maximum payload size (e.g. MTU - ETH/IP/UDP headers)
    end

    properties (SetAccess = protected)
        address;        % IP address of destination
        port;           % Port of destination
    end
    
    properties (SetAccess = public)
        hdr;            % Transport header object
        rxBufferSize;   % OS's receive buffer size in bytes
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
    end

%********************************* Methods ************************************

    methods
        function obj = wl_transport_eth_udp_java()
            import java.net.InetAddress
            
            obj.hdr         = wl_transport_header;
            obj.hdr.pktType = obj.hdr.PKTTYPE_HTON_MSG;
            obj.status      = 0;
            obj.timeout     = 1;
            obj.maxPayload  = 1000;    % Sane default. This gets overwritten by CMD_PAYLOADSIZETEST command.
            obj.port        = 0;
            obj.address     = '10.0.0.0';
            obj.j_address   = InetAddress.getByName(obj.address);
            
            obj.create_internal_pkts();
            
            obj.checkSetup();
        end
        
        function checkSetup(obj)
            % Currently not implemented
        end

        function setMaxPayload(obj, value)
            if (isempty(value))
               error('wl_transport_eth_udp_java:setMaxPayload', 'setMaxPayload requires a non-empty argument.');
            else
                obj.maxPayload = value;
                
                % Update the send / recv packets
                obj.update_pkt_length();
            end
        end
        
        function out = getMaxPayload(obj)
            out = double(obj.maxPayload);
        end

        function setAddress(obj, value)
            import java.net.InetAddress
            
            if(ischar(value))
                obj.address = value;
            else
                obj.address = obj.int2IP(value);
            end
            
            % Update the java address
            obj.j_address    = InetAddress.getByName(obj.address);
            
            % Update the send / recv packets
            obj.update_pkt_address();
        end
        
        function out = getAddress(obj)
            out = obj.address;
        end

        function setPort(obj, value)
            obj.port = value;
            
            % Update the send / recv packets
            obj.update_pkt_port();
        end
        
        function out = getPort(obj)
            out = obj.port;
        end

        function open(obj, varargin)
            % varargin{1}: (optional) IP address
            % varargin{2}: (optional) port

            REQUESTED_BUF_SIZE = 2^22;
            
            import java.io.*
            import java.net.DatagramSocket
            import java.net.DatagramPacket
            
            if(nargin==3)
               obj.setAddress(varargin{1});
               obj.setPort(varargin{2});
            end
            
            obj.sock = DatagramSocket();
            
            obj.sock.setSoTimeout(1);
            obj.sock.setReuseAddress(1);
            obj.sock.setBroadcast(true);
            obj.sock.setSendBufferSize(REQUESTED_BUF_SIZE);            
            obj.sock.setReceiveBufferSize(REQUESTED_BUF_SIZE); 
 
            x = obj.sock.getReceiveBufferSize();
            
            if(x < REQUESTED_BUF_SIZE)
                fprintf('OS reduced recv buffer size to %d\n', x);
            end
            
            obj.rxBufferSize = x;
            obj.status       = 1;
        end
        
        function out = procCmd(obj, nodeInd, node, cmdStr, varargin)
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
                    out = true; %sendCmd will timeout and raise error if board doesn't respond
                    
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
                        myCmd = wl_cmd(node.calcCmd(obj.GRP, obj.CMD_PAYLOADSIZETEST));
                        myCmd.addArgs(1:payloadTestSizes(index));
                        try
                            resp = node.sendCmd(myCmd);
                            obj.setMaxPayload( resp.getArgs );
                            if(obj.getMaxPayload < (payloadTestSizes(index) * 4))
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
                    obj.sock.close();
                catch closeError
                    warning( 'Error closing socket; java error was %s', closeError.message)
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
            import java.io.*
            import java.net.DatagramSocket
            import java.net.DatagramPacket

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
            
            pkt_send = obj.send_pkt;                                                      % Get pointer to statically allocated packet
            pkt_send.setData(data8, 0, length(data8));                                    % Update the send packet with the correct data
            % pkt_send = DatagramPacket(data8, length(data8), obj.j_address, obj.port);     % Create a java DatagramPacket to send over the network

            % Send the packet
            obj.sock.send(pkt_send);
            
            % Wait to receive reply from the board                
            if(robust == 1)
                hdr_length       = obj.hdr.length;
                pkt_recv         = obj.recv_pkt;
                currTx           = 1;
                numWaitRetries   = 0;
                receivedResponse = 0;
                currTime         = tic;

                while (receivedResponse == 0)
                    try
                        obj.sock.receive(pkt_recv);
                        recv_len = pkt_recv.getLength;
                    catch receiveError
                        if ~isempty(strfind(receiveError.message,'java.net.SocketTimeoutException'))
                            % Timeout receiving; do nothing
                            recv_len = 0;
                        else
                            fprintf('%s.m--Failed to receive UDP packet.\nJava error message follows:\n%s',mfilename,receiveError.message);
                        end
                    end
                    
                    % If we have a packet, then process the contents
                    if(recv_len > 0)
                        recv_data8 = pkt_recv.getData;
                        reply8     = [recv_data8(3:recv_len) zeros(mod(-(recv_len - 2), 4), 1, 'int8')].';
                        reply      = swapbytes(typecast(reply8, 'uint32'));

                        % Check the header to see if this was a valid reply                        
                        if(obj.hdr.isReply(reply(1:hdr_length)))
                        
                            % Check the header to see if we need to wait for the node to be ready
                            if ( obj.hdr.isNodeReady(reply(1:hdr_length)) )
                            
                                % Strip off transport header to give response to caller
                                reply  = reply((hdr_length + 1):end);
                                
                                if(isempty(reply))
                                    reply = []; 
                                end
                                
                                receivedResponse = 1;
                                
                            else
                                % Node is not ready; Wait and try again
                                pause( obj.TRANSPORT_NOT_READY_WAIT_TIME );                
                                numWaitRetries = numWaitRetries + 1;

                                % Send packet packet again
                                obj.sock.send(pkt_send);

                                % Check that we have not spent a "long time" waiting for samples to be ready                
                                if ( numWaitRetries > obj.TRANSPORT_NOT_READY_MAX_RETRY )
                                    error('wl_transport_eth_java:send:isReady', 'Error:  Timeout waiting for node to be ready.  Please check the node operation.');
                                end
                                
                                reply = [];
                            end
                        end
                    end
                    
                    % Look for timeout
                    if ((toc(currTime) > obj.timeout) && (receivedResponse == 0))
                        if(currTx == maxAttempts)
                            error('wl_transport_eth_java:send:noReply','maximum number of retransmissions met without reply from node'); 
                        end
                        
                        % Retry the packet
                        obj.sock.send(pkt_send);
                        currTx   = currTx + 1;
                        currTime = tic;
                    end
                end
            end
        end
        
        function send_raw(obj, send_data, send_length)
            % send_data   : Raw data to be sent over the socket
            % send_length : Length of data to be sent over the socket
            %
            import java.io.*
            import java.net.DatagramSocket
            import java.net.DatagramPacket

            % Format the data / address arguments for transmission
            data8    = typecast(send_data, 'uint8');                                           % Change to uint8
            pkt_send = obj.send_pkt;                                                           % Get pointer to statically allocated packet
            pkt_send.setData(data8, 0, send_length);                                           % Update the send packet with the correct data
            
            % Send the packet
            obj.sock.send(pkt_send);
        end
        
        function resp = receive(obj, varargin)
            % Receive all packets from the Ethernet interface and pass array
            %     of valid responses to the caller.
            %
            % NOTE:  This function will strip off the transport header from the responses
            % NOTE:  This function is non-blocking and will return an empty response if 
            %            there are no packets available.
            %
            import java.io.*
            import java.net.DatagramSocket
            import java.net.DatagramPacket

            % Initialize variables
            hdr_length  = obj.hdr.length;
            pkt_recv    = obj.recv_pkt;
            done        = false;
            resp        = [];
            
            % Receive packets            
            while ~done
                try
                    obj.sock.receive(pkt_recv);
                    recv_len = pkt_recv.getLength;
                catch receiveError
                    if ~isempty(strfind(receiveError.message,'java.net.SocketTimeoutException'))
                        % Timeout receiving; do nothing
                        recv_len = 0;
                    else
                        fprintf('%s.m--Failed to receive UDP packet.\nJava error message follows:\n%s',mfilename,receiveError.message);
                    end
                end
                
                % If we have a packet, then process the contents
                if(recv_len > 0)
                    recv_data8 = pkt_recv.getData;
                    reply8     = [recv_data8(3:recv_len) zeros(mod(-(recv_len - 2), 4), 1, 'int8')].';
                    reply      = swapbytes(typecast(reply8, 'uint32'));
                    
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
        
        function [recv_len, data] = receive_raw(obj, varargin)
            % Receive raw data from the Ethernet interface
            %
            
            % Initialize variables
            pkt_recv    = obj.recv_pkt;
            data        = [];

            % Try to receive a packet
            try
                obj.sock.receive(pkt_recv);
                recv_len = pkt_recv.getLength;
            catch receiveError
                if ~isempty(strfind(receiveError.message, 'java.net.SocketTimeoutException'))
                    recv_len = 0;
                else
                    fprintf('%s.m--Failed to receive UDP packet.\nJava error message follows:\n%s', mfilename, receiveError.message);
                end
            end

            % If we have a packet, then process the contents
            if(recv_len > 0)
                recv_data8 = pkt_recv.getData;
                reply8     = [recv_data8(3:recv_len) zeros(mod(-(recv_len - 2), 4), 1, 'int8')].';
                data       = swapbytes(typecast(reply8, 'uint32'));
            end
        end
        
        function dottedIPout = int2IP(obj,intIn)
            addrChars(4) = mod(intIn, 2^8);
            addrChars(3) = mod(bitshift(intIn, -8), 2^8);
            addrChars(2) = mod(bitshift(intIn, -16), 2^8);
            addrChars(1) = mod(bitshift(intIn, -24), 2^8);
            dottedIPout  = sprintf('%d.%d.%d.%d', addrChars);
        end

        function intOut = IP2int(obj,dottedIP)
            addrChars = sscanf(dottedIP, '%d.%d.%d.%d')';
            intOut    = 2^0 * addrChars(4) + 2^8 * addrChars(3) + 2^16 * addrChars(2) + 2^24 * addrChars(1);
        end
        
        function create_internal_pkts(obj)
            import java.io.*
            import java.net.DatagramPacket

            send_pkt_len     = obj.getMaxPayload();
            send_data        = [zeros(1, send_pkt_len, 'int8')];
            obj.send_pkt     = DatagramPacket(send_data, send_pkt_len, obj.j_address, obj.port);

            recv_pkt_len     = send_pkt_len + 100;
            recv_data        = [zeros(1, recv_pkt_len, 'int8')];
            obj.recv_pkt     = DatagramPacket(recv_data, recv_pkt_len);
        end
        
        function update_pkt_port(obj)
            obj.send_pkt.setPort(obj.port);        
        end
        
        function update_pkt_address(obj)
            obj.send_pkt.setAddress(obj.j_address);
        end
        
        function update_pkt_length(obj)
            send_pkt_len     = obj.getMaxPayload();
            send_data        = zeros(1, send_pkt_len, 'int8');
            obj.send_pkt.setData(send_data);
            obj.send_pkt.setLength(send_pkt_len);
        
            recv_pkt_len     = send_pkt_len + 100;
            recv_data        = zeros(1, recv_pkt_len, 'int8');
            obj.recv_pkt.setData(recv_data);
            obj.recv_pkt.setLength(recv_pkt_len);
        end
    end
end % classdef
