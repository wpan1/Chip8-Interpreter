#define RESOLUTION_H 64
#define RESOLUTION_W 32
#define RESOLUYION_SCALE 10

void cpuInit();
void cpuLoadFile(char *filename, unsigned char *memory_buffer);
void cpuEmulateLoop();
void cpuLoadGame();

// Draw flag
extern unsigned int draw_flag;
// 64 * 32 res black and white output
extern unsigned char gfx_out[RESOLUTION_H * RESOLUTION_W];