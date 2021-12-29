 #include <Adafruit_DotStar.h>
#include <SPI.h>

#define NUMPIXELS 64 // Number of LEDs in strip

// Hardware SPI is a little faster, but must be wired to specific pins
// Arduino Uno = pin 11 for data, 13 for clock

Adafruit_DotStar matrix(NUMPIXELS, DOTSTAR_BRG);
uint32_t color = 0x0000FF;      // 'On' color (starts red)
// Colors:
  //axA900FF
  //0x0000FF blue
  //0x00FF00 red
  //0xggrrbb
  //color >>= 8 : shift the color 8 bits: 0xFF0000 -> 0x00FF00 (green-> red)


//Analog input
float camFrRate = 30.0;
float camFrP = (1000./camFrRate);
int analogPin = A3; // potentiometer wiper (middle terminal) connected to analog pin 3
                    // outside leads to ground and +5V
int val = 0;  // variable to store the value read
int prevVal = 0;

int z = 0;               

int arrZsize = 20;
long actZ[20];

int remTime[20];
long cyclePeriods[20];


int zcc1 = 0;
int zcc2 = 0;

int lightNum = 0;
int ledON = 0;
int changeFirstZCC1 = 1;

byte buf[4];

int firstMsg = 1;

void setup() {
  Serial.begin(9600);
  matrix.begin(); // Initialize pins for output
  matrix.setBrightness(255); //255
  matrix.show();  // Turn all LEDs off ASAP
}

int cc = 0;
long prevCurrTime = 0;

long inputInt = 0;
int ccInput = 0; // counter for every 4 data receives. It should update every for iteration
long prevInputInt = 0;

void loop() {
  prevVal = val;
  val = analogRead(analogPin);  // read the input pin
  //Serial.println(val); 
  
  if (val > 600 && prevVal<=600){// && firstMsg == 0){
    cc++;
    z++;
    if (ledON>0){
      ledON--;
    }
    //Printing the z
    /*
    if (z%10==0){
      String zString = String(z);
      Serial.println(zString);
    }
    */
  }

  if(z > actZ[zcc1%arrZsize] && changeFirstZCC1 == 1){
    changeFirstZCC1 = 0;
    zcc1++;
  }
    
  if (z == actZ[zcc1%arrZsize] && z>0){
    /*String strzcc1 = String(zcc1);
    Serial.print("zcc1: ");
    Serial.println(strzcc1);*/

    int theDelay = remTime[zcc1%arrZsize]*1000;//microSec
    long msdelay = remTime[zcc1%arrZsize];

    long currTime = (long)round(actZ[zcc1%arrZsize]*camFrP + remTime[zcc1%arrZsize]);
    long cycleP = currTime - prevCurrTime;
//    String strCycleP = String(cycleP);
    prevCurrTime = currTime;
//    String strCyclePeriod = String(cyclePeriods[zcc1%arrZsize]);
    actZ[zcc1%arrZsize]=0;
    remTime[zcc1%arrZsize]=0;
//    cyclePeriods[zcc1%arrZsize]=0;
    zcc1++;

    delayMicroseconds(theDelay);
    int c = 3;
    for (int i = 0; i<8;i++){
      if (i>3){
        matrix.setPixelColor(c, color);
      }
      c = c+8;
    }
     matrix.show();
     Serial.println("z==actZ");
     ledON = 3;
     //delay(100);
  }else if (ledON == 0){
    int c = 3;
    for (int i = 0; i<64;i++){
      matrix.setPixelColor(i, 0);
      c = c+8;
    }
     matrix.show();
     
  }

  
  // Receiving the first message from the server
  if (Serial.available() && firstMsg == 1){
    // This strategy looks nice but the delays in the loop will lose a few ms
    firstMsg = 0;
    byte firstInput;
    firstInput = Serial.read();
    String strFirstMsg = String(firstInput);
    Serial.print("First Msg: ");
    Serial.println(strFirstMsg);

  }

  // Receivng a message from the server
  if (Serial.available() && firstMsg == 0){
    //Serial.println("nodeInput");
    byte input;
    input = Serial.read();

    // Printing the result
    /*String strCC = String(ccInput);
    String strInput = String(input);
    String strInputInt = String(inputInt);
    Serial.print(strInputInt);
    Serial.print(" + ");
    Serial.print(strInput);
    Serial.print(" * (256^");
    Serial.print(strCC);
    Serial.print(")");
    Serial.println();*/
    

    // Building the input from the server
    if (ccInput == 0){inputInt = inputInt + input*1;}
    if (ccInput == 1){inputInt = inputInt + (long)input*256;}
    if (ccInput == 2){inputInt = inputInt + (long)input*65536;}
    if (ccInput == 3){inputInt = inputInt + (long)input*16777216;}
    //inputInt = inputInt + input*pow(256,ccInput);
    ccInput++;
    if(ccInput == 4){
      if (inputInt == 0){
        String zString = String(z);
        Serial.print("Last z: ");
        Serial.println(zString);
      }

      actZ[zcc2%arrZsize] = abs((long)((float)(inputInt)/camFrP));
      int temp = (int)(inputInt%100);
      remTime[zcc2%arrZsize] = abs((int)round(temp-((int)((float)temp/camFrP))*camFrP));


      // Printing the results
      /*String strActZ = String(actZ[zcc2%arrZsize]);
      String currZZ = String(z);
      Serial.print("recZ: ");
      Serial.print(strActZ);
      Serial.print(",   currZ: ");
      Serial.println(currZZ);
      String strRemTime = String(remTime[zcc2%arrZsize]);
      Serial.print("RemTime: ");
      Serial.println(strRemTime);*/

      zcc2++;

      inputInt = 0;
      ccInput = 0;

    }  
    
  } 
   
}
