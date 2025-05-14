# Supplementary Material: Hybrid-Cardiac-Model

## Overview

We present a software package designed for a hybrid experimental-computational system that integrates real-time 2D cardiac simulations—implemented using the WebGL-based Abubu.js library—with living cardiac monolayers via a fully optical interface. This documentation provides a comprehensive, step-by-step guide intended for users with limited coding experience, detailing the functionality and execution of each component within the hybrid system.

![image](https://user-images.githubusercontent.com/54210190/147422781-7e663cee-ce4e-4a3a-bb87-ffe2b6e7ce45.png)


# Step 1 — NodeJS Server for Hybrid Cardiac Feedback System

This guide describes the NodeJS server architecture used in our hybrid system. It facilitates real-time interaction between a 2D simulation (via Abubu.js), a cardiac monolayer, a microcontroller, and a camera using `socket.io`, `serialport`, and TCP protocols.

---

## 1. Setting Up the NodeJS Server

### 1.1 Initializing the Project

Ensure **NodeJS** (v12.18.3) and **npm** (v6.14.6) are installed. Open a terminal (Linux/macOS) or command prompt (Windows), navigate to your project folder, and initialize the project:

```bash
npm init
```

This command prompts for metadata like package name, version, and entry point. Set the **entry point to `server.js`** and confirm with `yes`.

### 1.2 Installing Required Packages

```bash
npm install express
npm install socket.io
npm install serialport
```

- `express`: Creates the client-server structure  
- `socket.io`: Enables real-time communication between server and client  
- `serialport`: Manages communication with the Arduino microcontroller

---

##  2. Building the Server–Client Communication System

### 2.1 Importing Libraries and Launching the Server

In your `server.js`, add the following code to initialize the server and expose a static folder named `public`:

```javascript
const express = require('express');
const app = express();
const server = require('http').createServer(app);
const io = require("socket.io")(server);

const HOST = 'localhost';
const PORTIO = 8081;

server.listen(PORTIO, function(){});
app.use(express.static('public'));
```

This makes the server accessible at `http://localhost:8081`.

### 2.2 Receiving Data from the Client

The following code sets up the `socket.io` event listener to receive data (e.g., light patterns) from the client. The channel name is `led`.

```javascript
io.on("connection", function(sockIO){
    console.log('Client is connected');

    // Receive data from the public folder (client)
    sockIO.on('led', function(data) {
        var received_data = data.value;
        /** Insert code here to forward received_data to the Arduino */
    });

    sockIO.on('disconnect', (reason) => {
        console.log('Client is disconnected');
    });
});
```

### 2.3 Sending Data to the Client

To send a signal (e.g., from the camera) to the client, use the following:

```javascript
io.emit('led', { value: camera_signal });
```

---

## 3. Setting Up NodeJS–Arduino Communication

### 3.1 Configuring SerialPort

Add the following to `server.js` to connect to the Arduino over USB:

```javascript
const SerialPort = require("serialport");
const Readline = require('@serialport/parser-readline');
const serialPort = new SerialPort("COM5", { baudRate: 9600 });
const parser = serialPort.pipe(new Readline({ delimiter: '\n' }));
```

> **Note:**  
> - Windows: `COM1`, `COM2`, ..., `COM5`  
> - macOS: `/dev/cu.usbmodem14101`  
> - Ubuntu: `/dev/ttyACM0`

### 3.2 Sending Data to Arduino

After receiving data from the client, you must convert it into a format compatible with the serial port (buffer of 4 bytes):

```javascript
let buf = Buffer.allocUnsafe(4);
buf.writeInt32LE(received_data);
serialPort.write(buf);
```

### 3.3 Receiving Data from Arduino

Although not necessary for the hybrid system, you can also listen for incoming data from the Arduino:

```javascript
serialPort.on("open", () => {
    console.log('Arduino is connected');
});

parser.on('data', data => {
    // data is the byte value received from Arduino
});
```

---

## 4. Connecting the Camera via TCP

### 4.1 TCP Socket on the NodeJS Server

The Transmission Control Protocol (TCP) socket lets the camera (written in C++) communicate with the server. Add this to `server.js`:

```javascript
const net = require('net');
const PORTNET = 8080;

net.createServer(function(sockNet){
    sockNet.on('data', async function(data) {
        camera_signal = JSON.parse(data);

        /** Insert code here to send data to the client */

        sockNet.write("Return String\n");  // Acknowledge receipt
    });

    sockNet.on('close', function(data) {});
}).listen(PORTNET, HOST);
```

> **Important:** TCP is a closed loop — each data received from the client must be acknowledged with a response.

---

## 5. TCP Client in C++ (Camera Side)

### 5.1 Establishing the Connection

```cpp
string ipAddress = "127.0.0.1";  // Server IP
int port = 8080;                 // TCP Port

WSAData data;
WORD ver = MAKEWORD(2, 2);
int wsResult = WSAStartup(ver, &data);

// Create socket
SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

sockaddr_in hint;
hint.sin_family = AF_INET;
hint.sin_port = htons(port);
inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

// Connect
int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
```

### 5.2 Sending Signal to the Server

Convert the signal to a string, send it, and await a response:

```cpp
char buf[4096];
std::string s = std::to_string(signal);
char const* pchar = s.c_str();

int sendResult = send(sock, pchar, (int)strlen(pchar), 0);

if (sendResult != SOCKET_ERROR) {
    ZeroMemory(buf, 4096);
    int bytesReceived = recv(sock, buf, 4096, 0);
}
```

> Port `8081` is used for NodeJS–HTML communication  
> Port `8080` is used for TCP communication with the camera

# Step 2 —AbubuJS simulation

### Brief Summary of the Files and Folders

The hybrid model facilitates bidirectional communication between the cardiac monolayer and the simulation through an all-optical interface. A Node.js server acts as the central hub, coordinating interactions between the simulation, the camera, and the LED control system.

All AbubuJS simulation files are located within the `public/` directory. The structure is illustrated below:

![image](https://user-images.githubusercontent.com/54210190/147422615-63822462-c58e-41fc-87be-c78020be5fca.png)

- `main.js`: Contains the primary simulation algorithm and handles WebSocket communication with the server.
- Shader files (`.frag`): GLSL fragment shaders used for parallel simulation computations.
- `server.js`: Builds the NodeJS server, handling both:
  - `socket.io` communication with the simulation
  - `serialport` communication with the Arduino
- `Grab.cpp`: A C++ script used with the Pylon SDK to interface with the Basler camera, sending image-derived signals to the Node server via TCP.
- `ArduinoCode.ino`: The Arduino firmware for controlling the LED matrix. This file should be opened using the Arduino IDE.

### File Overview

1. `server.js`: NodeJS server to manage simulation, camera, and microcontroller communication  
2. `main.js` and shader files (`initShader.frag`, `compShader.frag`) in the `public/` folder for running the AbubuJS-based simulation  
3. `Grab.cpp`: Captures and processes signals from the camera; sends TCP data to the server  
4. `ArduinoCode.ino`: Runs on Arduino to translate serial inputs into LED stimulation patterns

This section explains how to build the hybrid cardiac simulation using the AbubuJS library. The model combines cellular automata dynamics with WebGL-based parallel computing. It also interfaces with the Node.js server to receive real-time input from a camera and emit feedback to a microcontroller.

**Contents:**
- Setting up the HTML and JavaScript files
- Initializing computational textures
- Developing solvers for simulation steps
- Rendering and running the simulation
- Communicating with the server
- Shader codes for the cellular automata algorithm

---

### 1. HTML File Setup (`index.html`)

Include the following scripts in your HTML file:

```html
<script src="socket.io/socket.io.js"></script>
<script src='config.js'></script>
<script src='libs/stats.js'></script>
<script data-main="app/main" src="libs/require.js"></script>
```

And define a canvas to display the simulation:

```html
<canvas width=512 height=512 id='canvas'></canvas>
```

---

### 2. JavaScript Setup (`main.js`)

Use `require.js` to define the simulation module and load shaders:

```javascript
define([
    'jquery',
    'Abubu/Abubu.js',
    'shader!initShader.frag',
    'shader!compShader.frag',
    'shader!clickShader.frag'
],
function($, Abubu, initShader, compShader, clickShader) {
    // Simulation logic and socket communication go here
});
```

The FK-based cellular automata model is constructed using this setup.

---

### 3. Defining the Canvas and Textures

Set up the canvas and define the textures for state updates:

```javascript
env.canvas_1 = document.getElementById('canvas');
env.canvas_1.width = env.width;
env.canvas_1.height = env.height;

env.txtCA1 = new Abubu.Float32RTexture(256, 256);
env.txtCA2 = new Abubu.Float32RTexture(256, 256);
```

Create an initialization texture with random perturbations and scar definitions:

```javascript
var table = new Float32Array(256 * 256 * 4);
var idx = 0;
for (var j = 0; j < 256; j++) {
    for (var i = 0; i < 256; i++) {
        table[idx++] = env.psize * (Abubu.random() - 0.5);
        table[idx++] = env.psize * (Abubu.random() - 0.5);
        table[idx++] = 1.; // marks block
        table[idx++] = 0.;
    }
}
env.txtInit1 = new Abubu.Float32Texture(env.width, env.height, { data: table });
```

---

### 4. Solver Setup

#### Initial Solver

```javascript
env.initSolver = new Abubu.Solver({
    fragmentShader: initShader,
    renderTargets: {
        o_col_0: { location: 0, target: env.txtCA1 },
        o_col_1: { location: 1, target: env.txtCA2 },
    }
});
env.initSolver.render();
```

#### Cellular Automata Solvers

Two alternating solvers are defined to iteratively compute the next state:

```javascript
env.solverCA1 = new Abubu.Solver({
    fragmentShader: compShader,
    uniforms: {
        input_txt: { type: 's', value: env.txtCA1 },
        inital_txt: { type: 's', value: env.txtInit1 },
        radius: { type: 'f', value: env.radius },
        threshold: { type: 'f', value: env.threshold },
        Lx: { type: 'f', value: env.Lx },
    },
    renderTargets: {
        out_txt: { location: 0, target: env.txtCA2 },
    }
});

env.solverCA2 = new Abubu.Solver({
    fragmentShader: compShader,
    uniforms: {
        input_txt: { type: 's', value: env.txtCA2 },
        inital_txt: { type: 's', value: env.txtInit1 },
        radius: { type: 'f', value: env.radius },
        threshold: { type: 'f', value: env.threshold },
        Lx: { type: 'f', value: env.Lx },
    },
    renderTargets: {
        out_txt: { location: 0, target: env.txtCA1 },
    }
});
```

Define a function to alternate between solvers:

```javascript
env.march = function() {
    env.solverCA1.render();
    env.solverCA2.render();
};
```

---

### 5. Displaying the Simulation

```javascript
env.displayCA = new Abubu.Plot2D({
    target: env.txtCA1,
    channel: 'r',
    minValue: 0.,
    enableMinColor: true,
    minColor: [1, 1, 1],
    maxValue: 1.,
    colormap: env.colormap,
    canvas: env.canvas_1,
});
env.displayCA.init();
```

---

### 6. Running the Simulation Loop

```javascript
function run() {
    env.march();
    env.displayCA.render();
    requestAnimationFrame(run);
}
requestAnimationFrame(run);
```

---

### 7. Server Communication via WebSocket

To receive signals from the server:

```javascript
const socket = io.connect('http://localhost:8081');
socket.on('led', function(data){
    t = data.value;
});
```

To send updated simulation time back:

```javascript
socket.emit("led", { value: delta_t_total });
```

---

### 8. Reading Texture Data

To read voltage values from the simulation texture:

```javascript
env.txtCA1Reader = new Abubu.TextureReader(env.txtCA1);
env.txtCA1Data = env.txtCA1Reader.read();
```

This returns a 1D array of size `256*256*4`, where every four values represent the RGBA channels of a simulation cell.

For more advanced features (e.g., clicking to create excitation), refer to:
👉 https://github.com/kaboudian/WebGLTutorials

# Step 3 — Initializing the Pylon Software and Setting Up the Camera

### Installing the SDK Package

A C++ program was developed using Microsoft Visual Studio (Community Edition 2019) and linked to the Pylon SDK (version 6.0.1). To modify the base program, locate and update the following path within the SDK:

```
SDK-kit\Development\Samples\C++\Grab\Grab.cpp
```

> 📌 The development version of the SDK must be installed to access the `Development` folder.

The algorithm detects wave motion by analyzing pixel intensities across specific rows of the frame. Based on the calculated conduction velocity, the wave’s future location is predicted.

![image](https://user-images.githubusercontent.com/54210190/147423120-4d68f023-32c9-4a0a-89c2-7667c40282dd.png)

The row-based detection activates only if the average intensity of the region exceeds a predefined threshold. Once this condition is met, the algorithm identifies the wave based on row-wise averages. Thresholds are discussed in more detail in the next section.

### Required Camera Settings in Pylon Viewer (64-bit)

Before executing the C++ algorithm, configure the camera settings as follows:

- **Analog Control → Gain Auto**: `Continuous`
- **Acquisition Control → Exposure Time [µs]**: `30000.0`
- **Acquisition Control → Enable Acquisition Frame Rate**: `Checked`
- **Acquisition Control → Acquisition Frame Rate [Hz]**: `30.0`
- **Digital I/O Control → Line Selector**: `Line 3`
- **Digital I/O Control → Line Mode**: `Output`
- **Digital I/O Control → Line Source**: `Exposure Active`

> ⚠️ Make sure the camera is closed in the Pylon Viewer before executing the C++ code from the terminal.



##  Camera Algorithm and Motion Analysis


###  Frame Filtering Logic

Locate the following line in `Grab.cpp`:

```cpp
// Image grabbed successfully?
if (ptrGrabResult->GrabSucceeded()) {
    const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
    // Insert filtering algorithm here
}
```

#### 📌 Frame Filtering Pseudocode

```cpp
abs(current_frame - sixth_previous_frame)
```

```text
xdim = 1920
ydim = 1200
current_frame[xdim * ydim]
six_previous_frames[xdim * ydim * 6]
z = 0 // current frame number
```

```cpp
for (each pixel i in current_frame):
    current_frame[i] = pixel_value
    six_previous_frames[i] = pixel_value
    if (z >= 6):
        filtered_image[i] = abs(current_frame[i] - six_previous_frames[i - 6 * xdim * ydim])
```

---

###  Conduction Velocity and Δt_cam Calculation

The algorithm uses two rectangular ROIs (as shown in the figure above) to determine the wave’s velocity and timing.

#### 📌 Row and Area Threshold Evaluation

```cpp
float area_threshold = 10.6;
float row_threshold = 15.0;
```

You can determine thresholds manually by printing row and area averages using `cout << endl`. If waves are not detected, lower the thresholds until regular peaks are observed.

#### 📌 Velocity Estimation Pseudocode

```cpp
// First ROI
if (area_average_box1 > area_threshold && refractory_value == 0) {
    z1 = z;
    refractory_value = max_refractory;
    for (each row i in box1) {
        if (row_averages_box1[i] > row_threshold) {
            y1 = i;
            break;
        }
    }
}

// Second ROI
if (area_average_box2 > area_threshold && z1 and y1 are valid) {
    z2 = z;
    for (each row i in box2) {
        if (row_averages_box2[i] > row_threshold) {
            y2 = i;
            break;
        }
    }

    conduction_velocity = (y2 - y1) / (z2 - z1);
    delta_y = distance from y2 to predicted location;
    delta_t_camera = delta_y / conduction_velocity * (30 / 1000); // Convert FPS to ms

    // Reset and ignore future waves for a refractory period
    reset y1, y2, z1, z2;
}
```

> Recommended: `max_refractory = 20` for 30 FPS.

---

###  Saving Frames for Post-Processing

The last 500 filtered frames are saved for external analysis. Below is the code used to format and save data compatible with the GView application:

```cpp
int numFrames = 500;
int storeFrameSize = 500 * xdim * ydim;

uint8_t* unsigShiftedFrames = new uint8_t[storeFrameSize];
for (int i = 0; i < storeFrameSize; i++) {
    unsigShiftedFrames[i] = (unsigned short int)stored500Frames[i];
}

uint32_t bubmode = _byteswap_ulong(2); 
uint32_t bubzdim = _byteswap_ulong(numFrames);
uint32_t bubxdim = _byteswap_ulong(xdim);
uint32_t bubydim = _byteswap_ulong(ydim);

fstream storeFile;
storeFile = fstream("C:\file location\binary_file", ios::out | ios::binary);
storeFile.write(reinterpret_cast<char*>(&bubmode), sizeof(uint32_t));
storeFile.write(reinterpret_cast<char*>(&bubzdim), sizeof(uint32_t));
storeFile.write(reinterpret_cast<char*>(&bubydim), sizeof(uint32_t));
storeFile.write(reinterpret_cast<char*>(&bubxdim), sizeof(uint32_t));
storeFile.write((char*)unsigShiftedFrames, storeFrameSize * sizeof(uint8_t));
```

#  Step 4 — Connecting the Arduino to the LED Matrix

### Hardware Connections

Connect the Arduino Uno to the matrix LED as follows:

1. **Port 13** → `DIN` pin on the matrix  
2. **Port 11** → `CLK` pin  
3. **GND** → `GND`  
4. **5V** → `5V`  

![image](https://user-images.githubusercontent.com/54210190/147423809-611938ca-b8a3-4d0a-9ffb-c3df1d2caf27.png)

---

### Arduino IDE Configuration

Use **Arduino IDE version 1.8.49**. Under the **Tools** menu:

- **Board**: Select `Arduino Uno`
- **Port**: Choose the one assigned by your system (e.g., `COM5` on Windows)
- Install the **Adafruit DotStarMatrix** library (version 1.0.5) via `Tools → Manage Libraries`.

Arduino uses standard C/C++ syntax. Below is a breakdown of the required setup and code structure.

---

### Code Overview

#### Required Libraries and Initializations

```cpp
#include <Adafruit_DotStar.h>
#include <SPI.h>

#define NUMPIXELS 64 
Adafruit_DotStar matrix(NUMPIXELS, DOTSTAR_BRG);
uint32_t color = 0x0000FF;  // Set blue color for LEDs
int analogPin = A3;         // Analog input from the camera
```

#### `setup()` Function

```cpp
void setup() {
  Serial.begin(9600);
  matrix.begin();            // Initialize output pins
  matrix.setBrightness(255); // Max brightness
  matrix.show();             // Ensure all LEDs are off at start
}
```

---

### Main Logic (`loop()` Function)

The `loop()` function is executed repeatedly and performs the following tasks:

1. **Read the current frame from the camera**
2. **Activate the LED at the right time**
3. **Receive timing signal from the NodeJS server**
4. **Decompose the received signal into frame count and remainder**

#### Camera Signal Handling

```cpp
val = analogRead(analogPin); 
if (val > 600 && prevVal <= 600) {
    f_camera++;
}

if (f_camera == f_arduino) {
    delayMicroseconds(remainder * 1000);        // Wait before activation
    matrix.setPixelColor(LED_number, color);    // Activate LED
    matrix.show();
    // LEDs should remain on for ~100 ms
}
```

> 💡 LEDs can stay on for ~3 frames (~100 ms) to ensure reliable tissue activation.

---

### Receiving Timing from NodeJS

```cpp
if (Serial.available()) {
    byte input;
    input = Serial.read();
```

The server sends four bytes representing the signal `t`. The Arduino reconstructs `t` by combining the bytes:

```cpp
if (first_signal)  inputInt = inputInt + input * 1;
if (second_signal) inputInt = inputInt + (long)input * 256;
if (third_signal)  inputInt = inputInt + (long)input * 65536;
if (fourth_signal) inputInt = inputInt + (long)input * 16777216;
```

---

### Timing Decomposition

Convert the integer `t` into the number of frames and the remainder (in milliseconds):

```cpp
float frP = 1000.0 / 30.0; // Frame period at 30 FPS

int f_arduino = abs((long)((float)(inputInt) / frP));
int temp = (int)(inputInt % 100);
int remainder = abs((int)round(temp - ((int)((float)temp / frP)) * frP));
```

> 🛠 Using a middle variable (`temp`) helps reduce float rounding errors when converting from time to frames.

# Step 5 — Running the Code

First, launch the Node.js server in your terminal:

```bash
node server.js
```

Next, open a browser and navigate to your local server. If you’re using port 8081, the address will be:

```bash
http://localhost:8081/
```

This page hosts the simulation interface. From here:
- Click on the canvas to trigger activations.
- **Shift + Click** will create block zones in the simulation.

---

### Visual Studio Configuration

In Visual Studio:

1. Set **Solution Configuration** to `Release`
2. Set **Solution Platform** to `x64`

Then, compile the project.

To run the camera program, navigate to the following folder and execute the binary:

```bash
cd SDK-kit\Development\Samples\C++\Grab\bin\Release\x64
./Grab
```

To stop recording and save the last 500 frames as a binary file, press:

```bash
s
```

> 📁 The saved binary file will appear in the same directory as `Grab.exe`.

To safely shut down the program, press:

```bash
q
```

> ⚠️ It is important to quit properly to avoid corrupting the saved data.

---

# Step 6 — Configuring ROIs, Thresholds, and Refractory Frames

Place the following two configuration files in the same folder as `Grab.exe`:

### `camera_input.txt`

This file defines the coordinates of the two ROIs and controls output settings:

1. `yEdgeSec` — Top edge of the bottom ROI  
2. `yEndSec` — Bottom edge of the bottom ROI  
3. `xEdgeSec` — Left edge of the bottom ROI  
4. `xEndSec` — Right edge of the bottom ROI  
5. `yEdge` — Top edge of the top ROI  
6. `yEnd` — Bottom edge of the top ROI  
7. `xEdge` — Left edge of the top ROI  
8. `xEnd` — Right edge of the top ROI  
9. **Print area mean** — Set to `1` to print mean value of the top ROI  
10. **Print row means** — Set to `1` to print row-wise means of the top ROI  
11. **Refractory frames** — Number of frames to ignore after each wave detection. A value of `20` works well for 30 FPS.

### `threshold.txt`

This file sets the detection thresholds for the wave:

1. **Area mean threshold** for both ROIs  
2. **Row mean threshold** for both ROIs  

---

###  Debugging

If you encounter issues during execution, uncomment the following lines in `Grab.cpp` to help trace the problem:

```cpp
// cout << "y1: " << y1sec << ", y2: " << y2 << "   z1: " << z1sec << ", z2: " << z2 << endl;
// cout << "delta Y: " << y1sec - y2 << endl;
// cout << "delta Z: " << z2 - z1sec << endl;
// cout << "CV: " << conductionVelocity << endl;
// cout << "y2: " << y2 << "    timeRemInt: " << timeRemInt  << " = " << frameRemInt << "   frames" << endl;
// cout << "num Fr from Start: " << numFrSinceStart << endl;

prevNumFrSinceStart = numFrSinceStart;
```
