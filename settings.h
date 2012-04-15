#define INPUTSIZE 22
#define OUTPUTSIZE 10
#define NUMEYES 4
#define BRAINSIZE 75
#define CONNS 5

#ifndef SETTINGS_H
#define SETTINGS_H

namespace conf {
    
    const int WIDTH = 6000;  //width and height of simulation
    const int HEIGHT = 5000;
    const int WWIDTH = 1300;  //window width and height
    const int WHEIGHT = 700;
    
    const int CZ = 50; //cell size in pixels, for food squares. Should divide well into Width Height
	const int MINFOOD = 0; //Minimum number of food cells which must have food durring simulation. 0 = off
	const int INITFOOD = 1800; //initial number of full food cells

	const int   REPORTS_PER_EPOCH = 10; // number of times to record data per epoch. 0 for off. (David Coleman)
    
    const int NUMBOTS=30; //initially, and minimally
    const float BOTRADIUS=10; //for drawing
    const float BOTSPEED= 0.3;
    const float SPIKESPEED= 0.005; //how quickly can attack spike go up?
    const float SPIKEMULT= 1; //essentially the strength of every spike impact
    const int BABIES=2; //number of babies per agent when they reproduce
    const float BOOSTSIZEMULT=2; //how much boost do agents get? when boost neuron is on
	const float MINREPRATE=3; //absolute lowest limit for reprate
	const int MAXAGE=1000; //Age at which the full AGEDAMAGE amount is given to an agent
	const float AGEDAMAGE=0.0001;

    const float DIST= 200; //how far can the eyes see on each bot?
    const float METAMUTRATE1= 0.002; //what is the change in MUTRATE1 and 2 on reproduction? lol
    const float METAMUTRATE2= 0.05;

    const float FOODINTAKE= 0.0015; //how much does every agent consume?
	const float FOODGROWTH= -0.000005; //how much does food grow/decay on a square?
    const float FOODWASTE= 0.00003; //how much food disapears if agent eats?
    const float FOODMAX= 0.5; //how much food per cell can there be at max?
    const int FOODADDFREQ= 400; //how often does random square get to full food?
	const float FOODSPREAD= 0.0001; //probability of a food cell spreading food to a nearby cell per tick

    const float FOODTRANSFER= 0.001; //how much is transfered between two agents trading food? per iteration
    const float FOOD_SHARING_DISTANCE= 50; //how far away is food shared between bots?
    
    const float TEMPERATURE_DISCOMFORT = 0.002; //how quickly does health drain in nonpreferred temperatures (0= disabled. 0.005 is decent value)

    const float FOOD_DISTRIBUTION_RADIUS=100; //when bot is killed, how far is its body distributed?
    
    const float REPMULT = 1; //when a body of dead animal is distributed, how much of it goes toward increasing birth counter for surrounding bots?
}

#endif
