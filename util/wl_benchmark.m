%==============================================================================
% Function wl_benchmark()
%
% Usage:  
%     - results = wl_benchmark( nodes )                                                             - Interactive mode
%     - results = wl_benchmark( nodes, 'read_iq', num_trials, num_samples_per_read, num_buffers )   - Read IQ benchmark
%     - results = wl_benchmark( nodes, 'write_iq', num_trials, num_samples_per_write, num_buffers ) - Write IQ benchmark
%
% NOTE:  Can be used on a single node or an array of nodes
% NOTE:  If no output arguments are specified, then the function just prints the benchmark data
%
% Output:
%     - Pretty print of benchmark info
%     - Array of benchmark info (if the number of output arguments is 1)
% 
%==============================================================================

function data = wl_benchmark(varargin)
    interactive_mode = 0;
    benchmark        = 99;
    num_rx_samples   = 0;                   % Default to zero, set to the max samples per the node Read IQ length
    num_tx_samples   = 0;                   % Default to zero, set to the max samples per the node Write IQ length
    num_trials       = 1000;                % Default to 1000
    num_buffers      = 1;
    
    
    %--------------------------------------------------------------------------
    % Get inputs 
    %    
    if (nargin == 0)
        fprintf('Usage wl_benchmark:\n');
        fprintf('  1. Interactive Mode:  \n');
        fprintf('       results = wl_benchmark( nodes ) \n');
        fprintf('  2. Batch Mode: \n');
        fprintf('       results = wl_benchmark( nodes, "benchmark", num_trials, num_samples_per_trial, num_buffers ) \n');
        fprintf('       where \n');
        fprintf('         benchmark             is: [read_iq, write_iq]   \n');
        fprintf('         num_trials            is: Number of trials to conduct for the benchmark\n');
        fprintf('         num_samples_per_trial is: Number (1 ... Max IQ length) or "max" \n\n');
        fprintf('         num_buffers           is: Number of RF buffers to request ([1..4] depending on the node capabilities)\n\n');
        error('Not enough arguments are provided');

    elseif (nargin == 1)
        interactive_mode = 1;
        
        switch( class(varargin{1}) ) 
            case 'wl_node'
                nodes    = varargin{1};
                numNodes = length(nodes);
            otherwise
                error('Unknown argument.  Argument is of type "%s", need "wl_node"', class(varargin{1}));
        end        

    elseif ((nargin == 4) || (nargin == 5))
        switch( class(varargin{1}) ) 
            case 'wl_node'
                nodes    = varargin{1};
                numNodes = length(nodes);
            otherwise
                error('Unknown argument.  Argument is of type "%s", need "wl_node"', class(varargin{1}));
        end        

        switch( class(varargin{2}) ) 
            case 'char'
                func     = lower( varargin{2} );
                
                switch( func )
                    case 'read_iq'
                        benchmark = 1;
                    case 'write_iq'
                        benchmark = 2;
                    otherwise
                        fprintf('Unknown Argument.  Currently supported benchmarks are: \n');
                        fprintf('    read_iq \n');
                        fprintf('    write_iq \n');
                        error('Unknown benchmark function: %s \n', varargin{1} );
                end
            otherwise
                error('Unknown argument.  Argument is of type "%s", need "string"', class(varargin{2}));
        end
        
        switch( class(varargin{3}) ) 
            case 'double'
                num_trials = varargin{3};
                
                if ( num_trials <= 0 )
                    num_trials = 1;
                end

            otherwise
                error('Unknown argument.  Argument is of type "%s", need "double"', class(varargin{3}));
        end        

        switch( class(varargin{4}) ) 
            case 'double'
                num_rx_samples = varargin{4};
                num_tx_samples = varargin{4};
                
                % Error checking for num_*_samples is done below on a per node basis

            case 'char'
                if( strcmp( 'max', lower( varargin{4} ) ) )
                    num_rx_samples = 0;
                    num_tx_samples = 0;
                else
                    fprintf('Unknown Argument.  Number of Samples currently supports:  \n');
                    fprintf('    Number  - (ie 2^15) \n');
                    fprintf('    "max"   - Set the number of samples to the maximum number of samples supported by the board \n');
                    error('Unknown argument: %s \n', varargin{4} );
                end

                % Error checking for num_*_samples is done below on a per node basis

            otherwise
                error('Unknown argument.  Argument is of type "%s", need "double" or "char"', class(varargin{4}));
        end

        % Default to 1 buffer if num_buffers is not specified        
        if (nargin == 5) 
            switch( class(varargin{5}) ) 
                case 'double'
                    num_buffers = varargin{5};
                    
                    if ( num_buffers <= 0 )
                        num_buffers = 1;
                    end

                otherwise
                    error('Unknown argument.  Argument is of type "%s", need "double"', class(varargin{5}));
            end
        else
            num_buffers = 1;
        end
    else
        error('Arguments incorrect.  Please see usage.');
    end

    

    if ( interactive_mode ) 

        fprintf('------------------------------------------------------------\n');
        fprintf('Please select benchmark to run:\n');
        fprintf('    [ 1] Read IQ \n');
        fprintf('    [ 2] Write IQ \n');
        fprintf('    [99] Quit       (default) \n');

        temp = input('Enter benchmark number: ','s');
        if(isempty(temp))
            temp = benchmark;
            fprintf('   quitting.\n');
            return
        else
            fprintf('   running test %s\n',temp); 
            benchmark = str2num( temp );
        end

        %----------------------------------------------------------------------
        % Specific Arguments for each benchmark 
        %
        switch( benchmark ) 
        
            %------------------------------------------------------------------
            % Read IQ Benchmark
            %
            case 1
                fprintf('------------------------------------------------------------\n');
                temp = input('Please enter the number of samples (default = Read IQ length of board): ','s');
                if(isempty(temp))
                    fprintf('   defaulting to Read IQ length of the board \n');
                else
                    fprintf('   setting to %s (will be truncated to Read IQ length of the board)\n',temp); 
                    num_rx_samples = str2num( temp );
                end

            %------------------------------------------------------------------
            % Write IQ Benchmark
            %
            case 2
                fprintf('------------------------------------------------------------\n');
                temp = input('Please enter the number of samples (default = Write IQ length of board): ','s');
                if(isempty(temp))
                    fprintf('   defaulting to Write IQ length of the board \n');
                else
                    fprintf('   setting to %s (will be truncated to Write IQ length of the board)\n',temp); 
                    num_tx_samples = str2num( temp );
                end
            
            %------------------------------------------------------------------
            % Default case
            %
            otherwise
                fprintf('Invalid benchmark selection.  Quitting.  \n');
                return
                
        end

        %----------------------------------------------------------------------
        % Common Arguments for all benchmarks
        %
        fprintf('------------------------------------------------------------\n');
        temp = input('Please enter the number of trials (default = 1000): ','s');
        if(isempty(temp))
            temp = sprintf('%d', num_trials);
            fprintf('   defaulting to %s \n', temp);
        else
            num_trials = str2num( temp );
            
            if ( num_trials <= 0 )
                num_trials = 1;
                fprintf('   setting to 1 \n'); 
            else
                fprintf('   setting to %s \n',temp); 
            end            
        end
    
    end  % END interactive_mode
    
    
    
    %--------------------------------------------------------------------------
    % Run the benchmark
    %
    
    fprintf('------------------------------------------------------------\n');
    
    results  = [];
    RxLength = num_rx_samples;
    TxLength = num_tx_samples;

    for n = 1:1:numNodes
        currNode = nodes(n);
        
        % Set the severity of the sequence number matching to IGNORE
        curr_severity = currNode.baseband.seq_num_match_severity;
        currNode.baseband.seq_num_match_severity = currNode.baseband.SEQ_NUM_MATCH_IGNORE;

        %----------------------------------------------------------------------
        % Common Parameters for all benchmarks
        %        
        if(currNode.hwVer == 3)
            SN = sprintf('W3-a-%05d',currNode.serialNumber);
        else
            SN = 'Serial Number N/A';
        end
        
        ifc_ids        = currNode.wl_getInterfaceIDs();
        maximum_rx_len = wl_basebandCmd(currNode, ifc_ids.RF_A, 'rx_buff_max_num_samples');

        if ( ( RxLength == 0 ) || ( RxLength > maximum_rx_len ) )
            RxLength       = maximum_rx_len;
            num_rx_samples = RxLength;
        end

        maximum_tx_len = wl_basebandCmd(currNode, ifc_ids.RF_A, 'tx_buff_max_num_samples');
        
        if ( ( TxLength == 0 ) || ( TxLength > maximum_tx_len ) )
            TxLength       = maximum_tx_len;
            num_tx_samples = TxLength;
        end
        
        % Set up the buffers to transfer        
        if (num_buffers <= currNode.num_interfaces)
            switch (num_buffers)
                case 1
                    RF_INF = [ifc_ids.RF_A];
                case 2
                    RF_INF = [ifc_ids.RF_A, ifc_ids.RF_B];
                case 3
                    RF_INF = [ifc_ids.RF_A, ifc_ids.RF_B, ifc_ids.RF_C];
                case 4
                    RF_INF = [ifc_ids.RF_A, ifc_ids.RF_B, ifc_ids.RF_C, ifc_ids.RF_D];
            end
        else
            error('Node does not support that many buffers');
        end        
        
        
        % Run Benchmark        
        % 
        % NOTE:  To get a look at the memory used during each benchmark, use the following
        %     commands to print memory usage:
        %         x = memory;
        %         fprintf('Memory Usage = %20d\n', x.MemUsedMATLAB);
        %
        switch( benchmark ) 
        
            %------------------------------------------------------------------
            % Read IQ Benchmark
            %
            case 1
               fprintf('Read IQ running on node %d of %d:  ID = %4s  Serial Number = %12s  Num samples = %10d  Num buffers = %5d\n', n, numNodes, sprintf('%d',currNode.ID), SN, RxLength, num_buffers);
                
                rx_time = zeros(num_trials, 1);

                i = num_trials;

                % Test TX performance of node
                while( i > 0 )
                    
                    xt = tic;
                    
                    rx_IQ        = wl_basebandCmd(currNode, RF_INF, 'read_IQ', 0, RxLength);                    
                    
                    rx_time(i,1) = toc(xt);
                    
                    % NOTE:  We clear the rx_IQ variable so that it is not maintained in memory during the 
                    %     next Read IQ, thereby doubling the memory required for the test.
                    clear rx_IQ;
                    
                    i = i - 1;
                end

                if ( num_trials > 5 ) 
                    rx_min_time = min(  rx_time(1:(num_trials - 5),:) );
                    rx_max_time = max(  rx_time(1:(num_trials - 5),:) );
                    rx_avg_time = mean( rx_time(1:(num_trials - 5),:) );
                else
                    rx_min_time = min(  rx_time(1:(num_trials),:) );
                    rx_max_time = max(  rx_time(1:(num_trials),:) );
                    rx_avg_time = mean( rx_time(1:(num_trials),:) );                
                end
            
                sec_ota_time(n) = RxLength / 40e6;                                       % Each sample is at 40 MHz
                sec_per_iq(n)   = rx_avg_time(1) / num_buffers;                          % Average IQ time over all trials (for a single buffer)
                bits_per_sec(n) = ((RxLength * num_buffers * 4 * 8) / rx_avg_time(1));   % Each sample is 4 bytes; 8 bits per byte
                
            
            %------------------------------------------------------------------
            % Write IQ Benchmark
            %
            case 2
                fprintf('Write IQ running on node %d of %d:  ID = %4s  Serial Number = %12s  Num samples = %10d  Num buffers = %5d\n', n, numNodes, sprintf('%d',currNode.ID), SN, TxLength, num_buffers );

                TxData  = zeros(TxLength, 1, 'double');
                tx_time = zeros(num_trials, 1);
    
                i = num_trials;
                                
                % Test RX performance of node
                while ( i > 0 )
                    xt = tic;
                    wl_basebandCmd(currNode, RF_INF, 'write_IQ', TxData);
                    tx_time(i,1) = toc(xt);
                    
                    i = i - 1;
                end

                if ( num_trials > 5 ) 
                    tx_min_time = min(  tx_time(1:(num_trials - 5),:) );
                    tx_max_time = max(  tx_time(1:(num_trials - 5),:) );
                    tx_avg_time = mean( tx_time(1:(num_trials - 5),:) );
                else
                    tx_min_time = min(  tx_time(1:(num_trials),:) );
                    tx_max_time = max(  tx_time(1:(num_trials),:) );
                    tx_avg_time = mean( tx_time(1:(num_trials),:) );
                end

                sec_ota_time(n) = TxLength / 40e6;                         % Each sample is at 40 MHz
                sec_per_iq(n)   = tx_avg_time(1);                          % Average IQ time over all trials
                bits_per_sec(n) = ((TxLength * 4 * 8) / tx_avg_time(1));   % Each sample is 4 bytes; 8 bits per byte
                
            
            %------------------------------------------------------------------
            % Default case
            %
            otherwise
                fprintf('Invalid benchmark selection.  Quitting.  \n');
                return
                
        end  % END switch( benchmark )
        
        % Restore the severity of the sequence number matching on the node
        currNode.baseband.seq_num_match_severity = curr_severity;
        
    end  % END for each node    


    %--------------------------------------------------------------------------
    % Print the benchmark results
    % 
    fprintf('------------------------------------------------------------\n');
    fprintf('Displaying results of %d nodes:\n\n', n);

    extraTitle = '';
    extraLine  = '';
    extraArgs  = '';
    
    switch( benchmark ) 

        %----------------------------------------------------------------------
        % Read IQ Benchmark
        %
        case 1
            extraTitle = '   Transport | MTU Size (B) | Num Samples | TX/RX Time (ms) | Num Trials | Avg Read  IQ Time (ms) | Avg Transfer Speed (Mbps) |';
            extraLine  = '-------------------------------------------------------------------------------------------------------------------------------';
        
        %----------------------------------------------------------------------
        % Write IQ Benchmark
        %
        case 2
            extraTitle = '   Transport | MTU Size (B) | Num Samples | TX/RX Time (ms) | Num Trials | Avg Write IQ Time (ms) | Avg Transfer Speed (Mbps) |';
            extraLine  = '-----------------------------------------------------------------------------------------------------------------------------';
        
        %----------------------------------------------------------------------
        % Default case
        %
        otherwise
            fprintf('Invalid benchmark selection.  Quitting.  \n');
            return
            
    end  % END switch( benchmark )


    fprintf('------------------------------%s\n',extraLine);
    fprintf('|  ID |  WLVER |    Serial # |%s\n',extraTitle);
    fprintf('------------------------------%s\n',extraLine);

    for n = 1:1:numNodes
        currNode = nodes(n);

        ID       = sprintf('%d', currNode.ID);
        WLVER    = sprintf('%d.%d.%d',currNode.wlVer_major,currNode.wlVer_minor,currNode.wlVer_revision);
        HWVER    = sprintf('%d', currNode.hwVer);
        MTU_SIZE = sprintf('%d', currNode.transport.getMaxPayload());
            
        if(currNode.hwVer == 3)
            SN = sprintf('W3-a-%05d',currNode.serialNumber);
        else
            SN = 'N/A';
        end
        
        if( strcmp( class(currNode.transport), 'wl_transport_eth_udp_mex' ) )
            TPORT = 'WL Mex UDP';
        elseif ( strcmp( class(currNode.transport), 'wl_transport_eth_udp_java' ) )
            TPORT = 'Java UDP';
        else
            TPORT = 'N/A';
        end


        switch( benchmark ) 

            %------------------------------------------------------------------
            % Read IQ Benchmark
            %
            case 1
                extraArgs = sprintf('%12s |%13s |%12d |%16.2f |%11d |%23.2f |%26.2f |',TPORT, MTU_SIZE, num_rx_samples, (sec_ota_time(n) * 1e3), num_trials, (sec_per_iq(n) * 1e3), (bits_per_sec(n) / 1e6) );
            
            %------------------------------------------------------------------
            % Write IQ Benchmark
            %
            case 2
                extraArgs = sprintf('%12s |%13s |%12d |%16.2f |%11d |%23.2f |%26.2f |',TPORT, MTU_SIZE, num_tx_samples, (sec_ota_time(n) * 1e3), num_trials, (sec_per_iq(n) * 1e3), (bits_per_sec(n) / 1e6) );
            
            %------------------------------------------------------------------
            % Default case
            %
            otherwise
                fprintf('Invalid benchmark selection.  Quitting.  \n');
                return
                
        end  % END switch( benchmark )
                
        fprintf('|%4s |%7s |%12s |%s\n',ID,WLVER,SN,extraArgs);
        fprintf('------------------------------%s\n',extraLine);
    end

    fprintf('\n');

    
    %--------------------------------------------------------------------------
    % Set outputs
    % 
    if nargout == 0
        return
    elseif nargout == 1
        data = results;
    else
        error('Too many output arguments provided');    
    end
    
end



