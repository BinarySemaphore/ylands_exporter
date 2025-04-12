#ifndef REDUCER_H
#define REDUCER_H

class Node;

int removeSceneInternalFaces(Node& scene, float min_dist);
int joinSceneRelatedVerts(Node& scene, float min_dist);

#endif // REDUCER_H