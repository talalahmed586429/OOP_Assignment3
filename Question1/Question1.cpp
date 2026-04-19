#include "Question1.h"

Tile::Tile() : nutrientLevel(90.0f), toxicity(2.0f), occupant(nullptr) {}

//Organism.cpp
Organism::Organism(int startX, int startY, int w, int h)
    : x(startX), y(startY), width(w), height(h),
    internalShape(nullptr), alive(true), spawnRequest(nullptr)
{
    allocateShape(w, h);
    for (int i = 0; i < h; i++)
        for (int j = 0; j < w; j++)
            internalShape[i][j] = true;
}

Organism::~Organism() {
    deallocateShape();

}


void Organism::allocateShape(int w, int h) {
    internalShape = new bool* [h];
    for (int i = 0; i < h; i++) {
        internalShape[i] = new bool[w];
        for (int j = 0; j < w; j++)
            internalShape[i][j] = false;
    }
}

void Organism::deallocateShape() {
    if (internalShape) {
        for (int i = 0; i < height; i++)
            delete[] internalShape[i];
        delete[] internalShape;
        internalShape = nullptr;
    }
}


float Organism::getAverageNutrients(Tile** world) {
    float total = 0.0f;
    int   count = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (internalShape && internalShape[i][j]) {
                total += world[y + i][x + j].nutrientLevel;
                count++;
            }
        }
    }
    return (count > 0) ? (total / (float)count) : 0.0f;
}


void Organism::shrinkByOne() {}

bool Organism::isAlive()            const { return alive; }
int  Organism::getX()               const { return x; }
int  Organism::getY()               const { return y; }
int  Organism::getWidth()           const { return width; }
int  Organism::getHeight()          const { return height; }

bool Organism::getShapeAt(int r, int c) const {
    if (!internalShape || r < 0 || r >= height || c < 0 || c >= width)
        return false;
    return internalShape[r][c];
}

//FractalSprawler.cpp
FractalSprawler::FractalSprawler(int startX, int startY, int w, int h)
    : Organism(startX, startY, w, h) {}

FractalSprawler::~FractalSprawler() {}

char FractalSprawler::getType() const { return 'F'; }


void FractalSprawler::shrinkByOne() {
    if (!alive) return;

    int newW = width - 1;
    int newH = height - 1;

    if (newW <= 0 || newH <= 0) {
        alive = false;  
        return;
    }

    deallocateShape();
    width = newW;
    height = newH;
    allocateShape(width, height);
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            internalShape[i][j] = true;
}


void FractalSprawler::growShape() {
    int newW = width + 2;
    int newH = height + 2;

    deallocateShape(); 
    width = newW;
    height = newH;
    allocateShape(width, height);
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            internalShape[i][j] = true;
}


void FractalSprawler::reproduce(Tile** world, int worldWidth, int worldHeight) {
    int searchR = 2 * width + 4;
    int cx = x + width / 2;
    int cy = y + height / 2;

    for (int dy = -searchR; dy <= searchR && spawnRequest == nullptr; dy++) {
        for (int dx = -searchR; dx <= searchR && spawnRequest == nullptr; dx++) {
  
            int nx = cx + dx - 1;
            int ny = cy + dy - 1;

            if (nx < 0 || ny < 0 || nx + 2 > worldWidth || ny + 2 > worldHeight)
                continue;

            bool overlapsMe = (nx + 2 > x) && (nx < x + width)
                && (ny + 2 > y) && (ny < y + height);
            if (overlapsMe) continue;

            bool empty = true;
            for (int i = 0; i < 2 && empty; i++)
                for (int j = 0; j < 2 && empty; j++)
                    if (world[ny + i][nx + j].occupant != nullptr)
                        empty = false;

            if (empty)
                spawnRequest = new FractalSprawler(nx, ny, 2, 2);
        }
    }

    deallocateShape();
    width = 2;
    height = 2;
    allocateShape(width, height);
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            internalShape[i][j] = true;
}


void FractalSprawler::update(Tile** world, int worldWidth, int worldHeight) {
    if (!alive) return;

    float avgNutrients = getAverageNutrients(world);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (!internalShape[i][j]) continue;
            int r = y + i, c = x + j;
            if (r < 0 || r >= worldHeight || c < 0 || c >= worldWidth) continue;
            world[r][c].nutrientLevel -= 1.5f;
            if (world[r][c].nutrientLevel < 0.0f)
                world[r][c].nutrientLevel = 0.0f;
        }
    }

    if (avgNutrients > 70.0f) {
        growShape();
        if (x + width > worldWidth)  x = worldWidth - width;
        if (y + height > worldHeight) y = worldHeight - height;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
    }
    else if (avgNutrients < 30.0f) {
        shrinkByOne();
        if (!alive) return; 
    }

    if (width >= 5 && height >= 5 && spawnRequest == nullptr)
        reproduce(world, worldWidth, worldHeight);
}

//KineticHunter.cpp
static int absVal(int v) { 
    return v < 0 ? -v : v; 
}


KineticHunter::KineticHunter(int startX, int startY, int w, int h)
    : Organism(startX, startY, w, h) {}

KineticHunter::~KineticHunter() {}

char KineticHunter::getType() const { return 'K'; }

void KineticHunter::update(Tile** world, int worldWidth, int worldHeight) {
    if (!alive) return;

    int scanRadius = 4 * width;
    int cx = x + width / 2;
    int cy = y + height / 2;

    //1. Scan for the closest FractalSprawler within scanRadius
    Organism* target = nullptr;
    int       bestDist = scanRadius * 3 + 1; 
    int       tCX = -1, tCY = -1;     

    int r0 = cy - scanRadius;  if (r0 < 0)           r0 = 0;
    int r1 = cy + scanRadius;  if (r1 >= worldHeight) r1 = worldHeight - 1;
    int c0 = cx - scanRadius;  if (c0 < 0)           c0 = 0;
    int c1 = cx + scanRadius;  if (c1 >= worldWidth)  c1 = worldWidth - 1;

    for (int r = r0; r <= r1; r++) {
        for (int c = c0; c <= c1; c++) {
            Organism* occ = world[r][c].occupant;
            if (!occ || occ == this || occ->getType() != 'F' || !occ->isAlive())
                continue;

            int dist = absVal(c - cx) + absVal(r - cy); 
            if (dist < bestDist) {
                bestDist = dist;
                target = occ;
                tCX = occ->getX() + occ->getWidth() / 2;
                tCY = occ->getY() + occ->getHeight() / 2;
            }
        }
    }

    if (!target) return;

    // 2.now we Check if we are already overlapping the target
    bool overlapping = false;
    for (int i = 0; i < height && !overlapping; i++) {
        for (int j = 0; j < width && !overlapping; j++) {
            if (!internalShape[i][j]) continue;
            int r = y + i, c = x + j;
            if (r < 0 || r >= worldHeight || c < 0 || c >= worldWidth) continue;
            Organism* occ = world[r][c].occupant;
            if (occ && occ != this && occ->getType() == 'F')
                overlapping = true;
        }
    }

    if (overlapping) {
        // 3 Drain nutrients from every tile we cover
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                if (!internalShape[i][j]) continue;
                int r = y + i, c = x + j;
                if (r < 0 || r >= worldHeight || c < 0 || c >= worldWidth) continue;
                world[r][c].nutrientLevel -= 15.0f;
                if (world[r][c].nutrientLevel < 0.0f)
                    world[r][c].nutrientLevel = 0.0f;
            }
        }
        // 4. Reduce prey size 
        target->shrinkByOne();

    }
    else {
        // 5. Move one step toward the target

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                if (!internalShape[i][j]) continue;
                int r = y + i, c = x + j;
                if (r >= 0 && r < worldHeight && c >= 0 && c < worldWidth)
                    if (world[r][c].occupant == this)
                        world[r][c].occupant = nullptr;
            }
        }

        if (tCX > cx) x++;
        else if (tCX < cx) x--;

        if (tCY > cy) y++;
        else if (tCY < cy) y--;

        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x + width > worldWidth)  x = worldWidth - width;
        if (y + height > worldHeight) y = worldHeight - height;

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                if (!internalShape[i][j]) continue;
                int r = y + i, c = x + j;
                if (r >= 0 && r < worldHeight && c >= 0 && c < worldWidth)
                    world[r][c].occupant = this;
            }
        }
    }
}

//World.cpp
// ANSI terminal colour macros
#define COL_GREEN  "\033[42m"
#define COL_RED    "\033[41m"
#define COL_DARK   "\033[40m"
#define COL_RESET  "\033[0m"
#define COL_BOLD   "\033[1m"
#define SCR_HOME   "\033[2J\033[H"

static void pauseMs(int ms) {
    clock_t end = clock() + (clock_t)((long)ms * CLOCKS_PER_SEC / 1000);
    while (clock() < end);
}


World::World(int w, int h)
    : width(w), height(h), popCount(0), popCapacity(64), iteration(0)
{
    // Allocate 2D grid
    grid = new Tile * [h];
    for (int i = 0; i < h; i++)
        grid[i] = new Tile[w];

    // Add positional nutrient variation
    for (int i = 0; i < h; i++)
        for (int j = 0; j < w; j++) {
            float v = (float)((i * 7 + j * 11) % 61) - 30.0f; 
            grid[i][j].nutrientLevel += v;
            if (grid[i][j].nutrientLevel < 0.0f)   grid[i][j].nutrientLevel = 0.0f;
            if (grid[i][j].nutrientLevel > 100.0f)  grid[i][j].nutrientLevel = 100.0f;
        }

    population = new Organism * [popCapacity];
    for (int i = 0; i < popCapacity; i++) population[i] = nullptr;
}

World::~World() {
 
    for (int i = 0; i < height; i++) delete[] grid[i];
    delete[] grid;

    for (int i = 0; i < popCount; i++) {
        delete population[i];
        population[i] = nullptr;
    }
    delete[] population;
}

// Population Management

void World::resizePopulation() {
    int newCap = popCapacity * 2;
    Organism** arr = new Organism * [newCap];
    for (int i = 0; i < popCount; i++) arr[i] = population[i];
    for (int i = popCount; i < newCap; i++) arr[i] = nullptr;
    delete[] population;
    population = arr;
    popCapacity = newCap;
}

void World::spawnOrganism(Organism* org) {
    if (popCount >= popCapacity) resizePopulation();
    population[popCount++] = org;
}

void World::removeDeadOrganisms() {
    int alive = 0;
    for (int i = 0; i < popCount; i++) {
        if (!population[i]) continue;
        if (population[i]->isAlive()) {
            population[alive++] = population[i];
        }
        else{
            for (int r = 0; r < height; r++)
                for (int c = 0; c < width; c++)
                    if (grid[r][c].occupant == population[i])
                        grid[r][c].occupant = nullptr;
            delete population[i];
        }
    }
    for (int i = alive; i < popCount; i++) population[i] = nullptr;
    popCount = alive;
}

void World::refreshOccupants() {
    for (int r = 0; r < height; r++)
        for (int c = 0; c < width; c++)
            grid[r][c].occupant = nullptr;

    for (int k = 0; k < popCount; k++) {
        Organism* org = population[k];
        if (!org || !org->isAlive()) continue;
        for (int i = 0; i < org->getHeight(); i++) {
            for (int j = 0; j < org->getWidth(); j++) {
                if (!org->getShapeAt(i, j)) continue;
                int r = org->getY() + i;
                int c = org->getX() + j;
                if (r >= 0 && r < height && c >= 0 && c < width)
                    grid[r][c].occupant = org;
            }
        }
    }
}

// Slight nutrient regeneration 
void World::regenerateNutrients() {
    for (int r = 0; r < height; r++)
        for (int c = 0; c < width; c++) {
            grid[r][c].nutrientLevel += 0.5f;
            if (grid[r][c].nutrientLevel > 100.0f)
                grid[r][c].nutrientLevel = 100.0f;
        }
}

float World::getAverageToxicity() const {
    float total = 0.0f;
    for (int r = 0; r < height; r++)
        for (int c = 0; c < width; c++)
            total += grid[r][c].toxicity;
    return total / (float)(width * height);
}

// Console Rendering
void World::displayGrid() const {
    printf("%s", SCR_HOME);

    printf("+");
    for (int j = 0; j < width; j++) printf("--");
    printf("+\n");

    printf("| %sPRIMORDIAL GRID%s  Iteration: %-5d", COL_BOLD, COL_RESET, iteration);
  
    int titleLen = 25; 
    int padNeeded = width * 2 - titleLen;
    for (int p = 0; p < padNeeded; p++) printf(" ");
    printf("|\n");

    printf("+");
    for (int j = 0; j < width; j++) printf("--");
    printf("+\n");

    for (int r = 0; r < height; r++) {
        printf("|");
        for (int c = 0; c < width; c++) {
            Organism* occ = grid[r][c].occupant;
            if (!occ)                       printf("%s  %s", COL_DARK, COL_RESET);
            else if (occ->getType() == 'F')      printf("%s  %s", COL_GREEN, COL_RESET);
            else                                 printf("%s  %s", COL_RED, COL_RESET);
        }
        printf("|\n");
    }

    printf("+");
    for (int j = 0; j < width; j++) printf("--");
    printf("+\n");

    printf("  Total Population       : %d\n", popCount);
    printf("  Average World Toxicity : %.2f\n", getAverageToxicity());
    printf("  Legend : %s  %s Producer (Fractal Sprawler)   "
        "%s  %s Consumer (Kinetic Hunter)   "
        "%s  %s Empty\n",
        COL_GREEN, COL_RESET,
        COL_RED, COL_RESET,
        COL_DARK, COL_RESET);

    fflush(stdout);
}

// Simulation Loop
void World::runIteration() {
    iteration++;

    // Phase 1: Nutrient regeneration 
    regenerateNutrients();

    //Phase 2: Snapshot occupants
    refreshOccupants();

    //Phase 3: Update each organism 
    int currentCount = popCount;
    for (int k = 0; k < currentCount; k++) {
        if (!population[k] || !population[k]->isAlive()) continue;

        population[k]->update(grid, width, height);

        if (population[k]->spawnRequest) {
            spawnOrganism(population[k]->spawnRequest);
            population[k]->spawnRequest = nullptr;
        }
    }

    //Phase 4: Garbage-collect dead organisms 
    removeDeadOrganisms();

    //Phase 5: Refresh occupants & render
    refreshOccupants();
    displayGrid();
}

void World::run(int maxIterations) {
    refreshOccupants();
    displayGrid();
    pauseMs(1200);

    for (int i = 0; i < maxIterations; i++) {
        runIteration();
        pauseMs(350);
    }

    printf("\n  === Simulation ended after %d iterations ===\n\n", iteration);
}
