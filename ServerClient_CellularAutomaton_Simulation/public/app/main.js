/*---------------Designed by Younes Valibeigi-------------*/
/*
for more info contact younes.valibeigi@mail.mcgill.ca
*/
console.log("Designed by Younes Valibeigi");
const t0 = performance.now();
define(['jquery',
        'Abubu/Abubu.js',
        'shader!compShader.frag',
        'shader!clickShader.frag',
        'shader!initShader.frag',
        'shader!baslerShader.frag'
        ],
        function($,
                Abubu,
                compShader,
                clickShader,
                initShader,
                baslerShader,
                ) {

"use strict";
var env = {
    // Model parameter .........................
    threshold   : 0.25,
    radius      : 4., //2.
    Lx          : 256, //512
    heterValue  : 0.5, 

    // Click parameter .........................
    clickRadius : 0.02,
    psize       : .5,
    feedbackCouner  : 0,
    
    // domain size .............................
    width:  256, //512
    height: 256, //512
    widthBas: 213,//640,
    heightBas: 160,//480,

    // closed loop ............................
    cameraFeedback:   0,
    timePerIter: 10, //10ms per iteration
    deltaT:     0,
    activate:  0,

    running: true,
    skip: 1,
    solving: false,
    colormap: 'rainbowHotSpring',

    solve: function(){
        env.running = !env.running;
    }

};
window.env = env;

/*-------------------- define Canvas------------------------*/
env.canvas_1 = document.getElementById('canvas');
env.canvas_1.width = env.width;
env.canvas_1.height= env.height;
env.canvas_1.style = "border:1px solid #000000;" ;
document.body.append(env.canvas_1) ;



/*------------------------define textures--------------------*/
// Initial Texture-----------------------------
function randomTable(w,h){
    var table = new Float32Array(w*h*4);
    var centX = w/2.;
    var centY = h/2.;
    var idx = 0 ;
    for(var j=0; j<w; j++){            // Along y-axis
        for(var i=0 ; i <h; i++){       // Along x-axis
            table[idx++] = env.psize*(Abubu.random()-env.heterValue); // red - Perturbation
            table[idx++] = env.psize*(Abubu.random()-env.heterValue); // green - Perturbation
            table[idx++] = 1. ; // blue - block
            table[idx++] = 0. ; // a
        }
    }
    return table ;
}
var table = randomTable(env.width,env.height);
env.txtInit1 = new Abubu.Float32Texture(env.width,env.height,{data:table}) ; //itxt
env.txtInit2 = new Abubu.Float32Texture(env.width,env.height) ; //itxts

// Cellular Automato Textures------------------
env.txtCA1 = new Abubu.Float32RTexture(env.width,env.height) ;
env.txtCA2 = new Abubu.Float32RTexture(env.width,env.height) ;



/*=============================Solvers================================ */
/*---------------------Initial Solver-------------------------*/
env.initSolver = new Abubu.Solver({
    fragmentShader  :   initShader,
    renderTargets   :{
        o_col_0 : { location : 0, target : env.txtCA1 } ,
        o_col_1 : { location : 1, target : env.txtCA2 } ,
    }
});
env.initSolver.render(); //run onece in the beginning
env.initialize = function(){ //run whenever we need it
    env.initSolver.render();
    env.time=0.;
}

/*---------------------Click Solver-------------------------*/
env.clickSolver = new Abubu.Solver({
    fragmentShader  :   clickShader,
    uniforms        :   {
        in_txt : { type : 't', value : env.txtCA1 } ,
        in_itxt: { type : 't', value : env.txtInit1} ,
        clickPosition : { type : 'v2', value : [0,0] } ,
        clickShiftPosition : { type : 'v2', value : [0,0] } ,  
        clickCtrlPosition  : { type : 'v2', value : [0,0]},
        clickRadius : { type : 'f', value : env.clickRadius } ,
        shiftClick : {type : 'b', value : false} ,
        ctrlClick  : {type : 'b', value : false},
    },
    renderTargets   :  {
        out_txt : { location : 0, target :  env.txtCA2 } ,
        out_itxt: { location : 1, target : env.txtInit2}
    }
});
env.copy1 = new Abubu.Copy(env.txtCA2, env.txtCA1) ;
env.copy2 = new Abubu.Copy(env.txtInit2, env.txtInit1) ;
env.clicker = function(){
    env.clickSolver.render();
    env.copy1.render();
    env.copy2.render();
};

var mouse1 = new Abubu.MouseListener({
    canvas  :  env.canvas_1,
    event   :   'drag',
    callback:   function(e){
        env.clickSolver.uniforms.clickPosition.value = e.uv;
        env.clickSolver.uniforms.shiftClick.value = false;
        env.clicker();
    } 
});
var mouse2 = new Abubu.MouseListener({
    canvas  :   env.canvas_1,
    event   :   'drag',
    shift   :   true,
    callback:   function(e){
        env.clickSolver.uniforms.clickShiftPosition.value = e.uv;
        env.clickSolver.uniforms.shiftClick.value = true;
        env.clicker();
    }
});
var mouse3 = new Abubu.MouseListener({
    canvas  :   env.canvas_1,
    event   :   'drag',
    ctrl   :   true,
    callback:   function(e){
        env.clickSolver.uniforms.clickCtrlPosition.value = e.uv;
        env.clickSolver.uniforms.ctrlClick.value = true;
        env.clicker();
    }
});

/*---------------------CA Solver-OneTimeStep------------------*/
var ComputeUniforms = function(inputTexture){
    this.in_txt = { type : 's', value : inputTexture } ;
    this.in_itxt = { type : 's', value : env.txtInit1 } ;
    this.radius       = { type : 'f', value : env.radius       } ;
    this.threshold      = { type : 'f', value : env.threshold      } ;
    this.Lx       = { type : 'f', value : env.Lx       } ;
    this.cameraFeedback = {type: 'i', value: env.cameraFeedback } ;
    return this ;
};
env.sovlerCA1 = new Abubu.Solver({
    fragmentShader  :   compShader,
    uniforms        :   new ComputeUniforms(env.txtCA1),
    renderTargets   :   {
        out_txt : {location: 0, target : env.txtCA2},
    }
});
env.sovlerCA2 = new Abubu.Solver({
    fragmentShader  :   compShader,
    uniforms        :   new ComputeUniforms(env.txtCA2),
    renderTargets   :   {
        out_txt : {location: 0, target : env.txtCA1},
    }
});

env.march = function(){
    env.sovlerCA1.render();
    env.sovlerCA2.render();
};
/*---------------------Read Texture and send to Node----------*/
env.matW = 8.;
env.matH = 8.;
env.yStep = env.width/env.matW; // =64
env.xStep = env.height/env.matH;
env.vThresh = 0.96;

// Chech 1 side points, if the wave hit one, send on data===============
env.detect = function(){
    env.matrixLED = 0; //Just one variable, no need for a matrix
    env.txtCA1Reader = new Abubu.TextureReader(env.txtCA1);
    env.txtCA2Reader = new Abubu.TextureReader(env.txtCA2);
    env.txtCA1Data = env.txtCA1Reader.read();
    env.txtCA2Data = env.txtCA2Reader.read();
    //console.log(env.txtCA2Data);
    
    var idx=0;
    var detectCond = 0;
    for (var idi = -10;idi<10;idi++){
        idx = env.width*4 + 4*env.width*(env.height/2+idi) -4;
        if(env.txtCA2Data[idx]>env.vThresh){
            detectCond = 1;
        }
    }
    if(detectCond==1){
        
        return 1;
    }else{
        return 0;
    }
};





/*============================Displaying Solvers======================*/
/*--------------------------CA solver--------------------------*/
env.displayCA = new Abubu.Plot2D({
    target          :   env.txtCA1,
    channel         :   'r',
    minValue        :   0.,
    enableMinColor  : true,
    minColor        : [1,1,1],
    maxValue        : 1.,
    colormap        : env.colormap,
    canvas          : env.canvas_1,  
});
env.displayCA.init();



/*-------------------Graphical User Interface---------------*/
function createGUI() {
    var gui = new Abubu.Gui();
    var pnl = gui.addPanel({width   : 250});
    var prm = pnl.addFolder('Model Parameters') ;
    prm.coefs = gui.addCoefficients( prm, env, ['radius','threshold', 'Lx', 'cameraFeedback'], [env.sovlerCA1, env.sovlerCA2] ) ;
    prm.open() ;
    var dsp = pnl.addFolder('Display') ;
    pnl.add(env, 'colormap', Abubu.getColormapList()).onChange(function(){
        env.displayCA.colormap = env.colormap;
    });
    dsp.add(env, 'skip').min(1).step(1) ;
    dsp.open() ;
    pnl.add(env, 'clickRadius').min(0.01).step(0.01).onChange(function(){
        env.clickSolver.uniforms.clickRadius.value = env.clickRadius ;
    } );
    pnl.add(env, 'timePerIter').name('Delay per Iter');
    pnl.add(env, 'initialize').name('Initialize') ;
    pnl.add(env, 'solve').name('Solve/Pause') ;
    var cam = pnl.addFolder('Camera') ;
    pnl.add(env, 'stopCamera').name('Stop Camera');
    //pnl.add(env, 'startRecording').name('Start Recording');
    //pnl.add(env, 'stopRecording').name('Stop Recording');
}

/*=======================Run and visualize the program=================*/
const socket = io.connect('http://localhost:8081');
var nodeInput = -1;
var firstDataFromCamera = 1;
/*--------------------socket communication-------------------*/
socket.on('led', function(data){
    nodeInput = data.value;
});
/*--------------------Stop the Camera------------------------*/
env.stopCamera = function(){
    socket.emit("stopCamera", {value: 1});
}



/*-------------------------RUN------------------------------ */

var z = 0;
const arrSize = 20;
var zPrev = [];
var cameraTimeSinceStart = [];
for (var i=0;i<arrSize;i++){
    zPrev.push(0);
    cameraTimeSinceStart.push(0);
}
var ccZ1 = 0;
var ccZ2 = 0;

var ref = 0;

function run(){ 
    if (env.running){
        for(var i=0; i<env.skip; i++){
            
            if(nodeInput==1){
                console.log("camera starts");
                env.cameraFeedback = 0;
            } else if (nodeInput==0){
                console.log("camera stops");
                env.cameraFeedback = 0;
            } else if (nodeInput>1){
                //console.log("recieving numFrSinceStart");
                env.cameraFeedback = 1;
                zPrev[ccZ1%arrSize] = z;
                cameraTimeSinceStart[ccZ1%arrSize] = nodeInput;
                ccZ1++;
            } else if (nodeInput<0){
                env.cameraFeedback = 0;
            }
            env.sovlerCA1.uniforms.cameraFeedback.value = env.cameraFeedback;
        
            if (env.detect()==1 && ref ==0){
                ref = 4;
                // Wave speed 1 micrometer/ms = 1 cell/ms ==> R = 4 cell = 4 ms/SimFr
                const simTime = parseInt((z-zPrev[ccZ2%arrSize])*env.radius * 2);
                const cameraTime = cameraTimeSinceStart[ccZ2%arrSize];
                var timeOfAct = simTime + cameraTime; //ms
                //console.log("z: ", z, "    zPrev: ", zPrev[ccZ2%arrSize]);
                console.log(ccZ2,simTime);
                socket.emit("led", {value: timeOfAct});
                ccZ2++;
            }
            if (ref>0){ref--;}
            env.march(); //2 iterations
            z = z + 2;
            

            nodeInput = -1;
            
        }
        
    }
    env.displayCA.render();
    requestAnimationFrame(run);
    
}


/*===================Initiate=====================================*/
// Ececute createGUI
createGUI();

// execute run
run();



                });