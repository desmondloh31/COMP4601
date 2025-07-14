## REMOVE CLK CONSTRAINTS ##
## rst_n signal
set_property PACKAGE_PIN B10 [get_ports rst_n]
set_property IOSTANDARD LVCMOS33 [get_ports rst_n]

## init signal
set_property PACKAGE_PIN E10 [get_ports init]
set_property IOSTANDARD LVCMOS33 [get_ports init]

## go signal
set_property PACKAGE_PIN E12 [get_ports go]
set_property IOSTANDARD LVCMOS33 [get_ports go]

## absorb signal
set_property PACKAGE_PIN D10 [get_ports absorb]
set_property IOSTANDARD LVCMOS33 [get_ports absorb]

## squeeze signal
set_property PACKAGE_PIN D11 [get_ports squeeze]
set_property IOSTANDARD LVCMOS33 [get_ports squeeze]

## ready output
set_property PACKAGE_PIN C11 [get_ports ready]
set_property IOSTANDARD LVCMOS33 [get_ports ready]
