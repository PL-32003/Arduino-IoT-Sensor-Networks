#include <SPI.h>
#include <MFRC522.h>

// --------------------
// 腳位設定
// --------------------
#define SS_PIN 10    // MFRC522 的 SDA/SS 腳 (SPI chip select)
#define RST_PIN 9    // MFRC522 的 RST (reset)
#define RED_LED 5    // 紅色 LED（錯誤/拒絕提示）
#define GREEN_LED 6  // 綠色 LED（成功/通過提示）
#define BUZZER 7     // 蜂鳴器

// 建立 MFRC522 物件，使用 SS_PIN 與 RST_PIN
MFRC522 rfid(SS_PIN, RST_PIN);

// --------------------
// 使用者資料結構（User）與儲存空間設定
// --------------------
// 每個 User 包含卡片 ID（字串）與密碼（字串）
struct User 
{
  String id;        // 卡片 UID（以十六進位字串表示）
  String password;  // 該使用者的密碼
};

#define MAX_USERS 10  // 最多允許的使用者數量

// users 陣列保存所有帳號資訊，預設第 0 個為管理員
User users[MAX_USERS] = 
{
  {"FA2958BD", "0000"}, // 預設一個管理員，卡片 ID 與初始密碼
};
int userCount = 1;     // 目前使用者數量（陣列已填入的數量），初始為 1
int currentUser = -1;  // 記錄目前登入的使用者索引，-1 表示未登入

// --------------------
// 輔助函式：蜂鳴器與 LED 提示
// - 這些是 UI 回饋，讓使用者知道系統狀態
// --------------------
// beep: 讓蜂鳴器嗶嗶 n 次，每次持續 duration 毫秒
void beep(int times, int duration) 
{
  for (int i = 0; i < times; i++) 
  {
    digitalWrite(BUZZER, HIGH);   // 開啟蜂鳴器
    delay(duration);              // 等待 duration 毫秒
    digitalWrite(BUZZER, LOW);    // 關閉蜂鳴器
    delay(duration);              // 等待 duration 毫秒
  }
}

// blinkLED: 指定腳位閃爍 n 次，每次持續 duration 毫秒
void blinkLED(int pin, int times, int duration) 
{
  for (int i = 0; i < times; i++) 
  {
    analogWrite(pin, 255);      // LED ON
    delay(duration);
    analogWrite(pin, 0);       // LED OFF
    delay(duration);
  }
}

// --------------------
// 讀卡功能：readCardID
// - 使用 MFRC522 API 判斷是否有新卡並讀取 UID
// - 回傳格式為十六進位的大寫字串，例如 "FA2958BD"
// - 若沒有新卡或讀卡失敗，回傳空字串 ""
// --------------------
String readCardID() 
{
  // 先檢查是否有新卡進入感應範圍，且能成功讀取序列號
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
    return "";  // 沒有新卡或讀卡失敗，回傳空字串

  String id = "";
  // rfid.uid.uidByte[] 裡面是 UID 的每一個 byte
  // 將每個 byte 轉成 HEX 字串並連接
  for (byte i = 0; i < rfid.uid.size; i++) 
  {
    // String(value, HEX) 會把數值轉成十六進位字串
    id += String(rfid.uid.uidByte[i], HEX);
  }

  id.toUpperCase();      // 把整個字串轉成大寫，方便比對
  rfid.PICC_HaltA();     // 停止與卡片的通訊（釋放卡片）
  return id;             // 回傳 UID 字串
}

// --------------------
// findUser:
// - 在 users 陣列中查詢指定 id，找到則回傳該索引，找不到回傳 -1
// --------------------
int findUser(String id) 
{
  for (int i = 0; i < userCount; i++) 
  {
    if (users[i].id == id) return i;  // 找到相同 ID，回傳索引
  }
  return -1; // 未找到
}

// --------------------
// logout: 清除登入狀態並提示
// --------------------
void logout() 
{
  currentUser = -1;  // 重置登入索引
  Serial.println("登出成功，請刷卡以登入");
}

// --------------------
// 登入流程：loginFlow
// - 持續等待刷卡（在此函式內是阻塞式等待）
// - 若卡片存在且已註冊，要求輸入密碼驗證
// - 驗證成功後設定 currentUser 為該使用者的索引，並返回
// --------------------
void loginFlow() 
{
  Serial.println("請刷卡以登入");
  while (true) 
  {
    // 讀一次卡片 ID；若沒有卡或讀失敗，readCardID 會回傳空字串
    String cardID = readCardID();
    if (cardID == "") continue;   // 若沒卡，繼續等待（迴圈回到開頭）

    // 顯示讀到的卡片 ID
    Serial.print("偵測到卡片 ID: ");
    Serial.println(cardID);

    // 在已註冊的使用者中查詢此卡片是否存在
    int idx = findUser(cardID);
    if (idx == -1) 
    {
      // 查無此卡：提示錯誤（紅燈 + 兩聲嗶）並繼續等待刷卡
      blinkLED(RED_LED, 2, 200);
      beep(2, 150);
      Serial.println("查無此卡，請刷卡以登入");
      continue;
    }

    // 若卡片已存在於系統中，要求輸入密碼（來自 Serial）
    Serial.print("請輸入密碼：");
    // 等待使用者透過序列埠輸入密碼（阻塞直到有輸入）
    while (Serial.available() == 0);
    String pw = Serial.readStringUntil('\n');
    pw.trim(); // 移除行尾換行與空白

    // 比對輸入的密碼與系統儲存的密碼
    if (pw == users[idx].password) 
    {
      // 密碼正確：提示成功（綠燈 + 一聲嗶）並設定登入使用者
      blinkLED(GREEN_LED, 1, 300);
      beep(1, 200);
      Serial.println("登入成功！");
      currentUser = idx; // 記錄登入者的陣列索引
      break; //
    } 
    else 
    {
      // 密碼錯誤：提示失敗並回到等待刷卡狀態
      blinkLED(RED_LED, 2, 200);
      beep(2, 150);
      Serial.println("密碼錯誤！");
      Serial.println("請刷卡以登入");
    }
  }
}

// --------------------
// 管理員功能
// --------------------
// listUsers: 列出目前所有使用者（含 ID 與密碼及是否為管理員）
void listUsers() 
{
  Serial.println("目前帳號清單：");
  for (int i = 0; i < userCount; i++) 
  {
    // 顯示編號（從 1 開始）、卡片 ID 與密碼
    Serial.print(String(i + 1) + ". " + users[i].id + " " + users[i].password);
    // 顯示是否為管理員
    if (i == 0) Serial.println(" admin");
    else Serial.println(" member");
  }
}

// addUser: 新增使用者流程
// - 要求刷新的卡片（該卡片將被當作新使用者的 ID）
// - 要求輸入密碼，並加入 users 陣列
void addUser() 
{
  // 如果已達到上限，拒絕新增
  if (userCount >= MAX_USERS) 
  {
    Serial.println("使用者數量已滿！");
    return;
  }

  Serial.print("請刷新卡：");
  String newID = "";
  // 持續等待直到成功讀到卡片為止
  while (newID == "") 
  {
    newID = readCardID();
  }
  Serial.println(newID);

  // 如果卡片已經存在，拒絕新增
  if (findUser(newID) != -1) 
  {
    Serial.println("此卡已存在！");
    return;
  }

  // 要求設定新密碼
  Serial.print("請輸入新密碼：");
  // 等待使用者透過序列埠輸入新密碼（阻塞直到有輸入）
  while (Serial.available() == 0);
  String pw = Serial.readStringUntil('\n');
  pw.trim();

  // 儲存到 users 陣列，並增加使用者計數
  users[userCount++] = {newID, pw};
  Serial.println("新增成功");
}

// removeUser: 刪除指定編號的使用者
// - 使用者以序號輸入（介於 1 到 userCount）
// - 不允許刪除管理員（index = 0）
void removeUser() 
{
  Serial.print("請輸入要刪除的使用者編號：");
  // 等待使用者透過序列埠輸入使用者編號（阻塞直到有輸入）
  while (Serial.available() == 0);
  int idx = Serial.parseInt() - 1; // 將輸入的序號轉為索引（從 0 開始）

  // 檢查輸入是否有效
  if (idx < 0 || idx >= userCount) 
  {
    Serial.println("無效的編號！");
    return;
  }

  // 檢查是否為管理員
  if (idx == 0) 
  {
    // 不允許刪除管理員（位置 0）
    Serial.println("不可刪除管理員！");
    return;
  }

  // 將後面的使用者前移覆蓋欲刪除的位置
  for (int i = idx; i < userCount - 1; i++) users[i] = users[i + 1];
  userCount--; // 使用者總數減 1
  Serial.println("刪除成功");
}

// editAdmin: 更換管理員（將指定使用者移到 index 0）
// - 輸入欲成為新管理員的使用者編號
void editAdmin() 
{
  Serial.print("請輸入要設為新管理員的編號：");
  // 等待使用者透過序列埠輸入使用者編號（阻塞直到有輸入）
  while (Serial.available() == 0);
  int idx = Serial.parseInt() - 1; // 將輸入的序號轉為索引（從 0 開始）

  // 檢查輸入是否有效
  if (idx < 0 || idx >= userCount) 
  {
    Serial.println("無效的編號！");
    return;
  }

  // 交換 users[0] 與 users[idx]，使選定使用者成為管理員
  User temp = users[0];
  users[0] = users[idx];
  users[idx] = temp;
  Serial.println("變更成功");
}

// --------------------
// 一般使用者功能
// --------------------
// updatePassword: 讓目前登入的使用者更新自己的密碼
// - 要求輸入兩次新密碼以確認
void updatePassword() 
{
  Serial.println("請輸入新密碼：");
  // 等待使用者透過序列埠輸入新密碼（阻塞直到有輸入）
  while (Serial.available() == 0);
  String p1 = Serial.readStringUntil('\n'); 
  p1.trim(); // 移除行尾換行與空白

  Serial.print("請再輸入一次新密碼：");
  // 等待使用者透過序列埠重新輸入新密碼以確認（阻塞直到有輸入）
  while (Serial.available() == 0);
  String p2 = Serial.readStringUntil('\n'); 
  p2.trim(); // 移除行尾換行與空白

  if (p1 != p2) 
  {
    // 若兩次輸入不一致，取消更新
    Serial.println("兩次密碼不一致！");
    return;
  }

  // 更新使用者資料中的密碼欄位
  users[currentUser].password = p1;
  Serial.println("更新成功");
}

// deleteSelf: 刪除目前登入使用者的帳號
// - 會把陣列中後面的項目前移來覆蓋自己的位置
void deleteSelf() 
{
  for (int i = currentUser; i < userCount - 1; i++) users[i] = users[i + 1];
  userCount--;
  Serial.println("移除成功");
}

// --------------------
// 指令解析：commandLoop
// - 根據目前登入者是否為管理員提供不同的指令集
// - 讀取輸入後呼叫對應的處理函式
// - 執行完後執行 logout()
// 注意：此設計為阻塞式的簡單指令處理
// --------------------
void commandLoop() 
{
  // 根據 currentUser 決定顯示指令
  if (currentUser == 0) 
  {
    Serial.println("可用指令：ADD / REMOVE / LIST / EDITADMIN");
  } 
  else 
  {
    Serial.println("可用指令：UPDATE / DEL");
  }

  Serial.print("> ");
  // 等待使用者輸入指令字串
  while (Serial.available() == 0);
  String cmd = Serial.readStringUntil('\n');
  cmd.trim(); 
  cmd.toUpperCase(); // 去掉空白並轉成大寫方便比對

  // 管理員的指令解析
  if (currentUser == 0) 
  {
    if (cmd == "ADD") addUser();
    else if (cmd == "REMOVE") removeUser();
    else if (cmd == "LIST") listUsers();
    else if (cmd == "EDITADMIN") editAdmin();
    else Serial.println("未知指令");
  } 
  else 
  {
    // 一般使用者的指令解析
    if (cmd == "UPDATE") updatePassword();
    else if (cmd == "DEL") deleteSelf();
    else Serial.println("未知指令");
  }

  // 指令執行完畢後自動登出，回到初始刷卡狀態
  logout();
}

// --------------------
// 初始化：setup()
// - 設定 Serial、SPI、RFID 初始化，並將 LED 與蜂鳴器腳位設為輸出
// --------------------
void setup() 
{
  Serial.begin(9600); // 與電腦或序列終端通訊
  SPI.begin();        // 啟用 SPI
  rfid.PCD_Init();    // 初始化 MFRC522

  // 設定提示裝置腳位為輸出模式
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
}

// --------------------
// 主迴圈：loop()
// - 主要流程：先執行登入流程（loginFlow），登入成功後進入指令迴圈（commandLoop）
// - 完成指令後自動登出並重新回到等待刷卡
// --------------------
void loop() 
{
  loginFlow();   // 阻塞式等待刷卡並登入
  commandLoop(); // 執行使用者指令（執行完會登出）
}
