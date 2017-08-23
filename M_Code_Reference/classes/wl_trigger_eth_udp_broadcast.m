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

classdef wl_trigger_eth_udp_broadcast < wl_trigger

    properties (SetAccess = protected)
        description = 'UDP over Ethernet Broadcast Trigger';
        ID;
        transport;
    end
    
    methods
        function obj = wl_trigger_eth_udp_broadcast(varargin)
            if(nargin == 0)
                obj(1) = wl_trigger_eth_udp_broadcast(1);
            else
                if(ischar(varargin{1}))
                    if(strcmpi(varargin{1},'no_id'))
                       return; 
                    else
                       obj(1) = wl_trigger_eth_udp_broadcast(1,varargin{:});
                    end
                else
                    N = varargin{1};
                    IDvec = wl_trigger_IDs.getID(varargin{:});
                    
                    configFile = which('wl_config.ini');
                    
                    if(isempty(configFile))
                       error('cannot find wl_config.ini. please run wl_setup.m'); 
                    end
                    
                    readKeys = {'network','','transport',''};
                    transport = inifile(configFile,'read',readKeys);
                    transport = transport{1};
                    
                    for index = 1:N
                        obj(1,index) = wl_trigger_eth_udp_broadcast('no_id');
                        obj(index).ID = IDvec(index); 

                        switch(transport)
                            case 'java'
                                obj(index).transport = wl_transport_eth_udp_java_bcast;
                            case 'wl_mex_udp'
                                obj(index).transport = wl_transport_eth_udp_mex_bcast;
                        end
                       
                       obj(index).transport.open();                  
                    end
                end
            end           
        end
        
        function out = send(obj)
            if(isempty(obj.ID))
               error('Trigger is missing ID parameter.'); 
            end
            
            obj.transport.send('trigger',obj.ID);
        end
    end 
end

