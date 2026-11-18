// Persistence.cpp
#include "Persistence.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip>

#include "BinaryIO.h"
#include "Exceptions.h"
#include "IoManip.h"

namespace ReliefOps::Persistence {

namespace {
const std::uint32_t kMagic = 0x484C5245;  // "ERLH" file signature
const std::uint32_t kVersion = 1;

void writeLog(std::ostream& os, const TransitLog& t) {
    BinaryIO::writePod(os, static_cast<std::int32_t>(t.seq));
    BinaryIO::writeString(os, t.vehicleId);
    BinaryIO::writeString(os, t.vehicleType);
    BinaryIO::writeString(os, t.corridor);
    BinaryIO::writePod(os, t.distanceKm);
    BinaryIO::writePod(os, t.payloadKg);
    BinaryIO::writePod(os, t.fuelUsedL);
}

TransitLog readLog(std::istream& is) {
    TransitLog t{};
    std::int32_t seq = 0;
    BinaryIO::readPod(is, seq);
    t.seq = seq;
    t.vehicleId = BinaryIO::readString(is);
    t.vehicleType = BinaryIO::readString(is);
    t.corridor = BinaryIO::readString(is);
    BinaryIO::readPod(is, t.distanceKm);
    BinaryIO::readPod(is, t.payloadKg);
    BinaryIO::readPod(is, t.fuelUsedL);
    return t;
}

std::string divider() {
    const int widths[] = {6, 10, 12, 11, 11, 13, 10};
    std::string line = "+";
    for (int w : widths) line += std::string(static_cast<std::size_t>(w), '-') + "+";
    return line + "\n";
}
}  // namespace

void saveAtomic(const std::string& path, const std::vector<TransitLog>& logs,
                const Inventory::Manifest& auditManifest) {
    const std::string tmpPath = path + ".tmp";
    {
        std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
        if (!out) throw PersistenceException("Cannot create " + tmpPath);

        BinaryIO::writePod(out, kMagic);
        BinaryIO::writePod(out, kVersion);

        BinaryIO::writePod(out, static_cast<std::uint32_t>(logs.size()));
        for (const TransitLog& t : logs) writeLog(out, t);

        BinaryIO::writePod(out, static_cast<std::uint32_t>(auditManifest.size()));
        for (int i = 0; i < auditManifest.size(); ++i) auditManifest.at(i).writeBinary(out);

        out.flush();
        if (!out) {
            out.close();
            std::remove(tmpPath.c_str());
            throw PersistenceException("Write failed, original file left untouched");
        }
    }  // file is closed here

    if (std::rename(tmpPath.c_str(), path.c_str()) != 0) {  // Windows cannot rename over an existing file
        std::remove(path.c_str());
        if (std::rename(tmpPath.c_str(), path.c_str()) != 0) {
            std::remove(tmpPath.c_str());
            throw PersistenceException("Could not replace " + path);
        }
    }
}

void load(const std::string& path, std::vector<TransitLog>& logs, Inventory::Manifest& auditManifest) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw PersistenceException("Cannot open " + path);

    std::uint32_t magic = 0, version = 0;
    BinaryIO::readPod(in, magic);
    BinaryIO::readPod(in, version);
    if (magic != kMagic || version != kVersion) throw PersistenceException(path + " is not a valid ERL-HUB file");

    std::uint32_t logCount = 0;
    BinaryIO::readPod(in, logCount);
    if (logCount > 1000000) throw PersistenceException("Corrupt file: too many log entries");
    std::vector<TransitLog> tmpLogs;
    for (std::uint32_t i = 0; i < logCount; ++i) tmpLogs.push_back(readLog(in));

    std::uint32_t cargoCount = 0;
    BinaryIO::readPod(in, cargoCount);
    if (cargoCount > 1000000) throw PersistenceException("Corrupt file: too many cargo records");
    Inventory::Manifest tmpManifest;
    for (std::uint32_t i = 0; i < cargoCount; ++i) {
        Inventory::ReliefCargo c;
        c.readBinary(in);
        tmpManifest += c;
    }

    logs = tmpLogs;                // commit only now: everything was read successfully
    auditManifest = tmpManifest;
}

void printLogs(std::ostream& os, const std::vector<TransitLog>& logs) {
    using Manip::Col;
    std::ios_base::fmtflags oldFlags = os.flags();
    std::streamsize oldPrecision = os.precision();

    const std::string line = divider();
    os << line;
    os << "| " << Col(4) << "No" << " | " << Col(8) << "Vehicle" << " | " << Col(10) << "Type" << " | " << Col(9)
       << "Corridor" << " | " << Col(9) << "Dist(km)" << " | " << Col(11) << "Payload(kg)" << " | " << Col(8)
       << "Fuel(L)" << " |\n";
    os << line << std::fixed << std::setprecision(1);
    for (const TransitLog& t : logs) {
        os << "| " << Col(4) << t.seq << " | " << Col(8) << t.vehicleId << " | " << Col(10) << t.vehicleType << " | "
           << Col(9) << t.corridor << " | " << Col(9) << t.distanceKm << " | " << Col(11) << t.payloadKg << " | "
           << Col(8) << t.fuelUsedL << " |\n";
    }
    os << line;
    if (logs.empty()) os << "  (no trips recorded yet)\n";

    os.flags(oldFlags);
    os.precision(oldPrecision);
}

}  // namespace ReliefOps::Persistence
