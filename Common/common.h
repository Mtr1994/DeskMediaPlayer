#include <stdint.h>

typedef struct
{
    int64_t pts;
    int width;
    int height;
    int samplesize;
    double timebase;
    uint8_t* buffer;
    int linesize;
} VideoFrame;

typedef struct
{
    int64_t pts;
    int size;
    double timebase;
    uint8_t* data;
    int32_t samples;
} AudioFrame;
