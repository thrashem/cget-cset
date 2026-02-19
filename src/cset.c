/*
 * cset - 標準入力からテキストを読み取りクリップボードに書き込み
 *
 * Copyright (c) 2025 thrashem
 * Released under the MIT License
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

#define CHUNK_SIZE 8192
#define CP_SJIS    932

void show_usage(void) {
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
    printf("  5  Clipboard memory allocation failed\n");
    printf("  6  Clipboard memory lock failed\n");
    printf("  7  Clipboard set data failed\n");
}

/*
 * 入力元がコンソール（TTY）かどうかを判定する。
 *
 * コンソール直接入力  → TRUE  : GetConsoleCP() のCPに従う（SJIS or UTF-8）
 * パイプ・リダイレクト → FALSE : UTF-8として読む
 *   - `echo text | cset` のようなパイプ
 *   - `cset < file.txt` のようなリダイレクト
 *   - Windows Terminal / VSCode 統合ターミナルもこちらになる場合がある
 *     （その場合もUTF-8前提で動いているので問題ない）
 */
static BOOL is_console_input(void) {
    return GetFileType(GetStdHandle(STD_INPUT_HANDLE)) == FILE_TYPE_CHAR;
}

/*
 * GMEM_MOVEABLEブロックにデータを書き込むヘルパー。
 * 成功時はハンドルを、失敗時はNULLを返す。
 * SetClipboardData後はOSが所有権を持つため、
 * 成功したハンドルをGlobalFreeしてはいけない。
 */
static HGLOBAL alloc_clipboard_mem(const void* data, SIZE_T size) {
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (hMem == NULL) return NULL;
    void* p = GlobalLock(hMem);
    if (p == NULL) { GlobalFree(hMem); return NULL; }
    memcpy(p, data, size);
    GlobalUnlock(hMem);
    return hMem;
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 ||
            strcmp(argv[1], "--help") == 0 ||
            strcmp(argv[1], "/?") == 0) {
            show_usage();
            return 0;
        }
    }

    /*
     * 入力元に応じてコードページを決定する。
     *
     * コンソール直接  → GetConsoleCP() に従う
     *   chcp 932   なら SJIS として読む
     *   chcp 65001 なら UTF-8 として読む
     *
     * パイプ・リダイレクト → UTF-8 固定
     *   `echo text | cset` は通常 UTF-8 で流れてくる。
     *   `cget | cset` のパイプも cget が UTF-8 を出力するので整合する。
     */
    UINT inputCP = is_console_input() ? GetConsoleCP() : CP_UTF8;

    /* バイナリモードで読む（改行変換を防ぐ） */
    _setmode(_fileno(stdin), _O_BINARY);

    /* --- 標準入力を全部読む --- */
    char*  buffer         = NULL;
    size_t totalSize      = 0;
    size_t bufferCapacity = CHUNK_SIZE;

    buffer = (char*)malloc(bufferCapacity);
    if (buffer == NULL) return 1;

    char   tempBuffer[CHUNK_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(tempBuffer, 1, CHUNK_SIZE, stdin)) > 0) {
        if (totalSize + bytesRead + 1 >= bufferCapacity) {
            bufferCapacity = (totalSize + bytesRead + CHUNK_SIZE) * 2;
            char* nb = (char*)realloc(buffer, bufferCapacity);
            if (nb == NULL) { free(buffer); return 1; }
            buffer = nb;
        }
        memcpy(buffer + totalSize, tempBuffer, bytesRead);
        totalSize += bytesRead;
    }

    if (totalSize == 0) { free(buffer); return 2; }
    buffer[totalSize] = '\0';

    /* --- 入力 → UTF-16LE --- */
    int wideLen = MultiByteToWideChar(inputCP, 0, buffer, -1, NULL, 0);
    if (wideLen <= 0) { free(buffer); return 3; }

    wchar_t* wideBuffer = (wchar_t*)malloc(wideLen * sizeof(wchar_t));
    if (wideBuffer == NULL) { free(buffer); return 1; }

    MultiByteToWideChar(inputCP, 0, buffer, -1, wideBuffer, wideLen);
    free(buffer);

    /* --- UTF-16LE → SJIS ---
     * CP_ACPではなくCP_SJIS(932)を明示する。
     * CP_ACPはシステムロケール依存で、日本語環境以外では別CPになる。
     */
    int   sjisLen  = WideCharToMultiByte(CP_SJIS, 0, wideBuffer, -1, NULL, 0, NULL, NULL);
    char* sjisBuffer = NULL;
    if (sjisLen > 0) {
        sjisBuffer = (char*)malloc(sjisLen);
        if (sjisBuffer != NULL) {
            WideCharToMultiByte(CP_SJIS, 0, wideBuffer, -1, sjisBuffer, sjisLen, NULL, NULL);
        }
    }

    /* --- クリップボードへの書き込み ---
     * OpenClipboard〜CloseClipboardの区間を最短にするため、
     * 変換処理はすべてここより前に完了させている。
     * この区間中は他プロセスがクリップボードにアクセスできない。
     */
    if (!OpenClipboard(NULL)) {
        free(wideBuffer);
        free(sjisBuffer);
        return 4;
    }
    EmptyClipboard();

    /* CF_UNICODETEXT を先にセットする。
     * Windowsはフォーマット登録順で優先度を判断するアプリが多いため、
     * Unicodeを先に置くのが安全。
     */
    HGLOBAL hUnicode = alloc_clipboard_mem(wideBuffer, wideLen * sizeof(wchar_t));
    free(wideBuffer);
    wideBuffer = NULL;

    if (hUnicode == NULL) {
        free(sjisBuffer);
        CloseClipboard();
        return 5;
    }
    if (SetClipboardData(CF_UNICODETEXT, hUnicode) == NULL) {
        GlobalFree(hUnicode); /* 所有権移転前なので解放OK */
        free(sjisBuffer);
        CloseClipboard();
        return 7;
    }
    /* 所有権移転済み。以降 hUnicode を触らない */

    /* CF_TEXT (SJIS) をセット。
     * 失敗しても CF_UNICODETEXT は入っているので致命的ではない。
     */
    if (sjisBuffer != NULL) {
        HGLOBAL hAnsi = alloc_clipboard_mem(sjisBuffer, sjisLen);
        free(sjisBuffer);
        if (hAnsi != NULL) {
            if (SetClipboardData(CF_TEXT, hAnsi) == NULL) {
                GlobalFree(hAnsi); /* 所有権移転前なので解放OK */
            }
            /* 成功時は所有権移転済み。触らない */
        }
    }

    CloseClipboard();
    return 0;
}
