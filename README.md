# switcherduino

switcherduino is an Arduino sketch that can be flashed onto an [Arduino Pro Micro](https://www.sparkfun.com/products/12640) to turn it into a USB-serial transmitter for controlling Nexa and Maplin remote control plug sockets.

The only hardware component required is a 433mhz RF transmitter (expensive example from [Sparkfun](https://www.sparkfun.com/products/10534) - cheaper available on AliExpress).

## Features

The transmitter currently has protocol support for Nexa sockets (protocol shared with Home Easy and some others), and some inexpensive [Maplin sockets](http://www.maplin.co.uk/p/remote-controlled-mains-sockets-5-pack-n38hn). The protocol used by the Nexa sockets seems to be more resilient to interference, so I'd strongly recommend using them over the Maplin ones.

When controlling Maplin sockets, any of the 16 sockets (4 buttons on 4 channels) can be controlled. When controlling Nexa sockets, any toggleable sockets can be turned on/off and any dimmable socket can be set to a dim level between 0 and 15.

## Serial protocol

These are some notes that I wrote up before; these are accurate but need reformatting for this documentation page.

> The device to control, and what we want to do with it (toggle power, or set dim level) is set in one command.
> Multiple commands can be strung together, and when a "\n" is received they'll all be executed rapidly in sequence.
> Each of these commands should be appended to each other with no gaps in no particular order.
>
> t67108863;
> Set the Nexa self-learning transmitter number. This can be used multiple times per command string, but only the last
> set number will be used when all of the commands are executed. This means that only devices that are paired with the
> same transmitter code can be used in one execution. Maximum transmitter code value is 67108863 (26 bits)
>
> m341
> Turn Maplin socket on channel 3, button 4 on. Format: m[channel][button][on/off]
>
> n051
> Turn Nexa device 05 on. Pad with zeros if required. Format: n[device code 2 chars][on/off]
>
> n13d10
> Set Nexa device 13 to dim level 10. Pad with zeros if required. Format: n[device code 2 chars]d[dim level 2 chars]
>
> \n
> Execute the sequence of commands
>
>
> Example:
> t151632;n020n12d06m411\n
