int ldrPins[3] = {A0, A1, A2};  // 光敏
int ledPins[3] = {9, 10, 11};  // LED (PWM)
int sensorValue[3] = {0};  // 光敏電阻數值
int buttonPin = 2;  // 按鈕
bool justEntered = false;  // 判斷是否剛進入該模式
int baseline[3] = {0};  // 紀錄基準光值

int mode = 0;  // 當前模式
bool lastButtonState = HIGH;  // 前一次的按鈕狀態

// 初始化設定
void setup() 
{
  // put your setup code here, to run once:
  for (int i = 0; i < 3; i++) 
  {
    pinMode(ledPins[i], OUTPUT);  // 將 LED 腳位設為輸出
  }
  pinMode(buttonPin, INPUT_PULLUP);  // 將按鈕腳位設為輸入
  Serial.begin(9600);  // 初始化序列通訊，速率為 9600 bps
}

// 主程式
void loop() 
{
  // put your main code here, to run repeatedly:
  // 按鈕切換模式
  bool buttonState = digitalRead(buttonPin);  // 讀取按鍵的狀態
  if (lastButtonState == HIGH && buttonState == LOW)
  {
    mode++;
    if (mode > 2) mode = 0;  // 三種模式循環
    Serial.print("切換到模式 ");
    Serial.println(mode + 1);
    justEntered = true;  // 標記「剛進入模式」
    delay(50); // 防止按鈕抖動 (Debounce)
  }
  lastButtonState = buttonState;  // 更新前一次的按鈕狀態

  // 序列埠輸入切換模式
  if (Serial.available()) 
  {
    String command = Serial.readStringUntil('\n');  // 讀取指令
    command.trim();  // 去掉多餘的換行/空白

    if (command == "1") 
    {
      mode = 0;
      justEntered = true;  // 標記「剛進入模式」
    }
    else if (command == "2") 
    {
      mode = 1;
      justEntered = true;  // 標記「剛進入模式」
    }
    else if (command == "3") 
    {
      mode = 2;
      justEntered = true;  // 標記「剛進入模式」
    }
  }

  if (mode == 0)  // 進入模式1
  {
    mode_1();
  } 
  else if (mode == 1)  // 進入模式2
  {    
    mode_2();
  } 
  else if (mode == 2)  // 進入模式3
  {    
    mode_3();
  }
}

// 模式1
void mode_1()
{
  if (justEntered) 
  {
    for(int i = 0; i < 3; i++) 
    {
      analogWrite(ledPins[i], 0);  // LED 歸 0
    }
    delay(300);  // 只在第一次進模式時等一下，作為進模式後的緩衝時間
    justEntered = false;
  }

  sensorValue[0] = analogRead(ldrPins[0]);  // 讀取類比輸入的值會得到 0 ~ 1023

  // 印出相對數值
  Serial.print("A0");
  Serial.print("的周圍亮度： ");
  Serial.println(sensorValue[0]);

  if(sensorValue[0] < 400)
  {
    for(int i = 0; i < 3; i++)
    {
      analogWrite(ledPins[i], 255);  // 將結果用 PWM 的方式輸出給 LED ，改變亮度
    }
  }
  else if(sensorValue[0] < 500)
  {
    for(int i = 0; i < 3; i++)
    {
      analogWrite(ledPins[i], 135);  // 將結果用 PWM 的方式輸出給 LED ，改變亮度
    }
  }
  else if(sensorValue[0] < 600)
  {
    for(int i = 0; i < 3; i++)
    {
      analogWrite(ledPins[i], 50);  // 將結果用 PWM 的方式輸出給 LED ，改變亮度
    }
  }
  else
  {
    for(int i = 0; i < 3; i++)
    {
      analogWrite(ledPins[i], 0);  // 將結果用 PWM 的方式輸出給 LED ，改變亮度
    }
  }
}

// 模式2
void mode_2()
{
  if (justEntered) 
  {
    for(int i = 0; i < 3; i++) 
    {
      analogWrite(ledPins[i], 0);  // LED 歸 0
    }
    delay(300);  // 只在第一次進模式時等一下，作為進模式後的緩衝時間
    justEntered = false;
  }

  sensorValue[0] = analogRead(ldrPins[0]);  // 讀取類比輸入的值會得到 0 ~ 1023
    
  // 印出相對數值
  Serial.print("A0");
  Serial.print("的周圍亮度： ");
  Serial.println(sensorValue[0]);

  int brightness = map(sensorValue[0],0,1023,0,255);  // 將 0 ~ 1023 轉化成 0 ~ 255
  for(int i = 0; i < 3; i++)
  {
    analogWrite(ledPins[i], 255 - brightness);  // 將結果用 PWM 的方式輸出給 LED ，改變亮度
  }
}

// 模式3
void mode_3()
{
  if (justEntered) 
  {
    for (int i = 0; i < 3; i++) 
    {
      baseline[i] = analogRead(ldrPins[i]); // 以目前周圍亮度設定基準亮度 
      analogWrite(ledPins[i], 0);  // LED 歸 0
    }
    delay(300);  // 只在第一次進模式時等一下，作為進模式後的緩衝時間
    justEntered = false;
  }

  int count = 0;  // 計算幾個光敏電阻被按下
  bool push[3] = {false};  // 紀錄哪個光敏電阻被按下

  for(int i = 0; i < 3; i++)
  {
    sensorValue[i] = analogRead(ldrPins[i]);  // 讀取類比輸入的值會得到 0 ~ 1023
    
    // 印出相對數值
    Serial.print("A");
    Serial.print(i);
    Serial.print("的周圍亮度： ");
    Serial.println(sensorValue[i]);

    if (sensorValue[i] < baseline[i] - 100)  // 視為被按下
    {
      push[i] = true;
      count++;
    }
  }

  for(int i = 0; i < 3; i++)
  {
    if(push[i] == true)
    {
      if(count == 1)
      {
        analogWrite(ledPins[i], 255);  // 將結果用 PWM 的方式輸出給 LED ，改變亮度
      }
      else if(count == 2)
      {
        analogWrite(ledPins[i], 135);  // 將結果用 PWM 的方式輸出給 LED ，改變亮度
      }
      else if(count == 3)
      {
        analogWrite(ledPins[i], 50);  // 將結果用 PWM 的方式輸出給 LED ，改變亮度
      }
    }
    else
    {
      analogWrite(ledPins[i], 0);  // 將結果用 PWM 的方式輸出給 LED ，改變亮度
    }
  }
}