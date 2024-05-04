#ifndef AUDIO_H
#define AUDIO_H

#include <SFML/Audio.hpp>

extern sf::SoundBuffer audio_buffer; // Declare as extern


int main()
{
    if (!audio_buffer.loadFromFile("sound.wav")){
        return -1;
    }
}

#endif // AUDIO_H