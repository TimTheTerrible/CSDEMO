/*********************
  COUNTER STRIKE BOMB
**********************/

// include the library code:
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Keypad.h>
#include <Adafruit_MCP23017.h>
#include "LiquidCrystal.h"
#include "debugprint.h"

// Sound stuff
AudioPlaySdWav           playWav1;
AudioOutputI2S           audioOutput;
AudioConnection          patchCord1(playWav1, 0, audioOutput, 0);
AudioConnection          patchCord2(playWav1, 1, audioOutput, 1);
AudioControlSGTL5000     sgtl5000_1;

// Use these with the audio adaptor board
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

// Display stuff
LiquidCrystal lcd(0);

// These #defines make it easy to set the backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

#define MAX_LINE_LEN   17

// Sound stuff
#define SND_BEEP            0
#define SND_LETS_GO         1
#define SND_BOMB_PLANTED    2
#define SND_BOMB_DEFUSED    3
#define SND_T_WIN           4
#define SND_CT_WIN          5
#define SND_BOMB_DETONATED  6
#define MAX_SOUNDS          7

const char * fileNames[MAX_SOUNDS] = {
  "T01.WAV",
  "T02.WAV",
  "T03.WAV",
  "T04.WAV",
  "T05.WAV",
  "T06.WAV",
  "T07.WAV",
};

// Keypad stuff
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
const char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = {2, 1, 21, 20}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 4, 3}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

#define KEY_ZERO      0x39
#define KEY_ONE       0x30
#define KEY_TWO       0x31
#define KEY_THREE     0x32
#define KEY_FOUR      0x33
#define KEY_FIVE      0x34
#define KEY_SIX       0x35
#define KEY_SEVEN     0x36
#define KEY_EIGHT     0x37
#define KEY_NINE      0x38
#define KEY_ASTERISK  0x2A
#define KEY_POUND     0x23

// State stuff
#define NUM_STATES      6
#define STATE_SAFE      0
#define STATE_ARMING    1
#define STATE_ARMED     2
#define STATE_DISARMING 3
#define STATE_DISARMED  4
#define STATE_DETONATED 5

const char * StateNames[NUM_STATES] = {
  "SAFE",
  "ARMING",
  "ARMED",
  "DISARMING",
  "DISARMED",
  "DETONATED"
};

unsigned long timeToGoBOOM ;
int State;
int oldState;
char inputBuf[MAX_LINE_LEN] = "";
char armingCode[MAX_LINE_LEN] = "7355608";
int oldKey = NO_KEY;

// Main program

void playSound(int snd) {
  debugprint(DEBUG_TRACE,"Playing sound ID: %d ",snd);
  
  playWav1.play(fileNames[snd]);

  // A brief delay for the library read WAV info
  delay(5);

  // Simply wait for the file to finish playing.
  while (playWav1.isPlaying()) {
    // uncomment these lines if you audio shield
    // has the optional volume pot soldered
    //float vol = analogRead(15);
    //vol = vol / 1024;
    // sgtl5000_1.volume(vol);
  }
  debugprint(DEBUG_TRACE, "...done!");
}

void checkTimer() {

  if ( millis() >= timeToGoBOOM and ( State == STATE_ARMED or State == STATE_DISARMING ) ) {
      State = STATE_DETONATED;
      playSound(SND_BOMB_DETONATED);
  }
}

void updateDisplay () {
  char line1[MAX_LINE_LEN] = "";
  char line2[MAX_LINE_LEN] = "";  

  long minutes = long( (timeToGoBOOM - millis() ) / 60000 );
  char s_minutes[3]; sprintf(s_minutes, "%2ld", minutes);
  long seconds = long( ( ( timeToGoBOOM - millis() ) / 1000 ) % 60 );
  char s_seconds[3]; sprintf(s_seconds, "%2.2ld", seconds);


  // Do a little debug...
  if ( State != oldState ) {
    debugprint(DEBUG_TRACE,"******************************");
    debugprint(DEBUG_TRACE, "State changed to: %s", StateNames[State]);
    oldState = State;
  }

/*
  if ( State == STATE_ARMED or State == STATE_DISARMING ) {
    debugprint(DEBUG_TRACE,"Counting down:");
    debugprint(DEBUG_TRACE, "State: %s", StateNames[State]);
    debugprint(DEBUG_TRACE, "Time left: %s:%s", s_minutes, s_seconds);
    debugprint(DEBUG_TRACE, "timeToGoBOOM = %lu", timeToGoBOOM);
    debugprint(DEBUG_TRACE, "millis() = %lu", millis());
    debugprint(DEBUG_TRACE, "millis left = %lu", timeToGoBOOM - millis());
  }
*/

  // Prepare the display contents...
  switch ( State ) {
    case STATE_SAFE:
      sprintf(line1, "%-16s", "SAFE");
      sprintf(line2, "%-16s", "PUSH # TO ARM");
      break;
    case STATE_ARMING:
      sprintf(line1, "%-16s", "ARM CODE");
      sprintf(line2, "> %-14s", inputBuf);
      break;
    case STATE_ARMED:
      sprintf(line1, "%-11s%s:%s", "ARMED", s_minutes, s_seconds);
      sprintf(line2, "%-16s", "PUSH # TO DEFUSE");
      break;
    case STATE_DISARMING:
      sprintf(line1, "%-11s%s:%s", "DEFUSE CODE", s_minutes, s_seconds);
      sprintf(line2, "> %-14s", inputBuf);
      break;
    case STATE_DISARMED:
      sprintf(line1, "%-16s", "DEFUSED!");
      sprintf(line2, "%-16s", "PUSH # TO RESET");
      break;
    case STATE_DETONATED:
      sprintf(line1, "%-16s", "DETONATED!");
      sprintf(line2, "%-16s", "PUSH # TO RESET");
      break;
    default:
      strcpy(line1, "INVALID STATE");
      strcpy(line2, "BOMB IS BROKEN!");
      break;
  }

  // Update the display...
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
  // if ( in_input )
  //lcd.setCursor(0, 1);
  //lcd.cursor();
}

void handleInput() {  
  char key = NO_KEY;

  // Handle input
  key = keypad.getKey();
  if ( key != NO_KEY and key != oldKey ) {

    debugprint(DEBUG_TRACE,"Key pressed: %d", key);

    // TODO: Play key beep sound
    playSound(SND_BEEP);
  
    // Handle the input
    switch ( State ) {
      case STATE_SAFE:
        if ( key == KEY_POUND )
          State = STATE_ARMING;
          memset(inputBuf, 0x0, MAX_LINE_LEN);
        break;
      case STATE_ARMING:
        if ( key == KEY_POUND ) {
          
          // Did they enter the right code?
          if ( strncmp(inputBuf, armingCode, MAX_LINE_LEN) == 0 ) {
            State = STATE_ARMED;
            playSound(SND_BOMB_PLANTED);
            timeToGoBOOM = millis() + 300000;
            debugprint(DEBUG_TRACE,"handleInput() set State to STATE_ARMED"); 
            debugprint(DEBUG_TRACE,"handleInput() set timetoGoBOOM to %ld", timeToGoBOOM);
          }
          else
            State = STATE_SAFE;
        }
        else if ( key == KEY_ASTERISK ) {
          State = STATE_SAFE;
        }
        else {
          strncat(inputBuf, &key, 1);
        }
        break;
      case STATE_ARMED:
        if ( key == KEY_POUND )
          State = STATE_DISARMING;
          memset(inputBuf, 0x0, MAX_LINE_LEN);
          timeToGoBOOM = millis() + 30000;
          debugprint(DEBUG_TRACE,"handleInput() set State to STATE_ARMED"); 
          debugprint(DEBUG_TRACE,"handleInput() set timetoGoBOOM to %ld", timeToGoBOOM);
        break;
      case STATE_DISARMING:
        if ( key == KEY_POUND ) {
          if ( strncmp(inputBuf, armingCode, MAX_LINE_LEN) == 0 ) {
            State = STATE_DISARMED;
            playSound(SND_BOMB_DEFUSED);
            timeToGoBOOM = 0;
            debugprint(DEBUG_TRACE,"handleInput() set State to STATE_DISARMED"); 
            debugprint(DEBUG_TRACE,"handleInput() set timetoGoBOOM to 0");
          }
          else
            State = STATE_ARMED;
        }
        else if ( key == KEY_ASTERISK ) {
          State = STATE_ARMED;
        }
        else {
          strncat(inputBuf, &key, 1);
        }
        break;
      case STATE_DISARMED:
        if ( key == KEY_POUND ) {
          State = STATE_SAFE;
          playSound(SND_CT_WIN);
        }
        break;
      case STATE_DETONATED:
        if ( key == KEY_POUND ) {
          State = STATE_SAFE;
          playSound(SND_T_WIN);
        }
        break;
      default:
        break;
    }

    debugprint(DEBUG_TRACE,"inputBuf = '%s'", inputBuf);
  }

  oldKey = key;
}

void setup() {
  // Debugging output
  Serial.begin(9600);

  // Set up the sound...
  AudioMemory(20);

  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
      debugprint(DEBUG_TRACE,"Unable to access the SD card");
      delay(500);
    }
  }

  // Set up the LCD...
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);

  // Draw the welcome message...
  lcd.setCursor(0, 0);
  lcd.print("Counter Strike");
  lcd.setCursor(0, 1);
  lcd.print("Bomb V1.0");
  delay(2000);
  lcd.clear();

  //playSound(SND_BOMB_PLANTED);
  
  // Set up the state of the bomb...
  State = STATE_SAFE;
  oldState = State;

  playSound(SND_LETS_GO);
  debugprint(DEBUG_TRACE,"Starting!");
  debugprint(DEBUG_TRACE,"State: STATE_SAFE");
}

void loop() {
  // Handle Input
  handleInput();
  
  // Update Display
  updateDisplay();
  
  // Check Timer
  checkTimer();

}