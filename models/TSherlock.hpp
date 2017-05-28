#pragma once

#include "TimeBPR.hpp"


class TSherlock : public TimeBPR
{
public:
	TSherlock(corpus* corp, int K, int K2, vector<int> dims, double lambda, double lambda2, double biasReg, int nEpoch) 
			: TimeBPR(corp, K, lambda, biasReg, nEpoch) 
			, K2(K2), lambda2(lambda2), dims(dims) {}
	
	~TSherlock(){}
	
	void init();
	void cleanUp();

	void getParametersFromVector(	double*   g,
									double*** beta_item_t,
									double*** gamma_user, 
									double*** gamma_item,
									double*** theta_user,
									double*** U,
									double**** U_t,
									double** beta_cnn,
									double***  beta_cnn_t,
									action_t  action);
	
	void getVisualAux();
	void getVisualAuxForItem(int x, int ep);

	double prediction(vote* v);
	double prediction(int user, int item, int ep);

	void updateFactors(int user_id, int pos_item_id, int neg_item_id, int ep, double learn_rate);

	string toString();

	unordered_map<int, map<int, vector<int> > > itemEmbedings;
	int nVec;
	int K2;
	double lambda2;


	vector<int> dims;
   
	double**  beta_item_t;
	double**  theta_item;
	double*** theta_item_t;
	double**  U;
	double*** U_t;
	double*   beta_cnn;
	double**  beta_cnn_t;
	double**  visual_item;
	double*** visual_item_ep;
	double*   visual_bias;
	double**  visual_bias_ep;
	double**  theta_user;	
};
