/*
             CBC Mini DDM
 
Date:        8/24/2022
Author:      Charles Current
github:      https://github.com/currentc57 

Description:
An inexpensive and simple Digital Dial Magnifier.  
  
This code is written to be easy to understand and modify.  Readability has been prioritized over speed and code size optimization.
  
It is written to run on small rp2040 or ESP32-C3 based boards with builtin LCD displays and using a 1000 pul/rotation quadrature encoder.  
  https://www.aliexpress.us/item/3256804222218771.html 
  https://www.aliexpress.us/item/3256803057031062.html

However, this code avoids using micro-controller specific libraries, where possible, in order to make porting to other microcontrollers 
that support Arduino code relatively easy.  This should allow users to port the code to other Arduino compatible microcontrollers. 
  
This code is written to use either
  the Earle F. Philhower arduino-pico core https://github.com/earlephilhower/arduino-pico 
  When uploading, you will need to select the "Seeed XIAO RP2040" board.
or
  the Arduino core for the Espressif ESP32 https://github.com/espressif/arduino-esp32/
  When uploading, you will need to select the "ESP32C3 Dev Module" board.
  
The display code uses the Universal 8 bit Graphics Library by olikraus https://github.com/olikraus/u8g2/ 
  
The encoder code is very simplistic and can miss steps if turned very fast (faster than most people can turn a safe dial by hand).  It can be made more efficient with the use of code specific to the microcontroller used, but it would be more difficult to read and port to other microcontrollers.  
  
*/  
//------------------------------------------------------------------------------
#include <stdint.h>
#include <Wire.h>
#include <U8g2lib.h>                                        

//------------------------------------------------------------------------------
// these define the pins the encoder is connected to
#if defined(ARDUINO_SEEED_XIAO_RP2040) 
  #define ENC_A_PIN               4                                      
  #define ENC_B_PIN               3                                       
#elif defined(ARDUINO_ESP32C3_DEV)
  #define ENC_A_PIN               21                                      
  #define ENC_B_PIN               20                                      
#else 
  #error Unsupported board selection.  Please select (Seeed XIAO RP2040) or (ESP32C3 Dev Module)
#endif

// these are the pins for the builtin lcd display
#if defined(ARDUINO_SEEED_XIAO_RP2040)
  #define SCL_PIN                 23                                      
  #define SDA_PIN                 22                                        
#elif defined(ARDUINO_ESP32C3_DEV)
  #define SCL_PIN                 6                                       
  #define SDA_PIN                 5     
#endif

// //////////////// change these as needed for your application /////////////////
#define NUM_ON_DIAL             100.0                                     // change if your safe has more or less than 100 digits on the dial
#define ENC_PUL_ROTATION        4000L                                     // we count rising and falling edges, so use (mfr. listed p/r * 4)
// //////////////////////////////////////////////////////////////////////////////

#define PUL_PER_NUMBER          ((float)ENC_PUL_ROTATION / NUM_ON_DIAL)   // pulses of encoder per digit, used to make the code read better    

//------------------------------------------------------------------------------
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);  // tells the display library we are using an EastRising 0.42" OLED
                                                                          // display rotation can be changed using the constants U8G2_R0 = 0deg or U8G2_R2 = 180deg

//------------------------------------------------------------------------------
//  global variables
int32_t zero_position = 0;                                                // we zero on boot
float   dial_num = 0.0;                                                   // the current dial number 
volatile int32_t encoder_count = 0;                                       // encoder count 

//------------------------------------------------------------------------------
void aChange() {                                                          // interrupt routine for a change on encoder A pin
//                           _______         _______       
//               PinA ______|       |_______|       |______ PinA
// positive <<<          _______         _______         __      >>> negative
//               PinB __|       |_______|       |_______|   PinB

  switch (digitalRead(ENC_A_PIN)) {
    case HIGH : 
      if (digitalRead(ENC_B_PIN) == LOW) {                                // if pin A went high and pin B is low  
        encoder_count++;                                                  // we are turning CCW
      } else {
        encoder_count--;                                                  // if pin A went high and pin B is high
      }                                                                   // we are turning CW
      break;
      
    case LOW :
      if (digitalRead(ENC_B_PIN) == HIGH) {                               // if pin A went low and pin B is high
        encoder_count++;                                                  // we are turning CCW
      } else {
        encoder_count--;                                                  // if pin A went low and pin B is low
      }                                                                   // we are turning CW
      break;
  }
}

//------------------------------------------------------------------------------
void bChange() {                                                          // interrupt routine for a change on encoder B pin
//                           _______         _______       
//               PinA ______|       |_______|       |______ PinA
// positive <<<          _______         _______         __      >>> negative
//               PinB __|       |_______|       |_______|   PinB
                                                        
  switch (digitalRead(ENC_B_PIN)) {
    case HIGH :
      if (digitalRead(ENC_A_PIN) == HIGH) {                               // if pin B went high and pin A is high
        encoder_count++;                                                  // we are turning CCW
      } else {
        encoder_count--;                                                  // if pin B went high and pin A is low
      }                                                                   // we are turning CW
      break;
      
    case LOW :
      if (digitalRead(ENC_A_PIN) == LOW) {                                // if pin B went low and pin A is low
        encoder_count++;                                                  // we are turning CCW
      } else {
        encoder_count--;                                                  // if pin B went low and pin A is high
      }                                                                   // we are turning CW
      break;
  }  
}

//------------------------------------------------------------------------------
void encoderSetup(uint8_t a_pin, uint8_t b_pin) {                         // setting up the pins and interrupts for the encoder        
  
  pinMode(a_pin, INPUT_PULLUP);                                           // Configure the pins connected to the encoder to be inputs   
  pinMode(b_pin, INPUT_PULLUP);                                           // set to INPUT if the encoder drives outputs, INPUT_PULLUP if it doesn't
                                                                          // if in doubt, leave the pullups on. It shouldn't hurt anything
  
  attachInterrupt(digitalPinToInterrupt(ENC_A_PIN), aChange, CHANGE);     // setup interrupts for change on encoder pins
  attachInterrupt(digitalPinToInterrupt(ENC_B_PIN), bChange, CHANGE);     // so we can read rising and falling edges
}                                                                         // this gives the best encoder resolution (4x the listed pul/rotation)

//------------------------------------------------------------------------------
void dispSetup(uint8_t sda, uint8_t scl) {                                // setup the display and show the boot screens
  
#if defined(ARDUINO_SEEED_XIAO_RP2040)  
  Wire.setSDA(sda);                                                       // sets which rp2040 pins we are using for i2c
  Wire.setSCL(scl);                                                       
  Wire.begin();                                                           
#elif defined(ARDUINO_ESP32C3_DEV)
  Wire.begin(SDA_PIN, SCL_PIN);                                           // setup and start i2c on the ESP32-C3 board
#endif

  u8g2.begin();                                                           // initialize the display
  u8g2.setDrawColor(1);                                                   
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);                                                        
  
  u8g2.clearBuffer();	                                                    // clear the internal memory  
  u8g2.setFont(u8g2_font_luIS18_tf);                                      // set a font that looks good
  u8g2.drawStr(10,10,"CBC");                                              // write to the buffer
  u8g2.sendBuffer();                                                      // send the buffer to the display
  delay(2000);
  
  u8g2.clearBuffer();	                                                    // clear the internal memory                                     
  u8g2.setFont(u8g2_font_lubI14_tf);                                      // use a slightly smaller font to fit on the small screen
  u8g2.drawStr(5,0,"Mini");                                               // write to the buffer
  u8g2.drawStr(20,20,"DDM");
  u8g2.sendBuffer();                                                      // send the buffer to the display
  delay(2000);
}  

//------------------------------------------------------------------------------
void dispUpdate(float number) {                                           // display the dial number on the display
  static float prev_number = NUM_ON_DIAL / 2.0;                           // init to something other than zero to force initial display update
  
  if (number != prev_number) {                                            // to prevent flicker, only update the screen when needed    
    u8g2.clearBuffer();                                                   // clear the internal memory 
    u8g2.setFont(u8g2_font_inb16_mn);                                     // Set a good font for displaying numbers
    if (number < 10.0) {                                                  // change where whe start displaying the dial number
      u8g2.setCursor(10, 10);                                             // if it's less than 10, keeps its position close to center
    } else {
      u8g2.setCursor(2, 10);
    }
    u8g2.print(number);                                                   // print the dial position
    u8g2.sendBuffer();                                                    // send to the display
    prev_number = number;
  }
}  

//------------------------------------------------------------------------------
void posUpdate(const int32_t ENC_POSITION) {                              // calculate and update the dial position and direction
  static int32_t prev_position = 0;

  if (ENC_POSITION != prev_position) {                                    // if we've moved, we need to update the display number
    dial_num = ((float)((ENC_POSITION - zero_position) % ENC_PUL_ROTATION)) / PUL_PER_NUMBER; // remove full rotations and calculate what number we're on
    if (dial_num < 0.0) {                                                 // keep number in bounds
      dial_num += NUM_ON_DIAL;
    }
    prev_position = ENC_POSITION;                                         // save position for next time through
  }
}

//------------------------------------------------------------------------------
void setup(void) {  
  dispSetup(SDA_PIN, SCL_PIN);                                            // sets up the i2c display and displays the boot screens
  encoderSetup(ENC_A_PIN, ENC_B_PIN);                                     // sets up the encoder interrupts 
}

//------------------------------------------------------------------------------
void loop(void) {   
  posUpdate(encoder_count);                                               // calculate the dial position
  dispUpdate(dial_num);                                                   // updates the display
  delay(50);                                                              // waits 50 ms between updates, 
}                                                                         // bug-fix, if it updates too fast, the display may flicker or lock up

