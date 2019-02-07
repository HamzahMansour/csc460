#include <LiquidCrystal.h>

// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
int lcd_key     = 0;
int adc_key_in  = 0;

int x, y, z, x0, y0, x1=506, y1=514;
double z0, z1=1;
int JS1X=A8, JS1Y=A9, JS1Z=22;
int panSpeed = 0, maxAdjustPan = 40;
int tiltSpeed = 0, maxAdjustTilt = 10;
int sendByte;
const int LIGHT_PIN = A15;
const float VCC = 4.98;
const float R_DIV = 4660.0;
const float DARK_THRESHOLD = 100000.0;

void setup(){
  Serial.begin(9600);
  Serial1.begin(9600);
  pinMode(JS1Z, INPUT_PULLUP);

  // LCD things
  lcd.begin(16, 2);   // start the library
  lcd.setCursor(0,0);
  lcd.print("LGHT  JSX  JSY Z");

  // Light sensor
  pinMode(LIGHT_PIN, INPUT);

}

void writeTwo(int a, int b){
  Serial1.write(a);
  Serial1.write(b);
}

void panHandle(int panInput){
  int oldSpeed = panSpeed;
  if(panInput > 540){
    panSpeed = map(panInput, 540, 1021, 0, -maxAdjustPan);
  }
  else if (panInput < 440){
    panSpeed = map(panInput, 440, 0, 0, maxAdjustPan);
  }
  else{
    panSpeed = 0;
  }

  if(panSpeed != oldSpeed){
    sendByte = map(panSpeed, -maxAdjustPan, maxAdjustPan-1, 3, 84);
    writeTwo(1,sendByte);
  }
}

void tiltHandle(int tiltInput){
  int oldSpeed = tiltSpeed;

  if(tiltInput > 555){
    tiltSpeed = map(tiltInput, 555, 1022, 0, maxAdjustTilt);
  }
  else if(tiltInput < 425){
    tiltSpeed = map(tiltInput, 425, 0, 0, -maxAdjustTilt);
  }
  else{
    tiltSpeed = 0;
  }

  Serial.println(tiltSpeed);

  if(tiltSpeed != oldSpeed){
    sendByte = map(tiltSpeed, -maxAdjustTilt, maxAdjustTilt-1, 1, 22);
    Serial.println(tiltSpeed);
    writeTwo(2,sendByte);
  }
}

void loop(){
  // laser handling
  z0 = z1;
  z = digitalRead(JS1Z);
  z1 = z0 + ((z-z0)*0.3);
  z = (int)(z1 + 0.5);
  if(z==1) writeTwo(1,1);
  else if(z==0) writeTwo(1,2);

  // pan handling (x: 1,3 -> 1,44)
  x0 = x1;
  x = analogRead(JS1X);
  x1 = x0 + ((x-x0)>>1);
  panHandle(x1);

  //tilt handling
  y0 = y1;
  y = analogRead(JS1Y);
  y1 = y0 + ((y-y0)>>1);
  tiltHandle(y1);

  // LCD print joystick X,Y,Z
  lcd.setCursor(0,1);
  lcd.print("                ");
  lcd.setCursor(5,1);
  char printfStr[16];
  sprintf(printfStr,"%4d %4d %d",x1, y1,(int)(z1 + 0.5));
  lcd.print(printfStr);

  int lightADC = analogRead(LIGHT_PIN);
  if (lightADC > 0)
  {
    // Use the ADC reading to calculate voltage and resistance
    float lightV = lightADC * VCC / 1023.0;
    float lightR = R_DIV * (VCC / lightV - 1.0);
    // If resistance of photocell is greater than the dark
    if (lightR >= DARK_THRESHOLD){
      lcd.setCursor(0,1);
      lcd.print("SAFE");
    }
    else{
      lcd.setCursor(0,1);
      lcd.print("SHOT");
    }
  }


  delay(50);
  Serial1.write(0);
}
