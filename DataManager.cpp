#include "DataManager.h"
#include <stdexcept>
#include <sstream>
#include <map>        // for sorted output by key
#include <iostream>   // optional diagnostics

// --- helper: incorporate a record into an Extremes bucket ---
void DataManager::updateExtremes(Extremes& ex, const ZipCodeRecord& rec) {
    if (!ex.initialized) {
        ex.easternmost = rec;
        ex.westernmost = rec;
        ex.northernmost = rec;
        ex.southernmost = rec;
        ex.initialized = true;
        return;
    }
    if (rec.isEastOf(ex.easternmost))   ex.easternmost = rec;   // smaller longitude
    if (rec.isWestOf(ex.westernmost))   ex.westernmost = rec;   // larger longitude
    if (rec.isNorthOf(ex.northernmost)) ex.northernmost = rec;  // larger latitude
    if (rec.isSouthOf(ex.southernmost)) ex.southernmost = rec;  // smaller latitude
}

std::size_t DataManager::loadFromCsv(const std::string& csvPath) {
    records_.clear();

    CSVBuffer buf;                 // use default ctor, then openFile()
    if (!buf.openFile(csvPath)) {  // matches CSVBuffer::openFile(...)
        std::ostringstream oss;
        oss << "Failed to open CSV \"" << csvPath << "\"";
        if (buf.hasError()) oss << " - " << buf.getLastError();
        throw std::runtime_error(oss.str());
    }

    std::size_t loaded = 0;
    ZipCodeRecord rec;

    // Iterate using hasMoreRecords() + getNextRecord(...)
    while (buf.hasMoreRecords()) {
        if (buf.getNextRecord(rec)) {
            // Assume CSVBuffer + ZipCodeRecord setters already validated fields
            records_.push_back(rec);
            ++loaded;
        } else if (buf.hasError()) {
            // Skip bad line but keep going
            std::cerr << "[WARN] Parse error on line " << buf.getCurrentLineNumber()
                      << ": " << buf.getLastError() << "\n";
        }
    }

    buf.closeFile();

    if (loaded == 0) { // if no records could be loaded throw error
        throw std::runtime_error("No valid records loaded from: " + csvPath);
    }
    return loaded;
}

void DataManager::computeExtremes() {
    stateExtremes_.clear();
    for (const auto& rec : records_) {
        // getState() returns const char* 
        const char* st = rec.getState();
        if (!st || st[0] == '\0') continue;

        // enforce two-char state IDs
        if (st[0] == '\0' || st[1] == '\0' || st[2] != '\0') continue;

        Extremes& ex = stateExtremes_[std::string(st)];
        updateExtremes(ex, rec);
    }
}

void DataManager::printTable(std::ostream& os) const {
    // Header row
    os << "State, EasternmostZIP, WesternmostZIP, NorthernmostZIP, SouthernmostZIP\n";

    // Sort states alphabetically for output determinism
    std::map<std::string, Extremes> sorted(stateExtremes_.begin(), stateExtremes_.end());
    for (const auto& kv : sorted) {
        const std::string& state = kv.first;
        const Extremes& ex = kv.second;
        if (!ex.initialized) continue;

        os << state << ", "
           << ex.easternmost.getZipCode()  << ", "
           << ex.westernmost.getZipCode()  << ", "
           << ex.northernmost.getZipCode() << ", "
           << ex.southernmost.getZipCode() << "\n";
    }
}

std::string DataManager::signature() const {
    std::ostringstream oss;
    std::map<std::string, Extremes> sorted(stateExtremes_.begin(), stateExtremes_.end()); 
    for (const auto& kv : sorted) {
        const std::string& state = kv.first;
        const Extremes& ex = kv.second;
        if (!ex.initialized) continue;
        oss << state << ":"
            << ex.easternmost.getZipCode()  << "|"
            << ex.westernmost.getZipCode()  << "|"
            << ex.northernmost.getZipCode() << "|"
            << ex.southernmost.getZipCode() << "\n";
    }
    return oss.str();
}

std::string DataManager::signatureFromCsv(const std::string& csvPath) {
    DataManager mgr;
    mgr.loadFromCsv(csvPath);
    mgr.computeExtremes();
    return mgr.signature();
}

bool DataManager::verifyIdenticalResults(const std::string& csvA,
                                         const std::string& csvB) {
    return signatureFromCsv(csvA) == signatureFromCsv(csvB);
}
