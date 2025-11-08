// Copyright (c) Tzvetan Mikov and contributors
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text

// Pointer access builtins.
const _ptr_write_char = $SHBuiltin.extern_c({declared: true}, function _sh_ptr_write_char(ptr: c_ptr, offset: c_int, v: c_char): void {
});
const _ptr_read_uchar = $SHBuiltin.extern_c({declared: true}, function _sh_ptr_read_uchar(ptr: c_ptr, offset: c_int): c_uchar {
    throw 0;
});

/// Allocate native memory using calloc() or throw an exception.
function calloc(size: number): c_ptr {
    "inline";
    "use unsafe";

    let res = _calloc(1, size);
    if (res === 0) throw Error("OOM");
    return res;
}

/// Allocate native memory using malloc() or throw an exception.
function malloc(size: number): c_ptr {
    "inline";
    "use unsafe";

    let res = _malloc(size);
    if (res === 0) throw Error("OOM");
    return res;
}

function copyToAsciiz(s: any, buf: c_ptr, size: number): void {
    if (s.length >= size) throw Error("String too long");
    let i = 0;
    for (let e = s.length; i < e; ++i) {
        let code: number = s.charCodeAt(i);
        if (code > 127) throw Error("String is not ASCII");
        _ptr_write_char(buf, i, code);
    }
    _ptr_write_char(buf, i, 0);
}

/// Convert a JS string to UTF-8 encoded null-terminated string.
/// Returns the number of bytes written (excluding null terminator).
function copyToUtf8(s: any, buf: c_ptr, maxSize: number): number {
    let byteIndex = 0;
    for (let i = 0, e = s.length; i < e; ++i) {
        let code: number = s.charCodeAt(i);

        if (code < 0x80) {
            // 1-byte sequence (ASCII)
            if (byteIndex >= maxSize - 1) throw Error("String too long");
            _ptr_write_char(buf, byteIndex++, code);
        } else if (code < 0x800) {
            // 2-byte sequence
            if (byteIndex >= maxSize - 2) throw Error("String too long");
            _ptr_write_char(buf, byteIndex++, 0xC0 | (code >> 6));
            _ptr_write_char(buf, byteIndex++, 0x80 | (code & 0x3F));
        } else if (code < 0xD800 || code >= 0xE000) {
            // 3-byte sequence (not a surrogate)
            if (byteIndex >= maxSize - 3) throw Error("String too long");
            _ptr_write_char(buf, byteIndex++, 0xE0 | (code >> 12));
            _ptr_write_char(buf, byteIndex++, 0x80 | ((code >> 6) & 0x3F));
            _ptr_write_char(buf, byteIndex++, 0x80 | (code & 0x3F));
        } else {
            // Surrogate pair - 4-byte sequence
            if (i + 1 >= e) throw Error("Incomplete surrogate pair");
            let high = code;
            let low = s.charCodeAt(++i);
            if (low < 0xDC00 || low > 0xDFFF) throw Error("Invalid surrogate pair");

            let codepoint = 0x10000 + ((high & 0x3FF) << 10) + (low & 0x3FF);
            if (byteIndex >= maxSize - 4) throw Error("String too long");
            _ptr_write_char(buf, byteIndex++, 0xF0 | (codepoint >> 18));
            _ptr_write_char(buf, byteIndex++, 0x80 | ((codepoint >> 12) & 0x3F));
            _ptr_write_char(buf, byteIndex++, 0x80 | ((codepoint >> 6) & 0x3F));
            _ptr_write_char(buf, byteIndex++, 0x80 | (codepoint & 0x3F));
        }
    }
    _ptr_write_char(buf, byteIndex, 0);
    return byteIndex;
}

/// Convert a JS string to ASCIIZ.
function stringToAsciiz(s: any): c_ptr {
    "use unsafe";

    if (typeof s !== "string") s = String(s);
    let buf = malloc(s.length + 1);
    try {
        copyToAsciiz(s, buf, s.length + 1);
        return buf;
    } catch (e) {
        _free(buf);
        throw e;
    }
}

/// Convert a JS string to UTF-8 with temp allocation.
function tmpUtf8(s: any): c_ptr {
    "use unsafe";

    if (typeof s !== "string") s = String(s);
    // UTF-8 can be up to 4 bytes per char, so allocate conservatively
    let buf = allocTmp(s.length * 4 + 1);
    copyToUtf8(s, buf, s.length * 4 + 1);
    return buf;
}

/// Convert a JS string to ASCIIZ.
function tmpAsciiz(s: any): c_ptr {
    "use unsafe";

    if (typeof s !== "string") s = String(s);
    let buf = allocTmp(s.length + 1);
    copyToAsciiz(s, buf, s.length + 1);
    return buf;
}


// Bump pointer allocator for temporary allocations
const INITIAL_BLOCK_SIZE = 4096;
const MAX_BLOCK_SIZE = 65536;
const ALIGNMENT = 8;

let _blocks: c_ptr[] = [];
let _currentBlock: c_ptr = c_null;
let _currentOffset: number = 0;
let _currentSize: number = 0;
let _nextBlockSize: number = INITIAL_BLOCK_SIZE;

function allocTmp(size: number): c_ptr {
    // Round up to alignment boundary
    size = (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);

    // Check if allocation fits in current block
    if (_currentOffset + size <= _currentSize) {
        let ptr = _sh_ptr_add(_currentBlock, _currentOffset);
        _currentOffset += size;
        return ptr;
    }

    // Need a new block
    let blockSize = _nextBlockSize;
    // Large allocation - allocate exact size but don't grow block size
    if (size > blockSize)
        blockSize = size;

    let newBlock = calloc(blockSize);
    _blocks.push(newBlock);
    _currentBlock = newBlock;
    _currentSize = blockSize;
    _currentOffset = size;

    // Grow next block size
    if (_nextBlockSize < MAX_BLOCK_SIZE)
        _nextBlockSize = _nextBlockSize * 2;

    return newBlock;
}

function flushAllocTmp(): void {
    // Free all blocks
    for (let i = 0; i < _blocks.length; ++i) {
        _free(_blocks[i]);
    }

    // Reset all state
    let empty: c_ptr[] = [];
    _blocks = empty;
    _currentBlock = c_null;
    _currentOffset = 0;
    _currentSize = 0;
    _nextBlockSize = INITIAL_BLOCK_SIZE;
}
