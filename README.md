
# 📷 Arducam Streamer

**Arducam Streamer** is a real-time video streaming project that uses the **RP2040 Pico W** and the **Arducam Mega 3MP NoIR** camera. It captures MJPEG frames, encrypts them using AES, and sends them over Wi-Fi using UDP. A Python script (`udp_server.py`) is included to receive, decrypt, and display the video stream on your computer.

---

## 🚀 Features

- 🔒 AES CBC-mode encryption of image data
- 📷 Dual-core processing for concurrent camera polling and data encryption
- 🌐 Low-latency streaming via UDP with fragmentation-aware transmission
- 🎞️ Real-time MJPEG decoding with OpenCV on the receiving end

---

## 🛠️ Getting Started

### 1. Clone the Repository

```bash
git clone https://github.com/your_username/arducam-streamer.git
cd arducam-streamer
```

### 2. Configure the Firmware

Edit `Arducam_Streamer_v2.c` and set your Wi-Fi and encryption parameters:

```c
#define WIFI_SSID     "YourSSID"
#define WIFI_PASSWORD "YourPassword"
#define SERVER_IP     "x.x.x.x"
#define KEY           "your-key"
#define IV            "your-iv"
```

> 🔑 For more details about KEY and IV see the AES implementation used for this project at [kokke/tiny-AES-c](https://github.com/kokke/tiny-AES-c).

### 3. Compile and Flash the Firmware

Use **Visual Studio Code** with the **Raspberry Pi Pico SDK** to compile the firmware and flash it to the Pico W.

### 4. Set Up the Python Environment

Install the required dependencies:

```bash
pip install opencv-python pycryptodome
```

### 5. Update the Python Demo Script

In `udp_server.py`, update the encryption parameters on line 29-30 to match the ones used in the firmware:
```python
key = 'your-key'
iv  = 'your-iv'
```

### 6. Allow UDP Through the Firewall

Enable inbound UDP traffic on **port 20001** (default).
- **Windows:** Add a new rule in the Firewall settings.  
- **Linux/macOS:** Use `ufw` or system firewall tools.

### 7. Run the Python UDP Server

```bash
python udp_server.py
```

### 8. View the Video Stream

The decrypted video feed will appear in a new window. Press `q` to close it.

---

## ⚙️ Technical Highlights & Optimizations

### 🧠 Dual-Core Parallelism

The RP2040's dual-core architecture is leveraged for improved performance:

- **Core 0**: Handles AES encryption and UDP packet transmission.
- **Core 1**: Continuously polls the camera, retrieves image data, and stores it in a shared buffer.

This concurrent design ensures that encryption and transmission can occur without waiting for the camera, significantly improving throughput.

### 🔁 Double Buffering

Two memory buffers (`buf0` and `buf1`) are used in an alternating fashion:

- While one buffer is being filled with new camera data,
- The other is being encrypted and transmitted.

This non-blocking mechanism allows for near-continuous capture and stream processing.

### ✂️ Fragmentation-Aware UDP Streaming

Each image frame is encrypted as a whole and then **split into multiple UDP packets** manually to avoid IP-layer fragmentation. Each packet includes:

- A **frame ID**
- A **packet index**
- A **completion flag**

These are appended to the end of the payload, allowing the receiver to reconstruct and decrypt the full frame in correct order.

### 🔐 Secure Streaming

The entire image buffer is padded and encrypted using **AES CBC mode** before being split and sent, ensuring privacy even over insecure networks. The receiver uses `pycryptodome` to decrypt and reassemble the image in memory before displaying it.

---

## 📝 Notes

- The Pico W and host machine must be on the **same local network**.
- The UDP protocol is used for speed, so occasional packet loss may occur.
- A hardware watchdog is used to ensure the Pico resets if a fault or hang occurs.
- The receiver uses OpenCV to decode MJPEG images in real-time.

---
