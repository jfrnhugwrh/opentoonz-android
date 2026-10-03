#pragma once

#ifndef ANDROID_TOUCH_PANEL_H
#define ANDROID_TOUCH_PANEL_H

//=============================================================================
//
//  Touch oriented controls for the Android build.
//
//  On the desktop the most frequent drawing commands are driven by keyboard
//  shortcuts and by a menu bar.  Neither is available on a touch device, so
//  this panel reproduces the same commands as large, thumb reachable buttons.
//
//  The panel is a *view* over the existing command system: every button
//  triggers the same CommandManager action the desktop shortcut triggers, so
//  the behaviour of the application is unchanged - only the way it is invoked
//  differs.  Nothing is reimplemented, and any command the user binds to a
//  shortcut on the desktop keeps working here through the same action.
//
//=============================================================================

#include <QWidget>
#include <QHash>

class QAction;
class QToolButton;
class QGridLayout;

class AndroidTouchPanel final : public QWidget {
  Q_OBJECT

public:
  //! Which group of commands the panel shows.
  enum Group {
    Drawing,   //!< undo/redo, onion skin, tool switching
    Timeline,  //!< frame navigation, playback, cell operations
    View,      //!< zoom, fit, rotation, show/hide toggles
    Levels     //!< level creation, save, export
  };

  explicit AndroidTouchPanel(QWidget *parent = nullptr);
  ~AndroidTouchPanel() override;

  //! Shows the commands of \p group.
  void setGroup(Group group);
  Group group() const { return m_group; }

  //! Adds a button bound to the CommandManager action \p commandId.
  void addCommand(const char *commandId, const QString &text = QString(),
                  int rowSpan = 1, int columnSpan = 1);

  //! Re-reads the enabled/checked state of the bound actions.
  void refresh();

  //! Clears the panel.
  void clear();

protected:
  void paintEvent(QPaintEvent *) override;

private slots:
  void onActionChanged();

private:
  struct Button {
    QToolButton *widget;
    QAction *action;
  };

  QWidget *buildDrawingGroup();
  QWidget *buildTimelineGroup();
  QWidget *buildViewGroup();
  QWidget *buildLevelsGroup();

  Group m_group;
  QGridLayout *m_layout;
  QList<Button> m_buttons;
  QHash<const char *, QAction *> m_actions;
};

//=============================================================================
//  Layout helpers
//=============================================================================

namespace AndroidUi {

//! Minimum edge length of a control that can be hit reliably with a finger,
//! in device independent pixels.  Matches the platform accessibility guidance.
constexpr int TouchTargetSize = 48;

//! True when the device has enough room for the two column desktop layouts.
bool isLargeScreen();

//! Applies the touch oriented metrics (font size, spacing, scrollbars) to the
//! whole application.  Called once, from the main window constructor.
void applyTouchTheme();

//! Height of the system bars that overlap the window in full screen mode.
int systemBarInset();

}  // namespace AndroidUi

#endif  // ANDROID_TOUCH_PANEL_H
