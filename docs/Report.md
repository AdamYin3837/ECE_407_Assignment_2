# ECE 407: Assignment 2 - Custom IMU Chip Emulator

### Group Name/Number
* **Name:** Adam Yin
* **Group:** Group0x05

### Purpose
The purpose of this assignment is to create a custom simulated chip within the Wokwi environment that emulates a reduced version of the MPU-6000 Motion Tracking Device (only providing X, Y, and Z accelerometer data). The core objective is to demonstrate the ability to implement and communicate via both I2C and 3-wire SPI protocols.

### Introduction
The custom chip developed for this assignment acts as a slave device that mimics the behavior of an MPU-6000 Inertial Measurement Unit. Rather than implementing the full register map, this emulator specifically targets the accelerometer data registers (`0x3B` through `0x40`). It responds to a Raspberry Pi Pico master device, returning 16-bit dummy data for the X, Y, and Z axes over both I2C and SPI buses.

### Assignment Specific Details
#### 1. Chip Architecture
The custom chip was written in C using Wokwi's Custom Chip API. It maintains an internal state machine using a `chip_state_t` struct to track register addresses and bus states. 
* **Dummy Data:** The chip is pre-loaded with simulated accelerometer data: X=1000 (`0x03E8`), Y=-500 (`0xFE0C`), and Z=16384 (`0x4000`).
* **Register Auto-Increment:** Standard burst-read behavior was implemented, allowing the master to read all 6 bytes sequentially in a single transaction.

#### 2. I2C Implementation
The I2C interface listens on address `0x68`. Wokwi's `on_i2c_read` and `on_i2c_write` callbacks were utilized to parse the requested register address and return the appropriate byte from the dummy data array.

#### 3. SPI (3-Wire) Implementation
To satisfy the 3-wire SPI requirement, the custom chip maps both the MOSI and MISO internal Wokwi API hooks to a single bi-directional `SDIO` pin. 
* The chip utilizes Wokwi's `pin_watch` API on the Chip Select (CS) pin to start and stop the `spi_start()` listening buffer.
* The `on_spi_done` callback interprets the first byte as a command byte (parsing the MSB for read/write and lower 7 bits for the register address) and subsequently fills the transfer buffer with the requested dummy data on following clock cycles.

#### 4. Pi Pico Master Tests
To prove the custom chip functions correctly, two separate `main.c` test scripts were executed on the Raspberry Pi Pico.
* **I2C Test:** The Pico configured its I2C pins with pull-ups, pinged address `0x68`, wrote register `0x3B`, and requested 6 bytes.
* **SPI Test:** The Pico configured its SPI pins, tying both MOSI and MISO to the custom chip's SDIO pin. It sent the command byte `0xBB` (Read register `0x3B`) and sequentially read 6 bytes.

In both tests, the Pico successfully retrieved the bytes, bit-shifted them back into 16-bit integers, and printed the expected simulated values to the serial monitor.