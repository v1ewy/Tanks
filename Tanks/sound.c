#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "sound.h"
#include <stdio.h>
#include <string.h>

static ma_engine engine;
static ma_sound  currentMusic;

typedef struct {
    char name[64];
    double playTime;
    int active;
} DelayedSound;
static DelayedSound delayedSound = { .active = 0 };

void sound_play_delayed(const char* name, double delay_seconds) {
    delayedSound.active = 1;
    strncpy(delayedSound.name, name, sizeof(delayedSound.name) - 1);
    delayedSound.name[sizeof(delayedSound.name) - 1] = '\0';
    delayedSound.playTime = glfwGetTime() + delay_seconds;
}

void sound_update_delayed(double currentTime) {
    if (delayedSound.active && currentTime >= delayedSound.playTime) {
        sound_play(delayedSound.name);
        delayedSound.active = 0;
    }
}

void sound_init(void) {
    ma_result result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Не удалось инициализировать звуковой движок (код %d)\n", result);
        return;
    }
    printf("Звуковой движок готов\n");
}

void sound_play(const char* name) {
    char path[256];
    snprintf(path, sizeof(path), "Assets/Audio/%s.wav", name);
    ma_result result = ma_engine_play_sound(&engine, path, NULL);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Не удалось воспроизвести звук: %s (код %d)\n", path, result);
    }
}

void sound_play_music(const char* name) {
    sound_stop_music();
    char path[256];
    snprintf(path, sizeof(path), "Assets/Audio/%s.wav", name);
    ma_result result = ma_sound_init_from_file(&engine, path, 0, NULL, NULL, &currentMusic);
    if (result == MA_SUCCESS) {
        ma_sound_set_looping(&currentMusic, MA_TRUE);
        ma_sound_start(&currentMusic);
    }
    else {
        fprintf(stderr, "Не удалось загрузить музыку: %s (код %d)\n", path, result);
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