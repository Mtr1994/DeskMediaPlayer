#include <stdint.h>

typedef struct {
    int64_t pts;
    int width;
    int height;
    int samplesize;
    double timebase;
    uint8_t* d1;
    int width1;
    int height1;
    int linesize1;
    uint8_t* d2;
    int width2;
    int height2;
    int linesize2;
    uint8_t* d3;
    int width3;
    int height3;
    int linesize3;
} VideoFrame;

typedef struct {
    int64_t pts;
    int size;
    double timebase;
    uint8_t* data;
} AudioFrame;
