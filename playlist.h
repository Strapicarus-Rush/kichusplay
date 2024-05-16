#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <SFML/Audio.hpp>

class Playlist
{
public:
    Playlist(bool &debug);
    Playlist();
    void playNextIfFinished();
    void addSongsFromDirectory(const std::string &directoryPath);
    void addSong(const std::string &filename);
    std::string formatFlacMetadata(const std::string &filename);
    void play();
    void pause();
    void stop();
    void next();
    void previous();
    bool loading_status();
    bool debugin = false;
    bool loading = true;
    
private:
    std::vector<sf::SoundBuffer> m_buffers;
    sf::Sound m_sound;
    int m_currentIndex = 0;
    
   
};

#endif // PLAYLIST_H