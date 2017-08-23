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

classdef wl_user_ext < handle_light
   properties(Hidden = true,Constant = true)
       GRP = 'user_extension';
   end
   
   methods
       function obj = wl_user_ext()
           if(strcmp(class(obj),mfilename))
               error('%s is not intended to be constructed directly',mfilename)
           end
       end
       
       function out = procCmd(obj,varargin);
          out = [];
          error('User extension failed to implement the ''procCmd'' method'); 
       end
   end
end