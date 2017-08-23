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

classdef handle_light < handle
    % handle_light Helper class for inheriting from handle
    %     All WARPLab classes inherit from handle, but don't use many of handle's methods
    %     In order to hide these methods in the auto-generated documentation,
    %     WARPLab classes inherit from handle via handle_light, which re-classifies
    %     handle's methods as "Hidden".
    %
    % This idea and original code come from sclarke81 on stackoverflow:
    %     http://stackoverflow.com/questions/6621850
    %
    methods(Hidden)
    
        function lh = addlistener(varargin)
           lh = addlistener@handle(varargin{:});
        end
        
        function notify(varargin)
           notify@handle(varargin{:});
        end
        
        function delete(varargin)
           delete@handle(varargin{:});
        end
        
        function Hmatch = findobj(varargin)
           Hmatch = findobj@handle(varargin{:});
        end
        
        function p = findprop(varargin)
           p = findprop@handle(varargin{:});
        end
        
        function TF = eq(varargin)
           TF = eq@handle(varargin{:});
        end
        
        function TF = ne(varargin)
           TF = ne@handle(varargin{:});
        end
        
        function TF = lt(varargin)
           TF = lt@handle(varargin{:});
        end
        
        function TF = le(varargin)
           TF = le@handle(varargin{:});
        end
        
        function TF = gt(varargin)
           TF = gt@handle(varargin{:});
        end
        
        function TF = ge(varargin)
           TF = ge@handle(varargin{:});
        end
    end
end
