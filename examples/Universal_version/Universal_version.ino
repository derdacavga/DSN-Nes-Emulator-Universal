#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <driver/spi_master.h>
#include <esp_heap_caps.h>
#include "hw_config.h"
#include <TFT_eSPI.h>

extern "C" {
#include <nofrendo.h>
  int show_menu();
  const char* get_selected_game();
  bool vid_preload_rom(const char* path);
}

TFT_eSPI tft = TFT_eSPI();
extern void setup_controller();

SPIClass spiSD(SPI2_HOST);

enum SystemState {
  STATE_MAIN_MENU,
  STATE_PLAYING
};

SystemState currentState = STATE_MAIN_MENU;
const char* current_rom = nullptr;
TaskHandle_t emuTaskHandle = NULL;

void emu_task(void *pvParameters) {
  const char* rom = (const char*)pvParameters;
  
#if ENABLE_SOUND
  char* argv[] = {
    (char*)"nes",
    (char*)"-sound",
    (char*)"-volume", (char*)"100",
    (char*)"-sample", (char*)"30000",
    (char*)rom
  };
  nofrendo_main(7, argv);
#else
  char* argv[] = {
    (char*)"nes",
    (char*)"-nosound",
    (char*)rom
  };
  nofrendo_main(3, argv);
#endif

  currentState = STATE_MAIN_MENU;
  vTaskDelete(NULL);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  setup_controller();

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  SD.begin(SD_CS, spiSD, 20000000);
}

void loop() {
  switch (currentState) {
    case STATE_MAIN_MENU:
      {
        tft.fillScreen(TFT_BLACK);
        int selected = show_menu();
        if (selected >= 0) {
          current_rom = get_selected_game();
          currentState = STATE_PLAYING;

          xTaskCreatePinnedToCore(
            emu_task,
            "EmuTask",
            49152,
            (void*)current_rom,
            1,
            &emuTaskHandle,
            0);
        }
        break;
      }

    case STATE_PLAYING:
      {
        delay(100);
        break;
      }
  }
}