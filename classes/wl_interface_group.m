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

classdef wl_interface_group < handle_light
    properties (Abstract, SetAccess = protected)
        num_interface;
        ID;
        label;
        description;        
    end
    
    methods (Abstract)
        out = wl_ifcValid(varargin);
        out = procCmd(varargin);
    end 
end
