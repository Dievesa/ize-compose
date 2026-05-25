#define ARDUINO_INKPLATE5V2 

#include <Adafruit_GFX.h>
#include "Inkplate.h"                
#include "U8g2_for_Adafruit_GFX.h"   
#include "Typewriter_16px.h" 
#include <esp_sleep.h>
#include <driver/uart.h>
#include <Update.h> // OTA 업데이트용 내장 라이브러리 추가
bool isFirmwareUpdateMode = false; // 일반 네트워크와 업데이트 모드 구분용 스위치
#define FIRMWARE_VERSION "v2.0.0" //rupertwriter.bin
const char* FIRMWARE_SIGNATURE = "RUPERT_OFFICIAL_KOR";
enum AppMode { TYPING_MODE, FILE_MENU_MODE, INITIAL_MODE, SEARCH_MODE, WIFI_SCAN_MODE, WIFI_PASSWORD_MODE };
class InkplateProxy : public Inkplate {
public:
    InkplateProxy(uint8_t mode) : Inkplate(mode), Adafruit_GFX(800, 600) {}
    void resetInternalCounter() { _partialUpdateCounter = 0; } 
};
extern InkplateProxy display;
InkplateProxy display(INKPLATE_1BIT); 
AppMode currentMode = TYPING_MODE;
bool needUpdate = false;
bool isSignatureChecked = false;
bool isSignatureValid = false;
bool isUpdating = false;             // 업데이트 중인지 확인하는 플래그
unsigned long lastActivityTime = 0;
#include "ZeroWriter_Helper.h"

// --- [ 듀얼코어: 백그라운드 계산] ---
TaskHandle_t CalcTaskHandle;
volatile bool needCountUpdate = false; // 계산 시작 신호
String calcBuffer = "";               // 계산용 텍스트 복사본
int sharedWordCount = 0;               // 계산 결과 임시 저장소
int sharedCharCount = 0;
AppMode lastMode = TYPING_MODE; 

NetworkSubMode currentNetSubMode = NET_MAIN;

#include "ZeroWriter_Helper.h"
bool isDeletingFile = false; 
String clipboard = "";
bool isEnglishInputMode = false;

NetworkSubMode tempNetCursor = NET_MAIN; // 스페이스바로 굴릴 '가짜 모드' 변수
float displayScale = 2.0;       
int baseFontSize = 16;     
String currentFileName = "doc_1.txt"; 

struct FileInfo {
    String name;
    String preview;
    float sizeKB;
    uint32_t time;
};
FileInfo files[65]; 
int fileCount = 0;

uint8_t *imgBuffer = NULL;
uint32_t imgSize = 0;

// --- [듀얼 코어 비동기 렌더링 전용 변수] ---

int refreshLimit = 2000;   
int charCounter = 0;    
int lineSpacing = 2;     
int letterSpacing = 0;   
int typingSpeed = 0;     
int leftMenuOffset = 0;
int countMode = 0;
int charwordcount = 0;
int latinMode = 0;
bool isAltPressed = false;
unsigned long lastTypingTime = 0; // 마지막으로 알파벳을 친 시간
char lastBaseChar = 0;            // 변환의 기준이 될 알파벳 (예: 'a')
int accentCycleIdx = 0;           // Alt를 누를 때마다 올라갈 순번
int lastAccentByteLen = 1;        // 직전에 찍힌 글자의 바이트 길이 (지울 때 필요함)

String searchQuery = ""; 
#include <Preferences.h> // 설정 저장용 라이브러리
Preferences prefs;
String fullText = ""; 
int cursorPos = 0; 
bool isKoreanMode = false; 
bool isShiftPressed = false; 
bool isCtrlPressed = false; 
bool isCapsLockOn = false; 
unsigned long lastKeyPress = 0; 
bool statusBarNeedsUpdate = true; 
unsigned long showSavedMessageTime = 0; 

// 자동 잠자기 관련 변수
int autoSleepIndex = 2; // 기본값 5분 (0:30s, 1:1m, 2:5m, 3:10m, 4:30m, 5:1h, 6:OFF)
unsigned long sleepIntervals[] = {30000, 60000, 300000, 600000, 1800000, 3600000, 0};
String sleepLabels[] = {"30s", "1m", "5m", "10m", "30m", "1h", "OFF"};
unsigned long lastInputTime = 0; // 마지막 키 입력 시간 저장

int startIdx = 0;
int menuFocusSide = 0;   
int leftMenuIndex = 0;   
int fileScrollOffset = 0; 
bool isEditingValue = false; 
bool inSystemSubMenu = false; // 시스템 설정 내부인지 확인

const char* choStrs[] = {"ㄱ","ㄲ","ㄴ","ㄷ","ㄸ","ㄹ","ㅁ","ㅂ","ㅃ","ㅅ","ㅆ","ㅇ","ㅈ","ㅉ","ㅊ","ㅋ","ㅌ","ㅍ","ㅎ"};
const char* jungStrs[] = {"ㅏ","ㅐ","ㅑ","ㅒ","ㅓ","ㅔ","ㅕ","ㅖ","ㅗ","ㅘ","ㅙ","ㅚ","ㅛ","ㅜ","우","ㅝ","ㅞ","ㅟ","ㅠ","ㅡ","ㅢ","ㅣ"};
int cho = -1, jung = -1, jong = -1; char lastJongChar = 0;
int rightFileIndex;
int oldCursorCx = 5;
int oldCursorCy = 35;
int lastSy = 0; 

class ScaledDisplay : public Adafruit_GFX {
public:
  InkplateProxy* tft;
  float* scaleRef; 
  ScaledDisplay(InkplateProxy* displayInstance, int16_t w, int16_t h, float* s) 
    : Adafruit_GFX(w, h), tft(displayInstance), scaleRef(s) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    tft->fillRect((int)(x * (*scaleRef)), (int)(y * (*scaleRef)), (int)ceil(*scaleRef), (int)ceil(*scaleRef), color);
  }
};

float currentActiveScale = 2.0;
ScaledDisplay bigDisplay(&display, 780, 600, &currentActiveScale); 
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

// --- 유틸리티 및 제어 함수 ---

int getTrueLength(String text) {
  int count = 0;
  for (int i = 0; i < text.length(); ) {
    int l = 1; if ((text[i] & 0x80) != 0) { if ((text[i] & 0xE0) == 0xC0) l = 2; else if ((text[i] & 0xF0) == 0xE0) l = 3; else l = 4; }
    i += l; count++;
  }
  return count;
}

String makeKorStr(int c, int ju, int jo) { 
  if (c == -1 || ju == -1) return ""; 
  int uni = 0xAC00 + (c * 21 * 28) + (ju * 28) + (jo == -1 ? 0 : jo); 
  char utf8[4]; utf8[0] = 0xE0 | ((uni >> 12) & 0x0F); utf8[1] = 0x80 | ((uni >> 6) & 0x3F); utf8[2] = 0x80 | (uni & 0x3F); utf8[3] = '\0'; 
  return String(utf8); 
}

void doBackspace() { 
    if (cho != -1 || jung != -1) { 
        if (jong != -1) { jong = -1; lastJongChar = 0; } 
        else if (jung != -1 && cho != -1) jung = -1;
        else { cho = -1; jung = -1; } 
    } else if (currentMode == SEARCH_MODE) {
        // 검색 모드에서의 백스페이스 (한글 바이트 깨짐 방지)
        if (searchQuery.length() > 0) {
            int p = searchQuery.length() - 1;
            while (p > 0 && (searchQuery[p] & 0xC0) == 0x80) p--;
            searchQuery = searchQuery.substring(0, p);
        }
    } else if (cursorPos > 0) { 
      if (isCtrlPressed) { 
            int p = cursorPos - 1;
            
            // 1. 커서 바로 앞의 공백이나 줄바꿈을 먼저 찾아서 지울 범위에 포함
            while (p > 0 && (fullText[p] == ' ' || fullText[p] == '\n')) p--;
            
            // 2. 단어의 시작점(공백이나 줄바꿈의 직후)을 찾을 때까지 거슬러 올라감
            while (p > 0 && fullText[p-1] != ' ' && fullText[p-1] != '\n') p--;
            
            // 3. 찾아낸 위치(p)부터 현재 커서까지 통째로 날림
            fullText = fullText.substring(0, p) + fullText.substring(cursorPos); 
            cursorPos = p;
        }
        else{
            // 기존 본문 백스페이스
            int p = cursorPos - 1;
            while (p > 0 && (fullText[p] & 0xC0) == 0x80) p--; 
            fullText = fullText.substring(0, p) + fullText.substring(cursorPos); 
            cursorPos = p;
        }
    } 
    needUpdate = true; 
    statusBarNeedsUpdate = true; 
}

void setupUSB() {
  Serial.println("USB Serial Ready...");
}

void handleUSB() {
  if (Serial.available()) {
    String incoming = Serial.readStringUntil('\n');
    Serial.print("Received via USB: ");
    Serial.println(incoming);
  }
}

#include <BleKeyboard.h>

BleKeyboard bleKeyboard("Rupertwriter", "Google", 100);

void setupBLE() {
  bleKeyboard.setName("Rupertwriter");
  bleKeyboard.begin();
  Serial.println("[BLE] Rupertwriter is now discoverable.");
}

void sendToBluetooth(String text) {
  if (bleKeyboard.isConnected()) {
    bleKeyboard.print(text); // 기기에서 쓴 글을 연결된 폰으로 전송
  }
}

void sendFullTextViaBLE() {
    if (!bleKeyboard.isConnected()) return;
    
    delay(2000); 
    
    for(int i = 0; i < fullText.length(); i++) {
        char c = fullText[i];
        
        if (c == '\n') {
            bleKeyboard.write(KEY_RETURN); // 줄바꿈은 순수 엔터 신호로만 보냄
        } 
        else if (c == '/' || c == '\\' || c == '-' || c == '=') {
            // 시스템 단축키로 오해받기 쉬운 기호들은 딜레이를 더 주어 '글자'로 인식하게 함
            bleKeyboard.print(c);
            delay(30); 
        }
        else {
            bleKeyboard.print(c);
        }
        delay(20); 
    }
    
    Serial.println("Safe BLE Transfer Complete!");
}

void insertText(String str) { 
    if (currentMode == SEARCH_MODE) {
        searchQuery += str; // 검색 모드일 때는 검색어 주머니로
    } else {
        fullText = fullText.substring(0, cursorPos) + str + fullText.substring(cursorPos); 
        cursorPos += str.length();
    }
}

void flushKorean() { 
  String s = "";
  if (cho != -1 && jung != -1) s = makeKorStr(cho, jung, jong);
  else if (cho != -1) s = String(choStrs[cho]);
  else if (jung != -1) s = String(jungStrs[jung]);
  if (s != "") { insertText(s); }
  cho = -1; jung = -1; jong = -1; lastJongChar = 0; 
}

void preloadInitialImage() {
  SdFile file; if (file.open("initial.png", O_RDONLY)) { imgSize = file.fileSize(); if (imgBuffer) free(imgBuffer); imgBuffer = (uint8_t *)malloc(imgSize); if (imgBuffer) file.read(imgBuffer, imgSize); file.close(); }
}

void showInitialImage() { 
    flushKorean(); 
    saveFile(); // 잠들기 전 강제 저장 (데이터 보호의 핵심!)
    // showInitialImage() 함수 안에서 잠들기 직전에 추가
    prefs.begin("zero", false);
    prefs.putBool("kor", isKoreanMode);
    prefs.putBool("caps", isCapsLockOn);
    // 커서 위치 강제 고정 (맨 아래)
    prefs.putInt("cursor", fullText.length()); 
    prefs.end();
    saveSystemSettings();
    display.clearDisplay(); 

    // 1. 잠들기 전 배터리 체크 (저전력 상태면 아예 여기서 끝냄)
    float batV = display.readBattery();
    if (batV < 3.5 && batV < 2.0) { 
        u8g2_for_adafruit_gfx.setFont(Typewriter_16px);
        printCleanText(u8g2_for_adafruit_gfx, "Low Battery! Please Charge.", MARGIN_X, 300);
        display.display(); // partial 대신 전체 출력으로 확실히 박제
        delay(2000);
        esp_deep_sleep_start(); 
    }   

    // 2. 화면 구성 (그림 또는 영문 메시지)
    if (imgBuffer) {
        int16_t x = (display.width() - 800) / 2;
        int16_t y = (display.height() - 600) / 2;
        if (x < 0) x = 0; if (y < 0) y = 0;
        display.image.drawPngFromBuffer(imgBuffer, imgSize, x, y, true, false); 
    } else {
        u8g2_for_adafruit_gfx.setFont(Typewriter_16px);
        printCleanText(u8g2_for_adafruit_gfx, "SYSTEM SLEEPING", MARGIN_X, 150);
        printCleanText(u8g2_for_adafruit_gfx, "Press any key to wake up", MARGIN_X, 180);
    }
    display.display(); // 화면 최종 출력
    delay(1000); 

    // 3. [수정] 딥슬립 설정 및 실행
    // 딥슬립은 라이트슬립과 달리 ext0 wakeup 설정을 써야 확실히 깨어남
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_3, 0); // 3번 핀(키보드)이 LOW가 되면 깨어남
    
    // 이제 라이트슬립은 지우고 딥슬립으로 모든 메모리를 리셋
    esp_deep_sleep_start(); 

    // ---------------------------------------------------------
    // 아래 로직은 딥슬립에서 깨어날 때 실행되지 않음 (setup으로 가기 때문)
    // 하지만 만약의 상황(딥슬립 실패 등)을 대비해 배터리 체크만 남겨둠
    if (display.readBattery() < 3.5) {
        esp_deep_sleep_start();
    }
}

void hardRefresh() {
    display.partialUpdate(false); 
    display.clearDisplay(); display.display();
    display.partialUpdate(true);
    display.resetInternalCounter(); 
    needUpdate = true; statusBarNeedsUpdate = true; lastSy = -1;
}

int viewBottomIdx = 0; // 화면 맨 아랫줄에 고정될 글자 위치

void adjustViewBottom() {
    if (viewBottomIdx > fullText.length()) viewBottomIdx = fullText.length();

    // 1. 커서가 화면 바닥보다 밑으로 내려가면, 바닥 앵커를 커서 위치로
    if (cursorPos > viewBottomIdx) {
        viewBottomIdx = cursorPos;
    }

    // 2. 화면 줄 수 계산
    int dynamicVisibleLines = ((display.height() / displayScale) - (MARGIN_Y * 1.5)) / (baseFontSize + lineSpacing);
    float avgCharWidth = baseFontSize * 0.8; 
    int dynamicCharsPerLine = ((display.width() / displayScale) - MARGIN_X - RIGHT_EDGE_MARGIN) / avgCharWidth;

    // 3. 바닥에서부터 거꾸로 올라가며 커서까지 몇 줄인지 카운트
    int lineCount = 0;
    int tempIdx = viewBottomIdx;
    int chars = 0;
    
    while (tempIdx > cursorPos) {
        tempIdx--;
        chars++;
        if (fullText[tempIdx] == '\n' || chars >= dynamicCharsPerLine) {
            lineCount++;
            chars = 0;
        }
    }

    // 4. 커서가 화면 위로 도망갈 때
    if (lineCount >= dynamicVisibleLines - 1) {
        // 커서가 화면 꼭대기에 보이도록, 커서부터 아래로 화면 줄 수만큼 이동
        int forwardLines = 0;
        int fwdTemp = cursorPos;
        int fwdChars = 0;
        while (fwdTemp < fullText.length() && forwardLines < dynamicVisibleLines - 1) {
            if (fullText[fwdTemp] == '\n' || fwdChars >= dynamicCharsPerLine) {
                forwardLines++;
                fwdChars = 0;
            }
            fwdTemp++;
            fwdChars++;
        }
        viewBottomIdx = fwdTemp; // 새로운 바닥 앵커 확정
    }
}

void printDualFont(String text, int x, int y, bool isMenu = false) {
  int cx = x;
  int applyLetterSp = isMenu ? 0 : letterSpacing; 
  u8g2_for_adafruit_gfx.setFontMode(0); 
  int currentFontState = 0;

  for (int i = 0; i < text.length(); ) {
    int l = 1;
    unsigned char c_val = (unsigned char)text[i];
    if (c_val >= 0x80) {
        if ((c_val & 0xE0) == 0xC0) l = 2;       // 2바이트 (유럽어 특수문자)
        else if ((c_val & 0xF0) == 0xE0) l = 3;  // 3바이트 (한글)
        else l = 4;
    }
    String c = text.substring(i, i + l);
    
    // --- [정밀 서체 분기 프로토콜] ---
    if (l == 3) {
        // 정확히 3바이트를 차지하는 순수 한글 영역 -> 사커스텀 폰트 강제 밀착
        //u8g2_for_adafruit_gfx.setFont(Typewriter_16px);
        if (currentFontState != 1) { 
                // 이미 한글 폰트 상태라면 이 무거운 명령을 건너뜀(최적화)
                u8g2_for_adafruit_gfx.setFont(Typewriter_16px);
                currentFontState = 1;
            }
        u8g2_for_adafruit_gfx.drawUTF8(cx, y, c.c_str());
        cx += 16 + applyLetterSp; // 한글 고정폭 16 유지
    } else {
        // 1, 2, 4바이트 (영어, 숫자, 기호 및 유럽/터키 특수문자) -> 내장 regular 폰트 적용
        //u8g2_for_adafruit_gfx.setFont(u8g2_font_unifont_te); 
        if (currentFontState != 2) { 
                // 이미 영문 폰트 상태라면 이 무거운 명령을 건너뜀(최적화)
                u8g2_for_adafruit_gfx.setFont(u8g2_font_unifont_te); 
                currentFontState = 2;
            }
        u8g2_for_adafruit_gfx.drawUTF8(cx, y, c.c_str());
        cx += u8g2_for_adafruit_gfx.getUTF8Width(c.c_str()) + applyLetterSp + (isMenu ? 1 : 0);
    }
    i += l;
  }
}

void printMenuEntry(String text, int x, int y, bool isSelected, bool isRightSide) {
  int drawX = (int)(x * displayScale);
  int drawY = (int)(y * displayScale);
  int boxW = isRightSide ? (int)(540 * displayScale) : (int)(180 * displayScale);
  int boxH = (int)(20 * displayScale);

  if (isSelected) {
    // 선택된 항목에만 검은 박스
    display.fillRect(drawX - 4, drawY - (int)(14 * displayScale), boxW, boxH, BLACK);
    u8g2_for_adafruit_gfx.setForegroundColor(WHITE); 
    u8g2_for_adafruit_gfx.setBackgroundColor(BLACK);
  } else {
    u8g2_for_adafruit_gfx.setForegroundColor(BLACK); 
    u8g2_for_adafruit_gfx.setBackgroundColor(WHITE);
  }
  
  // 밖에서 조립되어 들어온 "새 문서 <2.0>"를 한 번에 출력
  printDualFont(text, x, y, true); 
}

void refreshFileList() {
  fileCount = 0;
  SdFile root;
  if (!root.open("/")) return;
  SdFile file;
  
  while (file.openNext(&root, O_RDONLY)) {
    if (!file.isDir() && !file.isHidden()) {
      char name[64];
      file.getName(name, sizeof(name));
      String fn = String(name);
      
      String fnLower = fn; 
      fnLower.toLowerCase();
      
      if (fnLower.endsWith(".txt") && !fn.startsWith(".") && !fn.startsWith("._")) {
        fileCount++;
        files[fileCount].name = fn;
        
        char buf[33];
        int n = file.read(buf, 32);
        if (n > 0) buf[n] = '\0'; else buf[0] = '\0';
        String pv = String(buf);
        pv.replace("\n", " ");
        files[fileCount].preview = pv;
      }
    }
    file.close();
    if (fileCount >= 64) break; 
  }
  root.close();
  for (int i = 1; i < fileCount; i++) {
    for (int j = i + 1; j <= fileCount; j++) {
      
      String sI = ""; String sJ = "";
      for(int k=0; k<files[i].name.length(); k++) { if(isDigit(files[i].name[k])) sI += files[i].name[k]; }
      for(int k=0; k<files[j].name.length(); k++) { if(isDigit(files[j].name[k])) sJ += files[j].name[k]; }
      
      int numI = sI.toInt();
      int numJ = sJ.toInt();

      if (numI < numJ) {
        FileInfo temp = files[i];
        files[i] = files[j];
        files[j] = temp;
      }
    }
  }
}

void saveFile() { 
    SdFile f; 
    if (f.open(currentFileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC)) { 
        f.write(fullText.c_str(), fullText.length()); 
        f.sync(); f.close(); 
    } 
    showSavedMessageTime = millis(); needUpdate = true; statusBarNeedsUpdate = true; refreshFileList(); 
}


void moveCursorToLineStart() {//단어 단위로 앞으로 이동(변경)
  // 커서 이동 전, 조합 중인 한글이 있다면 본문에 확정(Flush)
      if (cho != -1 || jung != -1) {
          String composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
          
          // 조립된 글자를 본문(fullText)의 현재 커서 위치로
          fullText = fullText.substring(0, cursorPos) + composing + fullText.substring(cursorPos);
          cursorPos += composing.length(); // 글자가 확정된 길이만큼 커서를 뒤로 밀어줌
          
          // 확정 완료 후 한글 엔진 초기화
          cho = -1; 
          jung = -1; 
          jong = -1;
      }
          flushKorean(); 
          int p = cursorPos - 1;
          
          // 1. 현재 커서 앞의 공백이나 줄바꿈을 먼저 건너뜀
          while (p > 0 && (fullText[p] == ' ' || fullText[p] == '\n')) p--;
          
          // 2. 글자들을 지나치다가 다시 공백이나 줄바꿈을 만나면 그 직후가 단어의 시작
          while (p > 0 && fullText[p-1] != ' ' && fullText[p-1] != '\n') p--;
          
          cursorPos = (p < 0) ? 0 : p;
          needUpdate = true;    
}

void moveCursorToLineEnd() {//단어 단위로 뒤로 이동(변경)
  // 커서 이동 전, 조합 중인 한글이 있다면 본문에 확정(Flush)
      if (cho != -1 || jung != -1) {
          String composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
          
          // 조립된 글자를 본문(fullText)의 현재 커서 위치로
          fullText = fullText.substring(0, cursorPos) + composing + fullText.substring(cursorPos);
          cursorPos += composing.length(); // 글자가 확정된 길이만큼 커서를 뒤로 밀어줌
          
          // 확정 완료 후 한글 엔진 초기화
          cho = -1; 
          jung = -1; 
          jong = -1;
      }
    // 현재 위치에서 앞으로 가며 줄바꿈 문자 찾기
      flushKorean(); 
      int n = cursorPos;
      
      // 1. 현재 커서 위치의 공백이나 줄바꿈을 먼저 건너뜀
      while (n < fullText.length() && (fullText[n] == ' ' || fullText[n] == '\n')) n++;
      
      // 2. 글자들을 지나치다가 다음 공백이나 줄바꿈을 만나면 그곳이 단어의 끝
      while (n < fullText.length() && fullText[n] != ' ' && fullText[n] != '\n') n++;
      
      cursorPos = n;
      needUpdate = true;
}

void moveCursorToParagraphStart() {//글 맨 앞으로 이동(변경)
  // 커서 이동 전, 조합 중인 한글이 있다면 본문에 확정(Flush)
      if (cho != -1 || jung != -1) {
          String composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
          
          // 조립된 글자를 본문(fullText)의 현재 커서 위치로
          fullText = fullText.substring(0, cursorPos) + composing + fullText.substring(cursorPos);
          cursorPos += composing.length(); // 글자가 확정된 길이만큼 커서를 뒤로 밀어줌
          
          // 확정 완료 후 한글 엔진 초기화
          cho = -1; 
          jung = -1; 
          jong = -1;
      }
    // 글의 시작까지 위로 점프
    cursorPos = 0;
    
    needUpdate = true;
}

void moveCursorToParagraphEnd() {//글 맨 뒤로 이동(변경)
  // 커서 이동 전, 조합 중인 한글이 있다면 본문에 확정(Flush)
      if (cho != -1 || jung != -1) {
          String composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
          
          // 조립된 글자를 본문(fullText)의 현재 커서 위치로
          fullText = fullText.substring(0, cursorPos) + composing + fullText.substring(cursorPos);
          cursorPos += composing.length(); // 글자가 확정된 길이만큼 커서를 뒤로 밀어줌
          
          // 확정 완료 후 한글 엔진 초기화
          cho = -1; 
          jung = -1; 
          jong = -1;
      }
    // 글의 끝까지 아래로 점프
    cursorPos = fullText.length();
    needUpdate = true;
}

void selectLeft() { 
    // 커서 이동 전, 조합 중인 한글이 있다면 본문에 확정(Flush)
      if (cho != -1 || jung != -1) {
          String composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
          
          // 조립된 글자를 본문(fullText)의 현재 커서 위치로
          fullText = fullText.substring(0, cursorPos) + composing + fullText.substring(cursorPos);
          cursorPos += composing.length(); // 글자가 확정된 길이만큼 커서를 뒤로 밀어줌
          
          // 확정 완료 후 한글 엔진 초기화
          cho = -1; 
          jung = -1; 
          jong = -1;
      }
      if (cursorPos > 0) cursorPos--; 
      needUpdate = true; 
  }

void selectRight() { 
    // 커서 이동 전, 조합 중인 한글이 있다면 본문에 확정(Flush)
      if (cho != -1 || jung != -1) {
          String composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
          
          // 조립된 글자를 본문(fullText)의 현재 커서 위치로
          fullText = fullText.substring(0, cursorPos) + composing + fullText.substring(cursorPos);
          cursorPos += composing.length(); // 글자가 확정된 길이만큼 커서를 뒤로 밀어줌
          
          // 확정 완료 후 한글 엔진 초기화
          cho = -1; 
          jung = -1; 
          jong = -1;
      }
      if (cursorPos < fullText.length()) cursorPos++; 
      needUpdate = true; 
  }
void selectUp() { 
    // 위쪽 줄의 같은 위치로 계산
    moveCursorUp(); 
}
void selectDown() { 
    // 아래쪽 줄의 같은 위치로 계산
    moveCursorDown(); 
}

void moveCursorUp() {
    // 커서 이동 전, 조합 중인 한글이 있다면 본문에 확정(Flush)
      if (cho != -1 || jung != -1) {
          String composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
          
          // 조립된 글자를 본문(fullText)의 현재 커서 위치로
          fullText = fullText.substring(0, cursorPos) + composing + fullText.substring(cursorPos);
          cursorPos += composing.length(); // 글자가 확정된 길이만큼 커서를 뒤로 밀어줌
          
          // 확정 완료 후 한글 엔진 초기화
          cho = -1; 
          jung = -1; 
          jong = -1;
      }
    // 1. 현재 줄의 시작 위치
    int lineStart = cursorPos;
    while (lineStart > 0 && fullText[lineStart - 1] != '\n') {
        lineStart--;
    }
    if (lineStart == 0) return; // 첫 줄

    // 2. 현재 줄에서 커서가 몇 번째 칸에 있는지 계산
    int column = cursorPos - lineStart;

    // 3. 윗줄의 시작 위치
    int prevLineStart = lineStart - 1;
    while (prevLineStart > 0 && fullText[prevLineStart - 1] != '\n') {
        prevLineStart--;
    }

    // 4. 윗줄의 길이와 현재 칸(column)을 비교
    int prevLineLength = lineStart - 1 - prevLineStart;
    if (column > prevLineLength) {
        cursorPos = lineStart - 1; // 윗줄이 더 짧으면 윗줄 맨 끝으로
    } else {
        cursorPos = prevLineStart + column; // 아니면 같은 칸으로
    }
    needUpdate = true;
}

void moveCursorDown() {
    // 커서 이동 전, 조합 중인 한글이 있다면 본문에 확정(Flush)
      if (cho != -1 || jung != -1) {
          String composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
          
          // 조립된 글자를 본문(fullText)의 현재 커서 위치로
          fullText = fullText.substring(0, cursorPos) + composing + fullText.substring(cursorPos);
          cursorPos += composing.length(); // 글자가 확정된 길이만큼 커서를 뒤로 밀어줌
          
          // 확정 완료 후 한글 엔진 초기화
          cho = -1; 
          jung = -1; 
          jong = -1;
      }
    // 1. 현재 줄의 시작 위치
    int lineStart = cursorPos;
    while (lineStart > 0 && fullText[lineStart - 1] != '\n') {
        lineStart--;
    }
    int column = cursorPos - lineStart;

    // 2. 다음 줄의 시작 위치
    int nextLineStart = cursorPos;
    while (nextLineStart < fullText.length() && fullText[nextLineStart] != '\n') {
        nextLineStart++;
    }
    if (nextLineStart >= fullText.length()) return; // 마지막 줄이면 끝냄
    nextLineStart++; // '\n' 다음 글자로 이동

    // 3. 다음 줄의 끝 위치
    int nextLineEnd = nextLineStart;
    while (nextLineEnd < fullText.length() && fullText[nextLineEnd] != '\n') {
        nextLineEnd++;
    }

    // 4. 다음 줄의 길이와 현재 칸을 비교해서 이동
    int nextLineLength = nextLineEnd - nextLineStart;
    if (column > nextLineLength) {
        cursorPos = nextLineEnd;
    } else {
        cursorPos = nextLineStart + column;
    }
    needUpdate = true;
}

int getWordCount(const String& text) {
    int wordCount = 0;
    bool inWord = false;
    
    for (int i = 0; i < text.length(); i++) {
        char c = text[i];
        
        // 공백, 탭, 줄바꿈 문자를 만나면 단어가 끝난 것으로 인식
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            inWord = false;
        } 
        // 일반 문자를 만나면 단어의 시작으로 인식
        else if (!inWord) {
            inWord = true;
            wordCount++;
        }
    }
    return wordCount;
}

void loadFile() { 
    SdFile f; 
    if (f.open(currentFileName.c_str(), O_RDONLY)) { 
        fullText = ""; while(f.available()) fullText += (char)f.read(); f.close(); 
        cursorPos = fullText.length(); 
    } 
}

void createNewDoc() { 
    int n = 1; SdFile temp; 
    while (temp.open(("doc_" + String(n) + ".txt").c_str(), O_RDONLY)) { temp.close(); n++; } 
    currentFileName = "doc_" + String(n) + ".txt"; fullText = ""; cursorPos = 0; 
    saveFile(); currentMode = TYPING_MODE; needUpdate = true; 
}

void CalculationTask(void * pvParameters) {
    for(;;) {
        if (needCountUpdate) {
            // 1. 글자 수 계산 (바이트가 아니라 실제 글자 수로 정확히 계산)
            sharedCharCount = getTrueLength(calcBuffer); 
            
            // 2. 단어 수 계산 (공백 기준)
            int words = 0;
            bool inWord = false;
            for (int i = 0; i < calcBuffer.length(); i++) {
                if (isspace(calcBuffer[i])) inWord = false;
                else if (!inWord) { inWord = true; words++; }
            }
            sharedWordCount = words;
            
            needCountUpdate = false; // 계산 완료
        } // if문 닫기
        
        vTaskDelay(50); // 0.1초마다 체크 (CPU 부하 감소)
    } // for문 닫기
} // CalculationTask 함수 닫기

void setup() {//부팅
    setCpuFrequencyMhz(240);
  Serial.begin(921600); display.begin(); 
  if ((display.readBattery() < 3.5) && (display.readBattery() < 2.0)) {
    display.clearDisplay();
    display.display(); 
    esp_deep_sleep_start(); 
    }
    loadSystemSettings();
    display.setRotation(0); u8g2_for_adafruit_gfx.begin(bigDisplay);
    // "Booting..." 메시지
    display.clearDisplay();
    printCleanText(u8g2_for_adafruit_gfx, "Booting...", MARGIN_X, MARGIN_Y);
    //display.clearDisplay();
    display.partialUpdate(); // 기기가 살아있음을 즉시 보고
    if (display.sdCardInit()) { preloadInitialImage(); refreshFileList(); if (fileCount > 0) {
        currentFileName = files[1].name;
    } loadFile(); }
    setupUSB();
    delay(1000); // 기계가 안정될 때까지 1초 대기
    while(Serial.available() > 0) Serial.read();  // 메인 시리얼 비우기
    while(Serial1.available() > 0) Serial1.read(); // 키보드 연결 시리얼 비우기

    Serial.println("Boot garbage cleared.");
    // 파일을 로드한 후 실행

    // 1. "zero" 서랍 열기 (true는 읽기 전용 모드)
    prefs.begin("zero", true); 
    
    // 2. 내용물 꺼내서 변수에 담기
    isKoreanMode = prefs.getBool("kor", false); // 저장된 게 없으면 기본값 false
    isCapsLockOn = prefs.getBool("caps", false);
    prefs.end();
    // 3. 커서는 무조건 맨 아래에 다시 배치
    cursorPos = fullText.length(); 
    needUpdate = true;

    xTaskCreatePinnedToCore(
        CalculationTask, "CalcTask", 4000, NULL, 1, &CalcTaskHandle, 0
    );
}

void saveSystemSettings() {
    prefs.begin("rupert", false); // "rupert"라는 이름의 저장소 열기
    prefs.putBool("isKor", isKoreanMode);
    prefs.putBool("isCaps", isCapsLockOn); // 캡스락 상태 (변수명 확인 필요)
    prefs.putInt("fIndex", rightFileIndex); // 현재 열린 파일 인덱스
    prefs.putInt("latin", latinMode);
    
    prefs.end();
    Serial.println("System settings saved to NVS.");
}

void loadSystemSettings() {
    prefs.begin("rupert", true); // 읽기 전용으로 열기
    isKoreanMode = prefs.getBool("isKor", false); // 없으면 기본값 false(영어)
    isCapsLockOn = prefs.getBool("isCaps", false);
    rightFileIndex = prefs.getInt("fIndex", 0);
    latinMode = prefs.getInt("latin", 0);
    prefs.end();
    Serial.println("System settings restored.");
}
// --- [업데이트 1] 펌웨어 업로드 웹페이지 서빙 ---
void handleUpdateForm() {
    String html = "<html><head><meta charset='UTF-8'><title>RupertWriter Update</title></head>"
                  "<body style='font-family:sans-serif; text-align:center; margin-top:50px;'>";
    
    if (isKoreanMode) {
        html += "<h2>펌웨어 무선 업데이트 (OTA)</h2>"
                "<p style='color:#555;'>컴파일된 .bin 파일을 선택한 후 아래 버튼을 눌러주세요.</p>"
                "<form method='POST' action='/update' enctype='multipart/form-data'>"
                "<input type='file' name='update' accept='.bin' required><br><br>"
                "<input type='submit' value='업데이트 시작' style='padding:10px 20px; font-size:16px; cursor:pointer;'>"
                "</form>";
    } else {
        html += "<h2>Firmware OTA Update</h2>"
                "<p style='color:#555;'>Select the compiled .bin file and click the button below.</p>"
                "<form method='POST' action='/update' enctype='multipart/form-data'>"
                "<input type='file' name='update' accept='.bin' required><br><br>"
                "<input type='submit' value='Start Update' style='padding:10px 20px; font-size:16px; cursor:pointer;'>"
                "</form>";
    }
    html += "</body></html>";
    server.send(200, "text/html", html);
}

// --- [업데이트 2] 전송된 bin 파일 덮어쓰기 로직 ---
void handleUpdateUpload() {
    HTTPUpload& upload = server.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        // 업데이트 시작 시 플래그와 타이머 설정
        isUpdating = true; 
        lastActivityTime = millis(); 
        
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.println("Update Success");
        } else {
            Update.printError(Serial);
        }
        // 업데이트 완료 후 플래그 해제
        isUpdating = false;
    }
}

void setupWiFi() {
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_AP); // AP 모드로 확실히 고정
    delay(200);

    IPAddress local_IP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_IP, gateway, subnet);

    if (WiFi.softAP(ssid, password)) {
        Serial.println("SoftAP Ready: 192.168.4.1");
    }

    if (MDNS.begin("rupertwriter")) {
        MDNS.addService("http", "tcp", 80); 
        Serial.println("mDNS responder started: rupertwriter.local");
    }
    WiFi.setTxPower(WIFI_POWER_15dBm);
    server.on("/", handleRoot);
    server.on("/download", handleDownload);
    server.on("/delete", handleDelete);
    
    // ---  업데이트 전용 페이지 맵핑 ---
    server.on("/update", HTTP_GET, handleUpdateForm);
    server.on("/update", HTTP_POST, []() {
        if (Update.hasError()) {
            server.send(200, "text/plain; charset=utf-8", isKoreanMode ? "업데이트 실패!" : "Update Failed!");
        } else {
            server.send(200, "text/plain; charset=utf-8", isKoreanMode ? "업데이트 성공! 재부팅 중..." : "Update Success! Rebooting...");
            delay(1000);
            ESP.restart(); 
        }
    }, handleUpdateUpload);
    
    server.begin();
} // setupWiFi() 끝

String getAccentChar(char base, int mode, int cycle) {
    int idx = cycle - 1;
    if (mode == 1) { // 1: 대통합 EU 모드
        if (base == 'a') { String arr[] = {"á", "à", "â", "ä", "ã", "æ", "a"}; return arr[idx % 7]; }
        if (base == 'A') { String arr[] = {"Á", "À", "Â", "Ä", "Ã", "Æ", "A"}; return arr[idx % 7]; }
        if (base == 'e') { String arr[] = {"é", "è", "ê", "ë", "e"}; return arr[idx % 5]; }
        if (base == 'E') { String arr[] = {"É", "È", "Ê", "Ë", "E"}; return arr[idx % 5]; }
        if (base == 'i') { String arr[] = {"í", "ì", "î", "ï", "ı", "i"}; return arr[idx % 6]; } 
        if (base == 'I') { String arr[] = {"Í", "Ì", "Î", "Ï", "İ", "I"}; return arr[idx % 6]; } 
        if (base == 'o') { String arr[] = {"ó", "ò", "ô", "ö", "õ", "œ", "o"}; return arr[idx % 7]; }
        if (base == 'O') { String arr[] = {"Ó", "Ò", "Ô", "Ö", "Õ", "Œ", "O"}; return arr[idx % 7]; }
        if (base == 'u') { String arr[] = {"ú", "ù", "û", "ü", "u"}; return arr[idx % 5]; }
        if (base == 'U') { String arr[] = {"Ú", "Ù", "Û", "Ü", "U"}; return arr[idx % 5]; }
        if (base == 'c') { String arr[] = {"ç", "ć", "č", "c"}; return arr[idx % 4]; } 
        if (base == 'C') { String arr[] = {"Ç", "Ć", "Č", "C"}; return arr[idx % 4]; } 
        if (base == 'n') { String arr[] = {"ñ", "n"}; return arr[idx % 2]; }
        if (base == 'N') { String arr[] = {"Ñ", "N"}; return arr[idx % 2]; }
        if (base == 's') { String arr[] = {"ß", "š", "ś", "ş", "s"}; return arr[idx % 5]; } 
        if (base == 'S') { String arr[] = {"Š", "Ś", "Ş", "S"}; return arr[idx % 4]; } 
        if (base == 'd') { String arr[] = {"đ", "d"}; return arr[idx % 2]; } 
        if (base == 'D') { String arr[] = {"Đ", "D"}; return arr[idx % 2]; } 
        if (base == 'z') { String arr[] = {"ž", "ź", "z"}; return arr[idx % 3]; } 
        if (base == 'Z') { String arr[] = {"Ž", "Ź", "Z"}; return arr[idx % 3]; } 
        if (base == 'g') { String arr[] = {"ğ", "g"}; return arr[idx % 2]; } 
        if (base == 'G') { String arr[] = {"Ğ", "G"}; return arr[idx % 2]; } 
    }
    return ""; 
}

String getLocalLayoutChar(char us, bool isAlt, int mode) {
    if (mode == 2) { // 2: DE (QWERTZ)
        if (isAlt) { // AltGr 매핑
            switch(us) { case 'q': case 'Q': return "@";
            case 'e': case 'E': return "€"; case 'm': case 'M': return "µ"; case '8': return "["; case '9': return "]";
            case '7': return "{"; case '0': return "}"; case '-': return "\\"; case ']': return "~";
            }
        } else {
            switch(us) { 
            case '`': return "^"; case '~': return "°"; 
            case 'z': return "y";
            case 'Z': return "Y"; case 'y': return "z"; case 'Y': return "Z"; case '-': return "ß"; case '_': return "?";
            case '=': return "´"; case '+': return "`"; case '[': return "ü"; case '{': return "Ü"; case ']': return "+";
            case '}': return "*"; case ';': return "ö"; case ':': return "Ö"; case '\'': return "ä";
            case '"': return "Ä"; case '\\': return "#"; case '|': return "'"; case '/': return "-"; case '?': return "_"; }
        }
    }
    else if (mode == 3) { // 3: MN (Montenegrin QWERTZ)
        if (isAlt) {
            switch(us) { case 'v': case 'V': return "@"; case 'e': case 'E': return "€"; case 'f': case 'F': return "["; case 'g': case 'G': return "]"; case 'b': case 'B': return "{"; 
            case 'n': case 'N': return "}"; case 'q': case 'Q': return "\\"; }
        } else {
            switch(us) { case 'z': return "y"; case 'Z': return "Y"; case 'y': return "z"; case 'Y': return "Z"; case '[': return "š"; case '{': return "Š"; case ']': return "đ"; case '}': return "Đ"; case ';': return "č"; case ':': return "Č"; case '\'': return "ć"; case '"': return "Ć";
            case '\\': return "ž"; case '|': return "Ž"; case '/': return "-"; case '?': return "_"; case '=': return "+";
            case '+': return "*"; case '-': return "'"; case '_': return "?";
            }
        }
    }
    else if (mode == 4) { // 4: TR (Turkish Q)
        if (isAlt) {
            switch(us) { case 'q': case 'Q': return "@";
            case 'e': case 'E': return "€"; case 't': case 'T': return "₺"; case '8': return "["; case '9': return "]";
            case '7': return "{"; case '0': return "}"; case '-': return "\\"; case '=': return "~";
            }
        } else {
            switch(us) { 
            case '`': return "\""; case '~': return "é"; 
            case '[': return "ğ";
            case '{': return "Ğ"; case ']': return "ü"; case '}': return "Ü"; case ';': return "ş"; case ':': return "Ş";
            case '\'': return "i"; case '"': return "İ"; case 'i': return "ı"; case 'I': return "I"; case ',': return "ö"; case '<': return "Ö"; case '.': return "ç"; case '>': return "Ç"; case '/': return "."; case '?': return ":"; }
        }
    }
    else if (mode == 5) { // 5: FR (AZERTY)
        if (isAlt) {
            switch(us) { case 'e': case 'E': return "€"; case '0': return "@"; case '5': return "["; case '_': return "]"; case '4': return "{"; case '+': return "}"; case '8': return "\\"; case '2': return "~"; }
        } else {
            switch(us) { 
            case '`': case '~': return "²"; // 프랑스어 ² 예외
            case 'q': return "a"; case 'Q': return "A"; case 'a': return "q"; case 'A': return "Q"; case 'w': return "z"; case 'W': return "Z"; case 'z': return "w"; case 'Z': return "W"; case 'm': return ","; case 'M': return "?"; case ',': return ";"; case '<': return "."; case '.': return ":"; case '>': return "/"; case '/': return "!"; case '?': return "§"; case ';': return "m"; case ':': return "M"; case '\'': return "ù"; case '"': return "%";
            case '[': return "^"; case '{': return "¨"; case ']': return "$"; case '}': return "£"; case '-': return ")";
            case '_': return "°"; case '=': return "="; case '+': return "+"; case '\\': return "*"; case '|': return "µ";
            case '1': return "&"; case '!': return "1"; case '2': return "é"; case '@': return "2";
            case '3': return "\""; case '#': return "3"; case '4': return "'"; case '$': return "4"; case '5': return "("; case '%': return "5"; case '6': return "-"; case '^': return "6"; case '7': return "è"; case '&': return "7"; case '8': return "_"; case '*': return "8"; case '9': return "ç"; case '(': return "9"; case '0': return "à"; case ')': return "0"; }
        }
    }
    return isAlt ? "" : String(us); // 알트 누른 상태에서 매핑 없는 키는 오작동 방지를 위해 무시
}


void loop() {
  if (currentNetSubMode == NET_WIFI) {
      server.handleClient(); // 폰의 접속 요청을 계속 대기함
  }
  // --- 입력부: 키를 읽고 데이터 전송 ---
  while (Serial.available() > 0) {
    byte k = Serial.read(); 
    char real = 0;
    lastKeyPress = millis(); 
    needUpdate = true; 
    statusBarNeedsUpdate = true;
    // 특수키 처리
    if (k == 240) { isShiftPressed = true; continue; } 
    if (k == 241) { isShiftPressed = false; continue; } 
    if (k == 242) { isCtrlPressed = true; continue; } 
    if (k == 243) { isCtrlPressed = false; continue; }

    if (k == 244) { 
        isAltPressed = true;
        // 🚀 EU(1번) 모드일 때만 알트 순환 방식 작동
        if (latinMode == 1 && (millis() - lastTypingTime <= 3000)) {
            accentCycleIdx++;
            String newChar = getAccentChar(lastBaseChar, latinMode, accentCycleIdx);
            if (newChar != "") {
                doBackspace(); // 방금 찍혔던 원본 알파벳(또는 이전 엑센트)을 지움
                insertText(newChar); // 새 엑센트 글자 삽입
                lastTypingTime = millis(); // 0.5초 타이머 연장 (Alt를 또 누를 수 있게)
                needUpdate = true;
            }
        }
        continue; 
    } 
    if (k == 245) { isAltPressed = false; continue; }

    if (currentNetSubMode != NET_MAIN) {
        if (isCtrlPressed && k == 246) { // Ctrl + Menu 진입 시 꺼짐으로 복귀
            WiFi.softAPdisconnect(true); 
            WiFi.mode(WIFI_OFF); btStop();
            currentNetSubMode = NET_MAIN; 
            isFirmwareUpdateMode = false;
            needUpdate = true; 
            statusBarNeedsUpdate = true;
            continue;
        }
        
        if (k == 56 || k == 246) {
            if (leftMenuIndex == 9) {
                if (k == 56) {
            if (currentNetSubMode == NET_MAIN) currentNetSubMode = NET_WIFI;
            else if (currentNetSubMode == NET_WIFI) currentNetSubMode = NET_BT_SELECT;
            else currentNetSubMode = NET_MAIN;
            needUpdate = true;
        }
    }
        } else {
            continue; // 그 외 모든 키는 안내 화면 유지를 위해 무시
        }
    }
    
    if (k == 28) {
        isCapsLockOn = !isCapsLockOn;
        continue; // 0으로 매핑되어 글자가 입력되지 않도록 바로 넘김
    }
    const char engMap[] = { '`','1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\\',0,'a','s','d','f','g','h','j','k','l',';','\'','\n',0,'z','x','c','v','b','n','m',',','.','/',0,0,0,0,0,' ',0,0,0 }; 
    const char shiftMap[] = { '~','!','@','#','$','%','^','&','*','(',')','_','+','\b','\t','Q','W','E','R','T','Y','U','I','O','P','{','}','|',0,'A','S','D','F','G','H','J','K','L',':','\"','\n',0,'Z','X','C','V','B','N','M','<','>','?',0,0,0,0,0,' ',0,0,0 };
    
    
    if (k < sizeof(engMap)) {
        char base = engMap[k];
        // 알파벳 범위(a-z) 판단
        if (base >= 'a' && base <= 'z') {
            // CapsLock과 Shift 중 하나만 적용되었을 때 대문자 (XOR 논리)
            real = (isShiftPressed != isCapsLockOn) ? shiftMap[k] : engMap[k];
        } else {
            // 알파벳이 아닌 기호/숫자는 기존처럼 Shift로만 판단
            real = isShiftPressed ? shiftMap[k] : engMap[k];
        }
    }

        if (k == 56) real = ' '; 
        
    // 메뉴 진입 및 컨트롤키 로직
    if (isCtrlPressed) {
      if (real == 'l' || real == 'L') { flushKorean(); isCtrlPressed = false; showInitialImage(); continue; }
      if (real == 'c' || real == 'C') { flushKorean(); clipboard = fullText; continue; } // 전체 복사
      if (real == 'v' || real == 'V') { flushKorean(); insertText(clipboard); continue; } // 붙여넣기
      
      if (real == 's' || real == 'S') { flushKorean(); saveFile(); continue; }
      if (real == 'n' || real == 'N') { flushKorean(); createNewDoc(); continue; }
      if (real == 'r' || real == 'R') { flushKorean(); hardRefresh(); continue; }
      if (real == 'f' || real == 'F') { 
          flushKorean(); 
          if (currentMode == SEARCH_MODE) currentMode = TYPING_MODE;
          else { currentMode = SEARCH_MODE; searchQuery = ""; }
          needUpdate = true; continue; 
      }
      if (k == 57) { moveCursorToLineStart(); continue; } // Ctrl + Left : 앞 단어로 이동
      if (k == 60) { moveCursorToLineEnd(); continue; }   // Ctrl + Right : 뒷 단어로 이동
      if (k == 58) { moveCursorToParagraphStart(); continue; } // Ctrl + Up : 글 맨 앞으로 이동
      if (k == 59) { moveCursorToParagraphEnd(); continue; }   // Ctrl + Down : 글 맨 뒤로 이동
    }
    if (k == 246) { 
      if (currentMode == TYPING_MODE) { 
        flushKorean(); 
        currentMode = FILE_MENU_MODE; 
        tempNetCursor = currentNetSubMode;
        refreshFileList(); 
        rightFileIndex = 1;
        isDeletingFile = false; } 
      else currentMode = TYPING_MODE; 
      continue; 
    }
    if (currentNetSubMode != NET_MAIN) {
        int statusBarBottom = (int)(45 * displayScale);
        // 배율이 커지면 박스도 화면 가득 차게 유동적으로 설정
        int boxW = (display.width() / displayScale) * 0.8; // 화면의 80% 사용
        int boxH = 150; // 높이는 고정해도 줄바꿈으로 해결
        int boxX = ((display.width() / displayScale) - boxW) / 2;
        int boxY = 100; // 상단에서 적당히 띄움
        display.fillRect(0, statusBarBottom, display.width(), display.height() - statusBarBottom, WHITE);
        u8g2_for_adafruit_gfx.setFont(Typewriter_16px);
        // 기존 한 줄짜리 print를 두 부분으로 
        u8g2_for_adafruit_gfx.setCursor(boxX + 15, boxY + 40);
        u8g2_for_adafruit_gfx.print(isKoreanMode ? "네트워크 모드 : 192.168.4.1 또는 rupertwriter.local" : "Network Active : 192.168.4.1 or rupertwriter.local");

        // 다음 줄: y좌표에 폰트 크기만큼 더해서 출력 (박스 밖 탈출 방지)
        u8g2_for_adafruit_gfx.setCursor(boxX + 15, boxY + 40 + (baseFontSize + lineSpacing));
        u8g2_for_adafruit_gfx.print("Exit: [Ctrl + Menu]");
        int infoY = statusBarBottom + 40;
        
    } 
    
    else if (currentMode == FILE_MENU_MODE) {
      // [메뉴 조작 로직]
      if (isDeletingFile) { 
        if (real == '\n') { 
          SdFile root; if (root.open("/", O_RDONLY)) { 
            SdFile f; if (f.open(&root, files[rightFileIndex].name.c_str(), O_WRONLY)) { f.remove(); refreshFileList(); if (fileCount == 0) createNewDoc(); if (rightFileIndex > fileCount) rightFileIndex = fileCount; } 
            root.close(); 
          } 
          isDeletingFile = false; 
        } else if (real == '\b' || k == 58 || k == 59 || k == 57 || k == 60) isDeletingFile = false; 
        continue; 
      }
      if (isEditingValue) {
        // [1. 서브 시스템 설정 메뉴 조절]
        if (inSystemSubMenu) { 
            switch (leftMenuIndex) {
                case 1: // 글자크기
                    if (k == 57 && displayScale > 0.5) displayScale -= 0.1;
                    if (k == 60 && displayScale < 5.0) displayScale += 0.1;
                    break;
                case 2: // 줄간격
                    if (k == 57 && lineSpacing > 0) lineSpacing--;
                    if (k == 60 && lineSpacing < 30) lineSpacing++;
                    break;
                case 3: // 글자간격
                    if (k == 57 && letterSpacing > -5) letterSpacing--;
                    if (k == 60 && letterSpacing < 10) letterSpacing++;
                    break;
                case 4: // 속도
                    if (k == 57 && typingSpeed > 0) typingSpeed--;
                    if (k == 60 && typingSpeed < 10) typingSpeed++;
                    break;
                case 5: // 새로고침
                    if (k == 57 && refreshLimit > 10) refreshLimit -= 10;
                    if (k == 60 && refreshLimit < 5000) refreshLimit += 10;
                    break;
                case 6: // 라틴키보드 선택 중
                    if (k == 57 && latinMode > 0) { // 왼쪽 화살표
                        latinMode--;
                    }
                    if (k == 60 && latinMode < 5) { // 오른쪽 화살표
                        latinMode++;
                    }
                    break;
            }
            if (real == '\n') {
                if (leftMenuIndex == 6 && isEditingValue) {
                    // 이미 꺾쇠(< >)가 켜진 상태에서 엔터를 쳤을 때만 작동
                    isEditingValue = false;
                    currentMode = TYPING_MODE; // 타자 화면으로 복귀
                    //isKoreanMode = false;      // 이제 글쓰기는 영어(라틴)로 강제 고정
                    isCapsLockOn = false;      // 캡스락 해제
                    needUpdate = true;
                } else {
                    isEditingValue = !isEditingValue;
                }
            }
        } 
        // [2. 메인 메뉴 값 조절 (통일성)]
        else {
            switch (leftMenuIndex) {
                case 4: // 네트워크 설정 조절
                    if (k == 57) { // Left (역순환)
                        if (tempNetCursor == NET_MAIN) tempNetCursor = NET_BT_SELECT;
                        else if (tempNetCursor == NET_BT_SELECT) tempNetCursor = NET_WIFI;
                        else tempNetCursor = NET_MAIN;
                    } 
                    if (k == 60) { // Right (정순환)
                        if (tempNetCursor == NET_MAIN) tempNetCursor = NET_WIFI;
                        else if (tempNetCursor == NET_WIFI) tempNetCursor = NET_BT_SELECT;
                        else tempNetCursor = NET_MAIN;
                    }
                    if (real == '\n') { // 두 번째 엔터: 실제 기능 켜고 확정
                        isEditingValue = false;
                        currentNetSubMode = tempNetCursor;
                        
                        if (currentNetSubMode == NET_WIFI) { setupWiFi(); currentMode = TYPING_MODE; }
                        else if (currentNetSubMode == NET_BT_SELECT) { setupBLE(); currentMode = TYPING_MODE; }
                        else { WiFi.softAPdisconnect(true); WiFi.mode(WIFI_OFF); currentMode = TYPING_MODE; }
                    }
                    break;

                case 7: // 자동 잠자기 조절
                    if (k == 57) { // Left (시간 감소)
                        autoSleepIndex = (autoSleepIndex <= 0) ? 6 : autoSleepIndex - 1;
                    }
                    if (k == 60) { // Right (시간 증가)
                        autoSleepIndex = (autoSleepIndex + 1) % 7;
                    }
                    if (real == '\n'|| real == '\r') { // 두 번째 엔터: 저장 후 확정
                        isEditingValue = false;
                        saveSystemSettings();
                    }
                    break;
            }
        }
        
        needUpdate = true; // 화면 값 변동 반영
        continue; // 편집 중엔 메뉴 위아래 이동을 원천 차단
    }
    if (k == 57) { menuFocusSide = 0; needUpdate = true; } 
    if (k == 60) { menuFocusSide = 1; needUpdate = true; }

    if (menuFocusSide == 0) {
            int maxVisibleMenu = ((display.height() / displayScale) - 70) / 24; 
            if (maxVisibleMenu < 1) maxVisibleMenu = 1;

            if (k == 58 && leftMenuIndex > 0) { // 위로
                leftMenuIndex--; 
                if (leftMenuIndex < leftMenuOffset) leftMenuOffset = leftMenuIndex;
            } 
            if (k == 59 && leftMenuIndex < 7) { // 아래로
                leftMenuIndex++; 
                if (leftMenuIndex >= leftMenuOffset + maxVisibleMenu) {
                    leftMenuOffset = leftMenuIndex - maxVisibleMenu + 1;
                }
            }
            needUpdate = true;
          if (real == '\n') { 
            if (!inSystemSubMenu) { 
                // --- [1. 메인 메뉴 로직] ---
                if (leftMenuIndex == 0) {
                    isKoreanMode = !isKoreanMode; 
                    isCapsLockOn = false;   // 메뉴에서 바꿀 때도 강제 해제!
                    isShiftPressed = false;
                }
                else if (leftMenuIndex == 1) createNewDoc(); 
                else if (leftMenuIndex == 2) { saveFile(); currentMode = TYPING_MODE; } 
                else if (leftMenuIndex == 3) { countMode = (countMode + 1) % 3; needUpdate = true; }
                else if (leftMenuIndex == 4) { 
                    isEditingValue = true;
                }
                else if (leftMenuIndex == 5) { inSystemSubMenu = true; leftMenuIndex = 0; } // 시스템 설정
                else if (leftMenuIndex == 6) { currentMode = TYPING_MODE; showInitialImage(); ESP.restart(); } // 잠자기
                else if (leftMenuIndex == 7) { isEditingValue = true; needUpdate = true;}
                else if (leftMenuIndex == 8) { currentMode = TYPING_MODE; } // 나가기
            } 
            else { 
                // --- [2. 시스템 설정 서브 메뉴 로직] ---
                if (leftMenuIndex == 0) { 
                    // < 뒤로가기 누르면 메인의 '시스템 설정' 위치로 복귀
                    inSystemSubMenu = false;
                    leftMenuIndex = 4; 
                } 
                else if (leftMenuIndex >= 1 && leftMenuIndex <= 6) {
                    if (isEditingValue) {
                        // [중요] 이미 언어를 고르고 있는 상태에서 엔터를 누르면
                        if (leftMenuIndex == 6) {
                            isEditingValue = false;    // 1. 수정 모드 종료
                            currentMode = TYPING_MODE; // 2. 글쓰기 창으로 즉시 이동
                            needUpdate = true;         // 3. 화면 갱신
                            isCapsLockOn = false;      // 5. 캡스락 해제
                        } else {
                            isEditingValue = false;    // 다른 메뉴는 평소대로 수정만 종료
                        }
                    } else {
                        isEditingValue = true;         // 처음 엔터 치면 수정 모드 진입
                    }                
                }
                else if (leftMenuIndex == 7) { 
                    // [수정] 7번(업데이트) 인덱스일 때 펌웨어 OTA 모드 진입
                    isFirmwareUpdateMode = true; 
                    currentNetSubMode = NET_WIFI; 
                    setupWiFi();
                    currentMode = TYPING_MODE; 
                    inSystemSubMenu = false; 
                    needUpdate = true;
                    continue;
                }
            }
            needUpdate = true;
            statusBarNeedsUpdate = true;
            continue;
        }
        } else {
          // 화면 크기에 맞춰 한 화면에 들어갈 수 있는 최대 개수 계산
          int maxVisibleItems = ((display.height() / displayScale) - 70) / 22;
          if (maxVisibleItems < 1) maxVisibleItems = 1;
          
          if (k == 58 && rightFileIndex > 1) { // 위로
              rightFileIndex--;
              // 커서가 화면 천장에 닿으면 오프셋을 위로 밀어 올림
              if (rightFileIndex <= fileScrollOffset) fileScrollOffset = rightFileIndex - 1;
          }
          if (k == 59 && rightFileIndex < fileCount) { // 아래로
              rightFileIndex++;
              // 커서가 화면 바닥에 닿으면 오프셋을 아래로 밀어 내림
              if (rightFileIndex > fileScrollOffset + maxVisibleItems) {
                  fileScrollOffset = rightFileIndex - maxVisibleItems;
              }
          }
          if (fileScrollOffset < 0) fileScrollOffset = 0;
          // 파일 열기
          if (real == '\n' || k == 40) {
              if (fileCount > 0 && rightFileIndex <= fileCount) {
                  currentFileName = files[rightFileIndex].name;
                  loadFile();
                  currentMode = TYPING_MODE;
                  needUpdate = true;
                  continue;
              }
          }
          if (real == '\b') {
              isDeletingFile = true;
              needUpdate = true;
          }
        } continue;
    }
     else {
      // [타이핑 모드 입력 로직]
      if (k == 57 || k == 58 || k == 59 || k == 60) {
          flushKorean();
      }
      if (k == 58) { int ls = fullText.lastIndexOf('\n', cursorPos - 1); int col = cursorPos - (ls + 1); int ps = fullText.lastIndexOf('\n', ls - 1); int pl = ls - (ps + 1); if (ls != -1) cursorPos = (ps + 1) + min(col, pl); continue; } 
      if (k == 59) { 
        int ns = fullText.indexOf('\n', cursorPos); 
        if (ns != -1) { 
          int ne = fullText.indexOf('\n', ns + 1); 
          if (ne == -1) ne = fullText.length(); 
          int nl = ne - (ns + 1); 
          int ls = fullText.lastIndexOf('\n', cursorPos - 1); 
          int col = cursorPos - (ls + 1); 
          cursorPos = (ns + 1) + min(col, nl); 
          } 
          else if (fullText.indexOf('\n', cursorPos) == -1) {
              cursorPos = fullText.length();
          }

          needUpdate = true;
          continue; 
          }
      if (k == 57) { if (cursorPos > 0) { int p = cursorPos - 1; while (p > 0 && (fullText[p] & 0xC0) == 0x80) p--; cursorPos = p; } continue; }
      if (k == 60) { if (cursorPos < fullText.length()) { int n = cursorPos + 1; while (n < fullText.length() && (fullText[n] & 0xC0) == 0x80) n++; cursorPos = n; } continue; }
      if (real == '\b') doBackspace();
      else if (real != 0 || k == 56) { 
        if (isCtrlPressed && real == ' ') { 
            flushKorean(); 
            isKoreanMode = !isKoreanMode; 
            isCapsLockOn = false;   // 굳어버린 캡스락 강제 해제!
            isShiftPressed = false; // 시프트 꼬임 방지
            continue; 
        } 
        if (real == ' ' || real == '\n' || real == '\t') { 
            flushKorean();
            // 검색 모드에서 엔터를 쳤을 때의 동작
            if (currentMode == SEARCH_MODE) {
                if (real == '\t') { // Tab: 창 닫고 빠져나가기
                    currentMode = TYPING_MODE; 
                    needUpdate = true; 
                    continue; 
                }
                if (real == '\n') { // Enter: 창 유지하고 다음 단어로 화면 점프
                    int found = fullText.indexOf(searchQuery, cursorPos);
                    if (found == -1) found = fullText.indexOf(searchQuery, 0); 
                    if (found != -1) cursorPos = found + searchQuery.length();
                    needUpdate = true; 
                    continue;
                }
            }
            insertText((real == '\t') ? "    " : String(real));
        } 
        else if (isKoreanMode) {
          // [한글 조합 엔진]
          char korInput = real; 
          if (k < sizeof(engMap)) {
            korInput = isShiftPressed ? shiftMap[k] : engMap[k];
        }
        if (isShiftPressed && real >= 'A' && real <= 'Z') { 
            if (real != 'Q' && real != 'W' && real != 'E' && real != 'R' && real != 'T' && real != 'O' && real != 'P') 
            korInput = real + 32; 
        } 
          auto getCho = [](char c) { switch(c) { case 'r': return 0; case 'R': return 1; case 's': return 2; case 'e': return 3; case 'E': return 4; case 'f': return 5; case 'a': return 6; case 'q': return 7; case 'Q': return 8; case 't': return 9; case 'T': return 10; case 'd': return 11; case 'w': return 12; case 'W': return 13; case 'c': return 14; case 'z': return 15; case 'x': return 16; case 'v': return 17; case 'g': return 18; default: return -1; } }; 
          auto getJung = [](char c) { switch(c) { case 'k': return 0; case 'o': return 1; case 'i': return 2; case 'O': return 3; case 'j': return 4; case 'p': return 5; case 'u': return 6; case 'P': return 7; case 'h': return 8; case 'y': return 12; case 'n': return 13; case 'b': return 17; case 'm': return 18; case 'l': return 20; default: return -1; } }; 
          auto getJong = [](char c) { switch(c) { case 'r': return 1; case 'R': return 2; case 's': return 4; case 'e': return 7; case 'f': return 8; case 'a': return 16; case 'q': return 17; case 't': return 19; case 'T': return 20; case 'd': return 21; case 'w': return 22; case 'c': return 23; case 'z': return 24; case 'x': return 25; case 'v': return 26; case 'g': return 27; default: return 0; } }; 
          int ci = getCho(korInput), ji = getJung(korInput), joi = getJong(korInput); 
          if (ci != -1) { if (cho == -1 && jung == -1) cho = ci; else if (cho != -1 && jung == -1) { flushKorean(); cho = ci; } else if (cho != -1 && jung != -1 && jong == -1) { if (joi > 0) { jong = joi; lastJongChar = korInput; } else { flushKorean(); cho = ci; } } else if (cho != -1 && jung != -1 && jong != -1) { auto combineJong = [](int j1, int j2) { if (j1 == 1 && j2 == 19) return 3; if (j1 == 4 && j2 == 22) return 5; if (j1 == 4 && j2 == 27) return 6; if (j1 == 8 && j2 == 1) return 9; if (j1 == 8 && j2 == 16) return 10; if (j1 == 8 && j2 == 17) return 11; if (j1 == 8 && j2 == 19) return 12; if (j1 == 8 && j2 == 25) return 13; if (j1 == 8 && j2 == 26) return 14; if (j1 == 8 && j2 == 27) return 15; if (j1 == 17 && j2 == 19) return 18; return -1; }; int comb = combineJong(jong, joi); if (comb != -1) { jong = comb; lastJongChar = korInput; } else { flushKorean(); cho = ci; } } else if (cho == -1 && jung != -1) { flushKorean(); cho = ci; } } 
          else if (ji != -1) { if (cho != -1 && jung == -1) jung = ji; else if (cho != -1 && jung != -1 && jong == -1) { auto combineJung = [](int j1, int j2) { if (j1 == 8 && j2 == 0) return 9; if (j1 == 8 && j2 == 1) return 10; if (j1 == 8 && j2 == 20) return 11; if (j1 == 13 && j2 == 4) return 14; if (j1 == 13 && j2 == 5) return 15; if (j1 == 13 && j2 == 20) return 16; if (j1 == 18 && j2 == 20) return 19; return -1; }; int comb = combineJung(jung, ji); if (comb != -1) jung = comb; else { flushKorean(); jung = ji; } } else if (cho != -1 && jung != -1 && jong != -1) { int prev = lastJongChar; if ((jong == 3 || jong == 5 || jong == 6 || (jong >= 9 && jong <= 15) || jong == 18)) { switch(jong){case 3:jong=1;break;case 5:case 6:jong=4;break;case 18:jong=17;break;default:jong=8;break;} } else jong = -1; flushKorean(); cho = getCho(prev); jung = ji; } else if (cho == -1) { if (jung == -1) jung = ji; else { auto combineJung = [](int j1, int j2) { if (j1 == 8 && j2 == 0) return 9; if (j1 == 8 && j2 == 1) return 10; if (j1 == 8 && j2 == 20) return 11; if (j1 == 13 && j2 == 4) return 14; if (j1 == 13 && j2 == 5) return 15; if (j1 == 13 && j2 == 20) return 16; if (j1 == 18 && j2 == 20) return 19; return -1; }; int comb = combineJung(jung, ji); if (comb != -1) jung = comb; else { flushKorean(); jung = ji; } } } 
          } else { 
              flushKorean();
              insertText(String(real)); 
          } 
        } else {
            // [추가] 2,3,4,5 모드일 때 현지 키보드 배열 직접 매핑 적용
            String insertStr = String(real);
            if (latinMode > 1) {
                insertStr = getLocalLayoutChar(real, isAltPressed, latinMode);
            }
            if (insertStr != "") insertText(insertStr); 
        }

        // --- [추가] 방금 찍힌 글자가 알파벳이면 0.5초 타이머 시작 ---
        // EU(1번) 모드일 때만 알트 순환을 위한 타이머 기록
        if (latinMode == 1 && ((real >= 'a' && real <= 'z') || (real >= 'A' && real <= 'Z'))) {
            lastTypingTime = millis();
            lastBaseChar = real;
            accentCycleIdx = 0;
        } else if (real != 0 && real != ' ' && real != '\n' && real != '\t') {
            lastBaseChar = 0;
        }
      } // 기존 괄호 닫기
      
      charCounter++;
      if (typingSpeed > 0) delay(typingSpeed * 5); 
      else delay(0.1);
    }
  } // <<< while 루프

  // --- 출력부: 화면 그리기 (while문 밖) ---
  if (needUpdate) {
    // 계산팀이 바쁘지 않을 때만 최신 텍스트 복사본 전달
    if (!needCountUpdate) {
        calcBuffer = fullText;
        needCountUpdate = true;
    }
    
    // ... 기존 화면 그리기 로직 시작 ...
    currentActiveScale = displayScale; 
    int maxVisibleMenu = ((display.height() / displayScale) - 70) / 24; 
    if (maxVisibleMenu < 1) maxVisibleMenu = 1;
    bool doFullRefresh = false;
    if (currentMode == TYPING_MODE && refreshLimit > 0 && charCounter >= refreshLimit) { doFullRefresh = true; charCounter = 0; }

    if (currentNetSubMode != NET_MAIN) {
        // 1. 상태바 아래 본문 영역만 하얗게 밀기
        int statusBarBottom = (int)(45 * displayScale);
        display.fillRect(0, statusBarBottom, display.width(), display.height() - statusBarBottom, WHITE);

        // 2. 안내 문구 출력 (본문 맨 위)
        u8g2_for_adafruit_gfx.setFont(Typewriter_16px);
        int infoY = statusBarBottom + 40;
        if (isFirmwareUpdateMode) {
            printCleanText(u8g2_for_adafruit_gfx, "Update Mode (192.168.4.1/update)", MARGIN_X, infoY);
            printCleanText(u8g2_for_adafruit_gfx, "EXIT: Ctrl + Menu", MARGIN_X, infoY + 25);
        } else {
            printCleanText(u8g2_for_adafruit_gfx, "네트워크모드입니다. 나가기: 컨트롤 + 메뉴 키", MARGIN_X, infoY);
            printCleanText(u8g2_for_adafruit_gfx, "Network Mode On : (EXIT:Press Ctrl + Menu)", MARGIN_X, infoY + 25);
        }
    }
    else if (currentMode == FILE_MENU_MODE) {
        display.clearDisplay(); lastSy = -1; 
        display.drawLine((int)(215 * displayScale), 0, (int)(215 * displayScale), 700, BLACK);
        u8g2_for_adafruit_gfx.setForegroundColor(BLACK); // 글자는 검정색
        u8g2_for_adafruit_gfx.setBackgroundColor(WHITE); // 배경은 흰색
        printDualFont(isKoreanMode ? "=== 메뉴 ===" : "=== Menu ===", 50, 30, true);
       // 메인 메뉴 (기존 항목 축소)
        String m_main = isKoreanMode ? 
        "메뉴 [English],새 문서[N],저장[S],카운트,네트워크,시스템 설정 >,잠자기,자동 잠자기" : 
        "Menu [한글],New,Save,Count,Network,System Set >,Sleep,Auto Sleep";
        // 시스템 서브 메뉴 (새로 만든 보관함)
        String m_sys = isKoreanMode ? 
            "< 뒤로가기,글자크기,줄간격,글자간격,속도,새로고침,라틴키보드,업데이트" : 
            "< Back,Size,Line Sp,Char Sp,Speed,Refresh,Latin,Update";

        int menuCount = inSystemSubMenu ? 7 : 8; // 현재 각 메뉴의 항목 개수 (필요시 조정)
        String targetM = inSystemSubMenu ? m_sys : m_main;

for(int i = leftMenuOffset; i < 12; i++) { 
    if (i >= leftMenuOffset + maxVisibleMenu) break;

    // 1. 기본 메뉴 이름 가져오기
    String lbl = getValue(targetM, ',', i - leftMenuOffset);
    if (lbl == "") continue; // 항목이 없으면 패스

    // 2. [시스템 서브 메뉴]일 때만 설정값들을 뒤에 붙여줌
    if (inSystemSubMenu) {
        String valStr = "";
        if(i == 1)      valStr = String(displayScale, 1);
        else if(i == 2) valStr = String(lineSpacing);
        else if(i == 3) valStr = String(letterSpacing);
        else if(i == 4) valStr = String(typingSpeed);
        else if(i == 5) valStr = String(refreshLimit);
        // 6번 라틴 키보드 이름 결정
        else if(i == 6) { 
            valStr = "EN";
            if (latinMode == 1) valStr = "EU";
            else if (latinMode == 2) valStr = "DE";
            else if (latinMode == 3) valStr = "MN";
            else if (latinMode == 4) valStr = "TR";
            else if (latinMode == 5) valStr = "FR";
        }
        
        if (valStr != "") {
            if (isEditingValue && leftMenuIndex == i) {
                lbl += " < " + valStr + " >";
            } else {
                lbl += "   " + valStr + "   ";
            }
        }
        // 7번 업데이트 메뉴 뒤에 버전 표시 (원래 메뉴명은 유지)
        if (i == 7) {
            lbl += " " + String(FIRMWARE_VERSION); 
        }

    } 
    // 3. [메인 메뉴]일 때 상태 표시
    else {
        if (i == 3) { 
            // 카운트 모드는 방향키가 아니라 엔터로 바로 바뀌는 항목이므로 꺾쇠 대신 대괄호 사용
            if (countMode == 0) lbl += isKoreanMode ? " [글자수]" : " [Chars]";
            else if (countMode == 1) lbl += isKoreanMode ? " [단어수]" : " [Words]";
            else lbl += isKoreanMode ? " [끄기]" : " [OFF]";
        }
        if (i == 7) { // 자동 잠자기
            if (isEditingValue && leftMenuIndex == i) lbl += " < " + sleepLabels[autoSleepIndex] + " >";
            else lbl += "   " + sleepLabels[autoSleepIndex] + "   ";
        }
        if(i == 4) { // 네트워크 모드
            String netStr = "";
            if(tempNetCursor == NET_MAIN) netStr = "OFF";
            else if(tempNetCursor == NET_BT_SELECT) netStr = (bleKeyboard.isConnected() ? "BLE:OK" : "BLE");
            else if(tempNetCursor == NET_WIFI) netStr = "WIFI";
            
            if (isEditingValue && leftMenuIndex == i) lbl += " < " + netStr + " >";
            else lbl += "   " + netStr + "   ";
        }
    }

    // 4. 드디어 출력! (유령 없이 깔끔하게 한 번에)
    printMenuEntry(lbl, 25, 60 + ((i - leftMenuOffset) * 24), (menuFocusSide == 0 && leftMenuIndex == i), false);

        }
        u8g2_for_adafruit_gfx.setForegroundColor(BLACK); // 글자는 검정색
        u8g2_for_adafruit_gfx.setBackgroundColor(WHITE); // 배경은 흰색
        printDualFont(isKoreanMode ? "=== 문서목록 ===" : "=== DOCUMENTS ===", 245, 30, true); 
        // 계산된 최대 개수만큼만 화면에 그리고, 넘어가면 스크롤되게
        int maxVisibleItems = ((display.height() / displayScale) - 70) / 22;
        for (int f=1; f<=fileCount; f++) { 
            if (f > fileScrollOffset && f <= fileScrollOffset + maxVisibleItems) { 
                String docLabel = String(f) + ". " + files[f].name + " [" + String(files[f].sizeKB, 1) + "KB] | " + files[f].preview;
                printMenuEntry(docLabel, 225, 60 + ((f-fileScrollOffset-1)*22), (menuFocusSide == 1 && rightFileIndex == f), true);
            } 
        }
    } 
    else {
        // 타이핑 화면 렌더링
        adjustViewBottom(); // 그리기 직전에 커서 이탈 방지 앵커 계산

        String d = fullText;
        String composing = "";
        if (currentMode == TYPING_MODE && (cho != -1 || jung != -1)) {
            composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
        }
        
        int targetIdx = cursorPos + composing.length();
        d = d.substring(0, cursorPos) + composing + d.substring(cursorPos);

        // 문서 맨 끝(d.length)이 아니라, 계산된 바닥 앵커부터 그리기 시작
        int renderEnd = viewBottomIdx;
        if (currentMode == TYPING_MODE && cursorPos <= viewBottomIdx) renderEnd += composing.length();
        if (renderEnd > d.length()) renderEnd = d.length();

        int statusBarBottom = (int)(45 * displayScale);
        display.fillRect(0, statusBarBottom, display.width(), display.height() - statusBarBottom, WHITE);
        int currentY = (display.height() / displayScale) - 25; 
        
        int lastLineEnd = renderEnd; 
        for (int i = renderEnd; i >= 0; i--) { 
            if (i == 0 || d[i-1] == '\n') {
                String para = d.substring(i, lastLineEnd);
                int maxWidth = (display.width() / displayScale) - MARGIN_X - RIGHT_EDGE_MARGIN;
                String lines[40]; 
                int lineStartIdx[40]; 
                int lineCount = 0;
                int currentLineWidth = 0; 
                int currentLineStartK = i; // 현재 줄의 시작 인덱스

                u8g2_for_adafruit_gfx.setFont(Typewriter_16px);
                
                if (i >= lastLineEnd) { // 빈 줄일 경우
                    lines[0] = "";
                    lineStartIdx[0] = 0;
                    lineCount = 1;
                } else {
                    // [초고속화] d.substring 없이 원본 문자열(d)의 인덱스(k)로 직접 탐색
                    for(int k = i; k < lastLineEnd; ) {
                        int l = 1;
                        if ((d[k] & 0x80) != 0) {
                            if ((d[k] & 0xE0) == 0xC0) l = 2;
                            else if ((d[k] & 0xF0) == 0xE0) l = 3;
                            else l = 4;
                        }
                        
                        // [초고속화] 무거운 getUTF8Width 함수 대신, 한글(3)은 16, 나머지는 8로 폭 고정!
                        int charWidth = (l == 3) ? (16 + letterSpacing) : 8; 

                        if (currentLineWidth + charWidth > maxWidth) {
                            if (lineCount < 40) {
                                // 줄이 넘어갈 때 딱 한 번만 메모리(substring) 생성
                                lines[lineCount] = d.substring(currentLineStartK, k);
                                lineStartIdx[lineCount] = currentLineStartK - i;
                                lineCount++;
                            }
                            currentLineStartK = k; // 다음 줄 시작점 갱신
                            currentLineWidth = charWidth; 
                        } else {
                            currentLineWidth += charWidth;
                        }
                        k += l;
                    }
                    // 루프가 끝난 뒤 마지막에 남은 꼬투리 줄 처리
                    if (currentLineStartK < lastLineEnd && lineCount < 40) {
                        lines[lineCount] = d.substring(currentLineStartK, lastLineEnd);
                        lineStartIdx[lineCount] = currentLineStartK - i;
                        lineCount++;
                    }
                }
                for (int j = lineCount - 1; j >= 0; j--) {
                    if (currentY > STATUS_Y + 20) {
                        printCleanText(u8g2_for_adafruit_gfx, lines[j], MARGIN_X, currentY);
                        
                        int absLineStart = i + lineStartIdx[j];
                        int absLineEnd = (j < lineCount - 1) ? (i + lineStartIdx[j+1]) : lastLineEnd;
                        
                        if (targetIdx >= absLineStart && targetIdx <= absLineEnd) {
                            if (targetIdx < absLineEnd || j == lineCount - 1) {
                                int cursorStrLen = targetIdx - absLineStart;
                                String beforeCursor = para.substring(lineStartIdx[j], lineStartIdx[j] + cursorStrLen);
                                
                                int cursorXOffset = 0;
                                for(int x = 0; x < beforeCursor.length(); ) {
                                    int cl = 1;
                                    if ((beforeCursor[x] & 0x80) != 0) {
                                        if ((beforeCursor[x] & 0xE0) == 0xC0) cl = 2;
                                        else if ((beforeCursor[x] & 0xF0) == 0xE0) cl = 3;
                                        else cl = 4;
                                    }
                                    String cc = beforeCursor.substring(x, x+cl);
                                    cursorXOffset += (cl == 3) ? (16 + letterSpacing) : 8;
                                    x += cl;
                                }
                                
                                if (currentMode == TYPING_MODE) {
                                    bigDisplay.fillRect(MARGIN_X + cursorXOffset, currentY + 2, 12, 4, BLACK);
                                }
                                else if (currentMode == SEARCH_MODE && searchQuery.length() > 0) {
                                    int searchStart = targetIdx - searchQuery.length();
                                    
                                    // 현재 화면에 그리는 줄(j)에 검색어 시작점이 포함되어 있다면
                                    if (searchStart >= absLineStart && searchStart < absLineEnd) {
                                        
                                        // 1. 검색어 시작점(X 좌표) 계산
                                        int startStrLen = searchStart - absLineStart;
                                        String beforeSearch = para.substring(lineStartIdx[j], lineStartIdx[j] + startStrLen);
                                        int searchXOffset = 0;
                                        for(int x = 0; x < beforeSearch.length(); ) {
                                            int cl = 1;
                                            if ((beforeSearch[x] & 0x80) != 0) {
                                                if ((beforeSearch[x] & 0xE0) == 0xC0) cl = 2;
                                                else if ((beforeSearch[x] & 0xF0) == 0xE0) cl = 3;
                                                else cl = 4;
                                            }
                                            searchXOffset += (cl == 3) ? (16 + letterSpacing) : 8;
                                            x += cl;
                                        }

                                        // 2. 검색어 자체의 폭(Width) 계산
                                        int queryWidth = 0;
                                        for(int x = 0; x < searchQuery.length(); ) {
                                            int cl = 1;
                                            if ((searchQuery[x] & 0x80) != 0) {
                                                if ((searchQuery[x] & 0xE0) == 0xC0) cl = 2;
                                                else if ((searchQuery[x] & 0xF0) == 0xE0) cl = 3;
                                                else cl = 4;
                                            }
                                            queryWidth += (cl == 3) ? (16 + letterSpacing) : 8;
                                            x += cl;
                                        }

                                        // 3. 이미 까만색으로 그려진 전체 줄 위에, 까만색 박스를 덮어서 배경을 지움 (높이는 16px 폰트 기준)
                                        bigDisplay.fillRect(MARGIN_X + searchXOffset, currentY - 14, queryWidth, 18, BLACK);

                                        // 4. 전경색을 하얗게 바꾸고, 검색어만 다시 그 자리에 출력 (반전 효과)
                                        u8g2_for_adafruit_gfx.setForegroundColor(WHITE);
                                        u8g2_for_adafruit_gfx.setBackgroundColor(BLACK);
                                        //printCleanText(u8g2_for_adafruit_gfx, searchQuery, MARGIN_X + searchXOffset, currentY);
                                        printDualFont(searchQuery, MARGIN_X + searchXOffset, currentY);

                                        // 5. 다음 줄 출력을 위해 색상 원상 복구
                                        u8g2_for_adafruit_gfx.setForegroundColor(BLACK);
                                        u8g2_for_adafruit_gfx.setBackgroundColor(WHITE);
                                    }
                                }
                            }
                        }
                    }
                    currentY -= (baseFontSize + lineSpacing);
                    if (currentY < STATUS_Y + 10) break;
                }

                lastLineEnd = i - 1;
                if (currentY < STATUS_Y + 10) break; 
            }
        }
        lastSy = 0;
        
        if (currentMode == SEARCH_MODE) {
            int boxW = 500; int boxH = 60;
            int boxX = (display.width() / displayScale - boxW) / 2;
            int boxY = (display.height() / displayScale - boxH) / 2;
            display.fillRect((int)(boxX * displayScale), (int)(boxY * displayScale), (int)(boxW * displayScale), (int)(boxH * displayScale), BLACK);
            display.fillRect((int)((boxX + 4) * displayScale), (int)((boxY + 4) * displayScale), (int)((boxW - 8) * displayScale), (int)((boxH - 8) * displayScale), WHITE);
            String dispSearch = "찾기(Search): " + searchQuery;
            if (cho != -1 || jung != -1) dispSearch += ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
            dispSearch += "_";
            printCleanText(u8g2_for_adafruit_gfx, dispSearch, boxX + 20, boxY + 38, true);
        }
    } // if (currentMode == FILE_MENU_MODE) else 끝 
    
    // 화면 실제 갱신 처리
    //if (doFullRefresh) display.display(); 
    //else display.partialUpdate(false); 
    
    display.resetInternalCounter(); 
    needUpdate = false;
    lastMode = currentMode;

    // --- [상태바 로직 시작] ---
    if (currentMode == TYPING_MODE || currentMode == SEARCH_MODE) {
        int hY = 30;
        // 배경 깔기
        display.fillRect(0, 0, display.width(), (int)(40 * displayScale), WHITE);
        
        // 1. 모드 문자열 결정
        String modeStr = "";
        if (isKoreanMode) modeStr = "[가]";
        else {
            if (latinMode == 1)      modeStr = isCapsLockOn ? "[EU]" : "[eu]";
            else if (latinMode == 2) modeStr = isCapsLockOn ? "[DE]" : "[de]";
            else if (latinMode == 3) modeStr = isCapsLockOn ? "[MN]" : "[mn]";
            else if (latinMode == 4) modeStr = isCapsLockOn ? "[TR]" : "[tr]";
            else if (latinMode == 5) modeStr = isCapsLockOn ? "[FR]" : "[fr]"; // 🚀 추가됨
            else                     modeStr = isCapsLockOn ? "[EN]" : "[en]";
        }

        // 2. 카운트 모드에 따른 계산 (countMode 연동)
        String countStr = "";
        if (countMode == 0) { 
            charwordcount = sharedCharCount; // 🚀 Core 0이 계산해둔 값 즉시 가져오기! (0.0001초 컷)
            countStr = (isKoreanMode ? "글자: " : "Chars: ") + String(charwordcount);
        } else if (countMode == 1) { 
            charwordcount = sharedWordCount; // 🚀 Core 0이 계산해둔 값 즉시 가져오기!
            countStr = (isKoreanMode ? "단어: " : "Words: ") + String(charwordcount);
        }

        int countX   = (int)(120 / displayScale);                  
        int titleX   = (int)(340 / displayScale);                  
        int savedX   = (int)(540 / displayScale);                  
        int batteryX = (int)(display.width() / displayScale) - 55; 

        // 3. 화면 출력 (순서대로)
        printCleanText(u8g2_for_adafruit_gfx, modeStr + "  ", 15, hY, true);
        
        // 카운트 라벨 분기
        String countLabel = (countMode == 0) ? (isKoreanMode ? "글자: " : "Chars: ") : (isKoreanMode ? "단어: " : "Words: ");
        printCleanText(u8g2_for_adafruit_gfx, countStr, countX, hY, true);;
        
        // 문서 제목
        printCleanText(u8g2_for_adafruit_gfx, currentFileName, titleX, hY, true);

        // 저장 완료 메시지
        if (millis() - showSavedMessageTime < 3000) {
            u8g2_for_adafruit_gfx.setForegroundColor(BLACK);
            printCleanText(u8g2_for_adafruit_gfx, "[Saved!]", savedX, hY, true);
        }

        // 배터리
        float batV = display.readBattery();
        int batPct = constrain((int)((batV - 3.3) / (4.2 - 3.3) * 100), 0, 100);
        printCleanText(u8g2_for_adafruit_gfx, String(batPct) + "%   ", batteryX, hY, true);
        
        display.drawFastHLine(0, (int)(39 * displayScale), display.width(), BLACK);
    }
    // --- [마지막 화면 갱신: 여기서 딱 한 번만!] ---
    if (doFullRefresh) display.display();
    else display.partialUpdate(false); 
    
    display.resetInternalCounter();
    needUpdate = false; 
    lastMode = currentMode;
  } // if (needUpdate) 블록 끝
  
  if (currentMode == TYPING_MODE && autoSleepIndex != 6) {
    if (millis() - lastKeyPress > sleepIntervals[autoSleepIndex]) {
        showInitialImage(); // 딥슬립 진입
        }
    }
} // loop() 함수 끝
