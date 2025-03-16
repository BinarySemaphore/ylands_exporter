#ifndef EXPORTER_H
#define EXPORTER_H

#include "json.hpp"
using json = nlohmann::json;

// Forward declaration to avoid using headers and getting multiple redefines
class Config;
class Node;
class MeshObj;
class Workpool;

extern Workpool* wp;

int extractAndExport(Config& config);
void exportAsJson(const char* filename, const json& data, bool pprint);
void exportAsObj(const char* filename, Node& scene);

MeshObj* combineMeshFromScene(Node& scene);

#endif // EXPORTER_H