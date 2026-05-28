# Ize Compose
Ize comes from the Korean word ije (이제): now, from this moment on, and the near future ahead.

### Ize Compose — Multilingual firmware for focused writing on e-paper hardware.

> [!NOTE]
> **v1.1.0 is available.** This release enables web-based firmware updates (OTA) on Zerowriter Ink.  
> Because it changes the flash partition layout, **v1.1.0 must be installed once via USB**. Future firmware updates can then be installed through the device's web update page.


Multilingual writing firmware for the [Zerowriter Ink](https://www.zerowriter.org/) (Inkplate 5 V2). Started as a Korean-input firmware, now supports 92 keyboard layouts across dozens of scripts.

Built from scratch, Ize Compose transforms the Zerowriter Ink keyboard's serial key events into a Unicode-capable multilingual writing system, with script-specific input handling.

---

## Supported Device

- **Zerowriter Ink** (Inkplate 5 V2)
  - ESP32, 800×600 monochrome e-ink display
  - Requires SD card for non-Latin fonts and document storage

---

## Features

**Writing**
- Plain-text editing with cursor navigation
- Phonetic Korean composition (cho/jung/jong jamo assembly)
- Latin accent cycling (e.g., a → á → â → ã → ...)
- Right-to-left (RTL) display for Arabic-script and Hebrew layouts
- Text search (Ctrl+F)
- Copy / paste (Ctrl+C / Ctrl+V)
- Word and character count (3 display modes)

**Keyboard & Language**
- 92 keyboard layouts selectable from the system menu
- Two independent layout slots: an English slot (QWERTY or Dvorak) and a selected-language slot
- Switch between the English slot and the selected-language slot with `Ctrl+Space`
- `Alt` provides dead-key / character-conversion input for supported layouts
* The English slot supports `Alt`-key character conversion regardless of the selected layout, including variants for keys such as `s`, `i`, and `o`.
- 12 script composition engines: Korean, Arabic, Indic scripts, Thai, Myanmar, Khmer, Lao, Tibetan, Sinhala, Ethiopic, Japanese, Hebrew
- RTL layout support: Arabic, Hebrew, Kurdish (Arabic), Pashto, Persian, Urdu
- Arabic-script text is saved as logical Unicode text for compatibility on other devices

**Files**
- Saves and loads `.txt` files on SD card (`/ize_compose/`)
- File browser (up to 65 files)
- WiFi (AP mode) for uploading/downloading text files and web-based firmware updates (OTA)

**Display**
- Partial screen update for responsive typing feedback
- Configurable full-refresh threshold
- Boot/sleep image loaded from `/ize_compose/initial.png` on SD card
- Sleep mode: Ctrl+L or sleep button; wake with wake button

**Settings (system menu)**
- Line spacing
- Typing speed (key repeat delay)
- Screen refresh limit
- Keyboard layout selection
- Font slot

---

## Keyboard Layouts (92)

Dvorak, QWERTY, 한국어, Shqip, العربية, Հայերեն, Deutsch (AT/DE/CH), Azərbaycanca, Беларуская, Nederlands (BE/NL), বাংলা, Bosanski / Босански, Português (BR/PT), Български, Français (CA/FR/CH), Català, Hrvatski, Čeština, Dansk, देवनागरी, Eesti, ኢትዮጵያ, Føroyskt, Suomi, Georgian, Ελληνικά, ગુજરાતી, Hausa, עברית, Magyar, Íslenska, Gaeilge, Italiano, 日本語, ಕನ್ನಡ, Qazaq / Қазақ, ខ្មែរ, Kurdî / کوردی, Кыргызча, ລາວ, Español América, Latviešu, Lietuvių, Lëtzebuergesch, മലയാളം, Malti, Māori, Română (MD) / Молдовеняскэ, Монгол, Crnogorski / Црногорски, မြန်မာ, नेपाली, Македонски, Norsk, پښتو, فارسی, Polski, ਪੰਜਾਬੀ, Română, Русский, Srpski / Српски, සිංහල, Slovenčina, Slovenščina, Español, Kiswahili, Svenska, Тоҷикӣ, தமிழ், తెలుగు, ไทย, བོད་སྐད, Türkçe, Українська, English UK, اردو, Oʻzbek / Ўзбек, Tiếng Việt, Cymraeg

---

## Font Files

Writing fonts are loaded from the SD card at boot. The firmware keeps only a minimal built-in fallback font for startup and error messages when an SD font is unavailable.

Place the following files in `/ize_compose/hwalja/` on the SD card:

| File | Scripts covered |
|---|---|
| `hwalja_latin.bin` | Latin and Latin extended characters |
| `hwalja_hangul.bin` | Korean (Hangul syllables) |
| `hwalja_jamo.bin` | Korean (Jamo, composition glyphs) |
| `hwalja_jp.bin` | Japanese (Hiragana, Katakana) |
| `hwalja_greek_cyrillic.bin` | Greek, Cyrillic |
| `hwalja_arabic.bin` | Arabic, Persian, Urdu, and related Arabic-script layouts |
| `hwalja_indic.bin` | Devanagari, Bengali, Gujarati, Kannada, Malayalam, Punjabi, Tamil, Telugu, Sinhala |
| `hwalja_sea.bin` | Thai, Khmer, Lao, Myanmar, Tibetan |
| `hwalja_misc.bin` | Ethiopic, Georgian, Armenian, and others |

Without the required `.bin` font files, document text may be stored correctly but may not display correctly on the device.

---

## Build Environment

### Platform / Board / Framework

| Item | Value |
|---|---|
| Platform | `espressif32` |
| Board | `esp32dev` + Inkplate 5 V2 build flags |
| Framework | Arduino |
| CPU clock | 240 MHz |
| Upload / monitor speed | 921600 baud |

> `board = esp32dev` is used with manual build flags rather than a dedicated Inkplate board definition. This firmware will not work on a generic ESP32 dev board — the flags and PSRAM are specific to the Inkplate 5 V2 hardware.

### Libraries

| Library | Version | Source |
|---|---|---|
| InkplateLibrary | 11.0.0 | `lib/` (local, no separate install needed) |
| SdFat | 2.3.1 | PlatformIO registry |
| U8g2_for_Adafruit_GFX | 1.8.0 | PlatformIO registry |
| ESP32 BLE Keyboard | 0.3.2 | PlatformIO registry |
| Adafruit GFX Library | 1.12.6 | PlatformIO registry |
| Adafruit BusIO | — | PlatformIO registry |

### Build flags

| Flag | Purpose |
|---|---|
| `-DARDUINO_INKPLATE5V2` | Board identification |
| `-DINKPLATE_5V2` | Enables correct code path inside InkplateLibrary |
| `-DBOARD_HAS_PSRAM` | Declares PSRAM presence to ESP-IDF |
| `-mfix-esp32-psram-cache-issue` | Workaround for ESP32 PSRAM cache bug (older silicon) |
| `-DSCREEN_WIDTH=800` / `-DSCREEN_HEIGHT=600` | Display resolution constants |
| `-Os` | Size optimization required to preserve room for dual-slot OTA firmware updates |
| `-D CORE_DEBUG_LEVEL=0` | Suppresses all serial debug output |

### Flash / partition settings

| Item | Value |
|---|---|
| Partition table | `min_spiffs.csv` (dual-slot OTA layout for 4 MB flash) |
| Flash speed | 80 MHz |
| Flash mode | QIO (quad I/O) |

> `min_spiffs.csv` is a built-in partition table provided by the PlatformIO espressif32 platform. It provides the two application slots required for OTA updates.

---

## Installation

### Requirements
- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- Zerowriter Ink (Inkplate 5 V2) with SD card

### Initial installation of v1.1.0 via USB-C

> **Required when upgrading from v1.0.0 or earlier:** v1.1.0 changes the flash partition layout to enable OTA. It must be installed once via USB-C. After that initial installation, later firmware releases can be installed over WiFi.

1. Open the Zerowriter Ink enclosure.
2. Disconnect the keyboard cable.
3. Connect the device to your computer using a USB-C cable.
4. Install the v1.1.0 USB build with its OTA-compatible partition layout.
5. After installation completes, disconnect the USB-C cable.
6. Reconnect the keyboard cable and close the device.
7. Insert the prepared SD card and start the device.

The standalone firmware file attached to a release is intended for web-based updates after the OTA-compatible partition layout has already been installed.

### SD card setup
1. Format SD card as FAT32.
2. Create the folder `/ize_compose/hwalja/`.
3. Copy all required `hwalja_*.bin` font files, including `hwalja_latin.bin`, into `/ize_compose/hwalja/`.
4. (Optional) Place `initial.png` (800×600 PNG) in `/ize_compose/` for the sleep/boot image.

### Firmware OTA update (WiFi)

Available after v1.1.0 has been installed once via USB.

1. Download `izefirmware.bin` from the latest GitHub Release.
2. On the device, open the system menu and select **Update**.
3. Enter a 4-digit PIN on the device, confirm it with Enter, and keep the PIN shown on screen.
4. Connect a PC or phone to the WiFi network shown on the device update screen.
5. Open `192.168.4.1` in a browser.
6. Select the downloaded `izefirmware.bin`, enter the same 4-digit PIN, and upload it.
7. The device installs the firmware and reboots when the update is complete.

The same update page can also upload supported font resource files and `initial.png`.

---

## Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| Ctrl+Space | Switch between the English slot (QWERTY/Dvorak) and the selected-language slot |
| Ctrl+L | Sleep (shows boot image) |
| Ctrl+F | Text search |
| Ctrl+C | Copy all text to internal clipboard |
| Ctrl+V | Paste internal clipboard |
| Alt | Dead-key / character-conversion input for supported layouts |

---

## Repository Structure

```
src/
  IZEcompose.ino        — main firmware
  jado.h                — keyboard layout definitions and keymaps (92 layouts)
  jeong_eum.h           — Korean composition engine and script engine types
  insoe.h               — text rendering, font selection
  EmbeddedLatinFont.h   — legacy embedded Latin font source (not used for the writing font in v1.1.0)
  PsramAssets.h         — PSRAM asset loading helpers
  hwalja_*.bin          — font binary files (copy to SD card, not compiled in)

lib/
  InkplateLibrary/      — Inkplate driver (local copy)

build/
  noto_fonts/           — source Noto font TTFs used for font building

platformio.ini          — PlatformIO build config
```

---

## Current Limitations

- Supports only Inkplate 5 V2 (800×600). Other Inkplate boards are not tested.
- Korean cursor movement during mid-syllable composition is not supported.
- Only `.txt` files; no formatting.
- Single document open at a time.
- Writing mode is unavailable while WiFi or BLE transfer mode is active.
- Arabic-script text is saved as logical Unicode, but on-device joining appearance depends on the supplied font glyphs.
- The sleep image must be exactly 800×600 pixels; other sizes are not handled.

---

## Credits

- [Inkplate Arduino Library](https://github.com/SolderedElectronics/Inkplate-Arduino-library) — Soldered Electronics
- [U8g2_for_Adafruit_GFX](https://github.com/olikraus/U8g2_for_Adafruit_GFX) — Oliver Kraus
- [ESP32 BLE Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard) — T-vK
- [SdFat](https://github.com/greiman/SdFat) — Bill Greiman
- [Noto Fonts](https://fonts.google.com/noto) — Google (used for font building; license: SIL OFL 1.1)
- [Zerowriter Ink](https://www.zerowriter.org/) — original hardware

[![Ize Compose Keyboard Layouts](https://img.youtube.com/vi/NxzNPiyAiqk/maxresdefault.jpg)](https://youtube.com/shorts/gFukSrRGRPw?feature=share)

## License

Ize Compose is made publicly available so that people who cannot access affordable writing hardware in their own language can install, use, and improve it for noncommercial purposes.

Commercial use, including selling firmware, installation services, modified versions, or hardware products incorporating Ize Compose, requires separate permission from Dievesa.
