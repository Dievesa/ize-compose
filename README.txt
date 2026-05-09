제로라이터 잉크(zerowriter ink) 
한글버전 자작 커스텀 펌웨어 
DIY Custom Firmware

글자를 치던 중 글자가 완성되지 않은 상태(예: ㄱ만 친 경우)에서 화살표로 커서 이동 시 자음 등이 따라오는 현상이 있습니다. 글자를 다 치기 전에는 반드시 스페이스바를 누른 후 이동해 주세요. 자모음 분리하여 보이게 하다보니 나온 현상인데, 글자가 다 만들어지고 나서 보이게 할지 중간에도 보이게 할지 중 선택하다보니 나온 현상입니다.
글씨체는 [둥근모꼴](https://cactus.tistory.com/193)입니다.

1. 초기화면은 SD카드에 'initial.png'로 넣으면 됩니다. 잠자기 모드에서 볼 수 있습니다. 
2. 단축키는 다음과 같습니다(ctrl을 누른 상태에서).
  C : 전체 복사
  V : 붙여넣기(전체 복사와 붙여넣기는, 따로 문서 복사 기능이 없어서 그렇습니다...)
  L : 잠자기(initial.png파일의 그림 띄우기)
  R : e-ink 새로고침
  N : 새로 만들기(Ctrl+C 상태로 Ctrl+N을 한 후 Ctrl+V를 하면 똑같은 문서를 만들 수 있습니다.)
  S : 현재 문서 저장하기
  F : 현재 문서에서 찾기
  좌우화살표 : 해당 줄의 맨 앞과 맨 뒤
  상하화살표 : 문단의 맨 앞과 맨 뒤
3. 네트워크 사용법은 다음과 같습니다.
  BLE : Rupertwriter로 검색 후 연결하면 블루투스 외장 키보드로 사용할 수 있습니다.
  wifi : Rupertwriter로 검색 후 연결하면 파일을 볼 수 있습니다. 미리보기 및 파일삭제 기능이 있습니다. 문서 각각의 편집은 불가능합니다. 미리보기화면에서 복사해서 다른 앱에서 사용할 수 있습니다.

**현재 구동 후 눈에 띄는 버그만 없는 상태입니다. 긴 글을 작성하거나 파일 개수가 많아지면 어떻게 될지 모르니 테스트용 혹은 메모용으로만 사용해 주세요.**

**넣을 때는 https://zerowriter.ink/pages/firmware-updates 에서 보이는 것처럼, 우선 키보드 케이블을 분리해야 합니다.** 

https://adafruit.github.io/Adafruit_WebSerial_ESPTool/ 에 접속 후, 맨 위 우측에서 속도를 921600으로 맞춘 후 연결합니다.
주소는 0x부터 넣으면 됩니다. 파일을 선택하고 Program을 실행합니다. 업로드가 끝나면 껐다 켜면 적용됩니다.

SD카드는 반드시 FAT32로 포맷되어 있어야 한다고 합니다(저도 모릅니다). 에러가 나면 포맷을 윈도우 기본 포매터가 아니라 https://www.sdcard.org/downloads/formatter/ 에서 다운받아 설치한 포매터를 사용해서 포맷해 보세요.
보드의 라이브러리 특성상 32기가 이하의 메모리를 사용하라고 합니다.

This project was built from scratch with the help of Gemini, driven by the desire for a better Korean writing environment.:-)
