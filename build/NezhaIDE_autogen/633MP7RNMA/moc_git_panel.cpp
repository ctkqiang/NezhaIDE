/****************************************************************************
** Meta object code from reading C++ file 'git_panel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../views/git_panel/git_panel.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'git_panel.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN8NezhaIDE5Views8GitPanelE_t {};
} // unnamed namespace

template <> constexpr inline auto NezhaIDE::Views::GitPanel::qt_create_metaobjectdata<qt_meta_tag_ZN8NezhaIDE5Views8GitPanelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "NezhaIDE::Views::GitPanel",
        "fileStaged",
        "",
        "path",
        "fileUnstaged",
        "commitRequested",
        "message",
        "branchChanged",
        "branch",
        "fileOpened",
        "onRefresh",
        "onStageFile",
        "onStageAll",
        "onUnstageFile",
        "onUnstageAll",
        "onDiscardFile",
        "onShowDiff",
        "onOpenFile",
        "onCommit",
        "onStatusFinished",
        "exit_code",
        "QProcess::ExitStatus",
        "status",
        "onBranchFinished",
        "onListItemClicked",
        "QListWidgetItem*",
        "item",
        "onListItemDoubleClicked",
        "onCustomContextMenu",
        "QPoint",
        "pos",
        "onCommitSelected",
        "hash",
        "subject"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'fileStaged'
        QtMocHelpers::SignalData<void(const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'fileUnstaged'
        QtMocHelpers::SignalData<void(const QString &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'commitRequested'
        QtMocHelpers::SignalData<void(const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 6 },
        }}),
        // Signal 'branchChanged'
        QtMocHelpers::SignalData<void(const QString &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Signal 'fileOpened'
        QtMocHelpers::SignalData<void(const QString &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Slot 'onRefresh'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStageFile'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStageAll'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onUnstageFile'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onUnstageAll'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDiscardFile'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onShowDiff'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onOpenFile'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onCommit'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStatusFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 20 }, { 0x80000000 | 21, 22 },
        }}),
        // Slot 'onBranchFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(23, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 20 }, { 0x80000000 | 21, 22 },
        }}),
        // Slot 'onListItemClicked'
        QtMocHelpers::SlotData<void(QListWidgetItem *)>(24, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 25, 26 },
        }}),
        // Slot 'onListItemDoubleClicked'
        QtMocHelpers::SlotData<void(QListWidgetItem *)>(27, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 25, 26 },
        }}),
        // Slot 'onCustomContextMenu'
        QtMocHelpers::SlotData<void(const QPoint &)>(28, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 29, 30 },
        }}),
        // Slot 'onCommitSelected'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(31, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 32 }, { QMetaType::QString, 33 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<GitPanel, qt_meta_tag_ZN8NezhaIDE5Views8GitPanelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject NezhaIDE::Views::GitPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8NezhaIDE5Views8GitPanelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8NezhaIDE5Views8GitPanelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8NezhaIDE5Views8GitPanelE_t>.metaTypes,
    nullptr
} };

void NezhaIDE::Views::GitPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<GitPanel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->fileStaged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->fileUnstaged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->commitRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->branchChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->fileOpened((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->onRefresh(); break;
        case 6: _t->onStageFile(); break;
        case 7: _t->onStageAll(); break;
        case 8: _t->onUnstageFile(); break;
        case 9: _t->onUnstageAll(); break;
        case 10: _t->onDiscardFile(); break;
        case 11: _t->onShowDiff(); break;
        case 12: _t->onOpenFile(); break;
        case 13: _t->onCommit(); break;
        case 14: _t->onStatusFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 15: _t->onBranchFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 16: _t->onListItemClicked((*reinterpret_cast<std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        case 17: _t->onListItemDoubleClicked((*reinterpret_cast<std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        case 18: _t->onCustomContextMenu((*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 19: _t->onCommitSelected((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (GitPanel::*)(const QString & )>(_a, &GitPanel::fileStaged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (GitPanel::*)(const QString & )>(_a, &GitPanel::fileUnstaged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (GitPanel::*)(const QString & )>(_a, &GitPanel::commitRequested, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (GitPanel::*)(const QString & )>(_a, &GitPanel::branchChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (GitPanel::*)(const QString & )>(_a, &GitPanel::fileOpened, 4))
            return;
    }
}

const QMetaObject *NezhaIDE::Views::GitPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NezhaIDE::Views::GitPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8NezhaIDE5Views8GitPanelE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int NezhaIDE::Views::GitPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 20)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 20;
    }
    return _id;
}

// SIGNAL 0
void NezhaIDE::Views::GitPanel::fileStaged(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void NezhaIDE::Views::GitPanel::fileUnstaged(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void NezhaIDE::Views::GitPanel::commitRequested(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void NezhaIDE::Views::GitPanel::branchChanged(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void NezhaIDE::Views::GitPanel::fileOpened(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}
QT_WARNING_POP
