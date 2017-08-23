WARPLab 7 was built with user extensions in mind. This directory serves as an example of how to extend
the capabilities of WARPLab without having to directly modify the existing framework.

This directory contains two files: (a) user_extension_example_class.m and (b) user_extension_script.m

(a) user_extension_example_class.m
	This file is an example user extension object that implements two new kinds of WARPLab commands
	(commands for writing and reading the EEPROM located on the WARP board). To add these commands,
	nothing inside the rest of the M_CODE_REFERENCE folder that contains the WARPLab framework was 
	modified. This allows user extensions to seamlessly attach to new versions of WARPLab as they are
	released.
	
(b) user_extension_script.m
	This file is an example script that knows how to use the user extension example class. It first will
	read from the EEPROM and print any string that is stored there. It then overwrites that part of the EEPROM
	with a new string. After written, the board can be unplugged and reconfigured. Despite the power loss, 
	the script is able to read the stored EEPROM string.