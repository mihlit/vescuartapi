# VescUartApi
This is VescUartApi library. It provides API to interface with VESC motor controllers through UART port.

Tested with Arduino Nano, Mini, Mega, STM32 Blue Pill, ESP32 Lolin D32

![screenshot](https://raw.githubusercontent.com/mihlit/vescuartapi/master/connection.png)

## Requirements

1) Make sure your VESC has updated firmware, as there were changes in the protocol. While some functions would still work, the main "get values" method returns data in different format and you would see a pretty much garbage. Use VESC-Tool for this [homepage](https://vesc-project.com/vesc_tool), [github](https://github.com/vedderb/vesc_tool)

2) Use VESC-Tool to configure VESC - section App Settings -> General -> APP to USE = UART or it won't talk over UART.

