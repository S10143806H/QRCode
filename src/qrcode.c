/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Richard Moore
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 *  Special thanks to Nayuki (https://www.nayuki.io/) from which this library was
 *  heavily inspired and compared against.
 *
 *  See: https://github.com/nayuki/QR-Code-generator/tree/master/cpp
 */


#include "qrcode.h"

#include <stdlib.h>
#include <string.h>

#pragma mark - Error Correction Lookup tables

#if LOCK_VERSION == 0

static const int16_t NUM_ERROR_CORRECTION_CODEWORDS[4][41] = {
    // 1,  2,  3,  4,  5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,   25,   26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,   40    Error correction level
    { 10, 16, 26, 36, 48,  64,  72,  88, 110, 130, 150, 176, 198, 216, 240, 280, 308, 338, 364, 416, 442, 476, 504, 560,  588,  644,  700,  728,  784,  812,  868,  924,  980, 1036, 1064, 1120, 1204, 1260, 1316, 1372},  // Medium
    {  7, 10, 15, 20, 26,  36,  40,  48,  60,  72,  80,  96, 104, 120, 132, 144, 168, 180, 196, 224, 224, 252, 270, 300,  312,  336,  360,  390,  420,  450,  480,  510,  540,  570,  570,  600,  630,  660,  720,  750},  // Low
    { 17, 28, 44, 64, 88, 112, 130, 156, 192, 224, 264, 308, 352, 384, 432, 480, 532, 588, 650, 700, 750, 816, 900, 960, 1050, 1110, 1200, 1260, 1350, 1440, 1530, 1620, 1710, 1800, 1890, 1980, 2100, 2220, 2310, 2430},  // High
    { 13, 22, 36, 52, 72,  96, 108, 132, 160, 192, 224, 260, 288, 320, 360, 408, 448, 504, 546, 600, 644, 690, 750, 810,  870,  952, 1020, 1050, 1140, 1200, 1290, 1350, 1440, 1530, 1590, 1680, 1770, 1860, 1950, 2040},  // Quartile
};

static const int8_t NUM_ERROR_CORRECTION_BLOCKS[4][41] = {
    // Version: (note that index 0 is for padding, and is set to an illegal value)
    // 1, 2, 3, 4, 5, 6, 7, 8, 9,10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40    Error correction level
    {  1, 1, 1, 2, 2, 4, 4, 4, 5, 5,  5,  8,  9,  9, 10, 10, 11, 13, 14, 16, 17, 17, 18, 20, 21, 23, 25, 26, 28, 29, 31, 33, 35, 37, 38, 40, 43, 45, 47, 49},  // Medium
    {  1, 1, 1, 1, 1, 2, 2, 2, 2, 4,  4,  4,  4,  4,  6,  6,  6,  6,  7,  8,  8,  9,  9, 10, 12, 12, 12, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 24, 25},  // Low
    {  1, 1, 2, 4, 4, 4, 5, 6, 8, 8, 11, 11, 16, 16, 18, 16, 19, 21, 25, 25, 25, 34, 30, 32, 35, 37, 40, 42, 45, 48, 51, 54, 57, 60, 63, 66, 70, 74, 77, 81},  // High
    {  1, 1, 2, 2, 4, 4, 6, 6, 8, 8,  8, 10, 12, 16, 12, 17, 16, 18, 21, 20, 23, 23, 25, 27, 29, 34, 34, 35, 38, 40, 43, 45, 48, 51, 53, 56, 59, 62, 65, 68},  // Quartile
};


// @TODO: Put other LOCK_VERSIONS here
#elif LOCK_VERSION == 3

static const int16_t NUM_ERROR_CORRECTION_CODEWORDS[4] = {
    26, 15, 44, 36
};

static const int8_t NUM_ERROR_CORRECTION_BLOCKS[4] = {
    1, 1, 2, 2
};

#else

#error Unsupported LOCK_VERSION (add it...)

#endif


static int max(int a, int b) {
    if (a > b) { return a; }
    return b;
}

/*
static int abs(int value) {
    if (value < 0) { return -value; }
    return value;
}
*/


#pragma mark - Mode testing and conversion

static int8_t getAlphanumeric(char c) {
    
    if (c >= '0' && c <= '9') { return (c - '0'); }
    if (c >= 'A' && c <= 'Z') { return (c - 'A' + 10); }
    
    switch (c) {
        case ' ': return 36;
        case '$': return 37;
        case '%': return 38;
        case '*': return 39;
        case '+': return 40;
        case '-': return 41;
        case '.': return 42;
        case '/': return 43;
        case ':': return 44;
    }
    
    return -1;
}

static bool isAlphanumeric(const char *text) {
    for (; *text != '\0'; text++) {
        if (getAlphanumeric(*text) == -1) { return false; }
    }
    return true;
}


static bool isNumeric(const char *text) {
    for (; *text != '\0'; text++) {
        char c = *text;
        if (c < '0' || c > '9') { return false; }
    }
    return true;
}


#pragma mark - Counting

// We store the following tightly packed (less 8) in modeInfo
//               <=9  <=26  <= 40
// NUMERIC      ( 10,   12,    14);
// ALPHANUMERIC (  9,   11,    13);
// BYTE         (  8,   16,    16);
static char getModeBits(uint8_t version, uint8_t mode) {
    // Note: We use 15 instead of 16; since 15 doesn't exist and we cannot store 16 (8 + 8) in 3 bits
    // hex(int("".join(reversed([('00' + bin(x - 8)[2:])[-3:] for x in [10, 9, 8, 12, 11, 15, 14, 13, 15]])), 2))
    unsigned int modeInfo = 0x7bbb80a;
    
#if LOCK_VERSION == 0 || LOC_VERSION > 9
    if (version > 9) { modeInfo >>= 9; }
#endif
    
#if LOCK_VERSION == 0 || LOC_VERSION > 26
    if (version > 26) { modeInfo >>= 9; }
#endif
    
    char result = 8 + ((modeInfo >> (3 * mode)) & 0x07);
    if (result == 15) { result = 16; }
    
    return result;
}


// Returns the number of raw data modules (bits) available at the given version number.
// These data modules are used for both user data codewords and error correction codewords.
// This stateless pure function could be implemented as a 40-entry lookup table.
// @TODO: Do the lookup table solution
static int getNumRawDataModules(int ver) {
    int result = (16 * ver + 128) * ver + 64;
    if (ver >= 2) {
        int numAlign = ver / 7 + 2;
        result -= (25 * numAlign - 10) * numAlign - 55;
        if (ver >= 7) {
            result -= 18 * 2;  // Subtract version information
        }
    }
    return result;
}

#pragma mark - BitBucket

typedef struct BitBucket {
    uint32_t bitOffsetOrWidth;
    uint32_t capacityBytes;
    uint8_t *data;
} BitBucket;

/*
void bb_dump(BitBucket *bitBuffer) {
    printf("Buffer: ");
    for (uint32_t i = 0; i < bitBuffer->capacityBytes; i++) {
        printf("%02x", bitBuffer->data[i]);
        if ((i % 4) == 3) { printf(" "); }
    }
    printf("\n");
}
*/

uint32_t bb_getGridSizeBytes(uint8_t size) {
    return (((size * size) + 7) / 8);
}

uint32_t bb_getBufferSizeBytes(uint32_t bits) {
    return ((bits + 7) / 8);
}

void bb_initBuffer(BitBucket *bitBuffer, uint8_t *data, int32_t capacityBytes) {
    bitBuffer->bitOffsetOrWidth = 0;
    bitBuffer->capacityBytes = capacityBytes;
    bitBuffer->data = data;
    
    memset(data, 0, bitBuffer->capacityBytes);
}

void bb_initGrid(BitBucket *bitGrid, uint8_t *data, uint8_t size) {
    bitGrid->bitOffsetOrWidth = size;
    bitGrid->capacityBytes = bb_getGridSizeBytes(size);
    bitGrid->data = data;

    memset(data, 0, bitGrid->capacityBytes);
}

void bb_appendBits(BitBucket *bitBuffer, uint32_t val, int length) {
    uint32_t offset = bitBuffer->bitOffsetOrWidth;
    for (int i = length - 1; i >= 0; i--, offset++) {
        bitBuffer->data[offset >> 3] |= ((val >> i) & 1) << (7 - (offset & 7));
    }
    bitBuffer->bitOffsetOrWidth = offset;
}

void bb_setBits(BitBucket *bitBuffer, uint32_t val, int offset, int len) {
    for (int i = len - 1; i >= 0; i--, offset++) {
        bitBuffer->data[offset >> 3] |= ((val >> i) & 1) << (7 - (offset & 7));
    }
}

void bb_setBit(BitBucket *bitGrid, uint32_t x, uint32_t y, bool on) {
    uint32_t offset = y * bitGrid->bitOffsetOrWidth + x;
    char mask = 1 << (7 - (offset & 0x07));
    if (on) {
        bitGrid->data[offset >> 3] |= mask;
    } else {
        bitGrid->data[offset >> 3] &= ~mask;
    }
}

void bb_invertBit(BitBucket *bitGrid, uint32_t x, uint32_t y, bool invert) {
    uint32_t offset = y * bitGrid->bitOffsetOrWidth + x;
    char mask = 1 << (7 - (offset & 0x07));
    bool on = ((bitGrid->data[offset >> 3] & (1 << (7 - (offset & 0x07)))) != 0);
    if (on ^ invert) {
        bitGrid->data[offset >> 3] |= mask;
    } else {
        bitGrid->data[offset >> 3] &= ~mask;
    }
}

bool bb_getBit(BitBucket *bitGrid, uint32_t x, uint32_t y) {
    uint32_t offset = y * bitGrid->bitOffsetOrWidth + x;
    return (bitGrid->data[offset >> 3] & (1 << (7 - (offset & 0x07)))) != 0;
}


#pragma mark - Drawing Patterns

// XORs the data modules in this QR Code with the given mask pattern. Due to XOR's mathematical
// properties, calling applyMask(m) twice with the same value is equivalent to no change at all.
// This means it is possible to apply a mask, undo it, and try another mask. Note that a final
// well-formed QR Code symbol needs exactly one mask applied (not zero, not two, etc.).
static void applyMask(BitBucket *modules, BitBucket *isFunction, uint8_t mask) {
    unsigned char size = modules->bitOffsetOrWidth;
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (bb_getBit(isFunction, x, y)) { continue; }
            
            bool invert = 0;
            switch (mask) {
                case 0:  invert = (x + y) % 2 == 0;                    break;
                case 1:  invert = y % 2 == 0;                          break;
                case 2:  invert = x % 3 == 0;                          break;
                case 3:  invert = (x + y) % 3 == 0;                    break;
                case 4:  invert = (x / 3 + y / 2) % 2 == 0;            break;
                case 5:  invert = x * y % 2 + x * y % 3 == 0;          break;
                case 6:  invert = (x * y % 2 + x * y % 3) % 2 == 0;    break;
                case 7:  invert = ((x + y) % 2 + x * y % 3) % 2 == 0;  break;
            }
            bb_invertBit(modules, x, y, invert);
        }
    }
}

void setFunctionModule(BitBucket *modules, BitBucket *isFunction, int x, int y, bool on) {
    bb_setBit(modules, x, y, on);
    bb_setBit(isFunction, x, y, true);
}

// Draws a 9*9 finder pattern including the border separator, with the center module at (x, y).
static void drawFinderPattern(BitBucket *modules, BitBucket *isFunction, int x, int y) {
    int32_t size = modules->bitOffsetOrWidth;

    for (int i = -4; i <= 4; i++) {
        for (int j = -4; j <= 4; j++) {
            int dist = max(abs(i), abs(j));  // Chebyshev/infinity norm
            int xx = x + j, yy = y + i;
            if (0 <= xx && xx < size && 0 <= yy && yy < size) {
                setFunctionModule(modules, isFunction, xx, yy, dist != 2 && dist != 4);
            }
        }
    }
}

// Draws a 5*5 alignment pattern, with the center module at (x, y).
static void drawAlignmentPattern(BitBucket *modules, BitBucket *isFunction, int x, int y) {
    for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
            setFunctionModule(modules, isFunction, x + j, y + i, max(abs(i), abs(j)) != 1);
        }
    }
}

// Draws two copies of the format bits (with its own error correction code)
// based on the given mask and this object's error correction level field.
static void drawFormatBits(BitBucket *modules, BitBucket *isFunction, uint8_t ecc, uint8_t mask) {
    
    int32_t size = modules->bitOffsetOrWidth;

    // Calculate error correction code and pack bits
    int data = ecc << 3 | mask;  // errCorrLvl is uint2, mask is uint3
    int rem = data;
    for (int i = 0; i < 10; i++) {
        rem = (rem << 1) ^ ((rem >> 9) * 0x537);
    }
    
    data = data << 10 | rem;
    data ^= 0x5412;  // uint15
    
    // Draw first copy
    for (int i = 0; i <= 5; i++) {
        setFunctionModule(modules, isFunction, 8, i, ((data >> i) & 1) != 0);
    }
    
    setFunctionModule(modules, isFunction, 8, 7, ((data >> 6) & 1) != 0);
    setFunctionModule(modules, isFunction, 8, 8, ((data >> 7) & 1) != 0);
    setFunctionModule(modules, isFunction, 7, 8, ((data >> 8) & 1) != 0);
    
    for (int i = 9; i < 15; i++) {
        setFunctionModule(modules, isFunction, 14 - i, 8, ((data >> i) & 1) != 0);
    }
    
    // Draw second copy
    for (int i = 0; i <= 7; i++) {
        setFunctionModule(modules, isFunction, size - 1 - i, 8, ((data >> i) & 1) != 0);
    }
    
    for (int i = 8; i < 15; i++) {
        setFunctionModule(modules, isFunction, 8, size - 15 + i, ((data >> i) & 1) != 0);
    }
    
    setFunctionModule(modules, isFunction, 8, size - 8, true);
}


// Draws two copies of the version bits (with its own error correction code),
// based on this object's version field (which only has an effect for 7 <= version <= 40).
static void drawVersion(BitBucket *modules, BitBucket *isFunction, uint8_t version) {
    
    int32_t size = modules->bitOffsetOrWidth;

#if LOCK_VERSION != 0 && LOCK_VERSION < 7
    return;
    
#else
    if (version < 7) { return; }
    
    // Calculate error correction code and pack bits
    int rem = version;  // version is uint6, in the range [7, 40]
    for (int i = 0; i < 12; i++) {
        rem = (rem << 1) ^ ((rem >> 11) * 0x1F25);
    }
    
    int data = version << 12 | rem;  // uint18
    
    // Draw two copies
    for (int i = 0; i < 18; i++) {
        bool bit = ((data >> i) & 1) != 0;
        int a = size - 11 + i % 3, b = i / 3;
        setFunctionModule(modules, isFunction, a, b, bit);
        setFunctionModule(modules, isFunction, b, a, bit);
    }
    
#endif
}

static void drawFunctionPatterns(BitBucket *modules, BitBucket *isFunction, uint8_t version, uint8_t ecc) {
    
    int32_t size = modules->bitOffsetOrWidth;

    // Draw the horizontal and vertical timing patterns
    for (int i = 0; i < size; i++) {
        setFunctionModule(modules, isFunction, 6, i, i % 2 == 0);
        setFunctionModule(modules, isFunction, i, 6, i % 2 == 0);
    }
    
    // Draw 3 finder patterns (all corners except bottom right; overwrites some timing modules)
    drawFinderPattern(modules, isFunction, 3, 3);
    drawFinderPattern(modules, isFunction, size - 4, 3);
    drawFinderPattern(modules, isFunction, 3, size - 4);
    
#if LOCK_VERSION ==0 || LOCK_VERSION > 1

    if (version > 1) {

        // Draw the numerous alignment patterns
        
        int alignCount = version / 7 + 2;
        int step;
        if (version != 32) {
            step = (version * 4 + alignCount * 2 + 1) / (2 * alignCount - 2) * 2;  // ceil((size - 13) / (2*numAlign - 2)) * 2
        } else { // C-C-C-Combo breaker!
            step = 26;
        }
        
        uint8_t alignPositionIndex = alignCount - 1;
        uint8_t alignPosition[alignCount];
        
        alignPosition[0] = 6;
        
        int size = version * 4 + 17;
        for (int i = 0, pos = size - 7; i < alignCount - 1; i++, pos -= step) {
            alignPosition[alignPositionIndex--] = pos;
        }
        
        for (int i = 0; i < alignCount; i++) {
            for (int j = 0; j < alignCount; j++) {
                if ((i == 0 && j == 0) || (i == 0 && j == alignCount - 1) || (i == alignCount - 1 && j == 0)) {
                    continue;  // Skip the three finder corners
                } else {
                    drawAlignmentPattern(modules, isFunction, alignPosition[i], alignPosition[j]);
                }
            }
        }
    }
    
#endif
    
    // Draw configuration data
    drawFormatBits(modules, isFunction, ecc, 0);  // Dummy mask value; overwritten later in the constructor
    drawVersion(modules, isFunction, version);
}


// Draws the given sequence of 8-bit codewords (data and error correction) onto the entire
// data area of this QR Code symbol. Function modules need to be marked off before this is called.
static void drawCodewords(BitBucket *modules, BitBucket *isFunction, BitBucket *codewords) {
    
    int bitLength = codewords->bitOffsetOrWidth;
    uint8_t *data = codewords->data;
    
    int32_t size = modules->bitOffsetOrWidth;
    
    // Bit index into the data
    size_t i = 0;
    
    // Do the funny zigzag scan
    for (int right = size - 1; right >= 1; right -= 2) {  // Index of right column in each column pair
        if (right == 6) { right = 5; }
        
        for (int vert = 0; vert < size; vert++) {  // Vertical counter
            for (int j = 0; j < 2; j++) {
                int x = right - j;  // Actual x coordinate
                bool upwards = ((right & 2) == 0) ^ (x < 6);
                int y = upwards ? size - 1 - vert : vert;  // Actual y coordinate
                if (!bb_getBit(isFunction, x, y) && i < bitLength) {
                    bb_setBit(modules, x, y, ((data[i >> 3] >> (7 - (i & 7))) & 1) != 0);
                    i++;
                }
                // If there are any remainder bits (0 to 7), they are already
                // set to 0/false/white when the grid of modules was initialized
            }
        }
    }
}



#pragma mark - Penalty Calculation

#define PENALTY_N1      3
#define PENALTY_N2      3
#define PENALTY_N3     40
#define PENALTY_N4     10

// Calculates and returns the penalty score based on state of this QR Code's current modules.
// This is used by the automatic mask choice algorithm to find the mask pattern that yields the lowest score.
static int getPenaltyScore(BitBucket *modules) {
    int result = 0;
    
    unsigned char size = modules->bitOffsetOrWidth;
    
    // Adjacent modules in row having same color
    for (int y = 0; y < size; y++) {
        
        bool colorX = bb_getBit(modules, 0, y);
        for (int x = 1, runX = 1; x < size; x++) {
            bool cx = bb_getBit(modules, x, y);
            if (cx != colorX) {
                colorX = cx;
                runX = 1;
                
            } else {
                runX++;
                if (runX == 5) {
                    result += PENALTY_N1;
                } else if (runX > 5) {
                    result++;
                }
            }
        }
    }
    
    // Adjacent modules in column having same color
    for (int x = 0; x < size; x++) {
        bool colorY = bb_getBit(modules, x, 0);
        for (int y = 1, runY = 1; y < size; y++) {
            bool cy = bb_getBit(modules, x, y);
            if (cy != colorY) {
                colorY = cy;
                runY = 1;
            } else {
                runY++;
                if (runY == 5) {
                    result += PENALTY_N1;
                } else if (runY > 5) {
                    result++;
                }
            }
        }
    }
    
    // 2*2 blocks of modules having same color
    for (int y = 0; y < size - 1; y++) {
        for (int x = 0; x < size - 1; x++) {
            bool  color = bb_getBit(modules, x, y);
            if (color == bb_getBit(modules, x + 1, y) && color == bb_getBit(modules, x, y + 1) && color == bb_getBit(modules, x + 1, y + 1)) {
                result += PENALTY_N2;
            }
        }
    }
    
    // Finder-like pattern in rows
    for (int y = 0; y < size; y++) {
        for (int x = 0, bits = 0; x < size; x++) {
            bits = ((bits << 1) & 0x7FF) | (bb_getBit(modules, x, y) ? 1 : 0);
            if (x >= 10 && (bits == 0x05D || bits == 0x5D0)) { // Needs 11 bits accumulated
                result += PENALTY_N3;
            }
        }
    }
    
    // Finder-like pattern in columns
    for (int x = 0; x < size; x++) {
        for (int y = 0, bits = 0; y < size; y++) {
            bits = ((bits << 1) & 0x7FF) | (bb_getBit(modules, x, y) ? 1 : 0);
            if (y >= 10 && (bits == 0x05D || bits == 0x5D0)) { // Needs 11 bits accumulated
                result += PENALTY_N3;
            }
        }
    }
    
    // Balance of black and white modules
    int black = 0;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (bb_getBit(modules, x, y)) {
                black++;
            }
        }
    }
    int total = size * size;
    // Find smallest k such that (45-5k)% <= dark/total <= (55+5k)%
    for (int k = 0; black*20 < (9-k)*total || black*20 > (11+k)*total; k++) {
        result += PENALTY_N4;
    }
    
    return result;
}


#pragma mark - ReedSolomonGenerator

uint8_t rs_multiply(uint8_t x, uint8_t y) {
    // Russian peasant multiplication
    int z = 0;
    for (int i = 7; i >= 0; i--) {
        z = (z << 1) ^ ((z >> 7) * 0x11D);
        z ^= ((y >> i) & 1) * x;
    }
    return z;
}

void rs_init(uint32_t degree, uint8_t *coeff) {
    memset(coeff, 0, degree);
    coeff[degree - 1] = 1;
    
    // Compute the product polynomial (x - r^0) * (x - r^1) * (x - r^2) * ... * (x - r^{degree-1}),
    // drop the highest term, and store the rest of the coefficients in order of descending powers.
    // Note that r = 0x02, which is a generator element of this field GF(2^8/0x11D).
    int root = 1;
    for (int i = 0; i < degree; i++) {
        // Multiply the current product by (x - r^i)
        for (size_t j = 0; j < degree; j++) {
            coeff[j] = rs_multiply(coeff[j], root);
            if (j + 1 < degree) {
                coeff[j] ^= coeff[j + 1];
            }
        }
        root = (root << 1) ^ ((root >> 7) * 0x11D);  // Multiply by 0x02 mod GF(2^8/0x11D)
    }
}

void rs_getRemainder(uint32_t degree, uint8_t *coeff, uint8_t *data, int length, uint8_t *result, uint8_t stride) {
    // Compute the remainder by performing polynomial division
    
    //for (uint8_t i = 0; i < degree; i++) { result[] = 0; }
    //memset(result, 0, degree);
    
    for (size_t i = 0; i < length; i++) {
        uint8_t factor = data[i] ^ result[0];
        for (uint8_t j = 1; j < degree; j++) {
            result[(j - 1) * stride] = result[j * stride];
        }
        result[(degree - 1) * stride] = 0;
        
        for (size_t j = 0; j < degree; j++) {
            result[j * stride] ^= rs_multiply(coeff[j], factor);
        }
    }
}



#pragma mark - QrCode

static int8_t encodeDataCodewords(BitBucket *dataCodewords, const uint8_t *text, uint32_t length, uint8_t version) {
    
    uint32_t charCount = 0;
    int8_t mode = MODE_BYTE;
    
    if (isNumeric((char*)text)) {
        mode = MODE_NUMERIC;
        bb_appendBits(dataCodewords, 1 << MODE_NUMERIC, 4);
        bb_appendBits(dataCodewords, 0, getModeBits(version, MODE_NUMERIC));
        
        int accumData = 0;
        int accumCount = 0;
        for (; *text != '\0'; text++, charCount++) {
            char c = *text;
            accumData = accumData * 10 + (c - '0');
            accumCount++;
            if (accumCount == 3) {
                bb_appendBits(dataCodewords, accumData, 10);
                accumData = 0;
                accumCount = 0;
            }
        }
        
        // 1 or 2 digits remaining
        if (accumCount > 0) {
            bb_appendBits(dataCodewords, accumData, accumCount * 3 + 1);
        }
        
    } else if (isAlphanumeric((char*)text)) {
        mode = MODE_ALPHANUMERIC;
        bb_appendBits(dataCodewords, 1 << MODE_ALPHANUMERIC, 4);
        bb_appendBits(dataCodewords, 0, getModeBits(version, MODE_ALPHANUMERIC));
        
        int accumData = 0;
        int accumCount = 0;
        for (; *text != '\0'; text++, charCount++) {
            accumData = accumData * 45 + getAlphanumeric(*text);
            accumCount++;
            if (accumCount == 2) {
                bb_appendBits(dataCodewords, accumData, 11);
                accumData = 0;
                accumCount = 0;
            }
        }
        
        // 1 character remaining
        if (accumCount > 0) {
            bb_appendBits(dataCodewords, accumData, 6);
        }
        
    } else {
        bb_appendBits(dataCodewords, 1 << MODE_BYTE, 4);
        bb_appendBits(dataCodewords, 0, getModeBits(version, MODE_BYTE));
        for (; *text != '\0'; text++, charCount++) {
            char c = (*text);
            bb_appendBits(dataCodewords, c, 8);
        }
    }
    
    bb_setBits(dataCodewords, charCount, 4, getModeBits(version, mode));
    
    return mode;
}

static void performErrorCorrection(uint8_t version, uint8_t ecc, BitBucket *data) {
    
    // See: http://www.thonky.com/qr-code-tutorial/structure-final-message
    
    uint32_t moduleCount = getNumRawDataModules(version);
    
#if LOCK_VERSION == 0
    int numBlocks = NUM_ERROR_CORRECTION_BLOCKS[ecc][version - 1];
    int totalEcc = NUM_ERROR_CORRECTION_CODEWORDS[ecc][version - 1];
#else
    int numBlocks = NUM_ERROR_CORRECTION_BLOCKS[ecc];
    int totalEcc = NUM_ERROR_CORRECTION_CODEWORDS[ecc];
#endif
    
    int blockEccLen = totalEcc / numBlocks;
    int numShortBlocks = numBlocks - moduleCount / 8 % numBlocks;
    int shortBlockLen = moduleCount / 8 / numBlocks;
    
    uint32_t shortDataBlockLen = shortBlockLen - blockEccLen;
    
    uint8_t result[data->capacityBytes];
    memset(result, 0, sizeof(result));
    
    uint8_t coeff[blockEccLen];
    rs_init(blockEccLen, coeff);
    
    uint32_t offset = 0;
    uint8_t *dataBytes = data->data;
    
    
    // Interleave all short blocks
    for (int32_t i = 0; i < shortDataBlockLen; i++) {
        uint32_t index = i;
        uint32_t stride = shortDataBlockLen;
        for (int32_t blockNum = 0; blockNum < numBlocks; blockNum++) {
            result[offset++] = dataBytes[index];
            
#if LOCK_VERSION == 0 || LOCK_VERSION >= 5
            if (blockNum == numShortBlocks) { stride++; }
#endif
            index += stride;
        }
    }
    
    // Version less than 5 only have short blocks
#if LOCK_VERSION == 0 || LOCK_VERSION >= 5
    {
        // Interleave long blocks
        uint32_t index = shortDataBlockLen * (numShortBlocks + 1);
        uint32_t stride = shortDataBlockLen;
        for (int32_t blockNum = 0; blockNum < numBlocks - numShortBlocks; blockNum++) {
            result[offset++] = dataBytes[index];
            
            if (blockNum == 0) { stride++; }
            index += stride;
        }
    }
#endif
    
    // Add all ecc blocks, interleaved
    uint8_t blockSize = shortDataBlockLen;
    for (int32_t blockNum = 0; blockNum < numBlocks; blockNum++) {
        
#if LOCK_VERSION == 0 || LOCK_VERSION >= 5
        if (blockNum == numShortBlocks) { blockSize++; }
#endif
        
        rs_getRemainder(blockEccLen, coeff, dataBytes, blockSize, &result[offset + blockNum], numBlocks);
        dataBytes += blockSize;
    }
    
    memcpy(data->data, result, data->capacityBytes);
    data->bitOffsetOrWidth = moduleCount;
}

// We store the Format bits tightly packed into a single byte (each of the 4 modes is 2 bits)
// The format bits can be determined by ECC_FORMAT_BITS >> (2 * ecc)
static const uint8_t ECC_FORMAT_BITS = (0x02 << 6) | (0x03 << 4) | (0x00 << 2) | (0x01 << 0);


#pragma mark - Public QRCode functions

uint32_t qrcode_getBufferSize(uint8_t version) {
    //printf("FOO: %d %d\n", bb_getGridSizeBytes(4 * version + 17), getNumRawDataModules(version));
    return bb_getGridSizeBytes(4 * version + 17);
}

// @TODO: Return error if data is too big.
int8_t qrcode_initBytes(QRCode *qrcode, uint8_t *modules, uint8_t version, uint8_t ecc, uint8_t *data, uint16_t length) {
    uint8_t size = version * 4 + 17;
    qrcode->version = version;
    qrcode->size = size;
    qrcode->ecc = ecc;
    qrcode->modules = modules;
    
    uint8_t eccFormatBits = (ECC_FORMAT_BITS >> (2 * ecc)) & 0x03;
    
#if LOCK_VERSION == 0
    uint32_t moduleCount = getNumRawDataModules(version);
    int dataCapacity = moduleCount / 8 - NUM_ERROR_CORRECTION_CODEWORDS[eccFormatBits][version - 1];
#else
    version = LOCK_VERSION;
    uint32_t moduleCount = getNumRawDataModules(LOCK_VERSION);
    int dataCapacity = moduleCount / 8 - NUM_ERROR_CORRECTION_CODEWORDS[eccFormatBits];
#endif
    
    struct BitBucket codewords;
    uint8_t codewordBytes[bb_getBufferSizeBytes(moduleCount)];
    bb_initBuffer(&codewords, codewordBytes, (int32_t)sizeof(codewordBytes));
    
    // Place the data code words into the buffer
    int8_t mode = encodeDataCodewords(&codewords, data, length, version);
    
    if (mode < 0) { return -1; }
    qrcode->mode = mode;
    
    // Add terminator and pad up to a byte if applicable
    uint32_t padding = (dataCapacity * 8) - codewords.bitOffsetOrWidth;
    if (padding > 4) { padding = 4; }
    bb_appendBits(&codewords, 0, padding);
    bb_appendBits(&codewords, 0, (8 - codewords.bitOffsetOrWidth % 8) % 8);

    // Pad with alternate bytes until data capacity is reached
    for (uint8_t padByte = 0xEC; codewords.bitOffsetOrWidth < (dataCapacity * 8); padByte ^= 0xEC ^ 0x11) {
        bb_appendBits(&codewords, padByte, 8);
    }

    BitBucket modulesGrid;
    bb_initGrid(&modulesGrid, modules, size);
    
    BitBucket isFunctionGrid;
    uint8_t isFunctionGridBytes[bb_getGridSizeBytes(size)];
    bb_initGrid(&isFunctionGrid, isFunctionGridBytes, size);
    
    // Draw function patterns, draw all codewords, do masking
    drawFunctionPatterns(&modulesGrid, &isFunctionGrid, version, eccFormatBits);
    performErrorCorrection(version, eccFormatBits, &codewords);
    drawCodewords(&modulesGrid, &isFunctionGrid, &codewords);
    
    // Find the best (lowest penalty) mask
    uint8_t mask = 0;
    int32_t minPenalty = INT32_MAX;
    for (uint8_t i = 0; i < 8; i++) {
        drawFormatBits(&modulesGrid, &isFunctionGrid, eccFormatBits, i);
        applyMask(&modulesGrid, &isFunctionGrid, i);
        int penalty = getPenaltyScore(&modulesGrid);
        if (penalty < minPenalty) {
            mask = i;
            minPenalty = penalty;
        }
        applyMask(&modulesGrid, &isFunctionGrid, i);  // Undoes the mask due to XOR
    }
    
    qrcode->mask = mask;
    
    // Overwrite old format bits
    drawFormatBits(&modulesGrid, &isFunctionGrid, eccFormatBits, mask);
    
    // Apply the final choice of mask
    applyMask(&modulesGrid, &isFunctionGrid, mask);

    return 0;
}

int8_t qrcode_initText(QRCode *qrcode, uint8_t *modules, uint8_t version, uint8_t ecc, const char *data) {
    return qrcode_initBytes(qrcode, modules, version, ecc, (uint8_t*)data, strlen(data));
}

bool qrcode_getModule(QRCode *qrcode, uint8_t x, uint8_t y) {
    if (x < 0 || x >= qrcode->size || y < 0 || y >= qrcode->size) {
        return false;
    }

    uint32_t offset = y * qrcode->size + x;
    return (qrcode->modules[offset >> 3] & (1 << (7 - (offset & 0x07)))) != 0;
}
/*
uint8_t qrcode_getHexLength(QRCode *qrcode) {
    return ((qrcode->size * qrcode->size) + 7) / 4;
}

void qrcode_getHex(QRCode *qrcode, char *result) {
    
}
*/
