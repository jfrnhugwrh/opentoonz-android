//=============================================================================
//
//  Touch oriented controls for the Android build.
//
//  See androidtouchpanel.h for the rationale: this file contains no drawing
//  logic of its own.  Every button is bound to a CommandManager action, which
//  is the same entry point the desktop keyboard shortcuts and menu items use.
//
//=============================================================================

#include "androidtouchpanel.h"

#include "menubarcommandids.h"
#include "tapp.h"

#include "tools/toolcommandids.h"

#include "toonzqt/menubarcommand.h"
#include "toonzqt/viewcommandids.h"
#include "toonzqt/gutil.h"

#include "toonz/preferences.h"
#include "toonz/tscenehandle.h"
#include "toonz/tframehandle.h"

#include <QAction>
#include <QFrame>
#include <QGridLayout>
#include <QPainter>
#include <QToolButton>

#ifdef ANDROID
#include "otandroid.h"
#endif

//=============================================================================
//  AndroidUi
//=============================================================================

namespace AndroidUi {

bool isLargeScreen() {
#ifdef ANDROID
  return otandroid::isLargeScreen();
#else
  return true;
#endif
}

int systemBarInset() {
#ifdef ANDROID
  int widthDp = 0, heightDp = 0;
  otandroid::screenSize(widthDp, heightDp);
  // The status bar and the navigation bar are hidden in full screen mode; the
  // drawing surface still has to keep the cutout area clear.
  return isLargeScreen() ? 24 : 32;
#else
  return 0;
#endif
}

void applyTouchTheme() {
#ifdef ANDROID
  if (qApp) {
    QFont appFont = qApp->font();
    // The desktop interface is tuned for a mouse at 13 pixels; a finger needs
    // a noticeably larger face, and the platform's own scaling handles the
    // rest.
    appFont.setPixelSize(isLargeScreen() ? 15 : 14);
    qApp->setFont(appFont);
  }
#endif
}

}  // namespace AndroidUi

//=============================================================================
//  AndroidTouchPanel
//=============================================================================

AndroidTouchPanel::AndroidTouchPanel(QWidget *parent)
    : QWidget(parent), m_group(Drawing), m_layout(new QGridLayout(this)) {
  setObjectName("AndroidTouchPanel");
  m_layout->setContentsMargins(6, 6, 6, 6);
  m_layout->setSpacing(6);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
  setGroup(Drawing);
}

//-----------------------------------------------------------------------------

AndroidTouchPanel::~AndroidTouchPanel() {}

//-----------------------------------------------------------------------------

void AndroidTouchPanel::paintEvent(QPaintEvent *) {
  QPainter painter(this);
  painter.fillRect(rect(), palette().window());
}

//-----------------------------------------------------------------------------

void AndroidTouchPanel::clear() {
  m_buttons.clear();
  m_actions.clear();

  while (QLayoutItem *item = m_layout->takeAt(0)) {
    if (QWidget *widget = item->widget()) widget->deleteLater();
    delete item;
  }
}

//-----------------------------------------------------------------------------

void AndroidTouchPanel::addCommand(const char *commandId, const QString &text,
                                   int rowSpan, int columnSpan) {
  if (!commandId) return;

  QAction *action = CommandManager::instance()->getAction(commandId);
  if (!action) {
    // A command that does not exist on this build is simply skipped: the panel
    // is a convenience layer, not a hard dependency.
    return;
  }

  QToolButton *button = new QToolButton(this);
  button->setDefaultAction(action);
  button->setToolButtonStyle(text.isEmpty() ? Qt::ToolButtonIconOnly
                                            : Qt::ToolButtonTextUnderIcon);
  button->setText(text);
  button->setMinimumSize(AndroidUi::TouchTargetSize,
                         AndroidUi::TouchTargetSize);
  button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  button->setFocusPolicy(Qt::NoFocus);  // no focus rectangle for touch
  button->setAutoRaise(false);

  const int row = m_layout->rowCount();
  m_layout->addWidget(button, row, 0, rowSpan, columnSpan);

  Button entry;
  entry.widget = button;
  entry.action = action;
  m_buttons.append(entry);
  m_actions.insert(commandId, action);

  connect(action, &QAction::changed, this, &AndroidTouchPanel::onActionChanged);
}

//-----------------------------------------------------------------------------

void AndroidTouchPanel::refresh() {
  for (const Button &button : m_buttons) {
    if (!button.action) continue;
    button.widget->setEnabled(button.action->isEnabled());
    button.widget->setCheckable(button.action->isCheckable());
    button.widget->setChecked(button.action->isChecked());
  }
}

//-----------------------------------------------------------------------------

void AndroidTouchPanel::onActionChanged() {
  QAction *action = qobject_cast<QAction *>(sender());
  if (!action) return;

  for (const Button &button : m_buttons) {
    if (button.action != action) continue;
    button.widget->setEnabled(action->isEnabled());
    button.widget->setChecked(action->isChecked());
  }
}

//-----------------------------------------------------------------------------

void AndroidTouchPanel::setGroup(Group group) {
  m_group = group;
  clear();

  QWidget *page = nullptr;
  switch (group) {
  case Drawing: page = buildDrawingGroup(); break;
  case Timeline: page = buildTimelineGroup(); break;
  case View: page = buildViewGroup(); break;
  case Levels: page = buildLevelsGroup(); break;
  }

  if (page) {
    // The group builders fill *this* panel directly; the returned widget is
    // only an unused placeholder kept for symmetry with future layouts.
    page->deleteLater();
  }
  refresh();
}

//-----------------------------------------------------------------------------

QWidget *AndroidTouchPanel::buildDrawingGroup() {
  // Undo / redo are the two commands a drawing session needs most: they get a
  // full row of their own and are always within reach of the thumb.
  addCommand(MI_Undo, tr("Undo"), 1, 2);
  addCommand(MI_Redo, tr("Redo"), 1, 2);

  addCommand(T_Brush, tr("Brush"), 1, 1);
  addCommand(T_Eraser, tr("Eraser"), 1, 1);
  addCommand(T_Fill, tr("Fill"), 1, 1);
  addCommand(T_Type, tr("Text"), 1, 1);

  addCommand(MI_ShiftTrace, tr("Onion"), 1, 1);
  addCommand(MI_ViewCamera, tr("Camera"), 1, 1);
  addCommand(MI_ViewTable, tr("Table"), 1, 1);
  addCommand(MI_ViewBBox, tr("Boxes"), 1, 1);
  return nullptr;
}

//-----------------------------------------------------------------------------

QWidget *AndroidTouchPanel::buildTimelineGroup() {
  addCommand(MI_PrevFrame, tr("Prev"), 1, 2);
  addCommand(MI_NextFrame, tr("Next"), 1, 2);

  addCommand(MI_PrevDrawing, tr("Prev drawing"), 1, 2);
  addCommand(MI_NextDrawing, tr("Next drawing"), 1, 2);

  addCommand(MI_Play, tr("Play"), 1, 2);
  addCommand(MI_Loop, tr("Loop"), 1, 2);

  addCommand(MI_InsertSceneFrame, tr("Insert"), 1, 2);
  addCommand(MI_RemoveSceneFrame, tr("Remove"), 1, 2);
  return nullptr;
}

//-----------------------------------------------------------------------------

QWidget *AndroidTouchPanel::buildViewGroup() {
  addCommand(V_ZoomIn, tr("Zoom in"), 1, 2);
  addCommand(V_ZoomOut, tr("Zoom out"), 1, 2);

  addCommand(V_ZoomFit, tr("Fit"), 1, 2);
  addCommand(V_ActualPixelSize, tr("1:1"), 1, 2);

  addCommand(V_RotateLeft, tr("Rotate -"), 1, 2);
  addCommand(V_RotateRight, tr("Rotate +"), 1, 2);

  addCommand(V_FlipX, tr("Flip H"), 1, 2);
  addCommand(V_FlipY, tr("Flip V"), 1, 2);
  return nullptr;
}

//-----------------------------------------------------------------------------

QWidget *AndroidTouchPanel::buildLevelsGroup() {
  addCommand(MI_NewVectorLevel, tr("Vector"), 1, 2);
  addCommand(MI_NewToonzRasterLevel, tr("Toonz raster"), 1, 2);

  addCommand(MI_SaveLevel, tr("Save level"), 1, 2);
  addCommand(MI_SaveLevelAs, tr("Save as"), 1, 2);

  addCommand(MI_ExportLevel, tr("Export"), 1, 2);
  addCommand(MI_ExportCurrentScene, tr("Export scene"), 1, 2);
  return nullptr;
}
