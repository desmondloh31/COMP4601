# Kria KV260 Resources for COMP3601 24T4 

This directory contains everything you need to get started with the KV260.

## sd_card_image

This contains a compressed .wic file which can be loaded to an SD card for the KV260 using e.g. `dd`.
It has a number of custom extra features including the COMP3601 default memory map, as well as some
pre-loaded designs and programs. It is self-hosted, meaning it also contains gcc so it can self-compile
software programs.

## constraints_file

This contains the .xdc constraints file used in Vivado for making designs for the Kria KV260. It maps
the pins in the PMOD connector to our custom PMOD board which has an LED and a push button, as well as
the four I2S ports (two for microphones, two for speaker/amplifiers).

This file is called `kria-constraints.xdc`.

## lab1_blink_led

This is the "beginner" lab, which provides instructions on how to blink an LED on the KV260 using
custom hardware. It also covers loading and unloading hardware and connecting to the Kria via a UART terminal.

## lab2_axi_led

This is the "advanced" lab, which provides instructions on finishing an AXI memory-mapped GPIO module which
can output to the LED and read from the button on the expansion PMOD board.

This directory also contains the hardware source code for lab2:
- `gpio_axi.vhd`: the unfinished implementation of the AXI memory-mapped GPIO module.

## comp3601_project_quickstart

This is the follow-on from the two labs and provides instructions on how to get started building the COMP3601
project - an audio peripheral / I2S bus master over AXI.

This directory also contains all the hardware source code in VHDL for the baseline project.
- `audio_pipeline.vhd`: the top module that wraps all the subcomponents around
- `ctrl_bus.vhd`: an AXI lite implementation that has 8 registers on the control bus
- `fifo.vhd`: An overruning FIFO implementation where writing to the FIFO when it is full is not prohibited and pushs the read pointer further so that reading the FIFO always gets the latest samples.
- `i2s_master.vhd`: An implementation of the I2S master that drives the I2S MEMs microphone. Pushes the audio into the overruning FIFO.
- `params.vhd`: Library package that currently consists of the components declaration.


## software_example_code/app

The app directory contains the software example source code for using the FPGA hardware.
- `audio_i2s.c/h`: Driver for the control bus of the audio_pipeline module. It memory maps the hardware address of the device to the virtual memory space for userspace software to access. The address can be found in the Vivado project once you have the block design configured. It will be next to the diagrams tab.
- `axi_dma.c/h`: Driver for controlling the AXI DMA for transferring data from the FPGA to software. It does not support scatter-gather mode and only implements the FPGA to SW (S2MM) data path since the current implementation does not transfer data from software to FPGA.
- `main.c`: Example main file that uses the above two drivers and transfers a few frames of audio sample data.

## software_example_code/bin

This directory contains two example app binaries that have been build and provided as a testing tool for the baseline project.
- `control_bus_test` tests the software configuration of the control bus of the audio_pipeline module. At the moment there are 8 registers implemented in the baseline project with register 1 and register 2 being read only. The binary prints out the values before being written and tries to write 0xDEADBEEF to all the registers and prints them out again.
- `sample256` is an example app that transfers 10 frames of 256 samples and prints the value out to the console. The samples will be 18-bit wide while the rest (31th bit to 18th) will be sample sequence numbers and should be an incrementation. Note that between frames currently there is a value mismatch of 5.

## sdk

This contains `sdk.sh`, the cross-compilation SDK installer which is a self-extracting installer. Follow the documentation sent to set this up if you would like to cross-compile software which will run natively on the Kria KV260.
