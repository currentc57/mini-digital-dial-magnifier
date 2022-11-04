# cbc-mini-ddm
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
