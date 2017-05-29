# EspAccontroller
The hardware is an Esp-12 with an IR-sender on Gpio5 and a DS18B20 temperature sensor on Gpio13.
The main function is to send a code by IR to a portable AC to put it on or off at daytime.
It uses time info from the network which a Node Red-server sends as a Mqtt message. 
You can also set the AC on or off by sending "on" or "off" to the topic "accontrol".
The status of the AC is sent as the topic "accontrollerPower".
The temperature read by the temp senor is sent with the topic "accontroller".
