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

classdef wl_trigger_IDs < handle_light
    
    properties(SetAccess = private)
        IDs;
        currID_index;
    end
    
    methods(Access = private)
        function obj = wl_trigger_IDs()
            obj.currID_index = 1;
            obj.IDs = uint32(1);
            
            for k = 2:32
               obj.IDs(k) = uint32(bitshift(obj.IDs(k-1),1)); 
            end
        end
    end
    
    methods(Static)
        function obj = instance()
            persistent uniqueInstance
            
            if isempty(uniqueInstance)
                obj = wl_trigger_IDs;
                uniqueInstance = obj;
            else
                obj = uniqueInstance;
            end
        end
        
        function out = numAllocatedTrigs()
            obj = wl_trigger_IDs.instance();
            out = obj.currID_index - 1;
        end
        
        function reset()
            obj = wl_trigger_IDs.instance();
            obj.currID_index = 1;
        end
        
        function out = getID(varargin)
           obj = wl_trigger_IDs.instance();
            
           N = varargin{1};
           reset = true;
           
           for k = 1:length(varargin)
              if(ischar(varargin{k}))
                  if(strcmpi(varargin{k},'incrementid'))
                      reset = false;
                  elseif(strcmpi(varargin{k},'resetid'))
                      reset = true;
                  end
              end
           end
           
           if(reset)
              obj.reset();
           end
           
           if((obj.currID_index + (N - 1)) > length(obj.IDs))
                error('Too many trigger IDs have been allocated. The total number of triggers from the host computer cannot exceed 32.'); 
           end
           
           out = obj.IDs(obj.currID_index:((obj.currID_index + (N - 1))));
           obj.currID_index = obj.currID_index + N;
        end
    end
end