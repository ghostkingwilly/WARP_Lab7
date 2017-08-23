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

classdef wl_msg_helper < handle_light
    properties (Abstract)
    end
    
    methods (Abstract)
        out = serialize(varargin);
        out = deserialize(varargin);
        out = sizeof(varargin);
    end 
end
