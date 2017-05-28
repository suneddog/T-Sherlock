#pragma once

#include "common.hpp"

class corpus
{
public:
	corpus() {}
	~corpus() {}

	vector<vote*> V; // vote

	int nUsers; // Number of users
	int nItems; // Number of items
	int nVotes; // Number of ratings

	map<string, int> userIds; // Maps a user's string-valued ID to an integer
	map<string, int> itemIds; // Maps an item's string-valued ID to an integer

	map<int, string> rUserIds; // Inverse of the above maps
	map<int, string> rItemIds;

	/* For pre-load */
        unordered_set<string> imgAsins;
	map<string, int> uCounts;
	map<string, int> bCounts;

	vector<vector<pair<int, float> > > imageFeatures;
	int imFeatureDim;  // fixed to 4096

	/* For WWW demo */
	vector<double> avgRatingPerItem;
	vector<int> numReviewsPerItem;


        unordered_map<string, vector<string> > productCategories; // Set of categories of each product (products can belong to multiple categories)

        /* Category information */
        int nCategory;
        int* itemCategoryId;
        unordered_map<string,int> nodeIds;
        unordered_map<int,string> rNodeIds;

        virtual void loadData(  const char* imgFeatPath,
                                                        const char* categoryPath, vector<string> CTPath, int layer,
                                                        const char* voteFile, int userMin, int itemMin);

	virtual void cleanUp();

private:
        void loadImgAsins(const char* imgFeatPath, unordered_set<string>& imgAsins);
        void loadCategories(const char* categoryPath, vector<string>& CTPath, int layer);
	void loadVotes(const char* voteFile, int userMin, int itemMin);
	void loadImgFeatures(const char* imgFeatPath);
	void generateVotes(map<pair<int, int>, long long>& voteMap);
        void mapItemToCategory();
};
