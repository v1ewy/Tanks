#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "sound.h"
#include <stdio.h>
#include <string.h>

static ma_engine engine;
static ma_sound  currentMusic;

// Таблица соответствия имён файлов 
static const char* sound_files[] = {
    "menu_music", "player_shoot", "bot_shoot", "hit_armor",
    "bot_destroy", "player_death", "wood_break", "game_over", "victory"
};
#define SOUND_COUNT 9

void sound_init(void) {
    ma_result result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Не удалось инициализировать звуковой движок\n");
        return;
    }
    printf("Звуковой движок готов\n");
}

void sound_play(const char* name) {
    char path[256];
    snprintf(path, sizeof(path), "Assets/Audio/%s.wav", name);
    ma_result result = ma_engine_play_sound(&engine, path, NULL);
    if (result != MA_SUCCESS) {
    }
}

// музыка
void sound_play_music(const char* name) {
    // Останавливаем предыдущую музыку, если есть
    sound_stop_music();

    char path[256];
    snprintf(path, sizeof(path), "Assets/Audio/%s.wav", name);
    ma_result result = ma_sound_init_from_file(&engine, path, 0, NULL, NULL, &currentMusic);
    if (result == MA_SUCCESS) {
        ma_sound_set_looping(&currentMusic, MA_TRUE);
        ma_sound_start(&currentMusic);
    }
    else {
        fprintf(stderr, "Не удалось загрузить музыку %s\n", path);
    }
}

void sound_stop_music(void) {
    if (ma_sound_is_playing(&currentMusic)) {
        ma_sound_stop(&currentMusic);
    }
    ma_sound_uninit(&currentMusic);
    memset(&currentMusic, 0, sizeof(currentMusic));
}

void sound_cleanup(void) {
    sound_stop_music();
    ma_engine_uninit(&engine);
}
