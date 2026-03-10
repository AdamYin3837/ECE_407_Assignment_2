# ECE 407 Assignment 2: Wokwi Simulation Links

Below are the direct links to the live Wokwi simulations for the custom MPU-6000 emulator chip. I have separated them by protocol so they can be tested individually.

### 🔗 Project Links

* **[I2C Implementation Simulation](https://wokwi.com/projects/458059965716645889)**
  * **Description:** This simulation connects the Raspberry Pi Pico to the custom chip using the I2C protocol (SDA and SCL). The Pico runs the `main_i2c.c` script.

* **[SPI (3-Wire) Implementation Simulation](https://wokwi.com/projects/458054227729797121)**
  * **Description:** This simulation connects the Pico to the custom chip using a 3-wire SPI protocol. The Pico's MOSI and MISO pins are both routed to the chip's single bidirectional `SDIO` pin. The Pico runs the `main_spi.c` script.