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
#include <stdlib.h>

void show_usage() {
    fprintf(stdout, "cget - Read text from clipboard and output to stdout\n");
    fprintf(stdout, "Copyright (c) 2025 thrashem\n\n");
    fprintf(stdout, "Usage:\n");
    fprintf(stdout, "  cget                 Read text from clipboard\n");
    fprintf(stdout, "  cget -h, --help, /?  Show this help\n\n");
    fprintf(stdout, "Examples:\n");
    fprintf(stdout, "  cget > output.txt    Save clipboard content to file\n");
    fprintf(stdout, "  cget | grep keyword  Search for keyword\n");
    fprintf(stdout, "  cget | findstr ABC   Search lines containing ABC\n\n");
    fprintf(stdout, "Exit codes:\n");
    fprintf(stdout, "  0  Success (text output)\n");
    fprintf(stdout, "  1  Cannot open clipboard\n");
    fprintf(stdout, "  2  No text data\n");
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
    
    // バイナリモードに設定（改行変換を防ぐ）
    _setmode(_fileno(stdout), _O_BINARY);
    
    // クリップボードを開く
    if (!OpenClipboard(NULL)) {
        fprintf(stderr, "Error: Cannot open clipboard\n");
        return 1;
    }

    HANDLE hData = NULL;
    wchar_t* pszText = NULL;

    // まずUnicodeテキストを試す
    if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        hData = GetClipboardData(CF_UNICODETEXT);
        if (hData != NULL) {
            pszText = (wchar_t*)GlobalLock(hData);
            if (pszText != NULL) {
                // UTF-8に変換
                int utf8_size = WideCharToMultiByte(CP_UTF8, 0, pszText, -1, NULL, 0, NULL, NULL);
                if (utf8_size > 0) {
                    char* utf8_text = (char*)malloc(utf8_size);
                    if (utf8_text != NULL) {
                        WideCharToMultiByte(CP_UTF8, 0, pszText, -1, utf8_text, utf8_size, NULL, NULL);
                        fwrite(utf8_text, 1, strlen(utf8_text), stdout);
                        free(utf8_text);
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
                // マルチバイト文字列を適切に処理
                fprintf(stdout, "%s", pszAnsiText);
                GlobalUnlock(hData);
                CloseClipboard();
                return 0;
            }
        }
    }

    CloseClipboard();
    fprintf(stderr, "Error: No text data in clipboard\n");
    return 2;
}