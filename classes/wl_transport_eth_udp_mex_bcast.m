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

classdef wl_transport_eth_udp_mex_bcast < wl_transport & handle_light
% Mex physical layer Ethernet UDP Transport for broadcast traffic
% User code should not use this object directly-- the parent wl_node will
% instantiate the appropriate transport object for the hardware in use

%******************************** Properties **********************************

    properties (SetAccess = protected, Hidden = true)
        sock;
        status;
        isopen;
        maxSamples;          %Maximum number of samples able to be transmitted (based on maxPayload)
        maxPayload;          %Maximum payload size (e.g. MTU - ETH/IP/UDP headers)
    end

    properties (SetAccess = public)
        hdr;
        address;
        port;        
        rxBufferSize;
    end

    properties(Hidden = true, Constant = true)
        REQUIRED_MEX_VERSION           = '1.0.4a';         % Must match version in MEX transport
    end
    
%********************************* Methods ************************************

    methods
        function obj = wl_transport_eth_udp_mex_bcast()
            obj.hdr = wl_transport_header;
            
            % At the moment, a trigger is the only type of broadcast packet.
            % In a future release this type will be exposed to objects that
            % create the broadcast transport object.
            obj.hdr.pktType = obj.hdr.PKTTYPE_TRIGGER;
            obj.checkSetup();
            obj.status = 0;

            configFile = which('wl_config.ini');
            
            if(isempty(configFile))
                error('cannot find wl_config.ini. please run wl_setup.m'); 
            end
            
            readKeys = {'network','','host_address',''};
            IP = inifile(configFile,'read',readKeys);
            IP = IP{1};
            IP = sscanf(IP,'%d.%d.%d.%d');

            readKeys = {'network','','host_ID',[]};
            hostID = inifile(configFile,'read',readKeys);
            hostID = hostID{1};
            hostID = sscanf(hostID,'%d');
            
            readKeys = {'network','','bcast_port',[]};
            bcastport = inifile(configFile,'read',readKeys);
            bcastport = bcastport{1};
            bcastport = sscanf(bcastport,'%d');
            
            obj.address    = sprintf('%d.%d.%d.%d',IP(1),IP(2),IP(3),255);
            obj.port       = bcastport;
            obj.hdr.srcID  = hostID;
            obj.hdr.destID = 65535;    % Changed from 255 in WARPLab 7.1.0            
            obj.maxPayload = 1000;     % Default value;  Can explicitly set a different maxPayload if 
                                       % you are certain that all nodes support a larger packet size.
        end
        
        function checkSetup(obj)
            % Check to make sure wl_mex_udp_transport exists and is is configured
            temp = which('wl_mex_udp_transport');
            
            if(isempty(temp))
                error('wl_transport_eth_udp_mex:constructor','WARPLab Mex UDP transport not found in Matlab''s path');
            elseif(strcmp(temp((end-length(mexext)+1):end),mexext) == 0)
                error('wl_transport_eth_udp_mex:constructor','WARPLab Mex UDP transport found, but it is not a compiled mex file');
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

        function setMaxPayload(obj,value)
            obj.maxPayload = value;
        
            % Compute the maximum number of samples in each Ethernet packet
            %   - Start with maxPayload is the max number of bytes per packet (nominally the Ethernet MTU)
            %   - Subtract sizes of the transport header, command header and samples header
            %   - Due to DMA alignment issues in the node, the max samples must be 4 sample aligned.
            obj.maxSamples = double(bitand(((floor(double(obj.maxPayload)/4) - sizeof(obj.hdr)/4 - sizeof(wl_cmd)/4) - (sizeof(wl_samples)/4)), 4294967292));
        end
        
        function out = getMaxPayload(obj)
            out = double(obj.maxPayload);
        end

        function open(obj,varargin)
            % varargin{1}: (optional) IP address
            % varargin{2}: (optional) port
            %
            if(isempty(obj.isopen))
                if(nargin==3)
                   if(ischar(varargin{1}))
                      obj.address = varargin{1}; 
                   else
                      obj.address = obj.int2IP(varargin{1});
                   end
                   obj.port = varargin{2};
                end
                
                REQUESTED_BUF_SIZE = 2^22;
                
                % Call to 'init_sockets' will internally call setReuseAddress and setBroadcast
                obj.sock = wl_mex_udp_transport('init_socket');
                wl_mex_udp_transport('set_so_timeout', obj.sock, 2000);
                wl_mex_udp_transport('set_send_buf_size', obj.sock, REQUESTED_BUF_SIZE);
                wl_mex_udp_transport('set_rcvd_buf_size', obj.sock, REQUESTED_BUF_SIZE);
                x = wl_mex_udp_transport('get_rcvd_buf_size', obj.sock);
 
                obj.rxBufferSize = x;
                if(x < REQUESTED_BUF_SIZE)
                    fprintf('OS reduced recv buffer size to %d\n', x);
                end

                obj.status = 1;
                obj.isopen = 1;    
            end
        end
                           
        function close(obj)
            try
                wl_mex_udp_transport('close', obj.sock);
            catch closeError
                warning( 'Error closing socket; mex error was %s', closeError.message)
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
        function send(obj,type,varargin)
            % varargin{1}: data    

            switch(lower(type))
                case 'trigger'
                    bitset(obj.hdr.flags,1,0); %no response
                    obj.hdr.pktType = obj.hdr.PKTTYPE_TRIGGER;
                case 'message'
                    obj.hdr.pktType = obj.hdr.PKTTYPE_HTON_MSG;
            end
            
            data = uint32(varargin{1});
            
            obj.hdr.msgLength = (length(data))*4;          % Length in bytes
            obj.hdr.flags     = bitset(obj.hdr.flags,1,0);
            obj.hdr.increment;
            
            data  = [obj.hdr.serialize,data];    
            data8 = [zeros(1,2,'uint8') typecast(swapbytes(uint32(data)),'uint8')];
            
            size = wl_mex_udp_transport('send', obj.sock, data8, length(data8), obj.address, obj.port);
            
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
        
        %-----------------------------------------------------------------
        % write_buffers
        %     Command to utilize additional functionality in the wl_mex_udp_transport C code in order to 
        %     speed up processing of 'writeIQ' commands
        % 
        function reply = write_buffers(obj, func, num_samples, samples_I, samples_Q, buffer_ids, start_sample, hw_ver, wl_command, check_chksum)
            % func           : Function within read_buffers to call
            % number_samples : Number of samples requested
            % samples_I      : Array of uint16 I samples
            % samples_Q      : Array of uint16 Q samples
            % buffer_ids     : Array of Buffer IDs
            % start_sample   : Start sample
            % hw_ver         : Hardware version of the Node
            % wl_command     : Ethernet WARPLab command
            % check_chksum   : Perform the WriteIQ checksum check inside the function
            
            %Calculate how many transport packets are required
            numPktsRequired = ceil(double(num_samples)/double(obj.maxSamples));

            % Construct the WARPLab command that will be used used to write the samples
            payload           = uint32( wl_command.serialize() );        % Convert command to uint32
            obj.hdr.flags     = bitset(obj.hdr.flags,1,0);                % We do not need a response for the sent command
            obj.hdr.msgLength = ( length( payload ) ) * 4;                % Length in bytes
            data              = [obj.hdr.serialize, payload];
            data8             = [zeros(1,2,'uint8') typecast(swapbytes(uint32(data)),'uint8')];
            num_write_iq      = length(buffer_ids);
            reply             = zeros(num_write_iq);
            
            func = lower(func);
            switch(func)
                case 'iq'
                    % Calls the MEX read_iq command
                    %
                    % Needs to handle all the array dimensionality; MEX will only process one buffer ID at a atime 
                    
                    for index = 1:(num_write_iq)                    
                        [cmds_used, checksum] = wl_mex_udp_transport('write_iq', obj.sock, data8, obj.getMaxPayload(), obj.address, obj.port, num_samples, samples_I(:,index), samples_Q(:,index), buffer_ids(index), start_sample, numPktsRequired, obj.maxSamples, hw_ver, check_chksum);
                        
                        % increment the transport header by cmds_used (ie number of commands used
                        obj.hdr.increment(cmds_used);
                        
                        % Record the checksum for that Write IQ
                        reply(index) = checksum;
                    end
                    
                otherwise
                    error('unknown command ''%s''',cmdStr);
                    
            end
            
            reply = checksum;
        end

    end
end % classdef
