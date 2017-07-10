/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "elidinglabel.h"
#include <QFontMetrics>
#include <QPainter>
#include <QStyle>
//OPENMV-DIFF//
#include <QStyleOption>
#include <utils/hostosinfo.h>
#include <utils/theme/theme.h>
//OPENMV-DIFF//

/*!
    \class Utils::ElidingLabel

    \brief The ElidingLabel class is a label suitable for displaying elided
    text.
*/

namespace Utils {

ElidingLabel::ElidingLabel(QWidget *parent)
    : QLabel(parent), m_elideMode(Qt::ElideRight)
{
    //OPENMV-DIFF//
    //setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred, QSizePolicy::Label));
    //OPENMV-DIFF//
}

ElidingLabel::ElidingLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent), m_elideMode(Qt::ElideRight)
{
    //OPENMV-DIFF//
    //setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred, QSizePolicy::Label));
    //OPENMV-DIFF//
}

Qt::TextElideMode ElidingLabel::elideMode() const
{
    return m_elideMode;
}

void ElidingLabel::setElideMode(const Qt::TextElideMode &elideMode)
{
    m_elideMode = elideMode;
    update();
}

void ElidingLabel::paintEvent(QPaintEvent *)
{
    const int m = margin();
    QRect contents = contentsRect().adjusted(m, m, -m, -m);
    QFontMetrics fm = fontMetrics();
    QString txt = text();
    if (txt.length() > 4 && fm.width(txt) > contents.width()) {
        //OPENMV-DIFF//
        //setToolTip(txt);
        //OPENMV-DIFF//
        txt = fm.elidedText(txt, m_elideMode, contents.width());
    } else {
        //OPENMV-DIFF//
        //setToolTip(QString());
        //OPENMV-DIFF//
    }
    int flags = QStyle::visualAlignment(layoutDirection(), alignment()) | Qt::TextSingleLine;

    QPainter painter(this);
    drawFrame(&painter);
    painter.drawText(contents, flags, txt);
}

//OPENMV-DIFF//
ElidingToolButton::ElidingToolButton(QWidget *parent)
    : QToolButton(parent), m_elideMode(Qt::ElideRight)
{

}

Qt::TextElideMode ElidingToolButton::elideMode() const
{
    return m_elideMode;
}

void ElidingToolButton::setElideMode(const Qt::TextElideMode &elideMode)
{
    m_elideMode = elideMode;
    update();
}

void ElidingToolButton::paintEvent(QPaintEvent *)
{
    const QFontMetrics fm = fontMetrics();
    const int baseLine = (height() - fm.height() + 1) / 2 + fm.ascent();

    QPainter p(this);

    QStyleOption styleOption;
    styleOption.initFrom(this);
    const bool hovered = !HostOsInfo::isMacHost() && (styleOption.state & QStyle::State_MouseOver);

    if(isEnabled())
    {
        Theme::Color c = Theme::BackgroundColorDark;

        if (hovered)
            c = Theme::BackgroundColorHover;
        else if (isDown() || isChecked())
            c = Theme::BackgroundColorSelected;

        if (c != Theme::BackgroundColorDark)
            p.fillRect(rect(), creatorTheme()->color(c));

        p.setFont(font());
        p.setPen(creatorTheme()->color(Theme::OutputPaneToggleButtonTextColorChecked));

        if (!isChecked())
            p.setPen(creatorTheme()->color(Theme::OutputPaneToggleButtonTextColorUnchecked));
    }

    p.drawText(4, baseLine, fm.elidedText(text(), Qt::ElideRight, width() - 5));
}
//OPENMV-DIFF//

} // namespace Utils
