// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include <cstdlib>

int main()
{
    int *p = new int;
    delete p;
    delete p;
    return 0;
}
