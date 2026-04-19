#ifndef QUESTION1_H
#define QUESTION1_H

#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;
//Tile.h
class Organism;
struct Tile {
    float     nutrientLevel; 
    float     toxicity;      
    Organism* occupant;      

    Tile();
};

//Organism.h
class Organism {
protected:
    int    x, y;
    int    width, height;
    bool** internalShape;
    bool   alive;

    void allocateShape(int w, int h);
    void deallocateShape();

public:
    Organism* spawnRequest;

    Organism(int startX, int startY, int w, int h);
    virtual ~Organism();

    virtual void update(Tile** world, int worldWidth, int worldHeight) = 0;

    virtual char getType() const = 0;

    virtual void shrinkByOne();

    float getAverageNutrients(Tile** world);

    bool isAlive()               const;
    int  getX()                  const;
    int  getY()                  const;
    int  getWidth()              const;
    int  getHeight()             const;
    bool getShapeAt(int r, int c) const;
};

//FractalSprawler.h
class FractalSprawler : public Organism {
public:
    FractalSprawler(int startX, int startY, int w = 2, int h = 2);
    virtual ~FractalSprawler();

    virtual void update(Tile** world, int worldWidth, int worldHeight) override;
    virtual void shrinkByOne() override;
    virtual char getType() const override;

private:
    void growShape();
    void reproduce(Tile** world, int worldWidth, int worldHeight);
};

//KineticHunter.h
class KineticHunter : public Organism {
public:
    KineticHunter(int startX, int startY, int w = 2, int h = 2);
    virtual ~KineticHunter();

    virtual void update(Tile** world, int worldWidth, int worldHeight) override;
    virtual char getType() const override;
};

//World.h
class World {
private:
    int    width, height;
    Tile** grid;

    Organism** population;
    int        popCount;
    int        popCapacity;

    int iteration;

    void resizePopulation();
    void removeDeadOrganisms();
    void refreshOccupants();
    void regenerateNutrients();
    void displayGrid()       const;
    float getAverageToxicity() const;

public:
    World(int w, int h);
    ~World();

    // Addition of an organism to simulation 
    void spawnOrganism(Organism* org);

    // Advance simulation by one step
    void runIteration();

    void run(int maxIterations);
};

#endif


