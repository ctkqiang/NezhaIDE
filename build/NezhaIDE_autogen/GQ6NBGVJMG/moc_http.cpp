/****************************************************************************
** Meta object code from reading C++ file 'http.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/services/http.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'http.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN8NezhaIDE8Services4HTTP17HttpClientServiceE_t {};
} // unnamed namespace

template <> constexpr inline auto NezhaIDE::Services::HTTP::HttpClientService::qt_create_metaobjectdata<qt_meta_tag_ZN8NezhaIDE8Services4HTTP17HttpClientServiceE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "NezhaIDE::Services::HTTP::HttpClientService",
        "responseReceived",
        "",
        "Model::HTTP::HttpResponse",
        "resp",
        "requestError",
        "Model::HTTP::RequestId",
        "id",
        "statusCode",
        "error"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'responseReceived'
        QtMocHelpers::SignalData<void(const Model::HTTP::HttpResponse &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'requestError'
        QtMocHelpers::SignalData<void(Model::HTTP::RequestId, int, const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 7 }, { QMetaType::Int, 8 }, { QMetaType::QString, 9 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<HttpClientService, qt_meta_tag_ZN8NezhaIDE8Services4HTTP17HttpClientServiceE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject NezhaIDE::Services::HTTP::HttpClientService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8NezhaIDE8Services4HTTP17HttpClientServiceE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8NezhaIDE8Services4HTTP17HttpClientServiceE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8NezhaIDE8Services4HTTP17HttpClientServiceE_t>.metaTypes,
    nullptr
} };

void NezhaIDE::Services::HTTP::HttpClientService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<HttpClientService *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->responseReceived((*reinterpret_cast<std::add_pointer_t<Model::HTTP::HttpResponse>>(_a[1]))); break;
        case 1: _t->requestError((*reinterpret_cast<std::add_pointer_t<Model::HTTP::RequestId>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (HttpClientService::*)(const Model::HTTP::HttpResponse & )>(_a, &HttpClientService::responseReceived, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (HttpClientService::*)(Model::HTTP::RequestId , int , const QString & )>(_a, &HttpClientService::requestError, 1))
            return;
    }
}

const QMetaObject *NezhaIDE::Services::HTTP::HttpClientService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NezhaIDE::Services::HTTP::HttpClientService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8NezhaIDE8Services4HTTP17HttpClientServiceE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int NezhaIDE::Services::HTTP::HttpClientService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void NezhaIDE::Services::HTTP::HttpClientService::responseReceived(const Model::HTTP::HttpResponse & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void NezhaIDE::Services::HTTP::HttpClientService::requestError(Model::HTTP::RequestId _t1, int _t2, const QString & _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2, _t3);
}
QT_WARNING_POP
