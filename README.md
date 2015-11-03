# FRCDriverStation
Playing around with the 2015 FRC Driver Station protocol

Currently just work in progress Lua Wireshark dissectors.

[See wiki](https://github.com/jcreigh/FRCDriverStation/wiki) for description of the protocol known thus far.

### Usage

These dissectors (of course) require Lua to be enabled in your Wireshark installation.

To use without installing, use `-X lua_script:FRC_Dissectors.lua` on the command line.

To install, place the FRC_Dissectors directory into Wireshark's plugin directory, such as `~/.wireshark/plugins`
