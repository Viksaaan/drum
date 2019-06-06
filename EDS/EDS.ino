#include <LiquidCrystal_PCF8574.h>  // Управление жидкокристаллическими экранами
#include <EEPROM.h>   // Чтение и запись в постоянное хранилище
#include <Wire.h>     // Обмен данными с устройствами I2C/TWI
//#include <LiquidCrystal_I2C.h>

#define PADS 9            // количество дрампадов
#define BAUD_RATE 115200  // скорость работы Монитора порта


//LiquidCrystal lcd(12, 11, 5, 4, 3, 2);  // LCD PIN
//LiquidCrystal_I2C lcd(0x27,16,2); // Указываем I2C адрес (наиболее распространенное значение), а также параметры экрана (в случае LCD 1602 - 2 строки по 16 символов в каждой 
LiquidCrystal_PCF8574 lcd(0x27); // Вариант для библиотеки PCF8574 


bool confirm_edit = true;    // применить редактирование
bool mode_edit_on = false;   // режим редактирования

String status = "";          // статус работы режима редактирования на lcd

short INC_DEC = 0;    // выбор инструмента & установка значений
short NEXT_BACK = 0;  // выбор настройки

// настройка дрампадов
/*
                          -T4
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
byte note[PADS]       = { 65,   36,   38,   69,   71,   46,   45,   79,   51};     // Частоты нот
short min_limit[PADS] = { 100,  100,  100,  100,  100,  100,  300,  300,  300};    // Нижний порог срабатывания                
short scan_time[PADS] = { 20,   20,   20,   20,   20,   20,   20,   20,   20};     // Частота опроса (и длительность игнорирования) датчиков (мс) 
short sensitivity[PADS]={ 80,   80,   80,   80,   80,   80,   80,   80,   80};     // Чувствительность

boolean playing[PADS]    = {false,false,false,false,false,false,false,false,false};
short high_score[PADS]   = {0,    0,    0,    0,    0,    0,    0,    0,    0};
unsigned long timer[PADS]= {0,    0,    0,    0,    0,    0,    0,    0,    0};

//short max_limit[PADS] = { 400,  400,  400,  400,  400,  400,  1000, 1000, 1000}; // Верхний порог срабатывания                      
// byte mask_time[PADS]  = { 20,   20,   20,   20,   20,   20,   20,   20,   20};  // Длительность игнорирования датчика после получения с него сигнала (мс)

/*  шаг изменения настройки
                  -Note (0..127)
                  |   -Sensitivity (50..100)   
                  |   |   -Min limit (100..1000) 
                  |   |   |    -Scan time (10..1000)
                  |   |   |    |       
*/
  byte step[4] = {1,  1,  10,  5}; 

  // набор инструментов для настройки
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

  // набор настроек для инструментов
  char* setting[] = {
    "Note: ",
    "Sensitivity:",
    "Min limit: ",  
    "Scan time: "
  };

  // начальные номера ячеек для хранения настроек в EEPROM
  byte addr_note = 0;           // 0-8   - note         (1 byte)
  short addr_sensitivity = 70;  // 10-27 - sensitivity  (2 byte)
  short addr_min_limit   = 10;  // 30-47 - min_limit    (2 byte)
  short addr_scan_time   = 50;  // 50-67 - scan_time    (2 byte)


void setup() {
    Serial.begin(BAUD_RATE);  

    // текст приветствия на дисплее при запуске
    lcd.setBacklight(10);
    lcd.begin(16, 2);
    lcd.print("welcome!");
    lcd.setCursor(0, 1);
    lcd.print("drum kit ready");
    

    // Установка режима работы кнопок
    for (byte b = 6; b <= 10; b++) 
      pinMode(b, INPUT_PULLUP);


    // Запись данных в EEPROM (производится единожды)
    // for (byte i = 0; i < PADS; i++) 
    // {
    //   EEPROM.put(addr_note, note[i]);
    //   EEPROM.put(addr_sensitivity, sensitivity[i]);
    //   EEPROM.put(addr_min_limit, min_limit[i]);
    //   EEPROM.put(addr_scan_time, scan_time[i]);

    //   addr_note += sizeof(addr_note);
    //   addr_sensitivity += sizeof(sensitivity);
    //   addr_min_limit += sizeof(addr_min_limit);
    //   addr_scan_time += sizeof(addr_scan_time);
    // }

    // addr_note = 0;          // 0-8   - note         (1 byte)
    // addr_sensitivity = 70;  // 10-27 - sensitivity  (2 byte)
    // addr_min_limit   = 10;  // 30-47 - min_limit    (2 byte)
    // addr_scan_time   = 50;  // 50-67 - scan_time    (2 byte)
    

    // Чтение данных из EEPROM 
    for (byte i = 0; i < PADS; i++)    
    {
      EEPROM.get(addr_note, note[i]);
      EEPROM.get(addr_sensitivity, sensitivity[i]);
      EEPROM.get(addr_min_limit, min_limit[i]);
      EEPROM.get(addr_scan_time, scan_time[i]);      

      addr_note += sizeof(addr_note);
      addr_sensitivity += sizeof(sensitivity);
      addr_min_limit += sizeof(addr_min_limit);
      addr_scan_time += sizeof(addr_scan_time);
    }

    // Очистка памяти EEPROM   
    //  for (byte b = 0; b < 100; b++)
    //    EEPROM.update(b, 0);
}


void loop() {

  // считываем данные с пьезоэлементов
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
      noteOn(0x91, note[i], high_score[i]/*, scan_time[i]*/);   // начало активации ноты
      noteOff(0x91, note[i], 0/*, scan_time[i]*/);              // конец активации ноты

      lcd.clear();
      lcd.print(instrument[i]);
      lcd.setCursor(0, 1);
      lcd.print(volume);

      high_score[i] = 0;
      playing[i] = false;
      timer[i] = millis();  // прошло мс с начала работы устройства
    }
  }

  short keystroke   = digitalRead(6);
  short button_INC  = digitalRead(7);
  short button_DEC  = digitalRead(8);
  short button_NEXT = digitalRead(9);
  short button_BACK = digitalRead(10);


  // если редактирование начато
  if (keystroke == LOW && confirm_edit == true && mode_edit_on == false) {
    lcd.clear();
    lcd.print("EDIT");
    confirm_edit = false;
    mode_edit_on = true;
    status = "(edit)";
    delay(500);
  }

  // если редактирование завершено
  if (keystroke == LOW && confirm_edit == true && mode_edit_on == true) {
    lcd.clear();
    lcd.print("EDIT DONE");
    confirm_edit = false;
    mode_edit_on = false;
    status = "";
    delay(500);
  }

  // изменение настроек
  if (button_INC == LOW && confirm_edit == true && mode_edit_on == true) {
    
    // [+] note
    if (NEXT_BACK == 0) {    
      note[INC_DEC] += step[NEXT_BACK]; 

      if (note[INC_DEC] > 127)  
        note[INC_DEC] = 127;

      EEPROM.update(INC_DEC, note[INC_DEC]);             
    }
    
    // [+] sensitivity
    if (NEXT_BACK == 1) {    
      sensitivity[INC_DEC] += step[NEXT_BACK]; 

      if (sensitivity[INC_DEC] > 100)  //*******
        sensitivity[INC_DEC] = 100;    //*******

      EEPROM.update(INC_DEC * sizeof(short) + 10, sensitivity[INC_DEC]);             
    }

    // [+] min_limit
    if (NEXT_BACK == 2) {
      min_limit[INC_DEC] += step[NEXT_BACK];

      if (min_limit[INC_DEC] > 1000)    
        min_limit[INC_DEC] = 1000;

      EEPROM.update(INC_DEC * sizeof(short) + 30, min_limit[INC_DEC]);    
    }   

    // [+] scan_time
    if (NEXT_BACK == 3) {    
      scan_time[INC_DEC] += step[NEXT_BACK]; 

      if (scan_time[INC_DEC] > 1000)
        scan_time[INC_DEC] = 1000; 

      EEPROM.update(INC_DEC * sizeof(short) + 50, scan_time[INC_DEC]);    
    }

    confirm_edit = false;
    delay(30);
  }


  if (button_DEC == LOW && confirm_edit == true && mode_edit_on == true) {   

    // [-] note
    if (NEXT_BACK == 0) {    
      note[INC_DEC] -= step[NEXT_BACK]; 

      if (note[INC_DEC] < 0)    
        note[INC_DEC] = 0;

      EEPROM.update(INC_DEC, note[INC_DEC]);             
    }

    // [-] sensitivity
    if (NEXT_BACK == 1) {    
      sensitivity[INC_DEC] -= step[NEXT_BACK]; 

      if (sensitivity[INC_DEC] < 50)    
        sensitivity[INC_DEC] = 50;

      EEPROM.update(INC_DEC * sizeof(short) + 10, sensitivity[INC_DEC]);            
    }

    // [-] min_limit
    if (NEXT_BACK == 2) {    
      min_limit[INC_DEC] -= step[NEXT_BACK];

      if (min_limit[INC_DEC] < 20)   
        min_limit[INC_DEC] = 20; 

      EEPROM.update(INC_DEC * sizeof(short) + 30, min_limit[INC_DEC]); 
    }

    // [-] scan_time
    if (NEXT_BACK == 3) {    
      scan_time[INC_DEC] -= step[NEXT_BACK];  

      if (scan_time[INC_DEC] < 10)
        scan_time[INC_DEC] = 10;

      EEPROM.update(INC_DEC * sizeof(short) + 50, scan_time[INC_DEC]); 
    }

    confirm_edit = false;
    delay(30);
  }

  /////////////////////////////  DEC ◄ ► INC |  BACK ◄ ► NEXT ////////////////////////////////
  
  // ► INC
  if (button_INC == LOW && confirm_edit == true && mode_edit_on == false) {   
    INC_DEC = ++INC_DEC; 
    
    if (INC_DEC > 8)    
      INC_DEC = 0;    
    
    confirm_edit = false;
    delay(30);
  }
    
  // ◄ DEC
  if (button_DEC == LOW && confirm_edit == true && mode_edit_on == false) {
    INC_DEC = --INC_DEC; 

    if (INC_DEC < 0)    
      INC_DEC = 8;

    confirm_edit = false;
    delay(30);
  }

  // ► NEXT
  if (button_NEXT == LOW && confirm_edit == true && mode_edit_on == false) {
    NEXT_BACK = ++NEXT_BACK; 

    if (NEXT_BACK > 3)     
      NEXT_BACK = 0;

    confirm_edit = false;
    delay(30);
  }

  // ► BACK
  if (button_BACK == LOW && confirm_edit == true && mode_edit_on == false) {
    NEXT_BACK = --NEXT_BACK; 

    if (NEXT_BACK < 0)     
      NEXT_BACK = 3;

    confirm_edit = false;
    delay(30);
  }
    

  if (confirm_edit == false && button_INC == HIGH && button_DEC == HIGH && button_NEXT == HIGH && button_BACK == HIGH && keystroke == HIGH) {    
    lcd.clear();
    lcd.print(instrument[INC_DEC]);

    lcd.setCursor(10, 0);
    lcd.print(status);

    lcd.setCursor(0, 1);
    lcd.print(setting[NEXT_BACK]);
    lcd.setCursor(13, 1);

    if (NEXT_BACK == 0)   lcd.print(note[INC_DEC]);
    if (NEXT_BACK == 1)   lcd.print(sensitivity[INC_DEC]);
    if (NEXT_BACK == 2)   lcd.print(min_limit[INC_DEC]);
    if (NEXT_BACK == 3)   lcd.print(scan_time[INC_DEC]);

    confirm_edit = true;
  }
}


// функция выравнивания силы звучания
// (влияет на громкость от силы удара)
void playNote (byte pad, short volume) {
  float velocity = (volume / (float)sensitivity[pad]) * 10;
  
  if (velocity > 127) 
    velocity = 127;
  if (velocity > high_score[pad]) 
    high_score[pad] = velocity;
}

// функция отправки MIDI-сообщения 
void noteOn(int cmd, int pitch, int velocity) /*, int ignore*/ {
  Serial.write(cmd);
  Serial.write(pitch);
  Serial.write(velocity);
  // delay(ignore);    // ***
}

// функция прекращения отправки MIDI-сообщения
void noteOff(int cmd, int pitch, int velocity) /*, int ignore*/ {
  Serial.write(cmd);
  Serial.write(pitch);
  Serial.write(velocity);
  // delay(ignore);    // ***
}