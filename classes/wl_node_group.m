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

classdef wl_node_group < wl_node

    properties (SetAccess = protected)
        nodes;                         % List of nodes in the node group
    end

    properties (SetAccess = public)
        verify_write_iq_checksum;      % Enable checking for WriteIQ checksum
    end
    
    methods
        function obj = wl_node_group(varargin)
            obj.verify_write_iq_checksum = 1;    % Enabled checking by default.

            if(isempty(varargin))
               ID = -1; %Invalid ID just so the error message is caught 
            else
               ID = varargin{1};
            end
            
            if(numel(ID)~=1)
                error('ID argument must be a scalar between 0 and 7');
            end
            
            switch(ID)
                case 0
                    obj.ID = hex2dec('01');
                case 1
                    obj.ID = hex2dec('02');
                case 2
                    obj.ID = hex2dec('04');
                case 3
                    obj.ID = hex2dec('08');
                case 4
                    obj.ID = hex2dec('10');
                case 5
                    obj.ID = hex2dec('20');
                case 6
                    obj.ID = hex2dec('40');
                case 7
                    obj.ID = hex2dec('80');
                otherwise
                    error('ID argument must be a scalar between 0 and 7');
            end
            
            % Get ini configuration file 
            configFile = which('wl_config.ini');
            if(isempty(configFile))
                error('cannot find wl_config.ini. please run wl_setup.m'); 
            end
            
            readKeys = {'network','','transport',''};
            transportType = inifile(configFile,'read',readKeys);
            transportType = transportType{1};

            switch(transportType)
                case 'java'
                    obj.transport = wl_transport_eth_udp_java_bcast();
                    obj.transport.open();
                case 'wl_mex_udp'
                    obj.transport = wl_transport_eth_udp_mex_bcast();
                    obj.transport.open();
            end
            
            obj.transport.hdr.destID = obj.ID;
        end
        
        
        function addNodes(obj,nodes)
            nodes.wl_transportCmd('add_node_group_id', obj.ID);
            obj.nodes = [obj.nodes(:);nodes(:)].';
            obj.updateGroupModules();
        end
        
        
        function removeNodes(obj,nodes)            
            for n = 1:length(nodes)
                I = find(obj.nodes == nodes(n));
                if(isempty(I))
                   error('Node %d is not a member of this node group', nodes(n).ID); 
                else
                   nodes(n).wl_transportCmd('clear_node_group_id', obj.ID);
                   obj.nodes(I) = [];  
                end
            end
            obj.updateGroupModules();
        end
        
        
        function setHwVer(obj, hwVer)
            obj.hwVer = hwVer;
        end
        
        
        function verify_writeIQ_checksum(obj, checksum)
            % This is a callback to verify the WriteIQ checksum in the case that WriteIQ is called by a 
            % broadcast transport.  Need to verify the checksum for each node in the node group.
            %
            if ( obj.verify_write_iq_checksum == 1 )            
                if ( numel(checksum) > 1 )
                    error('Cannot verify more than one checksum at a time.')
                end
            
                for i = 1:numel(obj.nodes)
                    node_checksum = obj.nodes(i).wl_basebandCmd('write_iq_checksum');
            
                    if ( node_checksum ~= checksum(1) )
                        warning('Checksums do not match on node %d:  %d != %d', obj.ID, node_checksum, checksum)
                    end
                end
            end
        end

        
        function out = sendCmd(obj, cmd)
            % Node groups can't receive anything. So we'll just silently
            % pass the command along to the sendCmd_noresp method.
            %
            sendCmd_noresp(obj, cmd);
            
            out = wl_resp();
        end
        
        
        function sendCmd_noresp(obj, cmd)
            % This method is responsible for serializing the command
            % objects provided by each of the components of WARPLab into a
            % vector of values that the transport object can send. This 
            % method is used when a board should not send an immediate
            % response. The transport object is non-blocking -- it will send
            % the command and then immediately return.
            %
            obj.transport.send('message', cmd.serialize());
        end 
        
        
        function out = receiveResp(obj)
            error('NoResponses', 'Node groups use a broadcast transport and cannot receive responses from WARP nodes.')
        end
        
        
        function disp(obj)
           fprintf('Node Group ID: %d\n',obj.ID);
           fprintf('Node Members:\n');
           disp(obj.nodes);
        end
    end
    
    
    methods (Hidden = true)
       function updateGroupModules(obj)
            if(~isempty(obj.nodes))
                obj.baseband = obj.nodes(1).baseband;
                obj.num_interfacesGroups = obj.nodes(1).num_interfacesGroups;
                obj.num_interfaces = obj.nodes(1).num_interfaces;
                obj.interfaceGroups = obj.nodes(1).interfaceGroups;
                obj.interfaceIDs = obj.nodes(1).interfaceIDs;
                obj.trigger_manager = obj.nodes(1).trigger_manager;
                obj.user = obj.nodes(1).user;
            else
                obj.baseband = [];
                obj.num_interfacesGroups = [];
                obj.num_interfaces = [];
                obj.interfaceGroups = [];
                obj.interfaceIDs = [];
                obj.trigger_manager = [];
                obj.user = [];
            end
        end 
    end
end % end classdef
