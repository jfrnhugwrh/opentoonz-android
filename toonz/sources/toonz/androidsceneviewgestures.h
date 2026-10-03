#pragma once

#ifndef ANDROID_SCENEVIEW_GESTURES_H
#define ANDROID_SCENEVIEW_GESTURES_H

//=============================================================================
//
//  Touch gestures for the viewer, expressed in terms of existing behaviour.
//
//  The desktop viewer is driven by the mouse wheel, by modifier keys and by
//  keyboard shortcuts (see sceneviewerevents.cpp).  A touch device has none of
//  those.  What this file adds is deliberately thin:
//
//    * the multi touch gestures the viewer already implements are switched on
//      by default, instead of being behind the "touch gesture control" command
//      the desktop users have to find;
//    * a long press is turned into the context menu the right mouse button
//      opens on the desktop;
//    * the choices that have no meaning on a touch device (hover cursors,
//      tablet pressure) are turned off.
//
//  Nothing about how the tools draw, how the renderer works or what a command
//  does is changed: the gestures reach the same code paths the desktop input
//  reaches.
//
//=============================================================================

#include <QObject>
#include <QPoint>
#include <QElapsedTimer>

class QWidget;

namespace AndroidGestures {

//! Applies the touch oriented defaults to the viewer and the preferences.
//! Called once, after the environment has been initialised.
void applyTouchDefaults();

//! Turns a long press into the context menu, which is what the right mouse
//! button does on the desktop.  Installed on the viewer widgets.
class LongPressFilter final : public QObject {
  Q_OBJECT

public:
  explicit LongPressFilter(QWidget *target);

  //! Travel, in device independent pixels, that cancels a long press.
  static int slop();

  //! Hold time, in milliseconds, that turns a press into a long press.
  static int holdTime();

signals:
  //! Emitted with the position, in the target's coordinates.
  void longPressed(const QPoint &pos);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  QWidget *m_target;
  QPoint m_pressPos;
  QElapsedTimer m_timer;
  bool m_pressed;
  bool m_triggered;
};

}  // namespace AndroidGestures

#endif  // ANDROID_SCENEVIEW_GESTURES_H
