#include "openmvterminal.h"

#define TERMINAL_SETTINGS_GROUP "OpenMVTerminal"
#define GEOMETRY "Geometry"
#define HSPLITTER_STATE "HSplitterState"
#define VSPLITTER_STATE "VSplitterState"
#define ZOOM_STATE "ZoomState"
#define FONT_ZOOM_STATE "FontZoomState"
#define LAST_SAVE_IMAGE_PATH "LastSaveImagePath"
#define HISTOGRAM_COLOR_SPACE_STATE "HistogramColorSpace"

MyPlainTextEdit::MyPlainTextEdit(qreal fontPointSizeF, QWidget *parent) : QPlainTextEdit(parent)
{
    m_tabWidth = TextEditor::TextEditorSettings::codeStyle()->tabSettings().m_serialTerminalTabSize;
    m_textCursor = QTextCursor(document());
    m_stateMachine = ASCII;
    m_shiftReg = QByteArray();
    m_frameBufferData = QByteArray();
    m_handler = Utils::AnsiEscapeCodeHandler();
    m_lastChar = QChar();
    connect(TextEditor::TextEditorSettings::codeStyle(), &TextEditor::ICodeStylePreferences::tabSettingsChanged,
            this, [this] (const TextEditor::TabSettings &settings) {
        m_tabWidth = settings.m_serialTerminalTabSize;
    });

    setReadOnly(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setFrameShape(QFrame::NoFrame);
    setUndoRedoEnabled(false);
    setMaximumBlockCount(100000);
    setWordWrapMode(QTextOption::NoWrap);

    QFont font = TextEditor::TextEditorSettings::fontSettings().defaultFixedFontFamily();
    font.setPointSize(fontPointSizeF);
    setFont(font);

    QPalette p = palette();
    p.setColor(QPalette::Highlight, p.color(QPalette::Active, QPalette::Highlight));
    p.setColor(QPalette::HighlightedText, p.color(QPalette::Active, QPalette::HighlightedText));
    p.setColor(QPalette::Base, QColor(QStringLiteral("#1E1E27")));
    p.setColor(QPalette::Text, QColor(QStringLiteral("#EEEEF7")));
    setPalette(p);
}

void MyPlainTextEdit::readBytes(const QByteArray &data)
{
    bool atBottom = verticalScrollBar()->value() == verticalScrollBar()->maximum();

    QByteArray buffer;

    for(int i = 0, j = data.size(); i < j; i++)
    {
        if((m_stateMachine == UTF_8) && ((data.at(i) & 0xC0) != 0x80))
        {
            m_stateMachine = ASCII;
        }

        if((m_stateMachine == EXIT_0) && ((data.at(i) & 0xFF) != 0x00))
        {
            m_stateMachine = ASCII;
        }

        switch(m_stateMachine)
        {
            case ASCII:
            {
                if(((data.at(i) & 0xE0) == 0xC0)
                || ((data.at(i) & 0xF0) == 0xE0)
                || ((data.at(i) & 0xF8) == 0xF0)
                || ((data.at(i) & 0xFC) == 0xF8)
                || ((data.at(i) & 0xFE) == 0xFC)) // UTF_8
                {
                    m_shiftReg.clear();

                    m_stateMachine = UTF_8;
                }
                else if((data.at(i) & 0xFF) == 0xFF)
                {
                    m_stateMachine = EXIT_0;
                }
                else if((data.at(i) & 0xC0) == 0x80)
                {
                    m_frameBufferData.append(data.at(i));
                }
                else if((data.at(i) & 0xFF) == 0xFE)
                {
                    int size = m_frameBufferData.size();
                    QByteArray temp;

                    for(int k = 0, l = (size / 4) * 4; k < l; k += 4)
                    {
                        int x = 0;
                        x |= (m_frameBufferData.at(k + 0) & 0x3F) << 0;
                        x |= (m_frameBufferData.at(k + 1) & 0x3F) << 6;
                        x |= (m_frameBufferData.at(k + 2) & 0x3F) << 12;
                        x |= (m_frameBufferData.at(k + 3) & 0x3F) << 18;
                        temp.append((x >> 0) & 0xFF);
                        temp.append((x >> 8) & 0xFF);
                        temp.append((x >> 16) & 0xFF);
                    }

                    if((size % 4) == 3) // 2 bytes -> 16-bits -> 24-bits sent
                    {
                        int x = 0;
                        x |= (m_frameBufferData.at(size - 3) & 0x3F) << 0;
                        x |= (m_frameBufferData.at(size - 2) & 0x3F) << 6;
                        x |= (m_frameBufferData.at(size - 1) & 0x0F) << 12;
                        temp.append((x >> 0) & 0xFF);
                        temp.append((x >> 8) & 0xFF);
                    }

                    if((size % 4) == 2) // 1 byte -> 8-bits -> 16-bits sent
                    {
                        int x = 0;
                        x |= (m_frameBufferData.at(size - 2) & 0x3F) << 0;
                        x |= (m_frameBufferData.at(size - 1) & 0x03) << 6;
                        temp.append((x >> 0) & 0xFF);
                    }

                    QPixmap pixmap = QPixmap::fromImage(QImage::fromData(temp, "JPG"));

                    if(!pixmap.isNull())
                    {
                        emit frameBufferData(pixmap);
                    }

                    m_frameBufferData.clear();
                }
                else if((data.at(i) & 0x80) == 0x00) // ASCII
                {
                    buffer.append(data.at(i));
                }

                break;
            }

            case UTF_8:
            {
                if((((m_shiftReg.at(0) & 0xE0) == 0xC0) && (m_shiftReg.size() == 1))
                || (((m_shiftReg.at(0) & 0xF0) == 0xE0) && (m_shiftReg.size() == 2))
                || (((m_shiftReg.at(0) & 0xF8) == 0xF0) && (m_shiftReg.size() == 3))
                || (((m_shiftReg.at(0) & 0xFC) == 0xF8) && (m_shiftReg.size() == 4))
                || (((m_shiftReg.at(0) & 0xFE) == 0xFC) && (m_shiftReg.size() == 5)))
                {
                    buffer.append(m_shiftReg + data.at(i));

                    m_stateMachine = ASCII;
                }

                break;
            }

            case EXIT_0:
            {
                m_stateMachine = EXIT_1;

                break;
            }

            case EXIT_1:
            {
                m_stateMachine = ASCII;

                break;
            }
        }

        m_shiftReg = m_shiftReg.append(data.at(i)).right(5);
    }

    foreach(const Utils::FormattedText &text, m_handler.parseText(Utils::FormattedText(QString::fromUtf8(buffer))))
    {
        QString string;
        int column = m_textCursor.columnNumber();

        if(m_lastChar.unicode() == '\n')
        {
            string.append(QLatin1Char('\n'));
        }

        for(int i = 0, j = text.text.size(); i < j; i++)
        {
            switch(text.text.at(i).unicode())
            {
                case 15:
                case 17: // XON
                case 19: // XOFF
                case 23:
                case 24:
                case 25:
                case 26:
                case 27: // Escape - AnsiEscapeCodeHandler
                case 28:
                case 29:
                case 30:
                case 31:
                {
                    break;
                }

                case 0: // Null
                {
                    m_textCursor.insertText(string, text.format);
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                case 1: // Home Cursor
                {
                    m_textCursor.insertText(string, text.format);
                    m_textCursor.movePosition(QTextCursor::Start);
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                case 2: // Move Cursor Left
                {
                    m_textCursor.insertText(string, text.format);
                    m_textCursor.movePosition(QTextCursor::Left);
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                case 3: // Clear Screen
                {
                    m_textCursor.insertText(string, text.format);
                    m_textCursor.select(QTextCursor::Document);
                    m_textCursor.removeSelectedText();
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                case 4:
                case 127: // Delete
                {
                    m_textCursor.insertText(string, text.format);
                    m_textCursor.deleteChar();
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                case 5: // End Cursor
                {
                    m_textCursor.insertText(string, text.format);
                    m_textCursor.movePosition(QTextCursor::End);
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                case 6: // Move Cursor Right
                {
                    m_textCursor.insertText(string, text.format);
                    m_textCursor.movePosition(QTextCursor::Right);
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                case 7: // Beep Speaker
                {
                    m_textCursor.insertText(string, text.format);
                    QApplication::beep();
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                case 8: // Backspace
                {
                    m_textCursor.insertText(string, text.format);
                    m_textCursor.deletePreviousChar();
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                case 9: // Tab
                {
                    for(int k = m_tabWidth - (column % m_tabWidth); k > 0; k--)
                    {
                        string.append(text.text.at(i));
                        column += 1;
                    }

                    break;
                }

                case 10: // Line Feed
                {
                    if(m_lastChar.unicode() != '\r')
                    {
                        string.append(QLatin1Char('\n'));
                        column = 0;
                    }

                    break;
                }

                case 11: // Clear to end of line.
                {
                    m_textCursor.insertText(string, text.format);
                    m_textCursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
                    m_textCursor.removeSelectedText();
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                case 12: // Clear lines below.
                {
                    m_textCursor.insertText(string, text.format);
                    m_textCursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
                    m_textCursor.removeSelectedText();
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                case 13: // Carriage Return
                {
                    string.append(QLatin1Char('\n'));
                    column = 0;
                    break;
                }

                case 14: // Move Cursor Down
                {
                    m_textCursor.insertText(string, text.format);
                    m_textCursor.movePosition(QTextCursor::Down);
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                case 16: // Move Cursor Up
                {
                    m_textCursor.insertText(string, text.format);
                    m_textCursor.movePosition(QTextCursor::Up);
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                case 18: // Move to start of line.
                {
                    m_textCursor.insertText(string, text.format);
                    m_textCursor.movePosition(QTextCursor::StartOfLine);
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                case 20: // Move to end of line.
                {
                    m_textCursor.insertText(string, text.format);
                    m_textCursor.movePosition(QTextCursor::EndOfLine);
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                case 21: // Clear to start of line.
                {
                    m_textCursor.insertText(string, text.format);
                    m_textCursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
                    m_textCursor.removeSelectedText();
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                case 22: // Clear lines above.
                {
                    m_textCursor.insertText(string, text.format);
                    m_textCursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
                    m_textCursor.removeSelectedText();
                    string.clear();
                    column = m_textCursor.columnNumber();
                    break;
                }

                default:
                {
                    string.append(text.text.at(i));
                    column += 1;
                    break;
                }
            }

            m_lastChar = text.text.at(i);
        }

        if(string.endsWith(QLatin1Char('\n')))
        {
            string.chop(1);
        }

        if(m_textCursor.document()->isEmpty())
        {
            string.remove(QRegularExpression(QStringLiteral("^\\s+")));
        }

        m_textCursor.insertText(string, text.format);
    }

    if(atBottom)
    {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
        // QPlainTextEdit destroys the first calls value in case of multiline
        // text, so make sure that the scroll bar actually gets the value set.
        // Is a noop if the first call succeeded.
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }
}

void MyPlainTextEdit::clear()
{
    m_textCursor.select(QTextCursor::Document);
    m_textCursor.removeSelectedText();
    m_textCursor = QTextCursor(document());
    m_stateMachine = ASCII;
    m_shiftReg = QByteArray();
    m_frameBufferData = QByteArray();
    m_handler = Utils::AnsiEscapeCodeHandler();
    m_lastChar = QChar();
}

void MyPlainTextEdit::execute()
{
    // Write bytes slowly so as to not overload the MicroPython board.

    QByteArray data = "\x05" + QString::fromUtf8(Core::EditorManager::currentEditor()->document()->contents()).toUtf8() + "\x04";

    for(int i = 0; i < data.size(); i++)
    {
        emit writeBytes(data.mid(i, 1));
    }

    // emit writeBytes("\x05" + QString::fromUtf8(Core::EditorManager::currentEditor()->document()->contents()).toUtf8() + "\x04");
}

void MyPlainTextEdit::interrupt()
{
    emit writeBytes("\x03");
}

void MyPlainTextEdit::reload()
{
    emit writeBytes("\x04");
}

void MyPlainTextEdit::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Qt::Key_Delete:
        {
            emit writeBytes("\x04"); // CTRL+D (4)
            break;
        }
        case Qt::Key_Home:
        {
            if(!(event->modifiers() & Qt::ControlModifier))
            {
                emit writeBytes("\x01"); // CTRL+A (1)
            }

            break;
        }
        case Qt::Key_End:
        {
            if(!(event->modifiers() & Qt::ControlModifier))
            {
                emit writeBytes("\x05"); // CTRL+E (5)
            }

            break;
        }
        case Qt::Key_Left:
        {
            emit writeBytes("\x02"); // CTRL+B (2)
            break;
        }
        case Qt::Key_Up:
        {
            emit writeBytes("\x10"); // CTRL+P (16)
            break;
        }
        case Qt::Key_Right:
        {
            emit writeBytes("\x06"); // CTRL+F (6)
            break;
        }
        case Qt::Key_Down:
        {
            emit writeBytes("\x0E"); // CTRL+N (14)
            break;
        }
        case Qt::Key_PageUp:
        {
            QPlainTextEdit::keyPressEvent(event);
            break;
        }
        case Qt::Key_PageDown:
        {
            QPlainTextEdit::keyPressEvent(event);
            break;
        }
        case Qt::Key_Tab:
        case Qt::Key_Backspace:
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Shift:
        case Qt::Key_Control:
        case Qt::Key_Space:
        case Qt::Key_Exclam:
        case Qt::Key_QuoteDbl:
        case Qt::Key_NumberSign:
        case Qt::Key_Dollar:
        case Qt::Key_Percent:
        case Qt::Key_Ampersand:
        case Qt::Key_Apostrophe:
        case Qt::Key_ParenLeft:
        case Qt::Key_ParenRight:
        case Qt::Key_Asterisk:
        case Qt::Key_Plus:
        case Qt::Key_Comma:
        case Qt::Key_Minus:
        case Qt::Key_Period:
        case Qt::Key_Slash:
        case Qt::Key_0:
        case Qt::Key_1:
        case Qt::Key_2:
        case Qt::Key_3:
        case Qt::Key_4:
        case Qt::Key_5:
        case Qt::Key_6:
        case Qt::Key_7:
        case Qt::Key_8:
        case Qt::Key_9:
        case Qt::Key_Colon:
        case Qt::Key_Semicolon:
        case Qt::Key_Less:
        case Qt::Key_Equal:
        case Qt::Key_Greater:
        case Qt::Key_Question:
        case Qt::Key_At:
        case Qt::Key_A:
        case Qt::Key_B:
        case Qt::Key_C:
        case Qt::Key_D:
        case Qt::Key_E:
        case Qt::Key_F:
        case Qt::Key_G:
        case Qt::Key_H:
        case Qt::Key_I:
        case Qt::Key_J:
        case Qt::Key_K:
        case Qt::Key_L:
        case Qt::Key_M:
        case Qt::Key_N:
        case Qt::Key_O:
        case Qt::Key_P:
        case Qt::Key_Q:
        case Qt::Key_R:
        case Qt::Key_S:
        case Qt::Key_T:
        case Qt::Key_U:
        case Qt::Key_V:
        case Qt::Key_W:
        case Qt::Key_X:
        case Qt::Key_Y:
        case Qt::Key_Z:
        case Qt::Key_BracketLeft:
        case Qt::Key_Backslash:
        case Qt::Key_BracketRight:
        case Qt::Key_AsciiCircum:
        case Qt::Key_Underscore:
        case Qt::Key_QuoteLeft:
        case Qt::Key_BraceLeft:
        case Qt::Key_Bar:
        case Qt::Key_BraceRight:
        case Qt::Key_AsciiTilde:
        {
            QByteArray data = event->text().toUtf8();

            if((data == "\r") || (data == "\r\n") || (data == "\n"))
            {
                emit writeBytes("\r\n");
            }
            else
            {
                emit writeBytes(data);
            }

            break;
        }
    }

    // Ensure we scroll also on Ctrl+Home or Ctrl+End

    if(event->matches(QKeySequence::MoveToStartOfDocument))
    {
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMinimum);
    }
    else if(event->matches(QKeySequence::MoveToEndOfDocument))
    {
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMaximum);
    }
}

void MyPlainTextEdit::wheelEvent(QWheelEvent *event)
{
    QPlainTextEdit::wheelEvent(event);

    if(event->modifiers() & Qt::ControlModifier) {
        Utils::FadingIndicator::showText(this, tr("Zoom: %1%").arg(int(100 * (font().pointSizeF() / TextEditor::TextEditorSettings::fontSettings().defaultFontSize()))), Utils::FadingIndicator::SmallText);
    }
}

void MyPlainTextEdit::resizeEvent(QResizeEvent *event)
{
    bool atBottom = verticalScrollBar()->value() == verticalScrollBar()->maximum();
    QPlainTextEdit::resizeEvent(event);

    if(atBottom)
    {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
        // QPlainTextEdit destroys the first calls value in case of multiline
        // text, so make sure that the scroll bar actually gets the value set.
        // Is a noop if the first call succeeded.
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }
}

void MyPlainTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;

    connect(menu.addAction(tr("Copy")), &QAction::triggered, this, [this] {
        copy();
    });

    connect(menu.addAction(tr("Paste")), &QAction::triggered, this, [this] {
        emit writeBytes(QApplication::clipboard()->text().toUtf8());
    });

    menu.addSeparator();

    connect(menu.addAction(tr("Select All")), &QAction::triggered, this, [this] {
        selectAll();
    });

    menu.addSeparator();

    connect(menu.addAction(tr("Find")), &QAction::triggered, this, [this] {
        Core::ActionManager::command(Core::Constants::FIND_IN_DOCUMENT)->action()->trigger();
    });

    menu.exec(event->globalPos());
}

bool MyPlainTextEdit::focusNextPrevChild(bool next)
{
    Q_UNUSED(next)

    return false;
}

OpenMVTerminal::OpenMVTerminal(const QString &displayName, QSettings *settings, const Core::Context &context, QWidget *parent) : QWidget(parent)
{
    setWindowTitle(displayName);
    setAttribute(Qt::WA_DeleteOnClose);

    m_settings = new QSettings(settings->fileName(), settings->format(), this);
    m_settings->beginGroup(QStringLiteral(TERMINAL_SETTINGS_GROUP));
    m_settings->beginGroup(displayName);

    Utils::StyledBar *styledBar0 = new Utils::StyledBar;
    QHBoxLayout *styledBar0Layout = new QHBoxLayout;
    styledBar0Layout->setMargin(0);
    styledBar0Layout->setSpacing(0);
    styledBar0Layout->addSpacing(4);
    styledBar0Layout->addWidget(new QLabel(tr("Frame Buffer")));
    styledBar0Layout->addSpacing(6);
    styledBar0->setLayout(styledBar0Layout);

    m_zoom = new QToolButton;
    m_zoom->setText(tr("Zoom"));
    m_zoom->setToolTip(tr("Zoom to fit"));
    m_zoom->setCheckable(true);
    styledBar0Layout->addWidget(m_zoom);

    OpenMVPluginFB *m_frameBuffer = new OpenMVPluginFB;
    QWidget *tempWidget0 = new QWidget;
    QVBoxLayout *tempLayout0 = new QVBoxLayout;
    tempLayout0->setMargin(0);
    tempLayout0->setSpacing(0);
    tempLayout0->addWidget(styledBar0);
    tempLayout0->addWidget(m_frameBuffer);
    tempWidget0->setLayout(tempLayout0);
    connect(m_zoom, &QToolButton::toggled, m_frameBuffer, &OpenMVPluginFB::enableFitInView);
    m_zoom->setChecked(m_settings->value(QStringLiteral(ZOOM_STATE), false).toBool());
    connect(m_frameBuffer, &OpenMVPluginFB::saveImage, this, [this] (const QPixmap &data) {
        QString path =
            QFileDialog::getSaveFileName(this, tr("Save Image"),
                m_settings->value(QStringLiteral(LAST_SAVE_IMAGE_PATH), QDir::homePath()).toString(),
                tr("Image Files (*.bmp *.jpg *.jpeg *.png *.ppm)"));

        if(!path.isEmpty())
        {
            if(data.save(path))
            {
                m_settings->setValue(QStringLiteral(LAST_SAVE_IMAGE_PATH), path);
            }
            else
            {
                QMessageBox::critical(this,
                    tr("Save Image"),
                    tr("Failed to save the image file for an unknown reason!"));
            }
        }
    });

    Utils::StyledBar *styledBar1 = new Utils::StyledBar;
    QHBoxLayout *styledBar1Layout = new QHBoxLayout;
    styledBar1Layout->setMargin(0);
    styledBar1Layout->setSpacing(0);
    styledBar1Layout->addSpacing(4);
    styledBar1Layout->addWidget(new QLabel(tr("Histogram")));
    styledBar1Layout->addSpacing(6);
    styledBar1->setLayout(styledBar1Layout);

    m_histogramColorSpace = new QComboBox;
    m_histogramColorSpace->setProperty("hideborder", true);
    m_histogramColorSpace->setProperty("drawleftborder", false);
    m_histogramColorSpace->insertItem(RGB_COLOR_SPACE, tr("RGB Color Space"));
    m_histogramColorSpace->insertItem(GRAYSCALE_COLOR_SPACE, tr("Grayscale Color Space"));
    m_histogramColorSpace->insertItem(LAB_COLOR_SPACE, tr("LAB Color Space"));
    m_histogramColorSpace->insertItem(YUV_COLOR_SPACE, tr("YUV Color Space"));
    m_histogramColorSpace->setToolTip(tr("Use Grayscale/LAB for color tracking"));
    styledBar1Layout->addWidget(m_histogramColorSpace);

    OpenMVPluginHistogram *m_histogram = new OpenMVPluginHistogram;
    QWidget *tempWidget1 = new QWidget;
    QVBoxLayout *tempLayout1 = new QVBoxLayout;
    tempLayout1->setMargin(0);
    tempLayout1->setSpacing(0);
    tempLayout1->addWidget(styledBar1);
    tempLayout1->addWidget(m_histogram);
    tempWidget1->setLayout(tempLayout1);
    connect(m_histogramColorSpace, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), m_histogram, &OpenMVPluginHistogram::colorSpaceChanged);
    m_histogramColorSpace->setCurrentIndex(m_settings->value(QStringLiteral(HISTOGRAM_COLOR_SPACE_STATE), RGB_COLOR_SPACE).toInt());
    connect(m_frameBuffer, &OpenMVPluginFB::pixmapUpdate, m_histogram, &OpenMVPluginHistogram::pixmapUpdate);

    Utils::StyledBar *styledBar2 = new Utils::StyledBar;
    QHBoxLayout *styledBar2Layout = new QHBoxLayout;
    styledBar2Layout->setMargin(0);
    styledBar2Layout->setSpacing(0);
    styledBar2Layout->addSpacing(5);
    //styledBar2Layout->addWidget(new QLabel(tr("Serial Terminal")));
    styledBar2Layout->addSpacing(7);
    styledBar2Layout->addWidget(new Utils::StyledSeparator);
    styledBar2->setLayout(styledBar2Layout);

    QToolButton *clearButton = new QToolButton;
    clearButton->setIcon(Utils::Icon({{QStringLiteral(":/core/images/clean_pane_small.png"), Utils::Theme::IconsBaseColor}}).icon());
    clearButton->setToolTip(tr("Clear"));
    styledBar2Layout->addWidget(clearButton);

    QToolButton *executeButton = new QToolButton;
    executeButton->setIcon(Utils::Icon({{QStringLiteral(":/core/images/run_small.png"), Utils::Theme::IconsBaseColor}}).icon());
    executeButton->setToolTip(tr("Run current script in editor window"));
    styledBar2Layout->addWidget(executeButton);
    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged, executeButton, [executeButton] (Core::IEditor *editor) {
        executeButton->setEnabled(editor ? (editor->document() ? (!editor->document()->contents().isEmpty()) : false) : false);
    });

    QToolButton *interruptButton = new QToolButton;
    interruptButton->setIcon(Utils::Icon({{QStringLiteral(":/core/images/stop_small.png"), Utils::Theme::IconsBaseColor}}).icon());
    interruptButton->setToolTip(tr("Stop running script"));
    styledBar2Layout->addWidget(interruptButton);

    QToolButton *reloadButton = new QToolButton;
    reloadButton->setIcon(Utils::Icon({{QStringLiteral(":/core/images/reload_gray.png"), Utils::Theme::IconsBaseColor}}).icon());
    reloadButton->setToolTip(tr("Soft reset"));
    styledBar2Layout->addWidget(reloadButton);
    styledBar2Layout->addStretch(1);

    m_edit = new MyPlainTextEdit(m_settings->value(QStringLiteral(FONT_ZOOM_STATE), TextEditor::TextEditorSettings::fontSettings().defaultFontSize()).toReal());
    connect(this, &OpenMVTerminal::readBytes, m_edit, &MyPlainTextEdit::readBytes);
    connect(m_edit, &MyPlainTextEdit::writeBytes, this, &OpenMVTerminal::writeBytes);
    connect(m_edit, &MyPlainTextEdit::frameBufferData, m_frameBuffer, &OpenMVPluginFB::frameBufferData);
    connect(clearButton, &QToolButton::clicked, m_edit, &MyPlainTextEdit::clear);
    connect(executeButton, &QToolButton::clicked, m_edit, &MyPlainTextEdit::execute);
    connect(interruptButton, &QToolButton::clicked, m_edit, &MyPlainTextEdit::interrupt);
    connect(reloadButton, &QToolButton::clicked, m_edit, &MyPlainTextEdit::reload);

    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate;
    aggregate->add(m_edit);
    aggregate->add(new Core::BaseTextFind(m_edit));

    QWidget *tempWidget2 = new QWidget;
    QVBoxLayout *tempLayout2 = new QVBoxLayout;
    tempLayout2->setMargin(0);
    tempLayout2->setSpacing(0);
    //tempLayout2->addWidget(styledBar2); // DGP
    tempLayout2->addWidget(m_edit);
    tempLayout2->addWidget(new Core::FindToolBarPlaceHolder(this));
    tempWidget2->setLayout(tempLayout2);

    m_hsplitter = new Core::MiniSplitter(Qt::Horizontal);
    m_vsplitter = new Core::MiniSplitter(Qt::Vertical);
    m_vsplitter->insertWidget(0, tempWidget0);
    m_vsplitter->insertWidget(1, tempWidget1);
    m_vsplitter->setStretchFactor(0, 0);
    m_vsplitter->setStretchFactor(1, 1);
    m_vsplitter->setCollapsible(0, true);
    m_vsplitter->setCollapsible(1, true);
    m_hsplitter->insertWidget(0, tempWidget2);
    m_hsplitter->insertWidget(1, m_vsplitter);
    m_hsplitter->setStretchFactor(0, 1);
    m_hsplitter->setStretchFactor(1, 0);
    m_hsplitter->setCollapsible(0, true);
    m_hsplitter->setCollapsible(1, true);

    QGridLayout *layout = new QGridLayout();
    layout->setMargin(0);
    layout->addWidget(m_hsplitter);
    setLayout(layout);

    QMainWindow *mainWindow = qobject_cast<QMainWindow *>(Core::ICore::mainWindow());

    if(mainWindow)
    {
        QWidget *widget = mainWindow->centralWidget();

        if(widget)
        {
            setStyleSheet(widget->styleSheet());
        }
    }

    m_context = new Core::IContext;
    m_context->setContext(context);
    m_context->setWidget(this);
    Core::ICore::addContextObject(m_context);

    Core::Command *overrideCtrlE = Core::ActionManager::registerAction(new QAction(QString(), Q_NULLPTR), Core::Id("NXTCam5.Terminal.Ctrl.E"), context);
    overrideCtrlE->setDefaultKeySequence(tr("Ctrl+E"));
    Core::Command *overrideCtrlR = Core::ActionManager::registerAction(new QAction(QString(), Q_NULLPTR), Core::Id("NXTCam5.Terminal.Ctrl.R"), context);
    overrideCtrlR->setDefaultKeySequence(tr("Ctrl+R"));
}

OpenMVTerminal::~OpenMVTerminal()
{
    Core::ICore::removeContextObject(m_context);
    delete m_context;
}

void OpenMVTerminal::showEvent(QShowEvent *event)
{
    if(m_settings->contains(QStringLiteral(GEOMETRY)))
    {
        restoreGeometry(m_settings->value(QStringLiteral(GEOMETRY)).toByteArray());
    }

    if(m_settings->contains(QStringLiteral(HSPLITTER_STATE)))
    {
        m_hsplitter->restoreState(m_settings->value(QStringLiteral(HSPLITTER_STATE)).toByteArray());
    }

    if(m_settings->contains(QStringLiteral(VSPLITTER_STATE)))
    {
        m_vsplitter->restoreState(m_settings->value(QStringLiteral(VSPLITTER_STATE)).toByteArray());
    }

    QWidget::showEvent(event);
}

void OpenMVTerminal::closeEvent(QCloseEvent *event)
{
    m_settings->setValue(QStringLiteral(GEOMETRY), saveGeometry());
    m_settings->setValue(QStringLiteral(HSPLITTER_STATE), m_hsplitter->saveState());
    m_settings->setValue(QStringLiteral(VSPLITTER_STATE), m_vsplitter->saveState());
    m_settings->setValue(QStringLiteral(ZOOM_STATE), m_zoom->isChecked());
    m_settings->setValue(QStringLiteral(FONT_ZOOM_STATE), m_edit->font().pointSizeF());
    m_settings->setValue(QStringLiteral(HISTOGRAM_COLOR_SPACE_STATE), m_histogramColorSpace->currentIndex());

    QWidget::closeEvent(event);
}

OpenMVTerminalSerialPort_private::OpenMVTerminalSerialPort_private(QObject *parent) : QObject(parent)
{
    m_port = Q_NULLPTR;
}

void OpenMVTerminalSerialPort_private::open(const QString &portName, int buadRate)
{
    if(m_port)
    {
        delete m_port;
    }

    m_port = new QSerialPort(portName, this);

    connect(m_port, &QSerialPort::readyRead, this, [this] {
        emit readBytes(m_port->readAll());
    });

    if((!m_port->setBaudRate(buadRate))
    || (!m_port->open(QIODevice::ReadWrite))
    || (!m_port->setDataTerminalReady(true)))
    {
        emit openResult(m_port->errorString());
        delete m_port;
        m_port = Q_NULLPTR;
    }
    else
    {
        emit openResult(QString());
    }
}

void OpenMVTerminalSerialPort_private::writeBytes(const QByteArray &data)
{
    if(m_port)
    {
        m_port->clearError();

        if((m_port->write(data) != data.size()) || (!m_port->flush()))
        {
            delete m_port;
            m_port = Q_NULLPTR;
        }
    }
}

OpenMVTerminalSerialPort::OpenMVTerminalSerialPort(QObject *parent) : OpenMVTerminalPort(parent)
{
    QThread *thread = new QThread;
    OpenMVTerminalSerialPort_private* port = new OpenMVTerminalSerialPort_private;
    port->moveToThread(thread);

    connect(this, &OpenMVTerminalSerialPort::open,
            port, &OpenMVTerminalSerialPort_private::open);

    connect(port, &OpenMVTerminalSerialPort_private::openResult,
            this, &OpenMVTerminalSerialPort::openResult);

    connect(this, &OpenMVTerminalSerialPort::writeBytes,
            port, &OpenMVTerminalSerialPort_private::writeBytes);

    connect(port, &OpenMVTerminalSerialPort_private::readBytes,
            this, &OpenMVTerminalSerialPort::readBytes);

    connect(this, &OpenMVTerminalSerialPort::destroyed,
            port, &OpenMVTerminalSerialPort_private::deleteLater);

    connect(port, &OpenMVTerminalSerialPort_private::destroyed,
            thread, &QThread::quit);

    connect(thread, &QThread::finished,
            thread, &QThread::deleteLater);

    thread->start();
}

OpenMVTerminalUDPPort_private::OpenMVTerminalUDPPort_private(QObject *parent) : QObject(parent)
{
    m_port = Q_NULLPTR;
}

void OpenMVTerminalUDPPort_private::open(const QString &hostName, int port)
{
    if(m_port)
    {
        delete m_port;
    }

    m_port = new QUdpSocket(this);

    connect(m_port, &QSerialPort::readyRead, this, [this] {
        emit readBytes(m_port->readAll());
    });

    m_port->connectToHost(hostName, port);

    if(!m_port->waitForConnected())
    {
        emit openResult(m_port->errorString());
        delete m_port;
        m_port = Q_NULLPTR;
    }
    else
    {
        emit openResult(QString());
    }
}

void OpenMVTerminalUDPPort_private::writeBytes(const QByteArray &data)
{
    if(m_port)
    {
        if((m_port->write(data) != data.size()) || (!m_port->flush()))
        {
            delete m_port;
            m_port = Q_NULLPTR;
        }
    }
}

OpenMVTerminalUDPPort::OpenMVTerminalUDPPort(QObject *parent) : OpenMVTerminalPort(parent)
{
    QThread *thread = new QThread;
    OpenMVTerminalUDPPort_private* port = new OpenMVTerminalUDPPort_private;
    port->moveToThread(thread);

    connect(this, &OpenMVTerminalUDPPort::open,
            port, &OpenMVTerminalUDPPort_private::open);

    connect(port, &OpenMVTerminalUDPPort_private::openResult,
            this, &OpenMVTerminalUDPPort::openResult);

    connect(this, &OpenMVTerminalUDPPort::writeBytes,
            port, &OpenMVTerminalUDPPort_private::writeBytes);

    connect(port, &OpenMVTerminalUDPPort_private::readBytes,
            this, &OpenMVTerminalUDPPort::readBytes);

    connect(this, &OpenMVTerminalUDPPort::destroyed,
            port, &OpenMVTerminalUDPPort_private::deleteLater);

    connect(port, &OpenMVTerminalUDPPort_private::destroyed,
            thread, &QThread::quit);

    connect(thread, &QThread::finished,
            thread, &QThread::deleteLater);

    thread->start();
}

OpenMVTerminalTCPPort_private::OpenMVTerminalTCPPort_private(QObject *parent) : QObject(parent)
{
    m_port = Q_NULLPTR;
}

void OpenMVTerminalTCPPort_private::open(const QString &hostName, int port)
{
    if(m_port)
    {
        delete m_port;
    }

    m_port = new QTcpSocket(this);

    connect(m_port, &QSerialPort::readyRead, this, [this] {
        emit readBytes(m_port->readAll());
    });

    m_port->connectToHost(hostName, port);

    if(!m_port->waitForConnected())
    {
        emit openResult(m_port->errorString());
        delete m_port;
        m_port = Q_NULLPTR;
    }
    else
    {
        emit openResult(QString());
    }
}

void OpenMVTerminalTCPPort_private::writeBytes(const QByteArray &data)
{
    if(m_port)
    {
        if((m_port->write(data) != data.size()) || (!m_port->flush()))
        {
            delete m_port;
            m_port = Q_NULLPTR;
        }
    }
}

OpenMVTerminalTCPPort::OpenMVTerminalTCPPort(QObject *parent) : OpenMVTerminalPort(parent)
{
    QThread *thread = new QThread;
    OpenMVTerminalTCPPort_private* port = new OpenMVTerminalTCPPort_private;
    port->moveToThread(thread);

    connect(this, &OpenMVTerminalTCPPort::open,
            port, &OpenMVTerminalTCPPort_private::open);

    connect(port, &OpenMVTerminalTCPPort_private::openResult,
            this, &OpenMVTerminalTCPPort::openResult);

    connect(this, &OpenMVTerminalTCPPort::writeBytes,
            port, &OpenMVTerminalTCPPort_private::writeBytes);

    connect(port, &OpenMVTerminalTCPPort_private::readBytes,
            this, &OpenMVTerminalTCPPort::readBytes);

    connect(this, &OpenMVTerminalTCPPort::destroyed,
            port, &OpenMVTerminalTCPPort_private::deleteLater);

    connect(port, &OpenMVTerminalTCPPort_private::destroyed,
            thread, &QThread::quit);

    connect(thread, &QThread::finished,
            thread, &QThread::deleteLater);

    thread->start();
}
