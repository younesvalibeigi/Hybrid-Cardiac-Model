# Hybrid-Cardiac-Model
## Summary
We are introducing software for a hybrid system that connects real-time 2D computational simulations developed by Abubu.js (A library developed for using WebGL) with cardiac monolayers using a fully optical system. 
![image](https://user-images.githubusercontent.com/54210190/147422781-7e663cee-ce4e-4a3a-bb87-ffe2b6e7ce45.png)

Note: all the detailed instructions are explained in the Appendices A, B, C, D, and F of my master's thesis. Link: [TBA]
## Step 1-Developing the NodeJS Server
### Initiating the NodeJS server
NodeJS (we used v12.18.3) and npm package manager (we used 6.14.6) must be installed prior to developing a NodeJS project. Open the project folder and make a file called server.js. Open a terminal (Linux/Mac) or command prompt (Windows) and go to the project folder’s location. The following commands will initiate the NodeJs server.
```
npm init
```
Executing this command asks you a series of questions about package name, version, author, etc. Choose the entry point as server.js and type yes at the end. The code initiates the server. The following commands install the necessary packages for developing communication systems.
```
npm install express
npm install socket.io
npm install serialport
```
The express package allows building the client. The socket.io and serialport package consecutively provides the tool for server-client and server-microcontroller communication systems. 
### Brief summary of the files and folders
The model allows bi-directional communication between the monolayer and the simulation through optical tools. NodeJS server works as a buffer zone connecting different system parts together. All the files related to the AbubuJS simulation are in a folder called Public.The below image shows the organization of AbubuJS files and libraries:

![image](https://user-images.githubusercontent.com/54210190/147422615-63822462-c58e-41fc-87be-c78020be5fca.png)

The main.js file in the AbubuJS simulation bi-directionally communicates with the Node server. main.js has the main algorithm for the simulation. The shaders include GLSL codes for parallel processing of the simulation cells. The server communicates with the camera code (Grap.cpp) through a TCP socket and provides feedback to the Arduino (ArduinoCode.ino) through a serial port. Arduino turns on LEDs on the matrix to control the tissue. 
server.js file includes the code for building socket.io communication system between the server (NodeJS) and the client (AbubuJs program).
serialport build the communication system between NodeJS and the Arduino. The server-side of the code is implemented in server.js, and the Arduino-end can be found in ArduinoCode.ino file. 

1. server.js file works as the NodeJS server
2. main.js and the shaders in the Public folder, including AbubuJS simulation
3. Grab.cpp in Pylon sofware for the camera code
4. ArduinoCode.ino in a folder called ArduinoCode. It has to be open using the Arduino IDE application.


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

**threshold.txt** must be placed in the same folder as Grab.exe too. It provides determines the threshold of detecting a wave.
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