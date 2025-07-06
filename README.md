# 📷 Arducam Streamer

**Arducam Streamer** is a real-time, AES-encrypted MJPEG video streaming project using the **RP2040 Pico W** and the **Arducam Mega 3MP NoIR** camera module. It streams video over Wi-Fi using a UDP socket. A demo Python script (`udp_server.py`) is included to receive and display the video feed.

---

## 🚀 Features

- 🔒 AES encryption (CBC mode)  
- 🌐 Video streaming over UDP  
- 🎞️ Real-time MJPEG decoding using OpenCV  
- 💻 Python-based demo server  

---

## 🛠️ Getting Started

### 1. Clone the Repository

```bash
git clone https://github.com/your_username/arducam-streamer.git
cd arducam-streamer
```

### 2. Configure the C Code

Open `Arducam_Streamer_v2.c` and edit the following lines:

```c
#define SERVER_IP     "192.168.x.x"
#define WIFI_SSID     "YourSSID"
#define WIFI_PASSWORD "YourPassword"
#define KEY           "your-32-byte-key"
#define IV            "your-16-byte-iv"
```

> 📝 Make sure your `KEY` is 32 bytes and your `IV` is 16 bytes.

### 3. Compile and Flash

Use **VS Code** with the **Raspberry Pi Pico extension** to compile and flash the firmware to your RP2040 Pico W.

### 4. Set Up the Python Environment

Create a virtual environment (optional but recommended), then install the dependencies:

```bash
pip install opencv-python pycryptodome
```

### 5. Update the Python UDP Server

Edit lines 29–30 in `udp_server.py`:

```python
key = b'your-32-byte-key'
iv  = b'your-16-byte-iv'
```

### 6. Open UDP Port in Firewall

Allow inbound UDP traffic on **port 20001** (default):

- **Windows:** Add a new rule in the Firewall settings.  
- **Linux/macOS:** Use `ufw` or system firewall tools.

### 7. Run the UDP Server

```bash
python udp_server.py
```

### 8. View the Video Feed

A window should appear with the live MJPEG stream.

- Press `q` to close the window.

---

## 📁 Project Structure

```
.
├── Arducam_Streamer_v2.c      # Pico W C firmware
├── udp_server.py              # Python demo UDP server
└── README.md
```

---

## 📌 Notes

- The Pico W and your computer **must be on the same network**.  
- UDP is used for low-latency streaming. You may experience some packet loss.  
- AES mode is **CBC** – both key and IV must match on sender and receiver.

---
