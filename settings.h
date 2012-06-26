#define INPUTSIZE 21
#define OUTPUTSIZE 11
#define NUMEYES 4
#define BRAINSIZE 75
#define CONNS 5
#define LAYERS 3
#define CURRENTVERSION 4

#ifndef SETTINGS_H
#define SETTINGS_H

namespace conf {
	
	const int WIDTH = 6000;  //width and height of simulation
	const int HEIGHT = 5000;
	const int WWIDTH = 1300;  //window width and height
	const int WHEIGHT = 700;
	
	const int CZ = 50; //cell size in pixels, for food squares. Should divide well into Width Height
	const int MINFOOD = 0; //Minimum number of food cells which must have food durring simulation. 0 = off
	const int INITFOOD = 2000; //initial number of full food cells

	const int REPORTS_PER_EPOCH = 10; // number of times to record data per epoch. 0 for off. (David Coleman)

	const int NUMBOTS=30; //initially, and minimally
	const float BOTRADIUS=10;
	const float BOTSPEED= 0.3;
	const float SPIKESPEED= 0.005; //how quickly can attack spike go up?
	const float SPIKEMULT= 1; //essentially the strength of every spike impact
	const int BABIES=2; //number of babies per agent when they reproduce
	const float BOOSTSIZEMULT=2; //how much boost do agents get? when boost neuron is on
	const float REPRATE=10; //amount of food required to be consumed for an agent to reproduce
	const float MAXMETABOLISM=6; //it is what it says. Metabolism is limited to [0,this]
	const int MAXAGE=1000; //Age at which the full AGEDAMAGE amount is given to an agent
	const float AGEDAMAGE=0.0001;
	const float MAXDEVIATION=5; //maximum difference a crossover reproducing agent will be willing to tolerate

	const float DIST= 300; //how far can the eyes see on each bot?
	const float TEMPERATURE_DISCOMFORT = 0.002; //how quickly does health drain in nonpreferred temperatures (0= disabled. 0.005 is decent value)
	const float REPMULT = 1; //when a body of dead animal is eaten, how much of it goes toward increasing birth counter for surrounding bots?
	const float METAMUTRATE1= 0.002; //what is the change in MUTRATE1 and 2 on reproduction? lol
	const float METAMUTRATE2= 0.05;

	const float FOODINTAKE= 0.002; //how much plant food an agent can consume per tick?
	const float FOODGROWTH= -0.000001; //how much does food grow/decay on a square?
	const float FOODWASTE= 0.1; //how much food disapears if agent eats?
	const float FOODMAX= 0.5; //how much food per cell can there be at max?
	const int FOODADDFREQ= 200; //how often does random square get to full food?
	const float FOODSPREAD= 0.00003; //probability of a food cell seeding food to a nearby cell per tick
	const int FOODRANGE= 2; //distance that single cell of food can seed. in cells.

	const float MEATINTAKE= 0.02; //how much meat an agent can consume per tick?
	const float MEATDECAY= 0.000001; //how much meat decays/grows on a square? through MEATDECAY/[meat_present]
	const float MEATWASTE= 0.3; //how much meat disapears if agent eats?
	const float MEATMAX= 0.5; //how much meat per cell can there be at max?
	const float MEATVALUE= 1; //how much meat a bot's body is worth? in range [0,1]

	const float FOODTRANSFER= 0.001; //how much is transfered between two agents trading food? per iteration
	const float FOOD_SHARING_DISTANCE= 50; //how far away is food shared between bots?
	
	const float FOOD_DISTRIBUTION_RADIUS=100; //when bot is killed, how far is its body distributed?
	}

#endif
