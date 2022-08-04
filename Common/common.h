#include <stdint.h>

typedef struct {
    int64_t pts;
    int width;
    int height;
    int samplesize;
    double timebase;
    uint8_t* y;
    int linesizey;
    uint8_t* u;
    int linesizeu;
    uint8_t* v;
    int linesizev;
} VideoFrame;

typedef struct {
    int64_t pts;
    int size;
    double timebase;
    uint8_t* data;
} AudioFrame;
