/*
 * cinfo - List clipboard formats and their sizes
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

#define MAX_FORMAT_NAME 128
#define CP_SJIS 932

void show_usage(void) {
    fprintf(stdout, "cinfo - List clipboard formats and their sizes\n");
    fprintf(stdout, "Copyright (c) 2025 thrashem\n\n");
    fprintf(stdout, "Usage:\n");
    fprintf(stdout, "  cinfo                 Show clipboard format list\n");
    fprintf(stdout, "  cinfo -h, --help, /?  Show this help\n\n");
    fprintf(stdout, "Examples:\n");
    fprintf(stdout, "  cinfo > formats.txt   Save format list to file\n\n");
    fprintf(stdout, "Exit codes:\n");
    fprintf(stdout, "  0  Success (formats listed)\n");
    fprintf(stdout, "  1  Cannot open clipboard\n");
    fprintf(stdout, "  2  No formats found\n");
}

static BOOL is_console_output(void) {
    return GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == FILE_TYPE_CHAR;
}

const char* get_standard_format_name(UINT fmt) {
    switch (fmt) {
        case CF_TEXT:         return "CF_TEXT";
        case CF_UNICODETEXT:  return "CF_UNICODETEXT";
        case CF_BITMAP:       return "CF_BITMAP";
        case CF_DIB:          return "CF_DIB";
        case CF_DIBV5:        return "CF_DIBV5";
        case CF_HDROP:        return "CF_HDROP";
        case CF_LOCALE:       return "CF_LOCALE";
        case CF_OEMTEXT:      return "CF_OEMTEXT";
        case CF_ENHMETAFILE:  return "CF_ENHMETAFILE";
        case CF_METAFILEPICT: return "CF_METAFILEPICT";
        case CF_PALETTE:      return "CF_PALETTE";
        case CF_RIFF:         return "CF_RIFF";
        case CF_WAVE:         return "CF_WAVE";
        case CF_SYLK:         return "CF_SYLK";
        case CF_DIF:          return "CF_DIF";
        case CF_PENDATA:      return "CF_PENDATA";
        case CF_OWNERDISPLAY: return "CF_OWNERDISPLAY";
        default:              return NULL;
    }
}

/*
 * wchar_t* を outputCP に変換して stdout に書き出すヘルパー。
 * cget と同じ設計。
 */
static void write_wide(const wchar_t* wstr, UINT outputCP) {
    int len = WideCharToMultiByte(outputCP, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (len <= 0) return;
    char* buf = (char*)malloc(len);
    if (buf == NULL) return;
    WideCharToMultiByte(outputCP, 0, wstr, -1, buf, len, NULL, NULL);
    fwrite(buf, 1, len - 1, stdout);
    free(buf);
}

/* write_wide の printf 相当ラッパー */
static void wprintf_out(UINT outputCP, const wchar_t* fmt, ...) {
    wchar_t wbuf[1024];
    va_list args;
    va_start(args, fmt);
    _vsnwprintf(wbuf, 1024, fmt, args);
    va_end(args);
    write_wide(wbuf, outputCP);
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
     * cget と同じ判定:
     *   コンソール直接 → GetConsoleOutputCP() に従う
     *   リダイレクト   → UTF-8 固定
     * SetConsoleOutputCP() は呼ばない（他プロセスに影響しないが
     * 自プロセスの以降の出力に副作用があるため使わない）。
     */
    UINT outputCP = is_console_output() ? GetConsoleOutputCP() : CP_UTF8;

    _setmode(_fileno(stdout), _O_BINARY);

    if (!OpenClipboard(NULL)) {
        fprintf(stderr, "Error: Cannot open clipboard\n");
        return 1;
    }

    UINT fmt   = 0;
    int  count = 0;

    wprintf_out(outputCP, L"クリップボードの内容一覧\n\n");

    while ((fmt = EnumClipboardFormats(fmt)) != 0) {
        count++;

        /* フォーマット名を取得 */
        wchar_t wname[MAX_FORMAT_NAME] = {0};
        const char* stdName = get_standard_format_name(fmt);
        const wchar_t* type;

        if (stdName != NULL) {
            /* 標準フォーマット: ASCII名をそのままワイド化 */
            MultiByteToWideChar(CP_ACP, 0, stdName, -1, wname, MAX_FORMAT_NAME);
            type = L"標準フォーマット";
        } else if (fmt < 0xC000) {
            /* 未知の標準フォーマット範囲 */
            _snwprintf(wname, MAX_FORMAT_NAME, L"Unknown Standard (0x%04X)", fmt);
            type = L"標準フォーマット";
        } else {
            /*
             * カスタムフォーマット: GetClipboardFormatNameW でワイド文字列取得。
             * W版を使うことでUnicode名にも対応する。
             */
            if (!GetClipboardFormatNameW(fmt, wname, MAX_FORMAT_NAME)) {
                _snwprintf(wname, MAX_FORMAT_NAME, L"Unknown Custom (0x%04X)", fmt);
            }
            type = L"カスタムフォーマット";
        }

        /* サイズを取得
         * 遅延レンダリング（ExcelのコピーなどOLEオブジェクト）の場合、
         * GetClipboardData を呼ぶと初めてデータが生成される。
         * サイズ0のフォーマットは遅延レンダリングの可能性が高い。
         */
        SIZE_T size = 0;
        BOOL   lazy = FALSE;
        HANDLE hData = GetClipboardData(fmt);
        if (hData != NULL) {
            size = GlobalSize(hData);
            if (size == 0) lazy = TRUE;
        } else {
            lazy = TRUE;
        }

        wprintf_out(outputCP, L"%d. %s\n", count, wname);
        wprintf_out(outputCP, L"   - 種類        : %s\n", type);
        if (lazy) {
            wprintf_out(outputCP, L"   - データサイズ: (遅延レンダリング)\n\n");
        } else {
            wprintf_out(outputCP, L"   - データサイズ: %llu バイト\n\n",
                        (unsigned long long)size);
        }
    }

    CloseClipboard();

    if (count == 0) {
        fprintf(stderr, "Error: No formats found in clipboard\n");
        return 2;
    }

    wprintf_out(outputCP, L"%d 件のフォーマットが検出されました。\n", count);
    return 0;
}