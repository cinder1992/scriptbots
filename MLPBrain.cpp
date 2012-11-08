#include "MLPBrain.h"
using namespace std;


MLPBox::MLPBox()
{

	w.resize(CONNS,0);
	id.resize(CONNS,0);
	type.resize(CONNS,0);

	//constructor
	for (int i=0;i<CONNS;i++) {
		w[i]= randf(-2,2);
		if(w[i]<-1) w[i]= -1;
		if(w[i]>1) w[i]= 1; //make equal valued connections more frequent
		if(randf(0,1)<0.6 && i<BRAINSIZE-OUTPUTSIZE) w[i]=0; //make brains sparse (except for the outputs)
		
		id[i]= randi(0,BRAINSIZE);
		if (randf(0,1)<0.2) id[i]= randi(0,INPUTSIZE); //20% of the brain AT LEAST should connect to input.
		
		type[i] = 0;
		if(randf(0,1)<0.1) type[i] = 1; //make 10% be change sensitive synapses
	}

	kp=randf(0.1,1);
	gw= randf(0,5);
	bias= randf(-1,1);

	out=0;
	oldout=0;
	target=0;
}

MLPBrain::MLPBrain()
{

	//constructor
	for (int i=0;i<BRAINSIZE;i++) {
		MLPBox a; //make a random box and copy it over
		boxes.push_back(a);
		}

	//do other initializations
	init();
}

MLPBrain::MLPBrain(const MLPBrain& other)
{
	boxes = other.boxes;
}

MLPBrain& MLPBrain::operator=(const MLPBrain& other)
{
	if( this != &other )
		boxes = other.boxes;
	return *this;
}


void MLPBrain::init()
{

}

void MLPBrain::tick(vector< float >& in, vector< float >& out)
{
	//do a single tick of the brain
	for (int i=0;i<BRAINSIZE;i++) {
		MLPBox* abox= &boxes[i];
		
		if (i<INPUTSIZE) { //take first few boxes and set their out to in[]. (no need to do these separately, since thay are first)
			abox->out= in[i];
		} else { //then do a dynamics tick and set all targets
			float acc=abox->bias;
			for (int j=0;j<CONNS;j++) {
				int idx=abox->id[j];
				int type = abox->type[j];
				float val= boxes[idx].out;
				
				if(type==1){
					val-= boxes[idx].oldout;
					val*=10;
				}

				float stim= out[10];
				abox->w[j]+= conf::LEARNRATE*stim*(abox->oldout-(1-val)); //modify weights based on matching old output and new input, if stimulant is active

				if (j==CONNS-1) acc+= val*abox->w[j]*(out[9]*2-1);//last connection is affected by to the 10th output, choice
				else acc+= val*abox->w[j];
			}
			
			acc*= abox->gw;
			
			//put through sigmoid
			acc= 1.0/(1.0+exp(-acc));
			
			abox->target= cap(acc);
		}
	}
	

	for (int i=0;i<BRAINSIZE;i++) {
		MLPBox* abox= &boxes[i];

		//back up current out for each box
		abox->oldout = abox->out;

		//make all boxes go a bit toward target
		if (i>=INPUTSIZE) abox->out= abox->out + (abox->target-abox->out)*abox->kp;
	}

	//finally set out[] to the last few boxes output
	for (int i=0;i<OUTPUTSIZE;i++) {
		out[i]= boxes[BRAINSIZE-1-i].out;
	}
}

void MLPBrain::mutate(float MR, float MR2)
{
	for (int j=0;j<BRAINSIZE;j++) {
		MLPBox* abox= &boxes[j];
		if (randf(0,1)<MR*5) {
			int k= randi(0,BRAINSIZE);
			if(k!=j) {
				abox->type= boxes[k].type;
				abox->id= boxes[k].id;
				abox->bias= boxes[k].bias;
				abox->kp= boxes[k].kp;
				abox->type= boxes[k].type;
				abox->w= boxes[k].w;
//				a2.mutations.push_back("box coppied\n");
				continue; //cancel all further mutations to this box
			}
		}

		if (randf(0,1)<MR*5) {
			int rc= randi(0, CONNS);
			abox->type[rc] = 1 - abox->type[rc]; //flip type of synapse
//		  a2.mutations.push_back("synapse switched\n");
			continue; //cancel all further mutations to this box
		}

		if (randf(0,1)<MR*5) {
			int rc= randi(0, CONNS);
			int ri= randi(0,BRAINSIZE);
			abox->id[rc]= ri;
//		  a2.mutations.push_back("connectivity changed\n");
			continue; //cancel all further mutations to this box
		}

		// more likely changes here
		if (randf(0,1)<MR*25) {
			abox->bias+= randn(0, MR2);
//			 a2.mutations.push_back("bias jiggled\n");
		}

		if (randf(0,1)<MR*25) {
			abox->kp+= randn(0, MR2);
			if (abox->kp<0.01) abox->kp=0.01;
			if (abox->kp>1) abox->kp=1;
//			 a2.mutations.push_back("kp jiggled\n");
		}
		
		if (randf(0,1)<MR*10) {
			abox->gw+= randn(0, MR2);
			if (abox->gw<0) abox->gw=0;
//			 a2.mutations.push_back("global weight jiggled\n");
		}

		if (randf(0,1)<MR*10) {
			int rc= randi(0, CONNS);
			abox->w[rc]+= randn(0, MR2);
//		  a2.mutations.push_back("weight jiggled\n");
		}
	}
}

MLPBrain MLPBrain::crossover(const MLPBrain& other)
{
	MLPBrain newbrain(*this);
	
	for (int i=0;i<newbrain.boxes.size(); i++) {
		if(randf(0,1)<0.5){
			newbrain.boxes[i].bias= this->boxes[i].bias;
			newbrain.boxes[i].gw= this->boxes[i].gw;
			newbrain.boxes[i].kp= this->boxes[i].kp;
			for (int j=0;j<newbrain.boxes[i].id.size();j++) {
				newbrain.boxes[i].id[j] = this->boxes[i].id[j];
				newbrain.boxes[i].w[j] = this->boxes[i].w[j];
				newbrain.boxes[i].type[j] = this->boxes[i].type[j];
			}
		
		} else {
			newbrain.boxes[i].bias= other.boxes[i].bias;
			newbrain.boxes[i].gw= other.boxes[i].gw;
			newbrain.boxes[i].kp= other.boxes[i].kp;
			for (int j=0;j<newbrain.boxes[i].id.size();j++) {
				newbrain.boxes[i].id[j] = other.boxes[i].id[j];
				newbrain.boxes[i].w[j] = other.boxes[i].w[j];
				newbrain.boxes[i].type[j] = other.boxes[i].type[j];
			}
		}
	}
	return newbrain;
}

