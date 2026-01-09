#include "analyzer.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <fstream>
#include <sstream>  
#include <iostream>
using namespace std;
// Students may use ANY data structure internally


bool isNumeric(const string& str) {
    if (str.empty()) return false;
    for (char c : str) {
        if (!isdigit(c)) return false; 
    }
    return true;
}

void TripAnalyzer::ingestFile(const string& csvPath) {
    using namespace std; // Talimatınız üzerine eklendi

    ifstream dataFile(csvPath);
    if (!dataFile.is_open()) {
        cerr << "ERROR: File not found!" << endl;
        return;
    }

    string row;
    
    // Eğer CSV'nin ilk satırı başlık (header) ise onu atlamak için:
    getline(dataFile, row); 

    while (getline(dataFile, row)) {
        if (row.empty()) continue;

        int slicedHour = -1;

        // --- İlk koddaki hızlı parçalama mantığı aynen korunmuştur ---
        size_t p0 = 0;
        size_t p1 = row.find(',', p0);
        if (p1 == string::npos || p1 == p0) continue; // tripID boş

        size_t p2 = row.find(',', p1 + 1);
        if (p2 == string::npos || p2 == p1 + 1) continue; // pickupZoneID boş
        string pickupZoneID = row.substr(p1 + 1, p2 - p1 - 1);

        size_t p3 = row.find(',', p2 + 1);
        if (p3 == string::npos || p3 == p2 + 1) continue; // dropoffZoneID boş

        size_t p4 = row.find(',', p3 + 1);
        if (p4 == string::npos) continue; // zaman alanı eksik

        size_t dtLen = p4 - (p3 + 1);
        if (dtLen < 16) continue;

        const char* dt = row.c_str() + p3 + 1;

        // Tarih ve saat formatı kontrolü (Hızlı karakter erişimi)
        if (dt[10] != ' ' || dt[13] != ':') continue;

        if (dt[11] < '0' || dt[11] > '9' ||
            dt[12] < '0' || dt[12] > '9' ||
            dt[14] < '0' || dt[14] > '9' ||
            dt[15] < '0' || dt[15] > '9') continue;

        int hour = (dt[11] - '0') * 10 + (dt[12] - '0');
        int minute = (dt[14] - '0') * 10 + (dt[15] - '0');

        if (hour < 0 || hour > 23 || minute < 0 || minute > 59) continue;

        slicedHour = hour;

        // Sonraki virgül kontrolleri
        size_t p5 = row.find(',', p4 + 1);
        if (p5 == string::npos) continue; 
        
        // Verileri map'lere kaydet
        zoneCountMap[pickupZoneID]++;
        slotCountMap[pickupZoneID][slicedHour]++;
    }

    dataFile.close();
}

struct compareCondition {
    bool operator()(const ZoneCount& a, const ZoneCount& b) {
        // If counts are different, the one with HIGHER count is "smaller" (kept at bottom)
        // The one with LOWER count bubbles to the top (to be popped)
        if (a.count != b.count) {
            return a.count > b.count; // Heap b wants to go on top if its count is LOWER
        }
        // If counts are equal, the one with "smaller" ID (A) is kept at bottom.
        // The one with "larger" ID (Z) bubbles to the top (to be popped).
        return a.zone < b.zone;
    }
};

struct compareConditionSlot {
    bool operator()(const SlotCount& a, const SlotCount& b) {
        if (a.count != b.count) {
            return a.count > b.count; 
        } else if (a.zone != b.zone) {
            return a.zone < b.zone;
        }
        return a.hour < b.hour;
    }
};

std::vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    // TODO:
    // - sort by count desc, zone asc
    // - return first k
    priority_queue<ZoneCount, vector<ZoneCount>, compareCondition> pq;
    for (const auto& pair : zoneCountMap) {
        if(pq.size() < k){pq.push(ZoneCount{pair.first, pair.second});}
        else if(pair.second > pq.top().count || (pair.second == pq.top().count && pair.first < pq.top().zone)){
            pq.pop();
            pq.push(ZoneCount{pair.first, pair.second});
        }
    }

    std::vector<ZoneCount> result={};
    while (!pq.empty()) {
        result.push_back(pq.top());
        pq.pop();
    }

    // The Heap does NOT guarantee the final vector is sorted 
    std::sort(result.begin(), result.end(), [](const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count; // Count Descending
        return a.zone < b.zone; // Zone Ascending (A before Z)
    });

    return result;
}

std::vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    // TODO:
    // - sort by count desc, zone asc, hour asc
    // - return first k
    
    priority_queue<SlotCount, vector<SlotCount>, compareConditionSlot> pq;
    for (const auto& pair : slotCountMap) {
        const auto& innerMap = pair.second; // This is the map<int, long long>
        // Inner Loop: Go through every Hour in that Zone
        for (const auto& hourPair : innerMap) {
            SlotCount sc = {pair.first, hourPair.first, hourPair.second};
            if(pq.size() < k){pq.push(sc);}
            else{
            const SlotCount& top = pq.top();
            if (
                hourPair.second > top.count ||
                (hourPair.second == top.count &&
                 (pair.first < top.zone ||
                  (pair.first == top.zone && hourPair.first < top.hour)))
            ) {
                pq.pop();
                pq.push(SlotCount{pair.first, hourPair.first, hourPair.second});
            }}
        }
    }

    std::vector<SlotCount> result={};
    while (!pq.empty()) {
        result.push_back(pq.top());
        pq.pop();
    }

    // The Heap does NOT guarantee the final vector is sorted 
    std::sort(result.begin(), result.end(), [](const SlotCount& a, const SlotCount& b) {
        if (a.count != b.count) return a.count > b.count; // Count Descending
        if (a.zone != b.zone) return a.zone < b.zone; // Zone Ascending (A before Z)
        return a.hour < b.hour; // Hour Ascending (0 before 23)
    });

    return result;
}



