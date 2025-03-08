#include <dual2s.h>
#include <PS2X_lib.h>

// DC電機GPIO
#define PIN_MOTO_1A  14
#define PIN_MOTO_1B  32
#define PIN_MOTO_2A  12
#define PIN_MOTO_2B  33
#define PIN_MOTO_3A  25
#define PIN_MOTO_3B  26
#define PIN_MOTO_4A  23
#define PIN_MOTO_4B  22

// 蜂鳴器GPIO
#define PIN_BUZZER   15

// 超音波GPIO
#define PIN_USC_FRONT_ECHO 13
#define PIN_USC_FRONT_TRIG 27

// 供電偵測GPIO
#define PIN_BATTERY    36

// N20馬達GPIO
#define PIN_MOTO_N20A  18
#define PIN_MOTO_N20B  19

GoSUMO gs;

Buzzer bz(PIN_BUZZER, 7);
#define PS2_DAT    4
#define PS2_CMD    17
#define PS2_SEL    16
#define PS2_CLK    21
#define pressures  false
#define rumble     false
PS2X ps2x;
int error = -1;
byte PS2X_type = 0;
byte vibrate = 0;
int tryNum = 1;

long readUltrasonicDistance(int echoPin, int trigPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  return pulseIn(echoPin, HIGH) / 58.0; // 距離（公分）
}


void PS2X_INIT(){
 while (error != 0) {
  delay(1000);
  error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);
  Serial.print("#try config ");  Serial.println(tryNum);  tryNum ++;
 }
 Serial.println(ps2x.Analog(1), HEX);
 PS2X_type = ps2x.readType();
 switch(PS2X_type) {
   case 0: Serial.printf("Unknown Controller type found, type is %d", PS2X_type);  break;
   case 1: Serial.printf("DualShock Controller found, type is %d", PS2X_type);  break;
   case 2: Serial.printf("GuitarHero Controller found, type is %d", PS2X_type);  break;
   case 3: Serial.printf("Wireless Sony DualShock Controller found, type is %d", PS2X_type);  break;
 }
}

void setup()
{
  Serial.begin(115200); // 初始化Serial，用於調試輸出
  PS2X_INIT(); //注意，此將占用DUAL右側超音波
  bz.alarm();
  pinMode(PIN_MOTO_N20A, OUTPUT);
  pinMode(PIN_MOTO_N20B, OUTPUT);
  pinMode(PIN_USC_FRONT_TRIG, OUTPUT);
  pinMode(PIN_USC_FRONT_ECHO, INPUT);
}

bool motorRunning = true; // 記錄N20馬達狀態
bool testMode = false; // 記錄超音波測試模式狀態

void loop()
{
    if (PS2X_type != 2) {
      ps2x.read_gamepad(false, vibrate);
      
    if(ps2x.Analog(PSS_LY) <= 118){ gs.act(GoSUMO::FORWARD, map(ps2x.Analog(PSS_LY),128,0,150,1023)); }    //前進 
    if(ps2x.Analog(PSS_LY) >= 138){ gs.act(GoSUMO::BACKWARD, map(ps2x.Analog(PSS_LY),128,255,150,1023)); } //後退    
    if(ps2x.Analog(PSS_LX) <= 118){ gs.act(GoSUMO::GO_LEFT, map(ps2x.Analog(PSS_LX),128,0,150,1023)); }    //左旋轉 
    if(ps2x.Analog(PSS_LX) >= 138){ gs.act(GoSUMO::GO_RIGHT, map(ps2x.Analog(PSS_LX),128,255,150,1023)); } //右旋轉
    if((ps2x.Analog(PSS_LX) > 125 && ps2x.Analog(PSS_LX) < 130) && (ps2x.Analog(PSS_LY) > 125 && ps2x.Analog(PSS_LY) < 130)) { gs.stop(); } 

      // 檢查PS2圓圈鍵按下狀態
      if (ps2x.ButtonPressed(PSB_CIRCLE)) {
        motorRunning = !motorRunning; // 切換N20馬達狀態
        Serial.print("PSB_CIRCLE pressed, motorRunning = ");
        Serial.println(motorRunning);
        if (motorRunning) {
          controlN20Motor(1); // 啟動N20馬達（正轉）
        } else {
          controlN20Motor(0); // 停止N20馬達
        }
      }

      // 檢查PS2 方形與三角鍵同時按下
      if (ps2x.Button(PSB_SQUARE) && ps2x.Button(PSB_TRIANGLE)) {
        testMode = true; // 啟動超音波測試模式
        Serial.println("Test Mode  on");
      }

      if (testMode){ testMode_Seek(); }
      // 超音波測試模式
    
    }
    delay(15); 
}

// 新增控制N20馬達的函數
void controlN20Motor(int direction) {
  Serial.print("controlN20Motor direction = ");
  Serial.println(direction);
  if (direction == 1) {
    digitalWrite(PIN_MOTO_N20A, HIGH);
    digitalWrite(PIN_MOTO_N20B, LOW);
  } else if (direction == -1) {
    digitalWrite(PIN_MOTO_N20A, LOW);
    digitalWrite(PIN_MOTO_N20B, HIGH);
  } else {
    digitalWrite(PIN_MOTO_N20A, LOW);
    digitalWrite(PIN_MOTO_N20B, LOW);
  }
}

void testMode_Seek() {
  
      if (ps2x.Button(PSB_CROSS)) {
        Serial.println("Test Mode  off");
        testMode = false;
      }
    
    long distance = readUltrasonicDistance(PIN_USC_FRONT_ECHO, PIN_USC_FRONT_TRIG);
    //Serial.print("Distance: ");
    Serial.println(distance);
    delay(500); // 減少延遲以提高響應性
  }

