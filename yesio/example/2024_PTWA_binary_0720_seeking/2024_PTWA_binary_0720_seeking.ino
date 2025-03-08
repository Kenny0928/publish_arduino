// 增加模式定義
#include <dual2s.h>
#include <PS2X_lib.h>
#include <Servo.h>

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

// 短距紅外線GPIO
#define PIN_IR_LEFT    34
#define PIN_IR_MIDDLE  35
#define PIN_IR_RIGHT   39

// 供電偵測GPIO
#define PIN_BATTERY    36

// 預設使用的PWM通道
#define PWM_CH_BUZZER 7
#define PWM_CH_M1A    8
#define PWM_CH_M1B    9
#define PWM_CH_M2A    10
#define PWM_CH_M2B    11
#define PWM_CH_M3A    12
#define PWM_CH_M3B    13
#define PWM_CH_M4A    14
#define PWM_CH_M4B    15

GoSUMO gs;
Buzzer bz(PIN_BUZZER, PWM_CH_BUZZER);
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

/*-- 宣告區(全域變數) --*/
int SeekCnt = 0;
unsigned long prevMillis = 0;  // 暫存經過時間（毫秒）
const long interval = 5000;     // 指定間隔時間5秒(5000毫秒)。

// 模式定義
enum Mode { MANUAL, AUTO_SEARCH, TEST };
Mode currentMode = MANUAL;

// Servo Motor 變數區 & 機械臂微調區 -------
Servo servoBase;
Servo servoPaw;
int homeBase = 90; // 機械臂原點角度。
int homePaw = 90;  // 機械爪原點角度。

int posBase = homeBase;
int posBaseMAX = 170; // 機械臂最大抬升角度。
int posBaseMIN = 90;  // 機械臂最小降低角度。
int posPaw = homePaw;
int posPawMAX = 90;   // 機械爪最大開啟角度。
int posPawMIN = 30;   // 機械爪最小閉合角度。

long readUltrasonicDistance(int echoPin, int trigPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  return pulseIn(echoPin, HIGH) / 58.0; // 距離（公分）
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_USC_FRONT_TRIG, OUTPUT);
  pinMode(PIN_USC_FRONT_ECHO, INPUT);

  // 初始化其他設備
  bz.alarm();
  PS2X_INIT(); // 注意，此將占用DUAL右側超音波

  // 檢查PS2控制器是否初始化成功
  if (error == 0) {
    Serial.println("PS2 Controller successfully initialized");
  } else {
    Serial.println("PS2 Controller initialization failed");
    while(true); // 停止程式以防止進一步動作
  }

  // ServoMotor初始化
  servoBase.attach(19); // servo-2、棕線
  servoPaw.attach(18);  // servo-1、白線
  ServoARM("Base", "MAX");   // 起始抬到最高
  ServoARM("Paw", "MAX");

  currentMode = MANUAL; // 設定初始模式
}

void loop() {
  if (ps2x.read_gamepad(false, vibrate)) { // 讀取PS2命令
    // 檢查模式切換
    if (ps2x.Button(PSB_L1)) {
      if (ps2x.Button(PSB_PAD_DOWN)) {
        currentMode = TEST;
      }
    }
    if (ps2x.Button(PSB_CIRCLE)) {
      currentMode = AUTO_SEARCH;
    }
    if (ps2x.Button(PSB_START) || ps2x.Button(PSB_SELECT) || ps2x.Button(PSB_L1) || ps2x.Button(PSB_R1)) {
      currentMode = MANUAL;
    }

    // 根據模式執行相應動作
    switch (currentMode) {
      case MANUAL:
        manualMode();
        break;
      case AUTO_SEARCH:
        autoSearchMode();
        break;
      case TEST:
        testMode();
        break;
    }
  }

  delay(15);
}

void manualMode() {
  Serial.println("Manual Mode");
  if (ps2x.Analog(PSS_RY) <= 118) { gs.act(GoSUMO::FORWARD, map(ps2x.Analog(PSS_RY), 128, 0, 150, 1023)); }    // 前進 
  if (ps2x.Analog(PSS_RY) >= 138) { gs.act(GoSUMO::BACKWARD, map(ps2x.Analog(PSS_RY), 128, 255, 150, 1023)); } // 後退    
  if (ps2x.Analog(PSS_RX) <= 118) { gs.act(GoSUMO::GO_LEFT, map(ps2x.Analog(PSS_RX), 128, 0, 150, 1023)); }    // 左旋轉 
  if (ps2x.Analog(PSS_RX) >= 138) { gs.act(GoSUMO::GO_RIGHT, map(ps2x.Analog(PSS_RX), 128, 255, 150, 1023)); } // 右旋轉
  if ((ps2x.Analog(PSS_RX) > 125 && ps2x.Analog(PSS_RX) < 130) && (ps2x.Analog(PSS_RY) > 125 && ps2x.Analog(PSS_RY) < 130)) { gs.stop(); }

  if (ps2x.Button(PSB_PAD_UP)) { ServoARM("Base", "UP"); }    // 上鍵：機械臂上升
  if (ps2x.Button(PSB_PAD_DOWN)) { ServoARM("Base", "DOWN"); }  // 下鍵：機械臂下降
  if (ps2x.Button(PSB_PAD_LEFT)) { ServoARM("Paw", "OPEN"); }   // 左鍵：機械爪打開
  if (ps2x.Button(PSB_PAD_RIGHT)) { ServoARM("Paw", "CLOSE"); } // 右鍵：機械爪關閉

  if (ps2x.Button(PSB_L1)) { ServoARM("Base", "MAX"); }         // L1鍵：機械臂移到最大角度
  if (ps2x.Button(PSB_L2)) { ServoARM("Base", "MIN"); }         // L2鍵：機械臂移到最小角度
  if (ps2x.Button(PSB_R1)) { ServoARM("Paw", "MAX"); }          // R1鍵：機械爪打開到最大角度

  if (ps2x.Button(PSB_L3) || ps2x.Button(PSB_R3)) { ServoARM("Home", ""); } // Servo復位90度
}

void autoSearchMode() {
  Serial.println("Auto Search Mode");
  long distance = readUltrasonicDistance(PIN_USC_FRONT_ECHO, PIN_USC_FRONT_TRIG);
  while (distance > 40 && !ps2x.Button(PSB_START)) {
    gs.act(GoSUMO::FORWARD, 500);
    distance = readUltrasonicDistance(PIN_USC_FRONT_ECHO, PIN_USC_FRONT_TRIG);
    delay(100);
  }
  gs.stop();

  // 旋轉並掃描環境
  long distances[60];
  for (int i = 0; i < 60; i++) {
    gs.act(GoSUMO::GO_RIGHT, 500);
    delay(150);
    distances[i] = readUltrasonicDistance(PIN_USC_FRONT_ECHO, PIN_USC_FRONT_TRIG);
  }
  gs.stop();

  // 分析掃描結果並前進至目標
  long minDistance = 9999;
  int targetIndex = -1;
  for (int i = 0; i < 60; i++) {
    if (distances[i] < minDistance && distances[i] > 20) { // 假設目標物在20-40公分之間
      minDistance = distances[i];
      targetIndex = i;
    }
  }

  if (targetIndex != -1) {
    // 計算前往目標的角度和距離
    int turnTime = targetIndex * 50; // 每個掃描點間隔50毫秒
    gs.act(GoSUMO::GO_LEFT, turnTime);
    delay(turnTime);

    // 前進到目標
    while (readUltrasonicDistance(PIN_USC_FRONT_ECHO, PIN_USC_FRONT_TRIG) > 20) {
      gs.act(GoSUMO::FORWARD, 500);
      delay(100);
    }
    gs.stop();
  } else {
    Serial.println("No target found");
  }

  delay(1000); // 每次循環後等待一秒
}

void testMode() {
  Serial.println("Test Mode");
  while (true) {
    if (ps2x.read_gamepad(false, 0)) {
      if (ps2x.Button(PSB_START) || ps2x.Button(PSB_SELECT) || ps2x.Button(PSB_L1) || ps2x.Button(PSB_R1)) {
        currentMode = MANUAL;
        break;
      }
    }
    long distance = readUltrasonicDistance(PIN_USC_FRONT_ECHO, PIN_USC_FRONT_TRIG);
    Serial.print("Distance: ");
    Serial.println(distance);
    delay(100); // 減少延遲以提高響應性
  }
}

void ServoARM(String _ACTION, String _DIR) {
  int step = 5; // 將角度變化的步伐設為5度，以加快速度
  if (_ACTION == "Base") {
    if ((_DIR == "UP") && (posBase < posBaseMAX)) { servoBase.write(posBase += step); }
    if ((_DIR == "DOWN") && (posBase > posBaseMIN)) { servoBase.write(posBase -= step); }
    if (_DIR == "MAX") { servoBase.write(posBaseMAX); posBase = posBaseMAX; }
    if (_DIR == "MIN") { servoBase.write(posBaseMIN); posBase = posBaseMIN; }
  }

  if (_ACTION == "Paw") {
    if ((_DIR == "OPEN") && (posPaw < posPawMAX)) { servoPaw.write(posPaw += step); }
    if ((_DIR == "CLOSE") && (posPaw > posPawMIN)) { servoPaw.write(posPaw -= step); }
    if (_DIR == "MAX") { servoPaw.write(posPawMAX); posPaw = posPawMAX; }
  }

  if (_ACTION == "Home") {
    Serial.println("servoHome");
    servoBase.write(homeBase);
    servoPaw.write(homePaw);
    posPaw = homePaw;
    posBase = homeBase;
    delay(1000);
  }
}

void PS2X_INIT() {
  while (error != 0) {
    delay(1000);
    error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);
    Serial.print("#try config ");  Serial.println(tryNum);  tryNum++;
  }
  Serial.println(ps2x.Analog(1), HEX);
  PS2X_type = ps2x.readType();
  switch (PS2X_type) {
    case 0: Serial.printf("Unknown Controller type found, type is %d", PS2X_type); break;
    case 1: Serial.printf("DualShock Controller found, type is %d", PS2X_type); break;
    case 2: Serial.printf("GuitarHero Controller found, type is %d", PS2X_type); break;
    case 3: Serial.printf("Wireless Sony DualShock Controller found, type is %d", PS2X_type); break;
  }
}
