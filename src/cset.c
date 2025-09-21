/*
 * cset - 標準入力からテキストを読み取りクリップボードに書き込み
 * 
 * Copyright (c) 2025 thrashem
 * Released under the MIT License
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>

#define CHUNK_SIZE 8192

void show_usage() {
    printf("cset - Read text from stdin and write to clipboard\n");
    printf("Copyright (c) 2025 thrashem\n\n");
    printf("Usage:\n");
    printf("  cset                 Read text from stdin\n");
    printf("  cset -h, --help, /?  Show this help\n\n");
    printf("Examples:\n");
    printf("  echo \"Hello\" | cset      Set string to clipboard\n");
    printf("  type file.txt | cset      Set file content to clipboard\n");
    printf("  cget | findstr ABC | cset Filter and return to clipboard\n");
    printf("  dir | cset               Set directory list to clipboard\n\n");
    printf("Exit codes:\n");
    printf("  0  Success\n");
    printf("  1  Memory allocation failed\n");
    printf("  2  No stdin input\n");
    printf("  3  Character conversion failed\n");
    printf("  4  Cannot open clipboard\n");
    printf("  5-7 Clipboard operation failed\n");
}

int main(int argc, char* argv[]) {
    // ヘルプオプションのチェック
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || 
            strcmp(argv[1], "--help") == 0 || 
            strcmp(argv[1], "/?") == 0) {
            show_usage();
            return 0;
        }
    }
    // 現在のコンソールコードページを取得
    UINT consoleCP = GetConsoleCP();
    
    char* buffer = NULL;
    size_t totalSize = 0;
    size_t bufferCapacity = CHUNK_SIZE;
    
    // 初期バッファを確保
    buffer = (char*)malloc(bufferCapacity);
    if (buffer == NULL) {
        return 1; // エラー：メモリ確保失敗
    }
    
    // 標準入力から全データを読み取り
    char tempBuffer[CHUNK_SIZE];
    size_t bytesRead;
    
    while ((bytesRead = fread(tempBuffer, 1, CHUNK_SIZE, stdin)) > 0) {
        // バッファサイズが足りない場合は拡張
        if (totalSize + bytesRead >= bufferCapacity) {
            bufferCapacity = (totalSize + bytesRead + CHUNK_SIZE) * 2;
            char* newBuffer = (char*)realloc(buffer, bufferCapacity);
            if (newBuffer == NULL) {
                free(buffer);
                return 1; // エラー：メモリ確保失敗
            }
            buffer = newBuffer;
        }
        
        memcpy(buffer + totalSize, tempBuffer, bytesRead);
        totalSize += bytesRead;
    }
    
    // 入力がない場合
    if (totalSize == 0) {
        free(buffer);
        return 2; // エラー：入力なし
    }
    
    buffer[totalSize] = '\0'; // null終端

    // 入力文字コードからUTF-16に変換
    wchar_t* wideBuffer = NULL;
    int wideLen = 0;
    
    if (consoleCP == 65001) { // UTF-8入力
        wideLen = MultiByteToWideChar(CP_UTF8, 0, buffer, -1, NULL, 0);
    } else { // コンソールコードページ（SJIS等）入力
        wideLen = MultiByteToWideChar(consoleCP, 0, buffer, -1, NULL, 0);
    }
    
    if (wideLen <= 0) {
        free(buffer);
        return 3; // エラー：文字コード変換失敗
    }

    wideBuffer = (wchar_t*)malloc(wideLen * sizeof(wchar_t));
    if (wideBuffer == NULL) {
        free(buffer);
        return 1; // エラー：メモリ確保失敗
    }

    if (consoleCP == 65001) { // UTF-8入力
        MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wideBuffer, wideLen);
    } else { // コンソールコードページ入力
        MultiByteToWideChar(consoleCP, 0, buffer, -1, wideBuffer, wideLen);
    }
    
    free(buffer);

    // クリップボードを開く
    if (!OpenClipboard(NULL)) {
        free(wideBuffer);
        return 4; // エラー：クリップボードを開けない
    }

    // クリップボードをクリア
    EmptyClipboard();

    // Unicodeデータ用メモリを確保
    HANDLE hMemUnicode = GlobalAlloc(GMEM_MOVEABLE, wideLen * sizeof(wchar_t));
    if (hMemUnicode == NULL) {
        CloseClipboard();
        free(wideBuffer);
        return 5; // エラー：メモリ確保失敗
    }

    // Unicodeデータをコピー
    wchar_t* pMemUnicode = (wchar_t*)GlobalLock(hMemUnicode);
    if (pMemUnicode == NULL) {
        GlobalFree(hMemUnicode);
        CloseClipboard();
        free(wideBuffer);
        return 6; // エラー：メモリロック失敗
    }

    memcpy(pMemUnicode, wideBuffer, wideLen * sizeof(wchar_t));
    GlobalUnlock(hMemUnicode);

    // ANSI版も作成（互換性のため）
    int ansiLen = WideCharToMultiByte(CP_ACP, 0, wideBuffer, -1, NULL, 0, NULL, NULL);
    HANDLE hMemAnsi = NULL;
    
    if (ansiLen > 0) {
        hMemAnsi = GlobalAlloc(GMEM_MOVEABLE, ansiLen);
        if (hMemAnsi != NULL) {
            char* pMemAnsi = (char*)GlobalLock(hMemAnsi);
            if (pMemAnsi != NULL) {
                WideCharToMultiByte(CP_ACP, 0, wideBuffer, -1, pMemAnsi, ansiLen, NULL, NULL);
                GlobalUnlock(hMemAnsi);
                SetClipboardData(CF_TEXT, hMemAnsi);
            }
        }
    }

    free(wideBuffer);

    // クリップボードにUnicodeデータを設定
    if (SetClipboardData(CF_UNICODETEXT, hMemUnicode) == NULL) {
        GlobalFree(hMemUnicode);
        if (hMemAnsi) GlobalFree(hMemAnsi);
        CloseClipboard();
        return 7; // エラー：データ設定失敗
    }

    CloseClipboard();
    return 0; // 成功
}