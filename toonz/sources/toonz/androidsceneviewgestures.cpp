//=============================================================================
//
//  Touch gestures for the viewer - implementation.
//
//  See androidsceneviewgestures.h.  The gesture recognition itself lives in
//  SceneViewer (it has been part of the viewer for years, for touch enabled
//  desktops); this file only makes sure it is enabled, and adds the long press
//  to context menu translation that touch devices need.
//
//=============================================================================

#include "androidsceneviewgestures.h"

#include "menubarcommandids.h"

#include "toonzqt/menubarcommand.h"

#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QWidget>

namespace AndroidGestures {

//-----------------------------------------------------------------------------

void applyTouchDefaults() {
  CommandManager *cm = CommandManager::instance();

  // Multi touch navigation (pinch to zoom, two finger rotate and pan) is what
  // the viewer does when "touch gesture control" is active.  On a touch device
  // it is the only way to navigate, so it is on by default rather than behind
  // a command the user has to discover.
  if (QAction *touchGesture = cm->getAction(MI_TouchGestureControl)) {
    touchGesture->setChecked(true);
    touchGesture->trigger();
    touchGesture->setChecked(true);
  }

}

//-----------------------------------------------------------------------------

int LongPressFilter::slop() { return 16; }

int LongPressFilter::holdTime() { return 550; }

//-----------------------------------------------------------------------------

LongPressFilter::LongPressFilter(QWidget *target)
    : QObject(target), m_target(target), m_pressed(false), m_triggered(false) {
  if (m_target) m_target->installEventFilter(this);
}

//-----------------------------------------------------------------------------

bool LongPressFilter::eventFilter(QObject *watched, QEvent *event) {
  if (watched != m_target) return QObject::eventFilter(watched, event);

  switch (event->type()) {
  case QEvent::MouseButtonPress: {
    QMouseEvent *me = static_cast<QMouseEvent *>(event);
    if (me->button() != Qt::LeftButton) break;
    m_pressPos  = me->pos();
    m_pressed   = true;
    m_triggered = false;
    m_timer.start();
    break;
  }

  case QEvent::MouseMove: {
    if (!m_pressed) break;
    QMouseEvent *me = static_cast<QMouseEvent *>(event);
    if ((me->pos() - m_pressPos).manhattanLength() > slop()) {
      // The user is drawing, not holding: the gesture is cancelled so that a
      // long stroke never turns into a context menu.
      m_pressed = false;
    }
    break;
  }

  case QEvent::MouseButtonRelease: {
    m_pressed = false;
    break;
  }

  case QEvent::Timer: {
    break;
  }

  case QEvent::Paint:
  case QEvent::UpdateRequest: {
    // The timer is polled from the paint cycle: a touch device already repaints
    // continuously while the pen is down, so this costs nothing and avoids a
    // second event source competing with the drawing tools.
    if (m_pressed && !m_triggered && m_timer.isValid() &&
        m_timer.elapsed() >= holdTime()) {
      m_triggered = true;
      m_pressed   = false;
      emit longPressed(m_pressPos);
      return true;
    }
    break;
  }

  default:
    break;
  }

  return QObject::eventFilter(watched, event);
}

}  // namespace AndroidGestures
