#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct ZoneCount {
    std::string zone;
    long long count;
};

struct SlotCount {
    std::string zone;
    int hour;              // 0–23
    long long count;
};

class TripAnalyzer {
public:
    // Parse Trips.csv, skip dirty rows, never crash
    void ingestFile(const std::string& csvPath);

    // Top K zones: count desc, zone asc
    std::vector<ZoneCount> topZones(int k = 10) const;

    // Top K slots: count desc, zone asc, hour asc
    std::vector<SlotCount> topBusySlots(int k = 10) const;
private:
    std::vector<ZoneCount> zoneCounts;  // Stores counts of zones
    std::vector<SlotCount> slotCounts;  // Stores counts of slots (zone + hour)
    std::unordered_map<std::string, long long> zoneCountMap; // Maps zone to count
    std::unordered_map<std::string, std::unordered_map<int, long long> > slotCountMap; // Maps zone and hour to count
};

