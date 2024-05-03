// world.h

#ifndef WORLD_H
#define WORLD_H

#include <vector>
#include <string>

class World {
public:
    World(const std::string& name);

    void loadMissions();
    void addObjective(const std::string& objective);

    // Add more member functions as needed

private:
    std::string name;
    std::vector<std::string> missions;
    std::vector<std::string> objectives;
};

#endif // WORLD_H
