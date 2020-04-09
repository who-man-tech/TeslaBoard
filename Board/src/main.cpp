#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SoftwareSerial.h>

#include "motor.h"
#include "main.h"
#include "light.h"

RF24 radio(RADIO_CS_PIN, RADIO_CSN_PIN);
SoftwareSerial blSerial(BL_RX, BL_TX);
Motor motor(MOTOR_PIN, TEMP_PIN);
Light light(LEDS_PIN, NUM_LEDS);

SettingMode sett_mode;

int got_data[6];
byte send_data[3];
int bl_data[6];
bool is_connect;

bool is_setting;
unsigned long send_timer;
unsigned long connect_timer;



// TODO: Сделать настойку режимов мотора
// TODO: Настройка максмальной температуры мотора
// TODO: Добавить сохранение данных в EEPROM
// TODO: Изменить управление скейтом так, чтобы мы могли использовать один общий массив



void parse() {
  // Если данные приходят от блютуза или радио-передатчика то is_connect = true
  // Если данные не приходять в течении 5 секунд и is_connect = true, то is_connect = false;
  // Если is_connect = false, но данные пришли, то сбрасываем таймер и говорим, что is_connect = true;
  // Сбрасываем таймер при каждом получении данных
  // if(данные от блютуза)
  // else if (данные от радио-передатчика)
  // else: Ждем 5 секунд и говорим что is_connect = false

  byte pipeNo;
  static uint8_t index;
  String string_convert = "";
  static boolean getStarted;


  if (blSerial.available() > 0) {
    Serial.println("bluetooth_connected");
    char incomingByte = blSerial.read(); // обязательно ЧИТАЕМ входящий символ
    if (getStarted) { // если приняли начальный символ (парсинг разрешён)
      if (incomingByte != ' ' && incomingByte != ';') { // если это не пробел И не конец
        string_convert += incomingByte; // складываем в строку
      }
      else { // если это пробел или ; конец пакета
        got_data[index] = string_convert.toInt(); // преобразуем строку в int и кладём в массив
        string_convert = ""; // очищаем строку
        index++; // переходим к парсингу следующего элемента массива
      }
    }
    if (incomingByte == '$') { // если это $
      getStarted = true; // поднимаем флаг, что можно парсить
      index = 0; // сбрасываем индекс
      string_convert = ""; // очищаем строку
    }
    if (incomingByte == ';') { // если таки приняли ; - конец парсинга
      getStarted = false; // сброс
      is_connect = true; // флаг на принятие
      connect_timer = millis(); 
    }
  }

  else if (radio.available(&pipeNo)) { // слушаем эфир со всех труб
    Serial.println("radio_connected");
    radio.read(&got_data, sizeof(got_data)); // читаем входящий сигнал
    if (millis() - send_timer > 2000) { // Отправляем данные обратно каждые 2 секунды
      radio.writeAckPayload(1, &send_data, sizeof(send_data));
      send_timer = millis();
    }
    connect_timer = millis();
    is_connect = true;
  }
  
  else{
    if (is_connect && millis() - connect_timer > 5000){
      is_connect = false;
      Serial.println("no_connection");
    }
  }
}



void setup() {
  Serial.begin(9600);

  pinMode(BUTT_PIN, INPUT_PULLUP);

  light.begin();

  blSerial.begin(9600);
  
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0,15);     //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах
  radio.openReadingPipe(1, 0xF0F0F0F0E1LL);      //хотим слушать трубу 0
  radio.setChannel(0x60);  //выбираем канал (в котором нет шумов!)
  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_1MBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!
  radio.powerUp(); //начать работу
  radio.startListening();  //начинаем слушать эфир, мы приёмный модуль
  radio.writeAckPayload(1, &send_data, sizeof(send_data));

  motor.begin();

  if (digitalRead(A0) == 0) {
    is_setting = true;
  }
}



void loop() {
  parse();
  light.update();
  motor.update();

  if (!is_setting) {
    if (is_connect) {

      send_data[0] = 20; // Отправляем данные о заряде
      send_data[1] = motor.getTemp(); // Отправляем данные о температуре

      Serial.print(got_data[0]); Serial.print("   ");
      Serial.print(got_data[1]); Serial.print("   ");
      Serial.print(got_data[2]); Serial.print("   ");
      Serial.print(got_data[3]); Serial.print("   ");
      Serial.println(got_data[4]);

      sett_mode = static_cast<SettingMode>(got_data[0]);
      switch (sett_mode) {
        case smMain: {
          motor.setMode(got_data[1]);
          int value = map(got_data[2], 20, 480, 0, 255);
          value = constrain(value, 0, 255);
          motor.setPower(value);
          break;
        }
        
        case smLight: {
          light.setOn(got_data[1]);
          light.setEffectByIndex(got_data[2]);
          light.setBrightness(got_data[3]);
          switch (light.getEffectIndex()) {
            case 0: {
              light.setEffectColor(got_data[4]);
              break;
            }

            case 1: {
              light.setLightsBlink(4);
              break;
            }

            case 2: {
              light.setEffectSpeed(got_data[4]);
              break;
            }
          }
          break;
        }
      }
    }

    else {
      // Выключаем мотор
      motor.setMode(Motor::mOff);
      motor.setPower(0);
    }


    //   byte pipeNo;
    //   if (radio.available(&pipeNo)) { // слушаем эфир со всех труб
    //     radio.read(&got_data, sizeof(got_data)); // читаем входящий сигнал
    //     if (millis() - send_timer > 2000) { // Отправляем данные обратно каждые 2 секунды
    //       radio.writeAckPayload(1, &send_data, sizeof(send_data));
    //       send_timer = millis();
    //     }

    //     radio_timer = millis(); // Сбрасываем таймер подключения

    //     motor.setPower(got_data[0]);  // Данные о положение потенциометра
    //     motor.setMode(got_data[1]); // Данные о выбранном режиме
    //     light.setOn(got_data[2]);

    //     send_data[0] = 20; // Отправляем данные о заряде
    //     send_data[1] = motor.getTemp(); // Отправляем данные о температуре
    //   }

    //   if (millis() - radio_timer > 5000) { // Если данные от пульта не поступали больше 5 секунд
    //     // Выключаем мотор
    //     motor.setMode(Motor::mOff);
    //     motor.setPower(0);
    //     radio_timer = millis();
    //   }
    // } 
  }

  // Режим настроек
  else { // В режиме настроек включаем спорт режим
    motor.setMode(Motor::mSport); // Максимальная чувствительность
    if (digitalRead(BUTT_PIN) == 0)
      motor.setPower(255);
    else
      motor.setPower(0);
  }
}