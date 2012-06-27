#include "World.h"

#include <ctime>

#include "settings.h"
#include "helpers.h"
#include "vmath.h"
#include <stdio.h>
#include <iostream>

using namespace std;

World::World() :
		CLOSED(false),
		pcontrol(false),
		pright(0),
		pleft(0),
		pinput1(0)
{
	//inititalize
	reset();

	//add random food
	while(numFood()<conf::INITFOOD) {
		cells[0][randi(0,CW)][randi(0,CH)] = conf::FOODMAX;
	}
	
	addRandomBots(conf::NUMBOTS, 1);
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
			agents[i].age+= 1;
		}		
	}
	
	if (conf::REPORTS_PER_EPOCH>0 && (modcounter%(10000/conf::REPORTS_PER_EPOCH)==0)) {
		//write report and record herbivore/carnivore counts
		std::pair<int,int> num_herbs_carns = numHerbCarnivores();
		numHerbivore[ptr]= num_herbs_carns.first;
		numCarnivore[ptr]= num_herbs_carns.second;
		numHybrid[ptr]= numHybrids();
		numTotal[ptr]= numAgents();
		ptr++;
		if(ptr == numHerbivore.size()) ptr = 0;

		writeReport();
	}

	if (modcounter>=10000) {
		modcounter=0;
		current_epoch++;
	}
	if ((modcounter%conf::FOODADDFREQ==0 && !CLOSED) || numFood()<conf::MINFOOD) {
		cx=randi(0,CW);
		cy=randi(0,CH);
		cells[0][cx][cy]= conf::FOODMAX;
	}

	for(cx=0;cx<CW;cx++){
		for(cy=0;cy<CH;cy++){
			//food = cells[0]...
			if (cells[0][cx][cy]>0) {
				cells[0][cx][cy]+= conf::FOODGROWTH; //food quantity is changed by FOODGROWTH

				if (randf(0,1)<conf::FOODSPREAD && cells[0][cx][cy]>0) {
					int ox= randi(cx-1-conf::FOODRANGE,cx+2+conf::FOODRANGE);
					int oy= randi(cy-1-conf::FOODRANGE,cy+2+conf::FOODRANGE);
					if (ox<1) ox= CW-1;
					if (ox>CW-1) ox= 0;
					if (oy<1) oy= CH-1;
					if (oy>CH-1) oy= 0;
					cells[0][ox][oy]+= conf::FOODMAX/3;
				}
			}
			cells[0][cx][cy]= capCell(cells[0][cx][cy], conf::FOODMAX); //cap food at FOODMAX

			//meat = cells[1]...
			if (cells[1][cx][cy]>0 && cells[1][cx][cy]<=conf::MEATMAX) {
				cells[1][cx][cy] -= conf::MEATDECAY/cells[1][cx][cy]; //meat decays exponentially
			}
			cells[1][cx][cy]= capCell(cells[1][cx][cy], conf::MEATMAX); //cap at MEATMAX
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

	//brains tick. computes in[] -> out[]
	brainsTick();

	//read output and process consequences of bots on environment. requires out[]
	processOutputs();

	//process bots:

	for (int i=0;i<agents.size();i++) {
		//health and deaths
		float baseloss= 0.00001*(abs(agents[i].w1) + abs(agents[i].w2))/2;

		baseloss += agents[i].age/conf::MAXAGE*conf::AGEDAMAGE; //getting older reduces health.

		if (agents[i].boost) { //if boosting, init baseloss + age loss is multiplied
			baseloss *= conf::BOOSTSIZEMULT*1.2;
		}

		//process temperature preferences
		//calculate temperature at the agents spot. (based on distance from horizontal equator)
		float dd= 2.0*abs(agents[i].pos.y/conf::HEIGHT - 0.5);
		float discomfort= sqrt(abs(dd-agents[i].temperature_preference));
		if (discomfort<0.08) discomfort=0;
		baseloss += conf::TEMPERATURE_DISCOMFORT*discomfort; //add to baseloss

		//baseloss then gets multiplied by metabolism
		baseloss *= agents[i].metabolism;

		agents[i].health -= baseloss;
	}
	
	for (int i=0;i<agents.size();i++) {
		//handle reproduction
		if (agents[i].repcounter<0 && agents[i].health>0.65 && modcounter%15==0) { 
			//agent is healthy and is ready to reproduce.
			for (int j=0;j<agents.size();j++) {
				float d= (agents[i].pos-agents[j].pos).length();
				float deviation= abs(agents[i].species - agents[j].species); //species deviation check
				if (d<conf::FOOD_DISTRIBUTION_RADIUS && i!=j && agents[j].give>0.5 && deviation<=conf::MAXDEVIATION) {
					//this adds conf::BABIES new agents to agents[], but with two parents
					reproduce(i, j, agents[i].MUTRATE1, agents[i].MUTRATE2, agents[j].MUTRATE1, agents[j].MUTRATE2);
					agents[i].health -= 0.3; //reduce health of birthing parent; not as much as assexual reproduction
					agents[i].repcounter= conf::REPRATE;
					agents[j].repcounter= conf::REPRATE;
					break;
				} else if (j==agents.size()-1 && agents[i].repcounter!=conf::REPRATE && agents[i].give<=0.5 && randf(0,1)<0.01){
					//this adds conf::BABIES new agents to agents[], with just one parent
					reproduce(i, i, agents[i].MUTRATE1, agents[i].MUTRATE2, agents[i].MUTRATE1, agents[i].MUTRATE2);
					agents[i].health -= 0.65; //reduce health of birthing parent
					agents[i].repcounter= conf::REPRATE;
					break;
				}
			}
		}
	}

	for (int i=0;i<agents.size();i++) {
		//remove dead agents. first distribute meat
		if (agents[i].health<=0) { 
		
			cx= (int) agents[i].pos.x/conf::CZ;
			cy= (int) agents[i].pos.y/conf::CZ;

			float meat= cells[1][cx][cy];
			float agemult= 1.0;
			float spikedmult= 0.5;
			if(agents[i].age<10) agemult= agents[i].age*0.1; //young killed agents should give very little resources until age 10
			if(agents[i].spiked) spikedmult= 1; //agents which were spiked will give even more meat to the table

			meat+= conf::MEATVALUE*conf::MEATMAX*agemult*spikedmult;
			cells[1][cx][cy]= capCell(meat,conf::MEATMAX);
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
		while (agents.size()<conf::NUMBOTS) {
			addRandomBots(1);
		}
		if (modcounter%200==0) {
			if (randf(0,1)<0.5){
				addRandomBots(1); //every now and then add random bots in
			}
		}
	}


}

void World::setInputs()
{
	// R1 G1 B1  R2 G2 B2  R3 G3 B3  R4 G4 B4 HEALTH CLOCK1 CLOCK2 SOUND HEARING SMELL BLOOD TEMP_DISCOMFORT PLAYER_INPUT1
	// 0  1  2   3  4  5   6  7  8   9  10 11   12	 13	 14	15	 16	 17	18	   19			   20

	float PI8=M_PI/8/2; //pi/8/2
	float PI38= 3*PI8; //3pi/8/2
	float PI4= M_PI/4;
   
	#pragma omp parallel for
	for (int i=0;i<agents.size();i++) {
		Agent* a= &agents[i];

		//HEALTH
		a->in[12]= cap(a->health/2); //divide by 2 since health is in [0,2]

		//FOOD
		cx= 0;
		cy= 0;
//		a->in[3]= cells[0][cx][cy]/conf::FOODMAX;

//		a->in[7]= cells[1][cx][cy]/conf::MEATMAX;

		//SOUND SMELL EYES
//		vector<float> p(NUMEYES,0);
		vector<float> r(NUMEYES,0);
		vector<float> g(NUMEYES,0);
		vector<float> b(NUMEYES,0);
					   
		float soaccum=0;
		float smaccum=0;
		float hearaccum=0;

		//BLOOD ESTIMATOR
		float blood= 0;

		//cell sence

		int minx, maxx, miny, maxy;
		int scx= (int) (a->pos.x/conf::CZ);
		int scy= (int) (a->pos.y/conf::CZ);

		minx= max((scx-1-conf::DIST/conf::CZ),(float)0);
		maxx= min((scx+2+conf::DIST/conf::CZ),(float)CW);
		miny= max((scy-1-conf::DIST/conf::CZ),(float)0);
		maxy= min((scy+2+conf::DIST/conf::CZ),(float)CH);

		for(scx=minx;scx<maxx;scx++){
			for(scy=miny;scy<maxy;scy++){
				if (cells[0][scx][scy]==0 && cells[1][scx][scy]) continue;
				Vector2f cellpos= Vector2f((float)(scx*conf::CZ+conf::CZ/2),(float)(scy*conf::CZ+conf::CZ/2)); //find midpoint of the cell
				float d= (a->pos-cellpos).length();

				if (d<conf::DIST) {
					float angle= (cellpos-a->pos).get_angle();

					for(int q=0;q<NUMEYES;q++){
						float aa = a->angle + a->eyedir[q];
						if (aa<-M_PI) aa += 2*M_PI;
						if (aa>M_PI) aa -= 2*M_PI;
						
						float diff1= aa- angle;
						if (fabs(diff1)>M_PI) diff1= 2*M_PI- fabs(diff1);
						diff1= fabs(diff1);
						
						float fov = a->eyefov[q];
						if (diff1<fov) {
							//we see this cell with this eye. Accumulate stats
							float mul1= a->eyesensmod*(fabs(fov-diff1)/fov)*((conf::DIST-d)/conf::DIST)*(d/conf::DIST)*2;
							r[q] += mul1*0.1*cells[1][scx][scy]; //meat looks red
							g[q] += mul1*0.1*cells[0][scx][scy]; //plants are green
							//b[q] += mul1*a2->blu;
						}
					}
				}
			}
		}
					
		for (int j=0;j<agents.size();j++) {
			if (i==j) continue;
			Agent* a2= &agents[j];

			if (a->pos.x<a2->pos.x-conf::DIST || a->pos.x>a2->pos.x+conf::DIST
					|| a->pos.y>a2->pos.y+conf::DIST || a->pos.y<a2->pos.y-conf::DIST) continue;

			float d= (a->pos-a2->pos).length();

			if (d<conf::DIST) {

				//smell
				smaccum+= 1-d/conf::DIST;

				//sound
				soaccum+= (1-d/conf::DIST)*(max(fabs(a2->w1),fabs(a2->w2)));

				//hearing. Listening to other agents
				hearaccum+= a2->soundmul*(1-d/conf::DIST);

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
						float mul1= a->eyesensmod*(fabs(fov-diff1)/fov)*(1-d/conf::DIST)*(1-d/conf::DIST);
//						p[q] += mul1*(d/conf::DIST);
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
					float mul4= ((PI38-diff4)/PI38)*(1-d/conf::DIST);
					//if we can see an agent close with both eyes in front of us
					blood+= mul4*(1-agents[j].health/2); //remember: health is in [0 2]
					//agents with high life dont bleed. low life makes them bleed more
				}
			}
		}

		//temperature varies from 0 to 1 across screen.
		//it is 0 at equator (in middle), and 1 on edges. Agents can sense discomfort
		float dd= 2.0*abs(a->pos.y/conf::HEIGHT - 0.5);
		float discomfort= abs(dd - a->temperature_preference);
		
		smaccum *= a->smellmod;
		soaccum *= a->soundmod;
		hearaccum *= a->hearmod;
		blood *= a->bloodmod;

		a->in[0]= cap(r[0]);
		a->in[1]= cap(g[0]);
		a->in[2]= cap(b[0]);
		
		a->in[3]= cap(r[1]);
		a->in[4]= cap(g[1]);
		a->in[5]= cap(b[1]);

		a->in[6]= cap(r[2]);
		a->in[7]= cap(g[2]);
		a->in[8]= cap(b[2]);

		a->in[9]= cap(r[3]);
		a->in[10]= cap(g[3]);
		a->in[11]= cap(b[3]);

		a->in[13]= abs(sin(modcounter/a->clockf1));
		a->in[14]= abs(sin(modcounter/a->clockf2));
		a->in[15]= cap(soaccum);
		a->in[16]= cap(hearaccum);
		a->in[17]= cap(smaccum);
		a->in[18]= cap(blood);
		a->in[19]= discomfort;
		
		if (a->selectflag) {
			a->in[20]= pinput1;
		} else {
			a->in[20]= 0;
		}
	}
}

void World::processOutputs()
{
	//assign meaning
	//LEFT RIGHT R G B SPIKE BOOST SOUND_MULTIPLIER GIVING CHOICE STIMULANT
	// 0	1	2 3 4   5	 6		 7			 8	  9	  10
	#pragma omp parallel for
	for (int i=0;i<agents.size();i++) {
		Agent* a= &agents[i];

		if (pcontrol && a->selectflag) {
			a->w1= pright;
			a->w2= pleft;
		} else {
			a->w1= a->out[0]; //-(2*a->out[0]-1);
			a->w2= a->out[1]; //-(2*a->out[1]-1);
		}
		a->red+= 0.2*(a->out[2]-a->red);
		a->gre+= 0.2*(a->out[3]-a->gre);
		a->blu+= 0.2*(a->out[4]-a->blu);
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
	#pragma omp parallel for
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
		if (a->pos.x<0) a->pos.x+= conf::WIDTH;
		if (a->pos.x>=conf::WIDTH) a->pos.x-= conf::WIDTH;
		if (a->pos.y<0) a->pos.y+= conf::HEIGHT;
		if (a->pos.y>=conf::HEIGHT) a->pos.y-= conf::HEIGHT;
	}

	//process food intake from cells
	#pragma omp parallel for
	for (int i=0;i<agents.size();i++) {
		Agent* a= &agents[i];

		int scx= (int) a->pos.x/conf::CZ;
		int scy= (int) a->pos.y/conf::CZ;

		float food= cells[0][scx][scy];
		float itk;
		if (food>0 && a->health<2) {
			//agent eats the food
			float speedmul= (1-(abs(a->w1)+abs(a->w2))/2)*0.7 + 0.3;
			//herbivores gain more from plant food while going slow;
			itk= min(food,conf::FOODINTAKE)*a->herbivore*speedmul;
			a->health+= a->metabolism*itk;
			a->repcounter -= a->metabolism*itk;
			cells[0][scx][scy]-= min(food,conf::FOODWASTE*a->metabolism*itk);
		}

		float meat= cells[1][scx][scy];
		if (meat>0 && a->health<2) {
			//agent eats meat
			itk= min(meat,conf::MEATINTAKE)*(1-a->herbivore)*(1-a->herbivore);
			//carnivores gain more from meat, and no herbivores are allowed
			a->health += a->metabolism*itk;
			a->repcounter -= a->metabolism*conf::REPMULT*itk; //good job, can use meat to make copies
			cells[1][scx][scy]-= min(meat,conf::MEATWASTE*a->metabolism*itk);
		}

		if (a->health>2) a->health= 2;
	}

	//process giving and receiving of food
	#pragma omp parallel for
	for (int i=0;i<agents.size();i++) {
		if (agents[i].give>0.5) {
			for (int j=0;j<agents.size();j++) {
				float d= (agents[i].pos-agents[j].pos).length();
				if (d<conf::FOOD_SHARING_DISTANCE && agents[j].health<2) {
					//initiate transfer
					agents[j].health += conf::FOODTRANSFER;
					agents[i].health -= conf::FOODTRANSFER;
					agents[j].dfood += conf::FOODTRANSFER; //only for drawing
					agents[i].dfood -= conf::FOODTRANSFER;
				}
			}
		}
	}

	//process spike dynamics
	if (modcounter%2==0) { //we dont need to do this TOO often. can save efficiency here since this is n^2 op in #agents
		#pragma omp parallel for
		for (int i=0;i<agents.size();i++) {
			Agent* a= &agents[i];

			for (int j=0;j<agents.size();j++) {
				Agent* a2= &agents[j];
				
				if (i==j) continue;
				float d= (a->pos-a2->pos).length();

				if (d<2*conf::BOTRADIUS) {
					//fix physics
					float ov= (2*conf::BOTRADIUS-d);
					if (ov>0 && d!=0) {
						float ff1= ov/d*0.5; //possibility of weight, inertia here
						float ff2= ov/d*0.5;
						a->pos.x-= (a2->pos.x-a->pos.x)*ff1;
						a->pos.y-= (a2->pos.y-a->pos.y)*ff1;
						a2->pos.x+= (a2->pos.x-a->pos.x)*ff2;
						a2->pos.y+= (a2->pos.y-a->pos.y)*ff2;

						if (a->pos.x>conf::WIDTH) a->pos.x-= conf::WIDTH;
						if (a->pos.y>conf::HEIGHT) a->pos.y-= conf::HEIGHT;
						if (a2->pos.x>conf::WIDTH) a2->pos.x-= conf::WIDTH;
						if (a2->pos.y>conf::HEIGHT) a2->pos.y-= conf::HEIGHT;
						if (a->pos.x<0) a->pos.x+= conf::WIDTH;
						if (a->pos.y<0) a->pos.y+= conf::HEIGHT;
						if (a2->pos.x<0) a2->pos.x+= conf::WIDTH;
						if (a2->pos.y<0) a2->pos.y+= conf::HEIGHT;

//						printf("%f, %f, %f, %f, and %f\n", a->pos.x, a->pos.y, a2->pos.x, a2->pos.y, ov);
					}
				}

				//low speed doesn't count, nor does a small spike (duh)
				if(a->spikeLength<0.2 || a->w1<0.2 || a->w2<0.2) continue;
				else if(d<=2*conf::BOTRADIUS + 3*conf::BOTRADIUS*a->spikeLength) {

					//these two are in collision and agent i has extended spike and is going decent fast!
					Vector2f v(1,0);
					v.rotate(a->angle);
					float diff= v.angle_between(a2->pos-a->pos);
					if (fabs(diff)<M_PI/8) {
						//bot i is also properly aligned!!! that's a hit
						float DMG= conf::SPIKEMULT*a->spikeLength*max(fabs(a->w1),fabs(a->w2))*conf::BOOSTSIZEMULT;

						a2->health-= DMG;

						if (a->health>2) a->health=2; //cap health at 2
						a->spikeLength= 0; //retract spike back down

						a->initEvent(20*DMG,1,1,0); //yellow event means bot has spiked the other bot. nice!
						if (a2->health<=0) a->initEvent(20*DMG,1,0,0); //red event means bot has killed the other bot. nice!

						Vector2f v2(1,0);
						v2.rotate(a2->angle);
						float adiff= v.angle_between(v2);
						if (fabs(adiff)<M_PI/2) {
							//this was attack from the back. Retract spike of the other agent (startle!)
							//this is done so that the other agent cant right away "by accident" attack this agent
							a2->spikeLength= 0;
						}
						
						a2->spiked= true; //set a flag saying that this agent was hit this turn
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
}

void World::positionOfInterest(int type, float &xi, float &yi) {
	int maxi= -1;
	if (type==1) {
		//the interest of type 1 is the oldest agent
		int maxage= -1;
		for (int i=0;i<agents.size();i++) {
		   if (agents[i].age>maxage) { 
			   maxage= agents[i].age; 
			   maxi= i;
		   }
		}
	} else if(type==2){
		//interest of type 2 is the selected agent
		for (int i=0;i<agents.size();i++) {
			if (agents[i].selectflag==1) {maxi= i; break; }
		}
	} else if(type==3){
		//interest of type 3 is most advanced generation
		int maxgen= -1;
		for (int i=0;i<agents.size();i++) {
		   if (agents[i].gencount>maxgen) { 
			   maxgen= agents[i].gencount; 
			   maxi= i;
		   }
		}
	} else if(type==4){
		//interest of type 3 is healthiest
		float maxhealth= -1;
		for (int i=0;i<agents.size();i++) {
			if (agents[i].health>maxhealth) {
				maxhealth= agents[i].health;
					maxi= i;
			}
		}
	}
	if (maxi!=-1) {
		xi= agents[maxi].pos.x;
		yi= agents[maxi].pos.y;
	}
}

void World::addCarnivore()
{
	Agent a;
	a.id= idcounter;
	idcounter++;
	a.herbivore= randf(0, 0.2);
	agents.push_back(a);
}

void World::addHerbivore()
{
	Agent a;
	a.id= idcounter;
	idcounter++;
	a.herbivore= randf(0.8, 1);
	agents.push_back(a);
}

void World::addRandomBots(int num, int type)
{
	for (int i=0;i<num;i++) {
		Agent a;
		if (type==1) a.herbivore= randf(0.8, 1); //herbivore
		else if (type==2) a.herbivore= randf(0, 0.2); //carnivore
//		else { //choose stomach type based on what's on the cell. Defaults to herbivore
//			cx= (int) a.pos.x/conf::CZ;
//			cy= (int) a.pos.y/conf::CZ;
//			if (cells[0][cx][cy] > cells[1][cx][cy]) a.herbivore= randf(0.8,1);
//			else if (cells[0][cx][cy] < cells[1][cx][cy]) a.herbivore= randf(0,0.2);
//		}
		a.id= idcounter;
		idcounter++;
		agents.push_back(a);
	}
}

void World::reproduce(int ai, int bi, float aMR, float aMR2, float bMR, float bMR2)
{
	float MR= min(aMR,bMR);
	float MR2= min(aMR2,bMR2); //Using min because max, though correct, will lead to rampant mutation (is that perhaps wanted?) will test
	if (randf(0,1)<0.04) MR= MR*randf(1, 10);
	if (randf(0,1)<0.04) MR2= MR2*randf(1, 10);

	Agent that= agents[bi];
	Agent a= agents[ai];
	if (ai==bi){
		that= agents[ai];
		agents[ai].initEvent(30,0,0.8,0); //green event means agent asexually reproduced
	}else{
		agents[ai].initEvent(30,0,0,0.8);
		agents[bi].initEvent(30,0,0,0.8); //blue events mean agents sexually reproduced.
	}

	for (int i=0;i<conf::BABIES;i++) {
		Agent a2 = a.reproduce(that,MR,MR2);
		if (ai!=bi) a2.hybrid= true; //if parents are not the same agent (sexual reproduction), then mark the child
		a2.id= idcounter;
		idcounter++;
		agents.push_back(a2);
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
	vector<int> species;
	species.resize(1,0);

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

		for(int s=0;s<species.size();s++){
			if(abs(species[s]-agents[i].species)>conf::MAXDEVIATION){
				species.push_back(agents[i].species);
				break;
			}
		}
	}

	FILE* fr = fopen("report.txt", "a");
	fprintf(fr, "Epoch: %i Agents: %i #Herb: %i #Carn: %i #0.5Food: %i TopH: %i TopC: %i Age>1/5Max: %i Age>2/5: %i Age>3/5: %i Age>4/5: %i Age>Max: %i NumSpecies: %i\n",
		current_epoch, numAgents(), numherb, numcarn, numFood(), topherb, topcarn, age1_5th, age2_5th, age3_5th, age4_5th, age5_5th, species.size());
	fclose(fr);
}


void World::reset()
{
	current_epoch= 0;
	modcounter= 0;
	idcounter= 0;
	CW= conf::WIDTH/conf::CZ;
	CH= conf::HEIGHT/conf::CZ; //note: can add custom variables from loaded savegames here possibly

	agents.clear();

	//handle layers
	for(cx=0;cx<CW;cx++){
		for(cy=0;cy<CH;cy++){
			for(int l=0;l<LAYERS;l++){
				cells[l][cx][cy]= 0;
			}
			cells[2][cx][cy]= 2.0*abs((float)cy/CH - 0.5);
		}
	}

//	addRandomBots(conf::NUMBOTS);

	//open report file; null it up if it exists
	FILE* fr = fopen("report.txt", "w");
	fclose(fr);

	numCarnivore.resize(200, 0);
	numHerbivore.resize(200, 0);
	numHybrid.resize(200, 0);
	numTotal.resize(200, 0);
	ptr=0;
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
		 if (mind<1000) {
			 //toggle selection of this agent
			 for (int i=0;i<agents.size();i++) {
				if(i!=mini) agents[i].selectflag=false;
			 }
			 agents[mini].selectflag= !agents[mini].selectflag;
			 agents[mini].printSelf();
			 setControl(false);
		 }
	 }
}
	 
void World::draw(View* view, int layer)
{
	//draw cell layer
	if(layer!=0) {
		for(int x=0;x<CW;x++) {
			for(int y=0;y<CH;y++) {
				float val;
				if (layer==1) { 
					//plant food
					val= 0.5*cells[0][x][y]/conf::FOODMAX;
				} else if (layer==2) { 
					//meat food
					val= 0.5*cells[1][x][y]/conf::MEATMAX;
				} else if (layer==3) { 
					//temperature
					val = cells[2][x][y];
				}

				view->drawCell(x,y,val);
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
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			float food= 0.5*cells[0][i][j]/conf::FOODMAX;
			if(food>conf::FOODMAX/2){
				numfood++;
			}
		}
	}
	return numfood;
}

int World::numMeat() const //(GPA) count food cells with 50% food or more
{
	int nummeat=0;
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			float meat= 0.5*cells[1][i][j]/conf::MEATMAX;
			if(meat>conf::MEATMAX/2){
				nummeat++;
			}
		}
	}
	return nummeat;
}

int World::numHybrids() const
{
	int numhybrid= 0;
	for (int i=0;i<agents.size();i++) {
		if (agents[i].hybrid) numhybrid++;
	}
	
	return numhybrid;
}

int World::epoch() const
{
	return current_epoch;
}

void World::setControl(bool state)
{
	//reset left and right wheel controls 
	pleft= 0;
	pright= 0;

	pcontrol= state;
}

void World::cellsRandomFill(int layer, float amount, int number)
{
	for (int i=0;i<number;i++) {
		cx=randi(0,CW);
		cy=randi(0,CH);
		cells[layer][cx][cy]= amount;
	}
}

float World::capCell(float a, float top) const
{
	return min(top,max(a,(float)0));
}
