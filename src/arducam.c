#include "arducam.h"

uint8_t read_register(uint8_t reg) {
    uint8_t ret;
    reg &= 0x7F; //read bit set to 0
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);
    spi_write_read_blocking(spi_default, &reg, &ret, 1);
    reg = 0;
    spi_write_read_blocking(spi_default, &reg, &ret, 1);
    spi_write_read_blocking(spi_default, &reg, &ret, 1);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    return ret;
}

void write_register(uint8_t buf[2]) {
    buf[0] |= 0x80; // for write bit set to 1
    uint8_t temp;
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);
    spi_write_read_blocking(spi_default, &buf[0], &temp, 1);
    spi_write_read_blocking(spi_default, &buf[1], &temp, 1);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
}

uint32_t camera_get_picture_length()
{
    uint32_t len1, len2, len3, length = 0;
    len1   = read_register(0x45);
    len2   = read_register(0x46);
    len3   = read_register(0x47);
    // Need to read three registers for total length of image stored in buffer
    length = ((len3 << 16) | (len2 << 8) | len1) & 0xffffff;
    return length;
}

void camera_wait() {
    while((read_register(0x44) & 0x03) != (1 << 1)) {
    }
}

uint32_t camera_take_picture() {
    uint8_t clear_fifo_flag[] = {0x04, 0x01};
    write_register(clear_fifo_flag);
    uint8_t take_picture[] = {0x04, 0x02};
    write_register(take_picture);

    //waits for camera to take picture
    while((read_register(0x44) & 0x04) == 0){}
    return camera_get_picture_length();
}

void load_image(uint8_t *buf, uint32_t size) {
    uint8_t fifo_burst[] = {0x3C, 0};
    uint8_t temp;
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);
    spi_write_read_blocking(spi_default, &fifo_burst[0], &temp, 1);
    spi_write_read_blocking(spi_default, &fifo_burst[1], &temp, 1);

    spi_read_blocking(spi_default, 0, buf, size);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
}

void camera_start() {
    uint8_t reset_camera[] = {0x07, (1 << 6) | (1 << 7) | (1 << 1)};
    // Reset the camera
    write_register(reset_camera);
    //Polls until camera resets
    camera_wait();

    uint8_t debug[] = {0x0A, 0x78};
    write_register(debug);
    camera_wait();

    //Returns jpeg through spi(only mode compatible wih video?)
    uint8_t set_jpeg_format[] = {0x20, 0x01};
    write_register(set_jpeg_format);
    camera_wait();

    //1 = 320 x 240 and 2 = 640x480 (only possible modes resolutions supported?)
    //Changed to normal capture mode(To change back change to (1 << 7))
    uint8_t set_video_resolution[] = {0x21 , (1 << 1) | (1 << 7)};
    write_register(set_video_resolution);
    camera_wait();

    //Allow camera to adjust to lighting
    sleep_ms(500);
}