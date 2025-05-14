# Supplementary Material: Hybrid-Cardiac-Model

## Overview

We are pleased to introduce a software package for a hybrid system that integrates real-time 2D cardiac simulations—built using the WebGL-based Abubu.js library—with living cardiac monolayers through a fully optical interface.

![image](https://user-images.githubusercontent.com/54210190/147422781-7e663cee-ce4e-4a3a-bb87-ffe2b6e7ce45.png)


#  NodeJS Server for Hybrid Cardiac Feedback System

This guide describes the NodeJS server architecture used in our hybrid system. It facilitates real-time interaction between a 2D simulation (via Abubu.js), a cardiac monolayer, a microcontroller, and a camera using `socket.io`, `serialport`, and TCP protocols.

---

## 🛠 1. Setting Up the NodeJS Server

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

## 🔌 2. Building the Server–Client Communication System

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

## 🔧 3. Setting Up NodeJS–Arduino Communication

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

## 🌐 4. Connecting the Camera via TCP

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

## 📷 5. TCP Client in C++ (Camera Side)

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

# AbubuJS simulation

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

### 8. Reading Texture Data (Optional)

To read voltage values from the simulation texture:

```javascript
env.txtCA1Reader = new Abubu.TextureReader(env.txtCA1);
env.txtCA1Data = env.txtCA1Reader.read();
```

This returns a 1D array of size `256*256*4`, where every four values represent the RGBA channels of a simulation cell.

For more advanced features (e.g., clicking to create excitation), refer to:
👉 https://github.com/kaboudian/WebGLTutorials

## Step 2-Initializing the Pylon software and setting up the camera
### Installing SDK package:
A C++ program linked to the Pylon software development kit (SDK, version 6.0.1) was written using Microsoft Visual Studio (version community 2019). The following path in the SDK kit reaches a C++ code that needs to include the content of the Grab.cpp file in this respiratory.
```
SDK-kit\Development\Samples\C++\Grab\Grab.cpp
```
Note that in order to access the Development folder, the development version of SDK must be installed. The camera algorithm look at the regions, calculate the time difference and distance between a detected wave in these two regions by locating the wave as specific rows. Using the calculated conduction velocity of the cardiac wave, the camera predicts the location of the wave. 
![image](https://user-images.githubusercontent.com/54210190/147423120-4d68f023-32c9-4a0a-89c2-7667c40282dd.png)

Note that the algorithm only looks at the rows if the average threshold of the region passes a specific threshold, and it locates the wave at a particular row if the row average passes another predetermined threshold. Step 5 explains how to determine these thresholds and set them. 
The following settings must be applied to the pylon Viewer 64-bit software after opening the camera on the software:
- Analog control => Gain Auto: Continuous
- Acquisition Control => Exposure Time [us]: 30000.0
- Acquisition Control => Enable Acquisition Frame Rate: Check
- Acquisition Control => Acquisition Frame Rate [Hz]: 30.0
- Digital I/O Control => Line Selector: Line 3
- Digital I/O Control => Line Mode: Output
- Digital I/O Control => Line Source: Exposure Active

Note that the camera should be closed on the software before running the camera algorithm through the cammand line. 
## Step 3-Connection between Arduino kit and the matrix LED
The following connection should be made between the Arduino Uno and the matrix LED
1. port 13 on the Arduino to DIN pin and the matrix
2. port 11 to CLK pin
3. Ground port to GND
4. 5V port to 5V pin

![image](https://user-images.githubusercontent.com/54210190/147423809-611938ca-b8a3-4d0a-9ffb-c3df1d2caf27.png)

## Step 4-Running the code
First, run the Node server on the local terminal (Linux/Mac) or Command Prompt (Windows):
```
node server.js
```
A browser page must be open, and the address of the appropriate local host should be searched. For our code, we used port 8081, so the address will be:
```
http://localhost:8081/
```
The simulation will show up. You can run or stop the simulation from here. 
- By clicking on the Canvas, you can create activation.
- Shift and Click will create blocks in the simulation.

Set these settings for Visual Studio:
1. Solution configuration: Release
2. Solution platform: x64

Then compile the code. Afterward, run the camera code on the terminal from this address:
```
cd SDK-kit\Development\Samples\C++\Grab\bin\Release\x64
./Grab
```
Using the below command, the program stops recording and starts saving the last 500 frames as a binary file. The file can be find in the same folder as Grab.exe:
```
s
```
Make sure to shut down the program using the following command:
```
q
```
Note that this step is crucial for avoiding errors in later recordings.
## Step 5-Settings for the ROIs, thresholds, and refractory
**camera_input.txt** must be placed in the same folder as Grab.exe. It provides the inputs for the two ROI four corners and the print conditions. 
1. bottom area top edge (yEdgeSec)
2. bottom area bottom edge (yEndSec)
3. bottom area left edge (xEdgeSec)
4. bottom area right edge (xEdgeSec) 
5. top area top edge (yEdge)
6. top area bottom edge (yEnd)
7. top area left edge (xEdge)
8. top area right edge (xEdge) 
9. print condition for area mean. If you change this value to 1, the program prints the area mean of the top ROI
10. print condition for row mean. If you change this value to 1, the program prints all the row means of the same ROI
11. refractory number, it tells the algorithm to ingonre how many frames before start looking at the ROIs. The number 20 usually works the best.

**threshold.txt** must be placed in the same folder as Grab.exe too. It determines the threshold of detecting a wave.
1. the threshold of detection of the area mean for both ROIs
2. the threshold of detection for row mean for both ROIs


In case you run into errors, these line in the Grab.cpp might help you find the bug:
```
// Printing the determined values
//cout << "y1: " << y1sec << ", y2: " << y2 << "   z1: " << z1sec << ", z2: " << z2 << endl; 

//cout << "delta Y: " << y1sec - y2 << endl;
//cout << "delta Z: " << z2 - z1sec << endl;
//cout << "CV: " << conductionVelocity << endl;

//cout << "y2: " << y2 << "    timeRemInt: " << timeRemInt  << " = " << frameRemInt << "   frames" << endl;
//cout << "num Fr from Start: " << numFrSinceStart << endl;

prevNumFrSinceStart = numFrSinceStart;

cyclePeriods[cvNum] = timeSinceStart - prevTimeSinceStart;
cout << cvNum << ": " << timeSinceStart - prevTimeSinceStart << endl;
prevTimeSinceStart = timeSinceStart;
```
