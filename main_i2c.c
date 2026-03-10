#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// --- CHANGE THESE TO MATCH YOUR WIRING! ---
#define I2C_PORT i2c0
#define I2C_SDA 4 // Pico pin connected to custom chip SDA
#define I2C_SCL 5 // Pico pin connected to custom chip SCL

#define MPU6000_ADDR 0x68
#define ACCEL_START_REG 0x3B

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Starting MPU-6000 Emulator Test via I2C...\n");

    // Initialize I2C at 400 kHz
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    while (true) {
        uint8_t reg = ACCEL_START_REG;
        uint8_t buffer[6] = {0};

        // 1. Write the register address we want to start reading from
        i2c_write_blocking(I2C_PORT, MPU6000_ADDR, &reg, 1, true); 

        // 2. Read the 6 bytes of dummy data
        i2c_read_blocking(I2C_PORT, MPU6000_ADDR, buffer, 6, false);

        // 3. Combine the bytes into 16-bit integers
        int16_t ax = (buffer[0] << 8) | buffer[1];
        int16_t ay = (buffer[2] << 8) | buffer[3];
        int16_t az = (buffer[4] << 8) | buffer[5];

        // Print the results!
        printf("I2C Accel X: %6d | Y: %6d | Z: %6d\n", ax, ay, az);
        
        sleep_ms(1000);
    }
}