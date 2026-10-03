//=============================================================================
//
//  Touch oriented replacement of the desktop menu bar.
//
//  The command list is built from the menu bar of the current room, so the
//  available commands and their enabled/checked state are exactly the ones the
//  desktop shows - this file only changes how they are presented.
//
//=============================================================================

#include "androidcommandbar.h"
#include "androidtouchpanel.h"

#include "toonzqt/menubarcommand.h"

#include <QAction>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QToolButton>
#include <QVBoxLayout>

//=============================================================================

AndroidCommandBar::AndroidCommandBar(QWidget *parent)
    : QWidget(parent)
    , m_commandsButton(nullptr)
    , m_roomsButton(nullptr)
    , m_homeButton(nullptr)
    , m_backButton(nullptr)
    , m_sheet(nullptr)
    , m_search(nullptr)
    , m_list(nullptr) {
  setObjectName("AndroidCommandBar");

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(4, 2, 4, 2);
  layout->setSpacing(4);

  auto makeButton = [this](const QString &text, const QString &objectName) {
    QToolButton *button = new QToolButton(this);
    button->setText(text);
    button->setObjectName(objectName);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setMinimumHeight(AndroidUi::TouchTargetSize);
    button->setFocusPolicy(Qt::NoFocus);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return button;
  };

  m_homeButton     = makeButton(tr("Home"), "AndroidCommandBarHome");
  m_backButton     = makeButton(tr("Back"), "AndroidCommandBarBack");
  m_roomsButton    = makeButton(tr("Rooms"), "AndroidCommandBarRooms");
  m_commandsButton = makeButton(tr("Commands"), "AndroidCommandBarCommands");

  layout->addWidget(m_homeButton, 1);
  layout->addWidget(m_backButton, 1);
  layout->addWidget(m_roomsButton, 1);
  layout->addWidget(m_commandsButton, 2);

  connect(m_commandsButton, &QToolButton::clicked, this,
          &AndroidCommandBar::showCommands);
  connect(m_roomsButton, &QToolButton::clicked, this,
          &AndroidCommandBar::showRooms);

  // The command sheet: a search field over the room's commands.
  m_sheet = new QFrame(this);
  m_sheet->setObjectName("AndroidCommandSheet");
  m_sheet->hide();

  QVBoxLayout *sheetLayout = new QVBoxLayout(m_sheet);
  sheetLayout->setContentsMargins(6, 6, 6, 6);
  sheetLayout->setSpacing(6);

  m_search = new QLineEdit(m_sheet);
  m_search->setPlaceholderText(tr("Search commands"));
  m_search->setMinimumHeight(AndroidUi::TouchTargetSize);
  m_search->setClearButtonEnabled(true);
  sheetLayout->addWidget(m_search);

  m_list = new QListWidget(m_sheet);
  m_list->setObjectName("AndroidCommandList");
  // Touch scrolling needs generous row heights: the default metric is sized
  // for a mouse wheel.
  m_list->setSpacing(2);
  m_list->setUniformItemSizes(false);
  sheetLayout->addWidget(m_list, 1);

  connect(m_search, &QLineEdit::textChanged, this,
          &AndroidCommandBar::onFilterChanged);
  connect(m_list, &QListWidget::itemActivated, this,
          [this](QListWidgetItem *item) {
            onItemActivated(m_list->row(item));
          });
  connect(m_list, &QListWidget::itemClicked, this,
          [this](QListWidgetItem *item) {
            onItemActivated(m_list->row(item));
          });
}

//-----------------------------------------------------------------------------

AndroidCommandBar::~AndroidCommandBar() {}

//-----------------------------------------------------------------------------

QString AndroidCommandBar::normalized(const QString &text) {
  QString out = text;
  // Ampersands mark the mnemonic in a menu label; they are noise in a search.
  out.remove('&');
  return out.simplified();
}

//-----------------------------------------------------------------------------

void AndroidCommandBar::collectActions(QMenuBar *menuBar) {
  m_entries.clear();
  if (!menuBar) return;

  const QList<QAction *> menus = menuBar->actions();
  for (QAction *menuAction : menus) {
    QMenu *menu = menuAction->menu();
    if (!menu) continue;

    const QString menuTitle = normalized(menu->title());
    const QList<QAction *> items = menu->actions();
    for (QAction *item : items) {
      if (item->isSeparator()) continue;

      // Sub menus are flattened with their parent's name, which is what makes
      // the search usable: "File > Save As" can be reached by typing "as".
      if (QMenu *sub = item->menu()) {
        const QString subTitle = normalized(sub->title());
        const QList<QAction *> subItems = sub->actions();
        for (QAction *subItem : subItems) {
          if (subItem->isSeparator()) continue;
          Entry entry;
          entry.label = menuTitle + " > " + subTitle + " > " +
                        normalized(subItem->text());
          entry.action = subItem;
          m_entries.append(entry);
        }
        continue;
      }

      Entry entry;
      entry.label  = menuTitle + " > " + normalized(item->text());
      entry.action = item;
      m_entries.append(entry);
    }
  }
}

//-----------------------------------------------------------------------------

void AndroidCommandBar::setSourceMenuBar(QMenuBar *menuBar) {
  collectActions(menuBar);
  applyFilter();
}

//-----------------------------------------------------------------------------

void AndroidCommandBar::applyFilter() {
  if (!m_list) return;

  const QString filter = m_search ? m_search->text().trimmed().toLower()
                                  : QString();

  m_list->clear();
  for (const Entry &entry : m_entries) {
    if (!entry.action || !entry.action->isEnabled()) continue;
    if (!filter.isEmpty() && !entry.label.toLower().contains(filter)) continue;

    QListWidgetItem *item = new QListWidgetItem(entry.label, m_list);
    item->setData(Qt::UserRole, entry.action->text());
    // Large enough to be tapped reliably; the icon is what the user recognises.
    item->setSizeHint(QSize(0, AndroidUi::TouchTargetSize));
    if (entry.action->isCheckable() && entry.action->isChecked())
      item->setCheckState(Qt::Checked);
    if (!entry.action->icon().isNull()) item->setIcon(entry.action->icon());
  }
}

//-----------------------------------------------------------------------------

void AndroidCommandBar::onFilterChanged(const QString &) { applyFilter(); }

//-----------------------------------------------------------------------------

void AndroidCommandBar::onItemActivated(int row) {
  if (!m_list || row < 0 || row >= m_list->count()) return;

  const QString label =
      m_list->item(row)->text();  // "Menu > Item", used to find the action
  for (const Entry &entry : m_entries) {
    if (entry.label != label || !entry.action) continue;

    // Trigger the action exactly as the desktop menu would: the command
    // framework, the undo stack and the tool state stay in charge.
    entry.action->trigger();
    break;
  }

  // Keep the sheet open: drawing sessions trigger several commands in a row,
  // and reopening the sheet each time would be tedious on a phone.
}

//-----------------------------------------------------------------------------

void AndroidCommandBar::showCommands() {
  if (m_sheet->isVisible()) {
    m_sheet->hide();
    return;
  }

  // The sheet is re-parented to the top level window: the bar itself is only
  // a strip at the top of the screen and could not contain the list.
  QWidget *host = window();
  if (m_sheet->parentWidget() != host) {
    m_sheet->setParent(host);
  }

  if (m_search) {
    m_search->clear();
    m_search->setFocus();
  }
  applyFilter();

  const QPoint topLeft = host->mapFromGlobal(mapToGlobal(QPoint(0, height())));
  m_sheet->setGeometry(QRect(topLeft, QSize(host->width(), 0)));
  m_sheet->setFixedHeight(qMax(240, host->height() / 2));
  m_sheet->move(topLeft);
  m_sheet->show();
  m_sheet->raise();
}

//-----------------------------------------------------------------------------

void AndroidCommandBar::setRooms(const QStringList &names, int currentIndex) {
  m_roomNames   = names;
  m_currentRoom = currentIndex;
}

//-----------------------------------------------------------------------------

void AndroidCommandBar::showRooms() {
  if (m_roomNames.isEmpty()) return;

  QMenu menu(this);
  // The rooms are owned by the main window; the bar only presents them and
  // reports the choice back through roomRequested, which is the same signal
  // path the desktop room tab bar uses.
  for (int i = 0; i < m_roomNames.size(); ++i) {
    QAction *action = menu.addAction(m_roomNames.at(i));
    action->setCheckable(true);
    action->setChecked(i == m_currentRoom);
    action->setData(i);
    // Large rows: the menu is used with a finger.
    action->setIconVisibleInMenu(false);
  }

  QAction *chosen = menu.exec(mapToGlobal(rect().bottomLeft()));
  if (!chosen) return;

  const int index = chosen->data().toInt();
  if (index == m_currentRoom) return;

  m_currentRoom = index;
  emit roomRequested(index);
}

//-----------------------------------------------------------------------------

void AndroidCommandBar::onHomeRequested() {}

void AndroidCommandBar::onBackRequested() {}

//-----------------------------------------------------------------------------

void AndroidCommandBar::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);

  if (!m_sheet || !m_sheet->isVisible()) return;

  QWidget *host = m_sheet->parentWidget();
  if (!host) return;
  const QPoint topLeft = host->mapFromGlobal(mapToGlobal(QPoint(0, height())));
  m_sheet->move(topLeft);
}
