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

//Ustawienie zmiennych globalnych potrzebne przy odtwarzaniu muzyki
//currentSong będzie przechowywać tablicę wybranej piosenki
//currentSongName przechowuje nazwę, która będzie się wyświetlać na ekranie
//currentSongNumber informuje o wybranej piosence
//songCount informuje ile jest piosenek, przy każdym dodawaniu nowej trzeba zmieniać
int* currentSong = 0;
String currentSongName = "DOOM ";
uint8_t currentSongNumber = 1;
int songCount = 6;

//Ustawienie wejść do microntrollera
//PINS
const int buzzerPin = A0;
const int ptMeterPin = A2;
const int backButton = 5;
const int pauseButton = 4;
const int nextButton = 3;
const int menuButton = 2;

//Do łatwiejszego zarządzania wciśnięciami przycisków, tworzymy zmienne 
//reprezentujące poszczególne stany
//BUTTON STATES
bool backState = 1;
bool nextState = 1;
bool pauseState = 1;
bool menuState = 1;

//Zmienne do zarządzania lcd
//LCD SCREEN
LCD_I2C lcd(0x27, 16, 2);
int row = 0;
int column = 2;
int iteration = 0;


//Millis(), do wykorzystania we własnym delay()
unsigned long previousMillis = 0;

//Własny znak przedstawiający ikonę muzyki, który wyświetla się na lcd
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


//Zmienne na ustawienie utworów
//wielkość tablicy utworu
int notes = sizeof(doom) / sizeof(doom[0]) / 2;

//Ustawienia szybkości utworu
int tempo = 250;
int min_tempo = 150;
int max_tempo = 350;

int wholenote = (60000 * 4) / tempo;

//Czas trwania noty
int divider = 0, noteDuration = 0;
const int duration = 200;

//Globalna zmienna sprawdzająca czy wybrany jest ekran menu
bool isMenu = false; 

//Konfiguracja początkowa wyświetlacza lcd
void setupScreen(){
    lcd.begin();
    lcd.backlight();
    lcd.createChar(0, noteChar);
    lcd.write(0);
    Serial.begin(9600);
}

//Konfiguracja początkowa przycisków
void setupButtons(){
    pinMode(backButton, INPUT_PULLUP);
    pinMode(pauseButton, INPUT_PULLUP);
    pinMode(nextButton, INPUT_PULLUP);
    pinMode(menuButton, INPUT_PULLUP);
}

//Funkcja reagująca na zmianę stanu przycisku
//Zapobiega problemowi "odbicia" po naciśnięciu przycisków
bool debounceButton(int pin ,bool state){
  bool newState = digitalRead( pin );
  if( state != newState){
    delay(10);
    newState = digitalRead( pin );
  }
  return newState;
}

//Pomocnicza funkcja, która określa czy przycisk przeszedł ze stanu niskiego
//w wysoki, czy na odwrót. Pomaga w określeniu czy użytkownik nacisnął czy zwolnił przycisk
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


//Na bieżąco odczytywana jest zmiana napięcia potencjometru
//Odczytywane wartości zmieniają tempo odtwarzanego utworu
void changeTempo(){
  int value = analogRead(ptMeterPin);
  value = map(value, 0, 1023, min_tempo, max_tempo);
  tempo = value;
  wholenote = (60000 * 4) / tempo;
}

//Każda piosenka ma swoje własne ustawienia, które są zmieniane w tej funkcji
void setSongSettings(int songTable[], int tableSize, int minTempo, int maxTempo){
  currentSong = songTable;
  notes = tableSize / sizeof(songTable[0]) / 2;
  min_tempo = minTempo;
  max_tempo = maxTempo;
}


//Funkcja, która zmienia aktualnie odtwarzany utwór
//W zależności od numeru w currentSongNumber zmienia się tablica currentSong 
//oraz wyświetlane informacje na lcd
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


//Pauza, która zostaje przerwana po naciśnięciu odpowiedniego przycisku
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

//Na bieżąco sprawdzane są stany przycisków, aby wykryć czy przypadkiem
//ktoryś nie został wciśnięty. W zależności od przycisku wywołujemy odpowiednie funkcje
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

//Wyświetla na lcd ikonę muzyki, na bieżąco zmienia położenie o wiersz wyżej/niżej
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


//Wyświetla nazwę utworu oraz przesuwa każdy znak w prawo
//Tworzy wrażenie poruszania się nazwy na lcd
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


//Wyświetla tekst "NOW PLAYING" na lcd
//oraz wywołuje funkcję wyświetlającą nazwę utworu
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


//Zamiast używać wbudowanej funkcji delay(), mamy własną, która
//pozwala na wykrywanie wciśnięć i zmianę tekstu na lcd
//kod jest oparty z przykładu https://docs.arduino.cc/built-in-examples/digital/BlinkWithoutDelay/
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


//Działa jak delayBetweenNotes, ale wywołuje inne funkcje
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


//Odtwarzanie utworu
//Kod oparty na https://github.com/robsoncouto/arduino-songs
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


//Funkcja odpowiedzialna za wyświetlanie menu
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


//Wywołanie początkowych ustawień
void setup() {
  setupScreen();
  setupButtons();
  menu();
}


//Główna pętla, która na bieżąco odtwarza utwory
void loop() {
  delay(100);
  playSong();
  if(isMenu)
    menu();
  changeSong();
}
