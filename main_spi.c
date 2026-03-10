#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define SPI_PORT spi0
#define PIN_MISO 16 // Pico pin connected to custom chip SDIO
#define PIN_CS   17 // Pico pin connected to custom chip CS
#define PIN_SCK  18 // Pico pin connected to custom chip SCLK
#define PIN_MOSI 19 // Pico pin connected to custom chip SDIO

#define ACCEL_START_REG 0x3B

int main() {
    // Initialize serial output
    stdio_init_all();
    sleep_ms(2000); // Give the serial monitor 2 seconds to open
    printf("Starting MPU-6000 Emulator Test via SPI...\n");

    // Initialize SPI hardware at 1 MHz
    spi_init(SPI_PORT, 1000 * 1000);
    
    // Assign pins to the SPI hardware function
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // Configure the Chip Select (CS) pin as a standard GPIO output
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1); // Set HIGH (idle state)

    while (true) {
        // MSB = 1 for a Read operation. Lower 7 bits = register 0x3B
        uint8_t command = ACCEL_START_REG | 0x80;
        uint8_t buffer[6] = {0}; // Buffer to catch the 6 bytes

        // 1. Pull CS Low to wake up your custom chip
        gpio_put(PIN_CS, 0);

        // 2. Send the command byte (your chip processes this and prepares data)
        spi_write_blocking(SPI_PORT, &command, 1);

        // 3. Read the 6 bytes of dummy data your chip sends back
        spi_read_blocking(SPI_PORT, 0x00, buffer, 6);

        // 4. Pull CS High to end the transaction and reset your chip's state machine
        gpio_put(PIN_CS, 1);

        // 5. Combine the bytes into 16-bit integers
        int16_t ax = (buffer[0] << 8) | buffer[1];
        int16_t ay = (buffer[2] << 8) | buffer[3];
        int16_t az = (buffer[4] << 8) | buffer[5];

        // Print the results!
        printf("SPI Accel X: %6d | Y: %6d | Z: %6d\n", ax, ay, az);
        
        sleep_ms(1000);
    }
}