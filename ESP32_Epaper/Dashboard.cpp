//             Leah's Dashboard Display Module                 //
/////////////////////////////////////////////////////////////////


#include "Dashboard.h"
#include "GxGDEH029A1/GxGDEH029A1.cpp"
#include "GxIO/GxIO_SPI/GxIO_SPI.cpp"
#include "GxIO/GxIO.cpp"
#include "BitmapGraphics.h"
#include "gpio.h"
#include "Fonts/RobotoMono75.h"
#include "Fonts/RobotoMono16.h"


GxIO_Class io(SPI, 15, 22, 21);
GxEPD_Class display(io, 21, 16);


//initialize fonts for speed and SOC
const char* name1 = "RobotoMono75";
const GFXfont* speedFont = &Roboto_Mono_Bold_75;
const char* name2 = "RobotoMono16";
const GFXfont* SOCfont = &Roboto_Mono_Bold_16;


uint16_t *speed;
uint16_t *temp;
uint16_t *curr;
boolean warning;
uint16_t *warningNum;
uint16_t *SOC;


void Dashboard::setupDashboard() {
  //Initialize LED outputs
  pinMode(TEMPR, OUTPUT);
  pinMode(TEMPG, OUTPUT);
  pinMode(CURRR, OUTPUT);
  pinMode(CURRG, OUTPUT);
  //Initialize buttons as inputs
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON1), buttonOne, RISING);
  attachInterrupt(digitalPinToInterrupt(BUTTON2), buttonTwo, RISING);
  Serial.begin(115200);

  //initialize the display
  display.init();
  display.fillScreen(GxEPD_BLACK); //erases any previous screens
  //dashboard background image with battery icon
  display.drawExampleBitmap(gImage_dash, 0, 0, 128, 296, GxEPD_BLACK);
  display.update();
  display.setRotation(45);

  //initialize font attributes
  display.setTextColor(GxEPD_WHITE);
  randomSeed(88); //for random speed display
  warningNum = &OD_warningNum;
  speed = &OD_speed;
  temp = &OD_temperature;
  curr = &OD_current;
  warning = false;
  SOC = &OD_SOC;
}

//constructor
Dashboard::Dashboard() {

}

void Dashboard::startDashboard() {
      setupDashboard();
  for (;;) {
  runDisplay();
  }
  
}

boolean onePress = false;
boolean twoPress = false;
boolean buttonPress = onePress || twoPress;
uint8_t dbDelay = 1000;

void buttonOne() //interrupt with debounce
{
  volatile static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = micros();
  if (interrupt_time - last_interrupt_time > dbDelay) {
    onePress = true;
  }
  else {
    onePress = false;
  }
  last_interrupt_time = interrupt_time;
}

void buttonTwo() //interrupt with debounce
{
  volatile static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = micros();
  if (interrupt_time - last_interrupt_time > dbDelay) {
    twoPress = true;
  }
  else {
    twoPress = false;
  }
  last_interrupt_time = interrupt_time;
}


void Dashboard::runDisplay() {

  //GET TEMP FROM CAN
  if (*temp < TEMPTHRESHY) { //temp is in green zone
    digitalWrite(TEMPG, HIGH);
    digitalWrite(TEMPR, LOW);
  } else if (*temp >= TEMPTHRESHR) { //temp is in red zone
    digitalWrite(TEMPG, LOW);
    digitalWrite(TEMPR, HIGH);
  } else { //temp is in yellow zone
    digitalWrite(TEMPG, HIGH);
    digitalWrite(TEMPR, HIGH);
  }

  //GET CURR FROM CAN
  if (*curr < CURRTHRESHY) { //curr is in green zone
    digitalWrite(CURRG, HIGH);
    digitalWrite(CURRR, LOW);
  } else if (*curr >= CURRTHRESHR) { //curr is in red zone
    digitalWrite(CURRG, LOW);
    digitalWrite(CURRR, HIGH);
  } else { //curr is in yellow zone
    digitalWrite(CURRG, HIGH);
    digitalWrite(CURRR, HIGH);
  }

  if (onePress || twoPress) {
    Serial.println("button pressed!");

    onePress = false;
    twoPress = false;
  }


  //if an error received on the monitor, display it
  if (warning) {
    handleWarning();
  }else {
    showPartialUpdate(); //if no warning, just update speed
  }
}

//partially update the screen to show the speed (and eventually SOC)
void Dashboard::showPartialUpdate()
{
  OD_speed = random(16,25);
  //set the speed and SOC strings
  String speedString = String(*speed);
  String socString = String(*SOC);

  //create box for speed value
  uint16_t speedBox_x = 98;
  uint16_t speedBox_y = 100;
  uint16_t speedBox_w = 150;
  uint16_t speedBox_h = 70;
  //set location for cursor for speed
  uint16_t speedCursor_y = speedBox_y - 16;

  //create box for SOC
  uint16_t SOCbox_x = 5;
  uint16_t SOCbox_y = 24;
  uint16_t SOCbox_w = 59;
  uint16_t SOCbox_h = 20;
  //set location for cursor for SOC
  uint16_t SOCcursor_y = SOCbox_y - 5;

  //cover previous values with black boxes
  display.fillRect(speedBox_x, speedBox_y - speedBox_h, speedBox_w, speedBox_h, GxEPD_BLACK);
  display.fillRect(SOCbox_x, SOCbox_y - SOCbox_h, SOCbox_w, SOCbox_h, GxEPD_BLACK);
  display.setFont(speedFont); //set font and size for speed
  display.setCursor(speedBox_x, speedCursor_y); //set the cursor for speed
  display.print(speedString); //print the speed
  display.setFont(SOCfont); //set font and size for SOC
  display.setCursor(SOCbox_x + 3, SOCcursor_y); //set cursor for SOC
  display.print(socString + "%"); //print SOC with %
  display.updateWindow(0, 0, 128, 296, false); //update the entire window
}

//method to handle the warnings received through CAN
void Dashboard::handleWarning() {
  if (*warningNum == 1 && !warning) {
    //need to display "motor overheated" warning
    warning = true;
    display.setRotation(0);
    display.drawExampleBitmap(gImage_heat_warning, 0, 0, 128, 296, GxEPD_BLACK);
    display.setRotation(45);
    display.update();
  }
  else if (*warningNum == 2 && !warning) {
    //need to display "overcurrent" warning
    warning = true;
    display.setRotation(0);
    display.drawExampleBitmap(gImage_current_warning, 0, 0, 128, 296, GxEPD_BLACK);
    display.setRotation(45);
    display.update();
  }
  else if (*warningNum == 3 && !warning) {
    //need to display "SLoop open" warning
    warning = true;
    display.setRotation(0);
    display.drawExampleBitmap(gImage_sloop_warning, 0, 0, 128, 296, GxEPD_BLACK);
    display.setRotation(45);
    display.update();
  }
}
