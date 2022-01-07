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

#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))

#include <QtCore/qdebug.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>
#include "framelesswindowsmanager.h"
#include "utilities.h"

FRAMELESSHELPER_BEGIN_NAMESPACE

FramelessHelper::FramelessHelper(QObject *parent) : QObject(parent) {}

void FramelessHelper::removeWindowFrame(QWindow *window)
{
    Q_ASSERT(window);
    if (!window) {
        return;
    }
    window->setFlags(window->flags() | Qt::FramelessWindowHint);
    window->installEventFilter(this);
    window->setProperty(Constants::kFramelessModeFlag, true);
}

void FramelessHelper::bringBackWindowFrame(QWindow *window)
{
    Q_ASSERT(window);
    if (!window) {
        return;
    }
    window->removeEventFilter(this);
    window->setFlags(window->flags() & ~Qt::FramelessWindowHint);
    window->setProperty(Constants::kFramelessModeFlag, false);
}

bool FramelessHelper::eventFilter(QObject *object, QEvent *event)
{
    Q_ASSERT(object);
    Q_ASSERT(event);
    if (!object || !event) {
        return false;
    }
    // Only monitor window events.
    if (!object->isWindowType()) {
        return false;
    }
    const QEvent::Type type = event->type();
    // We are only interested in mouse events.
    if ((type != QEvent::MouseButtonDblClick) && (type != QEvent::MouseButtonPress)
            && (type != QEvent::MouseMove) && (type != QEvent::MouseButtonRelease)) {
        return false;
    }
    const auto window = qobject_cast<QWindow *>(object);
    const int resizeBorderThickness = FramelessWindowsManager::getResizeBorderThickness(window);
    const int titleBarHeight = FramelessWindowsManager::getTitleBarHeight(window);
    const bool resizable = FramelessWindowsManager::getResizable(window);
    const int windowWidth = window->width();
    const auto mouseEvent = static_cast<QMouseEvent *>(event);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    const QPoint localMousePosition = mouseEvent->position().toPoint();
#else
    const QPoint localMousePosition = mouseEvent->windowPos().toPoint();
#endif
     const Qt::Edges edges = [window, resizeBorderThickness, windowWidth, &localMousePosition] {
        const int windowHeight = window->height();
        if (localMousePosition.y() <= resizeBorderThickness) {
            if (localMousePosition.x() <= resizeBorderThickness) {
                return Qt::TopEdge | Qt::LeftEdge;
            }
            if (localMousePosition.x() >= (windowWidth - resizeBorderThickness)) {
                return Qt::TopEdge | Qt::RightEdge;
            }
            return Qt::Edges{Qt::TopEdge};
        }
        if (localMousePosition.y() >= (windowHeight - resizeBorderThickness)) {
            if (localMousePosition.x() <= resizeBorderThickness) {
                return Qt::BottomEdge | Qt::LeftEdge;
            }
            if (localMousePosition.x() >= (windowWidth - resizeBorderThickness)) {
                return Qt::BottomEdge | Qt::RightEdge;
            }
            return Qt::Edges{Qt::BottomEdge};
        }
        if (localMousePosition.x() <= resizeBorderThickness) {
            return Qt::Edges{Qt::LeftEdge};
        }
        if (localMousePosition.x() >= (windowWidth - resizeBorderThickness)) {
            return Qt::Edges{Qt::RightEdge};
        }
        return Qt::Edges{};
    } ();
    const bool hitTestVisible = Utilities::isHitTestVisible(window);
    bool isInTitlebarArea = false;
    if ((window->windowState() == Qt::WindowMaximized)
            || (window->windowState() == Qt::WindowFullScreen)) {
        isInTitlebarArea = (localMousePosition.y() >= 0)
                && (localMousePosition.y() <= titleBarHeight)
                && (localMousePosition.x() >= 0)
                && (localMousePosition.x() <= windowWidth)
                && !hitTestVisible;
    }
    if (window->windowState() == Qt::WindowNoState) {
        isInTitlebarArea = (localMousePosition.y() > resizeBorderThickness)
                && (localMousePosition.y() <= titleBarHeight)
                && (localMousePosition.x() > resizeBorderThickness)
                && (localMousePosition.x() < (windowWidth - resizeBorderThickness))
                && !hitTestVisible;
    }

    // Determine if the mouse click occurred in the title bar

    static bool titlebarClicked = false;
    static QPoint dragGlobalPos;
    if (type == QEvent::MouseButtonPress) {
        if (isInTitlebarArea)
            titlebarClicked = true;
        else
            titlebarClicked = false;
        if (mouseEvent->button() == Qt::LeftButton)
        {
            dragGlobalPos = mouseEvent->globalPos();
        }
    }

    static QPoint resizeGlobalPos;
    static QRect origRect;
    static Qt::Edges resizeEdges;
    if (type == QEvent::MouseButtonDblClick) {
        if (isInTitlebarArea) {
            if (window->windowState() == Qt::WindowState::WindowMaximized || window->windowState() == Qt::WindowState::WindowFullScreen) {
                window->setWindowState(Qt::WindowState::WindowNoState);
            } else if(window->windowState() == Qt::WindowState::WindowNoState){
                window->setWindowState(Qt::WindowState::WindowMaximized);
            }
            window->setCursor(Qt::ArrowCursor);
        }
    } else if (type == QEvent::MouseMove) {
        // Display resize indicators
        static bool cursorChanged = false;
        if ((window->windowState() == Qt::WindowState::WindowNoState) && resizable) {
            if (((edges & Qt::TopEdge) && (edges & Qt::LeftEdge))
                    || ((edges & Qt::BottomEdge) && (edges & Qt::RightEdge))) {
                window->setCursor(Qt::SizeFDiagCursor);
                cursorChanged = true;
            } else if (((edges & Qt::TopEdge) && (edges & Qt::RightEdge))
                       || ((edges & Qt::BottomEdge) && (edges & Qt::LeftEdge))) {
                window->setCursor(Qt::SizeBDiagCursor);
                cursorChanged = true;
            } else if ((edges & Qt::TopEdge) || (edges & Qt::BottomEdge)) {
                window->setCursor(Qt::SizeVerCursor);
                cursorChanged = true;
            } else if ((edges & Qt::LeftEdge) || (edges & Qt::RightEdge)) {
                window->setCursor(Qt::SizeHorCursor);
                cursorChanged = true;
            } else {
                if (cursorChanged) {
                    window->setCursor(Qt::ArrowCursor);
                    cursorChanged = false;
                }
            }
        }

        if ((mouseEvent->buttons() & Qt::LeftButton) && titlebarClicked) {
            window->unsetCursor();
            if(window->windowState() == Qt::WindowState::WindowMaximized || window->windowState() == Qt::WindowState::WindowFullScreen)
            {
                window->setWindowState(Qt::WindowState::WindowNoState);
                window->setPosition(QPoint(dragGlobalPos.x() - window->geometry().width()/2, 0));
            }
            window->setPosition(window->position() + (mouseEvent->globalPos() - dragGlobalPos));
            dragGlobalPos = mouseEvent->globalPos();
        }
        if(!resizeGlobalPos.isNull())
        {
            int y0 = (mouseEvent->globalPos() - resizeGlobalPos).y();
            int x0 = (mouseEvent->globalPos() - resizeGlobalPos).x();
            int minWidth = window->minimumWidth();
            int minHeight = window->minimumHeight();
            if(resizeEdges & Qt::LeftEdge)
            {
                if(minWidth > origRect.width() - x0)
                {
                    x0 = origRect.width() - minWidth;
                }
            }
            if(resizeEdges & Qt::TopEdge)
            {
                if(minHeight > origRect.height() - y0)
                {
                    y0 = origRect.height() - minHeight;
                }
            }
            if(resizeEdges & Qt::RightEdge)
            {
                if(minWidth > origRect.width() + x0)
                {
                    x0 = minWidth - origRect.width();
                }
            }
            if(resizeEdges & Qt::BottomEdge)
            {
                if(minHeight > origRect.height() + y0)
                {
                    y0 = minHeight - origRect.height();
                }
            }
            if((resizeEdges & Qt::TopEdge) && (resizeEdges & Qt::LeftEdge))
            {
                window->setGeometry(origRect.adjusted(x0,y0,0,0));
            }
            else if((resizeEdges & Qt::TopEdge) && (resizeEdges & Qt::RightEdge))
            {
                window->setGeometry(origRect.adjusted(0,y0,x0,0));
            }
            else if((resizeEdges & Qt::BottomEdge) && (resizeEdges & Qt::RightEdge))
            {
                window->setGeometry(origRect.adjusted(0,0,x0,y0));

            }
            else if((resizeEdges & Qt::BottomEdge) && (resizeEdges & Qt::LeftEdge))
            {
                window->setGeometry(origRect.adjusted(x0,0,0,y0));
            }
            else if(resizeEdges & Qt::TopEdge)
            {
                window->setGeometry(origRect.adjusted(0,y0,0,0));
            }
            else if(resizeEdges & Qt::LeftEdge)
            {
                window->setGeometry(origRect.adjusted(x0,0,0,0));
            }
            else if(resizeEdges & Qt::BottomEdge)
            {
                window->setGeometry(origRect.adjusted(0,0,0,y0));
            }
            else if(resizeEdges & Qt::RightEdge)
            {
                window->setGeometry(origRect.adjusted(0,0,x0,0));
            }
        }

    } else if (type == QEvent::MouseButtonPress) {
        if (edges != Qt::Edges{}) {
            if ((window->windowState() == Qt::WindowState::WindowNoState) && !hitTestVisible && resizable) {
                resizeGlobalPos = mouseEvent->globalPos();
                origRect = window->geometry();
                resizeEdges = edges;
            }
        }
    }
    if(type == QEvent::MouseButtonRelease)
    {
        resizeGlobalPos = QPoint();
        origRect = QRect();
        resizeEdges = Qt::Edges{};
    }
    return false;
}

FRAMELESSHELPER_END_NAMESPACE

#endif
