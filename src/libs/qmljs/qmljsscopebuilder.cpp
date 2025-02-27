// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmljsscopebuilder.h"

#include "qmljsbind.h"
#include "qmljsevaluate.h"
#include "qmljsscopechain.h"
#include "qmljsutils.h"
#include "parser/qmljsast_p.h"

#include <utils/qtcassert.h>

using namespace QmlJS;
using namespace QmlJS::AST;

ScopeBuilder::ScopeBuilder(ScopeChain *scopeChain)
    : _scopeChain(scopeChain)
{
}

ScopeBuilder::~ScopeBuilder()
{
}

void ScopeBuilder::push(AST::Node *node)
{
    _nodes += node;

    // QML scope object
    Node *qmlObject = cast<UiObjectDefinition *>(node);
    if (! qmlObject)
        qmlObject = cast<UiObjectBinding *>(node);
    if (qmlObject) {
        // save the previous scope objects
        _qmlScopeObjects.push(_scopeChain->qmlScopeObjects());
        setQmlScopeObject(qmlObject);
    }

    // JS signal handler scope
    if (UiScriptBinding *script = cast<UiScriptBinding *>(node)) {
        QString name;
        if (script->qualifiedId) {
            name = script->qualifiedId->name.toString();
            if (!_scopeChain->qmlScopeObjects().isEmpty()
                    && name.startsWith(QLatin1String("on"))
                    && !script->qualifiedId->next) {
                const ObjectValue *owner = nullptr;
                const Value *value = nullptr;
                // try to find the name on the scope objects
                foreach (const ObjectValue *scope, _scopeChain->qmlScopeObjects()) {
                    value = scope->lookupMember(name, _scopeChain->context(), &owner);
                    if (value)
                        break;
                }
                // signals defined in QML
                if (const ASTSignal *astsig = value_cast<ASTSignal>(value)) {
                    _scopeChain->appendJsScope(astsig->bodyScope());
                // signals defined in C++
                } else if (const CppComponentValue *qmlObject = value_cast<CppComponentValue>(owner)) {
                    if (const ObjectValue *scope = qmlObject->signalScope(name))
                        _scopeChain->appendJsScope(scope);
                }
            }
        }
    }

    // JS scopes
    switch (node->kind) {
    case Node::Kind_UiScriptBinding:
    case Node::Kind_FunctionDeclaration:
    case Node::Kind_FunctionExpression:
    case Node::Kind_UiPublicMember:
    {
        ObjectValue *scope = _scopeChain->document()->bind()->findAttachedJSScope(node);
        if (scope)
            _scopeChain->appendJsScope(scope);
        break;
    }
    default:
        break;
    }
}

void ScopeBuilder::push(const QList<AST::Node *> &nodes)
{
    foreach (Node *node, nodes)
        push(node);
}

void ScopeBuilder::pop()
{
    Node *toRemove = _nodes.last();
    _nodes.removeLast();

    // JS scopes
    switch (toRemove->kind) {
    case Node::Kind_UiScriptBinding:
    case Node::Kind_FunctionDeclaration:
    case Node::Kind_FunctionExpression:
    case Node::Kind_UiPublicMember:
    {
        ObjectValue *scope = _scopeChain->document()->bind()->findAttachedJSScope(toRemove);
        if (scope) {
            QList<const ObjectValue *> jsScopes = _scopeChain->jsScopes();
            if (!jsScopes.isEmpty()) {
                jsScopes.removeLast();
                _scopeChain->setJsScopes(jsScopes);
            }
        }
        break;
    }
    default:
        break;
    }

    // QML scope object
    if (cast<UiObjectDefinition *>(toRemove) || cast<UiObjectBinding *>(toRemove)) {
        // restore the previous scope objects
        QTC_ASSERT(!_qmlScopeObjects.isEmpty(), return);
        _scopeChain->setQmlScopeObjects(_qmlScopeObjects.pop());
    }
}

void ScopeBuilder::setQmlScopeObject(Node *node)
{
    QList<const ObjectValue *> qmlScopeObjects;
    if (_scopeChain->document()->bind()->isGroupedPropertyBinding(node)) {
        UiObjectDefinition *definition = cast<UiObjectDefinition *>(node);
        if (!definition)
            return;
        const Value *v = scopeObjectLookup(definition->qualifiedTypeNameId);
        if (!v)
            return;
        const ObjectValue *object = v->asObjectValue();
        if (!object)
            return;

        qmlScopeObjects += object;
        _scopeChain->setQmlScopeObjects(qmlScopeObjects);
        return;
    }

    const ObjectValue *scopeObject = _scopeChain->document()->bind()->findQmlObject(node);
    if (scopeObject)
        qmlScopeObjects += scopeObject;
    else
        return; // Probably syntax errors, where we're working with a "recovered" AST.

    // check if the object has a Qt.ListElement or Qt.Connections ancestor
    // ### allow only signal bindings for Connections
    PrototypeIterator iter(scopeObject, _scopeChain->context());
    iter.next();
    while (iter.hasNext()) {
        const ObjectValue *prototype = iter.next();
        if (const CppComponentValue *qmlMetaObject = value_cast<CppComponentValue>(prototype)) {
            if ((qmlMetaObject->className() == "ListElement"
                 || qmlMetaObject->className() == "Connections")
                && (qmlMetaObject->moduleName() == "Qt" || qmlMetaObject->moduleName() == "QtQml"
                    || qmlMetaObject->moduleName() == "QtQuick")) {
                qmlScopeObjects.clear();
                break;
            }
        }
    }

    // check if the object has a Qt.PropertyChanges ancestor
    const ObjectValue *prototype = scopeObject->prototype(_scopeChain->context());
    prototype = isPropertyChangesObject(_scopeChain->context(), prototype);
    // find the target script binding
    if (prototype) {
        UiObjectInitializer *initializer = initializerOfObject(node);
        if (initializer) {
            for (UiObjectMemberList *m = initializer->members; m; m = m->next) {
                if (UiScriptBinding *scriptBinding = cast<UiScriptBinding *>(m->member)) {
                    if (scriptBinding->qualifiedId
                            && scriptBinding->qualifiedId->name == QLatin1String("target")
                            && ! scriptBinding->qualifiedId->next) {
                        Evaluate evaluator(_scopeChain);
                        const Value *targetValue = evaluator(scriptBinding->statement);

                        if (const ObjectValue *target = value_cast<ObjectValue>(targetValue))
                            qmlScopeObjects.prepend(target);
                        else
                            qmlScopeObjects.clear();
                    }
                }
            }
        }
    }

    _scopeChain->setQmlScopeObjects(qmlScopeObjects);
}

const Value *ScopeBuilder::scopeObjectLookup(AST::UiQualifiedId *id)
{
    // do a name lookup on the scope objects
    const Value *result = nullptr;
    foreach (const ObjectValue *scopeObject, _scopeChain->qmlScopeObjects()) {
        const ObjectValue *object = scopeObject;
        for (UiQualifiedId *it = id; it; it = it->next) {
            if (it->name.isEmpty())
                return nullptr;
            result = object->lookupMember(it->name.toString(), _scopeChain->context());
            if (!result)
                break;
            if (it->next) {
                object = result->asObjectValue();
                if (!object) {
                    result = nullptr;
                    break;
                }
            }
        }
        if (result)
            break;
    }

    return result;
}


const ObjectValue *ScopeBuilder::isPropertyChangesObject(const ContextPtr &context,
                                                   const ObjectValue *object)
{
    PrototypeIterator iter(object, context);
    while (iter.hasNext()) {
        const ObjectValue *prototype = iter.next();
        if (const CppComponentValue *qmlMetaObject = value_cast<CppComponentValue>(prototype)) {
            if (qmlMetaObject->className() == QLatin1String("PropertyChanges")
                    && (qmlMetaObject->moduleName() == QLatin1String("Qt")
                        || qmlMetaObject->moduleName() == QLatin1String("QtQuick")))
                return prototype;
        }
    }
    return nullptr;
}
