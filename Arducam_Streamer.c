#include <stdio.h>
#include "pico/cyw43_arch.h"
#include "pico/mem_ops.h"
#include "pico/malloc.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include <lwip/udp.h>
#include "arducam.h"
#include "aes.h"

/**
 * This version runs the video feed without AES encryption.
 * The format of the video stream is the MJPEG format.
 * Two cores are used, one for constantly polling the camera for data and one to send
 * the video data over UDP. A double buffer approach is used so that both cores
 * could write and read from RAM at the same time.
 */

#define SERVER_IP "YOUR_IP"
#define SERVER_PORT 20001

#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"

// Used for handling the buffer
#define BUFFER_SIZE 30000
#define FRAME_SIZE 1400

#define WATCHDOG_TIME 7500

uint8_t *buf0;
uint32_t buf0_len = 0;
uint8_t *buf1;
uint32_t buf1_len = 0;

// Resets the pico, usually after a watchdog timer expires
inline static void pico_reset() {
    *((volatile uint32_t*)(PPB_BASE + 0x0ED0C)) = 0x5FA0004;
    while(true) {
        sleep_us(10);
    }
}

void camera_poll() {
    uint32_t temp_len = 0;
    while(true) {
        //Checks if buf0 is available to load new image data(double buffer approach)
        if(buf0_len == 0) {
            temp_len = camera_take_picture();
            if(temp_len < BUFFER_SIZE) {
                load_image(buf0, temp_len);
                buf0_len = temp_len;
                watchdog_update();
            } else {
                //Resets camera(Likely error occured)
                camera_start();
            }
        //Checks if buf1 is available to load new image data(double buffer approach)
        } else if(buf1_len == 0) {
            temp_len = camera_take_picture();
            if(temp_len < BUFFER_SIZE) {
                load_image(buf1, temp_len);
                buf1_len = temp_len;
                watchdog_update();
            } else {
                // Resets camera(Likely error occured)
                camera_start();
            }
        } else {
            // Waiting for UDP socket to send image data
            sleep_us(5);
        }
    }
}

int main() {
    stdio_init_all();
    // Initialize SPI for camera
    spi_init(spi_default, 8 * 1000 * 1000);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    // sleep_ms(30000);
    camera_start();

    //Initialize UDP connection
    if (cyw43_arch_init()) {
        //Reset if error occured 
        printf("Wi-Fi init failed\n");
        pico_reset();
        return -1;
    }

    // Enable wifi station
    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    int error;
    if ((error = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000))) {
        printf("failed to connect. %d\n", error);
        //Reset if error occured 
        pico_reset();
    } else {
        printf("Connected.\n");
        // Read the ip address in a human readable way
        printf("MTU %d\n", cyw43_state.netif[0].mtu);
        uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
        printf("IP address %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    }

    struct udp_pcb *local = udp_new();
    if(!local) {
        printf("Couldn't allocate pcb\n");
        pico_reset();
    }
    err_t err = udp_bind(local, &(cyw43_state.netif[0].ip_addr), 0);
    if(err != ERR_OK) {
        printf("ERROR Binding: %d\n", err);
        pico_reset();
    }

    ip_addr_t *server_ip = malloc(sizeof(ip_addr_t));
    ipaddr_aton(SERVER_IP, server_ip);
    if(udp_connect(local, server_ip, SERVER_PORT)) {
        printf("Failed to connect\n");
        pico_reset();
    }

    buf0 = malloc(BUFFER_SIZE);
    buf1 = malloc(BUFFER_SIZE);

    //Will reset pico if something halts or stops
    watchdog_enable(WATCHDOG_TIME, 0);
    multicore_launch_core1(camera_poll);
    
    const uint16_t frag_size = FRAME_SIZE;
    // Filler variables 
    const uint8_t zero = 0;
    const uint8_t one = 1;
    // Keeps track of frame
    uint8_t id = 0;
    while(true) {
        cyw43_arch_poll();
        //printf("UDP loop\n");
        if(buf1_len != 0) {
            // Breaks image into fragements to avoid IP fragmentation(can significantly slow down the stream)
            uint32_t num_frags = buf1_len / frag_size;
            uint32_t leftover_bytes = buf1_len % frag_size;

            uint32_t i = 0;
            id++;
            for(;i < num_frags;i++) {
                struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, frag_size + 3, PBUF_RAM);
                pbuf_take(p, &buf1[i * frag_size], frag_size); 
                // Each packet has an "id"(uint8), "order number"(uint8), and "complete"(0 or 1 uint8) integer included.
                // This can be used to keep track of the order the packets 
                // as they reach the client and reorder them to display the image
                // To see an example, udp_server.py outputs this data into the console 
                pbuf_take_at(p, &id, 1, frag_size);
                pbuf_take_at(p, &i, 1, frag_size + 1);
                pbuf_take_at(p, &zero, 1, frag_size + 2);
                if((err = udp_send(local, p))) {
                    printf("ERROR: %d\n", err);
                    pico_reset();
                }
                cyw43_arch_poll();
                pbuf_free(p);
                watchdog_update();
            }
            // Each image is split into fragements, the code below handles the last fragement that 
            // may be less than or equal in size to all the other fragements(the remainder after the division)
            struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, leftover_bytes + 3, PBUF_RAM);
            if(leftover_bytes != 0) {
                pbuf_take(p, &buf1[i * frag_size], leftover_bytes);
            }
            
            pbuf_take_at(p, &id, 1, leftover_bytes);
            pbuf_take_at(p, &i, 1, leftover_bytes + 1);
            pbuf_take_at(p, &one, 1, leftover_bytes + 2);
            if((err = udp_send(local, p))) {
                    printf("ERROR: %d\n", err);
                    pico_reset();
            }
            cyw43_arch_poll();
            pbuf_free(p);

            buf1_len = 0;
            watchdog_update();
        // Same code as in previous if clause but with buf0(Should probably wrap it in a function)
        } else if(buf0_len != 0) {
            // Breaks image into fragements to avoid IP fragmentation
            uint32_t num_frags = buf0_len / frag_size;
            uint32_t leftover_bytes = buf0_len % frag_size;

            uint32_t i = 0;
            id++;
            for(;i < num_frags;i++) {
                struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, frag_size + 3, PBUF_RAM);
                pbuf_take(p, &buf0[i * frag_size], frag_size);
                
                // Last three bytes contain the id, the packet order, and if its the last buffer in the frame
                pbuf_take_at(p, &id, 1, frag_size);
                pbuf_take_at(p, &i, 1, frag_size + 1);
                pbuf_take_at(p, &zero, 1, frag_size + 2);
                if((err = udp_send(local, p))) {
                    printf("ERROR: %d\n", err);
                }
                cyw43_arch_poll();
                pbuf_free(p);
                watchdog_update();
            }

            struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, leftover_bytes + 3, PBUF_RAM);
            if(leftover_bytes != 0) {
                pbuf_take(p, &buf0[i * frag_size], leftover_bytes);
            }
            
            // Last three bytes contain the id, the packet order, and if its the last fragment in the frame
            pbuf_take_at(p, &id, 1, leftover_bytes);
            pbuf_take_at(p, &i, 1, leftover_bytes + 1);
            pbuf_take_at(p, &one, 1, leftover_bytes + 2);
            if((err = udp_send(local, p))) {
                    printf("ERROR: %d\n", err);
            }
            cyw43_arch_poll();
            pbuf_free(p);

            buf0_len = 0;
            watchdog_update();
        } else {
            sleep_us(5);
        }
    }
}