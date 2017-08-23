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

classdef wl_transport_eth_udp_java_bcast < wl_transport & handle_light
% Java physical layer Ethernet UDP Transport for broadcast traffic
% User code should not use this object directly-- the parent wl_node will
% instantiate the appropriate transport object for the hardware in use

%******************************** Properties **********************************

    properties (SetAccess = protected, Hidden = true)
        sock;
        status;
        isopen;
        maxPayload;
    end

    properties (SetAccess = public)
        hdr;
        address;
        port;        
        rxBufferSize;
    end

%********************************* Methods ************************************

    methods
        function obj = wl_transport_eth_udp_java_bcast()
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
            
            readKeys = {'network', '', 'host_address', ''};
            IP = inifile(configFile,'read',readKeys);
            IP = IP{1};
            IP = sscanf(IP,'%d.%d.%d.%d');

            readKeys = {'network', '', 'host_ID', []};
            hostID = inifile(configFile,'read',readKeys);
            hostID = hostID{1};
            hostID = sscanf(hostID,'%d');
            
            readKeys = {'network', '', 'bcast_port', []};
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
            % Currently not implemented
        end

        function setMaxPayload(obj,value)
            obj.maxPayload = value;
        end
        
        function out = getMaxPayload(obj)
            out = double(obj.maxPayload);
        end

        function open(obj,varargin)
            % varargin{1}: (optional) IP address
            % varargin{2}: (optional) port
            %
            REQUESTED_BUF_SIZE = 2^22;
                
            import java.io.*
            import java.net.DatagramSocket
            import java.net.DatagramPacket
            import java.net.InetAddress            

            if(isempty(obj.isopen))
                if(nargin==3)
                   if(ischar(varargin{1}))
                      obj.address = varargin{1}; 
                   else
                      obj.address = obj.int2IP(varargin{1});
                   end
                   obj.port = varargin{2};
                end
                
                obj.sock = DatagramSocket();
                
                obj.sock.setSoTimeout(2000);
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
                obj.isopen       = 1;              
            end
        end

        function close(obj)
            try
                obj.sock.close();
            catch closeError
                warning( 'Error closing socket; java error was %s', closeError.message)
            end
            
            obj.status=0;
        end    

        function delete(obj)
            obj.close();
        end

        function flush(obj)
            % Currently not implemented
        end
    end %methods

    methods (Hidden = true)
        function send(obj, type, varargin)
            % varargin{1}: data    

            import java.io.*
            import java.net.DatagramSocket
            import java.net.DatagramPacket
            import java.net.InetAddress            

            switch(lower(type))
                case 'trigger'
                    bitset(obj.hdr.flags,1,0); % no response
                    obj.hdr.pktType = obj.hdr.PKTTYPE_TRIGGER;
                case 'message'
                    obj.hdr.pktType = obj.hdr.PKTTYPE_HTON_MSG;
            end
            
            data = uint32(varargin{1});
            
            obj.hdr.msgLength = (length(data))*4; %Length in bytes
            obj.hdr.flags     = bitset(obj.hdr.flags,1,0);
            obj.hdr.increment;
            
            data  = [obj.hdr.serialize,data];    
            data8 = [zeros(1,2,'uint8') typecast(swapbytes(uint32(data)),'uint8')];
            
            jaddr = InetAddress.getByName(obj.address);
            
            pkt_send = DatagramPacket(data8, length(data8), jaddr, obj.port);
            obj.sock.send(pkt_send);
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
    end
end % classdef
