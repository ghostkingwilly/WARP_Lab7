% WARPLab MEX UDP Transport
%
%     This function is a custom MEX UDP transport layer for WARPLab communication.
%     It can be called with different options / commands to perform different 
%     functions.
%
% -----------------------------------------------------------------------------
% Authors:	Chris Hunter (chunter [at] mangocomm.com)
%			Patrick Murphy (murphpo [at] mangocomm.com)
%           Erik Welsh (welsh [at] mangocomm.com)
% License:	Copyright 2013, Mango Communications. All rights reserved.
%			Distributed under the WARP license  (http://warpproject.org/license)
% -----------------------------------------------------------------------------
% Command Syntax
%
% Standard WARPLab transport functions: 
%     1.                  wl_mex_udp_transport('version') 
%     2. index          = wl_mex_udp_transport('init_socket') 
%     3.                  wl_mex_udp_transport('set_so_timeout', index, timeout) 
%     4.                  wl_mex_udp_transport('set_send_buf_size', index, size) 
%     5. size           = wl_mex_udp_transport('get_send_buf_size', index) 
%     6.                  wl_mex_udp_transport('set_rcvd_buf_size', index, size) 
%     7. size           = wl_mex_udp_transport('get_rcvd_buf_size', index) 
%     8.                  wl_mex_udp_transport('close', index) 
%     9. size           = wl_mex_udp_transport('send', index, buffer, length, ip_addr, port) 
%    10. [size, buffer] = wl_mex_udp_transport('receive', index, length ) 
% 
% Additional WARPLab MEX UDP transport functions: 
%     1. [num_samples, cmds_used, samples]  = wl_mex_udp_transport('read_rssi' / 'read_iq', 
%                                                 index, buffer, length, ip_addr, port, 
%                                                 number_samples, buffer_id, start_sample) 
%     2. cmds_used                          = wl_mex_udp_transport('write_iq', 
%                                                 index, cmd_buffer, max_length, ip_addr, port, 
%                                                 number_samples, sample_buffer, buffer_id, 
%                                                 start_sample, num_pkts, max_samples, hw_ver) 
% 
% Please refer to comments within wl_mex_udp_transport.c for more information.
% 
% -----------------------------------------------------------------------------
% Compiling MEX
% 
% Please see the on-line documentation for how to compile WARPLab MEX functions at:
%    http://warpproject.org/trac/wiki/WARPLab/MEX
%
% The compile command within MATLAB is:
%
%    Windows:
%        mex -g -O wl_mex_udp_transport.c -lwsock32 -lKernel32 -DWIN32
%
%    MAC / Unix:
%        mex -g -O wl_mex_udp_transport.c
%

if ( ispc ) 

    % Allow both manual and automatic compilation on Windows
    %
    fprintf('WARPLab wl_mex_udp_transport Setup\n');
    fprintf('    Would you like [1] Manual (default) or [2] Automatic compilation?\n');
    temp = input('    Selection: ');

    if(isempty(temp))
        temp = 1;
    end

    if( ( temp > 2 ) || ( temp < 1 ) )
        temp = 1;
        fprintf('Invalid selection.  Setting to Manual.\n\n'); 
    end

else
    
    % Only allow manual compilation on MAC / UNIX
    %
    temp = 1; 
end

switch( temp ) 
    case 1

        fprintf('\nManual Compilation Steps:  \n');
        fprintf('    1) Please familiarize yourself with the process at: \n');
        fprintf('         http://warpproject.org/trac/wiki/WARPLab/MEX \n');
        fprintf('       to make sure your environment is set up properly. \n');
        fprintf('    2) Set up mex to choose your compiler.  (mex -setup) \n');
        
        if ( ispc )
            fprintf('         1. Select "Yes" to have mex locate installed compilers \n');
            fprintf('         2. Select "Microsoft Visual C++ 2010 Express" \n');
            fprintf('         3. Select "Yes" to verify your selections \n');
        else
            fprintf('         1. Select "/Applications/MATLAB_R2013a.app/bin/mexopts.sh : \n');
            fprintf('                    Template Options file for building MEX-files" \n');
            fprintf('         2. If necessary, overwrite:  /Users/<username>/.matlab/R2013a/mexopts.sh \n');
        end
        
        fprintf('    3) Change directory to M_Code_Reference/mex (cd mex) \n');
        fprintf('    4) Run the compile command: \n');
        
        if ( ispc )
            fprintf('         mex -g -O wl_mex_udp_transport.c -lwsock32 -lKernel32 -DWIN32 \n');
        else
            fprintf('         mex -g -O wl_mex_udp_transport.c \n');
        end
        
        fprintf('    5) Re-run wl_setup and make sure that "WARPLab Mex UDP" is an available transport. \n');
        fprintf('\n');
        
    case 2
        fprintf('\nRunning Automatic Setup procedure: ...');
        
        compiler_error = false;

        % Procedure to compile mex
        try 
            cc = mex.getCompilerConfigurations('c');
        catch ccError
            compiler_error = true;
        end

        if ( ~compiler_error )        
            fprintf('\nCompiling the WARPLab MEX UDP transport with compiler: \n    %s\n', cc.Name);
        
            % Since we are not guaranteed to be in the .../M_Code_Reference directory, 
            % save the current path and then cd using absolute paths
            %
            curr_path       = pwd;
            [wl_path,ig,ig] = fileparts(which('wl_mex_udp_transport'));
        
            cd(sprintf('%s',wl_path));
            
            if ( ispc )
                mex -g -O wl_mex_udp_transport.c -lwsock32 -lKernel32 -DWIN32
            else
                mex -g -O wl_mex_udp_transport.c
            end
            
            cd(sprintf('%s',curr_path));
            
            clear 
            fprintf('Done. \n\n');
            fprintf('***************************************************************\n');
            fprintf('***************************************************************\n');
            fprintf('Please re-run wl_setup to use your "WARPLab Mex UDP" transport.\n');
            fprintf('***************************************************************\n');
            fprintf('***************************************************************\n');
        else
            fprintf('\nMEX Compiler not found.  Running mex setup: \n');
            mex -setup
            
            try
                cc = mex.getCompilerConfigurations('c');
            catch ccError            
                error('Error with MEX compiler.\nSee http://warpproject.org/trac/wiki/WARPLab/MEX for more information');
            end
            
            fprintf('******************************************************************************\n');
            fprintf('******************************************************************************\n');
            fprintf('Please re-run wl_mex_udp_transport to compile the "WARPLab Mex UDP" transport.\n');
            fprintf('******************************************************************************\n');
            fprintf('******************************************************************************\n');
        end
    
    otherwise
        error('Invalid selection.');
end
