#ifndef EXPORTER_H
#define EXPORTER_H

#include "config.hpp"
#include "json.hpp"
using json = nlohmann::json;

int extractAndExport(Config& config);
void exportAsJson(const char* filename, const json& data, bool pprint);
// More export options here...

#endif // EXPORTER_H