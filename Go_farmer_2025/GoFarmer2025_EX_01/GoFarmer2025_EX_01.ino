/*-- 註解區
> GoFarmer_a_tesst_01 | ESP32 | 專題範例程式
> 原作者 / Author : Nick, 2023.01.29 / https://yesio.net/
>改編/ Ken ,2025.0504
--*/
/*-- 標頭檔區(程式庫) --*/
#include <dual2s.h>
#include "BluetoothSerial.h"
#include <Servo.h>

/*-- 腳位定義區 --*/
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

/*-- 宣告區(全域變數) --*/
BluetoothSerial SerialBT;  //藍牙串列物件 
char BTCMD[3] = {0};  //儲存藍牙串列接收到的指令
int PWMspeed = 600;  //儲存藍牙串列接收到的PWM speed數值
int SeekCnt = 0;

#define LOW_POWER_PROTECT
unsigned long lastTime = 0;
unsigned long timerDelay = 1000; //每2秒檢查一次供電電壓
float low_power_threadhold = 6.5;//低電壓警示值設定

//Servo Motor 變數區 & 機械臂微調區 -------
//重要，詳見影片介紹：https://youtu.be/PnZQBKgZXok
Servo servoBase;
Servo servoPTZ_LR;   //雲台左右
Servo servoPTZ_UD;   //雲台上下
int homeBase = 90; //機械臂原點角度。
int homePTZ_LR = 40;  //雲台左右原點角度。
int homePTZ_UD = 6;   //雲台上下原點角度。

int posBase = homeBase;
int posBaseMAX = 175; //機械臂最大抬升角度。
int posBaseMIN = homeBase;  //機械臂最小降低角度。
int posPTZ_LR = homePTZ_LR;
int posPTZ_LR_MAX = 180;   //雲台左右最大角度。
int posPTZ_LR_MIN = 0;     //雲台左右最小角度。
int posPTZ_UD = homePTZ_UD;
int posPTZ_UD_MAX = 80;    //雲台上下最大角度。
int posPTZ_UD_MIN = 6;     //雲台上下最小角度。

// 新增 dual2s 物件
GoSUMO robot;
Power power(PIN_BATTERY);  // 電池電壓檢測
Buzzer buzzer(PIN_BUZZER, PWM_CH_BUZZER);  // 蜂鳴器



/*-- 初始化區 --*/
void setup()
{
  SerialBT.begin("FA_01");  //啟動藍牙串列並設定藍牙裝置名稱
  Serial.begin(115200);  // 初始化序列埠
  Serial.println("The device started, now you can pair it with bluetooth!");
  buzzer.tone(600, 200, 50);  // 使用新的蜂鳴器控制

  //ServoMotor初始化
  servoBase.attach(18);     //舉臂 
  servoPTZ_LR.attach(19);   //雲台左右
  servoPTZ_UD.attach(2);   //雲台上下or土壤sensor探針上下
  
  ServoARM("Home", "");  
}

/*-- 主程式區(重複執行) --*/
void loop()
{   
#ifdef LOW_POWER_PROTECT   
  if ((millis() - lastTime) > timerDelay) {
    lastTime = millis();
    if(power.voltage() < low_power_threadhold){
      buzzer.tone(800, 200, 50);
    }
  }
#endif

  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available()) {
    int i;
    for (i=0; i<4; i++){ BTCMD[i] = SerialBT.read(); } 
  }

  if(atoi(BTCMD)>1) { PWMspeed = atoi(BTCMD);}

  if(BTCMD[0] == 'c') { BTReportADC(); BTCMD[0] = 'c'; }  
  else if(BTCMD[0] == 'F') { robot.act(GoSUMO::FORWARD, PWMspeed); }  //GoSUMO機器人向前進 
  else if(BTCMD[0] == 'B') { robot.act(GoSUMO::BACKWARD, PWMspeed); }  //GoSUMO機器人向後退 
  else if(BTCMD[0] == 'L') { robot.act(GoSUMO::GO_LEFT, PWMspeed); }
  else if(BTCMD[0] == 'R') { robot.act(GoSUMO::GO_RIGHT, PWMspeed); }
  else if(BTCMD[0] == 'Z') { robot.stop(); }
  else if(BTCMD[0] == 'H') { buzzer.tone(600, 200, 50); }
/*Servo Control, start here. */  
  else if(BTCMD[0] == 'A') { ServoARM("Base", "UP");  }	  //ServoBase Rising
  else if(BTCMD[0] == 'a') { ServoARM("Base", "DOWN");  }	//ServoBase Declining
  else if(BTCMD[0] == 'P') { ServoARM("PTZ_LR", "RIGHT");  }	//雲台左右向右
  else if(BTCMD[0] == 'p') { ServoARM("PTZ_LR", "LEFT");  }	//雲台左右向左
  else if(BTCMD[0] == 'u') { ServoARM("PTZ_UD", "UP");  }	//雲台上下向上
  else if(BTCMD[0] == 'U') { ServoARM("PTZ_UD", "DOWN");  }	//雲台上下向下
  else if(BTCMD[0] == 'z') { ServoARM("Home", "");  BTCMD[0] = 'Z';} //ARM Homing
  
}

/*====== 副程式區，start here. ========*/

void ServoARM(String _ACTION, String _DIR){
	if(_ACTION == "Base"){
	  if((_DIR == "UP")&&(posBase < posBaseMAX )){servoBase.write(++posBase); delay(15);  Serial.println("baseUP");}
	  if((_DIR == "DOWN")&&(posBase > posBaseMIN )){servoBase.write(--posBase); delay(15); Serial.println("baseDOWN");}
	}
	
	if(_ACTION == "PTZ_LR"){
		if((_DIR == "RIGHT")&&(posPTZ_LR < posPTZ_LR_MAX )){servoPTZ_LR.write(++posPTZ_LR); delay(15); Serial.println("PTZ_LR_RIGHT");}
		if((_DIR == "LEFT")&&(posPTZ_LR > posPTZ_LR_MIN )){servoPTZ_LR.write(--posPTZ_LR); delay(15); Serial.println("PTZ_LR_LEFT");}
	}
	
	if(_ACTION == "PTZ_UD"){
		if((_DIR == "UP")&&(posPTZ_UD < posPTZ_UD_MAX )){servoPTZ_UD.write(++posPTZ_UD); delay(15); Serial.println("PTZ_UD_UP");}
		if((_DIR == "DOWN")&&(posPTZ_UD > posPTZ_UD_MIN )){servoPTZ_UD.write(--posPTZ_UD); delay(15); Serial.println("PTZ_UD_DOWN");}
	}
	
	if(_ACTION == "Home"){
    Serial.println("servoHome");
	   servoBase.write(posBaseMAX);
     servoPTZ_LR.write(homePTZ_LR);
     servoPTZ_UD.write(homePTZ_UD);
     posPTZ_LR = homePTZ_LR;
     posPTZ_UD = homePTZ_UD;
     posBase = homeBase;
	   delay(1000);
	}	
}

void BTReportADC()
{
  float batValue = power.voltage();
  SerialBT.println(batValue);
}



