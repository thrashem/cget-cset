/*
 * cget - クリップボードからテキストを読み取り標準出力に出力
 * 
 * Copyright (c) 2025 thrashem
 * Released under the MIT License
 */

#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>

void show_usage() {
    printf("cget - Read text from clipboard and output to stdout\n");
    printf("Copyright (c) 2025 thrashem\n\n");
    printf("Usage:\n");
    printf("  cget                 Read text from clipboard\n");
    printf("  cget -h, --help, /?  Show this help\n\n");
    printf("Examples:\n");
    printf("  cget > output.txt    Save clipboard content to file\n");
    printf("  cget | grep keyword  Search for keyword\n");
    printf("  cget | findstr ABC   Search lines containing ABC\n\n");
    printf("Exit codes:\n");
    printf("  0  Success (text output)\n");
    printf("  1  Cannot open clipboard\n");
    printf("  2  No text data\n");
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
    UINT consoleCP = GetConsoleOutputCP();
    
    // クリップボードを開く
    if (!OpenClipboard(NULL)) {
        return 1; // エラー：クリップボードを開けない
    }

    HANDLE hData = NULL;
    wchar_t* pszText = NULL;

    // まずUnicodeテキストを試す
    if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        hData = GetClipboardData(CF_UNICODETEXT);
        if (hData != NULL) {
            pszText = (wchar_t*)GlobalLock(hData);
            if (pszText != NULL) {
                // コンソールのコードページに応じて出力方法を変更
                if (consoleCP == 65001) { // UTF-8
                    _setmode(_fileno(stdout), _O_U8TEXT);
                    wprintf(L"%s", pszText);
                } else { // SJIS等のマルチバイト
                    // UTF-16からコンソールコードページに変換
                    int mbLen = WideCharToMultiByte(consoleCP, 0, pszText, -1, NULL, 0, NULL, NULL);
                    if (mbLen > 0) {
                        char* mbText = (char*)malloc(mbLen);
                        if (mbText != NULL) {
                            WideCharToMultiByte(consoleCP, 0, pszText, -1, mbText, mbLen, NULL, NULL);
                            printf("%s", mbText);
                            free(mbText);
                        }
                    }
                }
                GlobalUnlock(hData);
                CloseClipboard();
                return 0;
            }
        }
    }

    // UnicodeがダメならANSIテキストを試す
    if (IsClipboardFormatAvailable(CF_TEXT)) {
        hData = GetClipboardData(CF_TEXT);
        if (hData != NULL) {
            char* pszAnsiText = (char*)GlobalLock(hData);
            if (pszAnsiText != NULL) {
                printf("%s", pszAnsiText);
                GlobalUnlock(hData);
                CloseClipboard();
                return 0;
            }
        }
    }

    CloseClipboard();
    return 2; // エラー：テキストデータがない
}