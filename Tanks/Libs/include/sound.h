#ifndef SOUND_H
#define SOUND_H

#include "miniaudio.h"

// Инициализация звуковой системы
void sound_init(void);
// Проигрывание однократного звука
void sound_play(const char* name);

// Запуск фоновой музыки
void sound_play_music(const char* name);

// Остановка текущей музыки
void sound_stop_music(void);

// Освобождение ресурсов
void sound_cleanup(void);
void sound_play_delayed(const char* name, double delay_seconds);
void sound_update_delayed(double currentTime);
#endif