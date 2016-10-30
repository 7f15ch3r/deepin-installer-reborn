// Copyright (c) 2016 Deepin Ltd. All rights reserved.
// Use of this source is governed by General Public License that can be found
// in the LICENSE file.

#include "language_list_model.h"

#include <QDebug>

namespace installer {

LanguageListModel::LanguageListModel(QObject* parent)
    : QAbstractListModel(parent),
      lang_list_() {
  this->setObjectName(QStringLiteral("language_list_model"));

  // TODO(xushaohua): Move to init() function.
  lang_list_ = GetLanguageList();
}

QVariant LanguageListModel::data(const QModelIndex& index, int role) const {
  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  if (!index.isValid()) {
    return QVariant();
  }

  return lang_list_.at(index.row()).local_name;
}

int LanguageListModel::rowCount(const QModelIndex& parent) const {
  Q_UNUSED(parent);
  return lang_list_.length();
}

QString LanguageListModel::locale(const QModelIndex& index) const {
  if (index.isValid()) {
    return lang_list_.at(index.row()).locale;
  }
  return QString();
}

}  // namespace installer