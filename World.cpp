#include "World.h"

#include <ctime>

#include "settings.h"
#include "helpers.h"
#include "vmath.h"
#include <stdio.h>

using namespace std;

World::World() :
        modcounter(0),
        current_epoch(0),
        idcounter(0),
        FW(conf::WIDTH/conf::CZ),
        FH(conf::HEIGHT/conf::CZ),
        CLOSED(false)
{
	//start the report file. null it up if it exists
	FILE* fp = fopen("report.txt", "w");
	fclose(fp);

    addRandomBots(conf::NUMBOTS);
    //inititalize food layer

    for (int x=0;x<FW;x++) {
        for (int y=0;y<FH;y++) {
            food[x][y]= 0;
        }
    }
	while(numFood()<conf::INITFOOD) {
		food[randi(0,FW)][randi(0,FH)] = conf::FOODMAX;
	}
    
    numCarnivore.resize(200, 0);
    numHerbivore.resize(200, 0);
    ptr=0;
}

void World::update()
{
    modcounter++;
	vector<int> dt;
	float tinit;
	float tfin;

    //Process periodic events
    //Age goes up!
    if (modcounter%100==0) {
        for (int i=0;i<agents.size();i++) {
            agents[i].age+= 1;    //agents age...
        }        
    }
    
	if (conf::REPORTS_PER_EPOCH>0 && (modcounter%(10000/conf::REPORTS_PER_EPOCH)==0)) { //Added custom report frequency
		//write report and record herbivore/carnivore counts
        std::pair<int,int> num_herbs_carns = numHerbCarnivores();
        numHerbivore[ptr]= num_herbs_carns.first;
        numCarnivore[ptr]= num_herbs_carns.second;
        ptr++;
        if(ptr == numHerbivore.size()) ptr = 0;

		writeReport();
	}

    if (modcounter>=10000) {
        modcounter=0;
        current_epoch++;
    }
	if (modcounter%conf::FOODADDFREQ==0 || numFood()<conf::MINFOOD) {
        fx=randi(0,FW);
        fy=randi(0,FH);
        food[fx][fy]= conf::FOODMAX;
    }

	for(int x=0;x<FW;x++){
		for(int y=0;y<FH;y++){
			if (food[x][y]>0) {
				food[x][y]+= conf::FOODGROWTH; //food quantity is changed by FOODGROWTH

				if (randf(0,1)<conf::FOODSPREAD && food[x][y]>conf::FOODMAX/2) {
					int ox= randi(x-1,x+2);
					int oy= randi(y-1,y+2);
					if (ox<1) ox= FW-1;
					if (ox>FW-1) ox= 0;
					if (oy<1) oy= FH-1;
					if (oy>FH-1) oy= 0;
					food[ox][oy]+= conf::FOODMAX/3;
					if (food[ox][oy]>conf::FOODMAX) food[ox][oy]= conf::FOODMAX;
				}
			}

			if(food[x][y]>conf::FOODMAX) food[x][y]=conf::FOODMAX; //cap at FOODMAX
			if(food[x][y]<0) food[x][y]= 0;

		}
	}
    
    //reset any counter variables per agent
    for(int i=0;i<agents.size();i++){
        agents[i].spiked= false;

		//process indicator (used in drawing)
        if(agents[i].indicator>0) agents[i].indicator -= 1;

		//reset dfood for processOutputs
        agents[i].dfood=0;
    }

    //give input to every agent. Sets in[] array
    setInputs();

	pinput1= int(pinput1/2);

    //brains tick. computes in[] -> out[]
    brainsTick();

    //read output and process consequences of bots on environment. requires out[]
    processOutputs();

    //process bots:
    for (int i=0;i<agents.size();i++) {
		//health and deaths
        float baseloss= 0.0005; // + 0.0001*(abs(agents[i].w1) + abs(agents[i].w2))/2;
        //if (agents[i].w1<0.1 && agents[i].w2<0.1) baseloss=0.0001; //hibernation :p

		baseloss += agents[i].age/conf::MAXAGE*conf::AGEDAMAGE; //getting older reduces health. Age > MAXAGE = no health (GPA)

		if (agents[i].boost) { //if boosting, init baseloss + age loss is multiplied
            baseloss *= conf::BOOSTSIZEMULT*1.2;
		}
   
		//process temperature preferences
        //calculate temperature at the agents spot. (based on distance from horizontal equator)
		float dd= 2.0*abs(agents[i].pos.y/conf::HEIGHT - 0.5);
        float discomfort= sqrt(abs(dd-agents[i].temperature_preference));
        if (discomfort<0.08) discomfort=0;
        baseloss += conf::TEMPERATURE_DISCOMFORT*discomfort; //add to baseloss

		agents[i].health -= baseloss;

		//handle reproduction
        if (agents[i].repcounter<0 && agents[i].health>0.65 && modcounter%15==0 && randf(0,1)<0.1) { //agent is healthy and is ready to reproduce. Also inject a bit non-determinism
            //agents[i].health= 0.8; //the agent is left vulnerable and weak, a bit
            reproduce(i, agents[i].MUTRATE1, agents[i].MUTRATE2); //this adds conf::BABIES new agents to agents[]
			agents[i].repcounter= agents[i].reprate;
        }
    }

    //remove dead agents.
    //first distribute foods
    for (int i=0;i<agents.size();i++) {
        //if this agent was spiked this round as well (i.e. killed). This will make it so that
        //natural deaths can't be capitalized on. I feel I must do this or otherwise agents
        //will sit on spot and wait for things to die around them. They must do work!
        if (agents[i].health<=0 && agents[i].spiked) { 
        
            //distribute its food. It will be erased soon
            //first figure out how many are around, to distribute this evenly
            int numaround=0;
            for (int j=0;j<agents.size();j++) {
                if (agents[j].herbivore < .5 && i!=j ) { //only carnivores get food. not same agent as dyingif (agents[j].herbivore < .5 && i!=j ) { //only carnivores get food. not same agent as dying
                    float d= (agents[i].pos-agents[j].pos).length();
                    if (d<conf::FOOD_DISTRIBUTION_RADIUS) {
                        numaround++;
                    }
                }
            }
            
            //young killed agents should give very little resources
            //at age 5, they mature and give full. This can also help prevent
            //agents eating their young right away
            float agemult= 1.0;
            if(agents[i].age<5) agemult= agents[i].age*0.2;
            
            if (numaround>0) {
                //distribute its food evenly
                for (int j=0;j<agents.size();j++) {
                    if (agents[j].herbivore < .5 && i!=j ) { //only carnivores get food. not same agent as dying
                        float d= (agents[i].pos-agents[j].pos).length();
                        if (d<conf::FOOD_DISTRIBUTION_RADIUS) {
                            agents[j].health += 5*(1-agents[j].herbivore)*(1-agents[j].herbivore)/pow(numaround,1.25)*agemult;
                            agents[j].repcounter -= conf::REPMULT*(1-agents[j].herbivore)*(1-agents[j].herbivore)/pow(numaround,1.25)*agemult; //good job, can use spare parts to make copies
                            if (agents[j].health>2) agents[j].health=2; //cap it!
                            agents[j].initEvent(30,1,1,1); //white means they ate! nice
                        }
                    }
                }
            }

        }
    }
    vector<Agent>::iterator iter= agents.begin();
    while (iter != agents.end()) {
        if (iter->health <=0) {
            iter= agents.erase(iter);
		} else if(iter->selectflag==1 && deleting==1){
			deleting= 0;
			iter= agents.erase(iter);
        } else {
            ++iter;
        }
    }

    //add new agents, if environment isn't closed
    if (!CLOSED) {
        //make sure environment is always populated with at least NUMBOTS bots
        if (agents.size()<conf::NUMBOTS) {
            //add new agent
            addRandomBots(1);
        }
        if (modcounter%200==0) {
            if (randf(0,1)<0.5){
                addRandomBots(1); //every now and then add random bots in
//            }else
//                addNewByCrossover(); //or by crossover
			}
        }
    }


}

void World::setInputs()
{
    //P1 R1 G1 B1 FOOD P2 R2 G2 B2 SOUND SMELL HEALTH P3 R3 G3 B3 CLOCK1 CLOCK 2 HEARING BLOOD_SENSOR TEMPERATURE_SENSOR PLAYER_INPUT1
    //0   1  2  3  4   5   6  7 8   9     10     11   12 13 14 15  16      17      18        19               20              25

    float PI8=M_PI/8/2; //pi/8/2
    float PI38= 3*PI8; //3pi/8/2
    float PI4= M_PI/4;
   
	#pragma omp parallel for
    for (int i=0;i<agents.size();i++) {
        Agent* a= &agents[i];

        //HEALTH
        a->in[11]= cap(a->health/2); //divide by 2 since health is in [0,2]

        //FOOD
        int cx= (int) a->pos.x/conf::CZ;
        int cy= (int) a->pos.y/conf::CZ;
        a->in[4]= food[cx][cy]/conf::FOODMAX;

        //SOUND SMELL EYES
        vector<float> p(NUMEYES,0);
        vector<float> r(NUMEYES,0);
        vector<float> g(NUMEYES,0);
        vector<float> b(NUMEYES,0);
                       
        float soaccum=0;
        float smaccum=0;
        float hearaccum=0;

        //BLOOD ESTIMATOR
        float blood= 0;

        for (int j=0;j<agents.size();j++) {
            if (i==j) continue;
            Agent* a2= &agents[j];

            if (a->pos.x<a2->pos.x-conf::DIST || a->pos.x>a2->pos.x+conf::DIST
                    || a->pos.y>a2->pos.y+conf::DIST || a->pos.y<a2->pos.y-conf::DIST) continue;

            float d= (a->pos-a2->pos).length();

            if (d<conf::DIST) {

                //smell
                smaccum+= (conf::DIST-d)/conf::DIST;

                //sound
                soaccum+= (conf::DIST-d)/conf::DIST*(max(fabs(a2->w1),fabs(a2->w2)));

                //hearing. Listening to other agents
                hearaccum+= a2->soundmul*(conf::DIST-d)/conf::DIST;

                float ang= (a2->pos- a->pos).get_angle(); //current angle between bots
                
                for(int q=0;q<NUMEYES;q++){
                    float aa = a->angle + a->eyedir[q];
                    if (aa<-M_PI) aa += 2*M_PI;
                    if (aa>M_PI) aa -= 2*M_PI;
                    
                    float diff1= aa- ang;
                    if (fabs(diff1)>M_PI) diff1= 2*M_PI- fabs(diff1);
                    diff1= fabs(diff1);
                    
                    float fov = a->eyefov[q];
                    if (diff1<fov) {
                        //we see a2 with this eye. Accumulate stats
                        float mul1= a->eyesensmod*(fabs(fov-diff1)/fov)*((conf::DIST-d)/conf::DIST);
                        p[q] += mul1*(d/conf::DIST);
                        r[q] += mul1*a2->red;
                        g[q] += mul1*a2->gre;
                        b[q] += mul1*a2->blu;
                    }
                }
                
                //blood sensor
                float forwangle= a->angle;
                float diff4= forwangle- ang;
                if (fabs(forwangle)>M_PI) diff4= 2*M_PI- fabs(forwangle);
                diff4= fabs(diff4);
                if (diff4<PI38) {
                    float mul4= ((PI38-diff4)/PI38)*((conf::DIST-d)/conf::DIST);
                    //if we can see an agent close with both eyes in front of us
                    blood+= mul4*(1-agents[j].health/2); //remember: health is in [0 2]
                    //agents with high life dont bleed. low life makes them bleed more
                }
            }
        }

		//temperature varies from 0 to 1 across screen.
        //it is 0 at equator (in middle), and 1 on edges. Agents can sense discomfort
        float dd= 2.0*abs(a->pos.x/conf::WIDTH - 0.5);
        float discomfort= abs(dd - a->temperature_preference);
        
        smaccum *= a->smellmod;
        soaccum *= a->soundmod;
        hearaccum *= a->hearmod;
        blood *= a->bloodmod;
        
        a->in[0]= cap(p[0]);
        a->in[1]= cap(r[0]);
        a->in[2]= cap(g[0]);
        a->in[3]= cap(b[0]);
        
        a->in[5]= cap(p[1]);
        a->in[6]= cap(r[1]);
        a->in[7]= cap(g[1]);
        a->in[8]= cap(b[1]);
        a->in[9]= cap(soaccum);
        a->in[10]= cap(smaccum);
        
        a->in[12]= cap(p[2]);
        a->in[13]= cap(r[2]);
        a->in[14]= cap(g[2]);
        a->in[15]= cap(b[2]);
        a->in[16]= abs(sin(modcounter/a->clockf1));
        a->in[17]= abs(sin(modcounter/a->clockf2));
        a->in[18]= cap(hearaccum);
        a->in[19]= cap(blood);
        
        a->in[20]= discomfort;
        
        a->in[21]= cap(p[3]);
        a->in[22]= cap(r[3]);
        a->in[23]= cap(g[3]);
        a->in[24]= cap(b[3]);

		if (a->selectflag) {
			a->in[25]= cap(pinput1);
		} else {
			a->in[25]= 0;
		}
    }
//	#pragma omp barrier //testing (GPA)
}

void World::processOutputs()
{
    //assign meaning
    //LEFT RIGHT R G B SPIKE BOOST SOUND_MULTIPLIER GIVING CHOICE
    // 0    1    2 3 4   5     6         7             8	  9
    for (int i=0;i<agents.size();i++) {
        Agent* a= &agents[i];

        a->red= a->out[2];
        a->gre= a->out[3];
        a->blu= a->out[4];
        a->w1= a->out[0]; //-(2*a->out[0]-1);
        a->w2= a->out[1]; //-(2*a->out[1]-1);
        a->boost= a->out[6]>0.5;
        a->soundmul= a->out[7];
        a->give= a->out[8];

        //spike length should slowly tend towards out[5]
        float g= a->out[5];
        if (a->spikeLength<g)
            a->spikeLength+=conf::SPIKESPEED;
        else if (a->spikeLength>g)
            a->spikeLength= g; //its easy to retract spike, just hard to put it up
    }

    //move bots
    //#pragma omp parallel for
    for (int i=0;i<agents.size();i++) {
        Agent* a= &agents[i];

        Vector2f v(conf::BOTRADIUS/2, 0);
        v.rotate(a->angle + M_PI/2);

        Vector2f w1p= a->pos+ v; //wheel positions
        Vector2f w2p= a->pos- v;

        float BW1= conf::BOTSPEED*a->w1;
        float BW2= conf::BOTSPEED*a->w2;
        if (a->boost) {
            BW1=BW1*conf::BOOSTSIZEMULT;
        }
        if (a->boost) {
            BW2=BW2*conf::BOOSTSIZEMULT;
        }

        //move bots
        Vector2f vv= w2p- a->pos;
        vv.rotate(-BW1);
        a->pos= w2p-vv;
        a->angle -= BW1;
        if (a->angle<-M_PI) a->angle= M_PI - (-M_PI-a->angle);
        vv= a->pos - w1p;
        vv.rotate(BW2);
        a->pos= w1p+vv;
        a->angle += BW2;
        if (a->angle>M_PI) a->angle= -M_PI + (a->angle-M_PI);

        //wrap around the map
        if (a->pos.x<0) a->pos.x= conf::WIDTH+a->pos.x;
        if (a->pos.x>=conf::WIDTH) a->pos.x= a->pos.x-conf::WIDTH;
        if (a->pos.y<0) a->pos.y= conf::HEIGHT+a->pos.y;
        if (a->pos.y>=conf::HEIGHT) a->pos.y= a->pos.y-conf::HEIGHT;
    }

    //process food intake for herbivors
    for (int i=0;i<agents.size();i++) {

        int cx= (int) agents[i].pos.x/conf::CZ;
        int cy= (int) agents[i].pos.y/conf::CZ;
        float f= food[cx][cy];
        if (f>0 && agents[i].health<2) {
            //agent eats the food
            float itk=min(f,conf::FOODINTAKE);
            float speedmul= (1-(abs(agents[i].w1)+abs(agents[i].w2))/2)*0.7 + 0.3;
            itk= itk*agents[i].herbivore*speedmul; //herbivores gain more from ground food
            agents[i].health+= itk;
            agents[i].repcounter -= 3*itk;
            food[cx][cy]-= min(f,conf::FOODWASTE);
        }
    }

	//process giving and receiving of food
    for (int i=0;i<agents.size();i++) {
        if (agents[i].give>0.5) {
            for (int j=0;j<agents.size();j++) {
                float d= (agents[i].pos-agents[j].pos).length();
                if (d<conf::FOOD_SHARING_DISTANCE) {
                    //initiate transfer
                    if (agents[j].health<2) agents[j].health += conf::FOODTRANSFER;
                    agents[i].health -= conf::FOODTRANSFER;
                    agents[j].dfood += conf::FOODTRANSFER; //only for drawing
                    agents[i].dfood -= conf::FOODTRANSFER;
                }
            }
        }
    }

    //process spike dynamics for carnivors
    if (modcounter%2==0) { //we dont need to do this TOO often. can save efficiency here since this is n^2 op in #agents
        for (int i=0;i<agents.size();i++) {

            //NOTE: herbivore cant attack. TODO: hmmmmm
            //fot now ok: I want herbivores to run away from carnivores, not kill them back
            if(agents[i].herbivore>0.8 || agents[i].spikeLength<0.2 || agents[i].w1<0.5 || agents[i].w2<0.5) continue; 
            
            for (int j=0;j<agents.size();j++) {
                
                if (i==j) continue;
                float d= (agents[i].pos-agents[j].pos).length();

                if (d<2*conf::BOTRADIUS) {
                    //these two are in collision and agent i has extended spike and is going decent fast!
                    Vector2f v(1,0);
                    v.rotate(agents[i].angle);
                    float diff= v.angle_between(agents[j].pos-agents[i].pos);
                    if (fabs(diff)<M_PI/8) {
                        //bot i is also properly aligned!!! that's a hit
                        float mult=1;
                        if (agents[i].boost) mult= conf::BOOSTSIZEMULT;
                        float DMG= conf::SPIKEMULT*agents[i].spikeLength*max(fabs(agents[i].w1),fabs(agents[i].w2))*conf::BOOSTSIZEMULT;

                        agents[j].health-= DMG;

                        if (agents[i].health>2) agents[i].health=2; //cap health at 2
                        agents[i].spikeLength= 0; //retract spike back down

                        agents[i].initEvent(40*DMG,1,1,0); //yellow event means bot has spiked other bot. nice!

                        Vector2f v2(1,0);
                        v2.rotate(agents[j].angle);
                        float adiff= v.angle_between(v2);
                        if (fabs(adiff)<M_PI/2) {
                            //this was attack from the back. Retract spike of the other agent (startle!)
                            //this is done so that the other agent cant right away "by accident" attack this agent
                            agents[j].spikeLength= 0;
                        }
                        
                        agents[j].spiked= true; //set a flag saying that this agent was hit this turn
                    }
                }
            }
        }
    }
}

void World::brainsTick()
{
    #pragma omp parallel for
    for (int i=0;i<agents.size();i++) {
        agents[i].tick();
    }
//    #pragma omp barrier //testing (GPA)
}

void World::addRandomBots(int num)
{
    for (int i=0;i<num;i++) {
        Agent a;
        a.id= idcounter;
        idcounter++;
        agents.push_back(a);
    }
}

void World::positionOfInterest(int type, float &xi, float &yi) {
	int maxi= -1;
    if(type==1){
        //the interest of type 1 is the oldest agent
        int maxage=-1;
        for(int i=0;i<agents.size();i++){
           if(agents[i].age>maxage) { 
			   maxage = agents[i].age; 
			   maxi=i;
		   }
        }
    } else if(type==2){
        //interest of type 2 is the selected agent
        for(int i=0;i<agents.size();i++){
           if(agents[i].selectflag==1) {maxi=i; break; }
        }
	} else if(type==3){
		//interest of type 3 is most advanced generation
		int maxgen=-1;
        for(int i=0;i<agents.size();i++){
           if(agents[i].gencount>maxgen) { 
			   maxgen = agents[i].gencount; 
			   maxi=i;
		   }
        }
	}
    if(maxi!=-1) {
        xi = agents[maxi].pos.x;
        yi = agents[maxi].pos.y;
    }
}

void World::addCarnivore()
{
    Agent a;
    a.id= idcounter;
    idcounter++;
    a.herbivore= randf(0, 0.1);
    agents.push_back(a);
}

void World::addHerbivore()
{
    Agent a;
    a.id= idcounter;
    idcounter++;
    a.herbivore= randf(0.9, 1);
    agents.push_back(a);
}


void World::addNewByCrossover()
{

    //find two success cases
    int i1= randi(0, agents.size());
    int i2= randi(0, agents.size());
    for (int i=0;i<agents.size();i++) {
        if (agents[i].age > agents[i1].age && randf(0,1)<0.1) {
            i1= i;
        }
        if (agents[i].age > agents[i2].age && randf(0,1)<0.1 && i!=i1) {
            i2= i;
        }
    }

    Agent* a1= &agents[i1];
    Agent* a2= &agents[i2];


    //cross brains
    Agent anew = a1->crossover(*a2);


    //maybe do mutation here? I dont know. So far its only crossover
    anew.id= idcounter;
    idcounter++;
    agents.push_back(anew);
}

void World::reproduce(int ai, float MR, float MR2)
{
    if (randf(0,1)<0.04) MR= MR*randf(1, 10);
    if (randf(0,1)<0.04) MR2= MR2*randf(1, 10);

    agents[ai].initEvent(30,0,0.8,0); //green event means agent reproduced.
    for (int i=0;i<conf::BABIES;i++) {

        Agent a2 = agents[ai].reproduce(MR,MR2);
        a2.id= idcounter;
        idcounter++;
        agents.push_back(a2);

        //TODO fix recording
        //record this
        //FILE* fp = fopen("log.txt", "a");
        //fprintf(fp, "%i %i %i\n", 1, this->id, a2.id); //1 marks the event: child is born
        //fclose(fp);
    }
}

void World::writeReport() //(GPA)
{
	printf("Writing Report, Epoch: %i\n", current_epoch); // (GPA)
    //save all kinds of nice data stuff
    int numherb=0;
    int numcarn=0;
    int topcarn=0;
    int topherb=0;
	int age1_5th=0;//(GPA)
	int age2_5th=0;//(GPA)
	int age3_5th=0;//(GPA)
	int age4_5th=0;//(GPA)
	int age5_5th=0;//(GPA)

    for(int i=0;i<agents.size();i++){
		if(agents[i].herbivore>0.5) numherb++;
		else numcarn++;
 
		if(agents[i].herbivore>0.5 && agents[i].gencount>topherb) topherb= agents[i].gencount;
		if(agents[i].herbivore<=0.5 && agents[i].gencount>topcarn) topcarn= agents[i].gencount;

		if(agents[i].age>=(conf::MAXAGE/5) && agents[i].age<(conf::MAXAGE*2/5)) age1_5th++;
		else if(agents[i].age>=(conf::MAXAGE*2/5) && agents[i].age<(conf::MAXAGE*3/5)) age2_5th++;
		else if(agents[i].age>=(conf::MAXAGE*3/5) && agents[i].age<(conf::MAXAGE*4/5)) age3_5th++;
		else if(agents[i].age>=(conf::MAXAGE*4/5) && agents[i].age<(conf::MAXAGE)) age4_5th++;
		else if(agents[i].age>=(conf::MAXAGE)) age5_5th++;
	}

	FILE* fr = fopen("report.txt", "a");
	fprintf(fr, "Epoch: %i Agents: %i #Herb: %i #Carn: %i #0.5Food: %i TopH: %i TopC: %i Age>1/5Max: %i Age>2/5: %i Age>3/5: %i Age>4/5: %i Age>Max: %i\n",
		current_epoch, numAgents(), numherb, numcarn, numFood(), topcarn, topherb, age1_5th, age2_5th, age3_5th, age4_5th, age5_5th);
	fclose(fr);
}


void World::reset()
{
    agents.clear();
	addRandomBots(conf::NUMBOTS);

	FILE* fr = fopen("report.txt", "w");
	fclose(fr);
}

void World::setClosed(bool close)
{
    CLOSED = close;
}

bool World::isClosed() const
{
    return CLOSED;
}


void World::processMouse(int button, int state, int x, int y)
{
     if (state==0) {        
         float mind=1e10;
         float mini=-1;
         float d;

         for (int i=0;i<agents.size();i++) {
			 d= pow(x-agents[i].pos.x,2)+pow(y-agents[i].pos.y,2);
                 if (d<mind) {
                     mind=d;
                     mini=i;
                 }
             }
         //toggle selection of this agent
         for (int i=0;i<agents.size();i++) agents[i].selectflag=false;
         agents[mini].selectflag= true;
         agents[mini].printSelf();
     }
}
     
void World::draw(View* view, bool drawfood)
{
    //draw food
    if(drawfood) {
        for(int i=0;i<FW;i++) {
            for(int j=0;j<FH;j++) {
                float f= 0.5*food[i][j]/conf::FOODMAX;
                view->drawFood(i,j,f);
            }
        }
    }
    
    //draw all agents
    vector<Agent>::const_iterator it;
    for ( it = agents.begin(); it != agents.end(); ++it) {
        view->drawAgent(*it);
    }
    
    view->drawMisc();
}

std::pair< int,int > World::numHerbCarnivores() const
{
    int numherb=0;
    int numcarn=0;
    for (int i=0;i<agents.size();i++) {
        if (agents[i].herbivore>0.5) numherb++;
        else numcarn++;
    }
    
    return std::pair<int,int>(numherb,numcarn);
}

int World::numAgents() const
{
    return agents.size();
}

int World::numFood() const //(GPA) count food cells with 50% food or more
{
	int numfood=0;
	for(int i=0;i<FW;i++) {
		for(int j=0;j<FH;j++) {
			float f= 0.5*food[i][j]/conf::FOODMAX;
			if(f>conf::FOODMAX/2){
				numfood++;
			}
		}
	}
	return numfood;
}

int World::epoch() const
{
    return current_epoch;
}

