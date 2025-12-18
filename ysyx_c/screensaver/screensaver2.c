#include <am.h>
#include <klib-macros.h>

// --- 全屏绘制函数（你的原始实现）---
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

// --- 颜色分量工具 ---
static inline uint8_t R(uint32_t c) { return (c >> 16) & 0xFF; }
static inline uint8_t G(uint32_t c) { return (c >> 8) & 0xFF; }
static inline uint8_t B(uint32_t c) { return c & 0xFF; }
static inline uint32_t RGB(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

// --- 调色板 ---
static uint32_t palette[] = {
  0x000000, // 黑
  0xff0000, // 红
  0x00ff00, // 绿
  0x0000ff, // 蓝
  0xffff00, // 黄
  0xff00ff, // 品红
  0x00ffff, // 青
  0xffffff  // 白
};

// --- 用时间初始化的伪随机数生成器 ---
static unsigned int lcg_seed = 0;

static void srand_time() {
  // 用微秒计时器的低 32 位作为种子
  lcg_seed = (unsigned int)(io_read(AM_TIMER_UPTIME).us);
}

static int rand_lcg() {
  lcg_seed = lcg_seed * 1103515245U + 12345U;
  return (int)((lcg_seed >> 16) & 0x7FFF); // 返回 0~32767
}

// --- 主函数 ---
int main() {
  ioe_init();

  // 用当前时间初始化随机种子（关键！）
  srand_time();

  uint32_t current_color = 0x000000;        // 起始为黑色
  uint32_t target_color = palette[rand_lcg() % 8]; // 初始目标随机
  const int total_steps = 100;              // 渐变步数
  int step = 0;

  unsigned long last_time = 0;
  const int interval_ms = 50;               // 每 50ms 更新一帧

  while (1) {
    unsigned long now = io_read(AM_TIMER_UPTIME).us / 1000; // 转为毫秒

    if (now - last_time >= interval_ms) {
      // 计算当前插值颜色并绘制
      if (step <= total_steps) {
        int t = step;
        uint8_t r = R(current_color) + (R(target_color) - R(current_color)) * t / total_steps;
        uint8_t g = G(current_color) + (G(target_color) - G(current_color)) * t / total_steps;
        uint8_t b = B(current_color) + (B(target_color) - B(current_color)) * t / total_steps;
        draw(RGB(r, g, b));
      }

      step++;

      // 完成一轮渐变后，选择新目标颜色
      if (step > total_steps) {
        current_color = target_color;
        int next_idx;
        do {
          next_idx = rand_lcg() % 8;
        } while (palette[next_idx] == current_color); // 防止重复
        target_color = palette[next_idx];
        step = 0;
      }

      last_time = now;
    }
  }

  return 0;
}
