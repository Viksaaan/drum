#include <LiquidCrystal.h>
#include <EEPROM.h>
#define PADS 9            // количество дрампадов
#define BAUD_RATE 115200  // скорость работы Монитора порта

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);  // LCD PIN

boolean confirm_edit  = true;   // применить редактирование
boolean mode_is_on    = false;  // режим редактирования 
String status = "";             // статус работы режима редактирования на lcd

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
byte scan_time[PADS]  = { 20,   20,   20,   20,   20,   20,   20,   20,   20};     // Частота опроса датчиков (мс)
byte mask_time[PADS]  = { 20,   20,   20,   20,   20,   20,   20,   20,   20};     // Длительность игнорирования датчика после получения с него сигнала (мс)

boolean playing[PADS]    = {false,false,false,false,false,false,false,false,false};
int high_score[PADS]     = {0,     0,    0,    0,    0,    0,    0,    0,    0};
unsigned long timer[PADS]= {0,     0,    0,    0,    0,    0,    0,    0,    0};

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
    "Note: ",
    "Min limit: ",  
    "Max limit: ", 
    "Scan time: ", 
    "Mask time: "
  };

  short addr_note = 0;        // 0-8   - note
  short addr_min_limit = 10;  // 10-18 - min_limit
  short addr_max_limit = 20;  // 20-28 - max_limit
  short addr_scan_time = 30;  // 30-38 - scan_time
  short addr_mask_time = 40;  // 40-48 - mask_time

////////////////////////////////// SETUP ////////////////////////////////////

void setup() {
    Serial.begin(BAUD_RATE);  

    // greeting on the monitor at startup
    lcd.begin(16, 2);
    lcd.print("welcome!");
    lcd.setCursor(0, 1);
    lcd.print("drum kit ready");
    
    // Buttons
    for (byte b = 6; b <= 10; b++) 
        pinMode(b, INPUT_PULLUP);


    // // for first write in EEPROM
    // for (byte i = 0; i < PADS; i++) {
    //   EEPROM.write(i,    note[i]);
    //   EEPROM.write(i+10, min_limit[i]);
    //   EEPROM.write(i+20, max_limit[i]);
    //   EEPROM.write(i+30, scan_time[i]);
    //   EEPROM.write(i+40, mask_time[i]);
    // }

    /////////////////////// EEPROM ///////////////////////
    for (byte i = 0; i < PADS; i++) {
      note[i] = EEPROM.read(addr_note++);
      min_limit[i] = EEPROM.read(addr_min_limit++);
      max_limit[i] = EEPROM.read(addr_max_limit++);
      scan_time[i] = EEPROM.read(addr_scan_time++);
      mask_time[i] = EEPROM.read(addr_mask_time++);
    }

}


////////////////////////////////// LOOP ////////////////////////////////////

void loop() {

  // считываем данные с пьезоэлемента
  for (byte i = 0; i < PADS; i++) {
    
    short volume = analogRead(i);

    // если считанное значение превышает Минимальное пороговое значение
    if (volume >= min_limit[i] && playing[i] == false) {
      if (millis() - timer[i] >= scan_time[i]) {
        playing[i] = true;
        playNote(i, volume);
      }
    }
    else if (volume >= min_limit[i] && playing[i] == true) {
      playNote(i, volume);
    }
    else if (volume < min_limit[i] && playing[i] == true) {
      noteOn(0x91, note[i], high_score[i], mask_time[i]);
      noteOff(0x91, note[i], 0, mask_time[i]);

      lcd.clear();
      lcd.print(instrument[i]);
      lcd.setCursor(0, 1);
      lcd.print(volume);

      high_score[i] = 0;
      playing[i] = false;
      timer[i] = millis();  // прошло мс с начала работы устройства
    }
  }

  //////////////////////////////// CIRCUIT ////////////////////////////////////

  short keystroke 	    = digitalRead(6);
  short button_UP 	    = digitalRead(7);
  short button_DOWN     = digitalRead(8);
  short button_NEXT 	= digitalRead(9);
  short button_BACK 	= digitalRead(10);


  ////////////////////////////// EDIT BUTTON ////////////////////////////////

  if (keystroke == LOW && confirm_edit == true && mode_is_on == false) {
    lcd.clear();
    lcd.print("EDIT");
    confirm_edit = false;
    mode_is_on   = true;
    status = "(edit)";
    delay(500);
  }

  if (keystroke == LOW && confirm_edit == true && mode_is_on == true) {
    lcd.clear();
    lcd.print("EDIT DONE");
    confirm_edit = false;
    mode_is_on   = false;
    status = "";
    delay(500);
  }

  // edit setting
  if (button_UP == LOW && confirm_edit == true && mode_is_on == true) {
    
    // + note
    if (NEXT_BACK == 0) {    
      note[UP_DOWN] += step[NEXT_BACK]; 

      if (note[UP_DOWN] > 127)  
        note[UP_DOWN] = 127;

      EEPROM.write(UP_DOWN, note[UP_DOWN]);             // ****
    }
    
    // + min_limit
    if (NEXT_BACK == 1) {
      min_limit[UP_DOWN] += step[NEXT_BACK];

      if (min_limit[UP_DOWN] >= max_limit[UP_DOWN])    
        min_limit[UP_DOWN] -= step[NEXT_BACK];

      EEPROM.write(UP_DOWN + 10, min_limit[UP_DOWN]);    // ****
    }   

    // + max_limit
    if (NEXT_BACK == 2) {     
      max_limit[UP_DOWN] += step[NEXT_BACK];  

      if (max_limit[UP_DOWN] > 1023)    
        max_limit[UP_DOWN] = 1020;
        
      EEPROM.write(UP_DOWN + 20, max_limit[UP_DOWN]);    // ****
    }

    // + scan_time
    if (NEXT_BACK == 3) {    
      scan_time[UP_DOWN] += step[NEXT_BACK]; 

      if (scan_time[UP_DOWN] > 1000)
        scan_time[UP_DOWN] = 1000; 

      EEPROM.write(UP_DOWN + 30, scan_time[UP_DOWN]);    // ****
    }

    // + mask_time
    if (NEXT_BACK == 4) {    
      mask_time[UP_DOWN] += step[NEXT_BACK];  

        // if (mask_time[UP_DOWN] >= scan_time[UP_DOWN])
        //   mask_time[UP_DOWN] -= step[NEXT_BACK]; 

        if (mask_time[UP_DOWN] > 1000)
          mask_time[UP_DOWN] = 1000;
        
        EEPROM.write(UP_DOWN + 40, mask_time[UP_DOWN]);  // ****
    }
    
    confirm_edit = false;
    delay(30);
  }


  if (button_DOWN == LOW && confirm_edit == true && mode_is_on == true) {
    
    // - note
    if (NEXT_BACK == 0) {    
      note[UP_DOWN] -= step[NEXT_BACK]; 

      if (note[UP_DOWN] < 0)    
        note[UP_DOWN] = 0;

      EEPROM.write(UP_DOWN, note[UP_DOWN]);             // ****
    }

    // - min_limit
    if (NEXT_BACK == 1) {    
      min_limit[UP_DOWN] -= step[NEXT_BACK];

      if (min_limit[UP_DOWN] < 20)   
        min_limit[UP_DOWN] = 20; 

      EEPROM.write(UP_DOWN + 10, min_limit[UP_DOWN]);    // ****
    }

    // - max_limit
    if (NEXT_BACK == 2) {    
      max_limit[UP_DOWN] -= step[NEXT_BACK]; 

      if (max_limit[UP_DOWN] <= min_limit[UP_DOWN])    
        min_limit[UP_DOWN] += step[NEXT_BACK];

      EEPROM.write(UP_DOWN + 20, max_limit[UP_DOWN]);    // ****
    }

    // - scan_time
    if (NEXT_BACK == 3) {    
      scan_time[UP_DOWN] -= step[NEXT_BACK]; 

      //  if (scan_time[UP_DOWN] <= mask_time[UP_DOWN])  
      //    scan_time[UP_DOWN] =   

      if (scan_time[UP_DOWN] < 0)
        scan_time[UP_DOWN] = 0;

      EEPROM.write(UP_DOWN + 30, scan_time[UP_DOWN]);    // ****
    }

    // - mask_time
    if (NEXT_BACK == 4) {    
      mask_time[UP_DOWN] -= step[NEXT_BACK]; 

      if (mask_time[UP_DOWN] < 0)
        mask_time[UP_DOWN] = 0;

      EEPROM.write(UP_DOWN + 40, mask_time[UP_DOWN]);  // ****
    }

    
    confirm_edit = false;
    delay(30);
  }

  ///////////////////////////// UP ▲ ▼ DOWN  |  BACK ◄ ► NEXT ////////////////////////////////

  // ▲ UP
  if (button_UP == LOW && confirm_edit == true && mode_is_on == false) {   
    UP_DOWN = ++UP_DOWN; 
    
    if (UP_DOWN > 8)    
      UP_DOWN = 0;    
    
    confirm_edit = false;
    delay(30);
  }
    
  // ▼ DOWN
  if (button_DOWN == LOW && confirm_edit == true && mode_is_on == false) {
    UP_DOWN = --UP_DOWN; 

    if (UP_DOWN < 0)    
      UP_DOWN = 8;

    confirm_edit = false;
    delay(30);
  }

  // ► NEXT
  if (button_NEXT == LOW && confirm_edit == true && mode_is_on == false) {
    NEXT_BACK = ++NEXT_BACK; 

    if (NEXT_BACK > 4)     
      NEXT_BACK = 0;

    confirm_edit = false;
    delay(30);
  }

  // ► BACK
  if (button_BACK == LOW && confirm_edit == true && mode_is_on == false) {
    NEXT_BACK = --NEXT_BACK; 

    if (NEXT_BACK < 0)     
      NEXT_BACK = 4;

    confirm_edit = false;
    delay(30);
  }
    

  if (confirm_edit == false && button_UP == HIGH && button_DOWN == HIGH && button_NEXT == HIGH && button_BACK == HIGH && keystroke == HIGH) {
    lcd.clear();
    lcd.print(instrument[UP_DOWN]);

    lcd.setCursor(10, 0);
    lcd.print(status);

    lcd.setCursor(0, 1);
    lcd.print(setting[NEXT_BACK]);
    lcd.setCursor(12, 1);

    if (NEXT_BACK == 0)   lcd.print(note[UP_DOWN]);
    if (NEXT_BACK == 1)   lcd.print(min_limit[UP_DOWN]);
    if (NEXT_BACK == 2)   lcd.print(max_limit[UP_DOWN]);
    if (NEXT_BACK == 3)   lcd.print(scan_time[UP_DOWN]);
    if (NEXT_BACK == 4)   lcd.print(mask_time[UP_DOWN]);

    confirm_edit = true;
  }
}


// функция выравнивания силы звучания
void playNote (byte pad, short volume) {
  float velocity = ((volume) / float(max_limit[pad] - min_limit[pad])) * 100;
  
  if (velocity > 127) 
    velocity = 127;
  if (velocity > high_score[pad]) 
    high_score[pad] = velocity;
}

// функция отправки MIDI-сообщения
void noteOn(int cmd, int pitch, int velocity, int ignore) {
  Serial.write(cmd);
  Serial.write(pitch);
  Serial.write(velocity);
  delay(ignore);
}

// функция прекращения отправки MIDI-сообщения
void noteOff(int cmd, int pitch, int velocity, int ignore) {
  Serial.write(cmd);
  Serial.write(pitch);
  Serial.write(velocity);
  delay(ignore);
}
