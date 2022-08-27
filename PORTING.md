# DFM17 Porting progress
The RS41 and the DFM are similar in terms of hardware, they both use a U-blox GPS, a STM32F100 processor and a Silicon Labs EZRadioPRO radio IC.

### Hardware Table

| Module           | [DFM17](https://wiki.recessim.com/view/DFM-17_Radiosonde) | [RS41](https://github.com/bazjo/RS41_Hardware) |
|------------------|-----------------------------------------------------------|------------------------------------------------|
| Processor        | STM32F100R8                                               | STM32F100C8                                    |
| Radio            | Si4063                                                    | Si4032                                         |
| GPS              | MAX-M8C-0                                                 | UBX-G6010                                      |
| SWD Interface    | Internal solder pads                                      | External pins                                  |
| Serial Interface | External *USB connector* &ast;                            | External pins                                  |

<sub>&ast; DFM USB connector implements a 3.3v serial interface with a 5v vcc line, you can use a conventional USB cable to power the sonde without serial. [Instructions](https://gist.github.com/MrARM/b09b71e6d3cb35c113658e19d755f0f5)<sub>

### Porting progress
This is a simple check list of everything I have set out to do to try to get RS41ng to run on the DFM, this list will update with time, but these goals are set out to get results.

During each step, code will be commented out until that step has been reached, as I don't want to try to interface with hardware that might be on different pins and fry something in the process.

* [X] Retarget build to the different processor
* [X] Get serial output to show up on the DFM system
* [ ] Get the radio to transmit a CW signal
* [X] Get GPS to acquire and display a fix
* [ ] Get the radio to transmit APRS or other modes
* [ ] Get the radio to transmit out of original bands

Some items such as the sensor boon and other peripherals present will be implemented during or after these steps have been completed. These components will likely be desired for flights.