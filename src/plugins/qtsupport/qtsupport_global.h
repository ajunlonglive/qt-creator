// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#if defined(QTSUPPORT_LIBRARY)
#  define QTSUPPORT_EXPORT Q_DECL_EXPORT
#elif defined(QTSUPPORT_STATIC_LIBRARY)
#  define QTSUPPORT_EXPORT
#else
#  define QTSUPPORT_EXPORT Q_DECL_IMPORT
#endif
