function wl_cmd_doc(cmd_str)
%WL_CMD_DOC WARPLab Command Documentation Printer
%This function prints documentation for individual commands based on the 
%command string provided as the argument.
%
% Example:
% wl_cmd_doc('write_IQ')

[wl_path, dummy_var0, dummy_var1] = fileparts(which('wl_node'));

file_list = what(wl_path);
files = {file_list.m{:}, file_list.classes{:}};

found_cmd_glbl = 0;
CR = char(10);
TAB = char(9);
for n = 1:length(files)
    procCmd_line = 0;
    cmd_line = 0;
    found_cmd = 0;
    cur_file = files{n};
    fid = fopen(cur_file);

    lnum = 0;
    res = [];

    %Find the 'function out=procCmd' section of the class
    while (procCmd_line == 0)
        line = fgetl(fid);
        lnum = lnum + 1;
        if(line == -1)
            %EOF - punt
            break;
        end
        
        procCmd_ind = regexp(line, '^[ \t]*function.*procCmd', 'once');
        if(~isempty(procCmd_ind))
            %Found the procCmd function
            procCmd_line = lnum;
        end
    end
    
    %Check for a case block for the requested command
    % This loop is skipped if no procCmd function was found
    while (procCmd_line > 0 && cmd_line == 0)
        line = fgetl(fid);
        lnum = lnum + 1;

        if(line == -1)
            %EOF - punt
            break;
        end
        
        %Check if we've reached the end of the procCmd switch/case
        endcase_ind = regexp(line, '^[ \t]*otherwise', 'once');
        if(~isempty(endcase_ind))
            %End of procCmd; punt
            break;
        end
        
        %Check if this line matches the requested command string
        cmd_ind = regexpi(line, ['^[ \t]*case[ \t]+''' cmd_str ''], 'once');

        if(~isempty(cmd_ind))
            %Found the command; retrieve all following lines until a non-comment is found
            found_cmd = 1;
            cmd_line = lnum;
            while 1
                line = fgetl(fid);
                txt_ind = regexp(line, '^[ \t]*%', 'end', 'once');
                if(~isempty(txt_ind))
                    res = [res CR TAB line(txt_ind+1:end)];
                else
                    break;
                end
            end
        end
    end

    if(found_cmd)
        fprintf('\rCommand ''%s'' found in %s (line %d):', lower(cmd_str), cur_file, cmd_line)
        fprintf('%s\r\r', res)
    end

    fclose(fid);
    found_cmd_glbl = found_cmd_glbl || found_cmd;
end

if(~found_cmd_glbl)
    fprintf('\r\tCommand %s not found\r\r', cmd_str)
end
