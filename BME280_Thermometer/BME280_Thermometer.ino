/******************************************************************************
  温度、湿度、気圧計

  BME280からI2Cでデータ取得し、LCDに表示する。
******************************************************************************/

#include <stdint.h>
#include "SparkFunBME280.h"
#include "Wire.h"
#include <LiquidCrystal.h>
#include <TimerOne.h>

// バックライトピン番号
#define BACKLIGHT 5
#define BL_ON 32
#define BL_OFF 0

// バックライトON/OFFスイッチ
#define PUSH_SWITCH 2

// コントラストピン番号
#define CONTRAST 3
#define CONT_VALUE 110

// hPa(ヘクトパスカル)からinHg(水銀柱インチ)への変換レート
#define INHG_RATE 0.02952998330101

//Global sensor object
BME280 mySensor;
LiquidCrystal lcd(6, 7, 8, 9, 10, 11);

// バックライトステータス
volatile int BLstate = HIGH;
volatile unsigned long time_now, time_prev = 0;
unsigned long time_chat = 20;

const char rotate[] = {
  '|', '/', '-', char(0),
};

// Create Backslash as Special Character #0
uint8_t specialChar0[8] = {
  0b00000,
  0b10000,
  0b01000,
  0b00100,
  0b00010,
  0b00001,
  0b00000,
};
volatile unsigned int rotate_pos = 0;

void setup()
{
  //***Driver settings********************************//
  //commInterface can be I2C_MODE or SPI_MODE
  //specify chipSelectPin using arduino pin names
  //specify I2C address.  Can be 0x77(default) or 0x76

  //For I2C, enable the following and disable the SPI section
  mySensor.settings.commInterface = I2C_MODE;
  mySensor.settings.I2CAddress = 0x76;

  //***Operation settings*****************************//

  //renMode can be:
  //  0, Sleep mode
  //  1 or 2, Forced mode
  //  3, Normal mode
  mySensor.settings.runMode = 3; //Normal mode

  //tStandby can be:
  //  0, 0.5ms
  //  1, 62.5ms
  //  2, 125ms
  //  3, 250ms
  //  4, 500ms
  //  5, 1000ms
  //  6, 10ms
  //  7, 20ms
  mySensor.settings.tStandby = 5;

  //filter can be off or number of FIR coefficients to use:
  //  0, filter off
  //  1, coefficients = 2
  //  2, coefficients = 4
  //  3, coefficients = 8
  //  4, coefficients = 16
  mySensor.settings.filter = 0;

  //tempOverSample can be:
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  mySensor.settings.tempOverSample = 1;

  //pressOverSample can be:
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  mySensor.settings.pressOverSample = 1;

  //humidOverSample can be:
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  mySensor.settings.humidOverSample = 1;

  // Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PUSH_SWITCH, INPUT_PULLUP); // バックライトON/OFFスイッチ入力
  delay(10);
  mySensor.begin();
  delay(10);

  analogWrite(CONTRAST, CONT_VALUE); // LCD Contrast Setting
  analogWrite(BACKLIGHT, BL_ON);     // Backlight LED

  lcd.begin(16, 2);
  lcd.clear();
  lcd.createChar(0, specialChar0);
  // バックライトボタン押下時の割込
  attachInterrupt(digitalPinToInterrupt(PUSH_SWITCH), toggleBackLight, LOW);
  Timer1.initialize(1000000);
  // Timer1.attachInterrupt(blinkLed);
  Timer1.attachInterrupt(rotateChar);
}

void loop()
{
  //Each loop, take a reading.
  //Start with temperature, as that data is needed for accurate compensation.
  //Reading the temperature updates the compensators of the other functions
  //in the background.

  char temp[8], hum[8], pres_hpa[8], pres_inhg[8], line1[17], line2[17];

  // 1行目: 温度と湿度の表示
  dtostrf(mySensor.readTempC(), 5, 2, temp);
  dtostrf(mySensor.readFloatHumidity(), 5, 2, hum);
  sprintf(line1, " %s\337C %s%% ", temp, hum);

  // 2行目: METAR風気圧表示
  float p = mySensor.readFloatPressure();
  // hPa値
  sprintf(pres_hpa, "%04d", (int)(p / 100));
  // inHg値 METAR風表示のため100倍で表示)
  sprintf(pres_inhg, "%04d", (int)(p * INHG_RATE));
  sprintf(line2, "  Q%s  A%s ", pres_hpa, pres_inhg);
  noInterrupts();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
  interrupts();

  delay(10000);
}

// 動作確認用本体Lチカ
void blinkLed() {
  digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN) ^ 1);
}

// 動作確認表示 スラッシュぐるぐる
void rotateChar() {
  if (rotate_pos > 3) {
    rotate_pos = 0;
  }
  lcd.setCursor(15, 1);
  lcd.print(rotate[rotate_pos]);
  rotate_pos++;
}

// バックライトON/OFFボタン動作
void toggleBackLight() {
  time_now = millis();
  // チャタリング対策で、20ms内の割込は無視
  if (time_now - time_prev < time_chat) {
    return;
  }
  if (BLstate == HIGH) {
    analogWrite(BACKLIGHT, BL_OFF);
  } else {
    analogWrite(BACKLIGHT, BL_ON);
  }
  BLstate = !BLstate;
  time_prev = time_now;
}

