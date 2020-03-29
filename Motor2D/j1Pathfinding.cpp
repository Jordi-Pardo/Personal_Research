#include "p2Defs.h"
#include "p2Log.h"
#include "j1App.h"
#include "j1PathFinding.h"
#include "j1Timer.h"

j1PathFinding::j1PathFinding() : j1Module(), map(NULL), last_path(DEFAULT_PATH_LENGTH),width(0), height(0)
{
	pathFinished = false;
	pathRequested = false;

	open = new PathList;
	close = new PathList;

	name.create("pathfinding");
}

// Destructor
j1PathFinding::~j1PathFinding()
{
	RELEASE_ARRAY(map);
}

// Called before quitting
bool j1PathFinding::CleanUp()
{
	LOG("Freeing pathfinding library");

	last_path.Clear();
	RELEASE_ARRAY(map);
	return true;
}

// Sets up the walkability map
void j1PathFinding::SetMap(uint width, uint height, uchar* data)
{
	this->width = width;
	this->height = height;

	RELEASE_ARRAY(map);
	map = new uchar[width*height];
	memcpy(map, data, width*height);
}

// Utility: return true if pos is inside the map boundaries
bool j1PathFinding::CheckBoundaries(const iPoint& pos) const
{
	return (pos.x >= 0 && pos.x <= (int)width &&
			pos.y >= 0 && pos.y <= (int)height);
}

// Utility: returns true is the tile is walkable
bool j1PathFinding::IsWalkable(const iPoint& pos) const
{
	uchar t = GetTileAt(pos);
	return t != INVALID_WALK_CODE && t > 0;
}

// Utility: return the walkability value of a tile
uchar j1PathFinding::GetTileAt(const iPoint& pos) const
{
	if(CheckBoundaries(pos))
		return map[(pos.y*width) + pos.x];

	return INVALID_WALK_CODE;
}

void j1PathFinding::RequestPath(const iPoint& origin, const iPoint& destination)
{
	this->origin = new iPoint(origin);
	this->destination = new iPoint(destination);
	pathRequested = true;

}



bool j1PathFinding::Update(float dt)
{
	if (pathRequested) {
		j1Timer timer;
		timer.Start();
		LOG("Path Requested");
		CreatePath(*origin, *destination);
		
		if (pathFinished) {
			pathRequested = false;
			RELEASE(origin);
			RELEASE(destination);
			open->list.clear();
			close->list.clear();
			pathFinished = false;
		}

		LOG("PathFinding time: %f", timer.ReadSec());
	}

	return true;
}

// To request all tiles involved in the last generated path
const p2DynArray<iPoint>* j1PathFinding::GetLastPath() const
{
	return &last_path;
}

// PathList ------------------------------------------------------------------------
// Looks for a node in this list and returns it's list node or NULL
// ---------------------------------------------------------------------------------
p2List_item<PathNode>* PathList::Find(const iPoint& point) const
{
	p2List_item<PathNode>* item = list.start;
	while(item)
	{
		if(item->data.pos == point)
			return item;
		item = item->next;
	}
	return NULL;
}

// PathList ------------------------------------------------------------------------
// Returns the Pathnode with lowest score in this list or NULL if empty
// ---------------------------------------------------------------------------------
p2List_item<PathNode>* PathList::GetNodeLowestScore() const
{
	p2List_item<PathNode>* ret = NULL;
	int min = 65535;

	p2List_item<PathNode>* item = list.end;
	while(item)
	{
		if(item->data.Score() < min)
		{
			min = item->data.Score();
			ret = item;
		}
		item = item->prev;
	}
	return ret;
}

// PathNode -------------------------------------------------------------------------
// Convenient constructors
// ----------------------------------------------------------------------------------
PathNode::PathNode() : g(-1), h(-1), pos(-1, -1), parent(NULL)
{}

PathNode::PathNode(int g, int h, const iPoint& pos, const PathNode* parent) : g(g), h(h), pos(pos), parent(parent)
{}

PathNode::PathNode(const PathNode& node) : g(node.g), h(node.h), pos(node.pos), parent(node.parent)
{}

// PathNode -------------------------------------------------------------------------
// Fills a list (PathList) of all valid adjacent pathnodes
// ----------------------------------------------------------------------------------
uint PathNode::FindWalkableAdjacents(PathList& list_to_fill) const
{
	iPoint cell;
	uint before = list_to_fill.list.count();

	// north
	cell.create(pos.x, pos.y + 1);
	if(App->pathfinding->IsWalkable(cell))
		list_to_fill.list.add(PathNode(-1, -1, cell, this));

	// south
	cell.create(pos.x, pos.y - 1);
	if(App->pathfinding->IsWalkable(cell))
		list_to_fill.list.add(PathNode(-1, -1, cell, this));

	// east
	cell.create(pos.x + 1, pos.y);
	if(App->pathfinding->IsWalkable(cell))
		list_to_fill.list.add(PathNode(-1, -1, cell, this));

	// west
	cell.create(pos.x - 1, pos.y);
	if(App->pathfinding->IsWalkable(cell))
		list_to_fill.list.add(PathNode(-1, -1, cell, this));

	return list_to_fill.list.count();
}

// PathNode -------------------------------------------------------------------------
// Calculates this tile score
// ----------------------------------------------------------------------------------
int PathNode::Score() const
{
	return g + h;
}

// PathNode -------------------------------------------------------------------------
// Calculate the F for a specific destination tile
// ----------------------------------------------------------------------------------
int PathNode::CalculateF(const iPoint& destination)
{
	g = parent->g + 1;
	h = pos.DistanceManhattan(destination);

	return g + h;
}

// ----------------------------------------------------------------------------------
// Actual A* algorithm: return number of steps in the creation of the path or -1 ----
// ----------------------------------------------------------------------------------
int j1PathFinding::CreatePath(const iPoint& origin, const iPoint& destination)
{
	j1Timer timer;
	timer.Start();
	
	// TODO 1: if origin or destination are not walkable, return -1
	if (!IsWalkable(origin) || !IsWalkable(destination)) 
	{
		LOG("Origin or destination are not walkable");
		return -1;
	}

	// TODO 2: Create two lists: open, close
	//PathList open;
	//PathList close;



	PathNode* node;
	// Add the origin tile to open
	if (open->GetNodeLowestScore() == NULL)
		open->list.add(PathNode(0, origin.DistanceTo(destination), origin, nullptr));

	uint iterations = 0;

	// Iterate while we have tile in the open list
	while (open->GetNodeLowestScore() != NULL && iterations < 50)
	{
		// TODO 3: Move the lowest score cell from open list to the closed list

		node = new PathNode(open->GetNodeLowestScore()->data);
		close->list.add(*node);
		open->list.del(open->Find(node->pos));
		
		// TODO 4: If we just added the destination, we are done!
		if (node->pos == destination) {
			const PathNode* iterator = node;

			// Backtrack to create the final path
			for ( iterator; iterator->pos != origin; iterator = iterator->parent)
			{
				last_path.PushBack(iterator->pos);
			}

			last_path.PushBack(origin);
			
			// Use the Pathnode::parent and Flip() the path when you are finish
			last_path.Flip();
			pathFinished = true;
			LOG("%f", timer.ReadSec());
			return 1;
		}

		// TODO 5: Fill a list of all adjancent nodes
		PathList adjacentNodes;
		uint numNodes = node->FindWalkableAdjacents(adjacentNodes);

		// TODO 6: Iterate adjancent nodes:
		for (uint i = 0; i < numNodes; i++)
		{
			// ignore nodes in the closed list
			if (close->Find(adjacentNodes.list[i].pos) == NULL) {
				// If it is NOT found, calculate its F and add it to the open list
				if(open->Find(adjacentNodes.list[i].pos) == NULL) {
					adjacentNodes.list[i].CalculateF(destination);
					open->list.add(adjacentNodes.list[i]);
				}
				// If it is already in the open list, check if it is a better path (compare G)
				else {
					if (adjacentNodes.list[i].g < open->Find(adjacentNodes.list[i].pos)->data.g) {
						// If it is a better path, Update the parent
						adjacentNodes.list[i].CalculateF(destination);
						open->list.del(open->Find(adjacentNodes.list[i].pos));
						open->list.add(adjacentNodes.list[i]);
					}
				}
			}
		}
		iterations++;
		LOG("Loops: %u", iterations);
	}






}

