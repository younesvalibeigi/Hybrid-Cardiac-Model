/*---------------Designed by Younes Valibeigi-------------*/
/*
for more info contact younes.valibeigi@mail.mcgill.ca
*/
console.log("Designed by Younes Valibeigi");
const t0 = performance.now();
define(['jquery',
        'Abubu/Abubu.js'
        ],
        function($,
                Abubu,
                ) {

"use strict";

var env = {
    tau     :   100.

};

/*-------------------Graphical User Interface---------------*/
function createGUI() {
    var gui = new Abubu.Gui();
    var pnl = gui.addPanel({width   : 250});
    var prm = pnl.addFolder('Model Parameters') ;
    pnl.add(env, 'tau').name('Fixed Delay (Tau) in ms');
    prm.open() ;
}
// Ececute createGUI
createGUI();
/*=======================Run and visualize the program=================*/
const socket = io.connect('http://localhost:8081');
var nodeInput = 0;
function sendOne(){
    socket.emit("led", {value: 1});
}
function sendZero(){
    socket.emit("led", {value: 0});
}
/*--------------------socket communication-------------------*/
socket.on('led', function(data){
    nodeInput = data.value;
    if (nodeInput != 0){
        
        var newData = nodeInput + env.tau; //add 30 frames = 1000ms
        socket.emit("led", {value: newData});
        
        /* I don't know why but is seems if you
        put an input for the function in setTimeout
        it won't count the delay */
    }
});
                });