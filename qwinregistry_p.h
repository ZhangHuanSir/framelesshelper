/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "framelesshelper_global.h"
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
#include <QtCore/private/qwinregistry_p.h>
#else // QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#include <QtCore/qpair.h>
#include <QtCore/qstring.h>
#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

class QWinRegistryKey
{
public:
    Q_DISABLE_COPY(QWinRegistryKey)
    QWinRegistryKey();
    explicit QWinRegistryKey(HKEY parentHandle, QStringView subKey,
                             REGSAM permissions = KEY_READ, REGSAM access = 0);
    ~QWinRegistryKey();

    QWinRegistryKey(QWinRegistryKey &&other) noexcept
        : m_key(exchange(other.m_key, nullptr)) {}
    QWinRegistryKey &operator=(QWinRegistryKey &&other) noexcept {
        QWinRegistryKey moved(std::move(other));
        swap(moved);
        return *this;
    }
    template<class _Ty,
    class _Other = _Ty> inline
    _Ty exchange(_Ty& _Val, _Other&& _New_val)
    {	// assign _New_val to _Val, return previous _Val
        _Ty _Old_val = std::move(_Val);
        _Val = std::forward<_Other>(_New_val);
        return (_Old_val);
    }
    void swap(QWinRegistryKey &other) noexcept { std::swap(m_key, other.m_key); }

    bool isValid() const { return m_key != nullptr; }
    operator HKEY() const { return m_key; }
    void close();

    QString stringValue(QStringView subKey) const;
    QPair<DWORD, bool> dwordValue(QStringView subKey) const;

private:
    HKEY m_key;
};

QT_END_NAMESPACE

#endif // QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
