#ifndef REDUCER_H
#define REDUCER_H

template <typename T>
class TNode;
template <typename T>
class Octree;
class FaceData;

int joinSceneRelatedVerts(TNode<Octree<FaceData>>& octscene, float min_dist);

#endif // REDUCER_H