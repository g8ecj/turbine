



typedef struct smoothing_array_S {
   int16_t * history;
   uint8_t avptr;
   uint8_t avstart;
   int32_t total;
   uint8_t size;
} S_ARRAY;

// defining the size of a history buffer that consists of an array of int16_t
#define BUFSIZE(a) (sizeof(a)/sizeof(*(a)))

void smooth_init (S_ARRAY * ptr, int16_t * hbuff, uint8_t size);
int16_t smooth (S_ARRAY * ptr, int16_t value);


