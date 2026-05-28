#ifndef PSRAM_ASSETS_H
#define PSRAM_ASSETS_H

#include <Arduino.h>
#include <SdFat.h>

uint8_t* g_fontData = nullptr;
uint32_t g_fontSize = 0;
uint8_t* g_imgBuffer = nullptr;
uint32_t g_imgSize = 0;

static uint8_t* loadFileToPsram(const char* path, uint32_t& outSize) {
    SdFile f;
    if (!f.open(path, O_RDONLY)) {
        return nullptr;
    }

    uint32_t sz = f.fileSize();
    if (sz == 0) {
        f.close();
        return nullptr;
    }

    uint8_t* buf = (uint8_t*)malloc(sz);
    if (!buf) {
        f.close();
        return nullptr;
    }

    uint32_t bytesRead = 0;
    uint8_t chunk[512];
    while (bytesRead < sz) {
        uint32_t toRead = min((uint32_t)sizeof(chunk), sz - bytesRead);
        int n = f.read(chunk, toRead);
        if (n <= 0) break;
        memcpy(buf + bytesRead, chunk, n);
        bytesRead += n;
        yield();
    }

    f.close();
    if (bytesRead != sz) {
        free(buf);
        return nullptr;
    }

    outSize = sz;
    return buf;
}

void loadAssetsToRAM() {
    g_imgBuffer = loadFileToPsram("/ize_compose/initial.png", g_imgSize);
}

void reloadAssetsToRAM() {
    if (g_fontData) {
        free(g_fontData);
        g_fontData = nullptr;
        g_fontSize = 0;
    }
    if (g_imgBuffer) {
        free(g_imgBuffer);
        g_imgBuffer = nullptr;
        g_imgSize = 0;
    }
    loadAssetsToRAM();
}

#endif
