#pragma once
#include <Preferences.h>
#include "type_def.h"

// Save gpsdata_type struct to Preferences (flash)
bool saveGpsDataToFlash(Preferences& prefs, const gpsdata_type& gps);

// Load gpsdata_type struct from Preferences (flash)
bool loadGpsDataFromFlash(Preferences& prefs, gpsdata_type& gps);

// Optional: Erase saved GPS data
void eraseGpsDataFromFlash(Preferences& prefs);