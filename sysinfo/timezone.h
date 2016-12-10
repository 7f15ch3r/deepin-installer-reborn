// Copyright (c) 2016 Deepin Ltd. All rights reserved.
// Use of this source is governed by General Public License that can be found
// in the LICENSE file.

#ifndef INSTALLER_SYSINFO_TIMEZONE_H
#define INSTALLER_SYSINFO_TIMEZONE_H

#include <QList>
#include <QHash>

namespace installer {

struct ZoneInfo {
 public:
  QString country;
  QString timezone;

  // Coordinates of zone.
  double latitude;
  double longitude;

  // Distance to clicked point for comparison.
  double distance;
};
bool ZoneInfoDistanceComp(const ZoneInfo& a, const ZoneInfo& b);
QDebug& operator<<(QDebug& debug, const ZoneInfo& info);
typedef QList<ZoneInfo> ZoneInfoList;

// Read available timezone info in zone.tab file.
ZoneInfoList GetZoneInfoList();

// Find ZoneInfo based on |country| or |timezone|.
// Returns -1 if not found.
int GetZoneInfoByCountry(const ZoneInfoList& list, const QString& country);
int GetZoneInfoByZone(const ZoneInfoList& list, const QString& timezone);

// Read current timezone in /etc/timezone file.
QString GetCurrentTimezone();

// Get prefer timezone based on locale.
QString GetPreferTimezone();

// Returns name of timezone, excluding continent name.
// TODO(xushaohua): Call time and localtime()
QString GetTimezoneName(const QString& timezone);

// A map between current name of timezone and their old names.
typedef QHash<QString, QString> BackwardTzMap;
BackwardTzMap GetBackwardTzMap();

// Validate |timezone|.
// TODO(xushaohua): Optimize
bool IsValidTimezone(const QString& timezone);

}  // namespace installer

#endif  // INSTALLER_SYSINFO_TIMEZONE_H
