// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "debuggerprotocol.h"

#include <utils/treemodel.h>

#include <QMetaType>

#include <vector>

namespace Debugger::Internal {

class GdbMi;

class WatchItem : public Utils::TypedTreeItem<WatchItem, WatchItem>
{
public:
    WatchItem();

    void parse(const GdbMi &input, bool maySort);

    bool isLocal()   const;
    bool isWatcher() const;
    bool isInspect() const;

    QString expression() const;
    QString sourceExpression() const;
    QString realName() const;
    QString internalName() const;
    QString toToolTip() const;

    QVariant editValue() const;
    int editType() const;

    static const qint64 InvalidId = -1;

    void setHasChildren(bool c)   { wantsChildren = c; }

    bool isValid()   const { return !iname.isEmpty(); }
    bool isVTablePointer() const;
    int guessSize() const;

    void setError(const QString &);
    void setValue(const QString &);

    QString toString() const;

    static QString shadowedName(const QString &name, int seen);

    QString hexAddress() const;
    QString key() const { return address ? hexAddress() : iname; }

public:
    qint64          id;            // Token for the engine for internal mapping
    QString         iname;         // Internal name sth like 'local.baz.public.a'
    QString         exp;           // The expression
    QString         name;          // Displayed name
    QString         value;         // Displayed value
    QString         editvalue;     // Displayed value
    QString         editformat;    // Format of displayed value
    DebuggerEncoding editencoding; // Encoding of displayed value
    QString         type;          // Type for further processing
    quint64         address;       // Displayed address of the actual object
    quint64         origaddr;      // Address of the pointer referencing this item (gdb auto-deref)
    uint            size;          // Size
    uint            bitpos;        // Position within bit fields
    uint            bitsize;       // Size in case of bit fields
    int             elided;        // Full size if value was cut off, -1 if cut on unknown size, 0 otherwise
    int             arrayIndex;    // -1 if not an array member
    uchar           sortGroup;     // 0 - ordinary member, 1 - vptr, 2 - base class
    bool            wantsChildren;
    bool            valueEnabled;  // Value will be enabled or not
    bool            valueEditable; // Value will be editable
    uint            autoDerefCount; // number of levels of automatic dereferencing that has taken place (for pointer types)
    bool            outdated;      // \internal item is to be removed.
    double          time = 0;      // Time used on the dumper side to produce this item

private:
    void parseHelper(const GdbMi &input, bool maySort);
};

} // Debugger::Internal
