#include "src/app/ApplicationTheme.h"

#include <QColor>
#include <QFont>
#include <QPalette>

namespace ApplicationTheme
{
void apply(QApplication &app)
{
    app.setStyle(QStringLiteral("Fusion"));

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(QStringLiteral("#EEF2F7")));
    palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#11243C")));
    palette.setColor(QPalette::Base, QColor(QStringLiteral("#FFFFFF")));
    palette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#F7F9FC")));
    palette.setColor(QPalette::Text, QColor(QStringLiteral("#11243C")));
    palette.setColor(QPalette::Button, QColor(QStringLiteral("#FFFFFF")));
    palette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#11243C")));
    palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#156FEC")));
    palette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#FFFFFF")));
    app.setPalette(palette);

    QFont font = app.font();
    font.setPointSizeF(10.0);
    app.setFont(font);

    app.setStyleSheet(QStringLiteral(
        "QMainWindow { background: #EEF2F7; }"
        "QFrame#topBar { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0F2744, stop:1 #173B66); border-radius: 20px; }"
        "QFrame#panelCard, QFrame#logCard, QFrame#channelCard, QFrame#passwordCard {"
        "  background: #FFFFFF; border: 1px solid #D7E0EA; border-radius: 18px; }"
        "QFrame#channelCard[alarm=\"true\"] { border: 2px solid #D94841; background: #FFF5F5; }"
        "QLabel#heroTitle { color: #FFFFFF; font-size: 20px; font-weight: 700; }"
        "QLabel#heroSubtitle { color: rgba(255,255,255,0.75); font-size: 11px; }"
        "QLabel#connectionBadge { border-radius: 12px; padding: 4px 10px; font-weight: 600; }"
        "QLabel#connectionBadge[connected=\"true\"] { background: #D1FADF; color: #067647; }"
        "QLabel#connectionBadge[connected=\"false\"] { background: #FEE4E2; color: #B42318; }"
        "QLabel#channelBadge { border-radius: 10px; padding: 3px 8px; font-weight: 700; }"
        "QLabel#channelBadge[alarm=\"true\"] { background: #FECACA; color: #B42318; }"
        "QLabel#channelBadge[alarm=\"false\"] { background: #D1FADF; color: #067647; }"
        "QLabel#sectionTitle { font-size: 15px; font-weight: 700; color: #163355; }"
        "QLabel#sectionCaption { color: #5D728A; }"
        "QPushButton {"
        "  background: #156FEC; color: #FFFFFF; border: none; border-radius: 10px; padding: 8px 14px; font-weight: 600; }"
        "QPushButton:hover { background: #0F62D6; }"
        "QPushButton:pressed { background: #0B4FAA; }"
        "QPushButton[secondary=\"true\"] { background: #E8EEF7; color: #17324D; }"
        "QPushButton[secondary=\"true\"]:hover { background: #DCE6F3; }"
        "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QTableWidget {"
        "  border: 1px solid #C8D4E0; border-radius: 10px; padding: 6px 8px; background: #FFFFFF; color: #11243C; }"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus { border: 1px solid #156FEC; }"
        "QGroupBox { border: 1px solid #E0E7EF; border-radius: 14px; margin-top: 12px; padding-top: 12px; font-weight: 700; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; color: #163355; }"
        "QHeaderView::section { background: #F7F9FC; border: none; border-bottom: 1px solid #E0E7EF; padding: 8px; color: #44566C; font-weight: 600; }"
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical { background: #E5EDF6; width: 24px; margin: 2px; border-radius: 12px; }"
        "QScrollBar::handle:vertical { background: #B1C2D8; min-height: 56px; border-radius: 10px; border: 3px solid #E5EDF6; }"
        "QScrollBar:horizontal { background: #E5EDF6; height: 24px; margin: 2px; border-radius: 12px; }"
        "QScrollBar::handle:horizontal { background: #B1C2D8; min-width: 56px; border-radius: 10px; border: 3px solid #E5EDF6; }"
        "QScrollBar::add-line, QScrollBar::sub-line, QScrollBar::add-page, QScrollBar::sub-page { background: transparent; border: none; }"
        "QStatusBar { background: transparent; color: #44566C; }"
    ));
}
}
