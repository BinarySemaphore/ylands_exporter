#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include "config.hpp"
#include "json.hpp"
using json = nlohmann::json;

extern const char* DATA_TYPE_BLOCKDEF;
extern const char* DATA_TYPE_SCENE;

void loadFromFile(const char* filename, json& data);
void extractFromYlands(Config& config, json& data);

void reformatSceneFlatToNested(json& data);

bool isDataBlockDef(const json& data);
bool isDataValidScene(const json& data);

#endif // EXPORTER_H