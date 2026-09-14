/* Minimal CP1251 <-> UTF-8 converters for Falcon-H1 UI.
 * Only Cyrillic + ASCII needed. No allocation. */
#ifndef CP1251_UTF8_H
#define CP1251_UTF8_H

#include <stddef.h>
#include <stdint.h>

/* CP1251 codepage -> Unicode codepoint for the 0x80-0xFF range.
 * Values < 0x80 are identity. 0 means unmapped. */
static const uint16_t cp1251_to_unicode[128] = {
    /* 0x80 */ 0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
    /* 0x88 */ 0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
    /* 0x90 */ 0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    /* 0x98 */ 0x0000, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
    /* 0xA0 */ 0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
    /* 0xA8 */ 0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
    /* 0xB0 */ 0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
    /* 0xB8 */ 0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
    /* 0xC0 */ 0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
    /* 0xC8 */ 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
    /* 0xD0 */ 0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
    /* 0xD8 */ 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
    /* 0xE0 */ 0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
    /* 0xE8 */ 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
    /* 0xF0 */ 0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
    /* 0xF8 */ 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F
};

/* Encode one Unicode codepoint to UTF-8. Returns bytes written (1-3) or 0. */
static int utf8_encode(uint32_t cp, char *out) {
    if (cp < 0x80) {
        out[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    return 0;
}

/* Decode one UTF-8 sequence.
 * Returns:
 *   >0  = bytes consumed, *cp set
 *    0  = invalid lead byte (caller should skip 1)
 *   -1  = incomplete sequence (need more bytes; leave in buffer)
 */
static int utf8_decode(const uint8_t *in, size_t len, uint32_t *cp) {
    if (len == 0) return -1;
    uint8_t c0 = in[0];
    if (c0 < 0x80) {
        *cp = c0;
        return 1;
    }
    if ((c0 & 0xE0) == 0xC0) {           /* 2-byte */
        if (len < 2) return -1;          /* incomplete */
        uint8_t c1 = in[1];
        if ((c1 & 0xC0) != 0x80) return 0; /* invalid */
        *cp = ((c0 & 0x1F) << 6) | (c1 & 0x3F);
        return 2;
    }
    if ((c0 & 0xF0) == 0xE0) {           /* 3-byte */
        if (len < 3) return -1;
        uint8_t c1 = in[1], c2 = in[2];
        if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) return 0;
        *cp = ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
        return 3;
    }
    /* 4-byte or pure invalid lead – treat as skip-one */
    return 0;
}

/* Reverse map: Unicode -> CP1251 byte (0 = unmapped). */
static uint8_t unicode_to_cp1251(uint32_t cp) {
    if (cp < 0x80) return (uint8_t)cp;
    /* Fast path for the main Cyrillic block */
    if (cp >= 0x0410 && cp <= 0x044F)
        return (uint8_t)(0xC0 + (cp - 0x0410));
    if (cp == 0x0401) return 0xA8; /* Ё */
    if (cp == 0x0451) return 0xB8; /* ё */
    /* rarer letters */
    for (int i = 0; i < 128; ++i)
        if (cp1251_to_unicode[i] == cp)
            return (uint8_t)(0x80 + i);
    return 0; /* unmapped */
}

/* Convert CP1251 string -> UTF-8. Returns bytes written (not including NUL).
 * out_size must be enough (worst case 3x input). Always NUL-terminates if space. */
static size_t cp1251_to_utf8(const char *in, size_t in_len,
                             char *out, size_t out_size) {
    size_t o = 0;
    for (size_t i = 0; i < in_len; ++i) {
        unsigned char c = (unsigned char)in[i];
        uint32_t cp = (c < 0x80) ? c : cp1251_to_unicode[c - 0x80];
        if (cp == 0) cp = '?';
        char tmp[4];
        int n = utf8_encode(cp, tmp);
        if (n == 0 || o + (size_t)n + 1 > out_size) break;
        for (int k = 0; k < n; ++k) out[o++] = tmp[k];
    }
    if (o < out_size) out[o] = '\0';
    return o;
}

/* Convert UTF-8 -> CP1251. Unmappable chars become '?'.
 * Returns bytes written. Always NUL-terminates if space. */
static size_t utf8_to_cp1251(const char *in, size_t in_len,
                             char *out, size_t out_size) {
    size_t o = 0, i = 0;
    while (i < in_len && o + 1 < out_size) {
        uint32_t cp;
        int n = utf8_decode((const uint8_t *)in + i, in_len - i, &cp);
        if (n <= 0) break;
        i += (size_t)n;
        uint8_t b = unicode_to_cp1251(cp);
        out[o++] = (char)(b ? b : '?');
    }
    if (o < out_size) out[o] = '\0';
    return o;
}

#endif
