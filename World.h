#ifndef WORLD_H
#define WORLD_H

#include "View.h"
#include "Agent.h"
#include "settings.h"
#include <vector>
class World
{
public:
    World();
    ~World();
    
    void update();
    void reset();
    
    void draw(View* view, bool drawfood);
    
    bool isClosed() const;
    void setClosed(bool close);
    
    /**
     * Returns the number of herbivores and 
     * carnivores in the world.
     * first : num herbs
     * second : num carns
     */
    std::pair<int,int> numHerbCarnivores() const;
    
    int numAgents() const;
	int numFood() const;
	int numHybrids() const;
    int epoch() const;
    
    //mouse interaction
    void processMouse(int button, int state, int x, int y);

    void addNewByCrossover();
    void addRandomBots(int num, int type=0);
    void addCarnivore();
    void addHerbivore();
    
    void positionOfInterest(int type, float &xi, float &yi);
    
    std::vector<int> numCarnivore;
    std::vector<int> numHerbivore; 
	std::vector<int> numHybrid;
	std::vector<int> numTotal;
    int ptr;

	int deleting;
	int pinput1;

	void save(const char *filename);
	void load(const char *filename);
    
private:
    void setInputs();
    void processOutputs();
    void brainsTick();  //takes in[] to out[] for every agent
    
    void writeReport();
    
    void reproduce(int ai, int bi, float aMR, float aMR2, float bMR, float bMR2);
    
    int modcounter;
    int current_epoch;
    int idcounter;
    
    std::vector<Agent> agents;

	//cells; replaces food layer, can be expanded (3 layers currently)
	//[LAYER]: 0= plant food (meat, poison, water/land, light, temperature layers also possible with very little coding)
	int CW;
	int CH;
	int cx;
	int cy;
	float cells[3][conf::WIDTH/conf::CZ][conf::HEIGHT/conf::CZ]; //[LAYER][CELL_X][CELL_Y]

    bool CLOSED; //if environment is closed, then no random bots are added per time interval
};

#endif // WORLD_H
