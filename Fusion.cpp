#include <math.h> 
#include <TimeLib.h>

#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

const int motion_pin = 2; // Motion Sensor's Output pin
const int touch_pin_1 = 3; // Touch Sensor 1 Output pin 
const int touch_pin_2 = 5; // Touch Sensor 2 Output pin
const int led_pin = 4; // LED control pin
const int busy_pin = 6; // MP3 busy output pin


SoftwareSerial mySoftwareSerial(10, 11); // RX, TX pins for software serial 
DFRobotDFPlayerMini myDFPlayer; // Create MP3 player object

void printDetail(uint8_t type, int value); // Prototype for MP3 debug function

long current_rand; // Random number for selecting MP3 track
const int available_files = 148; // Number of MP3 files on SD card  
int detections = 0; // Counter for motion detections
long max_detections = 1; // Max detections before BD-1 responds
long last_detection; // Timestamp of last motion detection
bool sleep_mode = false; // Flag for sleep mode
long time_elapsed; // Time since last detection
int motion_threshold = 10; // Max detections in time interval before sleep
int time_interval = 20; // Time interval length in seconds 
long time_since_last_detection; // Elapsed time since last detection
bool lonely; // Flag if BD-1 is lonely
bool talkative = true; // Flag if BD-1 should talk or just beep
bool touch_sensor_output; // Flag for touch sensor trigger
bool touch_sensor_output_1; // Flag for touch sensor 1 trigger
bool touch_sensor_output_2; // Flag for touch sensor 2 trigger
bool toggle_mute = false; // Flag to toggle mute on/off

// Puts BD-1 into sleep mode after too many detections
void enterSleepMode(){

  Serial.println(F("BD-1 is overstimulated. Entering sleep mode..."));
  digitalWrite(led_pin, LOW); // Turn off LEDs
  
  bool sleeping = true;
  int counter = 60; // 1 minute sleep time
  
  while(sleeping){
    
    touch_sensor_output = digitalRead(touch_pin_1) || digitalRead(touch_pin_2);

    if(touch_sensor_output == HIGH){ // Wake up if touched

        digitalWrite(led_pin, HIGH);
        Serial.println( "BD-1 wakes up!" );
        talk(); // Say something
        delay(5000); 
        sleeping = false; 
    }

    else if(counter == 0){ // Wake up after 1 minute

      sleeping = false;
      Serial.println( "BD-1 wakes up!" );

    }

    else{

      delay(1000);
      counter--;
      Serial.println(counter);

    }
  }

}

// Returns true a certain percentage of the time
bool chance(int percentage){
  
  int rand = random(0, 101);
  return rand <= percentage;

}

// Plays a random sequence of MP3 tracks
void talk() {
  
  int length = random(1, 3); 
  int i = 0;

  while (i < length) {
    
    myDFPlayer.play(current_rand); // Play random track
    current_rand = random(1, available_files); // Get new random track
    i++;
    delay(1000);
  }
}

void setup() {

  pinMode(led_pin, OUTPUT);
  
  mySoftwareSerial.begin(9600); // Start software serial
  Serial.begin(115200);
  
  randomSeed(analogRead(0)); // Seed random number generator

  Serial.println();
  Serial.println(F("BD-1 Booting up..."));
  
  Serial.println(F("Initializing DF Mini Mp3 Player ..."));

  delay(2500); // Wait for MP3 module to start up
  
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    
    while(true){
      delay(0); // Wait forever
    }
  }
  
  Serial.println(F("DF Mini online."));
  
  myDFPlayer.volume(30);  // Set volume

  // Startup sequence
  current_rand = random(1, available_files); 
  myDFPlayer.play(current_rand);  
  digitalWrite(led_pin, HIGH);
  delay(2000);
  digitalWrite(led_pin, LOW);

  Serial.println(F("Calibrating Motion Sensor... (2 mins)"));

  delay(60000); // Calibrate motion sensor

  current_rand = random(1, available_files);
  myDFPlayer.play(current_rand);
  digitalWrite(led_pin, HIGH);
  delay(2000);
  digitalWrite(led_pin, LOW);

  Serial.println(F("60 s remaining..."));

  delay(60000); // Wait rest of 2 minutes

  Serial.println(F("Calibration complete! BD-1 is now online."));

}

void loop() {

  int motion_detection = digitalRead(motion_pin);
  touch_sensor_output_1 = digitalRead(touch_pin_1); 
  touch_sensor_output_2 = digitalRead(touch_pin_2);
  current_rand = random(1, available_files);
  long current_time = now();
  
  if(motion_detection == HIGH){

    Serial.println( "Motion detected!" );
    lonely = false;
    detections++;
    time_elapsed = current_time - last_detection;
    last_detection = now();
    talkative = chance(40);

    if(touch_sensor_output == HIGH){ // If touched

      Serial.println("Touch Detected!");
      digitalWrite(led_pin, HIGH);
      detections = 0;
      max_detections = random(4, 15);

      if(talkative){
        
        Serial.println( "BD-1 Talks!" );
        talk();
        delay(5000);

      }

      else{

        Serial.println( "BD-1 Beeps!" );
        myDFPlayer.play(current_rand);
        delay(5000);

      }
    }

    else if(detections == max_detections){ // If hit max detections
      
      detections = 0;
      digitalWrite(led_pin, HIGH);

      if(talkative){

        Serial.println( "BD-1 Talks!" );
        talk();
        delay(5000);
        max_detections = random(4, 15);

      }

      else{

        Serial.println( "BD-1 Beeps!" );
        myDFPlayer.play(current_rand);
        delay(5000);
        max_detections = random(4, 15);

      }

    }

    else if(detections >= motion_threshold && time_elapsed < time_interval){
      // Too many detections in time interval
      
      enterSleepMode();

    }

    else if(time_elapsed >= time_interval){
      // Eager to talk if not seen in a while

      digitalWrite(led_pin, HIGH);
      talk();
      Serial.println( "BD-1 Talks!" );
      delay(5000);
      max_detections = 3;

    }

    else{

      digitalWrite(led_pin, HIGH);
      delay(5000);

    }

  }
  
  else if(now() - last_detection >= time_interval){
    // Turn off lights after no motion

    digitalWrite(led_pin, LOW);

  }
  
  else if(touch_sensor_output_1 == HIGH){ // If chin touched

    Serial.println( "Touch detected!" );
    digitalWrite(led_pin, HIGH);
    talk();
    Serial.println( "BD-1 Talks!" );
    delay(5000);

  }

  else if(touch_sensor_output_2 == HIGH){ // If foot touched

    Serial.println( "Touch detected!" );
    digitalWrite(led_pin, HIGH);

    if(!toggle_mute){
      myDFPlayer.volume(0); 
      Serial.println("Muted!");
      toggle_mute = true; 
    }

    else{
      myDFPlayer.volume(30);
      Serial.println("Unmuted!");
      toggle_mute = false;
    }
    
    digitalWrite(led_pin, LOW);
    delay(5000);

  }

  else{

    Serial.println( "Nothing detected..." );

    time_since_last_detection = current_time - last_detection;

    if (time_since_last_detection >= 5 * 60 && !lonely) {
      
      lonely = true;
      myDFPlayer.play(current_rand);
      digitalWrite(led_pin, HIGH);  
      Serial.println("BD-1 feels lonely...");
      delay(5000);  
      digitalWrite(led_pin, LOW);
      
    }
    else {
      
      delay(1000); // Delay between checks

    }

  }

}
