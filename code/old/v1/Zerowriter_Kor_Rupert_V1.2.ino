#define ARDUINO_INKPLATE5V2 

#include <Adafruit_GFX.h>
#include "Inkplate.h"                
#include "U8g2_for_Adafruit_GFX.h"   
#include "Typewriter_16px.h" 

#include <esp_sleep.h>
#include <driver/uart.h>
#include "ZeroWriter_Helper.h"

NetworkSubMode currentNetSubMode = NET_MAIN;
class InkplateProxy : public Inkplate {
public:
    InkplateProxy(uint8_t mode) : Inkplate(mode), Adafruit_GFX(800, 600) {}
    void resetInternalCounter() { _partialUpdateCounter = 0; } 
};

InkplateProxy display(INKPLATE_1BIT); 

enum AppMode { TYPING_MODE, FILE_MENU_MODE, INITIAL_MODE, SEARCH_MODE };
AppMode currentMode = TYPING_MODE;
AppMode lastMode = TYPING_MODE; 
bool isDeletingFile = false; 
String clipboard = "";

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

int refreshLimit = 2000;   
int charCounter = 0;    
int lineSpacing = 2;     
int letterSpacing = 0;   
int typingSpeed = 0;     
String searchQuery = ""; 

String fullText = ""; 
int cursorPos = 0; 
bool isKoreanMode = false; 
bool isShiftPressed = false; 
bool isCtrlPressed = false; 
bool isCapsLockOn = false; 
unsigned long lastKeyPress = 0; 
bool needUpdate = false;
bool statusBarNeedsUpdate = true; 
unsigned long showSavedMessageTime = 0; 

int startIdx = 0;
int menuFocusSide = 0;   
int leftMenuIndex = 0;   
int rightFileIndex = 1;  
int fileScrollOffset = 0; 
bool isEditingValue = false; 

const char* choStrs[] = {"ㄱ","ㄲ","ㄴ","ㄷ","ㄸ","ㄹ","ㅁ","ㅂ","ㅃ","ㅅ","ㅆ","ㅇ","ㅈ","ㅉ","ㅊ","ㅋ","ㅌ","ㅍ","ㅎ"};
const char* jungStrs[] = {"ㅏ","ㅐ","ㅑ","ㅒ","ㅓ","ㅔ","ㅕ","ㅖ","ㅗ","ㅘ","ㅙ","ㅚ","ㅛ","ㅜ","우","ㅝ","ㅞ","ㅟ","ㅠ","ㅡ","ㅢ","ㅣ"};
int cho = -1, jung = -1, jong = -1; char lastJongChar = 0;

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
            
            // 1. 커서 바로 앞의 공백이나 줄바꿈을 먼저 찾아서 지울 범위에 포함합니다.
            while (p > 0 && (fullText[p] == ' ' || fullText[p] == '\n')) p--;
            
            // 2. 단어의 시작점(공백이나 줄바꿈의 직후)을 찾을 때까지 거슬러 올라갑니다.
            while (p > 0 && fullText[p-1] != ' ' && fullText[p-1] != '\n') p--;
            
            // 3. 찾아낸 위치(p)부터 현재 커서까지 통째로 날려버립니다.
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

void showInitialImage() { //잠자기 모드 initial.png 그림 띄우기 
    flushKorean(); 
    display.clearDisplay(); 

    if (imgBuffer) {
        int16_t x = (display.width() - 800) / 2; // 그림이 800px 기준일 때
        int16_t y = (display.height() - 600) / 2; // 그림이 600px 기준일 때
        if (x < 0) x = 0;
        if (y < 0) y = 0;

        // [수정] drawPngFromBuffer(버퍼, 크기, x, y, 반전여부, 흑백여부)
        display.image.drawPngFromBuffer(imgBuffer, imgSize, x, y, true, false); 
    } 

    display.display(); 
    delay(1000); 

    // 잠자기 로직 시작
    gpio_wakeup_enable(GPIO_NUM_3, GPIO_INTR_LOW_LEVEL); 
    esp_sleep_enable_gpio_wakeup(); 
    esp_light_sleep_start(); 

    // 깨어난 후 정리
    display.clearDisplay(); 
    display.fillRect(0, 0, display.width(), display.height(), WHITE); 
    while (Serial.available() > 0) Serial.read(); 
    currentMode = TYPING_MODE; 
    needUpdate = true; 
    statusBarNeedsUpdate = true; 
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
  int cx = x; int applyLetterSp = isMenu ? 0 : letterSpacing; 
  u8g2_for_adafruit_gfx.setFontMode(0); 
  for (int i = 0; i < text.length(); ) {
    int l = 1; if ((text[i] & 0x80) != 0) { if ((text[i] & 0xE0) == 0xC0) l = 2; else if ((text[i] & 0xF0) == 0xE0) l = 3; else l = 4; }
    String c = text.substring(i, i + l);
    if (l == 1) {
                u8g2_for_adafruit_gfx.setFont(Typewriter_16px);
      u8g2_for_adafruit_gfx.setCursor(cx, y); u8g2_for_adafruit_gfx.print(c); 
      cx += u8g2_for_adafruit_gfx.getUTF8Width(c.c_str()) + applyLetterSp + (isMenu ? 1 : 0);
    } else {
      u8g2_for_adafruit_gfx.setFont(Typewriter_16px);
      u8g2_for_adafruit_gfx.setCursor(cx, y); u8g2_for_adafruit_gfx.print(c); 
      cx += 16 + applyLetterSp; 
    }
    i += l;
  }
}

void printMenuEntry(String text, int x, int y, bool isSelected, bool isRightSide) {
  if (isRightSide) {
    int dotIdx = text.indexOf('.');
    String numPart = text.substring(0, dotIdx + 1);
    String contentPart = text.substring(dotIdx + 1);
    if (isSelected && isDeletingFile) {//문서목록에서 삭제키(백스페이스)
        display.fillRect((int)(x * displayScale - 4), (int)((y - 14) * displayScale), (int)(580 * displayScale), (int)(20 * displayScale), BLACK);
        u8g2_for_adafruit_gfx.setForegroundColor(WHITE); u8g2_for_adafruit_gfx.setBackgroundColor(BLACK);
        String msg = isKoreanMode ? "삭제할까요? (Enter:확인 / BS:취소)" : "Delete? (Enter:OK / BS:Cancel)";//엔터 눌러서 삭제, 백스페이스로 취소
        printDualFont(msg, x, y, true);
    } else if (isSelected) {
      int numW = u8g2_for_adafruit_gfx.getUTF8Width(numPart.c_str()) + 4;
      display.fillRect((int)(x * displayScale - 4), (int)((y - 14) * displayScale), (int)(numW * displayScale), (int)(18 * displayScale), BLACK);
      u8g2_for_adafruit_gfx.setForegroundColor(WHITE); u8g2_for_adafruit_gfx.setBackgroundColor(BLACK);
      printDualFont(numPart, x, y, true);
      u8g2_for_adafruit_gfx.setForegroundColor(BLACK); u8g2_for_adafruit_gfx.setBackgroundColor(WHITE);
      printDualFont(contentPart, x + numW, y, true);
    } else { printDualFont(text, x, y, true); }
  } else {
    if (isSelected) {
      display.fillRect((int)(x * displayScale - 4), (int)((y - 14) * displayScale), (int)(180 * displayScale), (int)(20 * displayScale), BLACK);
      u8g2_for_adafruit_gfx.setForegroundColor(WHITE); u8g2_for_adafruit_gfx.setBackgroundColor(BLACK);
      if (isEditingValue && text.indexOf('<') != -1) {
          int openBracket = text.indexOf('<');
          String labelPart = text.substring(0, openBracket);
          String valuePart = text.substring(openBracket);
          printDualFont(labelPart, x, y, true);
          u8g2_for_adafruit_gfx.setForegroundColor(BLACK); u8g2_for_adafruit_gfx.setBackgroundColor(WHITE);
          printDualFont(valuePart, x + 115, y, true); 
      } else { printDualFont(text, x, y, true); }
    } else { printDualFont(text, x, y, true); }
  }
  u8g2_for_adafruit_gfx.setForegroundColor(BLACK); u8g2_for_adafruit_gfx.setBackgroundColor(WHITE);
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

void setup() {//부팅
  Serial.begin(921600); display.begin(); display.setRotation(0); u8g2_for_adafruit_gfx.begin(bigDisplay);
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
  needUpdate = true;
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
    server.begin();
}


void loop() {
  if (currentNetSubMode == NET_WIFI) {
      server.handleClient(); // 폰의 접속 요청을 계속 대기함
  }
  // --- 입력부: 키를 읽고 데이터 전송 ---
  while (Serial.available() > 0) {
    byte k = Serial.read(); 
    lastKeyPress = millis(); 
    needUpdate = true; 
    statusBarNeedsUpdate = true;
    
    // 특수키 처리
    if (k == 240) { isShiftPressed = true; continue; } 
    if (k == 241) { isShiftPressed = false; continue; } 
    if (k == 242) { isCtrlPressed = true; continue; } 
    if (k == 243) { isCtrlPressed = false; continue; }

    const char engMap[] = { '`','1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\\',0,'a','s','d','f','g','h','j','k','l',';','\'','\n',0,'z','x','c','v','b','n','m',',','.','/',0,0,0,0,0,' ',0,0,0 }; 
    const char shiftMap[] = { '~','!','@','#','$','%','^','&','*','(',')','_','+','\b','\t','Q','W','E','R','T','Y','U','I','O','P','{','}','|',0,'A','S','D','F','G','H','J','K','L',':','\"','\n',0,'Z','X','C','V','B','N','M','<','>','?',0,0,0,0,0,' ',0,0,0 };
    char real = (k < sizeof(engMap)) ? ((isShiftPressed || isCapsLockOn) ? shiftMap[k] : engMap[k]) : 0; 
    if (k == 56) real = ' '; 

    // 네트워크 실시간 전송
    if (currentMode == TYPING_MODE && (real != 0 || k == 56)) {
      if (!isCtrlPressed) { 
        String sendStr = (real == ' ' || k == 56) ? " " : String(real);
        if (currentNetSubMode == NET_USB) Serial.print(sendStr);
        if (currentNetSubMode == NET_BT_SELECT && bleKeyboard.isConnected()) bleKeyboard.print(sendStr);
      }
    }
    // 메뉴 진입 및 컨트롤키 로직
    if (isCtrlPressed) {
      if (real == 'c' || real == 'C') { flushKorean(); clipboard = fullText; continue; } // 전체 복사
      if (real == 'v' || real == 'V') { flushKorean(); insertText(clipboard); continue; } // 붙여넣기
      if (real == 'l' || real == 'L') { isCtrlPressed = false; showInitialImage(); continue; }
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
      if (currentMode == TYPING_MODE) { flushKorean(); currentMode = FILE_MENU_MODE; refreshFileList(); isDeletingFile = false; } 
      else currentMode = TYPING_MODE; 
      continue; 
    }
    
    // 모드별 입력 처리 분기
    if (currentMode == FILE_MENU_MODE) {
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
        if (k == 57) { // Left
          if (leftMenuIndex == 4 && displayScale > 0.5) displayScale -= 0.1; 
          else if (leftMenuIndex == 5 && lineSpacing > 0) lineSpacing--; 
          else if (leftMenuIndex == 6 && letterSpacing > -5) letterSpacing--; 
          else if (leftMenuIndex == 7 && typingSpeed > 0) typingSpeed--; 
          else if (leftMenuIndex == 8 && refreshLimit > 1) refreshLimit--; 
        }
        if (k == 60) { // Right
          if (leftMenuIndex == 4 && displayScale < 5.0) displayScale += 0.1; 
          else if (leftMenuIndex == 5 && lineSpacing < 30) lineSpacing++; 
          else if (leftMenuIndex == 6 && letterSpacing < 10) letterSpacing++; 
          else if (leftMenuIndex == 7 && typingSpeed < 10) typingSpeed++; 
          else if (leftMenuIndex == 8 && refreshLimit < 500) refreshLimit++; 
          
        }
        if (real == '\n') isEditingValue = false;
      } else {
        if (k == 57) menuFocusSide = 0; if (k == 60) menuFocusSide = 1; 
        if (menuFocusSide == 0) {
          if (k == 58 && leftMenuIndex > 0) leftMenuIndex--; 
          if (k == 59 && leftMenuIndex < 11) leftMenuIndex++; 
          if (real == '\n') { 
              if (leftMenuIndex == 0) isKoreanMode = !isKoreanMode; 
              else if (leftMenuIndex == 1) createNewDoc(); 
              else if (leftMenuIndex == 2) { saveFile(); currentMode = TYPING_MODE; } 
              else if (leftMenuIndex >= 4 && leftMenuIndex <= 8) isEditingValue = true; 
              else if (leftMenuIndex == 9) { // 네트워크 메뉴 엔터 실행
                  // 순환 로직: BLE와 WIFI
                  if (currentNetSubMode == NET_MAIN) { 
                      // OFF -> BLE로 바로 이동
                      currentNetSubMode = NET_BT_SELECT; 
                      setupBLE(); 
                      Serial.println("\n[SYSTEM] Bluetooth Mode Engaged."); 
                  }
                  else if (currentNetSubMode == NET_BT_SELECT) { 
                      // BLE -> WIFI로 전환 (자원 정화 필수)
                      WiFi.mode(WIFI_OFF); 
                      btStop(); 
                      delay(300); 

                      currentNetSubMode = NET_WIFI; 
                      setupWiFi(); 
                      Serial.println("\n[SYSTEM] WiFi Mode Engaged.");
                  }
                  else { 
                      // WIFI -> OFF (모든 무선 종료 및 초기화)
                      WiFi.softAPdisconnect(true);
                      WiFi.disconnect(true, true);
                      WiFi.mode(WIFI_OFF);
                      currentNetSubMode = NET_MAIN; 
                      Serial.println("\n[SYSTEM] All Networks OFF.");
                  }
                  
                  needUpdate = true;
                  statusBarNeedsUpdate = true;
              }
              else if (leftMenuIndex == 10) showInitialImage(); // 잠자기
              else if (leftMenuIndex == 11) currentMode = TYPING_MODE; // 메뉴 나가기
          }
        } else {
          // 화면 크기에 맞춰 한 화면에 들어갈 수 있는 최대 개수 계산
          int maxVisibleItems = ((display.height() / displayScale) - 70) / 22;
          if (maxVisibleItems < 1) maxVisibleItems = 1;
          
          if (k == 58 && rightFileIndex > 1) {
              rightFileIndex--;
              if (rightFileIndex <= fileScrollOffset) fileScrollOffset--;
          }
          if (k == 59 && rightFileIndex < fileCount) {
              rightFileIndex++;
              
              if (rightFileIndex > fileScrollOffset + maxVisibleItems) fileScrollOffset++;
          }
          // 파일 열기
          if (real == '\n' || k == 13 || k == 40) {
              if (fileCount > 0 && rightFileIndex <= fileCount) {
                  currentFileName = files[rightFileIndex].name;
                  loadFile();
                  currentMode = TYPING_MODE;
                  needUpdate = true;
              }
          }
          if (real == '\b') {
              isDeletingFile = true;
              needUpdate = true;
          }
        }
      }
    } else {
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
        if (isCtrlPressed && real == ' ') { flushKorean(); isKoreanMode = !isKoreanMode; continue; } 
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
          char korInput = real; if (isShiftPressed && real >= 'A' && real <= 'Z') { if (real != 'Q' && real != 'W' && real != 'E' && real != 'R' && real != 'T' && real != 'O' && real != 'P') korInput = real + 32; } 
          auto getCho = [](char c) { switch(c) { case 'r': return 0; case 'R': return 1; case 's': return 2; case 'e': return 3; case 'E': return 4; case 'f': return 5; case 'a': return 6; case 'q': return 7; case 'Q': return 8; case 't': return 9; case 'T': return 10; case 'd': return 11; case 'w': return 12; case 'W': return 13; case 'c': return 14; case 'z': return 15; case 'x': return 16; case 'v': return 17; case 'g': return 18; default: return -1; } }; 
          auto getJung = [](char c) { switch(c) { case 'k': return 0; case 'o': return 1; case 'i': return 2; case 'O': return 3; case 'j': return 4; case 'p': return 5; case 'u': return 6; case 'P': return 7; case 'h': return 8; case 'y': return 12; case 'n': return 13; case 'b': return 17; case 'm': return 18; case 'l': return 20; default: return -1; } }; 
          auto getJong = [](char c) { switch(c) { case 'r': return 1; case 'R': return 2; case 's': return 4; case 'e': return 7; case 'f': return 8; case 'a': return 16; case 'q': return 17; case 't': return 19; case 'T': return 20; case 'd': return 21; case 'w': return 22; case 'c': return 23; case 'z': return 24; case 'x': return 25; case 'v': return 26; case 'g': return 27; default: return 0; } }; 
          int ci = getCho(korInput), ji = getJung(korInput), joi = getJong(korInput); 
          if (ci != -1) { if (cho == -1 && jung == -1) cho = ci; else if (cho != -1 && jung == -1) { flushKorean(); cho = ci; } else if (cho != -1 && jung != -1 && jong == -1) { if (joi > 0) { jong = joi; lastJongChar = korInput; } else { flushKorean(); cho = ci; } } else if (cho != -1 && jung != -1 && jong != -1) { auto combineJong = [](int j1, int j2) { if (j1 == 1 && j2 == 19) return 3; if (j1 == 4 && j2 == 22) return 5; if (j1 == 4 && j2 == 27) return 6; if (j1 == 8 && j2 == 1) return 9; if (j1 == 8 && j2 == 16) return 10; if (j1 == 8 && j2 == 17) return 11; if (j1 == 8 && j2 == 19) return 12; if (j1 == 8 && j2 == 25) return 13; if (j1 == 8 && j2 == 26) return 14; if (j1 == 8 && j2 == 27) return 15; if (j1 == 17 && j2 == 19) return 18; return -1; }; int comb = combineJong(jong, joi); if (comb != -1) { jong = comb; lastJongChar = korInput; } else { flushKorean(); cho = ci; } } else if (cho == -1 && jung != -1) { flushKorean(); cho = ci; } } 
          else if (ji != -1) { if (cho != -1 && jung == -1) jung = ji; else if (cho != -1 && jung != -1 && jong == -1) { auto combineJung = [](int j1, int j2) { if (j1 == 8 && j2 == 0) return 9; if (j1 == 8 && j2 == 1) return 10; if (j1 == 8 && j2 == 20) return 11; if (j1 == 13 && j2 == 4) return 14; if (j1 == 13 && j2 == 5) return 15; if (j1 == 13 && j2 == 20) return 16; if (j1 == 18 && j2 == 20) return 19; return -1; }; int comb = combineJung(jung, ji); if (comb != -1) jung = comb; else { flushKorean(); jung = ji; } } else if (cho != -1 && jung != -1 && jong != -1) { int prev = lastJongChar; if ((jong == 3 || jong == 5 || jong == 6 || (jong >= 9 && jong <= 15) || jong == 18)) { switch(jong){case 3:jong=1;break;case 5:case 6:jong=4;break;case 18:jong=17;break;default:jong=8;break;} } else jong = -1; flushKorean(); cho = getCho(prev); jung = ji; } else if (cho == -1) { if (jung == -1) jung = ji; else { auto combineJung = [](int j1, int j2) { if (j1 == 8 && j2 == 0) return 9; if (j1 == 8 && j2 == 1) return 10; if (j1 == 8 && j2 == 20) return 11; if (j1 == 13 && j2 == 4) return 14; if (j1 == 13 && j2 == 5) return 15; if (j1 == 13 && j2 == 20) return 16; if (j1 == 18 && j2 == 20) return 19; return -1; }; int comb = combineJung(jung, ji); if (comb != -1) jung = comb; else { flushKorean(); jung = ji; } } } 
          } else { flushKorean(); insertText(String(real)); } 
        } else insertText(String(real)); 
      } 
      charCounter++;
      if (typingSpeed > 0) delay(typingSpeed * 5); 
      else delay(1);
    }
  } // <<< while 루프

  // --- 출력부: 화면 그리기 (while문 밖) ---
  if (needUpdate) {
    currentActiveScale = displayScale; 
    bool doFullRefresh = false;
    if (currentMode == TYPING_MODE && refreshLimit > 0 && charCounter >= refreshLimit) { doFullRefresh = true; charCounter = 0; }

    if (currentMode == FILE_MENU_MODE) {
        display.clearDisplay(); lastSy = -1; 
        display.drawLine((int)(200 * displayScale), 0, (int)(200 * displayScale), 700, BLACK);
        printDualFont(isKoreanMode ? "=== 시스템 ===" : "=== SYSTEM ===", 40, 30, true);
        String m[] = { 
            (isKoreanMode ? "메뉴 [한/영]" : "Menu [EN/KOR]"), 
            (isKoreanMode ? "새 문서 [Ctrl+N]" : "New [Ctrl+N]"), 
            (isKoreanMode ? "저장 [Ctrl+S]" : "Save [Ctrl+S]"), 
            (isKoreanMode ? "찾기 [Ctrl+F]" : "Search [Ctrl+F]"), 
            (isKoreanMode ? "글자크기" : "Size"), 
            (isKoreanMode ? "줄간격" : "Line Sp"), 
            (isKoreanMode ? "글자간격" : "Char Sp"), 
            (isKoreanMode ? "속도" : "Speed"), 
            (isKoreanMode ? "새로고침" : "Refresh"), 
            (isKoreanMode ? "네트워크" : "Network"), 
            (isKoreanMode ? "잠자기 [Ctrl+L]" : "Sleep [Ctrl+L]"),
            (isKoreanMode ? "메뉴나가기 [MENU]" : "Exit [MENU]")   // 11번: 끝
        };
for(int i=0; i<12; i++) {
          String lbl = m[i]; 
          if(i==4) lbl += " <" + String(displayScale, 1) + ">"; 
          else if(i==5) lbl += " <" + String(lineSpacing) + ">";
          else if(i==6) lbl += " <" + String(letterSpacing) + ">";
          else if(i==7) lbl += " <" + String(typingSpeed) + ">";
          else if(i==8) lbl += " <" + String(refreshLimit) + ">";
          else if(i==9) { // 9번 네트워크 메뉴 표시 로직
            if(currentNetSubMode == NET_MAIN) lbl += " <OFF>";
            
            else if(currentNetSubMode == NET_BT_SELECT) {
                lbl += " <BLE>"; 
                if(bleKeyboard.isConnected()) lbl += " [OK]"; // 연결되면 OK 표시
            }
            else if(currentNetSubMode == NET_WIFI) lbl += " <WIFI>"; 
          }
          printMenuEntry(lbl, 10, 60 + (i*24), (menuFocusSide == 0 && leftMenuIndex == i), false);
        }
        printDualFont(isKoreanMode ? "=== 문서목록 ===" : "=== DOCUMENTS ===", 235, 30, true); 
        // 계산된 최대 개수만큼만 화면에 그리고, 넘어가면 스크롤되게
        int maxVisibleItems = ((display.height() / displayScale) - 70) / 22;
        for (int f=1; f<=fileCount; f++) { 
            if (f > fileScrollOffset && f <= fileScrollOffset + maxVisibleItems) { 
                String docLabel = String(f) + ". " + files[f].name + " [" + String(files[f].sizeKB, 1) + "KB] | " + files[f].preview;
                printMenuEntry(docLabel, 205, 60 + ((f-fileScrollOffset-1)*22), (menuFocusSide == 1 && rightFileIndex == f), true);
            } 
        }
    } else {
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
                String currentLine = "";
                int currentLineWidth = 0; 
                int currentLineStartK = 0;

                u8g2_for_adafruit_gfx.setFont(Typewriter_16px);
                
                if (para.length() == 0) {
                    lines[0] = "";
                    lineStartIdx[0] = 0;
                    lineCount = 1;
                } else {
                    for(int k = 0; k < para.length(); ) {
                        int l = 1;
                        if ((para[k] & 0x80) != 0) {
                            if ((para[k] & 0xE0) == 0xC0) l = 2;
                            else if ((para[k] & 0xF0) == 0xE0) l = 3;
                            else l = 4;
                        }
                        String c = para.substring(k, k+l);
                        int charWidth = (l == 1) ? u8g2_for_adafruit_gfx.getUTF8Width(c.c_str()) : (16 + letterSpacing);

                        if (currentLineWidth + charWidth > maxWidth) {
                            if (lineCount < 40) {
                                lines[lineCount] = currentLine;
                                lineStartIdx[lineCount] = currentLineStartK;
                                lineCount++;
                            }
                            currentLine = c; 
                            currentLineWidth = charWidth; 
                            currentLineStartK = k;
                        } else {
                            currentLine += c;
                            currentLineWidth += charWidth;
                        }
                        k += l;
                    }
                    if (currentLine.length() > 0 && lineCount < 40) {
                        lines[lineCount] = currentLine;
                        lineStartIdx[lineCount] = currentLineStartK;
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
                                    cursorXOffset += (cl == 1) ? u8g2_for_adafruit_gfx.getUTF8Width(cc.c_str()) : (16 + letterSpacing);
                                    x += cl;
                                }
                                
                                if (currentMode == TYPING_MODE) {
                                    bigDisplay.fillRect(MARGIN_X + cursorXOffset, currentY + 2, 12, 4, BLACK);
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
    if (doFullRefresh) display.display(); 
    else display.partialUpdate(true); 
    
    display.resetInternalCounter(); 
    needUpdate = false;
    lastMode = currentMode;
  } // if (needUpdate) 블록 끝

  // --- 상태바 업데이트 (입력 후 일정 시간 뒤에만 실행) ---
  if (!needUpdate && statusBarNeedsUpdate && (millis() - lastKeyPress >= 1000) && currentMode == TYPING_MODE) {
    int hY = 30;
    
    display.fillRect(0, 0, (int)(80 * displayScale), (int)(45 * displayScale), WHITE); 

    display.fillRect(0, 0, display.width(), (int)(45 * displayScale), WHITE);
    display.drawFastHLine(0, (int)(45 * displayScale), display.width(), BLACK); // 45px 밑에 검은 구분선 긋기
    printCleanText(u8g2_for_adafruit_gfx, isKoreanMode ? "[한]" : "[EN]", 15, hY, true);
    
    printCleanText(u8g2_for_adafruit_gfx, (isKoreanMode ? "글자수: " : "Count: ") + String(getTrueLength(fullText)) + "    ", (int)(360 / displayScale)+20, hY, true);   
    String netStr = (currentNetSubMode == NET_BT_SELECT) ? "BT" : (currentNetSubMode == NET_WIFI ? "WiFi" : "OFF");
    if (currentNetSubMode == NET_BT_SELECT && bleKeyboard.isConnected()) netStr += "[OK]";
    printCleanText(u8g2_for_adafruit_gfx, netStr + "    ", (int)(display.width() / displayScale) - 130, hY, true);
    // 저장 메시지 2초 출력 후 깔끔하게 지우기
    if (showSavedMessageTime > 0) {
        if (millis() - showSavedMessageTime < 2000) {
            // 2초 동안은 저장 완료 메시지 표시
            printCleanText(u8g2_for_adafruit_gfx, "Saved!    ", (int)(display.width() / displayScale) - 250, hY, true);
        } else {
            // 2초가 지나면 공백으로 덮어써서 지우고 타이머 초기화
            printCleanText(u8g2_for_adafruit_gfx, "          ", (int)(display.width() / displayScale) - 250, hY, true);
            showSavedMessageTime = 0; 
        }
    }
    
    float batV = display.readBattery();
    int batPct = (batV - 3.3) / (4.2 - 3.3) * 100;
    if (batPct > 100) batPct = 100; // 100% 초과 방지
    if (batPct < 0) batPct = 0;     // 0% 미만 방지
    
    // 변환된 %를 화면에 출력 (뒤에 공백 지우개 포함)
    printCleanText(u8g2_for_adafruit_gfx, String(batPct) + "%   ", (int)(display.width() / displayScale) - 50, hY, true);
    //display.drawLine(35, (int)((hY + 4) * displayScale), display.width() - 35, (int)((hY + 4) * displayScale), BLACK);
    
    display.partialUpdate(false);
    statusBarNeedsUpdate = false;
  }
} // loop() 함수 완전 종료
