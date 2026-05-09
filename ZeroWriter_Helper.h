#ifndef ZEROWRITER_HELPER_H
#define ZEROWRITER_HELPER_H
extern int lineSpacing;
extern int letterSpacing;
#include <ESPmDNS.h> // 이 라이브러리가 주소를 이름으로 바꿔줍니다.
#include <U8g2_for_Adafruit_GFX.h>
extern const uint8_t Typewriter_16px[];
// [사장님 설정존] 여기서 여백을 한 번에 조절하십시오.
const int MARGIN_X = 35; 
const int MARGIN_Y = 65; 
const int STATUS_Y = 30;
const int RIGHT_EDGE_MARGIN = 35;

// 네트워크 모드 상태 정의
enum NetworkSubMode { NET_MAIN, NET_USB, NET_BT_SELECT, NET_WIFI };
extern NetworkSubMode currentNetSubMode;

/**
 * [잔상 제거 핵심] 글자 배경색을 강제로 입히는 렌더링 함수
 * setFontMode(0)을 사용하여 글자 뒤의 잔상을 흰색으로 지우며 씁니다.
 */
void printCleanText(U8G2_FOR_ADAFRUIT_GFX &u8g2, const String& text, int x, int y, bool isMenu = false, int maxWidth = 800) {
    int cx = x;
    u8g2.setFontMode(0); 
    u8g2.setBackgroundColor(WHITE);
    u8g2.setForegroundColor(BLACK);

    const char* ptr = text.c_str();
    int len = text.length();

    for (int i = 0; i < len; ) {
        int l = 1; 
        if (cx > maxWidth) break;
        
        unsigned char c_val = (unsigned char)ptr[i];
        if (c_val >= 0x80) {
            if ((c_val & 0xE0) == 0xC0) l = 2;
            else if ((c_val & 0xF0) == 0xE0) l = 3;
            else l = 4;
        }
        
        // 메모리를 갉아먹는 String 생성 대신 가벼운 char 배열 사용
        char buf[5] = {0};
        for(int m=0; m<l; m++) buf[m] = ptr[i+m];
        
        if (l == 1) {
            u8g2.setFont(Typewriter_16px);
            u8g2.drawUTF8(cx, y, buf);
            cx += u8g2.getUTF8Width(buf) + (isMenu ? 1 : 0);
        } else {
            u8g2.setFont(Typewriter_16px);
            u8g2.drawUTF8(cx, y, buf);
            cx += 16 + (isMenu ? 1 : letterSpacing);
        }
        i += l;
    }
}

/**
 * 네트워크 메뉴 화면 그리기
 */
void drawNetworkUI(Inkplate &display, U8G2_FOR_ADAFRUIT_GFX &u8g2, float scale, int selectedIdx) {
    // 1. 화면 전체를 물리적으로 하얗게 밀어버림 (잔상 방지)
    display.fillRect(0, 0, display.width(), display.height(), WHITE);
    
    printCleanText(u8g2, "=== NETWORK MODE ===", MARGIN_X, MARGIN_Y, true);
    
    String options[] = {"1. USB (SD Reader Mode)", "2. Bluetooth (HID/Send)", "3. Wi-Fi (Email Send)"};
    for(int i=0; i<3; i++) {
        int ty = MARGIN_Y + 40 + (i * 30);
        if (i == selectedIdx) {
            // 선택된 항목 강조 (반전)
            display.fillRect((int)((MARGIN_X-4)*scale), (int)((ty-16)*scale), (int)(300*scale), (int)(22*scale), BLACK);
            u8g2.setForegroundColor(WHITE); u8g2.setBackgroundColor(BLACK);
            printCleanText(u8g2, options[i], MARGIN_X, ty, true);
            u8g2.setForegroundColor(BLACK); u8g2.setBackgroundColor(WHITE);
        } else {
            printCleanText(u8g2, options[i], MARGIN_X, ty, true);
        }
    }
    display.partialUpdate();
}

#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

// 와이파이 접속 정보 (사장님 폰으로 접속할 이름)
const char* ssid = "RupertWriter_FileServer";
const char* password = "00009888";



void handleRoot() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    // 한글 깨짐 방지를 위해 charset=utf-8을 반드시 명시합니다.
    //server.send(200, "text/html; charset=utf-8", "<html><head><style>body{font-family:sans-serif;padding:20px;} li{margin-bottom:15px;} .pv{color:#666;font-size:0.9em;display:block;}</style></head><body><h1>ZeroWriter Documents</h1><ul>");
    server.send(200, "text/html; charset=utf-8", "<html><head><style>body{font-family:sans-serif;padding:20px;} li{margin-bottom:15px;} .pv{color:#666;font-size:0.9em;display:block;} .del{color:red;margin-left:15px;text-decoration:none;font-size:0.8em;}</style></head><body><h1>ZeroWriter Documents</h1><ul>");
    SdFile root;
    if (root.open("/", O_RDONLY)) {
        SdFile file;
        while (file.openNext(&root, O_RDONLY)) {
            char name[64];
            file.getName(name, 64);
            String fn = String(name);
            
            if (fn.endsWith(".txt")) {
                // 미리보기용으로 앞 120바이트만 읽습니다.
                char buf[121];
                int n = file.read(buf, 120);
                if (n > 0) buf[n] = '\0'; else buf[0] = '\0';
                String preview = String(buf);
                preview.replace("\n", " "); // 줄바꿈 제거

                //String row = "<li><a href='/download?file=" + fn + "'><b>" + fn + "</b></a>";
                //row += "<span class='pv'>" + preview + "...</span></li>";

                String row = "<li><a href='/download?file=" + fn + "'><b>" + fn + "</b></a>";
row += " <a href='/delete?file=" + fn + "' class='del' onclick=\"return confirm('정말 삭제할까요?')\">[삭제]</a>"; // 삭제 버튼 추가
row += "<span class='pv'>" + preview + "...</span></li>";
                server.sendContent(row);
            }
            file.close();
            yield();
        }
        root.close();
    }
    server.sendContent("</ul></body></html>");
    server.sendContent("");
}

void handleDownload() {
    String fn = server.arg("file");
    SdFile file;
    
    // 1. 파일 열기 확인
    if (!file.open(fn.c_str(), O_RDONLY)) {
        server.send(404, "text/plain", "File Not Found");
        return;
    }

    // 2. 헤더 전송 (한글 깨짐 방지 및 파일 크기 명시)
    // SdFile은 size() 대신 fileSize()를 사용합니다.
    uint32_t fileSize = file.fileSize();
    server.setContentLength(fileSize);
    server.send(200, "text/plain; charset=utf-8", "");

    // 3. [에러 해결 핵심] 파일을 직접 읽어서 스트리밍 전송
    uint8_t buffer[512]; // 512바이트씩 잘라서 전송
    while (file.available()) {
        int bytesRead = file.read(buffer, sizeof(buffer));
        if (bytesRead > 0) {
            server.sendContent((char*)buffer, bytesRead);
        }
        yield(); // 워치독 재부팅 방지
    }
    
    server.sendContent(""); // 전송 종료
    file.close();
    Serial.println("Streaming Complete: " + fn);
}

void handleDelete() {
    String fn = server.arg("file");
    SdFile file;
    // 사장님 코드 스타일대로 파일을 열어서 삭제를 실행합니다.
    if (file.open(fn.c_str(), O_RDWR)) { 
        if (file.remove()) {
            // 삭제 성공 시 메인 화면으로 돌아갑니다.
            server.sendHeader("Location", "/");
            server.send(303);
            return;
        }
    }
    server.send(500, "text/plain", "Delete Failed");
}

#endif