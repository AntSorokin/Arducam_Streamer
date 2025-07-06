#include "hardware/spi.h"
#include "pico/stdlib.h"

/**
 * Read register "reg" for arducam camera
 *  @returns Value stored in register
 */ 
uint8_t read_register(uint8_t reg);

/**
 * Write value to register
 * @param buf Index 0 is for the register, index 1 is for the value to write
 */
void write_register(uint8_t buf[2]);
uint32_t camera_take_picture();

/**
 * Get the length of the picture taken by the arducam
 * @warning Must call camera_take_picture() first
 * @returns the byte size of the image
 */
uint32_t camera_get_picture_length();

/**
 * Polls arducam until the camera finishes taking a picture
 * @warning Must call camera_take_picture() first
 */
void camera_wait();

/**
 * Loads the image frame from the camera to  "buf"
 * @param buf 
 * @param size Amount of bytes to load from caera into buf
 */
void load_image(uint8_t *buf, uint32_t size);

/**
 * Resets and configures camera to video mode
 */
void camera_start();