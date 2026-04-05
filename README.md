# STM32F411_TFT_OLED_ADXL355_UART

A personal embedded systems demonstration project built on the **STM32F411** to showcase practical integration of multiple peripherals, sensors, displays, timers, ADC, DMA, PWM, RTC, and UART communication in a single bare-metal application.

This project is not meant to be just a simple sensor readout demo. It is a hands-on demonstration of my ability to design, connect, initialize, manage, and coordinate multiple embedded peripherals together in one working system.

## Project Overview

This project combines several hardware modules and firmware features into one embedded application:

- **STM32F411** microcontroller
- **ILI9341 320x240 TFT display** over SPI  
  - Touch is **not used**
- **SSD1306 128x64 OLED display** over I2C
- **20x4 I2C LCD**
- **ADXL355 accelerometer**
- **BME280 environmental sensor**
- **ADC with DMA**
- **PWM output for LEDs**
- **RTC for date and time**
- **UART logging / serial monitoring**
- **Timer interrupt based scheduling**
- **Status LEDs and push button interaction**

The main purpose of this project is to demonstrate how multiple sensors, displays, and peripherals can work together reliably in a single STM32 firmware application.

---

## Why I Built This Project

I created this project as a **personal demonstration of my embedded systems knowledge** and my ability to integrate multiple peripherals into one structured application.

It helped me practice and demonstrate:

- peripheral initialization and coordination
- sensor interfacing
- display handling on multiple screens
- ADC + DMA usage
- PWM control
- timer-based task scheduling
- RTC handling
- UART data formatting and transmission
- basic fault recovery techniques such as I2C bus recovery

This project reflects the type of practical system integration work often required in real embedded product development.

---

## Main Features

### 1. Multi-Display Output
The system uses three different displays together:

#### TFT Display (ILI9341 - 320x240)
Used as the main live status display for:
- temperature
- humidity
- pressure
- live RAM information
- date
- time

#### OLED Display (SSD1306 - 128x64)
Used for:
- startup test display
- date display
- time display

#### 20x4 I2C LCD
Used for compact live monitoring of:
- temperature
- humidity
- pressure
- ADC channel reading
- converted voltage
- system timing information

---

### 2. BME280 Environmental Monitoring
The BME280 sensor is used to read:
- temperature
- humidity
- pressure

These values are displayed on the LCD and TFT, and are also transmitted over UART.

---

### 3. ADXL355-Based Motion / Tilt Demonstration
The ADXL355 is used as part of the motion/tilt demonstration in this project.

Its tilt/orientation data is used to control **PWM outputs**, which are connected to **LEDs**.  
This creates a visual demonstration where the LED brightness changes based on sensor movement/tilt.

This helps show:
- analog/digital sensor integration
- real-time response to sensor input
- timer/PWM control
- interactive embedded behavior

---

### 4. ADC + DMA
The firmware uses **ADC with DMA** to continuously sample analog channels.

This allows sensor/input data to be collected efficiently without repeatedly restarting ADC conversions in software.

The sampled ADC values are:
- processed in the main loop
- converted to millivolts
- displayed on the LCD
- sent over UART

---

### 5. PWM LED Control
Three PWM outputs are generated using STM32 timers and are connected to LEDs.

The PWM duty cycle is updated based on the tilt-related sensor data, demonstrating:
- timer configuration
- PWM generation
- analog-style LED brightness control
- sensor-to-actuator interaction

---

### 6. UART Monitoring and Data Output
UART is used for debugging and runtime monitoring.

The project sends:
- ADC data
- BME280 readings
- memory usage information
- formatted serial data packets

This makes the project easier to observe and debug during runtime and also demonstrates structured serial output formatting.

---

### 7. RTC Date and Time Display
The RTC is used to maintain date and time information.

The current date and time are displayed on:
- OLED
- TFT

This adds another real embedded subsystem into the project and shows how time-based data can be integrated into a live application.

---

### 8. Timer-Based Scheduling
The project uses a **timer interrupt driven approach** for periodic updates.

Instead of using an RTOS, the current version follows a **bare-metal structure** where timed events are managed using timer flags and the main loop.

This is useful for demonstrating:
- super-loop architecture
- periodic task execution
- interrupt-driven scheduling
- embedded timing coordination

---

### 9. Status LEDs and Threshold Indication
The project also uses onboard/external LEDs as status indicators.

Examples include:
- heartbeat indication
- environmental threshold alerts
- PWM brightness response
- activity indication during recovery/re-initialization events

---

### 10. I2C Bus Recovery Logic
An I2C bus recovery routine is included to help recover from a stuck I2C bus condition.

This demonstrates awareness of practical embedded reliability issues and shows an effort to make the system more robust.

---

## How the Project Works

At startup, the firmware initializes all major peripherals:

- GPIO
- DMA
- I2C
- SPI
- UART
- ADC
- RTC
- timers
- displays
- sensors

After initialization:

1. The OLED performs a quick startup display test.
2. The TFT is initialized and used as the main visual output screen.
3. The LCD is prepared for live compact status updates.
4. ADC + DMA begins continuous sampling.
5. PWM outputs are started.
6. The motion sensor and environmental sensor begin feeding live data into the application.
7. Timer interrupts create periodic update flags.
8. The main loop continuously handles display updates, sensor reading, LED control, UART output, and system monitoring.

In normal operation, the project:
- reads environmental data from the BME280
- processes ADC values
- updates date/time from RTC
- refreshes the OLED, TFT, and LCD
- adjusts PWM duty cycle for LEDs
- sends diagnostics and sensor data over UART
- indicates status with LEDs

---

## What This Project Demonstrates

This project demonstrates my practical ability to work with:

- **STM32 bare-metal embedded firmware**
- **multiple communication interfaces at once**
  - I2C
  - SPI
  - UART
  - ADC
- **sensor integration**
- **display drivers**
- **timer and PWM configuration**
- **DMA-based acquisition**
- **system diagnostics and debugging**
- **real hardware interaction and coordination**

More importantly, it demonstrates how I approach building a project where multiple independent modules must all work together in one application.

---

## Hardware Used

- STM32F411 development board
- ILI9341 320x240 TFT display
- SSD1306 128x64 OLED display
- 20x4 I2C LCD
- ADXL355 accelerometer
- BME280 sensor
- LEDs for PWM output indication
- push buttons / switches
- UART terminal connection for serial monitoring

---

## Software / Firmware Concepts Used

- STM32 HAL
- bare-metal embedded architecture
- timer interrupt callbacks
- PWM generation
- ADC scan mode
- ADC DMA transfer
- I2C peripheral communication
- SPI display communication
- UART transmission
- RTC date/time formatting
- buffer-based display and UART string handling

---

## Libraries and References Used

This project was built with the help of open-source libraries, examples, and reference repositories that I used for learning, understanding implementation details, and integrating peripherals more effectively.

### Display Libraries
- **ILI9341 TFT Library / Reference**  
  https://github.com/afiskon/stm32-ili9341/blob/master/Lib/ili9341/ili9341.c

- **SSD1306 OLED Library**  
  https://github.com/afiskon/stm32-ssd1306

### Sensor Libraries / References
- **ADXL335 Reference Library**  
  https://github.com/Seeed-Studio/Accelerometer_ADXL335

- **BME280 STM32 HAL Library**  
  https://github.com/eziya/STM32_HAL_BME280

I appreciate the work of the original authors and maintainers of these libraries and references. Their open-source contributions were very helpful in learning, testing, and building this project.

---

## Open Source Note

This project is shared as a personal learning and demonstration project, and it is intended to be **free to use, study, modify, and build upon**.

If you use this project, improve it, or adapt parts of it for your own work, that is absolutely welcome.

At the same time, please also respect the original licenses of any third-party libraries, drivers, or examples used inside this project.  
The external libraries and references listed above remain the work of their respective authors and are governed by their own licenses.

This repository is shared in the spirit of open-source collaboration, learning, and knowledge sharing.

---

## License

This project is intended to be released as an open-source project.

A separate `LICENSE` file should be included in this repository to clearly define the legal terms for use, modification, and distribution.  
For a simple and developer-friendly choice, the **MIT License** is a good option.

Until then, please treat this repository as a public learning and demonstration project and make sure to preserve credit to original third-party sources where required.

---

## Purpose of This Repository

This repository is mainly intended as a **portfolio / demonstration project**.

It shows my personal work in:
- integrating several embedded peripherals together
- testing my understanding of real STM32 firmware development
- building a practical multi-module embedded application
- demonstrating structured and scalable firmware design

---

## Future Improvements

I am currently planning the next stage of this work.

### Planned future update
I am working on adding an **additional STM32F103** connected through **UART** to perform additional actions in a future version of the project.

### Next project direction
The current version is built in a **bare-metal** style.  
In the future, I plan to redesign and extend this project into a **real-time RTOS-based version**.

That next version will focus on:
- task-based architecture
- better modularity
- improved scheduling
- more scalable real-time behavior
- inter-controller communication

Once that new project is ready, this README will be updated with a **new repository link** that will direct readers to the next-generation version of this system.

---

## Notes

- The current project is a **personal demonstration project**
- The TFT touch feature is **not used**
- The project is designed to show **integration ability**, not only a single-function demo
- Future versions will continue expanding this platform into a more advanced and real-time architecture

---

## Closing

This project represents my effort to move beyond isolated examples and build a more complete embedded system where multiple peripherals, sensors, displays, and outputs work together in one application.

It is a demonstration of both learning and implementation, and it reflects the kind of hands-on embedded development work I enjoy doing.
