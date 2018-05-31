/*
 * Fade
 * Using PWM to fade the onboard LED.
 */

const int ledPin = BUILTIN_LED;  // the onboard LED
int brightness = 0;        // how bright the LED is (0 = full, 512 = dim, 1023 = off)
int fadeAmount = 5;        // how many points to fade the LED by
const int delayMillis = 10;      // how long to pause between each loop

void setup() {
  // initialize onboard LED as output
}

void loop() {
  // set the LED brightness with analogWrite

  // increment/decrement the brightness for the next loop

  // limit to 10-bit (0-1023)

  // reverse the direction of the fading at each end
  if (brightness == 0 || brightness == 1023) {
    
  }

  // pause so you can see each brightness level
  delay(delayMillis);
}
