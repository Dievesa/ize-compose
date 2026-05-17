# 🖋️ Zerowriter: Rupert Edition v2.0.0
> **"The Ultimate Hardware-Level Typing Experience"**

---

## 🌍 Physical Layout Remapping (Firmware-Level)

Rupert Edition v2.0.0 solves the structural limitations of the original firmware. We have remapped the input matrix so that the machine behaves as a native device. **Swap your physical keycaps, and the firmware follows.** 
(To select your keyboard, go to submenu(System Set) and select your language at the *Latin* section.

### **French (AZERTY) - Specialized Mapping**
Full native AZERTY physical matrix mapping. Exact placements for `é, è, ç, à, ù` and all localized punctuation marks.
| Row | Key Layout |
|:---:|:---|
| **1** | <kbd>²</kbd> <kbd>&</kbd> <kbd>é</kbd> <kbd>"</kbd> <kbd>'</kbd> <kbd>(</kbd> <kbd>-</kbd> <kbd>è</kbd> <kbd>_</kbd> <kbd>ç</kbd> <kbd>à</kbd> <kbd>)</kbd> <kbd>=</kbd> |
| **2** | &nbsp;&nbsp;<kbd>A</kbd> <kbd>Z</kbd> <kbd>E</kbd> <kbd>R</kbd> <kbd>T</kbd> <kbd>Y</kbd> <kbd>U</kbd> <kbd>I</kbd> <kbd>O</kbd> <kbd>P</kbd> <kbd>^</kbd> <kbd>$</kbd> <kbd>*</kbd> |
| **3** | &nbsp;&nbsp;&nbsp;<kbd>Q</kbd> <kbd>S</kbd> <kbd>D</kbd> <kbd>F</kbd> <kbd>G</kbd> <kbd>H</kbd> <kbd>J</kbd> <kbd>K</kbd> <kbd>L</kbd> <kbd>M</kbd> <kbd>ù</kbd> |
| **4** | &nbsp;&nbsp;&nbsp;&nbsp;<kbd>W</kbd> <kbd>X</kbd> <kbd>C</kbd> <kbd>V</kbd> <kbd>B</kbd> <kbd>N</kbd> <kbd>,</kbd> <kbd>;</kbd> <kbd>:</kbd> <kbd>!</kbd> |

### **German (QWERTZ) - Standard Precision**
True 1:1 hardware mapping. Automatically swaps `Z` and `Y`. Natively supports `ä, ö, ü, ß` on their exact local key positions.
| Row | Key Layout |
|:---:|:---|
| **1** | <kbd>^</kbd> <kbd>1</kbd> <kbd>2</kbd> <kbd>3</kbd> <kbd>4</kbd> <kbd>5</kbd> <kbd>6</kbd> <kbd>7</kbd> <kbd>8</kbd> <kbd>9</kbd> <kbd>0</kbd> <kbd>ß</kbd> <kbd>´</kbd> |
| **2** | &nbsp;&nbsp;<kbd>Q</kbd> <kbd>W</kbd> <kbd>E</kbd> <kbd>R</kbd> <kbd>T</kbd> <kbd>Z</kbd> <kbd>U</kbd> <kbd>I</kbd> <kbd>O</kbd> <kbd>P</kbd> <kbd>Ü</kbd> <kbd>+</kbd> <kbd>#</kbd> |
| **3** | &nbsp;&nbsp;&nbsp;<kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> <kbd>F</kbd> <kbd>G</kbd> <kbd>H</kbd> <kbd>J</kbd> <kbd>K</kbd> <kbd>L</kbd> <kbd>Ö</kbd> <kbd>Ä</kbd> |
| **4** | &nbsp;&nbsp;&nbsp;&nbsp;<kbd>Y</kbd> <kbd>X</kbd> <kbd>C</kbd> <kbd>V</kbd> <kbd>B</kbd> <kbd>N</kbd> <kbd>M</kbd> <kbd>,</kbd> <kbd>.</kbd> <kbd>-</kbd> |

### **Turkish (Turkish-Q) - Native Implementation**
Hardware-level remapping for dedicated Turkish keys: `ğ, ü, ş, i, ı, ö, ç`.
| Row | Key Layout |
|:---:|:---|
| **1** | <kbd>"</kbd> <kbd>1</kbd> <kbd>2</kbd> <kbd>3</kbd> <kbd>4</kbd> <kbd>5</kbd> <kbd>6</kbd> <kbd>7</kbd> <kbd>8</kbd> <kbd>9</kbd> <kbd>0</kbd> <kbd>*</kbd> <kbd>-</kbd> |
| **2** | &nbsp;&nbsp;<kbd>Q</kbd> <kbd>W</kbd> <kbd>E</kbd> <kbd>R</kbd> <kbd>T</kbd> <kbd>Y</kbd> <kbd>U</kbd> <kbd>I</kbd> <kbd>O</kbd> <kbd>P</kbd> <kbd>Ğ</kbd> <kbd>Ü</kbd> <kbd>,</kbd> |
| **3** | &nbsp;&nbsp;&nbsp;<kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> <kbd>F</kbd> <kbd>G</kbd> <kbd>H</kbd> <kbd>J</kbd> <kbd>K</kbd> <kbd>L</kbd> <kbd>Ş</kbd> <kbd>İ</kbd> |
| **4** | &nbsp;&nbsp;&nbsp;&nbsp;<kbd><</kbd> <kbd>Z</kbd> <kbd>X</kbd> <kbd>C</kbd> <kbd>V</kbd> <kbd>B</kbd> <kbd>N</kbd> <kbd>M</kbd> <kbd>Ö</kbd> <kbd>Ç</kbd> <kbd>.</kbd> |

### **Montenegrin (QWERTZ) - Balkan Layout**
Native Balkan QWERTZ mapping with direct physical key support for `š, đ, č, ć, ž`.
| Row | Key Layout |
|:---:|:---|
| **1** | <kbd>'</kbd> <kbd>1</kbd> <kbd>2</kbd> <kbd>3</kbd> <kbd>4</kbd> <kbd>5</kbd> <kbd>6</kbd> <kbd>7</kbd> <kbd>8</kbd> <kbd>9</kbd> <kbd>0</kbd> <kbd>+</kbd> |
| **2** | &nbsp;&nbsp;<kbd>Q</kbd> <kbd>W</kbd> <kbd>E</kbd> <kbd>R</kbd> <kbd>T</kbd> <kbd>Z</kbd> <kbd>U</kbd> <kbd>I</kbd> <kbd>O</kbd> <kbd>P</kbd> <kbd>Š</kbd> <kbd>Đ</kbd> <kbd>Ž</kbd> |
| **3** | &nbsp;&nbsp;&nbsp;<kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> <kbd>F</kbd> <kbd>G</kbd> <kbd>H</kbd> <kbd>J</kbd> <kbd>K</kbd> <kbd>L</kbd> <kbd>Č</kbd> <kbd>Ć</kbd> |
| **4** | &nbsp;&nbsp;&nbsp;&nbsp;<kbd><</kbd> <kbd>Y</kbd> <kbd>X</kbd> <kbd>C</kbd> <kbd>V</kbd> <kbd>B</kbd> <kbd>N</kbd> <kbd>M</kbd> <kbd>,</kbd> <kbd>.</kbd> <kbd>-</kbd> |

### 🇪🇺 Universal Europe Mode (For Standard QWERTY Users)
If you prefer to keep your standard US QWERTY physical keycaps, use my built-in **EU Cycle Mode**.
Within 3 seconds of typing a base character, press <kbd>Alt</kbd> repeatedly to seamlessly cycle through its accent variations:
* `a` / `A` : á, à, â, ä, ã, æ, a
* `e` / `E` : é, è, ê, ë, e
* `i` / `I` : í, ì, î, ï, ı, i
* `o` / `O` : ó, ò, ô, ö, õ, œ, o
* `u` / `U` : ú, ù, û, ü, u
* `c` / `C` : ç, ć, č, c
* `n` / `N` : ñ, n
* `s` / `S` : ß, š, ś, ş, s
* `d` / `D` : đ, d
* `z` / `Z` : ž, ź, z
* `g` / `G` : ğ, g

---

## 🚀 Technical Breakouts

### **1. 2-Byte Encoding Integration**
Original Zerowriter layouts were limited by 1-byte encoding, making many European characters impossible to process. We have implemented a full UTF-8 compatible engine, ensuring every special character is processed natively without glitches.

### **2. Asynchronous Dual-Core Processing**
* **Core 1:** High-priority keyboard interrupts & 1-bit E-ink rendering.
* **Core 0:** Real-time character/word count background processing. 
* *Experience zero latency even when working on book-length manuscripts.*

### **3. Standalone Wireless File Server (No Cables Needed)**
Rupert Edition completely frees you from cables. The device operates as its own WiFi Access Point (`192.168.4.1` or `rupertwriter.local`). Simply connect your phone or PC to the device's network, open a web browser, and you can instantly download or delete your `.txt` drafts through a clean web interface.

### **4. Universal BLE Text Dumping**
Need your text on another device instantly? Connect RupertWriter as a standard Bluetooth Keyboard to your smartphone or laptop. With the 'Safe BLE Transfer' function, the device will automatically "type out" your entire manuscript directly into any app (Notepad, Word, Email). We've even implemented a micro-delay for specific symbols (`/ \ - =`) to ensure 100% typo-free transmission across different operating systems.

### **5. Intelligent Dual-Font Typography Engine**
Rendering different languages perfectly on a low-refresh E-ink screen is notoriously difficult. My custom engine parses characters byte-by-byte in real time. It applies a rigid monospaced 16px Typewriter font for 3-byte characters (Korean), while seamlessly swapping to a proportional Unifont for 1-to-2-byte characters (Latin, Symbols, Accents). The result? Flawless layout and alignment, no matter how many languages you mix in a single sentence.

### **6. E-ink Optimized Visual Search**
Press <kbd>Ctrl</kbd> + <kbd>F</kbd> to open the built-in search bar. When navigating through search results, the engine physically calculates the exact hardware matrix position of the target word on the E-ink screen. It then renders an inverted black box with white text directly over the word, providing a crystal-clear visual highlight without forcing a full screen refresh.

### **7. Hardware-Level Battery Protection & Persistent NVS**
All your preferences (Keyboard Layout, Sleep Timers, Caps Lock status) are saved to Non-Volatile Storage (NVS) and survive reboots. To protect your battery life, if the voltage drops below 3.5V, the system flashes a hardware warning and enters a deep-sleep protection mode. It can only be awakened via a direct `ext0` hardware interrupt from the physical keyboard matrix.
---

## 🛠️ Maintenance & Safety (OTA)
Updates are delivered over-the-air with **Signature Verification**. The device only accepts firmware containing the official `RUPERT_OFFICIAL_KOR` signature to prevent system corruption.

---

## ⌨️ Hotkeys (Hardware Mapped)

Rupert Edition uses intuitive shortcuts mapped directly to your mechanical keyboard to keep your writing flow uninterrupted.

| Function | Shortcut | Description |
| :--- | :--- | :--- |
| **New Document** | <kbd>Ctrl</kbd> + <kbd>N</kbd> | Opens a clean, new document (`doc_X.txt`). |
| **Save** | <kbd>Ctrl</kbd> + <kbd>S</kbd> | Safely saves the current draft to internal storage. |
| **Copy / Paste** | <kbd>Ctrl</kbd> + <kbd>C</kbd> / <kbd>V</kbd> | Copies the entire text to the clipboard and pastes it. |
| **Search (Find)** | <kbd>Ctrl</kbd> + <kbd>F</kbd> | Opens the search bar. Press `Enter` to jump to the next word. |
| **Force Sleep** | <kbd>Ctrl</kbd> + <kbd>L</kbd> | Instantly locks the screen and enters deep sleep mode. |
| **Hard Refresh** | <kbd>Ctrl</kbd> + <kbd>R</kbd> | Clears E-ink ghosting and refreshes the screen manually. |
| **Language Toggle**| <kbd>Ctrl</kbd> + <kbd>Space</kbd> | Toggles between Korean and English input modes. |
| **Open Menu** | <kbd>Menu Key</kbd> | Opens the file list and system settings menu. |
| **Exit Network** | <kbd>Ctrl</kbd> + <kbd>Menu Key</kbd>| Instantly turns off WiFi/Bluetooth and returns to typing. |
| **Word / Para Jump** | <kbd>Ctrl</kbd> + <kbd>Arrows</kbd>| Quickly navigate through words or jump to the end/beginning. |

---

## 💾 Installation

**The Easiest Way (Binary Flashing):**
1. Navigate to the **[Releases]** tab.
2. Download the latest `rupertwriter.bin` file.
3. Open the device case and disconnect the keyboard cable connected behind the display.
4. Flash the `.bin` file using an ESP32 Web Flasher tool or your device's existing OTA feature.

5. 🌐 Web Server Connection
Search for and connect to the Wi-Fi network named Rupertwriter (Password: 00009888).
Open a web browser on your connected device and go to 192.168.4.1.

💾 Firmware Installation via Web Flasher
Visit the Adafruit WebSerial ESPTool(https://adafruit.github.io/Adafruit_WebSerial_ESPTool/).
In the top-right corner, set the baud rate to 921600 and click Connect.
Enter the target address starting with 0x, select your firmware file, and click Program to start the flashing process.
Once the upload is complete, reboot the device to apply the changes.
