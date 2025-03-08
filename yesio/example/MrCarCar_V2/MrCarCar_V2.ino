#include <dual2s.h>
#include "BluetoothSerial.h"
#include "HUSKYLENS.h"
#include <Servo.h>

//DC電機GPIO
#define PIN_MOTO_1A  14
#define PIN_MOTO_1B  32
#define PIN_MOTO_2A  12
#define PIN_MOTO_2B  33
#define PIN_MOTO_3A  25
#define PIN_MOTO_3B  26
#define PIN_MOTO_4A  23
#define PIN_MOTO_4B  22

//蜂鳴器GPIO
#define PIN_BUZZER   15

//超音波GPIO
#define PIN_USC_FRONT_ECHO 13
#define PIN_USC_FRONT_TRIG 27


//短距紅外線GPIO
#define PIN_IR_LEFT    34
#define PIN_IR_MIDDLE  35
#define PIN_IR_RIGHT   39

//供電偵測GPIO
#define PIN_BATTERY    36

//預設使用的PWM通道
#define PWM_CH_BUZZER 7
#define PWM_CH_M1A    8
#define PWM_CH_M1B    9
#define PWM_CH_M2A    10
#define PWM_CH_M2B    11
#define PWM_CH_M3A    12
#define PWM_CH_M3B    13
#define PWM_CH_M4A    14
#define PWM_CH_M4B    15

//伺服馬達相關定義
#define SERVO_PIN 19
#define SCAN_START_ANGLE 0
#define SCAN_END_ANGLE 90
#define SCAN_SPEED_SLOW 300    // 慢速每步停留時間(ms)
#define SCAN_SPEED_NORMAL 100  // 中速每步停留時間(ms)
#define SCAN_SPEED_FAST 50     // 快速每步停留時間(ms)
#define SERVO_MIN_STEP 3      // 最小步進角度
#define SERVO_MAX_STEP 10     // 最大步進角度

// 辨識結果緩衝區
#define MAX_BUFFER_SIZE 10
String recognizedNames[MAX_BUFFER_SIZE];
int bufferIndex = 0;

BluetoothSerial SerialBT;  //藍牙串列物件 
HUSKYLENS huskylens;       //HUSKYLENS物件
Servo cameraServo;         //伺服馬達物件
char BTCMD[3] = {0};       //儲存藍牙串列接收到的指令
int PWMspeed = 800;       //儲存藍牙串列接收到的PWM speed數值
int mode_change = 1;       //預設為藍芽控制模式
int SeekCnt = 0;

// 掃描控制相關變數
bool scanningComplete = false;
bool scanningDirection = true;
int currentAngle = SCAN_START_ANGLE;

GoSUMO gs;
Buzzer bz(PIN_BUZZER, PWM_CH_BUZZER);
Power volt(PIN_BATTERY);
IR3CH ir(PIN_IR_LEFT, PIN_IR_MIDDLE, PIN_IR_RIGHT);

// 掃描控制結構體
struct ScanControl {
    int currentSpeed;
    int currentStepSize;
    unsigned long lastMoveTime;
    
    ScanControl() {
        currentSpeed = SCAN_SPEED_NORMAL;
        currentStepSize = 5;
        lastMoveTime = 0;
    }
    
    void setSpeed(char speed) {
        switch(speed) {
            case 'S':
                currentSpeed = SCAN_SPEED_SLOW;
                currentStepSize = SERVO_MIN_STEP;
                break;
            case 'N':
                currentSpeed = SCAN_SPEED_NORMAL;
                currentStepSize = 5;
                break;
            case 'F':
                currentSpeed = SCAN_SPEED_FAST;
                currentStepSize = SERVO_MAX_STEP;
                break;
        }
    }
    
    bool canMove() {
        return (millis() - lastMoveTime) >= currentSpeed;
    }
    
    void updateMoveTime() {
        lastMoveTime = millis();
    }
} scanControl;

void setup() {
    Serial.begin(115200);
    SerialBT.begin("GS_smhs");  //啟動藍牙串列並設定藍牙裝置名稱
    
    bz.alarm();  //開機提示音
    Wire.begin(21,17);  //初始化HUSKYLENS

    cameraServo.attach(SERVO_PIN);
    cameraServo.write(SCAN_START_ANGLE);  // 設定初始角度為 0 度
    delay(500);  // 等待伺服馬達到達位置

    while (!huskylens.begin(Wire)) {
        Serial.println("HUSKYLENS初始化失敗！");
        SerialBT.println("HUSKYLENS初始化失敗！");
        delay(100);
    }
    
    
}

void loop() {
    // 藍芽資料接收
    if (SerialBT.available()) {
        int i;
        for (i=0; i<4; i++) { 
            BTCMD[i] = SerialBT.read(); 
        } 
        
        // 模式切換判斷
        if(BTCMD[0] == '1') {
            if(mode_change != 1) {
                mode_change = 1;
                gs.stop();
                BTCMD[0] = 'Z';
            }
        }
        if(BTCMD[0] == '2') {
            if(mode_change != 2) {
                mode_change = 2;
            }
        }
        if(BTCMD[0] == '4') {
            if(mode_change != 4) {
                mode_change = 4;
                gs.stop();
            }
        }
    }
    
    // 執行對應模式
    switch(mode_change) {
        case 1: mode_01(); break;
        case 2: mode_02(); break;
        case 3: mode_03(); break;
        case 4: mode_04(); break;
    }
}

// 藍芽遙控模式
void mode_01() {
    if(atoi(BTCMD)>1) { PWMspeed = atoi(BTCMD); }
    
    if(BTCMD[0] == 'F') { gs.act(GoSUMO::FORWARD, PWMspeed); }
    else if(BTCMD[0] == 'B') { gs.act(GoSUMO::BACKWARD, PWMspeed); }
    else if(BTCMD[0] == 'L') { gs.act(GoSUMO::GO_LEFT, PWMspeed); }
    else if(BTCMD[0] == 'R') { gs.act(GoSUMO::GO_RIGHT, PWMspeed); }
    else if(BTCMD[0] == 'Z') { gs.stop(); }
    else if(BTCMD[0] == 'H') { bz.alarm(); }
    else if(BTCMD[0] == 'S') { scanControl.setSpeed('S'); }
    else if(BTCMD[0] == 'N') { scanControl.setSpeed('N'); }
    else if(BTCMD[0] == 'Q') { scanControl.setSpeed('F'); }
}

// 循線模式
void mode_02() {
    if (ir.ReadTCRT(1000, 0) == 2) {       
        gs.act(GoSUMO::FORWARD, (PWMspeed-300));
    }
    else if (ir.ReadTCRT(2700, 0) == 1) {
        gs.act(GoSUMO::GO_RIGHT, (PWMspeed - 200));
    }
    else if (ir.ReadTCRT(2700, 0) == 4) {
        gs.act(GoSUMO::GO_LEFT, (PWMspeed - 200));
    }
    else if (ir.ReadTCRT(2700, 0) == 3) {
        gs.act(GoSUMO::GO_RIGHT, (PWMspeed - 200));
    }
    else if (ir.ReadTCRT(2700, 0) == 6) {
        gs.act(GoSUMO::GO_LEFT, (PWMspeed-200));
    }
    else if (ir.ReadTCRT(2800, 0) == 5) {
        gs.stop();
        delay(1000);
        mode_change = 3;
    }
    else if (ir.ReadTCRT(1000, 0) == 0 || ir.ReadTCRT(1000, 0) == 7) {
        gs.stop();
    }
}

// 人臉辨識模式
void mode_03() {
 static bool initializeServo = true;
    static bool scanningPhase = true;         // 用於區分「掃描中」與「掃描完，脫離中」
    
    if (initializeServo) {
        cameraServo.attach(SERVO_PIN);
        cameraServo.write(SCAN_START_ANGLE);
        initializeServo = false;
        clearNameBuffer();
        scanControl.setSpeed('N');
        scanningPhase = true;                 // 一開始先進入「掃描階段」
        scanningComplete = false;            
        scanningDirection = true;
        currentAngle = SCAN_START_ANGLE;
    }
    
    // --------------- 1. 掃描階段 ---------------
    if (scanningPhase) {
        // 持續執行掃描，直到 scanningComplete = true
        if (!scanningComplete && scanControl.canMove()) {
            if (scanningDirection) {
                currentAngle += scanControl.currentStepSize;
                if (currentAngle >= SCAN_END_ANGLE) {
                    scanningDirection = false;
                    currentAngle = SCAN_END_ANGLE;
                }
            } else {
                currentAngle -= scanControl.currentStepSize;
                if (currentAngle <= SCAN_START_ANGLE) {
                    scanningComplete = true;
                    currentAngle = SCAN_START_ANGLE;
                }
            }
            
            cameraServo.write(currentAngle);
            scanControl.updateMoveTime();
            
            // 讀取 HuskyLens 辨識結果
            if (!huskylens.request()) {
                Serial.println("HUSKYLENS無法取得資料！");
            }
            else if(!huskylens.isLearned()) {
                Serial.println("HUSKYLENS尚未學習任何物體！");
            }
            else if(huskylens.available()) {
                while (huskylens.available()) {
                    HUSKYLENSResult result = huskylens.read();
                    printResult(result);  // 依你需求，若不想即時輸出，可把printResult裡的SerialBT印出註解
                }
            }
        }
        // 如果完成掃描
        else if (scanningComplete) {
            // 一次性輸出辨識結果
            sendRecognitionResults(); 
            // 接著進入「脫離階段」，讓機器人離開這個區域
            scanningPhase = false;
        }
    }
    // --------------- 2. 前進脫離階段 ---------------
    else {
        // 不斷前進，直到 ir.ReadTCRT(1000, 0) == 2
        if (ir.ReadTCRT(1000, 0) != 2) {
            gs.act(GoSUMO::FORWARD, (PWMspeed - 300)); 
        } else {
            // 感測到直線 → 切回 mode_02
            gs.stop();
            delay(200);
            mode_change = 2;         
            
            // 重置局部變數，下次再進到 mode_03() 時會重新掃描
            initializeServo = true;  
        }
    }
}

// 測試模式
void mode_04() {
    static String inputBuffer = "";
    
    if (Serial.available()) {
        char c = Serial.read();
        
        if (c == '\n' || c == '\r') {
            inputBuffer.trim();
            
            if (inputBuffer.length() > 0) {
                // 檢查是否為數字
                bool isNumber = true;
                for(char d : inputBuffer) {
                    if(!isDigit(d)) {
                        isNumber = false;
                        break;
                    }
                }
                
                // 如果是數字，檢查是否在0-180範圍內
                if(isNumber) {
                    int angle = inputBuffer.toInt();
                    if(angle >= 0 && angle <= 180) {
                        cameraServo.write(angle);
                        Serial.print("伺服馬達轉至角度: ");
                        Serial.println(angle);
                        SerialBT.print("伺服馬達轉至角度: ");
                        SerialBT.println(angle);
                    } else {
                        SerialBT.println(inputBuffer);
                        Serial.print("已發送: ");
                        Serial.println(inputBuffer);
                    }
                } else {
                    SerialBT.println(inputBuffer);
                    Serial.print("已發送: ");
                    Serial.println(inputBuffer);
                }
                
                while (SerialBT.available()) {
                    SerialBT.read();
                }
                
                delay(50);
            }
            inputBuffer = "";
        } else {
            inputBuffer += c;
        }
    }
}

// 輸出辨識結果
void printResult(HUSKYLENSResult result) {
    String name;
    bool isValidID = true;
    
    switch(result.ID) {
        case 1:
            name = "廖啟岑";
            break;
        case 2:
            name = "郭彥廷";
            break;
        case 3:
            name = "謝睿璁";
            break;
        case 4:
            name = "林奕呈";
            break;
        default:
            isValidID = false;
            name = "未知ID: " + String(result.ID);
            break;
    }
    
    if (isValidID) {
        bool isDuplicate = false;
        for (int i = 0; i < bufferIndex; i++) {
            if (recognizedNames[i] == name) {
                isDuplicate = true;
                break;
            }
        }
        
        if (!isDuplicate && bufferIndex < MAX_BUFFER_SIZE) {
            recognizedNames[bufferIndex] = name;
            bufferIndex++;
            
            //Serial.println(name);
            //SerialBT.println(name);
            bz.tone(131, 1000, 50);
        }
    }
}

// 清除名字緩衝區
void clearNameBuffer() {
    bufferIndex = 0;
    for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
        recognizedNames[i] = "";
    }
}

// 發送辨識結果
void sendRecognitionResults() {
    SerialBT.println("<<START>>");
    
    for (int i = 0; i < bufferIndex; i++) {
        SerialBT.println(recognizedNames[i]);
    }
    
    SerialBT.println("<<END>>");
    clearNameBuffer();
}