#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include "hw_config.h"
#include "turning.h"
#include <TFT_eSPI.h>

#if ENABLE_SOUND
#include <driver/i2s.h>
#endif

#include <string.h>
#include <stdarg.h>

extern "C" {
#include "noftypes.h"
#include "bitmap.h"
#include "osd.h"
#include "nofrendo.h"
#include "nesinput.h"
#include "event.h"
#include "nofconfig.h"
}

#define NES_SCREEN_WIDTH 256
#define NES_SCREEN_HEIGHT 240
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define AUDIO_SAMPLE_RATE 30000
#define TARGET_FRAME_MICROS 16666

int master_volume = 100;
bool show_fps = false;
bool select_pressed = false;
static bool runtime_sound_enabled = false;

extern TFT_eSPI tft;
extern "C" int nes_get_gamepad_state();

static uint16_t scale_color_565(uint16_t color, uint8_t alpha) {
  uint8_t r = (color >> 11) & 0x1F;
  uint8_t g = (color >> 5) & 0x3F;
  uint8_t b = color & 0x1F;

  r = (r * alpha) / 255;
  g = (g * alpha) / 255;
  b = (b * alpha) / 255;

  return (uint16_t)((r << 11) | (g << 5) | b);
}

static void draw_magic(uint8_t alpha) {
  static uint16_t line[hakki_W];
  
  int offset_x = (DISPLAY_WIDTH - hakki_W) / 2;
  int offset_y = (DISPLAY_HEIGHT - hakki_H) / 2;

  for (int y = 0; y < hakki_H; y++) {
    int row = y * hakki_W;
    for (int x = 0; x < hakki_W; x++) {
      uint16_t c = pgm_read_word(&hakki[row + x]);
      line[x] = scale_color_565(c, alpha);
    }
    
    tft.setSwapBytes(true); 
    tft.pushImage(offset_x, offset_y + y, hakki_W, 1, line);
  }
}

static void show_magic() {
  static bool shown = false;
  if (shown) {
    return;
  }
  shown = true;

  const int fade_in_ms = 300;
  const int hold_ms = 500;
  const int fade_out_ms = 500;
  const int steps = 30;
  const int step_delay_in = fade_in_ms / steps;
  const int step_delay_out = fade_out_ms / steps;

  tft.fillScreen(0x0000);
  for (int i = 0; i <= steps; i++) {
    uint8_t alpha = (uint8_t)((i * 255) / steps);
    draw_magic(alpha);
    delay(step_delay_in);
  }

  delay(hold_ms);

  for (int i = steps; i >= 0; i--) {
    uint8_t alpha = (uint8_t)((i * 255) / steps);
    draw_magic(alpha);
    delay(step_delay_out);
  }

  tft.fillScreen(0x0000);
}

static int16_t stereo_buffer[4096];
static int16_t mono_buffer[4096];
static void (*emulator_audio_callback)(void *buffer, int length) = NULL;

static uint16_t myPalette565[256];
static bitmap_t *game_bitmap = NULL;
static bool video_ready = false;
static uint16_t *frame_buffer = NULL;  
static bool low_mem_video_mode = false;

unsigned long frame_start_time = 0;
unsigned long frame_count = 0;
unsigned long last_fps_update = 0;
int current_fps = 0;

#define INP_JOYPAD0 0x0001
static nesinput_t joypad_p1;

#define HW_MASK_A 0x01
#define HW_MASK_B 0x02
#define HW_MASK_SELECT 0x04
#define HW_MASK_START 0x08
#define HW_MASK_UP 0x10
#define HW_MASK_DOWN 0x20
#define HW_MASK_LEFT 0x40
#define HW_MASK_RIGHT 0x80

#define INP_PAD_A 0x01
#define INP_PAD_B 0x02
#define INP_PAD_SELECT 0x04
#define INP_PAD_START 0x08
#define INP_PAD_UP 0x10
#define INP_PAD_DOWN 0x20
#define INP_PAD_LEFT 0x40
#define INP_PAD_RIGHT 0x80

esp_timer_handle_t nes_timer_handle = NULL;
void (*emu_timer_callback)(void) = NULL;

void IRAM_ATTR timer_callback_handler(void *arg) {
  if (emu_timer_callback) emu_timer_callback();
}

extern char *global_rom_data;
extern int global_rom_size;

extern "C" bool vid_preload_rom(const char *path);
extern "C" int osd_rom_open(const char *path);
extern "C" int osd_rom_read(void *dst, int len);
extern "C" void osd_rom_close(void);

#define MAX_GAMES 50
struct {
  char names[MAX_GAMES][32];
  int count;
  int selected;
} game_list = {{}, 0, 0};

int menu_volume = 100;

void scan_games() {
  File root = SD.open("/");
  game_list.count = 0;
  
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    
    if (!entry.isDirectory() && game_list.count < MAX_GAMES) {
      String name = entry.name();
      if (name.endsWith(".nes") || name.endsWith(".NES")) {
        strncpy(game_list.names[game_list.count], entry.name(), 31);
        game_list.names[game_list.count][31] = '\0';
        game_list.count++;
      }
    }
    entry.close();
  }
  root.close();
}

extern "C" int show_menu() {
  show_magic();
  scan_games();
  
  if (game_list.count == 0) {
    tft.fillScreen(0xF800); 
    delay(3000);
    return -1;
  }
  
  game_list.selected = 0;
  int top_index = 0; 
  bool in_menu = true;
  unsigned long last_input = 0;
  int last_selected = -1;
  
  bool needs_full_redraw = true; 
  bool needs_list_redraw = false; 

  auto draw_menu_item = [&](int screen_idx, int game_idx, bool selected) {
    int y_pos = 50 + (screen_idx * 40);
    uint16_t bg = selected ? 0xF800 : 0x0000;
    uint16_t fg = 0xFFFF;
    
    char display_name[32];
    strncpy(display_name, game_list.names[game_idx], 31);
    display_name[31] = '\0';
    char *dot = strchr(display_name, '.');
    if (dot) *dot = '\0';

    tft.fillRect(0, y_pos - 5, DISPLAY_WIDTH, 35, bg);
    tft.setTextColor(fg, bg);
    tft.drawString(display_name, 20, y_pos, 1);
  };
  
  while (in_menu) {
    if (needs_full_redraw) {
      tft.fillScreen(0x0000); 
      tft.setTextColor(0xFFFF, 0x0000);
      tft.drawString("GAME MENU", 10, 10, 2);
      
      for (int i = 0; i < 4 && (top_index + i) < game_list.count; i++) {
        draw_menu_item(i, top_index + i, (top_index + i) == game_list.selected);
      }
      
      tft.setTextColor(0x07E0, 0x0000);
      tft.drawString("UP/DN: Select", 10, DISPLAY_HEIGHT - 40, 1);
      tft.drawString("A: Play  SEL: Settings", 10, DISPLAY_HEIGHT - 20, 1);
      
      last_selected = game_list.selected;
      needs_full_redraw = false;
      needs_list_redraw = false;
    } 
    else if (needs_list_redraw) {
      tft.fillRect(0, 45, DISPLAY_WIDTH, 155, 0x0000); 
      
      for (int i = 0; i < 4 && (top_index + i) < game_list.count; i++) {
        draw_menu_item(i, top_index + i, (top_index + i) == game_list.selected);
      }
      
      last_selected = game_list.selected;
      needs_list_redraw = false;
    } 
    else if (last_selected != game_list.selected) {
      if (last_selected >= top_index && last_selected < top_index + 4) {
        draw_menu_item(last_selected - top_index, last_selected, false);
      }
      draw_menu_item(game_list.selected - top_index, game_list.selected, true);
      last_selected = game_list.selected;
    }
    
    int hw = nes_get_gamepad_state();
    
    if ((millis() - last_input) > 150) { 
      if (hw & HW_MASK_UP) {
        if (game_list.selected > 0) {
          game_list.selected--;
          if (game_list.selected < top_index) {
            top_index = game_list.selected;
            needs_list_redraw = true; 
          }
          last_input = millis();
        }
      }
      if (hw & HW_MASK_DOWN) {
        if (game_list.selected < game_list.count - 1) {
          game_list.selected++;
          if (game_list.selected >= top_index + 4) {
            top_index = game_list.selected - 3;
            needs_list_redraw = true; 
          }
          last_input = millis();
        }
      }
      if (hw & HW_MASK_A) {
        tft.fillScreen(0x0000);  
        delay(300); 
        return game_list.selected;
      }
      if (hw & HW_MASK_SELECT) {
        bool in_settings = true;
        unsigned long settings_input = 0;
        int last_menu_volume = -1;
        bool settings_full_redraw = true;
        
        while (in_settings) {
          if (settings_full_redraw) {
            tft.fillScreen(0x0000);
            tft.setTextColor(0xFFFF, 0x0000);
            tft.drawString("VOLUME SETTINGS", 10, 10, 2);
            
            tft.setTextColor(0x07E0, 0x0000);
            tft.drawString("UP/DN: Adjust", 10, DISPLAY_HEIGHT - 40, 1);
            tft.drawString("A: Save  B: Cancel", 10, DISPLAY_HEIGHT - 20, 1);
            
            settings_full_redraw = false;
            last_menu_volume = -1;
          }

          if (last_menu_volume != menu_volume) {
            char vol_str[20];
            sprintf(vol_str, "Vol: %d%%", menu_volume);
            
            tft.fillRect(20, 60, 200, 20, 0x0000);
            tft.setTextColor(0x07E0, 0x0000);
            tft.drawString(vol_str, 20, 60, 2);

            int bar_width = (menu_volume * (DISPLAY_WIDTH - 40)) / 200;
            tft.fillRect(20, 120, DISPLAY_WIDTH - 40, 30, 0x4208); 
            tft.fillRect(20, 120, bar_width, 30, 0xF800);

            last_menu_volume = menu_volume;
          }
          
          int hw2 = nes_get_gamepad_state();
          
          if ((millis() - settings_input) > 150) {
            if (hw2 & HW_MASK_UP) {
              menu_volume = min(200, menu_volume + 10);
              master_volume = menu_volume;
              settings_input = millis();
            }
            if (hw2 & HW_MASK_DOWN) {
              menu_volume = max(0, menu_volume - 10);
              master_volume = menu_volume;
              settings_input = millis();
            }
            if (hw2 & HW_MASK_A || hw2 & HW_MASK_B) {
              in_settings = false;
            }
          }
          delay(50);
        }
        last_input = millis();
        needs_full_redraw = true; 
      }
    }
    delay(20); 
  }
  return -1;
}

extern "C" const char *get_selected_game() {
  static char game_path[64];
  if (game_list.selected < game_list.count) {
    snprintf(game_path, 64, "/%s", game_list.names[game_list.selected]);
    return game_path;
  }
  return "/mario.nes";
}

extern "C" char *osd_getromdata(void) {
  const char *rom_path = get_selected_game();
  
  if (global_rom_data == NULL) {
    bool loaded = false;
    try {
      loaded = vid_preload_rom(rom_path);
    } catch (...) {
      loaded = false;
    }
    if (!loaded) return NULL;
  }
  return global_rom_data;
}

extern "C" void osd_release_romdata(void) {
  if (global_rom_data) {
    free(global_rom_data);
    global_rom_data = NULL;
    global_rom_size = 0;
  }
}

extern "C" int osd_init_sound(void) {
  if (!runtime_sound_enabled) {
    return 0;
  }
#if ENABLE_SOUND
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = AUDIO_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 500, 
    .use_apll = true, 
    .tx_desc_auto_clear = true
  };
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_DO,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  if (i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) != ESP_OK) {
    return -1;
  }
  if (i2s_set_pin(I2S_NUM_0, &pin_config) != ESP_OK) {
    return -1;
  }
  i2s_set_clk(I2S_NUM_0, AUDIO_SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
  return 0;
#else
  return 0;
#endif
}

extern "C" void osd_getsoundinfo(sndinfo_t *info) {
  info->sample_rate = AUDIO_SAMPLE_RATE;
  info->bps = 16;
}

extern "C" void osd_setsound(void (*playfunc)(void *buffer, int length)) {
  emulator_audio_callback = runtime_sound_enabled ? playfunc : NULL;
}

extern "C" void osd_stopsound(void) {
  if (!runtime_sound_enabled) return;
#if ENABLE_SOUND
  i2s_zero_dma_buffer(I2S_NUM_0);
#endif
}

extern "C" void osd_writesound(void *stream, int len) {}

extern "C" void osd_initvideo(int *lines) {
  *lines = NES_SCREEN_HEIGHT;
}

extern "C" void osd_shutdownvideo() {}

extern "C" void osd_setscreen(int x, int y, int width, int height) {}

extern "C" void osd_setpalette(rgb_t *pal) {
  if (!pal) return;
  for (int i = 0; i < 256; i++) {
    myPalette565[i] = tft.color565(pal[i].r, pal[i].g, pal[i].b);
  }
}

void in_game_menu() {
  tft.fillRect(60, 60, 200, 120, 0x0000);
  tft.drawRect(60, 60, 200, 120, 0xFFFF);
  
  tft.setTextColor(0xFFFF, 0x0000);
  tft.drawString("GAME PAUSED", 90, 80, 2);
  tft.drawString("A: Resume Game", 80, 120, 1);
  tft.drawString("B: Main Menu", 80, 140, 1);

  while (true) {
    int hw = nes_get_gamepad_state();
    if (hw & HW_MASK_A) {
      return;
    }
    if (hw & HW_MASK_B) {
      esp_restart();
    }
    delay(50);
  }
}

extern "C" void osd_getinput(void) {
  int hw = nes_get_gamepad_state();
  int nes_data = 0;

  if ((hw & HW_MASK_START) && (hw & HW_MASK_SELECT)) {
    in_game_menu();
  }

  if (hw & HW_MASK_A) nes_data |= INP_PAD_A;
  if (hw & HW_MASK_B) nes_data |= INP_PAD_B;
  if (hw & HW_MASK_SELECT) nes_data |= INP_PAD_SELECT;
  if (hw & HW_MASK_START) nes_data |= INP_PAD_START;
  if (hw & HW_MASK_UP) nes_data |= INP_PAD_UP;
  if (hw & HW_MASK_DOWN) nes_data |= INP_PAD_DOWN;
  if (hw & HW_MASK_LEFT) nes_data |= INP_PAD_LEFT;
  if (hw & HW_MASK_RIGHT) nes_data |= INP_PAD_RIGHT;
  joypad_p1.data = nes_data;
}

extern "C" int osd_init() {
  joypad_p1.type = INP_JOYPAD0;
  joypad_p1.data = 0;
  
  input_register(&joypad_p1);
  osd_init_sound();
  return 0;
}

extern "C" void osd_shutdown() {
  if (nes_timer_handle) {
    esp_timer_stop(nes_timer_handle);
    esp_timer_delete(nes_timer_handle);
  }
}

extern "C" int osd_installtimer(int freq, void *func, int func_param, void *func2, int func2_param) {
  emu_timer_callback = (void (*)(void))func;
  const esp_timer_create_args_t timer_args = { .callback = &timer_callback_handler, .name = "nes_timer" };
  esp_timer_create(&timer_args, &nes_timer_handle);
  esp_timer_start_periodic(nes_timer_handle, 1000000 / freq);
  return 0;
}

extern "C" int osd_gettime(void) { return millis(); }
extern "C" int osd_makesnapname(char *buf, int len) { return 0; }
extern "C" void osd_getmouse(int *x, int *y, int *button) { *x = 0; *y = 0; *button = 0; }

char *global_rom_data = NULL;
int global_rom_size = 0;
static File rom_stream_file;

extern "C" bool vid_preload_rom(const char *path) {
  if (global_rom_data) {
    free(global_rom_data);
    global_rom_data = NULL;
  }
  
  File file;
  try {
    file = SD.open(path, FILE_READ);
  } catch (...) {
    return false;
  }
  if (!file) return false;
  
  global_rom_size = file.size();
  global_rom_data = (char *)heap_caps_malloc(global_rom_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!global_rom_data) {
    global_rom_data = (char *)malloc(global_rom_size);
  }
  if (!global_rom_data) {
    file.close();
    return false;
  }
  
  size_t bytes_read = file.readBytes(global_rom_data, global_rom_size);
  file.close();
  
  if (bytes_read != global_rom_size) {
    free(global_rom_data);
    global_rom_data = NULL;
    return false;
  }
  return true;
}

extern "C" void IRAM_ATTR osd_blit(bitmap_t *bmp) {
  if (!video_ready || !bmp || !bmp->line || !bmp->line[0] || !frame_buffer) return;
  
  for (int y = 0; y < NES_SCREEN_HEIGHT; y++) {
    uint8_t *src = bmp->line[y];
    uint16_t *dst = &frame_buffer[y * NES_SCREEN_WIDTH];
    for (int x = 0; x < NES_SCREEN_WIDTH; x++) {
      dst[x] = myPalette565[src[x]]; 
    }
  }
}

extern "C" void vid_flush() {
  frame_start_time = micros();

  int x_offset = (DISPLAY_WIDTH - NES_SCREEN_WIDTH) / 2;
  tft.setSwapBytes(true); 

  if (frame_buffer) {
    osd_blit(game_bitmap);
    tft.pushImage(x_offset, 0, NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT, frame_buffer);
  } 

  if (runtime_sound_enabled && emulator_audio_callback) {
    int samples_needed = AUDIO_SAMPLE_RATE / 60;
    int bytes_needed = samples_needed * 2; 

    memset(mono_buffer, 0, sizeof(mono_buffer));
    emulator_audio_callback(mono_buffer, bytes_needed);

#if ENABLE_SOUND
    for (int i = 0; i < samples_needed; i++) {
      int32_t sample = mono_buffer[i];
      sample = (sample * master_volume) / 100;

      if (sample > 32767) sample = 32767;
      if (sample < -32768) sample = -32768;

      stereo_buffer[i * 2] = (int16_t)sample;
      stereo_buffer[i * 2 + 1] = (int16_t)sample;
    }
    
    size_t written;
    i2s_write(I2S_NUM_0, (const char *)stereo_buffer, samples_needed * 4, &written, portMAX_DELAY);
#endif
  } else {
    long elapsed_micros = micros() - frame_start_time;
    if (elapsed_micros < TARGET_FRAME_MICROS) {
      long wait_ms = (TARGET_FRAME_MICROS - elapsed_micros) / 1000;
      if (wait_ms > 0) {
        vTaskDelay(wait_ms / portTICK_PERIOD_MS);
      } else {
        vTaskDelay(1);
      }
    } else {
      vTaskDelay(1);
    }
  }

  vTaskDelay(1);

  frame_count++;
  if (millis() - last_fps_update >= 1000) {
    current_fps = frame_count;
    frame_count = 0;
    last_fps_update = millis();
  }
}

extern "C" int vid_init(int width, int height, viddriver_t *osd_driver) {
  if (!game_bitmap) game_bitmap = bmp_create(NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT, 0); 
  
  if (!frame_buffer) {
    frame_buffer = (uint16_t *)heap_caps_malloc(NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!frame_buffer) {
      frame_buffer = (uint16_t *)malloc(NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT * sizeof(uint16_t));
    }
    if (!frame_buffer) {
      low_mem_video_mode = true;
    }
  }

  video_ready = (game_bitmap && game_bitmap->data);
  return video_ready ? 0 : -1;
}

extern "C" int osd_rom_open(const char *path) {
  if (rom_stream_file) rom_stream_file.close();
  rom_stream_file = SD.open(path, FILE_READ);
  return !rom_stream_file ? -1 : 0;
}

extern "C" int osd_rom_read(void *dst, int len) {
  if (!rom_stream_file || !dst || len <= 0) return -1;
  return rom_stream_file.read((uint8_t *)dst, len);
}

extern "C" void osd_rom_close(void) {
  if (rom_stream_file) rom_stream_file.close();
}

extern "C" void vid_shutdown() {
  video_ready = false;
  if (frame_buffer) {
    free(frame_buffer);
    frame_buffer = NULL;
  }
}

extern "C" int vid_setmode(int width, int height) { return 0; }
extern "C" void vid_setpalette(rgb_t *pal) { osd_setpalette(pal); }
extern "C" bitmap_t *vid_getbuffer() {
  if (!game_bitmap) game_bitmap = bmp_create(NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT, 0);
  video_ready = (game_bitmap && game_bitmap->data);
  return game_bitmap;
}

extern "C" void osd_getvideoinfo(vidinfo_t *info) {
  info->default_width = NES_SCREEN_WIDTH;
  info->default_height = NES_SCREEN_HEIGHT;
  info->driver = 0;
}

extern "C" void osd_togglefullscreen(int code) {}
extern "C" char *osd_newextension(char *string, char *ext) { return string; }
extern "C" void osd_fullname(char *fullname, const char *shortname) { strcpy(fullname, shortname); }

extern "C" int osd_main(int argc, char *argv[]) {
  runtime_sound_enabled = false;
  for (int i = 1; i < argc; i++) {
    if (argv[i] && strcmp(argv[i], "-sound") == 0) runtime_sound_enabled = true;
    if (argv[i] && strcmp(argv[i], "-nosound") == 0) runtime_sound_enabled = false;
  }
  char *rom_name = "rom";
  if (argc > 0) rom_name = argv[argc - 1];
  return main_loop(rom_name, system_nes);
}

extern "C" int nofrendo_log_init(void) { return 0; }
extern "C" void nofrendo_log_shutdown(void) {}
extern "C" int nofrendo_log_print(const char *s) { return 0; }
extern "C" int nofrendo_log_printf(const char *format, ...) { return 0; }
extern "C" void nofrendo_log_assert(int expr, int line, const char *file, char *msg) {}