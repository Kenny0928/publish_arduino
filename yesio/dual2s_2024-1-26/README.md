# Dual2s 程式庫使用說明

這份文件提供了YESIO公司Dual系列自走車程式庫的詳細說明，旨在幫助學生快速了解和使用各種可用函式。

## 目錄

- [安裝說明](#安裝說明)
- [基本概念](#基本概念)
- [類別與函式](#類別與函式)
  - [GoSUMO類別](#gosumo類別)
  - [Motor類別](#motor類別)
  - [HCSR04類別](#hcsr04類別)
  - [IR3CH類別](#ir3ch類別)
  - [Buzzer類別](#buzzer類別)
  - [Power類別](#power類別)
- [範例程式](#範例程式)
- [常見問題](#常見問題)

## 安裝說明

1. 下載程式庫：從[GitHub](https://github.com/yesio/dual2s)下載最新版本的Dual2s程式庫
2. 安裝程式庫：
   - 打開Arduino IDE
   - 點選「草稿碼」>「匯入程式庫」>「加入.ZIP程式庫」
   - 選擇下載的Dual2s程式庫ZIP檔案
3. 確認安裝：安裝完成後，你可以在「草稿碼」>「匯入程式庫」中找到Dual2s程式庫

## 基本概念

Dual2s程式庫專為ESP32微控制器設計，用於控制YESIO公司的Dual系列自走車。程式庫提供了多個類別，用於控制自走車的各種功能，包括馬達控制、感測器讀取、聲音輸出和電源管理。

## 類別與函式

### GoSUMO類別

GoSUMO類別用於控制四輪驅動的相撲機器人，提供基本的移動功能。

#### 建構函式

```cpp
GoSUMO();
```

- **說明**：初始化GoSUMO類別，設定馬達引腳和PWM通道

#### 函式

```cpp
void act(uint8_t motion, uint16_t speed);
```

- **說明**：控制機器人的移動方向和速度
- **參數**：
  - `motion`：移動方向，可選值為：
    - `GoSUMO::FORWARD`：前進
    - `GoSUMO::BACKWARD`：後退
    - `GoSUMO::GO_LEFT`：左轉
    - `GoSUMO::GO_RIGHT`：右轉
  - `speed`：移動速度，範圍為0-1023

```cpp
void stop();
```

- **說明**：停止所有馬達

#### 使用範例

```cpp
#include <dual2s.h>

GoSUMO gs;

void setup() {
  // 初始化設定
}

void loop() {
  gs.act(GoSUMO::FORWARD, 800);  // 以800的速度前進
  delay(1000);                   // 等待1秒
  gs.stop();                     // 停止
  delay(500);                    // 等待0.5秒
  gs.act(GoSUMO::GO_LEFT, 600);  // 以600的速度左轉
  delay(1000);                   // 等待1秒
  gs.stop();                     // 停止
}
```

### Motor類別

Motor類別用於控制單個直流馬達，提供基本的旋轉控制功能。

#### 建構函式

```cpp
Motor(uint8_t pin_a, uint8_t pin_b, uint8_t channel_a, uint8_t channel_b);
```

- **說明**：初始化Motor類別
- **參數**：
  - `pin_a`：馬達A相引腳
  - `pin_b`：馬達B相引腳
  - `channel_a`：馬達A相PWM通道
  - `channel_b`：馬達B相PWM通道

#### 函式

```cpp
void act(uint8_t dir, uint16_t speed);
```

- **說明**：控制馬達的旋轉方向和速度
- **參數**：
  - `dir`：旋轉方向，可選值為：
    - `Motor::CW`：順時針旋轉
    - `Motor::CCW`：逆時針旋轉
    - `Motor::STOP`：停止
  - `speed`：旋轉速度，範圍為0-1023

```cpp
void stop();
```

- **說明**：停止馬達

#### 使用範例

```cpp
#include <dual2s.h>

#define PIN_MOTO_1A  14
#define PIN_MOTO_1B  32
#define PWM_CH_M1A   8
#define PWM_CH_M1B   9

Motor motor1(PIN_MOTO_1A, PIN_MOTO_1B, PWM_CH_M1A, PWM_CH_M1B);

void setup() {
  // 初始化設定
}

void loop() {
  motor1.act(Motor::CW, 800);  // 以800的速度順時針旋轉
  delay(1000);                 // 等待1秒
  motor1.stop();               // 停止
  delay(500);                  // 等待0.5秒
  motor1.act(Motor::CCW, 600); // 以600的速度逆時針旋轉
  delay(1000);                 // 等待1秒
  motor1.stop();               // 停止
}
```

### HCSR04類別

HCSR04類別用於控制超音波感測器，提供距離測量和物體偵測功能。

#### 建構函式

```cpp
HCSR04(uint8_t pin_echo, uint8_t pin_trig);
```

- **說明**：初始化HCSR04類別
- **參數**：
  - `pin_echo`：超音波感測器Echo引腳
  - `pin_trig`：超音波感測器Trig引腳

#### 函式

```cpp
float ObjDistance();
```

- **說明**：測量物體距離
- **回傳值**：物體距離，單位為公分

```cpp
bool ObjSeeking(uint8_t thresh);
```

- **說明**：偵測是否有物體在指定距離內
- **參數**：
  - `thresh`：距離閾值，單位為公分
- **回傳值**：
  - `true`：有物體在指定距離內
  - `false`：沒有物體在指定距離內

```cpp
unsigned long probing();
```

- **說明**：發送超音波並測量回波時間
- **回傳值**：回波時間，單位為微秒

#### 使用範例

```cpp
#include <dual2s.h>

#define PIN_USC_ECHO 13
#define PIN_USC_TRIG 27

HCSR04 usc(PIN_USC_ECHO, PIN_USC_TRIG);

void setup() {
  Serial.begin(115200);
}

void loop() {
  float distance = usc.ObjDistance();
  Serial.print("距離: ");
  Serial.print(distance);
  Serial.println(" 公分");
  
  if (usc.ObjSeeking(20)) {
    Serial.println("偵測到物體在20公分內！");
  }
  
  delay(500);
}
```

### IR3CH類別

IR3CH類別用於控制三通道紅外線感測器，提供循跡功能。

#### 建構函式

```cpp
IR3CH(uint8_t pin_L, uint8_t pin_M, uint8_t pin_R);
```

- **說明**：初始化IR3CH類別
- **參數**：
  - `pin_L`：左側紅外線感測器引腳
  - `pin_M`：中間紅外線感測器引腳
  - `pin_R`：右側紅外線感測器引腳

#### 函式

```cpp
byte ReadTCRT(uint8_t TH, bool DEBUG);
```

- **說明**：讀取三通道紅外線感測器的值
- **參數**：
  - `TH`：感測器閾值，用於判斷黑白線
  - `DEBUG`：是否輸出除錯信息
- **回傳值**：感測器狀態，以二進制表示，例如：
  - `B000`：三個感測器都沒有偵測到黑線
  - `B001`：右側感測器偵測到黑線
  - `B010`：中間感測器偵測到黑線
  - `B100`：左側感測器偵測到黑線
  - `B011`：右側和中間感測器偵測到黑線
  - 其他組合...

#### 使用範例

```cpp
#include <dual2s.h>

#define PIN_IR_LEFT   34
#define PIN_IR_MIDDLE 35
#define PIN_IR_RIGHT  39

IR3CH ir(PIN_IR_LEFT, PIN_IR_MIDDLE, PIN_IR_RIGHT);

void setup() {
  Serial.begin(115200);
}

void loop() {
  byte irStatus = ir.ReadTCRT(500, true);  // 閾值設為500，開啟除錯模式
  
  switch (irStatus) {
    case B000:
      Serial.println("沒有偵測到黑線");
      break;
    case B010:
      Serial.println("中間感測器偵測到黑線");
      break;
    case B001:
      Serial.println("右側感測器偵測到黑線");
      break;
    case B100:
      Serial.println("左側感測器偵測到黑線");
      break;
    default:
      Serial.print("複合狀態: ");
      Serial.println(irStatus, BIN);
  }
  
  delay(500);
}
```

### Buzzer類別

Buzzer類別用於控制蜂鳴器，提供發出音調和警報的功能。

#### 建構函式

```cpp
Buzzer(byte pin, byte channel);
```

- **說明**：初始化Buzzer類別
- **參數**：
  - `pin`：蜂鳴器引腳
  - `channel`：PWM通道

#### 函式

```cpp
void tone(uint16_t frequency, uint16_t duration, uint8_t volume);
```

- **說明**：發出指定頻率、持續時間和音量的音調
- **參數**：
  - `frequency`：音調頻率，單位為赫茲
  - `duration`：持續時間，單位為毫秒
  - `volume`：音量，範圍為0-255

```cpp
void alarm();
```

- **說明**：發出警報聲

```cpp
void noTone();
```

- **說明**：停止發聲

#### 音調常數

程式庫提供了一組音調常數，可用於產生音樂：

```cpp
Pitch pitch;  // 建立Pitch物件

// 使用音調常數
pitch.Do_3;   // Do (低音)
pitch.Re_3;   // Re (低音)
// ...
pitch.Do_4;   // Do (中音)
// ...
pitch.Do_5;   // Do (高音)
// ...
```

#### 使用範例

```cpp
#include <dual2s.h>

#define PIN_BUZZER  15
#define PWM_CH_BUZZER 7

Buzzer bz(PIN_BUZZER, PWM_CH_BUZZER);
Pitch pitch;  // 音調常數

void setup() {
  // 初始化設定
}

void loop() {
  bz.alarm();           // 發出警報聲
  delay(1000);          // 等待1秒
  
  bz.tone(pitch.Do_4, 500, 50);  // 發出Do音，持續500毫秒，音量50
  delay(100);                     // 短暫停頓
  bz.tone(pitch.Re_4, 500, 50);  // 發出Re音，持續500毫秒，音量50
  delay(100);                     // 短暫停頓
  bz.tone(pitch.Mi_4, 500, 50);  // 發出Mi音，持續500毫秒，音量50
  
  delay(2000);          // 等待2秒
}
```

### Power類別

Power類別用於監測電池電壓，提供電壓讀取功能。

#### 建構函式

```cpp
Power(uint8_t pin);
```

- **說明**：初始化Power類別
- **參數**：
  - `pin`：電池電壓監測引腳

#### 函式

```cpp
float voltage();
```

- **說明**：讀取電池電壓
- **回傳值**：電池電壓，單位為伏特

#### 使用範例

```cpp
#include <dual2s.h>

#define PIN_BATTERY  36

Power power(PIN_BATTERY);

void setup() {
  Serial.begin(115200);
}

void loop() {
  float batteryVoltage = power.voltage();
  Serial.print("電池電壓: ");
  Serial.print(batteryVoltage);
  Serial.println(" V");
  
  if (batteryVoltage < 7.0) {
    Serial.println("警告：電池電量低！");
  }
  
  delay(1000);
}
```

## 範例程式

Dual2s程式庫提供了多個範例程式，展示如何使用各種功能：

1. **Dual2s-PS2**：使用PS2控制器控制自走車
   - 位置：`範例 > Dual2s > Dual2s-PS2`
   - 功能：使用PS2控制器控制自走車的移動

2. **Dual2s-BT**：使用藍牙控制自走車
   - 位置：`範例 > Dual2s > Dual2s-BT`
   - 功能：使用藍牙控制自走車的移動

3. **Dual2s_GoARM-PS2**：控制帶有機械臂的自走車
   - 位置：`範例 > Dual2s > Dual2s_GoARM-PS2`
   - 功能：使用PS2控制器控制帶有機械臂的自走車

## 常見問題

### 問題1：馬達不轉動

**可能原因**：
- 馬達連接錯誤
- PWM通道設定錯誤
- 電池電量不足

**解決方法**：
- 檢查馬達連接是否正確
- 確認PWM通道設定是否正確
- 檢查電池電量

### 問題2：超音波感測器讀數不準確

**可能原因**：
- 感測器連接錯誤
- 環境干擾
- 感測器角度不正確

**解決方法**：
- 檢查感測器連接是否正確
- 避免在有強烈聲音或風的環境中使用
- 調整感測器角度

### 問題3：紅外線感測器無法正確偵測黑線

**可能原因**：
- 感測器高度不適當
- 閾值設定不正確
- 環境光線干擾

**解決方法**：
- 調整感測器高度，建議在5-10mm之間
- 調整閾值設定，可以使用DEBUG模式查看實際讀數
- 避免在強光環境下使用

---

如有更多問題，請訪問[YESIO官方網站](https://yesio.net/)或[GitHub頁面](https://github.com/yesio/dual2s)獲取支援。 