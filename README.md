# 🖋️ Zerowriter-Kor-Rupert-V1.2
> **The First Hangeul(Korean) Support Firmware for Zerowriter Ink**  

# 제로라이터 잉크 한글버전 자작 커스텀 펌웨어  
**zerowriter ink korean DIY Custom Firmware**  ![demo](https://private-user-images.githubusercontent.com/7774762/590768353-d1d9cab8-329b-4d52-aeaf-bdd33d0c5bb4.gif?jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3Nzg1NDU2NDEsIm5iZiI6MTc3ODU0NTM0MSwicGF0aCI6Ii83Nzc0NzYyLzU5MDc2ODM1My1kMWQ5Y2FiOC0zMjliLTRkNTItYWVhZi1iZGQzM2QwYzViYjQuZ2lmP1gtQW16LUFsZ29yaXRobT1BV1M0LUhNQUMtU0hBMjU2JlgtQW16LUNyZWRlbnRpYWw9QUtJQVZDT0RZTFNBNTNQUUs0WkElMkYyMDI2MDUxMiUyRnVzLWVhc3QtMSUyRnMzJTJGYXdzNF9yZXF1ZXN0JlgtQW16LURhdGU9MjAyNjA1MTJUMDAyMjIxWiZYLUFtei1FeHBpcmVzPTMwMCZYLUFtei1TaWduYXR1cmU9MzIwMWQ1MmIyNWJhNDVkZTNkZGY1MDExN2MyY2RhODI0NGU4ZjQwOGNmYTJhMmNkMDVmNWFkYWQzZGI0MjhkNyZYLUFtei1TaWduZWRIZWFkZXJzPWhvc3QmcmVzcG9uc2UtY29udGVudC10eXBlPWltYWdlJTJGZ2lmIn0.MhEXwwW-tgQ6WudJDrKdJTv0kJrYqf5gNFY6o5ZWyEI)  
## ✨ Overview
* **Vintage Aesthetics:** Inspired by the **Hermes Baby** typewriter.
* **Hangeul Engine:** Perfectly optimized Korean character composition.
* **Smart Features:** Bluetooth keyboard mode & WiFi file management.  
​> **English users:** Please scroll down for the feature list or use a translator for the detailed guide below!  

[github source code & release]( https://github.com/Dievesa/zerowriter/ )   
  
![Korean Menu]( https://raw.githubusercontent.com/Dievesa/zerowriter/main/image/zerowriter-korean-menu.jpg )  
![English Menu]( 
https://raw.githubusercontent.com/Dievesa/zerowriter/main/image/zerowriter-english-menu-fullview.jpg )  
    
글씨체font는 [둥근모꼴]( https://cactus.tistory.com/193 )입니다.

1. 초기화면sleep image은 SD card에 'initial.png'로 넣으면 됩니다. 잠자기 모드에서 볼 수 있습니다.  
  
2. ctrl 단축키는 다음과 같습니다(ctrl shortcut).  
  C : 전체 복사 copy all  
  V : 붙여넣기 paste(전체 복사와 붙여넣기는, 따로 문서 복사 기능이 없어서 그렇습니다...)  
  L : 잠자기 sleep(initial.png파일의 그림 띄우기)  
  R : e-ink 새로고침 refresh screen  
  스페이스바space bar: 한영전환 en-kor conversion  
  N : 새로 만들기 new file (Ctrl+C 상태로 Ctrl+N을 한 후 Ctrl+V를 하면 똑같은 문서를 만들 수 있습니다.)  
  S : 현재 문서 저장하기 save. 중간중간 반드시 저장해 주세요. 자동저장 기능은 1.0 버전에는 없습니다.  
  F : 현재 문서에서 찾기 find  
  좌우화살표 left right : 단어 단위로 앞뒤 이동 move by word  
  상하화살표 up down : 글의 맨 앞과 맨 뒤로 이동 beginning end  
  백스페이스 backspace: 단어 단위로 삭제 del by word  
  
3. 네트워크 network  
  Bluetooth : 메뉴menu의 네트워크netwotk에서 엔터enter를 눌러서 BLE 선택한 후 Rupertwriter로 검색해서 연결하면 블루투스 외장 키보드로 사용할 수 있습니다.  zerowriter works as an external keyboard  
  wifi : 메뉴menu의 네트워크network에서 엔터enter를 눌러서 WIFI 선택한 후 Rupertwriter로 검색해서 연결(password 00009888)하면 파일을 볼 수 있습니다. 미리보기 및 파일삭제 기능이 있습니다. 문서 각각의 편집은 불가능합니다. 링크를 클릭한 후 조회한 파일보기 화면에서 복사해서 다른 앱에서 사용할 수 있습니다.  
    wifi 접속 후 http://rupertwriter.local 또는 192.168.4.1 접속  

![Korean writing]( 
https://raw.githubusercontent.com/Dievesa/zerowriter/main/image/zerowriter-korean-writing.jpg )
  
**현재 구동 후 눈에 띄는 버그는 없는 상태입니다. 그러나 긴 글을 작성하거나 파일 개수가 많아지면 어떻게 될지 모르니 반드시 중간중간 백업을 하시거나 테스트용 혹은 메모용으로 사용해 주세요.**  
  
**펌웨어를 넣을 때는 https://zerowriter.ink/pages/firmware-updates 에서 보이는 것처럼, 우선 키보드 케이블을 분리해야 합니다.**  
  
https://adafruit.github.io/Adafruit_WebSerial_ESPTool/ 에 접속 후, 맨 위 우측에서 속도를 921600으로 맞춘 후 연결합니다.  
주소는 0x부터 넣으면 됩니다. 파일을 선택하고 Program을 쿨러 전송을 실행합니다. 업로드가 끝나면 껐다 켜면 적용됩니다.
  
SD카드는 반드시 FAT32로 포맷되어 있어야 한다고 합니다. 에러가 나면 포맷을 윈도우 기본 포매터가 아니라 https://www.sdcard.org/downloads/formatter/ 에서 다운받아 설치한 포매터를 사용해서 포맷해 보세요.
보드의 라이브러리 특성상 32기가 이하의 메모리를 사용하라고 합니다.  
  

## 📋 Features (English Summary)

* **Full Hangeul Support:** Real-time Korean character composition (Jamo-combination) using optimized **D2Coding** fonts.
* **Hermes Baby Aesthetics:** Minimalist and vintage UI design for a focused writing experience.
* **Smart Editing Shortcuts:** Supports essential shortcuts like `Ctrl+C`, `Ctrl+V`, and word-level navigation.
* **Dual Connectivity:** * **BLE Mode:** Use Zerowriter as a high-quality external Bluetooth keyboard.
    * **WiFi Mode:** Wireless file management and preview via web browser (`192.168.4.1`).
* **Stability:** Optimized firmware with reliable performance for daily drafting and note-taking.

This project was built from scratch with the help of Gemini, driven by the desire for a better Korean writing environment.:-)
![Gemini AI made image]( 
https://raw.githubusercontent.com/Dievesa/zerowriter/main/image/zerowriter-gemini-chrome.png )

