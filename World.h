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
    
    void draw(View* view, int layer);
    
    bool isClosed() const;
    void setClosed(bool close);

	//debug stuff
	bool isDebug() const;
	void setDebug(bool state);
	std::vector<Vector2f> linesA;
	std::vector<Vector2f> linesB;
    
    /**
     * Returns the number of herbivores and 
     * carnivores in the world.
     * first : num herbs
     * second : num carns
     */
    std::pair<int,int> numHerbCarnivores() const;
    
    int numAgents() const;
	int numFood() const;
	int numMeat() const;
	int numHazards() const;
	int numHybrids() const;
    int epoch() const;
    
    //mouse interaction
    void processMouse(int button, int state, int x, int y);

    void addCarnivore();
    void addHerbivore();
	void addRandomBots(int num, int type=0);
    
    void positionOfInterest(int type, float &xi, float &yi);
    
    std::vector<int> numCarnivore;
    std::vector<int> numHerbivore; 
	std::vector<int> numHybrid;
	std::vector<int> numTotal;
    int ptr;

	int deleting;
	int pinput1;
	float pleft;
	float pright;
	bool pcontrol;
	void setControl(bool state);

	int modcounter;
    int current_epoch;
    int idcounter;

	std::vector<Agent> agents;
	//cells; replaces food layer, can be expanded (4 layers currently)
	//[LAYER]: 0= plant food, 1= meat, 2= hazard (poison, waste, events) , 3= temperature
	//(water/land, light layers also possible with very little coding)
	int CW;
	int CH;
	int cx;
	int cy;
	float cells[LAYERS][conf::WIDTH/conf::CZ][conf::HEIGHT/conf::CZ]; //[LAYER][CELL_X][CELL_Y]

	void setInputs();
	void brainsTick();  //takes in[] to out[] for every agent
    void processOutputs();
    
private:    
    void writeReport();
    
    void reproduce(int ai, int bi, float aMR, float aMR2, float bMR, float bMR2);

	void cellsRandomFill(int layer, float amount, int number);
	float capCell(float a, float top) const;

    bool CLOSED; //if environment is closed, then no random bots or food are added per time interval
	bool DEBUG; //if debugging, collect additional data, print more feedback, and draw extra info
};

#endif // WORLD_H
