#pragma once

#ifndef ANDROID_COMMAND_BAR_H
#define ANDROID_COMMAND_BAR_H

//=============================================================================
//
//  Touch oriented replacement of the desktop menu bar.
//
//  OpenToonz organises its commands in rooms, each with its own menu bar (see
//  menubar.cpp).  A phone or tablet has neither the width nor the pointer to
//  use a menu bar: the commands are therefore presented as a searchable,
//  room aware list, opened from a single button, which is the navigation
//  pattern the platform expects.
//
//  Like the touch panel, this is a view over the command system: the list is
//  built from the very same QActions the desktop menu bar holds, so the
//  command set - and every user defined shortcut - is identical.
//
//=============================================================================

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QVector>

class QLineEdit;
class QListWidget;
class QToolButton;
class QAction;
class QMenuBar;

class AndroidCommandBar final : public QWidget {
  Q_OBJECT

public:
  explicit AndroidCommandBar(QWidget *parent = nullptr);
  ~AndroidCommandBar() override;

  //! Rebuilds the command list from the menu bar of the current room.  Called
  //! whenever the room changes.
  void setSourceMenuBar(QMenuBar *menuBar);

  //! Opens the command sheet.
  void showCommands();

  //! Opens the room switcher.
  void showRooms();

  //! Rooms offered by the switcher.  Filled by the main window, which owns
  //! the room list.
  void setRooms(const QStringList &names, int currentIndex);

signals:
  //! Emitted when the user picks a different room.
  void roomRequested(int index);

protected:
  void resizeEvent(QResizeEvent *) override;

private slots:
  void onFilterChanged(const QString &text);
  void onItemActivated(int row);
  void onHomeRequested();
  void onBackRequested();

private:
  void collectActions(QMenuBar *menuBar);
  void applyFilter();
  static QString normalized(const QString &text);

  QToolButton *m_commandsButton;
  QToolButton *m_roomsButton;
  QToolButton *m_homeButton;
  QToolButton *m_backButton;

  QWidget *m_sheet;
  QLineEdit *m_search;
  QListWidget *m_list;

  struct Entry {
    QString label;   //!< "Menu > Item", for the filter and the tooltip
    QAction *action; //!< Not owned
  };
  QVector<Entry> m_entries;

  QStringList m_roomNames;
  int m_currentRoom = 0;
};

#endif  // ANDROID_COMMAND_BAR_H
