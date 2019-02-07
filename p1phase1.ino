#include <Servo.h>
#include <LiquidCrystal.h>

// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
int lcd_key     = 0;
int adc_key_in  = 0;

double z1 = 1;
double z0;
int x1 = 230;
int y1 = 506;
int z, x0,x, y0, y;
int JS1Z = 22;
int JS1X = A8;
int JS1Y = A10;
int PAN = 53; 
int TILT = 52;
int LZ = A7;
//tiltvars
int maxAdjustTilt = 10;
int tiltSpeed = 0;
static unsigned int tiltValue = 500;
//panvars
int maxAdjustPan = 40;
int panSpeed = 0;
static unsigned int panValue = 2100;
Servo panServo;
Servo tiltServo;

// Light sensor and LED
const int LIGHT_PIN = A15; // Pin connected to voltage divider output
const int LED_PIN = 26; // Use built-in LED as dark indicator
// Measure the voltage at 5V and the actual resistance of your
// 47k resistor, and enter them below:
const float VCC = 4.98; // Measured voltage of Ardunio 5V line
const float R_DIV = 4660.0; // Measured resistance of 3.3k resistor
// Set this to the minimum resistance require to turn an LED on:
const float DARK_THRESHOLD = 130000.0;

void setup() {
  cli();
  pinMode(JS1Z, INPUT_PULLUP);
  pinMode(JS1X, INPUT);
  pinMode(JS1Y, INPUT);
  pinMode(LZ, OUTPUT);
  Serial.begin(9600);
  panServo.attach(PAN, 750, 2100);
  tiltServo.attach(TILT, 500, 1000);
  // timer setup for tilt
  TCCR4A = 0;
  TCCR4B = 0;
  //Set prescaller to 256
  TCCR4B |= (1<<CS42);
  
  //Set TOP value (0.5 seconds)
  OCR4B = 1736;

  //Enable interupt A for timer 3.
  TIMSK4 |= (1<<OCIE4B);
  
  TCNT4 = 0;
  
  // timer setup for pan
  // clear timer config
  TCCR3A = 0;
  TCCR3B = 0;
  //Set prescaller to 256
  TCCR3B |= (1<<CS32);
  
  //Set TOP value (0.5 seconds)
  OCR3B = 1736;

  //Enable interupt A for timer 3.
  TIMSK3 |= (1<<OCIE3B);
  
  //Set timer to 0 (optional here).
  TCNT3 = 0;
  sei();

  // LCD things
  lcd.begin(16, 2);              // start the library
  lcd.setCursor(0,0);
  lcd.print("LGHT  JSX  JSY Z");

  // Light sensor and LED
  pinMode(LIGHT_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
 
}
// tilt handling
/*
At this point tilt doesnt appear to be working I'm going to head home also
there was some inexplicable shange in our x axis range
*/
ISR(TIMER4_COMPB_vect){
  int tiltInput = y1;
  int oldSpeed = tiltSpeed;
  if(tiltInput > 570){
    tiltSpeed = map(tiltInput, 570, 1021, 0, -maxAdjustTilt);
  }
  else if(tiltInput < 440){
    tiltSpeed = map(tiltInput, 440, 0, 0, maxAdjustTilt);
  }
  else{
    tiltSpeed = 0;
  }
  if(oldSpeed - tiltSpeed > 2){
    tiltSpeed = oldSpeed - 2;
  }
  else if( oldSpeed - tiltSpeed < -2){
    tiltSpeed = oldSpeed + 2;
  }
  tiltValue += tiltSpeed;
  if(tiltValue > 1000)
    tiltValue = 1000;
  if(tiltValue < 500)
    tiltValue = 500;

  if(tiltSpeed != 0)
    tiltServo.writeMicroseconds(tiltValue);
  TCNT4 = 0;
}
//pan handling
ISR(TIMER3_COMPB_vect){
  int panInput = x1;
  int oldSpeed = panSpeed;
  if(panInput > 575){
    panSpeed = map(panInput, 525, 1022, 0, -maxAdjustPan);
  }
  else if (panInput < 425){
    panSpeed = map(panInput, 425, 0, 0, maxAdjustPan);
  }
  else{
    panSpeed = 0;
  }

  if(oldSpeed - panSpeed > 2){
    panSpeed = oldSpeed - 2;
  }
  else if( oldSpeed - panSpeed < -2){
    panSpeed = oldSpeed + 2;
  }
  panValue += panSpeed;
  
  if(panValue > 2100)
    panValue = 2100;
  if(panValue < 750)
    panValue = 750;

  if (panSpeed != 0)
    panServo.writeMicroseconds(panValue);
  TCNT3 = 0;
}

void loop() {
  // lazer handling
  z0 = z1;
  z = digitalRead(JS1Z);
  z1 = z0 + ((z-z0)*0.3);
  z = (int)(z1 + 0.5);
  analogWrite(LZ, (int)(1-z)*200);
  // pan handling
  x0 = x1;
  x = analogRead(JS1X);
  x1 = x0 + ((x-x0)>>1);
  //tilt handling
  y0 = y1;
  y = analogRead(JS1Y);
  y1 = y0 + ((y-y0)>>1);

  // LCD print joystick X,Y,Z 
  lcd.setCursor(5,1);
  lcd.print("           ");//0000 0000 0");
  lcd.setCursor(5,1);
  char printfStr[16];
  sprintf(printfStr,"%4d %4d %d",x1, y1,(int)(z1 + 0.5));
  lcd.print(printfStr);

  // Light sensor and LED
  // Read the ADC, and calculate voltage and resistance from it
  int lightADC = analogRead(LIGHT_PIN);
  if (lightADC > 0)
  {
    // Use the ADC reading to calculate voltage and resistance
    float lightV = lightADC * VCC / 1023.0;
    float lightR = R_DIV * (VCC / lightV - 1.0);
    // If resistance of photocell is greater than the dark
    // threshold setting, turn the LED on.
    if (lightR >= DARK_THRESHOLD){
      digitalWrite(LED_PIN, LOW);
      lcd.setCursor(0,1);
      lcd.print("SAFE");
    }
    else{
      digitalWrite(LED_PIN, HIGH);
      lcd.setCursor(0,1);
      lcd.print("SHOT");
    }
  }

}
