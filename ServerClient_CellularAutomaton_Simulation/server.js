const net = require('net');
const express = require('express');



const app = express();
const server = require('http').createServer(app);
const io = require("socket.io")(server);//const io = require('socket.io').listen(server);
const SerialPort = require("serialport");
const Readline = require('@serialport/parser-readline');
const serialPort = new SerialPort("COM5", { baudRate: 9600 });
const parser = serialPort.pipe(new Readline({ delimiter: '\n' }));
// Mac: /dev/cu.usbmodem14101
// Ubuntu: /dev/ttyACM0
// Win: COM3 or COM4
const HOST = 'localhost';
const PORTNET = 8080;
const PORTIO = 8081;
// const serialPort = new SerialPort("/dev/ttyACM0", { baudRate: 28800 });
// Mac: /dev/cu.usbmodem14101
// Ubuntu: /dev/ttyACM0
// Win: COM3



const fs = require('fs').promises;
const fs2 = require('fs');
//process.stdin.resume();



// Timer:

const {performance} = require('perf_hooks');
var t0Net = performance.now();
var t0IO = performance.now()
var tNet = 0;
var tIO = 0;
var t0 = performance.now();
var t1 = 0.;
var t2 = 0.;
var firstCount = 1;
var cyclePeriods = [];
var netArr = [];
var ioArr = [];
//var t0 = 0;

// Shared array between the two sockets
var numArray = [];
var myTypedArray = [];
var nums = 0;
var prevNums = 0;
var msgNumber = 0;

var countNet = 0;
var countIO = 0;
var timeNet = 0;
var TimeIO = 0;

var stopCamera = 0;

var cameraStarts = 1;
// Server is listening to localhost:8081
server.listen(PORTIO, function(){
    console.log("Web Server Started: Go to 'http://localhost:8081' in your Browser");
});
// Initiate Public folder (from index.html)
app.use(express.static('public'));

//---------------------------socket.io----------------------------------------
io.on("connection", function(sockIO){
    console.log('Client (simulation) connected')
    //Receive data from public
    sockIO.on('led', function(data) {
        
        nums = data.value;
        if (nums !=0 ){
            
            console.log(countIO, ": ", nums-prevNums);
            //console.log("nums: ", nums, " = ", nums/33.3333, " frames");
            prevNums = nums;
            let buf = Buffer.allocUnsafe(4);
            buf.writeInt32LE(nums);
            //console.log(buf.toJSON());
            serialPort.write(buf);

            countIO++;
        }        
    }); 
    
    sockIO.on('disconnect', (reason) => {
        console.log('Client (simulation) disconnected')
    });
});
//------------------------End socket.io--------------------------------------


// ------------------------TCP Socket (NET)-----------------------------------
net.createServer(function(sockNet){
    console.log('CONNECTED: ', sockNet.remoteAddress + ':' + sockNet.remotePort);
    
    // Recieve data from cpp
    sockNet.on('data', async function(data) {
        // This works like a loop    
        msgNumber = JSON.parse(data);
        
        if (msgNumber==1){ //first messge
            console.log("First MSG");
            var buf = new Buffer.alloc(1);
            buf.writeUInt8(1);
            serialPort.write(buf);
        }
        if (msgNumber==0){
            console.log("Last MSG");

            let buf = Buffer.allocUnsafe(4);
            buf.writeInt32LE(0);
            //console.log(buf.toJSON());
            serialPort.write(buf);

        }

        // Sending data to the socket.io 
        io.emit('led', {value: msgNumber});
        // Node should return a message to cpp
        sockNet.write("Return String\n");   
    });
    // close TCP connection 
    sockNet.on('close', function(data) {
        console.log('CLOSED: ', sockNet.remoteAddress + ' ' + sockNet.remotePort);
    });
}).listen(PORTNET, HOST);
console.log('Server listening on ' + HOST + ":" + PORTNET);
//-------------------------------------END Net Socket-------------------------------


// Read the port data
serialPort.on("open", () => {
    console.log('serial port open-Arduino');
  });
  parser.on('data', data =>{
    console.log('from arduino:', data);
  });