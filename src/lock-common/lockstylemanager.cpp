/*
 * Copyright (C) 2026 CharOfString <root@charofstring.cc>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "lockstylemanager.h"

#include "sessionbasemodel.h"

namespace gxde {
namespace lock {

LockStyleManager::LockStyleManager(SessionBaseModel* model, LockStyle running,
                                   const LockStylePaths& paths,
                                   QObject* parent)
    : QObject(parent), model_(model), running_(running), paths_(paths) {
  if (model_) {
    visible_ = model_->isShow();
    connect(model_, &SessionBaseModel::visibleChanged, this,
            &LockStyleManager::onVisibleChanged);
  }
}

LockStyle LockStyleManager::RunningStyle() const {
  return running_;
}

LockStyle LockStyleManager::ConfiguredStyle() const {
  return LockStyleResolver::Resolve(paths_);
}

bool LockStyleManager::HandoffPending() const {
  return handoff_pending_;
}

void LockStyleManager::Refresh() {
  const LockStyle configured = ConfiguredStyle();
  if (configured == running_) {
    handoff_pending_ = false;
    return;
  }

  if (visible_) {
    handoff_pending_ = true;
    return;
  }

  handoff_pending_ = false;
  emit HandoffRequested(
      LockStyleResolver::ExecutablePath(configured, paths_.binDir));
}

void LockStyleManager::onVisibleChanged(bool visible) {
  visible_ = visible;
  if (visible_ || !handoff_pending_) return;

  // Re-resolve instead of replaying the old request: the style may have been
  // changed back while the screen was locked.
  Refresh();
}

}  // namespace lock
}  // namespace gxde
