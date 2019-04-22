

#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);  // LCD PIN

#define PADS 9
bool playing[PADS] = {false,false,false,false,false,false,false,false,false};

boolean confirm_edit  = true;   // применить редактирование
boolean mode_is_on    = true;   // режим редактирования 

int UP_DOWN = 0;
int NEXT_BACK = 0;

// настройка дрампадов
/*
                          -T3
                          |A0   -Kick
                          |     |A1   -Snare
                          |     |     |A2   -T2
                          |     |     |     |A3   -T1
                          |     |     |     |     |A4   -C1
                          |     |     |     |     |     |A5   -Ride
                          |     |     |     |     |     |     |A6   -C2
                          |     |     |     |     |     |     |     |A7   -HH
                          |     |     |     |     |     |     |     |     |A8 
*/        
byte note[PADS]       = { 67,   36,   38,   69,   71,   46,   45,   79,   51};

short min_limit[PADS] = { 100,  100,  100,  100,  100,  100,  300,  300,  300};    // Нижний предел чувствительности                  
short max_limit[PADS] = { 400,  400,  400,  400,  400,  400,  1000, 1000, 1000};   // Верхний предел чувствительности                      
byte scan_time[PADS]  = { 10,   10,   10,   10,   10,   10,   10,   10,   10};     // Частота опроса датчиков (мс)
byte mask_time[PADS]  = { 20,   20,   20,   20,   20,   20,   20,   20,   20};     // Длительность игнорирования датчика после получения с него сигнала (мс)


////////////////////////////////// EDIT MODE ////////////////////////////////////

/*  change step
                  -Note
                  |    -Min limit
                  |    |    -Max limit
                  |    |    |    -Scan time
                  |    |    |    |    -Maskt ime
                  |    |    |    |    | 
*/
  byte step[5] = {1,  10,  10,   5,   5}; 

  char* instrument[] = {
    "Tom 3", 
    "Kick",  
    "Snare", 
    "Tom 2",
    "Tom 1", 
    "Cymbal 1",
    "Ride",
    "Cymbal 2",
    "HiHat"
  };

  char* setting[] = {
    "Note",
    "Min limit",  
    "Max limit", 
    "Scan time", 
    "Mask time"
  };




void setup() {
  MIDI.begin(10);

  // greeting on the monitor at startup
  lcd.begin(16, 2);
  lcd.print("welcome!");
  lcd.setCursor(0, 1);
  lcd.print("drum kit ready");
  
  // Buttons
  for (byte b = 6; b <= 10; b++) 
    pinMode(b, INPUT_PULLUP);
}

void loop() {

  /////////////////////////////////// CIRCUIT ///////////////////////////////////////

  short keystroke 	  = digitalRead(6);
  short button_UP 	  = digitalRead(7);
  short button_DOWN 	= digitalRead(8);
  short button_NEXT 	= digitalRead(9);
  short button_BACK 	= digitalRead(10);

// Menu Buttons
  if (UP_DOWN < 0)       UP_DOWN = 9;
  if (UP_DOWN > 9)       UP_DOWN = 0;
  if (NEXT_BACK < 0)     NEXT_BACK = 5;
  if (NEXT_BACK > 5)     NEXT_BACK = 0;
  

  ////////////////////////////// EDIT BUTTON ////////////////////////////////

  if (keystroke == LOW && confirm_edit == true && mode_is_on == true) {
    lcd.clear();
    lcd.print("EDIT");
    confirm_edit = false;
    mode_is_on   = false;
    delay(500);
  }

  if (keystroke == LOW && confirm_edit == true && mode_is_on == false) {
    lcd.clear();
    lcd.print("EDIT DONE");
    confirm_edit = false;
    mode_is_on   = true;
    delay(500);
  }

  // edit setting
  if (button_UP == LOW && confirm_edit == true && mode_is_on == false) {
    note[UP_DOWN] += step[NEXT_BACK];     
    confirm_edit = false;
    delay(30);
  }

  if (button_DOWN == LOW && confirm_edit == true && mode_is_on == false) {
    note[UP_DOWN] -= step[NEXT_BACK];
    confirm_edit = false;
    delay(30);
  }

  ///////////////////////////// UP ▲ ▼ DOWN  |  BACK ◄ ► NEXT ////////////////////////////////

  // ▲ UP
  if (button_UP == LOW && confirm_edit == true && mode_is_on == true) {
    UP_DOWN = ++UP_DOWN;     
    confirm_edit = false;
    delay(30);
  }
    
  // ▼ DOWN
  if (button_DOWN == LOW && confirm_edit == true && mode_is_on == true) {
    UP_DOWN = --UP_DOWN;     
    confirm_edit = false;
    delay(30);
  }

  // ► NEXT
  if (button_NEXT == LOW && confirm_edit == true && mode_is_on == true) {
    NEXT_BACK = ++NEXT_BACK;    
    confirm_edit = false;
    delay(30);
  }

  // ► BACK
  if (button_BACK == LOW && confirm_edit == true && mode_is_on == true) {
    NEXT_BACK = --NEXT_BACK;    
    confirm_edit = false;
    delay(30);
  }
    
  

  if (confirm_edit == false && button_UP == HIGH && button_DOWN == HIGH && button_NEXT == HIGH && button_BACK == HIGH && keystroke == HIGH) {
    lcd.clear();
    lcd.print(instrument[UP_DOWN]);
    lcd.setCursor(0, 1);
    lcd.print(setting[NEXT_BACK]);
    lcd.setCursor(12, 1);

    if (UP_DOWN == 0)   lcd.print(note[UP_DOWN]);
    if (UP_DOWN == 1)   lcd.print(min_limit[UP_DOWN]);
    if (UP_DOWN == 2)   lcd.print(max_limit[UP_DOWN]);
    if (UP_DOWN == 3)   lcd.print(scan_time[UP_DOWN]);
    if (UP_DOWN == 4)   lcd.print(mask_time[UP_DOWN]);

    confirm_edit = true;
  }

}