## Synopsis

This software is a command line utility to upload firmware using USB as interface. It is based on work of T. Fischl (www.fischl.de).

## Motivation

The usbBootloaderCommander allows firmware updates without the need for additional special purpose hardware (AVR ISP programmer).

## OS Selection

The git **libusb-1.0** branch is set up to compile a windows executable.

## Installation

On Windows platform, a libusb filter-driver must be installed for the microcontroller. The simplest way is to install Zadig WinUSB driver. Instructions and software are found on zadig.akeo.ie
Note that the microcontroller has to be attached and in bootloader mode during driver installation.    

## Usage

    usbBootloaderCommander firmware.hex

## Tests

The device looks for an USB device *AVRUSBBoot* with VID / PID : 0x16c0/0x5dc.
To test communication with bootloader, just run `usbBooloaderCommander notexistingfile.hex`.
If the error is *file could not be opened* instead of *device not found*, then communication works.

## Contributors

An issue tracker is available in case of problems.

## License

License is GPL v2

