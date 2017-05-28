#include "TSherlock.hpp"

void TSherlock::init()
{
	// allocate the mapping from the finest catory
        for (int e = 0; e < nEpoch; e ++) {
		itemEmbedings[e] = map<int, vector<int> > ();
		
		for (int x = 0; x < nItems; x ++) {
			itemEmbedings[e][x] = vector<int> ();
		}
	}

	int ind = 0;
	vector<int> layer_count;
	for (int e = 0; e < nEpoch; e ++){
		unordered_map<string, int> catMap;
		for (unsigned i = 0; i < dims.size(); i ++) {
			int cur_size = catMap.size();
			for (int x = 0; x < nItems; x ++) {
				string path = "";
				for (unsigned j = 0; j <= i; j ++) {
					path = path + corp->productCategories[corp->rItemIds[x]].at(j);
					if (j < i) {
						path += "|";
					}
				}	
				if (catMap.find(path) == catMap.end()) {
					catMap[path] = ind;
					ind += dims[i];
				}
				for (int c = 0; c < dims[i]; c ++) { 
					itemEmbedings[e][x].push_back(catMap[path] + c);
				}
			}
			if (e == 0) {
				layer_count.push_back(catMap.size() - cur_size);
			}
		}
	}

	printf("==== Category Tree: per-layer count ====\n");
	for (unsigned i = 0; i < layer_count.size(); i ++) {
		printf("layer %d: %d\n", i, layer_count[i]);
	}

	K2 = 0;
	for (unsigned i = 0; i < dims.size(); i ++) {
		K2 += dims[i];
	}

	// sanity check
	for (int e = 0; e < nEpoch; e ++) {

		for (int x = 0; x < nItems; x ++) {
			if ((int)itemEmbedings[e][x].size() != K2) {
				printf("Oops! item %d's itemEmbedings vector's size = %d, which is not K2 = %d\n", x, (int)itemEmbedings[e][x].size(), K2);
				exit(0);
			}
		}
	}

	// total number of parameters
	nVec = 0;
	for (unsigned i = 0; i < dims.size(); i ++) {
		nVec += layer_count[i] * dims[i];
	}
	
	NW = K * (nUsers + nItems )	// latent factors
		 + K2 * nUsers + nVec * corp->imFeatureDim  // visual factors
		 + corp->imFeatureDim // beta_cnn
		 + nVec * corp->imFeatureDim * nEpoch //
		 + corp->imFeatureDim * nEpoch; //

	// Initialize parameters
	W = new double[NW];
	bestW = new double[NW];

	// parameter initialization
	for (int w = 0; w < NW; w ++) {
		if (w < nItems || w >= NW - corp->imFeatureDim) {
			W[w] = 0;
		} else {
			W[w] = 1.0 * rand() / RAND_MAX;
		}
	}
	
	getParametersFromVector(W, &beta_item_t, &gamma_user, &gamma_item, &theta_user, &U, &U_t, &beta_cnn, &beta_cnn_t, INIT);

	/* speed up at test time*/
	visual_bias = new double [nItems];
	visual_item = new double* [nItems];
	for (int i = 0; i < nItems; i ++) {
		visual_item[i] = new double [K2];
	}
	
	visual_bias_ep = new double* [nEpoch];
	for (int e = 0; e < nEpoch; e ++) {
		visual_bias_ep[e] = new double [nItems];
	}
	
	visual_item_ep = new double** [nEpoch];
	for (int e = 0; e < nEpoch; e ++) {
		visual_item_ep[e] = new double* [nItems];
		for (int i = 0; i < nItems; i ++) {
			visual_item_ep[e][i] = new double [K2];
		}
	}
}

void TSherlock::cleanUp()
{
	getParametersFromVector(W, &beta_item_t, &gamma_user, &gamma_item, &theta_user, &U, &U_t, &beta_cnn, &beta_cnn_t, FREE);

	delete [] W;
	delete [] bestW;

	for (int i = 0; i < nItems; i ++) {
		delete [] visual_item[i];
	}
	delete [] visual_item;
	delete [] visual_bias;
	
	for (int e = 0; e < nEpoch; e ++) {
		for (int i = 0; i < nItems; i ++) {
			delete [] visual_item_ep[e][i];
		}
		delete [] visual_item_ep[e];
		delete [] visual_bias_ep[e];
	}
}

void TSherlock::getParametersFromVector(	double*   g,
										double*** beta_item_t,
										double*** gamma_user, 
										double*** gamma_item,
										double*** theta_user,
										double*** U,
										double**** U_t,
										double**  beta_cnn,
										double*** beta_cnn_t,
										action_t  action)
{
	if (action == FREE) {
		delete [] (*beta_item_t);
		delete [] (*gamma_user);
		delete [] (*gamma_item);
		delete [] (*theta_user);
		delete [] (*U);
		delete [] (*beta_cnn_t);
		for (int e = 0; e < nEpoch; e ++) {
			delete [] (*U_t)[e];
		}
		return;
	}

	if (action == INIT)	{
		*beta_item_t = new double* [nEpoch];
		*gamma_user = new double* [nUsers];
		*gamma_item = new double* [nItems];
		*theta_user = new double* [nUsers];
		*U = new double* [nVec];
		*U_t = new double** [nEpoch];
		for (int e = 0; e < nEpoch; e ++) {
			(*U_t)[e] = new double* [nVec];
		}
		*beta_cnn_t = new double* [nEpoch];
	}

	int ind = 0;

//	*beta_item = g + ind;
//	ind += nItems;

	for (int e = 0; e < nEpoch; e ++) {
		(*beta_item_t)[e] = g + ind;
		ind += nItems;
	}

	for (int u = 0; u < nUsers; u ++) {
		(*gamma_user)[u] = g + ind;
		ind += K;
	}
	for (int i = 0; i < nItems; i ++) {
		(*gamma_item)[i] = g + ind;
		ind += K;
	}

	for (int u = 0; u < nUsers; u ++) {
		(*theta_user)[u] = g + ind;
		ind += K2;
	}

	for (int k = 0; k < nVec; k ++) {
		(*U)[k] = g + ind;
		ind += corp->imFeatureDim;
	}

	for (int e = 0; e < nEpoch; e ++) {
		for (int k = 0; k < nVec; k ++) {
			(*U_t)[e][k] = g + ind;
			ind += corp->imFeatureDim;
		}
	}

	*beta_cnn = g + ind;
	ind += corp->imFeatureDim;

	for (int e = 0; e < nEpoch; e ++) {
		(*beta_cnn_t)[e] = g + ind;
		ind += corp->imFeatureDim;
	}

	if (ind != NW) {
		printf("Got bad index (Sherlock.cpp, line %d)", __LINE__);
		exit(1);
	}
}

void TSherlock::getVisualAuxForItem(int x, int ep)
{
	// cnn features
	vector<pair<int, float> >& feat = corp->imageFeatures[x];
	
	// visual factors
	int ind = 0;
	for (vector<int>::iterator it = itemEmbedings[ep][x].begin(); it != itemEmbedings[ep][x].end(); it ++) {
		visual_item[x][ind] = 0;
		for (unsigned j = 0; j < feat.size(); j ++) {
			visual_item[x][ind] += U[*it][feat[j].first] * feat[j].second;
			visual_item_ep[ep][x][ind] += U_t[ep][*it][feat[j].first] * feat[j].second;
		}
		ind ++;
	}

	// visual bias
	visual_bias[x] = 0;
	visual_bias_ep[ep][x] = 0;
	for (unsigned i = 0; i < feat.size(); i ++) {
		visual_bias[x] += beta_cnn[feat[i].first] * feat[i].second;
		visual_bias_ep[ep][x] += beta_cnn_t[ep][feat[i].first] * feat[i].second;
	}

	
}

void TSherlock::getVisualAux()
{
	#pragma omp parallel for schedule(dynamic)
	for (int e = 0; e < nEpoch; e ++) {

		for (int x = 0; x < nItems; x ++) {
			getVisualAuxForItem(x, e);
		}
	}
}

/*double TSherlock::prediction(vote* v)
{
	return  beta_item[v->item] 										\
			+ inner(gamma_user[v->user], gamma_item[v->item], K) 	\
			+ inner(visual_item[v->item], theta_user[v->user], K2)	\
			+ visual_bias[v->item];
}
*/
double TSherlock::prediction(int user, int item, int ep)
{
	return  beta_item_t[ep][item] 									\
			+ inner(gamma_user[user], gamma_item[item], K) 		\
			+ inner(visual_item[item], theta_user[user], K2) 	\
			+ inner(visual_item_ep[ep][item], theta_user[user], K2) \
			+ visual_bias[item] + visual_bias_ep[ep][item];
}

void TSherlock::updateFactors(int user_id, int pos_item_id, int neg_item_id, int ep, double learn_rate)
{
	getVisualAuxForItem(pos_item_id, ep);
	getVisualAuxForItem(neg_item_id, ep);

	// x_uij = prediction(user_id, pos_item_id) - prediction(user_id, neg_item_id);
	double x_uij = beta_item_t[ep][pos_item_id] - beta_item_t[ep][neg_item_id];
	x_uij += inner(gamma_user[user_id], gamma_item[pos_item_id], K) - inner(gamma_user[user_id], gamma_item[neg_item_id], K);
	x_uij += inner(theta_user[user_id], visual_item[pos_item_id], K2) - inner(theta_user[user_id], visual_item[neg_item_id], K2);
	x_uij += inner(theta_user[user_id], visual_item_ep[ep][pos_item_id], K2) - inner(theta_user[user_id], visual_item_ep[ep][neg_item_id], K2);
	x_uij += visual_bias[pos_item_id] - visual_bias[neg_item_id];
	x_uij += visual_bias_ep[ep][pos_item_id] - visual_bias_ep[ep][neg_item_id];	

	double deri = 1 / (1 + exp(x_uij));

	beta_item_t[ep][pos_item_id] += learn_rate * ( deri - biasReg * beta_item_t[ep][pos_item_id]);
	beta_item_t[ep][neg_item_id] += learn_rate * (-deri - biasReg * beta_item_t[ep][neg_item_id]);

	// adjust latent factors
	for (int f = 0; f < K; f ++) {
		double w_uf = gamma_user[user_id][f];
		double h_if = gamma_item[pos_item_id][f];
		double h_jf = gamma_item[neg_item_id][f];
 
		gamma_user[user_id][f]     += learn_rate * ( deri * (h_if - h_jf) - lambda * w_uf);
		gamma_item[pos_item_id][f] += learn_rate * ( deri * w_uf - lambda * h_if);
		gamma_item[neg_item_id][f] += learn_rate * (-deri * w_uf - lambda / 10.0 * h_jf);
	}

	// adjust embedding matrix
	vector<pair<int, float> >& feat_pos = corp->imageFeatures[pos_item_id];
	for (int k = 0; k < K2; k ++) {
		int r = itemEmbedings[ep][pos_item_id][k];
		double deri_k = deri * theta_user[user_id][k];
		for (unsigned j = 0; j < feat_pos.size(); j ++) {
			int c = feat_pos[j].first;
			U[r][c] += learn_rate * (deri_k * feat_pos[j].second - lambda2 * U[r][c]);
			U_t[ep][r][c] += learn_rate * (deri_k * feat_pos[j].second - lambda2 * U_t[ep][r][c]);
		}
	}

	vector<pair<int, float> >& feat_neg = corp->imageFeatures[neg_item_id];
	for (int k = 0; k < K2; k ++) {
		int r = itemEmbedings[ep][neg_item_id][k];
		double deri_k = - deri * theta_user[user_id][k];
		for (unsigned j = 0; j < feat_neg.size(); j ++) {
			int c = feat_neg[j].first;
			U[r][c] += learn_rate * (- deri_k * feat_neg[j].second - lambda2 * U[r][c]);
			U_t[ep][r][c] += learn_rate * (- deri_k * feat_neg[j].second - lambda2 * U_t[ep][r][c]);
		}
	}

	// adjust visual factors
	for (int f = 0; f < K2; f ++) {
		theta_user[user_id][f] += learn_rate * (deri * (visual_item[pos_item_id][f] - visual_item[neg_item_id][f]) - lambda * theta_user[user_id][f]);
	}

	// adjust visual bias vector 
	for (unsigned j = 0; j < feat_pos.size(); j ++) {
		int c = feat_pos[j].first;
		beta_cnn[c] += learn_rate * (deri * feat_pos[j].second - biasReg * beta_cnn[c]);
		beta_cnn_t[ep][c] += learn_rate * (deri * feat_pos[j].second - biasReg * beta_cnn_t[ep][c]);
	}
	for (unsigned j = 0; j < feat_neg.size(); j ++) {
		int c = feat_neg[j].first;
		beta_cnn[c] += learn_rate * (- deri * feat_neg[j].second - biasReg * beta_cnn[c]);
		beta_cnn_t[ep][c] += learn_rate * (- deri * feat_neg[j].second - biasReg * beta_cnn_t[ep][c]);
	}
}

string TSherlock::toString()
{
	stringstream ss;
	ss << "Sherlock____K_" << K;
	ss << "__Alloc_";
	for (unsigned i = 0; i < dims.size(); i ++) {
		ss << dims[i] << "_";
	}
	ss << "__lambda_" << lambda  << "_biasReg_" << biasReg;
	return ss.str();
}
