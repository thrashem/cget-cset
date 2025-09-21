# cget-cset

Lightweight clipboard tools for Windows that complement Microsoft's built-in `clip` command.

## Overview

Microsoft's `clip` command only supports writing to clipboard (stdin → clipboard). These tools provide the missing functionality:

- **cget**: Read text from clipboard and output to stdout (clipboard → stdout)
- **cset**: Write text from stdin to clipboard (stdin → clipboard, with Unicode support)

## Features

- 🚀 **Lightweight**: ~23KB each, no dependencies
- 🔤 **Unicode Support**: Automatic encoding detection (UTF-8/Shift_JIS)
- 🔄 **Bidirectional**: Complete clipboard read/write functionality
- 🖥️ **Windows Native**: Uses Windows API directly
- 📦 **Single Executable**: No runtime dependencies required

## Installation

### Option 1: Download Release
Download pre-built executables from [Releases](https://github.com/thrashem/cget-cset/releases)

### Option 2: Build from Source
```cmd
# Requirements: GCC (MinGW-w64)
winget install BrechtSanders.WinLibs.POSIX.UCRT

# Build
gcc -O2 -s -static src/cget.c -o cget.exe
gcc -O2 -s -static src/cset.c -o cset.exe
```

## Usage

### Basic Examples
```cmd
# Read from clipboard
cget

# Write to clipboard
echo "Hello World" | cset

# Save clipboard to file
cget > output.txt

# Load file to clipboard
type file.txt | cset
```

### Advanced Examples
```cmd
# Process clipboard content and return
cget | findstr "keyword" | cset

# Combine with Microsoft clip
echo "data" | clip     # Using Microsoft's clip
cget | cset           # Round-trip processing

# Directory listing to clipboard
dir | cset
```

### Command Line Options
```cmd
# Show help
cget --help
cset -h
cget /?
```

## Comparison with Microsoft clip

| Command | Direction | Notes |
|---------|-----------|-------|
| `clip` | stdin → clipboard | Microsoft official |
| `cset` | stdin → clipboard | Equivalent with Unicode support |
| `cget` | clipboard → stdout | Missing functionality (provided here) |

## Exit Codes

### cget
- `0`: Success (text output)
- `1`: Cannot open clipboard
- `2`: No text data

### cset
- `0`: Success
- `1`: Memory allocation failed
- `2`: No stdin input
- `3`: Character conversion failed
- `4`: Cannot open clipboard
- `5-7`: Clipboard operation failed

## Technical Details

- **Language**: C (Windows API)
- **Compiler**: GCC (MinGW-w64)
- **Size**: ~23KB each
- **Dependencies**: None (static linking)
- **Windows Support**: Windows XP and later
- **Encoding**: Automatic detection (UTF-8/Shift_JIS/ANSI)

## License

MIT License - see [LICENSE](LICENSE) file

## Author

Copyright (c) 2025 thrashem

## Contributing

Pull requests welcome! Please ensure:
- Code follows existing style
- Windows compatibility maintained
- No external dependencies added