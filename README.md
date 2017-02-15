# switcherduino

switcherduino is an Arduino sketch that can be flashed onto an [Arduino Pro Micro](https://www.sparkfun.com/products/12640) to turn it into a USB-serial transmitter for controlling Nexa and Maplin remote control plug sockets.

The only hardware components required in addition to the Arduino are a 433mhz RF transmitter (expensive example from [Sparkfun](https://www.sparkfun.com/products/10534) - cheaper available on AliExpress), and a 17.3cm length of wire to use as a simple antenna.

## Features

The transmitter currently has protocol support for [Nexa self-learning sockets](http://www.clasohlson.com/uk/Nexa-EYCR-250UK-Dimmable-Remote-Switch-Receiver/18-2653) (protocol shared with Home Easy and some others - not the Nexa sockets that use a house and unit code), and some inexpensive [Maplin sockets](http://www.maplin.co.uk/p/remote-controlled-mains-sockets-5-pack-n38hn). The protocol used by the Nexa sockets seems to be more resilient to interference on the crowded 433mhz band, so I'd strongly recommend using them over the Maplin ones. I stopped using my Maplin sockets in favour of Nexa ones for this reason.

When controlling Maplin sockets, any of the 16 sockets (4 buttons on 4 channels) can be set to on or off. When controlling Nexa sockets, any toggleable sockets can be set to on/off and any dimmable socket can be set to a dim level between 0 (off) and 15 (max).

## Usage

When connected via USB, the Arduino will appear to the USB host as a serial device. Opening a serial connection with it using a baud rate of 115200, 8 data bits, no parity, and 1 stop bit will allow you to send commands to the Arduino using the protocol specified in the section below.

## Serial protocol

A series of commands sent one after the other (concatenated) are sent to the Arduino, followed by a `\n` newline character - at this point, the commands will be executed sequentially. Command details:

### Set Nexa self-learning transmitter number

Example: `t67108863;`

Set the Nexa self-learning transmitter number. This can be used multiple times per command string, but only the last set number will be used when all of the commands are executed. This means that only devices that are paired with the same transmitter code can be used in one execution. Maximum transmitter code value is 67108863 (26 bits)

### Power Maplin socket on/off

Example: `m341`

Turn Maplin socket on channel 3, button 4 on. Format: m[channel][button][on/off]

### Power Nexa device on/off

Example: `n051`

Turn Nexa device 05 on. Pad device number with zeros if required. Format: n[device code 2 chars][on/off]

### Set Nexa device dim level

Example: `n13d10`

Set Nexa device 13 to dim level 10. Pad with zeros if required. Format: n[device code 2 chars]d[dim level 2 chars]

### Execute commands

i.e. `\n`

Execute the sequence of commands sent since the last `\n`

### Example

`t151632;n020n12d06m411\n`

This will use 151634 as the Nexa transmitter ID, and then turn Nexa device 2 off, Nexa device 12 dim to level 6, and turn Maplin socket channel 3 button 1 on.

## Credit

Most of the hard work for this project was done by David Edmundson, Martyn Henderson and Erik Simko for their work on the [Nexa transmitter](https://github.com/erix/NexaTransmitter), and somebody who's name I cannot currently find due to the [original source of the Maplin transmitter code](http://www.fanjita.org/serendipity/archives/53-Interfacing-with-radio-controlled-mains-sockets-part-2.html) being down.
