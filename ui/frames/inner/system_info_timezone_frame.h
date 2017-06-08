// Copyright (c) 2016 Deepin Ltd. All rights reserved.
// Use of this source is governed by General Public License that can be found
// in the LICENSE file.

#ifndef INSTALLER_UI_FRAMES_INNER_SYSTEM_INFO_TIMEZONE_FRAME_H
#define INSTALLER_UI_FRAMES_INNER_SYSTEM_INFO_TIMEZONE_FRAME_H

#include <QFrame>

#include "sysinfo/timezone.h"

namespace installer {

class CommentLabel;
class NavButton;
class TimezoneManager;
class TimezoneMap;
class TitleLabel;

// Displays a world map to let user select timezone.
class SystemInfoTimezoneFrame : public QFrame {
  Q_OBJECT

 public:
  explicit SystemInfoTimezoneFrame(QWidget* parent = nullptr);

 signals:
  // Emitted when back button is clicked.
  void finished();

  // Emit this signal to request to hide timezone page and timezone button.
  void hideTimezone();

  // Emitted when a new timezone is chosen.
  void timezoneUpdated(const QString& timezone);

 public slots:
  // Read default timezone and emit timezoneUpdated() signal.
  void readConf();

  // Emitted when new language is selected.
  void timezoneUpdatedByLanguage(const QString& timezone);

  // Validate timezone and write to conf file.
  void writeConf();

 protected:
  void changeEvent(QEvent* event) override;

  // Remark current timezone when current frame is raised.
  void showEvent(QShowEvent* event) override;

 private:
  void initConnections();
  void initUI();

  // Convert timezone alias to its original name.
  QString parseTimezoneAlias(const QString& timezone);

  QString timezone_;
  TimezoneAliasMap alias_map_;

  TimezoneManager* timezone_manager_ = nullptr;

  TitleLabel* title_label_ = nullptr;
  CommentLabel* comment_label_ = nullptr;
  TimezoneMap* timezone_map_ = nullptr;
  NavButton* back_button_ = nullptr;

  // Priority of timezone: User > Conf > Scan
  enum class TimezoneSource {
    NotSet,  // Timezone not set.
    User,  // Timezone is setup by user.
    Conf,  // Timezone is read from conf file
    Scan,  // Timezone is updated based on geoip or regdomain
    Language,  // Timezone is setup based on current selected language.
  };
  TimezoneSource timezone_source_;
  bool is_local_time_;

 private slots:
  void onBackButtonClicked();

  // Update timezone after receiving signals from timezone manager.
  void onTimezoneManagerUpdated(const QString& timezone);

  // Update timezone after a new one has been chosen by user.
  void onTimezoneMapUpdated(const QString& timezone);
};

}  // namespace installer

#endif  // INSTALLER_UI_FRAMES_INNER_SYSTEM_INFO_TIMEZONE_FRAME_H
