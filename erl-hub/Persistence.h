// Persistence.h  --  UNIT IV: binary file streams, atomic save, all-or-nothing restore
#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <ostream>
#include <string>
#include <vector>

#include "Inventory.h"

namespace ReliefOps::Persistence {

// One row of the transit log (a vehicle trip)
struct TransitLog {
    int seq;
    std::string vehicleId;
    std::string vehicleType;
    std::string corridor;
    double distanceKm;
    double payloadKg;
    double fuelUsedL;
};

// Writes to "<path>.tmp" first and renames it over <path> only when the whole write succeeded,
// so a crash or a full disk can never leave a half-written file behind.
void saveAtomic(const std::string& path, const std::vector<TransitLog>& logs,
                const Inventory::Manifest& auditManifest);

// Reads everything into temporaries first; the outputs change only if the WHOLE file was valid.
void load(const std::string& path, std::vector<TransitLog>& logs, Inventory::Manifest& auditManifest);

// Tabular printing with the user-defined manipulators
void printLogs(std::ostream& os, const std::vector<TransitLog>& logs);

}  // namespace ReliefOps::Persistence

#endif
