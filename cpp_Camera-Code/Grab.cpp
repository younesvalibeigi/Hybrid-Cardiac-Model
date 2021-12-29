// Grab.cpp
/*
    Note: Before getting started, Basler recommends reading the "Programmer's Guide" topic
    in the pylon C++ API documentation delivered with pylon.
    If you are upgrading to a higher major version of pylon, Basler also
    strongly recommends reading the "Migrating from Previous Versions" topic in the pylon C++ API documentation.

    This sample illustrates how to grab and process images using the CInstantCamera class.
    The images are grabbed and processed asynchronously, i.e.,
    while the application is processing a buffer, the acquisition of the next buffer is done
    in parallel.

    The CInstantCamera class uses a pool of buffers to retrieve image data
    from the camera device. Once a buffer is filled and ready,
    the buffer can be retrieved from the camera object for processing. The buffer
    and additional image data are collected in a grab result. The grab result is
    held by a smart pointer after retrieval. The buffer is automatically reused
    when explicitly released or when the smart pointer object is destroyed.
*/

// Link to OpenCV WebSite for adjusting: https://medium.com/@subwaymatch/opencv-410-with-vs-2019-3d0bc0c81d96   ==> must be on debug mode
// link to how adapt opencv with the release mode

//TCP Socket includes
#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")


// Include files to use the pylon API.
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

// Include files to use exposure time control
#include <pylon/usb/BaslerUsbInstantCamera.h>

//Other include libraries
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include <stdio.h>
using namespace std::chrono;
#define _USE_MATH_DEFINES
//#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <sys/stat.h>
#include <math.h>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <ctime>
#include <sys/timeb.h>
#define M_PI 3.1415


/*---------------Function to create Gaussian filter------------------------*/
void FilterCreation(double GKernel[][5]) {
    // intialising standard deviation to 1.0 
    double sigma = 1.0;
    double r, s = 2.0 * sigma * sigma;
    // sum is for normalization 
    double sum = 0.0;
    // generating 5x5 kernel 
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            r = sqrt(x * x + y * y);
            GKernel[x + 2][y + 2] = (exp(-(r * r) / s)) / (M_PI * s);
            sum += GKernel[x + 2][y + 2];
        }
    }
    // normalising the Kernel 
    for (int i = 0; i < 5; ++i)
        for (int j = 0; j < 5; ++j)
            GKernel[i][j] /= sum;
}
/*-------------------------1D Gaussian-----------------------------------*/

void oneDFilter(float DKernel[5]) {
    // intialising standard deviation to 1.0 
    float sigma = 1.0;
    float r, s = 2.0 * sigma * sigma;
    // sum is for normalization 
    float sum = 0.0;
    // generating 5x5 kernel 
    for (int x = -2; x <= 2; x++) {
        r = sqrt(x * x);
        DKernel[x + 2] = (exp(-(r * r) / s)) / (M_PI * s);
        sum += DKernel[x + 2];
    }
    // normalising the Kernel 
    for (int i = 0; i < 5; ++i)
        DKernel[i] /= sum;
}
/*-------------------------End Gaussian-----------------------------------*/


//The final frame that ended
int finalZ = 0;
/*--------------------------Prompt_stoping camera--------------------------*/
bool stop = false;
bool save = true;

//bool stopNoSave = false;
// s: stop and save
// q: quit with no save
void stopCamera() {
    int a = getchar();
    while (a != 's' && a != 'q') {
        a = getchar();
    }

    if (a == 'q') save = false;
    stop = true;
}

using std::cout; using std::endl;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 10;

int main(int argc, char* argv[])
{
    cout << "Grab_1-Msg-Node" << endl;

    /*--------------------------Building and Connection of TCP Socket-----------------*/
    string ipAddress = "127.0.0.1";			// IP Address of the server
    int port = 8080;						// Listening port # on the server

    // Initialize WinSock
    WSAData data;
    WORD ver = MAKEWORD(2, 2);
    int wsResult = WSAStartup(ver, &data);
    if (wsResult != 0)
    {
        cerr << "Can't start Winsock, Err #" << wsResult << endl;
        return 0;
    }

    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        cerr << "Can't create socket, Err #" << WSAGetLastError() << endl;
        WSACleanup();
        return 0;
    }

    // Fill in a hint structure
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

    // Connect to server
    int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
    if (connResult == SOCKET_ERROR)
    {
        cerr << "Can't connect to server, Err #" << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 0;
    }
    /*------------------------------------END of TCP Socket---------------------------*/
    
    // The exit code of the sample application.
    int exitCode = 0;

    /*-----------Defining Algorithm Parameters-------------*/
    int xdim = 1920;
    int ydim = 1200;
    int* sixPrevData = new int[1920 * 1200 * 6];
    int* imgArr = new int[1920 * 1200];
    int* testArr = new int[1920 * 1200];

    /*--------Reading ROI's coordinate from txt file------*/ 
    fstream inputFile;
    inputFile = fstream("camera_input.txt", ios::in);
    int* arr_camera_input = new int[11];
    int cc_camera_input = 0;
    string theRow;
    while (getline(inputFile, theRow)) {
        // Output the text from the file
        arr_camera_input[cc_camera_input] = stoi(theRow);
        //cout << arr_camera_input[cc_camera_input] << endl;
        cc_camera_input++;
    }
    

    // Bottom Area Sec-------------------------
    int yEdgSec = arr_camera_input[0];//550;//600;
    int yEndSec = arr_camera_input[1];//850;//1100;

    int xEdgSec = arr_camera_input[2];//600;
    int xEndSec = arr_camera_input[3];//1300;
    int narrowSec = 2;
    int rowMeanSecLen = yEndSec - yEdgSec;
    float* rowMeanSec = new float[rowMeanSecLen];
    float* gRowMeanSec = new float[rowMeanSecLen];
    int* rowSumSec = new int[rowMeanSecLen];
    for (int i = 0; i < rowMeanSecLen; i++) {
        rowMeanSec[i] = 0.;
        rowSumSec[i] = 0;
        gRowMeanSec[i] = 0.;
    }
    //Top area---------------------------------
    int yEdg = arr_camera_input[4];//200;//100; //200;
    int yEnd = arr_camera_input[5];//500;//600; //500;

    int xEdg = arr_camera_input[6];//600;
    int xEnd = arr_camera_input[7];;//1300;
    int narrow = 2;
    int rowMeanLen = yEnd - yEdg;
    float* rowMean = new float[rowMeanLen];
    float* gRowMean = new float[rowMeanLen];
    int* rowSum = new int[rowMeanLen];
    for (int i = 0; i < rowMeanLen; i++) {
        rowMean[i] = 0.;
        rowSum[i] = 0;
        gRowMean[i] = 0.;
    }

    //Print conditions and refractory------------
    int printArea = arr_camera_input[8];
    int printRow = arr_camera_input[9];
    int refractoryAmount = arr_camera_input[10];
    
    
    /*--------------Reading threshold values------------*/
    fstream theThresholds;
    theThresholds = fstream("threshold.txt", ios::in);
    float* arr_threshold = new float[2];
    int cc_threshold = 0;
    string theRowTh;
    while (getline(theThresholds, theRowTh)) {
        // Output the text from the file
        arr_threshold[cc_threshold] = stof(theRowTh);
        //cout << arr_threshold[cc_threshold] << endl;
        cc_threshold++;
    }
    float threshold =  arr_threshold[0];
    float rowThreshold = arr_threshold[1];

    //---------------------------------------

    // Storing the time required to send the data
    int cRemTimes = 0;
    float* theRemTimes = new float[10];
    for (int i = 0; i < 10; i++) {
        theRemTimes[i] = 0.;
    }
    int sizeCV = 200;//100000;
    float* conductionVelocities = new float[sizeCV];
    int* cyclePeriods = new int[sizeCV];
    for (int i = 0; i < sizeCV; i++) {
        conductionVelocities[i] = 0.;
        cyclePeriods[i] = 0;
    }

    // Initializing the big array to store last n frames
    int numFrames = 500;
    int storeFrameSize = xdim * ydim * numFrames;
    short int* storeFrames = new short int[storeFrameSize];
    short int* shiftedFrames = new short int[storeFrameSize];
    for (int i = 0; i < storeFrameSize; i++) {
        storeFrames[i] = 0;
        shiftedFrames[i] = 0;

    }

    int oneArrSize = xdim * ydim;
    int z = 0;
    int zFrame = 0;
    int refractory = 0;
    int refractorySec = 0;
    int ReceivedWaveSec = 0;
    int cvNum = 0;

    int numFrRem = 0;
    int prevNumFrSinceStart = 0;
    int prevTimeSinceStart = 0;

    int z1sec = 0;
    int z2 = 0;
    int y1sec = 0;
    int y2 = 0;

    clock_t  t0, t1, t2;
    time_t current_time;
    auto prevTimeOfAct = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    prevTimeOfAct = prevTimeOfAct + 0;

    int sizeNumFrRemArray = 20;
    int* numFrRemArray = new int[sizeNumFrRemArray];
    for (int i = 0; i < sizeNumFrRemArray; i++) {
        numFrRemArray[i] = 0;
    }

    // 1D Gaussian
    float DKernel[5];
    oneDFilter(DKernel);
    for (int i = 0; i < 5; i++) {
        //cout << DKernel[i] << "\t";
    }
    cout << endl;

    /*-------------------END Parameters--------------------*/



    // The thread => it looks for user input at the same time the while loop is running
    thread thread_obj(stopCamera);


    //First signal
    if (z == 0) {
        // ----------------Sending Data through TCP Socket----------------
        int firstMsg = 1;
        char buf[4096];
        std::string s = std::to_string(firstMsg);
        char const* pchar = s.c_str();  //use char const* as target type


        // Send the text
        int sendResult = send(sock, pchar, (int)strlen(pchar), 0);
        if (sendResult != SOCKET_ERROR)
        {
            // Wait for response
            ZeroMemory(buf, 4096);
            int bytesReceived = recv(sock, buf, 4096, 0);
            if (bytesReceived > 0)
            {
                // Echo response to console
                //cout << "SERVER> " << string(buf, 0, bytesReceived) << endl;
            }

        }
        //--------------------------End TCP Send----------------------------
    }


    // Before using any pylon methods, the pylon runtime must be initialized. 
    PylonInitialize();

    try
    {
        // Create an instant camera object with the camera device found first.
        //CInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice());
        CBaslerUsbInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());

        // Print the model name of the camera.
        cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;



        // The parameter MaxNumBuffer can be used to control the count of buffers
        // allocated for grabbing. The default value of this parameter is 10.
        camera.MaxNumBuffer = 5;

        // Start the grabbing of c_countOfImagesToGrab images.
        // The camera device is parameterized with a default configuration which
        // sets up free-running continuous acquisition.
        //camera.StartGrabbing( c_countOfImagesToGrab);
        camera.StartGrabbing();

        /*-------------------------set FrameRate-----------------------------*/
        // Set the upper limit of the camera's frame rate to 30 fps
        float frameRate = 30.0;
        float frameP = 1000. / frameRate;
        camera.AcquisitionFrameRateEnable.SetValue(true);
        camera.AcquisitionFrameRate.SetValue(frameRate);

        /*------------------------Settings for the camera--------------------*/
        /*camera.LineSelector.SetValue(LineSelector_Line3);
        LineSelectorEnums e = camera.LineSelector.GetValue();

        camera.LineMode.SetValue(LineMode_Output);
        LineModeEnums e = camera.LineMode.GetValue();*/


        // This smart pointer will receive the grab result data.
        CGrabResultPtr ptrGrabResult;

        // Camera.StopGrabbing() is called automatically by the RetrieveResult() method
        // when c_countOfImagesToGrab images have been retrieved.
        while (camera.IsGrabbing() && !stop)
        {




            // Wait for an image and then retrieve it. A timeout of 5000 ms is used.
            camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

            // Image grabbed successfully?
            if (ptrGrabResult->GrabSucceeded())
            {
                
                // Access the image data.
                //cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
                //cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
                const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
                //cout << "Gray value of first pixel: " << (uint32_t) pImageBuffer[0] << endl << endl;
                /*--------------------------------Algorithm---------------------------------------*/
                //z=0,2,3,4,5,...
                int count = z % 6; // 0,1,2,3,4,5,0,1,2,3,4,5
                int countFrame = z % numFrames; //0,1,...,nameFrames-1,0,1,2
                finalZ = countFrame;

                for (int ix = 0; ix < xdim * ydim; ix++) {
                    int data = pImageBuffer[ix];
                    int indX = (count)*oneArrSize + ix;
                    int indFrame = (countFrame)*oneArrSize + ix;

                    testArr[ix] = data;

                    if (z >= 6) {
                        imgArr[ix] = abs(data - sixPrevData[indX]);
                        storeFrames[indFrame] = (short int)(abs(data - sixPrevData[indX])); //store the data for future observation
                    }
                    else {
                        storeFrames[indFrame] = 0; //put zero
                    }
                    sixPrevData[indX] = data;
                }

                if (z >= 6) {
                    
                    //==================================Going through the bottom area==============
                    float areaSumSec = 0.0;
                    
                    for (int y = yEdgSec; y < yEndSec; y++) {
                        int rowSumVal = 0;
                        for (int x = xEdgSec; x < xEndSec; x++) {
                            
                            int i = y * xdim + x;

                            int indX = (count)*oneArrSize + i;
                            int indFrame = (countFrame)*oneArrSize + i;

                            areaSumSec = areaSumSec + imgArr[i];
                            rowSumVal = rowSumVal + imgArr[i];
                            if (abs(x - xEdgSec) < narrowSec || abs(x - xEndSec) < narrowSec) { storeFrames[indFrame] = 150; }
                            if (abs(y - yEdgSec) < narrowSec || abs(y - yEndSec) < narrowSec) { storeFrames[indFrame] = 150; }
                        }
                        rowSumSec[y - yEdgSec] = rowSumVal;
                        //cout << "rowSumVal" << rowSumVal << "  rowSumSec[y]:   " << rowSumSec[y] << endl;
                        rowMeanSec[y - yEdgSec] = (float)rowSumVal / (xEndSec - xEdgSec);
                        gRowMeanSec[y - yEdgSec] = rowMeanSec[y - yEdgSec];
                    }
                    int xsideSec = xEndSec - xEdgSec;
                    int ysideSec = yEndSec - yEdgSec;
                    float areaMeanSec = areaSumSec / (xsideSec * ysideSec);
                    //======================================== END ==============================

                    //=============================Going through the second area=================
                    float areaSum = 0.0;
                    
                    for (int y = yEdg; y < yEnd; y++) {
                        int rowSumVal = 0;
                        for (int x = xEdg; x < xEnd; x++) {
                            //cout << imgArr[(y)*ydim + x];
                            //int i = (y)*ydim + x;
                            int i = y * xdim + x;

                            int indX = (count)*oneArrSize + i;
                            int indFrame = (countFrame)*oneArrSize + i;

                            areaSum = areaSum + imgArr[i];
                            rowSumVal = rowSumVal + imgArr[i];
                            if (abs(x - xEdg) < narrow || abs(x - xEnd) < narrow) { storeFrames[indFrame] = 150; }
                            if (abs(y - yEdg) < narrow || abs(y - yEnd) < narrow) { storeFrames[indFrame] = 150; }
                        }
                        rowSum[y - yEdg] = rowSumVal;
                        rowMean[y - yEdg] = (float)rowSumVal / (xEnd - xEdg);
                        gRowMean[y - yEdg] = rowMean[y - yEdg];
                    }
                    int xside = xEnd - xEdg;
                    int yside = yEnd - yEdg;
                    float areaMean = areaSum / (xside * yside);

                    


                    // Printing the area sums
                    //cout << "areaSum: " << areaSum << endl;
                    if (printArea == 1){
                    cout << "Area Mean: " << areaMean << ",   Area Sec: " << areaMeanSec << endl;
                    }
                    
                    // Printing the row sums
                    if (printRow == 1){
                        for (int i = 0; i < yEndSec - yEdgSec; i++) {
                        cout << fixed << setprecision(2) << rowMean[i] << "|";
                        }
                        cout << endl;
                    }




                    // float threshold =  10.6;
                    // float rowThreshold = 15.0;
                    // int refractoryAmount = 20;

                    /*
                    * There are four algorithm desinged to detect waves at the two defined area
                    * Among these four algorithms, algorithm4 showed the best performance
                    * Algorithm 1,2,3 are commented out
                    */



                    /*******************************************************
                    * Algirithm4: Camera-Arduino Connection (sending num Fr)
                    ********************************************************/
                    if (areaMeanSec > threshold && refractorySec == 0) {
                        ReceivedWaveSec = 1;
                        z1sec = z;
                        refractorySec = refractoryAmount;
                        //y=0 top, as y increase we come down
                        for (int i = 1; i < yEndSec - yEdgSec; i++) {
                            //cout << "rowthreshold: " << rowMeanSec[i] << endl;
                            if (rowMeanSec[i] >= rowThreshold && rowMeanSec[i - 1] < rowThreshold) {
                                y1sec = i + yEdgSec;
                                //cout << "y1sec: " << y1sec << endl;
                                break;
                            }
                        }
                    }

                    if (areaMean > threshold && refractory == 0 && ReceivedWaveSec == 1) {
                        z2 = z;
                        for (int i = 1; i < yEnd - yEdg; i++) {
                            //cout << "rowthreshold: " << rowMean[i] << endl;
                            if (rowMean[i] >= rowThreshold && rowMean[i - 1] < rowThreshold) {
                                y2 = i + yEdg;
                                //cout << "y2: " << y2 << endl;
                                break;
                            }
                        }
                        //----Calculating Conduction Velocity------
                        //int pixDist = y1sec - y2;
                        //cout << "pixDist:  " << pixDist << endl;
                        float conductionVelocity = (((float)(y1sec - y2)) / ((float)(z2 - z1sec)));//pix per fr
                        conductionVelocities[cvNum] = conductionVelocity;
                        //cout << cvNum <<": Conduction Veloctiy: " << conductionVelocity << " pix/ms \n";





                        // Calculate the remaining number of frames for sending the data through net socket
                        float frameRem = (y2 / conductionVelocity); // fr
                        int frameRemInt = (int)((frameRem));
                        //cout << "timeRem: " << timeRem << endl;

                        int numFrSinceStart = z + frameRemInt;

                        float timeRem = frameRem * (1000 / frameRate);
                        int timeRemInt = (int)(round(timeRem)); //ms
                        int timeSinceStart = z * (1000 / frameRate) + timeRemInt; //ms


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


                        




                        //Refresh#######
                        cvNum++;
                        ReceivedWaveSec = 0;
                        z1sec = 0;
                        z2 = 0;
                        y1sec = 0;
                        y2 = 0;


                        refractory = refractoryAmount;


                        int number = 1;
                    
                        //----------------Sending Data through TCP Socket----------------
                        char buf[4096];
                        std::string s = std::to_string(timeSinceStart);
                        char const* pchar = s.c_str();  //use char const* as target type


                        // Send the text
                        int sendResult = send(sock, pchar, (int)strlen(pchar), 0);
                        if (sendResult != SOCKET_ERROR)
                        {
                            // Wait for response
                            ZeroMemory(buf, 4096);
                            int bytesReceived = recv(sock, buf, 4096, 0);
                            if (bytesReceived > 0)
                            {
                                // Echo response to console
                                //cout << "SERVER> " << string(buf, 0, bytesReceived) << endl;

                            }

                        }

                        //--------------------------End Send----------------------------

                    }

                    /*********************************
                    * Algorithm3: look at every 3 rows
                    **********************************/
                    
                    /*
                    if (refractorySec == 0) {
                        for (int i = 5; i < yEndSec - yEdgSec; i=i+3) {
                            float prevThreeRowMean = (rowSumSec[i - 5] + rowSumSec[i - 4] + rowSumSec[i-3]) / (float)(3 * (xEndSec - xEdgSec));
                            float threeRowMean = (rowSumSec[i - 2] + rowSumSec[i - 1] + rowSumSec[i]) / (float)(3*(xEndSec - xEdgSec));
                            //cout << "threeRowMean: " << rowSumSec[i] << endl;
                            if (threeRowMean >= rowThreshold && prevThreeRowMean < rowThreshold) {
                                ReceivedWaveSec = 1;
                                z1sec = z;
                                refractorySec = refractoryAmount;

                                y1sec = i + yEdgSec;
                                cout << "y1sec: " << y1sec << endl;
                                break;
                            }
                        }
                    }
                    if (refractory == 0 && ReceivedWaveSec == 1) {
                        for (int i = 5; i < yEnd - yEdg; i = i + 3) {
                            float prevThreeRowMean = (rowSum[i - 5] + rowSum[i - 4] + rowSum[i - 3]) / (float)(3 * (xEnd - xEdg));
                            float threeRowMean = (rowSum[i - 2] + rowSum[i - 1] + rowSum[i]) / (float)(3 * (xEnd - xEdg));
                            //cout << "threeRowMean: " << rowSum[i] << endl;
                            if (threeRowMean >= rowThreshold && prevThreeRowMean < rowThreshold) {
                                z2 = z;
                                y2 = i + yEdg;
                                cout << "y2: " << y2 << endl;

                                //----Calculating Conduction Velocity------
                                int pixDist = y1sec - y2;
                                //cout << "pixDist:  " << pixDist << endl;
                                float conductionVelocity = pixDist / ((z2 - z1sec) * 1000 / frameRate);//pix per ms
                                conductionVelocities[cvNum] = conductionVelocity;
                                //cout << cvNum <<": Conduction Veloctiy: " << conductionVelocity << " pix/ms " << endl;
                                //Printing Cycle Period
                                if (cvNum == 0) {
                                    t1 = clock();
                                }
                                else {
                                    t2 = clock();
                                    cout << "cycle Period " << cvNum - 1 << ": " << double(t2 - t1) / double(CLOCKS_PER_SEC) * 1000 << endl;
                                    t1 = t2;
                                }



                                // Calculate the remaining number of frames for sending the data through net socket
                                float timeRem = (y2 / conductionVelocity); // ms
                                int timeRemInt = (int)timeRem;
                                //cout << "timeRem: " << timeRem << endl;

                                numFrRem = (int)(timeRem / (1000 / frameRate));

                                int numFrCC = cvNum % sizeNumFrRemArray;
                                numFrRemArray[numFrCC] = numFrRem;

                                // Timing
                                auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                auto timeOfAct = millisec_since_epoch + timeRemInt;
                                //cout << " Delta Time: " << timeOfAct << endl;

                                //Refresh#######
                                cvNum++;
                                ReceivedWaveSec = 0;
                                z1sec = 0;
                                z2 = 0;
                                y1sec = 0;
                                y2 = 0;


                                refractory = refractoryAmount;


                                //cout << "Active####" << endl;
                                int number = 1;
                                //Refresh refractory
                                //cout << "Area Mean: " << areaMean << endl;
                                //}
                                    //----------------Sending Data through TCP Socket----------------
                                char buf[4096];
                                std::string s = std::to_string(number);
                                char const* pchar = s.c_str();  //use char const* as target type


                                // Send the text
                                int sendResult = send(sock, pchar, (int)strlen(pchar), 0);
                                if (sendResult != SOCKET_ERROR)
                                {
                                    // Wait for response
                                    ZeroMemory(buf, 4096);
                                    int bytesReceived = recv(sock, buf, 4096, 0);
                                    if (bytesReceived > 0)
                                    {
                                        // Echo response to console
                                        //cout << "SERVER> " << string(buf, 0, bytesReceived) << endl;

                                    }

                                }

                                //--------------------------End Send----------------------------

                                break;
                            }
                        }
                    }*/


                    /********************************
                    * Algorithm 2: look at the rows
                    *********************************/

                    /*
                    if (refractorySec == 0) {
                        for (int i = 1; i < yEndSec - yEdgSec; i++) {
                            cout << "rowmeansec: " << rowMeanSec[i] << endl;
                            if (rowMeanSec[i] >= rowThreshold && rowMeanSec[i - 1] < rowThreshold) {
                                ReceivedWaveSec = 1;
                                z1sec = z;
                                refractorySec = refractoryAmount;

                                y1sec = i + yEdgSec;
                                cout << "y1sec: " << y1sec << endl;
                                break;
                            }
                        }
                    }


                    if (refractory == 0 && ReceivedWaveSec == 1) {
                        for (int i = 1; i < yEnd - yEdg; i++) {
                            cout << "rowmean: " << rowMean[i] << endl;
                            if (rowMean[i] >= rowThreshold && rowMean[i - 1] < rowThreshold) {
                                z2 = z;

                                y2 = i + yEdg;
                                cout << "y2: " << y2 << endl;

                                //----Calculating Conduction Velocity------
                                int pixDist = y1sec - y2;
                                //cout << "pixDist:  " << pixDist << endl;
                                float conductionVelocity = pixDist / ((z2 - z1sec) * 1000 / frameRate);//pix per ms
                                conductionVelocities[cvNum] = conductionVelocity;
                                //cout << cvNum <<": Conduction Veloctiy: " << conductionVelocity << " pix/ms " << endl;
                                //Printing Cycle Period
                                if (cvNum == 0) {
                                    t1 = clock();
                                }
                                else {
                                    t2 = clock();
                                    cout << "cycle Period " << cvNum - 1 << ": " << double(t2 - t1) / double(CLOCKS_PER_SEC) * 1000 << endl;
                                    t1 = t2;
                                }



                                // Calculate the remaining number of frames for sending the data through net socket
                                float timeRem = (y2 / conductionVelocity); // ms
                                int timeRemInt = (int)timeRem;
                                //cout << "timeRem: " << timeRem << endl;

                                numFrRem = (int)(timeRem / (1000 / frameRate));

                                int numFrCC = cvNum % sizeNumFrRemArray;
                                numFrRemArray[numFrCC] = numFrRem;

                                // Timing
                                auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                auto timeOfAct = millisec_since_epoch + timeRemInt;
                                //cout << " Delta Time: " << timeOfAct << endl;

                                //Refresh#######
                                cvNum++;
                                ReceivedWaveSec = 0;
                                z1sec = 0;
                                z2 = 0;
                                y1sec = 0;
                                y2 = 0;


                                refractory = refractoryAmount;


                                //cout << "Active####" << endl;
                                int number = 1;
                                //Refresh refractory
                                //cout << "Area Mean: " << areaMean << endl;
                                //}
                                    //----------------Sending Data through TCP Socket----------------
                                char buf[4096];
                                std::string s = std::to_string(number);
                                char const* pchar = s.c_str();  //use char const* as target type


                                // Send the text
                                int sendResult = send(sock, pchar, (int)strlen(pchar), 0);
                                if (sendResult != SOCKET_ERROR)
                                {
                                    // Wait for response
                                    ZeroMemory(buf, 4096);
                                    int bytesReceived = recv(sock, buf, 4096, 0);
                                    if (bytesReceived > 0)
                                    {
                                        // Echo response to console
                                        //cout << "SERVER> " << string(buf, 0, bytesReceived) << endl;

                                    }

                                }

                                //--------------------------End Send----------------------------


                                break;
                            }
                        }
                    }*/


                    /**********************************************************************
                    * Algorithm 1: look at area mean then look at rows <== We use this one
                    **********************************************************************/

                    /*
                    if (areaMeanSec > threshold && refractorySec == 0) {
                        ReceivedWaveSec = 1;
                        z1sec = z;
                        refractorySec = refractoryAmount;
                        //y=0 top, as y increase we come down
                        for (int i = 1; i < yEndSec - yEdgSec; i++) {
                            //cout << "rowthreshold: " << rowMeanSec[i] << endl;
                            if (rowMeanSec[i] >= rowThreshold && rowMeanSec[i - 1] < rowThreshold) {
                                y1sec = i + yEdgSec;
                                //cout << "y1sec: " << y1sec << endl;
                                break;
                            }
                        }
                    }
                    //int number = 0;
                    if (areaMean > threshold && refractory == 0 && ReceivedWaveSec==1) {
                        z2 = z;
                        for (int i = 1 ; i < yEnd - yEdg; i++) {
                            //cout << "rowthreshold: " << rowMean[i] << endl;
                            if (rowMean[i] >= rowThreshold && rowMean[i - 1] < rowThreshold) {
                                y2 = i + yEdg;
                                //cout << "y2: " << y2 << endl;
                                break;
                            }
                        }
                        //----Calculating Conduction Velocity------
                        int pixDist = y1sec - y2;
                        //cout << "pixDist:  " << pixDist << endl;
                        float conductionVelocity = pixDist / ((z2-z1sec) * 1000 / frameRate);//pix per ms
                        conductionVelocities[cvNum] = conductionVelocity;
                        //cout << cvNum <<": Conduction Veloctiy: " << conductionVelocity << " pix/ms \n";

                        //Printing Cycle Period
                        if (cvNum == 0) {
                            t1 = clock();
                        }
                        else {
                            t2 = clock();
                            cout << "cycle Period " << cvNum - 1 << ": " << double(t2 - t1) / double(CLOCKS_PER_SEC) * 1000 << endl;
                            t1 = t2;
                        }



                        // Calculate the remaining number of frames for sending the data through net socket
                        float timeRem = (y2 / conductionVelocity); // ms
                        int timeRemInt = (int)timeRem;
                        //cout << "timeRem: " << timeRem << endl;

                        numFrRem = (int)(timeRem / (1000 / frameRate));
                        int numFrSinceStart = z + numFrRem;


                        //int numFrCC = cvNum % sizeNumFrRemArray;
                        //numFrRemArray[numFrCC] = numFrRem;

                        // Send a data to the


                        // Timing
                        //auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                        //auto timeOfAct = millisec_since_epoch + timeRemInt;
                        //cout << " Time of act: " << timeOfAct << "\n";
                        //cout << " Prev Time of act: " << prevTimeOfAct << "\n";
                        //cout << "cycle periods: " << timeOfAct - prevTimeOfAct << "\n";
                        //prevTimeOfAct = timeOfAct;


                        //Refresh#######
                        cvNum++;
                        ReceivedWaveSec = 0;
                        z1sec = 0;
                        z2 = 0;
                        y1sec = 0;
                        y2 = 0;


                        refractory = refractoryAmount;


                        //cout << "Active####" << endl;
                        int number = 1;
                        //Refresh refractory
                        //cout << "Area Mean: " << areaMean << endl;
                        //}
                            //----------------Sending Data through TCP Socket----------------
                        char buf[4096];
                        std::string s = std::to_string(numFrSinceStart);
                        char const* pchar = s.c_str();  //use char const* as target type


                        // Send the text
                        int sendResult = send(sock, pchar, (int)strlen(pchar), 0);
                        if (sendResult != SOCKET_ERROR)
                        {
                            // Wait for response
                            ZeroMemory(buf, 4096);
                            int bytesReceived = recv(sock, buf, 4096, 0);
                            if (bytesReceived > 0)
                            {
                                // Echo response to console
                                //cout << "SERVER> " << string(buf, 0, bytesReceived) << endl;

                            }

                        }

                        //--------------------------End Send----------------------------

                    }*/


                    if (refractory > 0) { refractory--; }
                    if (refractorySec > 0) { refractorySec--; }

                }
                //cout << "z: " << z << endl;
                z++;
                

#ifdef PYLON_WIN_BUILD
                    // Display the grabbed image.
                //Pylon::DisplayImage(1, ptrGrabResult);
#endif


            }
            else
            {
                cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
            }
            //End the loop

        }//End of while

        //destroyAllWindows();
        // Ending the thread
        thread_obj.join();
        camera.StopGrabbing();
        cout << "last z: " << z << endl;
        cout << "While loop stoped" << endl;

        char secbuf[4096];
        int lastMsg = 0;
        std::string s = std::to_string(lastMsg);
        char const* pchar = s.c_str();  //use char const* as target type
        // Send the text
        int sendResult = send(sock, pchar, (int)strlen(pchar), 0);
        if (sendResult != SOCKET_ERROR)
        {
            // Wait for response
            ZeroMemory(secbuf, 4096);
            int bytesReceived = recv(sock, secbuf, 4096, 0);
            if (bytesReceived > 0)
            {
                // Echo response to console
                cout << "SERVER> " << string(secbuf, 0, bytesReceived);
                cout << "Final z = " << finalZ << endl;

            }

        }

        if (save)
        {

            // Shift the final frame to adjust its start and end
            //shiftedFrames
            int frameLenght = xdim * ydim;
            for (int currFrame = 0; currFrame < numFrames; currFrame++) {
                for (int i = 0; i < xdim * ydim; i++) {
                    int indxShifted = currFrame * frameLenght + i;
                    int indxStore = ((finalZ + 1 + currFrame) % numFrames) * frameLenght + i;

                    shiftedFrames[indxShifted] = storeFrames[indxStore];
                }
            }



            uint8_t* unsigShiftedFrames = new uint8_t[storeFrameSize];
            for (int i = 0; i < storeFrameSize; i++) {
                unsigShiftedFrames[i] = (unsigned short int)shiftedFrames[i];
            }

            uint32_t bubmode = _byteswap_ulong(2); //htonl makes sure the byte order is java friendly (bigendian)
            uint32_t bubzdim = _byteswap_ulong(numFrames);
            uint32_t bubxdim = _byteswap_ulong(xdim);
            uint32_t bubydim = _byteswap_ulong(ydim);



            //version == 0 double, 1 short
            //z,x,y

            fstream storeFile;
            storeFile = fstream("C:\\Users\\Younes Valibeigi\\Documents\\StoreData\\BubBinaryData", ios::out | ios::binary);
            storeFile.write(reinterpret_cast<char*>(&bubmode), sizeof(uint32_t));
            storeFile.write(reinterpret_cast<char*>(&bubzdim), sizeof(uint32_t));
            storeFile.write(reinterpret_cast<char*>(&bubydim), sizeof(uint32_t));
            storeFile.write(reinterpret_cast<char*>(&bubxdim), sizeof(uint32_t));
            //storeFile.write((char*)shiftedFrames, storeFrameSize * sizeof(short int));
            storeFile.write((char*)unsigShiftedFrames, storeFrameSize * sizeof(uint8_t));



            fstream storeCV;
            storeCV = fstream("conuction_velocity.csv", ios::out | ios::app);
            for (int i = 0; i < sizeCV - 1; i++) {
                storeCV << conductionVelocities[i] << ',';
            }
            storeCV << conductionVelocities[sizeCV - 1] << endl;


            fstream storeCP;
            storeCP = fstream("cycle_periods.csv", ios::out | ios::app);
            for (int i = 0; i < sizeCV - 1; i++) {
                storeCP << cyclePeriods[i] << ',';
            }
            storeCP << cyclePeriods[sizeCV - 1] << endl;

            delete[]  unsigShiftedFrames;
        }

        // Deleting the arrays for not freezing up the memory --> at the end of try block
        delete[] sixPrevData, imgArr, testArr, storeFrames, shiftedFrames;
        delete[] rowMeanSec, rowMean, theRemTimes, conductionVelocities, numFrRemArray, rowSum, rowSumSec;

    }
    catch (const GenericException& e)
    {
        // Error handling.
        cerr << "An exception occurred." << endl
            << e.GetDescription() << endl;
        exitCode = 1;
    }

    // Comment the following two lines to disable waiting on exit.
    cerr << endl << "Press q to exit." << endl;
    while (cin.get() != 'q');

    // Releases all pylon resources. 
    PylonTerminate();



    return exitCode;
}