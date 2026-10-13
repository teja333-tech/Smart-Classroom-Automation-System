# Smart-Classroom-Automation-System
An STM32-based smart classroom automation system using sensors for automatic lighting, temperature monitoring, and fan control.
# Smart Classroom Automation System

An STM32-based embedded systems project that automates classroom lighting and temperature-based fan control using sensors. The system monitors environmental conditions, displays sensor readings on an LCD, and transmits data to an ESP32 through UART communication.

## Overview

The Smart Classroom Automation System aims to improve classroom convenience and energy efficiency through sensor-based automation. It uses an STM32 microcontroller to process sensor readings and control connected electrical devices according to predefined conditions.

The system monitors ambient light, temperature, and humidity. Sensor readings are displayed on an LCD, while UART communication enables data transmission to an ESP32.

## Features

* **Automatic Lighting:** Controls the classroom light according to the ambient brightness measured by an LDR sensor.
* **Temperature-Based Fan Control:** Controls a relay when the temperature exceeds a predefined threshold.
* **Temperature and Humidity Monitoring:** Uses a DHT11 sensor to obtain environmental readings.
* **LCD Display:** Displays temperature, humidity, and lighting information.
* **ESP32 Communication:** Transmits sensor readings and device states through UART.
* **Embedded Control:** Uses STM32 firmware to process sensor data and manage outputs.

## Hardware Components

* STM32 microcontroller
* DHT11 temperature and humidity sensor
* LDR light sensor
* ESP32 Wi-Fi module
* I2C LCD display
* Relay module
* Light/LED
* Fan
* Connecting wires and supporting circuit components

## Software and Technologies

* Embedded C
* STM32 HAL Library
* STM32CubeIDE
* STM32CubeMX
* UART communication
* I2C communication
* ADC (Analog-to-Digital Conversion)
* GPIO (General-Purpose Input/Output)

## Working Principle

1. The LDR sensor measures ambient light intensity, and the STM32 reads its analog output using the ADC.
2. When the calculated brightness falls below the configured threshold of 60%, the light output is activated.
3. The DHT11 sensor measures temperature and humidity.
4. When the measured temperature exceeds 35°C, the relay output is activated to control the connected fan.
5. The LCD displays the available temperature, humidity, and lighting information.
6. The STM32 transmits sensor readings and output states to the ESP32 through UART.
7. The system repeats these operations continuously.

## Project Structure

```text
Smart-Classroom-Automation-System/
├── README.md
├── main.c
├── images/
│   └── smart-classroom-device.jpg
└── docs/
    └── project-documentation.pdf



## Source Code

The repository includes the available STM32 firmware source code in `main.c`.

**Important:** The uploaded source file depends on STM32 HAL libraries, a `main.h` header file, and the associated microcontroller configuration and initialization files. The standalone source file is not a complete, independently buildable STM32CubeIDE project.

## Current Limitations

* The lighting and temperature thresholds are predefined in the firmware.
* Fan control is limited to ON/OFF operation.
* The available firmware does not implement a complete mobile application or cloud dashboard.
* Actual energy savings have not been quantified through energy-consumption measurements.
* The repository currently contains only the available firmware source file, not the complete STM32CubeIDE project.

## Future Improvements

* Develop a mobile or web-based monitoring dashboard.
* Integrate cloud connectivity and remote control.
* Add adjustable fan-speed control.
* Measure and analyze actual energy consumption.
* Extend the system to support multiple classrooms.
* Improve error handling and sensor calibration.

## Applications

* Smart classrooms
* Educational institutions
* Automated lighting systems
* Temperature-based appliance control
* IoT-based building automation prototypes

## Conclusion

The Smart Classroom Automation System demonstrates how embedded systems and sensors can be used to automate classroom lighting and temperature-based fan control. By combining an STM32 microcontroller, environmental sensors, an LCD, and ESP32 communication, the project provides a foundation for further development of intelligent classroom automation.
