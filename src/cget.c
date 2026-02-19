/*
 * cget - クリップボードからテキストを読み取り標準出力に出力
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

#define CP_SJIS 932

void show_usage(void) {
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
    fprintf(stdout, "  3  Character conversion failed\n");
}

/*
 * 出力先がコンソール（TTY）かどうかを判定する。
 *
 * コンソール直接出力  → TRUE  : GetConsoleOutputCP() のCPに従う
 * パイプ・リダイレクト → FALSE : UTF-8 固定で出力する
 *
 * FILE_TYPE_CHAR はコンソールハンドルのみ返す値。
 * Windows Terminal / VSCode 統合ターミナルは FILE_TYPE_PIPE になることがあるが、
 * それらは UTF-8 前提で動いているため UTF-8 固定で問題ない。
 */
static BOOL is_console_output(void) {
    return GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == FILE_TYPE_CHAR;
}

/*
 * wideStr (UTF-16LE) を targetCP に変換して stdout に書き出す。
 * 成功時 0、失敗時 3 を返す。
 *
 * 改行コードについて:
 *   Windowsクリップボードのテキストは \r\n。
 *   バイナリモードでそのまま出力するため \r\n が維持される。
 *   Unix系ツールにパイプする場合は受け取る側で対処すること。
 */
static int write_wide_as(const wchar_t* wideStr, UINT targetCP) {
    int outLen = WideCharToMultiByte(targetCP, 0, wideStr, -1, NULL, 0, NULL, NULL);
    if (outLen <= 0) return 3;

    char* outBuf = (char*)malloc(outLen);
    if (outBuf == NULL) return 3;

    WideCharToMultiByte(targetCP, 0, wideStr, -1, outBuf, outLen, NULL, NULL);
    fwrite(outBuf, 1, outLen - 1, stdout); /* null終端を除いたバイト数を書き出す */
    free(outBuf);
    return 0;
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
     * 出力先に応じてコードページを決定する。
     *
     * コンソール直接  → GetConsoleOutputCP() に従う
     *   chcp 932   のコンソールなら SJIS で出す（文字化けしない）
     *   chcp 65001 のコンソールなら UTF-8 で出す
     *
     * パイプ・リダイレクト → UTF-8 固定
     *   `cget > file.txt`  → UTF-8ファイルが生成される
     *   `cget | cset`      → cset が UTF-8 として受け取るので整合する
     *   `cget | findstr`   → findstr は SJIS を期待するが、
     *                         chcp 65001 にすれば UTF-8 パイプも通る
     *
     * これにより chcp 65001 なしで `cget > file.txt` が
     * UTF-8ファイルを生成するようになる。
     */
    UINT outputCP = is_console_output() ? GetConsoleOutputCP() : CP_UTF8;

    /* バイナリモードで出す（\n → \r\n の自動変換を防ぐ） */
    _setmode(_fileno(stdout), _O_BINARY);

    if (!OpenClipboard(NULL)) {
        fprintf(stderr, "Error: Cannot open clipboard\n");
        return 1;
    }

    wchar_t* pszWide = NULL;
    HANDLE   hData   = NULL;

    /* CF_UNICODETEXT を優先して取得する。
     * Windowsクリップボードは内部的にUnicodeで管理しており、
     * CF_UNICODETEXTが存在しないケースは実質ない。
     */
    if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        hData = GetClipboardData(CF_UNICODETEXT);
        if (hData != NULL) {
            pszWide = (wchar_t*)GlobalLock(hData);
        }
    }

    if (pszWide != NULL) {
        int result = write_wide_as(pszWide, outputCP);
        GlobalUnlock(hData);
        CloseClipboard();
        return result;
    }

    /* CF_UNICODETEXTが取れなかった場合のフォールバック（古いアプリ対応）。
     * CF_TEXTはCP_SJIS(932)で格納されている慣例に従い、
     * 一度UTF-16に起こしてからoutputCPに変換することで
     * SJIS→UTF-8等のクロス変換にも対応する。
     */
    if (IsClipboardFormatAvailable(CF_TEXT)) {
        hData = GetClipboardData(CF_TEXT);
        if (hData != NULL) {
            char* pszAnsi = (char*)GlobalLock(hData);
            if (pszAnsi != NULL) {
                int wLen = MultiByteToWideChar(CP_SJIS, 0, pszAnsi, -1, NULL, 0);
                if (wLen > 0) {
                    wchar_t* wBuf = (wchar_t*)malloc(wLen * sizeof(wchar_t));
                    if (wBuf != NULL) {
                        MultiByteToWideChar(CP_SJIS, 0, pszAnsi, -1, wBuf, wLen);
                        int result = write_wide_as(wBuf, outputCP);
                        free(wBuf);
                        GlobalUnlock(hData);
                        CloseClipboard();
                        return result;
                    }
                }
                GlobalUnlock(hData);
            }
        }
    }

    CloseClipboard();
    fprintf(stderr, "Error: No text data in clipboard\n");
    return 2;
}
