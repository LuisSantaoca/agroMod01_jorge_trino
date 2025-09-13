#include "data_storage.h"

#define GPS_NAMESPACE "gpsdata"
#define GPS_KEY "lastfix"

bool saveGpsDataToFlash(Preferences& prefs, const gpsdata_type& gps) {
    prefs.begin(GPS_NAMESPACE, false);
    size_t written = prefs.putBytes(GPS_KEY, &gps, sizeof(gpsdata_type));
    prefs.end();
    return (written == sizeof(gpsdata_type));
}

bool loadGpsDataFromFlash(Preferences& prefs, gpsdata_type& gps) {
    prefs.begin(GPS_NAMESPACE, true);
    size_t read = prefs.getBytes(GPS_KEY, &gps, sizeof(gpsdata_type));
    prefs.end();
    // Check if we read a full struct
    return (read == sizeof(gpsdata_type) && gps.year > 2000 && gps.month > 0 && gps.day > 0);
}

void eraseGpsDataFromFlash(Preferences& prefs) {
    prefs.begin(GPS_NAMESPACE, false);
    prefs.remove(GPS_KEY);
    prefs.end();
}