  /////////////////////////////////////////////////////////////////
 //             Leah's Dashboard Display Module                 //
/////////////////////////////////////////////////////////////////

#include "GxGDEH029A1/GxGDEH029A1.cpp"
#include "GxIO/GxIO_SPI/GxIO_SPI.cpp"
#include "GxIO/GxIO.cpp"
#include "BitmapGraphics.h"
#include "can.h"
#include "gpio.h"

#include "Fonts/RobotoMono75.h"
#include "Fonts/RobotoMono16.h"
#define TEMPR 2
#define TEMPG 17
#define CURRR 18
#define CURRG 19
#define TEMPTHRESHY 67
#define TEMPTHRESHR 68
#define CURRTHRESHY 25
#define CURRTHRESHR 250
#define BUTTON1 34
#define BUTTON2 35

int speed = 0;
int temp = 0;
int curr = 0;

GxIO_Class io(SPI, 15, 22, 21);
GxEPD_Class display(io, 21, 16);


//initialize fonts for speed and SOC
const char* name1 = "RobotoMono75";
const GFXfont* speedFont = &Roboto_Mono_Bold_75;
const char* name2 = "RobotoMono16";
const GFXfont* SOCfont = &Roboto_Mono_Bold_16;
boolean msgRcvd;
boolean warning;
int input;

void setup() {
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
  
  //Initialize configuration structures using macro initializers
  can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(GPIO_NUM_4, GPIO_NUM_5, CAN_MODE_NORMAL);
  can_timing_config_t t_config = CAN_TIMING_CONFIG_100KBITS();
  can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

  //Install CAN driver
  if (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
      Serial.print("Driver installed\n");
  } else {
      Serial.print("Failed to install driver\n");
      return;
  }

  //Start CAN driver
  if (can_start() == ESP_OK) {
      Serial.print("Driver started\n");
  } else {
      Serial.print("Failed to start driver\n");
      return;
  }

  //initialize the display
  display.init();
  display.fillScreen(GxEPD_BLACK); //erases any previous screens
  //dashboard background image with battery icon
  display.drawExampleBitmap(gImage_dash, 0, 0, 128, 296, GxEPD_BLACK);
  display.update();
  display.setRotation(45);

  //initialize font attributes
  display.setTextColor(GxEPD_WHITE);

  msgRcvd=false; //for CAN messages
  randomSeed(88); //for random speed display
  input=0;
  warning=false;
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


void loop() {
  //Wait for message to be received
  can_message_t message;
  if (can_receive(&message, pdMS_TO_TICKS(5)) == ESP_OK) {
      Serial.print("Message received\n");
      msgRcvd=true;
  } else {
      Serial.print("Failed to receive message\n");
      msgRcvd=false;
  }
  
  //Process received message
  if(msgRcvd){
    Serial.println(message.identifier);
    if (message.flags & CAN_MSG_FLAG_EXTD) {
        Serial.print("Message is in Extended Format\n");
    } else {
        Serial.print("Message is in Standard Format\n");
    }
    Serial.print("ID is " + message.identifier);
    if(message.identifier==2014){ //my identifier is 7DE
      if (!(message.flags & CAN_MSG_FLAG_RTR)) {
          for (int i = 0; i < message.data_length_code; i++) {
              Serial.print("Data byte is ");
              Serial.println(message.data[i]);
              if(message.data[i]!=0){
                //warning has been detected, set input to warning code
                warning=true;
                input=message.data[i];
              }else if(message.data[i]==0){
                //no warnings detected
                if(warning){
                  //if in warning state, go back to home screen
                  warning=false;
                  input=0;
                  display.setRotation(0);
                  display.drawExampleBitmap(gImage_dash, 0, 0, 128, 296, GxEPD_BLACK);
                  display.setRotation(45);
                  display.update();
                }
              }
          }
      }
    }  
    msgRcvd=false;
  }
  
  //generate and display a random speed (FOR NOW)
  speed = random(15,25);
  
  //GET TEMP FROM CAN
  if (temp < TEMPTHRESHY){ //temp is in green zone
    digitalWrite(TEMPG, HIGH);
    digitalWrite(TEMPR, LOW);
  }else if (temp >= TEMPTHRESHR){ //temp is in red zone
    digitalWrite(TEMPG, LOW);
    digitalWrite(TEMPR, HIGH);
  }else{ //temp is in yellow zone
    digitalWrite(TEMPG, HIGH);
    digitalWrite(TEMPR, HIGH);
  }

  //GET CURR FROM CAN
  if (curr < CURRTHRESHY){ //curr is in green zone
    digitalWrite(CURRG, HIGH);
    digitalWrite(CURRR, LOW);
  }else if (curr >= CURRTHRESHR){ //curr is in red zone
    digitalWrite(CURRG, LOW);
    digitalWrite(CURRR, HIGH);
  }else{ //curr is in yellow zone
    digitalWrite(CURRG, HIGH);
    digitalWrite(CURRR, HIGH);
  }

  if (onePress || twoPress){
    Serial.println("one pressed!");
    
    //Configure message to transmit
    can_message_t messaget;
    messaget.identifier = 0xAAAA;
    messaget.data_length_code = 2;
    messaget.data[0]=onePress;
    messaget.data[1]=twoPress;
    
    //Queue message for transmission
    if (can_transmit(&messaget, pdMS_TO_TICKS(1000)) == ESP_OK) {
        printf("Message queued for transmission\n");
        Serial.print(messaget.data[1]);
        Serial.println(messaget.data[0]);
    } else {
        printf("Failed to queue message for transmission\n");
    }
    onePress = false;
    twoPress = false;
  }

 
  //if an error received on the monitor, display it
  if (warning){
    handleWarning(input);
  }

  //if no warning, just update speed
  if (!warning){
    showPartialUpdate(speed);
  }  
}

//partially update the screen to show the speed (and eventually SOC)
void showPartialUpdate(int speed)
{
  //set the speed and SOC strings
  String speedString = String(speed);
  String socString = String(75);
  
  //create box for speed value
  uint16_t speedBox_x = 98;
  uint16_t speedBox_y = 100;
  uint16_t speedBox_w = 150;
  uint16_t speedBox_h = 70;
  //set location for cursor for speed
  uint16_t speedCursor_y = speedBox_y-16;

  //create box for SOC
  uint16_t SOCbox_x = 5;
  uint16_t SOCbox_y = 24;
  uint16_t SOCbox_w = 59;
  uint16_t SOCbox_h = 20;
  //set location for cursor for SOC
  uint16_t SOCcursor_y = SOCbox_y-5;

  //cover previous values with black boxes
  display.fillRect(speedBox_x, speedBox_y - speedBox_h, speedBox_w, speedBox_h, GxEPD_BLACK);
  display.fillRect(SOCbox_x, SOCbox_y - SOCbox_h, SOCbox_w, SOCbox_h, GxEPD_BLACK);
  display.setFont(speedFont); //set font and size for speed
  display.setCursor(speedBox_x, speedCursor_y); //set the cursor for speed
  display.print(speedString); //print the speed 
  display.setFont(SOCfont); //set font and size for SOC
  display.setCursor(SOCbox_x+3, SOCcursor_y); //set cursor for SOC
  display.print(socString+"%"); //print SOC with %
  display.updateWindow(0, 0, 128, 296, false); //update the entire window
}

//method to handle the warnings received through CAN
void handleWarning(int warningNum){
  if (warningNum==1){
    //need to display "motor overheated" warning
    warning = true;
    display.setRotation(0);
    display.drawExampleBitmap(gImage_heat_warning, 0, 0, 128, 296, GxEPD_BLACK);
    display.setRotation(45);
    display.update();
    input=0;
  }
  else if (warningNum==2){
    //need to display "overcurrent" warning
    warning = true;
    display.setRotation(0);
    display.drawExampleBitmap(gImage_current_warning, 0, 0, 128, 296, GxEPD_BLACK);
    display.setRotation(45);
    display.update();
    input=0;
  }
  else if (warningNum==3){
    //need to display "SLoop open" warning
    warning = true;
    display.setRotation(0);
    display.drawExampleBitmap(gImage_sloop_warning, 0, 0, 128, 296, GxEPD_BLACK);
    display.setRotation(45);
    display.update();
    input=0;
    }
}
