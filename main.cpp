#include <LCD_I2C.h>
#include "notes.h"
#include "doom.cpp"
#include "pacman.cpp"
#include "tetris.cpp"
#include "mario.cpp"
#include "rick.cpp"
#include "takeonme.cpp"

#define NOCHANGE 0
#define RISINGEDGE 1
#define FALLINGEDGE 2

int* currentSong = 0;
String currentSongName = "DOOM ";
uint8_t currentSongNumber = 1;
int songCount = 6;

//PINS
const int buzzerPin = A0;
const int ptMeterPin = A2;
const int backButton = 5;
const int pauseButton = 4;
const int nextButton = 3;
const int menuButton = 2;

//BUTTON STATES
bool backState = 1;
bool nextState = 1;
bool pauseState = 1;
bool menuState = 1;

//LCD SCREEN
LCD_I2C lcd(0x27, 16, 2);
int row = 0;
int column = 2;
int iteration = 0;


//Millis() to calculate delay
unsigned long previousMillis = 0;

//MUSIC
uint8_t noteChar[8] =
{
  B00001,
  B00011,
  B00101,
  B01001,
  B01001,
  B01011,
  B11011,
  B11000
};

int notes = sizeof(doom) / sizeof(doom[0]) / 2;

int tempo = 250;
int min_tempo = 150;
int max_tempo = 350;

int wholenote = (60000 * 4) / tempo;

int divider = 0, noteDuration = 0;
const int duration = 200;

bool isMenu = false; 


void setupScreen(){
    lcd.begin();
    lcd.backlight();
    lcd.createChar(0, noteChar);
    lcd.write(0);
    Serial.begin(9600);
}


void setupButtons(){
    pinMode(backButton, INPUT_PULLUP);
    pinMode(pauseButton, INPUT_PULLUP);
    pinMode(nextButton, INPUT_PULLUP);
    pinMode(menuButton, INPUT_PULLUP);
}


bool debounceButton(int pin ,bool state){
  bool newState = digitalRead( pin );
  if( state != newState){
    delay(10);
    newState = digitalRead( pin );
  }
  return newState;
}


int btnStateChange(int pin, bool* state){
  if( debounceButton(pin, *state) == LOW && *state == HIGH){
    *state = LOW;
    return FALLINGEDGE;
  }
  if( debounceButton(pin, *state) == HIGH && *state == LOW){
    *state = HIGH;
    return RISINGEDGE;
  }
  return NOCHANGE;
}



void changeTempo(){
  int value = analogRead(ptMeterPin);
  value = map(value, 0, 1023, min_tempo, max_tempo);
  tempo = value;
  wholenote = (60000 * 4) / tempo;
}


void setSongSettings(int songTable[], int tableSize, int minTempo, int maxTempo){
  currentSong = songTable;
  notes = tableSize / sizeof(songTable[0]) / 2;
  min_tempo = minTempo;
  max_tempo = maxTempo;
}


void changeSong(){

  if( btnStateChange(nextButton, &nextState) == FALLINGEDGE){
    currentSongNumber++;
    iteration = 0;
    tone(buzzerPin, NOTE_A2, 75);
  }
  if( btnStateChange(backButton, &backState) == FALLINGEDGE){
    currentSongNumber--;
    iteration = 0;
    tone(buzzerPin, NOTE_A2, 75);
  }

  if( currentSongNumber == songCount + 1 ) currentSongNumber = 1;
  if( currentSongNumber == 0 ) currentSongNumber = songCount;

  switch( currentSongNumber ){
    case 1: currentSongName = " DOOM BY JADE ARCADE ";
            setSongSettings(doom, sizeof(doom), 150, 350);
            break;
    case 2: currentSongName = " ARCADE PACMAN MUSIC ";
            setSongSettings(pacman, sizeof(pacman), 50, 105);
            break;
    case 3: currentSongName = " TETRIS NES MUSIC ";
            setSongSettings(tetris, sizeof(tetris), 100, 200);
            break;
    case 4: currentSongName = " SUPER MARIO BROS THEME ";
            setSongSettings(mario, sizeof(mario), 120, 220);
            break;
    case 5: currentSongName = " NEVER GONNA GIVE YOU UP ";
            setSongSettings(rick, sizeof(rick), 90, 150);
            break;
    case 6: currentSongName = " TAKE ON ME, ON ME TAKE ";
            setSongSettings(takeonme, sizeof(takeonme), 100, 170);
            break;        
  }

}


void pause(){
  while(true){
    if( btnStateChange(pauseButton, &pauseState) == FALLINGEDGE){
      break;
    }
    if( btnStateChange(menuButton, &menuState) == FALLINGEDGE){
      isMenu = true;
      break;
    }
  }
}


void checkUserInput(){

  if( btnStateChange(backButton, &backState) == FALLINGEDGE){
    currentSongNumber--;
    changeSong();
    Serial.println("Back");
  }

  if( btnStateChange(pauseButton, &pauseState) == FALLINGEDGE){
    pause();
    Serial.println("Pause");
  }

  if( btnStateChange(nextButton, &nextState) == FALLINGEDGE){
    currentSongNumber++;
    changeSong();
    Serial.println("Next");
  }

  if( btnStateChange(menuButton, &menuState) == FALLINGEDGE){
    isMenu = true;
    Serial.println("Menu");
  }
}


void drawMusicNotes(){
  lcd.setCursor(0,0);
  lcd.print(' ');
  lcd.setCursor(0,1);
  lcd.print(' ');
  lcd.setCursor(15,0);
  lcd.print(' ');
  lcd.setCursor(15,1);
  lcd.print(' ');
   if(row == 1)
      row = 0;
    else
      row = 1;
    lcd.setCursor(0,row);
    lcd.write(0);
    lcd.setCursor(15,row);
    lcd.write(0);
}

void drawSongName(String songName){
  int len = songName.length();

  if(iteration > len)
    iteration = 0;

  lcd.setCursor(2,1);
  for(int i = 2; i < 14; i++){
    lcd.print(songName[(iteration + i) % len]);
  }

  iteration++;
}

void drawText(){
  lcd.setCursor(1,0);
  lcd.print("|NOW PLAYING:|");
  lcd.setCursor(1,1);
  lcd.print("|");

  drawSongName(currentSongName);

  lcd.setCursor(14,1);
  lcd.print("|");
  column++;
}


void delayBetweenNotes(int expectedDuration){

    unsigned long startedMillis = millis();
    unsigned long currentMillis = millis();

    while(currentMillis - startedMillis < expectedDuration){

      checkUserInput();
      changeTempo();

      currentMillis = millis();
      if(currentMillis - previousMillis > duration){

        drawText();

        previousMillis = millis();
      }

    }
}


bool delayMenu(int expectedDuration){

    unsigned long startedMillis = millis();
    unsigned long currentMillis = millis();

    while(currentMillis - startedMillis < expectedDuration){

      currentMillis = millis();
      if(btnStateChange( pauseButton, &pauseState) != NOCHANGE)
        return false;

      if(currentMillis - previousMillis > duration){
        drawSongName(currentSongName);
        lcd.print(".|");
        previousMillis = millis();
      }

    }
    return true;
}


void playSong(){
  int thisSongNb = currentSongNumber;
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {

    divider = pgm_read_word_near(currentSong+thisNote + 1);
    if (divider > 0) {
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5;
    }

    tone(buzzerPin, pgm_read_word_near(currentSong+thisNote), noteDuration * 0.9);

    drawMusicNotes();
    delayBetweenNotes(noteDuration);

    noTone(buzzerPin);

    if(thisSongNb != currentSongNumber)return;
    if(isMenu)return;
  }
  currentSongNumber++;
}


void menu(){
  lcd.clear();
  lcd.print("WYBIERZ UTWOR: |");
  bool menuLoop = true;

  while( menuLoop ){
    lcd.setCursor(0,1);
    lcd.print(currentSongNumber);
    lcd.print(".");
    menuLoop = delayMenu(50);
    changeSong();
  };
  isMenu = false;
}


void setup() {
  setupScreen();
  setupButtons();
  menu();
}


void loop() {
  delay(100);
  playSong();
  if(isMenu)
    menu();
  changeSong();
}

