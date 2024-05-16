#include "playlist.h"
#include <iostream>
#include <filesystem>
#include <SFML/Audio.hpp>
#include <regex>

sf::SoundBuffer buffer;

namespace fs = std::filesystem;
bool debugging = false;

Playlist::Playlist(bool &debug)
{
    debugging = debug;
}

void Playlist::playNextIfFinished()
{
    if (m_sound.getStatus() == sf::Sound::Stopped)
    {
        // Current song has finished playing, play the next one
        next();
    }
}

void Playlist::addSongsFromDirectory(const std::string &directoryPath)
{
    if (fs::is_directory(directoryPath))
    {
        for (const auto &entry : fs::directory_iterator(directoryPath))
        {
            const auto &extension = entry.path().extension();
            if (extension == ".flac" || extension == ".ogg" || extension == ".wav")
            {
                addSong(entry.path().string());
                if (debugging){
                    std::cerr << formatFlacMetadata(entry.path().string()) << std::endl;
                }
                    
            }
        }
        loading = false;
    }
    else
    {
        std::cerr << "Directory is empty or does not exist: " << directoryPath << std::endl;
    }
}

void Playlist::addSong(const std::string &filename)
{
    if (buffer.loadFromFile(filename))
    {
        m_buffers.push_back(buffer);
    }
    else
    {
        std::cerr << "Failed to load: " << filename << std::endl;
    }
}

std::string Playlist::formatFlacMetadata(const std::string &filename)
{
    // Define the regular expression pattern to find the split point
    std::regex pattern("_-_\\d+_\\-_");

    // Find the position to split the filename
    std::smatch match;
    std::string author, title;
    if (std::regex_search(filename, match, pattern))
    {
        // Split the filename based on the regex match
        // 19 are the characters before the file name, CHANGE THIS  <-------------
        author = filename.substr(19, match.position() - 19);
        title = filename.substr(match.position() + match.length());
    }

    // Replace underscores with spaces
    std::replace(author.begin(), author.end(), '_', ' ');
    std::replace(title.begin(), title.end(), '_', ' ');

    // Remove file extensions from the title
    std::regex fileExtension("\\.(mp3|flac|wav)(\\.flac)?$");
    title = std::regex_replace(title, fileExtension, "");

    // // Remove leading and trailing whitespaces from author and title
    author.erase(0, author.find_first_not_of(" \t\r\n"));
    author.erase(author.find_last_not_of(" \t\r\n") + 1);
    title.erase(0, title.find_first_not_of(" \t\r\n"));
    title.erase(title.find_last_not_of(" \t\r\n") + 1);

    return "Autor: " + author + "\nTitle: " + title;
}

void Playlist::play()
{
    if (m_currentIndex >= 0 && m_currentIndex < m_buffers.size())
    {
        m_sound.setBuffer(m_buffers[m_currentIndex]);
        m_sound.play();
    }
    else
    {
        std::cout << "Playlist is empty or current index is out of bounds. Cannot play any song." << std::endl;
    }
}

void Playlist::pause()
{
    m_sound.pause();
}

void Playlist::stop()
{
    m_sound.stop();
}

void Playlist::next()
{
    if (++m_currentIndex >= m_buffers.size())
    {
        m_currentIndex = 0;
    }
    stop();
    play();
}

void Playlist::previous()
{
    if (--m_currentIndex < 0)
    {
        m_currentIndex = m_buffers.size() - 1;
    }
    stop();
    play();
}

