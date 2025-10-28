/*
 * cinfo - List clipboard formats and their sizes
 *
 * Copyright (c) 2025 thrashem
 * Released under the MIT License
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

#define MAX_FORMAT_NAME 128

void show_usage() {
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

// 標準フォーマット名のマッピング
const char* get_standard_format_name(UINT fmt) {
    switch (fmt) {
        case CF_TEXT: return "CF_TEXT";
        case CF_UNICODETEXT: return "CF_UNICODETEXT";
        case CF_BITMAP: return "CF_BITMAP";
        case CF_DIB: return "CF_DIB";
        case CF_DIBV5: return "CF_DIBV5";
        case CF_HDROP: return "CF_HDROP";
        case CF_LOCALE: return "CF_LOCALE";
        case CF_OEMTEXT: return "CF_OEMTEXT";
        case CF_ENHMETAFILE: return "CF_ENHMETAFILE";
        case CF_METAFILEPICT: return "CF_METAFILEPICT";
        case CF_PALETTE: return "CF_PALETTE";
        case CF_RIFF: return "CF_RIFF";
        case CF_WAVE: return "CF_WAVE";
        case CF_SYLK: return "CF_SYLK";
        case CF_DIF: return "CF_DIF";
        case CF_PENDATA: return "CF_PENDATA";
        case CF_OWNERDISPLAY: return "CF_OWNERDISPLAY";
        default: return "Unknown Standard Format";
    }
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

    // バイナリモード（改行変換防止）
    _setmode(_fileno(stdout), _O_BINARY);

    // UTF-8出力（SJIS環境ではコメントアウト可）
    SetConsoleOutputCP(CP_UTF8);

    if (!OpenClipboard(NULL)) {
        fprintf(stderr, "Error: Cannot open clipboard\n");
        return 1;
    }

    UINT fmt = 0;
    int count = 0;

    fprintf(stdout, "クリップボードの内容一覧\n\n");

    while ((fmt = EnumClipboardFormats(fmt)) != 0) {
        count++;

        char name[MAX_FORMAT_NAME] = {0};
        const char* type = NULL;

        if (fmt < 0xC000) {
            strncpy(name, get_standard_format_name(fmt), MAX_FORMAT_NAME - 1);
            type = "標準フォーマット";
        } else {
            if (!GetClipboardFormatName(fmt, name, MAX_FORMAT_NAME)) {
                strncpy(name, "Unknown Custom Format", MAX_FORMAT_NAME - 1);
            }
            type = "カスタムフォーマット";
        }

        SIZE_T size = 0;
        HANDLE hData = GetClipboardData(fmt);
        if (hData) {
            size = GlobalSize(hData);
        }

        fprintf(stdout, "%d. フォーマット名: %s\n", count, name);
        fprintf(stdout, "   - 種類        : %s\n", type);
        fprintf(stdout, "   - データサイズ: %llu バイト\n\n", (unsigned long long)size);
    }

    CloseClipboard();

    if (count == 0) {
        fprintf(stderr, "Error: No formats found in clipboard\n");
        return 2;
    }

    fprintf(stdout, "%d 件のフォーマットが検出されました。\n", count);
    return 0;
}