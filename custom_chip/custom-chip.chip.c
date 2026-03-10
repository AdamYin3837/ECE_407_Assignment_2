// Wokwi Custom Chip - For docs and examples see:
// https://docs.wokwi.com/chips-api/getting-started
//
// SPDX-License-Identifier: MIT
// Copyright 2026 Yan Yong Yin

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

// Define the MPU6000 I2C Address and Registers
#define MPU6000_I2C_ADDR 0x68
#define ACCEL_XOUT_H     0x3B
#define ACCEL_XOUT_L     0x3C
#define ACCEL_YOUT_H     0x3D
#define ACCEL_YOUT_L     0x3E
#define ACCEL_ZOUT_H     0x3F
#define ACCEL_ZOUT_L     0x40

// A struct to keep track of our chip's internal state
typedef struct {
  i2c_dev_t i2c;
  spi_dev_t spi;
  pin_t cs_pin;             // We need to keep track of the CS pin
  uint8_t current_register; 
  
  uint8_t accel_data[6]; 

  // Wokwi SPI State Tracking
  uint8_t spi_buffer[1];    // Wokwi uses a 1-byte array to transfer SPI data
  bool spi_first_byte_received;
  bool spi_is_read;
  uint8_t spi_reg_addr;
} chip_state_t;

// --- I2C Callbacks (Unchanged and working!) ---

static bool on_i2c_connect(void *user_data, uint32_t address, bool connect) {
  return true; 
}

static uint8_t on_i2c_read(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (chip->current_register >= ACCEL_XOUT_H && chip->current_register <= ACCEL_ZOUT_L) {
    uint8_t index = chip->current_register - ACCEL_XOUT_H;
    uint8_t data = chip->accel_data[index];
    chip->current_register++; 
    return data;
  }
  return 0x00; 
}

static bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  chip->current_register = data; 
  return true; 
}

// --- Wokwi Corrected SPI Callbacks ---

// 1. This runs every time a single byte transfer finishes
static void on_spi_done(void *user_data, uint8_t *buffer, uint32_t count) {
  chip_state_t *chip = (chip_state_t *)user_data;
  
  // The byte the Pico just sent us is sitting in buffer[0]
  uint8_t received_data = buffer[0];
  
  // If this was the very first byte (Command Byte)
  if (!chip->spi_first_byte_received) {
    chip->spi_is_read = (received_data & 0x80) != 0; 
    chip->spi_reg_addr = received_data & 0x7F; 
    chip->spi_first_byte_received = true;
    
    // Prepare the buffer for the *next* transfer! (Load our simulated data into it)
    buffer[0] = 0x00; 
    if (chip->spi_is_read) {
       if (chip->spi_reg_addr >= ACCEL_XOUT_H && chip->spi_reg_addr <= ACCEL_ZOUT_L) {
         buffer[0] = chip->accel_data[chip->spi_reg_addr - ACCEL_XOUT_H];
       }
       chip->spi_reg_addr++;
    }
  } 
  // Otherwise, we are in the data phase
  else {
    if (chip->spi_is_read) {
       buffer[0] = 0x00; // Default empty
       if (chip->spi_reg_addr >= ACCEL_XOUT_H && chip->spi_reg_addr <= ACCEL_ZOUT_L) {
         buffer[0] = chip->accel_data[chip->spi_reg_addr - ACCEL_XOUT_H];
       }
       chip->spi_reg_addr++;
    } else {
       // We ignore incoming data on a write for this assignment
       chip->spi_reg_addr++;
       buffer[0] = 0x00;
    }
  }
  
  // Crucial: If the CS pin is still LOW, Wokwi needs us to schedule the next byte transfer!
  if (pin_read(chip->cs_pin) == LOW) {
      spi_start(chip->spi, chip->spi_buffer, 1);
  }
}

// 2. This watches the CS pin to know exactly when the Pico starts and stops talking
static void on_cs_change(void *user_data, pin_t pin, uint32_t value) {
  chip_state_t *chip = (chip_state_t *)user_data;
  
  if (value == LOW) {
    // Pico pulled CS low. The transaction is starting!
    chip->spi_first_byte_received = false;
    chip->spi_buffer[0] = 0x00; // Clear out the buffer
    spi_start(chip->spi, chip->spi_buffer, 1); // Start listening for 1 byte
  } else {
    // Pico pulled CS high. The transaction is over.
    spi_stop(chip->spi);
  }
}

// --- Main Initialization ---

void chip_init() {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  
  // Dummy Data Setup
  chip->accel_data[0] = 0x03; 
  chip->accel_data[1] = 0xE8; 
  chip->accel_data[2] = 0xFE; 
  chip->accel_data[3] = 0x0C; 
  chip->accel_data[4] = 0x40; 
  chip->accel_data[5] = 0x00; 

  // Initialize I2C
  const i2c_config_t i2c_config = {
    .user_data = chip,
    .address = MPU6000_I2C_ADDR,
    .scl = pin_init("SCL", INPUT),
    .sda = pin_init("SDA", INPUT),
    .connect = on_i2c_connect,
    .read = on_i2c_read,
    .write = on_i2c_write,
  };
  chip->i2c = i2c_init(&i2c_config);

  // Initialize SPI
  chip->spi_first_byte_received = false;
  
  // We initialize the custom SDIO pin once and assign it to both MOSI and MISO
  pin_t sdio_pin = pin_init("SDIO", INPUT);
  
  const spi_config_t spi_config = {
      .user_data = chip,
      .sck = pin_init("SCLK", INPUT),
      .mosi = sdio_pin, 
      .miso = sdio_pin, 
      .mode = 0,
      .done = on_spi_done  // The correct Wokwi callback!
  };
  chip->spi = spi_init(&spi_config);

  // Set up the CS pin and attach our watcher so it triggers on LOW/HIGH changes
  chip->cs_pin = pin_init("CS", INPUT);
  const pin_watch_config_t watch_config = {
    .edge = BOTH,
    .pin_change = on_cs_change,
    .user_data = chip
  };
  pin_watch(chip->cs_pin, &watch_config);

  printf("MPU-6000 Emulator Initialized!\n");
}