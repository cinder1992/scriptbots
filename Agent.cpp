#include "Agent.h"

#include "settings.h"
#include "helpers.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include "DWRAONBrain.h"
#include "MLPBrain.h"
#include "AssemblyBrain.h"

using namespace std;
Agent::Agent()
{
    pos= Vector2f(randf(0,conf::WIDTH),randf(0,conf::HEIGHT));
    angle= randf(-M_PI,M_PI);
    health= 1.0+randf(0,0.1);
    age=0;
    spikeLength=0;
    red= 0;
    gre= 0;
    blu= 0;
    w1=0;
    w2=0;
    soundmul=1;
    give=0;
    clockf1= randf(5,100);
    clockf2= randf(5,100);
    boost=false;
    indicator=0;
    gencount=0;
    selectflag=0;
    ir=0;
    ig=0;
    ib=0;
	temperature_preference= cap(randn(2.0*abs(pos.y/conf::HEIGHT - 0.5),0.05));
    hybrid= false;
    herbivore= randf(0,1);
	repcounter= conf::REPRATE;
	metabolism= randf(0.1,conf::MAXMETABOLISM);

    id=0;
    
    smellmod= randf(0.1, 0.5);
    soundmod= randf(0.2, 0.6);
    hearmod= randf(0.7, 1.3);
    eyesensmod= randf(1, 3);
    bloodmod= randf(1, 3);
    
    MUTRATE1= 0.005; //randf(0.001, 0.005);
    MUTRATE2= 0.05; //randf(0.03, 0.07);

    spiked= false;
    
    in.resize(INPUTSIZE, 0);
    out.resize(OUTPUTSIZE, 0);
    
    eyefov.resize(NUMEYES, 0);
    eyedir.resize(NUMEYES, 0);
    for(int i=0;i<NUMEYES;i++) {
        eyefov[i] = randf(0.5, 2);
        eyedir[i] = randf(0, 2*M_PI);
    }
}

void Agent::printSelf()
{
    printf("Agent age=%i\n", age);
    for (int i=0;i<mutations.size();i++) {
        cout << mutations[i];
    }
}

void Agent::initEvent(float size, float r, float g, float b)
{
    indicator=size;
    ir=r;
    ig=g;
    ib=b;
}

void Agent::tick()
{
    brain.tick(in, out);
}
Agent Agent::reproduce(Agent that, float MR, float MR2)
{
	//first, error-check
	//create baby. Note that if the bot selects itself to mate with, this function acts also as asexual reproduction
	//NOTES: Agent this is mother, Agent that is father, Agent a2 is daughter
	Agent a2;

	//spawn the baby somewhere closeby behind agent
    //we want to spawn behind so that agents dont accidentally eat their young right away
    Vector2f fb(conf::BOTRADIUS,0);
    fb.rotate(-a2.angle);
    a2.pos= this->pos + fb + Vector2f(randf(-conf::BOTRADIUS*2,conf::BOTRADIUS*2), randf(-conf::BOTRADIUS*2,conf::BOTRADIUS*2));
    if (a2.pos.x<0) a2.pos.x= conf::WIDTH+a2.pos.x;
    if (a2.pos.x>conf::WIDTH) a2.pos.x= a2.pos.x-conf::WIDTH;
    if (a2.pos.y<0) a2.pos.y= conf::HEIGHT+a2.pos.y;
    if (a2.pos.y>conf::HEIGHT) a2.pos.y= a2.pos.y-conf::HEIGHT;

	//basic trait inheritance
	a2.gencount= max(this->gencount+1,that.gencount+1);
	a2.metabolism= randf(0,1)<0.5 ? this->metabolism : that.metabolism;
	a2.herbivore= randf(0,1)<0.5 ? this->herbivore : that.herbivore;

	a2.MUTRATE1= randf(0,1)<0.5 ? this->MUTRATE1 : that.MUTRATE1;
    a2.MUTRATE2= randf(0,1)<0.5 ? this->MUTRATE2 : that.MUTRATE2;
	a2.clockf1= randf(0,1)<0.5 ? this->clockf1 : that.clockf1;
    a2.clockf2= randf(0,1)<0.5 ? this->clockf2 : that.clockf2;

	a2.smellmod = randf(0,1)<0.5 ? this->smellmod : that.smellmod;
	a2.soundmod = randf(0,1)<0.5 ? this->soundmod : that.soundmod;
	a2.hearmod = randf(0,1)<0.5 ? this->hearmod : that.hearmod;
	a2.eyesensmod = randf(0,1)<0.5 ? this->eyesensmod : that.eyesensmod;
	a2.bloodmod = randf(0,1)<0.5 ? this->bloodmod : that.bloodmod;

	a2.temperature_preference= randf(0,1)<0.5 ? this->temperature_preference : that.temperature_preference;
    
	a2.eyefov = randf(0,1)<0.5 ? this->eyefov : that.eyefov;
	a2.eyedir = randf(0,1)<0.5 ? this->eyedir : that.eyedir;

	//mutations
	if (randf(0,1)<MR/2) a2.metabolism= randn(a2.metabolism, MR2);
	if (a2.metabolism<0) a2.metabolism= 0; //not going to bother limiting to 0.1; if it can survive, I don't even care if it's 0.000...1
	if (a2.metabolism>conf::MAXMETABOLISM) a2.metabolism= conf::MAXMETABOLISM;
	if (randf(0,1)<MR) a2.herbivore= cap(randn(a2.herbivore, 0.06));

	if (randf(0,1)<MR*2) a2.MUTRATE1= randn(a2.MUTRATE1, conf::METAMUTRATE1);
	if (randf(0,1)<MR*2) a2.MUTRATE2= randn(a2.MUTRATE2, conf::METAMUTRATE2);
    if (a2.MUTRATE1<0) a2.MUTRATE1= 0;
    if (a2.MUTRATE2<0) a2.MUTRATE2= 0;

	if (randf(0,1)<MR*4) a2.clockf1= randn(a2.clockf1, MR2/2);
    if (a2.clockf1<2) a2.clockf1= 2;
	if (randf(0,1)<MR*4) a2.clockf2= randn(a2.clockf2, MR2/2);
    if (a2.clockf2<2) a2.clockf2= 2;

	if (randf(0,1)<MR) a2.smellmod= randn(a2.smellmod, MR2);
	if (randf(0,1)<MR) a2.soundmod= randn(a2.soundmod, MR2);
	if (randf(0,1)<MR) a2.hearmod= randn(a2.hearmod, MR2);
	if (randf(0,1)<MR) a2.eyesensmod= randn(a2.eyesensmod, MR2);
	if (randf(0,1)<MR) a2.bloodmod= randn(a2.bloodmod, MR2);

	if (randf(0,1)<MR/2) a2.temperature_preference= cap(randn(a2.temperature_preference, MR2));

	for(int i=0;i<NUMEYES;i++){
        if(randf(0,1)<MR/2) a2.eyefov[i] = randn(a2.eyefov[i], MR2);
        if(a2.eyefov[i]<0) a2.eyefov[i] = 0;
        
        if(randf(0,1)<MR) a2.eyedir[i] = randn(a2.eyedir[i], MR2);
        if(a2.eyedir[i]<0) a2.eyedir[i] = 0;
        if(a2.eyedir[i]>2*M_PI) a2.eyedir[i] = 2*M_PI;
    }
    
    //create brain here
	a2.brain= this->brain.crossover(that.brain);
    a2.brain.mutate(MR,MR2);
    
    return a2;

}

Agent Agent::crossover(const Agent& other)
{
    //this could be made faster by returning a pointer
    //instead of returning by value
    Agent anew;
    anew.hybrid=true; //set this non-default flag
    anew.gencount= this->gencount;
    if (other.gencount<anew.gencount) anew.gencount= other.gencount;

    //agent heredity attributes
    anew.clockf1= randf(0,1)<0.5 ? this->clockf1 : other.clockf1;
    anew.clockf2= randf(0,1)<0.5 ? this->clockf2 : other.clockf2;
    anew.herbivore= randf(0,1)<0.5 ? this->herbivore : other.herbivore;
    anew.MUTRATE1= randf(0,1)<0.5 ? this->MUTRATE1 : other.MUTRATE1;
    anew.MUTRATE2= randf(0,1)<0.5 ? this->MUTRATE2 : other.MUTRATE2;
    anew.temperature_preference = randf(0,1)<0.5 ? this->temperature_preference : other.temperature_preference;
    
    anew.smellmod= randf(0,1)<0.5 ? this->smellmod : other.smellmod;
    anew.soundmod= randf(0,1)<0.5 ? this->soundmod : other.soundmod;
    anew.hearmod= randf(0,1)<0.5 ? this->hearmod : other.hearmod;
    anew.eyesensmod= randf(0,1)<0.5 ? this->eyesensmod : other.eyesensmod;
    anew.bloodmod= randf(0,1)<0.5 ? this->bloodmod : other.bloodmod;
    
    anew.eyefov= randf(0,1)<0.5 ? this->eyefov : other.eyefov;
    anew.eyedir= randf(0,1)<0.5 ? this->eyedir : other.eyedir;
    
    anew.brain= this->brain.crossover(other.brain);
    
    return anew;
}

void Agent::saveAgent(FILE *fs)
{
	//open savefile and dump data inside
	int hybridtemp= 0;
	int boosttemp= 0;
	if(this->hybrid) hybridtemp= 1;
	if(this->boost) boosttemp= 1;
	fprintf(fs, "<agent>\n"); //signals the creation of a new agent...
//	fprintf(fs, "id= %i\n", this->id); //id... (not loaded)
	fprintf(fs, "posx= %f\nposy= %f\n", this->pos.x, this->pos.y); //position...
	fprintf(fs, "angle= %f\n", this->angle); //angle...
	fprintf(fs, "health= %f\n", this->health);
//	fprintf(fs, "red= %f\ngre= %f\nblu= %f\n", this->red, this->gre, this->blu);
//	fprintf(fs, "w1= %f\nw2= %f\n", w1, w2);
//	fprintf(fs, "boost= %i\n", boosttemp);
	fprintf(fs, "herbivore= %f\n", this->herbivore);
	fprintf(fs, "spike= %f\n", this->spikeLength);
	fprintf(fs, "dfood= %f\n", this->dfood);
	fprintf(fs, "age= %i\n", this->age);
	fprintf(fs, "gen= %i\n", this->gencount);
	fprintf(fs, "hybrid= %i\n", hybridtemp);
	fprintf(fs, "cl1= %f\ncl2= %f\n", this->clockf1, this->clockf2);
	fprintf(fs, "smellmod= %f\n", this->smellmod);
	fprintf(fs, "soundmod= %f\n", this->soundmod);
	fprintf(fs, "hearmod= %f\n", this->hearmod);
	fprintf(fs, "bloodmod= %f\n", this->bloodmod);
	fprintf(fs, "eyesensemod= %f\n", this->eyesensmod);
	for(int q=0;q<NUMEYES;q++) {
		fprintf(fs, "<eye>\n");
		fprintf(fs, "eye#= %i\n", q);
		fprintf(fs, "eyedir= %f\n", this->eyedir[q]);
		fprintf(fs, "eyefov= %f\n", this->eyefov[q]);
		fprintf(fs, "</eye>\n");
	}
	fprintf(fs, "metabolism= %f\n", this->metabolism);
	fprintf(fs, "repcounter= %f\n", this->repcounter);
	fprintf(fs, "killed= %i\n", (int) this->spiked);
	fprintf(fs, "temppref= %f\n", this->temperature_preference);
//	fprintf(fs, "indicator= %f\n", this->indicator);
//	fprintf(fs, "ir= %f\nig= %f\nib= %f\n", this->ir, this->ig, this->ib);
//	fprintf(fs, "give= %f\n", this->give);
	fprintf(fs, "mutrate1= %f\nmutrate2= %f\n", this->MUTRATE1, this->MUTRATE2);
	fprintf(fs, "<brain>\n"); //signals the creation and loading of the brain

	for(int b=0;b<BRAINSIZE;b++){
		fprintf(fs, "<box>\n"); //signals the loading of a specific numbered box
		fprintf(fs, "box#= %i\n", b);
		fprintf(fs, "kp= %f\n", this->brain.boxes[b].kp);
		fprintf(fs, "bias= %f\n", this->brain.boxes[b].bias);
		fprintf(fs, "globalw= %f\n", this->brain.boxes[b].gw);
		fprintf(fs, "target= %f\n", this->brain.boxes[b].target);
		fprintf(fs, "out= %f\n", this->brain.boxes[b].out);
		fprintf(fs, "oldout= %f\n", this->brain.boxes[b].oldout);
		for(int c=0;c<CONNS;c++){
			fprintf(fs, "<conn>\n"); //signals the loading of a specific connection for a specific box
			fprintf(fs, "conn#= %i\n", c);
			fprintf(fs, "type= %i\n", this->brain.boxes[b].type[c]);
			fprintf(fs, "w= %f\n", this->brain.boxes[b].w[c]);
			fprintf(fs, "cid= %i\n", this->brain.boxes[b].id[c]);
			fprintf(fs, "</conn>\n"); //end of connection
		}
		fprintf(fs, "</box>\n"); //end of box
	}
	fprintf(fs, "</brain>\n"); //end of brain
	fprintf(fs, "</agent>\n"); //end of agent
//	fclose(fs);
}