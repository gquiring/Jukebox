/*

V1.0 10/20/2025
DY-SV5W music board paired with a ESP8266 to play randomizd MP3's with a LED ring

By Gary Quiring
Big thank you to the folks at Digital Town for their SV5W examples
https://www.digitaltown.co.uk/components18DYSV5W.php


Components required
DY-SV5W Music board
https://www.amazon.com/dp/B0BLCHH95W

Gikfun 2" 4 ohm 5watt speaker
https://www.amazon.com/dp/B081169PC5

WS2812B LED Ring 24 bits RGB (several choices)
https://www.amazon.com/Sparkleiot-Integrated-Drivers-Arduino-Raspberry/dp/B09K58DMMX
https://www.amazon.com/dp/B0B2D7742J

ESP8266 with USB-C
https://www.amazon.com/Development-Interface-Wireless-Compatible-Micropythonz/dp/B0D8S7XVWH

JST Connector for speaker (Optional)
https://www.amazon.com/gp/product/B01M5AHF0Z

USB-C Female connector (This does not support the data wires because it does not have the pull down resistors)
https://www.amazon.com/dp/B0CB395L99

Rocker switch (optional)
https://www.amazon.com/dp/B0CN8SLLPQ

Screws/nuts
 M3 20mm qty 1
 M3 10mm qty 2
 M3 nuts qty 3

3D files

Jukebox 3D project
https://makerworld.com/en/models/1261929-jukebox-night-light-and-music-box#profileId-1286728

Bracket for 8266 and DY-SV5W
https://makerworld.com/en/models/1867804-esp8266-arduino-dy-sv5w-music-mounting-bracket#profileId-1998763

////////  ARDUINO ////////

https://github.com/gquiring/Jukebox

Download the Arduino IDE:
https://www.arduino.cc/en/software

From the File, Preferences menu, install this additional Link in the board
Manager URL option:
http://arduino.esp8266.com/stable/package_esp8266com_index.json

If you can't see the COM port on Windows 11 try this driver, it worked for me:
https://sparks.gogo.co.nz/assets/_site_/downloads/CH34x_Install_Windows_v3_4.zip


Library versions used 
 ESP8266 board 3.1.2 
 Adafruit_NeoPixel 1.15.1


For power switch (Have not tested)
ESP.deepSleep(0);  // sleep indefinitely until reset pin is triggered
Then wire your momentary switch between RST and GND

*/

byte volume = 0x40;
byte commandLength;
byte command[6];
int checkSum = 0;


#include <Adafruit_NeoPixel.h>

#define LED_PIN     D1       // GPIO2 on ESP8266 (NodeMCU D4)
#define LED_COUNT   24       // Number of LEDs in the ring

Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);



void sendCommand(){
  int q;
  for(q=0;q < commandLength;q++){
    Serial1.write(command[q]);
    Serial.print(command[q],HEX);
  }
  Serial.println("End");
}


// Linear interpolation between two colors
uint32_t lerpColor(uint32_t color1, uint32_t color2, float t) {
  uint8_t r1 = (color1 >> 16) & 0xFF;
  uint8_t g1 = (color1 >> 8) & 0xFF;
  uint8_t b1 = color1 & 0xFF;

  uint8_t r2 = (color2 >> 16) & 0xFF;
  uint8_t g2 = (color2 >> 8) & 0xFF;
  uint8_t b2 = color2 & 0xFF;

  uint8_t r = r1 + (r2 - r1) * t;
  uint8_t g = g1 + (g2 - g1) * t;
  uint8_t b = b1 + (b2 - b1) * t;

  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

// Color sequence
uint32_t colors[] = {
  0xFF0000, // Red
  0x00FF00, // Green
  0x000064, // Dark Blue
  0xFFFF00, // Yellow
  0xFF0080, // Pink
  0x00FFFF, // Light Blue
  0xFFA500  // Orange
};

const int numColors = sizeof(colors) / sizeof(colors[0]);
const int stepsPerFade = 255;      // total steps for a fade
const int baseStepDelay = 20;      // normal delay (ms)
const float slowEndFactor = 3.0;   // last portion slowed by 3x
const float slowStart = 0.7;       // start slowing at 70% of transition
const uint8_t BRIGHTNESS = 120;    // You can make this brighter but some of the USB chargers starting dropping the voltage if the LED's draw too much power


void setup() {

  delay(1500);          // give DY-SV5W time to boot reliably on wall bricks
  Serial1.begin(9600);  //Open connection to music board

  //set volume to 35
  command[0] = 0xAA;    //first byte says it's a command
  command[1] = 0x13;
  command[2] = 0x01;
  command[3] = 35;      //volume
  checkSum = 0;
  for(int q=0;q<4;q++){
    checkSum +=  command[q]; 
  }
  command[4] = lowByte(checkSum);//SM check bit... low bit of the sum of all previous values
  commandLength = 5;
  sendCommand();

  
  //select random mode
  command[0] = 0xAA;//first byte says it's a command
  command[1] = 0x18;
  command[2] = 0x01;
  command[3] = 0x03;//random
  checkSum = 0;
  for(int q=0;q<4;q++){
    checkSum +=  command[q]; 
  }
  command[4] = lowByte(checkSum);//SM check bit... low bit of the sum of all previous values
  commandLength = 5;
  sendCommand();
  
  //play track (it always plays the first file 00001.MP3)
  command[0] = 0xAA;//first byte says it's a command
  command[1] = 0x02;
  command[2] = 0x00;
  command[3] = 0xAC;
  commandLength = 4;
  sendCommand();

  ring.begin();
  ring.setBrightness(BRIGHTNESS);
  ring.show();


}


void loop() {

  for (int i = 0; i < numColors; i++) {
    uint32_t startColor = colors[i];
    uint32_t endColor = colors[(i + 1) % numColors];

    // Cross-fade
    for (int step = 0; step <= stepsPerFade; step++) {
      float t = step / (float)stepsPerFade;
      uint32_t blended = lerpColor(startColor, endColor, t);
      setAllPixels(blended);

      // Extend the delay near the end of transition
      float stepDelay = baseStepDelay;
      if (t > slowStart) {
        float factor = 1.0 + (t - slowStart) / (1.0 - slowStart) * (slowEndFactor - 1.0);
        stepDelay *= factor;
      }
      delay((int)stepDelay);
    }
  }
  
}


// Set all LEDs to the same color
void setAllPixels(uint32_t color) {
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;

  for (int i = 0; i < LED_COUNT; i++) {
    ring.setPixelColor(i, r, g, b);
  }
  ring.show();
}



