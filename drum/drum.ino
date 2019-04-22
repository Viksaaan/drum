
#define PADS 9      // количество дрампадов
#define BAUD_RATE 115200  // скорость работы Монитора порта

/* Settings
 *  
   (36) Kick     0|1
   (38) Snare    1|2
   (71) Tom 1    2|4
   (69) Tom 2    3|3
   (67) Tom 3    4|0

   (51) HiHat     5|8
   (46) Cymbal 1  6|5
   (79) Cymbal 2  7|7
   (45) Ride      8|6

*/


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
byte note[PADS]      = { 67,   36,   38,   69,   71,   46,   45,   79,   51};

short minLimit[PADS] = { 100,  100,  100,  100,  100,  100,  300,  300,  300};    // Нижний порог чувствительности                  
short maxLimit[PADS] = { 400,  400,  400,  400,  400,  400,  1000, 1000, 1000};   // Верхний порог чувствительности                      
byte scanTime[PADS]  = { 10,   10,   10,   10,   10,   10,   10,   10,   10};     // Частота опроса датчиков (мс)
byte maskTime[PADS]  = { 20,   20,   20,   20,   20,   20,   20,   20,   20};     // Длительность игнорирования датчика после получения с него сигнала (мс)

//bool playing[PADS]       = {false,false,false,false,false,false,false,false,false};
//int highScore[PADS]      = {0,     0,    0,    0,    0,    0,    0,    0,    0};
//unsigned long timer[PADS]= {0,     0,    0,    0,    0,    0,    0,    0,    0};

unsigned long timer[PADS];
bool playing[PADS];      
int highScore[PADS];

/*
      

*/

void setup() {
  Serial.begin(BAUD_RATE);  

  // сброс значений
  for (byte x = 0; x < PADS; x++) {
    playing[x] = false;
    highScore[x] = 0;
    timer[x] = 0;
  }
}

void loop() {   
  // считываем данные с пьезоэлемента
  for (byte i = 0; i < PADS; i++) {
    
    short volume = analogRead(i);

    // если считанное значение превышает Минимальное пороговое значение
    if (volume >= minLimit[i] && playing[i] == false) {
      if (millis() - timer[i] >= scanTime[i]) {
        playing[i] = true;
        playNote(i, volume);
      }
    }
    else if (volume >= minLimit[i] && playing[i] == true) {
      playNote(i, volume);
    }
    else if (volume < minLimit[i] && playing[i] == true) {
      noteOn(0x91, note[i], highScore[i], maskTime[i]);
      noteOff(0x91, note[i], 0, maskTime[i]);
      highScore[i] = 0;
      playing[i] = false;
      timer[i] = millis();  // прошло мс с начала работы устройства
    }
  }
}

// функция выравнивания силы звучания
void playNote (byte pad, short volume) {
  float velocity = ((volume) / float(maxLimit[pad] - minLimit[pad])) * 200;
  
  if (velocity > 127) 
    velocity = 127;
  if (velocity > highScore[pad]) 
    highScore[pad] = velocity;
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