/*
 * MIT License
 *
 * Copyright (C) 2021 by wangwenx190 (Yuhang Zhao)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "framelesshelper.h"

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))

#include <QtCore/qdebug.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>
#include "framelesswindowsmanager.h"
#include "utilities.h"

FRAMELESSHELPER_BEGIN_NAMESPACE

FramelessHelper::FramelessHelper(QWindow *window)
    : QObject(window)
    , m_window(window)
{
    Q_ASSERT(window != nullptr && window->isTopLevel());
}

/*!
    Setup the window, make it frameless.
 */
void FramelessHelper::install()
{
    QRect origRect = m_window->geometry();
    m_origWindowFlags = m_window->flags();

#ifdef Q_OS_MAC
    m_window->setFlags(Qt::Window);
#else
    m_window->setFlags(m_origWindowFlags | Qt::FramelessWindowHint);
#endif

    m_window->setGeometry(origRect);
    resizeWindow(origRect.size());
}

/*!
    Restore the window to its original state
 */
void FramelessHelper::uninstall()
{
    m_window->setFlags(m_origWindowFlags);
    m_origWindowFlags = Qt::WindowFlags();
    resizeWindow(QSize());
}

/*!
    Resize non-client area
 */
void FramelessHelper::resizeWindow(const QSize& windowSize)
{
    if (windowSize == this->windowSize())
        return;

    setWindowSize(windowSize);
}

QRect FramelessHelper::titleBarRect()
{
    return QRect(0, 0, windowSize().width(), titleBarHeight());
}

QRect FramelessHelper::clientRect()
{
    QRect rect(0, 0, windowSize().width(), windowSize().height());
    rect = rect.adjusted(
        resizeBorderThickness(), titleBarHeight(),
        -resizeBorderThickness(), -resizeBorderThickness()
    );
    return rect;
}

QRegion FramelessHelper::nonClientRegion()
{
    QRegion region(QRect(QPoint(0, 0), windowSize()));
    region -= clientRect();
    return region;
}

bool FramelessHelper::isInTitlebarArea(const QPoint& pos)
{
    return nonClientRegion().contains(pos);
}

Qt::WindowFrameSection FramelessHelper::mapPosToFrameSection(const QPoint& pos)
{
    int border = 0;

    // TODO: get system default resize border
    const int sysBorder = Utilities::getSystemMetric(window(), SystemMetric::ResizeBorderThickness, false);

    Qt::WindowStates states = window()->windowState();
    if (!(states & Qt::WindowMaximized) && !(states & Qt::WindowFullScreen))
    {
        border = resizeBorderThickness();
        border = qMin(border, sysBorder);
    }

    QRect windowRect(0, 0, windowSize().width(), windowSize().height());

    if (windowRect.contains(pos))
    {
        QPoint mappedPos = pos - windowRect.topLeft();
        if (QRect(0, 0, border, border).contains(mappedPos))
            return Qt::TopLeftSection;

        if (QRect(border, 0, windowRect.width() - border * 2, border).contains(mappedPos))
            return Qt::TopSection;

        if (QRect(windowRect.width() - border, 0, border, border).contains(mappedPos))
            return Qt::TopRightSection;

        if (QRect(windowRect.width() - border, border, border, windowRect.height() - border * 2).contains(mappedPos))
            return Qt::RightSection;

        if (QRect(windowRect.width() - border, windowRect.height() - border, border, border).contains(mappedPos))
            return Qt::BottomRightSection;

        if (QRect(border, windowRect.height() - border, windowRect.width() - border * 2, border).contains(mappedPos))
            return Qt::BottomSection;

        if (QRect(0, windowRect.height() - border, border, border).contains(mappedPos))
            return Qt::BottomLeftSection;

        if (QRect(0, border, border, windowRect.height() - border * 2).contains(mappedPos))
            return Qt::LeftSection;

        if (isInTitlebarArea(pos))
            return Qt::TitleBarArea;
    }

    return Qt::NoSection;
}

bool FramelessHelper::isHoverResizeHandler()
{
    return m_hoveredFrameSection == Qt::LeftSection ||
        m_hoveredFrameSection == Qt::RightSection ||
        m_hoveredFrameSection == Qt::TopSection ||
        m_hoveredFrameSection == Qt::BottomSection ||
        m_hoveredFrameSection == Qt::TopLeftSection ||
        m_hoveredFrameSection == Qt::TopRightSection ||
        m_hoveredFrameSection == Qt::BottomLeftSection ||
        m_hoveredFrameSection == Qt::BottomRightSection;
}

QCursor FramelessHelper::cursorForFrameSection(Qt::WindowFrameSection frameSection)
{
    Qt::CursorShape cursor = Qt::ArrowCursor;

    switch (frameSection)
    {
    case Qt::LeftSection:
    case Qt::RightSection:
        cursor = Qt::SizeHorCursor;
        break;
    case Qt::BottomSection:
    case Qt::TopSection:
        cursor = Qt::SizeVerCursor;
        break;
    case Qt::TopLeftSection:
    case Qt::BottomRightSection:
        cursor = Qt::SizeFDiagCursor;
        break;
    case Qt::TopRightSection:
    case Qt::BottomLeftSection:
        cursor = Qt::SizeBDiagCursor;
        break;
    case Qt::TitleBarArea:
        cursor = Qt::ArrowCursor;
        break;
    default:
        break;
    }

    return QCursor(cursor);
}

void FramelessHelper::setCursor(const QCursor& cursor)
{
    m_window->setCursor(cursor);
    m_cursorChanged = true;
}

void FramelessHelper::unsetCursor()
{
    if (!m_cursorChanged)
        return;

    m_window->unsetCursor();
    m_cursorChanged = false;
}

void FramelessHelper::updateCursor()
{
    if (isHoverResizeHandler())
        setCursor(cursorForFrameSection(m_hoveredFrameSection));
}

bool FramelessHelper::eventFilter(QObject *object, QEvent *event)
{

}

FRAMELESSHELPER_END_NAMESPACE

#endif
