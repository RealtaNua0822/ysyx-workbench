#include <am.h>
#include <klib-macros.h>

void draw(uint32_t color) {
  AM_GPU_CONFIG_T cfg = io_read(AM_GPU_CONFIG);
  int w = cfg.width, h = cfg.height;
  static uint32_t buf[400 * 300];

  if (w * h > 400 * 300) return;

  for (int i = 0; i < w * h; i++) {
    buf[i] = color;
  }

  io_write(AM_GPU_FBDRAW, 0, 0, buf, w, h, true);
}

int main() {
  ioe_init();
  while (1) {
    draw(0x0000ff); // blue
  }
  return 0;
}
