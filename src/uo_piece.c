#include "uo_piece.h"
#include "uo_position.h"

#include <stddef.h>

//                                      0b00000000
//                                        b PNBRQK
const uo_piece uo_piece___P = 0xa0; // P 0b10100000
const uo_piece uo_piece__N = 0x90; // N 0b10010000
const uo_piece uo_piece__B = 0x88; // B 0b10001000
const uo_piece uo_piece__R = 0x84; // R 0b10000100
const uo_piece uo_piece__Q = 0x82; // Q 0b10000010
const uo_piece uo_piece__K = 0x81; // K 0b10000001
const uo_piece uo_piece__p = 0x20; // p 0b00100000
const uo_piece uo_piece__n = 0x10; // n 0b00010000
const uo_piece uo_piece__b = 0x08; // b 0b00001000
const uo_piece uo_piece__r = 0x04; // r 0b00000100
const uo_piece uo_piece__q = 0x02; // q 0b00000010
const uo_piece uo_piece__k = 0x01; // k 0b00000001

#define uo_piece___P 0xa0 // P 0b10100000
#define uo_piece___N 0x90 // N 0b10010000
#define uo_piece___B 0x88 // B 0b10001000
#define uo_piece___R 0x84 // R 0b10000100
#define uo_piece___Q 0x82 // Q 0b10000010
#define uo_piece___K 0x81 // K 0b10000001
#define uo_piece___p 0x20 // p 0b00100000
#define uo_piece___n 0x10 // n 0b00010000
#define uo_piece___b 0x08 // b 0b00001000
#define uo_piece___r 0x04 // r 0b00000100
#define uo_piece___q 0x02 // q 0b00000010
#define uo_piece___k 0x01 // k 0b00000001

const char uo_piece_to_char[0x100] = {
    [uo_piece___P] = 'P',
    [uo_piece___N] = 'N',
    [uo_piece___B] = 'B',
    [uo_piece___R] = 'R',
    [uo_piece___Q] = 'Q',
    [uo_piece___K] = 'K',
    [uo_piece___p] = 'p',
    [uo_piece___n] = 'n',
    [uo_piece___b] = 'b',
    [uo_piece___r] = 'r',
    [uo_piece___q] = 'q',
    [uo_piece___k] = 'k'};

const uo_piece uo_piece_from_char[0x100] = {
    ['P'] = uo_piece___P,
    ['N'] = uo_piece___N,
    ['B'] = uo_piece___B,
    ['R'] = uo_piece___R,
    ['Q'] = uo_piece___Q,
    ['K'] = uo_piece___K,
    ['p'] = uo_piece___p,
    ['n'] = uo_piece___n,
    ['b'] = uo_piece___b,
    ['r'] = uo_piece___r,
    ['q'] = uo_piece___q,
    ['k'] = uo_piece___k};

const size_t uo_position__piece_bitboard_offset[0x100] = {
    [uo_piece___P] = offsetof(uo_position, p),
    [uo_piece___N] = offsetof(uo_position, n),
    [uo_piece___B] = offsetof(uo_position, b),
    [uo_piece___R] = offsetof(uo_position, r),
    [uo_piece___Q] = offsetof(uo_position, q),
    [uo_piece___K] = offsetof(uo_position, k),
    [uo_piece___p] = offsetof(uo_position, p),
    [uo_piece___n] = offsetof(uo_position, n),
    [uo_piece___b] = offsetof(uo_position, b),
    [uo_piece___r] = offsetof(uo_position, r),
    [uo_piece___q] = offsetof(uo_position, q),
    [uo_piece___k] = offsetof(uo_position, k)};