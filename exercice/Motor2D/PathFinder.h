#ifndef __PATHFINDER_H__
#define __PATHFINDER_H__
#include "p2Point.h"
#include "p2List.h"
#include "p2DynArray.h"

#pragma region Structs


struct PathList;
struct PathNode
{
	// Convenient constructors
	PathNode();
	PathNode(int g, int h, const iPoint& pos, const PathNode* parent);
	PathNode(const PathNode& node);

	// Fills a list (PathList) of all valid adjacent pathnodes
	uint FindWalkableAdjacents(PathList& list_to_fill) const;
	// Calculates this tile score
	int Score() const;
	// Calculate the F for a specific destination tile
	int CalculateF(const iPoint& destination);

	// -----------
	int g;
	int h;
	iPoint pos;
	const PathNode* parent; // needed to reconstruct the path in the end
};

// ---------------------------------------------------------------------
// Helper struct to include a list of path nodes
// ---------------------------------------------------------------------
struct PathList
{

	// Looks for a node in this list and returns it's list node or NULL
	p2List_item<PathNode>* Find(const iPoint& point) const;

	// Returns the Pathnode with lowest score in this list or NULL if empty
	p2List_item<PathNode>* GetNodeLowestScore() const;

	// -----------
	// The list itself, note they are not pointers!
	p2List<PathNode> list;

};
#pragma endregion

// TODO 1: Just take a few minutes to undertand this new class and think from where it comes

class PathFinder
{
public:
	PathFinder();
	~PathFinder();

	void PreparePath(const iPoint& origin, const iPoint& destination);

	bool IteratePath();

	// To request all tiles involved in the last generated path
	const p2DynArray<iPoint>* GetLastPath() const;
	
	p2DynArray<iPoint> last_path;

	bool Update();

	bool pathCompleted;
	bool available;

private:

	PathList open;
	PathList close;

	iPoint origin;
	iPoint destination;

	// we store the created path here
};

#endif // !__PATHFINDER_H__

