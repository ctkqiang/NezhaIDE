/****************************************************************************
** Meta object code from reading C++ file 'hydra.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/tools/hydra.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'hydra.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN8NezhaIDE5Tools12HydraServiceE_t {};
} // unnamed namespace

template <> constexpr inline auto NezhaIDE::Tools::HydraService::qt_create_metaobjectdata<qt_meta_tag_ZN8NezhaIDE5Tools12HydraServiceE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "NezhaIDE::Tools::HydraService",
        "modulesProbed",
        "",
        "usernameDatasetChanged",
        "entryCount",
        "passwordDatasetChanged",
        "provenance",
        "githubImportFinished",
        "error",
        "runStateChanged",
        "NezhaIDE::Tools::HydraState",
        "state",
        "logLine",
        "line",
        "runFinished",
        "success",
        "attemptCount",
        "foundCount",
        "summary"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'modulesProbed'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'usernameDatasetChanged'
        QtMocHelpers::SignalData<void(int)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 4 },
        }}),
        // Signal 'passwordDatasetChanged'
        QtMocHelpers::SignalData<void(int, const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 4 }, { QMetaType::QString, 6 },
        }}),
        // Signal 'githubImportFinished'
        QtMocHelpers::SignalData<void(int, const QString &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 4 }, { QMetaType::QString, 8 },
        }}),
        // Signal 'runStateChanged'
        QtMocHelpers::SignalData<void(NezhaIDE::Tools::HydraState)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 11 },
        }}),
        // Signal 'logLine'
        QtMocHelpers::SignalData<void(const QString &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 13 },
        }}),
        // Signal 'runFinished'
        QtMocHelpers::SignalData<void(bool, int, int, const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 15 }, { QMetaType::Int, 16 }, { QMetaType::Int, 17 }, { QMetaType::QString, 18 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<HydraService, qt_meta_tag_ZN8NezhaIDE5Tools12HydraServiceE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject NezhaIDE::Tools::HydraService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8NezhaIDE5Tools12HydraServiceE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8NezhaIDE5Tools12HydraServiceE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8NezhaIDE5Tools12HydraServiceE_t>.metaTypes,
    nullptr
} };

void NezhaIDE::Tools::HydraService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<HydraService *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->modulesProbed(); break;
        case 1: _t->usernameDatasetChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->passwordDatasetChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->githubImportFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 4: _t->runStateChanged((*reinterpret_cast<std::add_pointer_t<NezhaIDE::Tools::HydraState>>(_a[1]))); break;
        case 5: _t->logLine((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->runFinished((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[4]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (HydraService::*)()>(_a, &HydraService::modulesProbed, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (HydraService::*)(int )>(_a, &HydraService::usernameDatasetChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (HydraService::*)(int , const QString & )>(_a, &HydraService::passwordDatasetChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (HydraService::*)(int , const QString & )>(_a, &HydraService::githubImportFinished, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (HydraService::*)(NezhaIDE::Tools::HydraState )>(_a, &HydraService::runStateChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (HydraService::*)(const QString & )>(_a, &HydraService::logLine, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (HydraService::*)(bool , int , int , const QString & )>(_a, &HydraService::runFinished, 6))
            return;
    }
}

const QMetaObject *NezhaIDE::Tools::HydraService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NezhaIDE::Tools::HydraService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8NezhaIDE5Tools12HydraServiceE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int NezhaIDE::Tools::HydraService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void NezhaIDE::Tools::HydraService::modulesProbed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void NezhaIDE::Tools::HydraService::usernameDatasetChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void NezhaIDE::Tools::HydraService::passwordDatasetChanged(int _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void NezhaIDE::Tools::HydraService::githubImportFinished(int _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}

// SIGNAL 4
void NezhaIDE::Tools::HydraService::runStateChanged(NezhaIDE::Tools::HydraState _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void NezhaIDE::Tools::HydraService::logLine(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void NezhaIDE::Tools::HydraService::runFinished(bool _t1, int _t2, int _t3, const QString & _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2, _t3, _t4);
}
QT_WARNING_POP
