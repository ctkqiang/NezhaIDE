//
// Created by 钟智强 on 2026/8/11.
//

#include "permission.h"

#include <QDir>
#include <QProcess>
#include <QSettings>

#ifdef Q_OS_MACOS
#include <ApplicationServices/ApplicationServices.h>
#include <AVFoundation/AVFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <UserNotifications/UserNotifications.h>
#include <cerrno>
#include <dirent.h>
#include <dispatch/dispatch.h>
#endif

namespace NezhaIDE::Utilities::Security {

namespace {

constexpr auto kSettingsOrg = "NezhaSecurity";
constexpr auto kSettingsApp = "NezhaIDE";

QString requestedKey(PermissionType type)
{
    return QStringLiteral("permission/%1/requested").arg(static_cast<int>(type));
}

/**
 * TCC 的布尔授权 API 无法区分"未请求"与"已被拒绝"，
 * 用本地持久化记录请求历史来区分这两种状态。
 */
void markRequested(PermissionType type)
{
    QSettings(kSettingsOrg, kSettingsApp).setValue(requestedKey(type), true);
}

bool wasRequested(PermissionType type)
{
    return QSettings(kSettingsOrg, kSettingsApp).value(requestedKey(type), false).toBool();
}

#ifdef Q_OS_MACOS

QString settingsUrl(PermissionType type)
{
    switch (type) {
    case PermissionType::ScreenRecording:
        return QStringLiteral("x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture");
    case PermissionType::Accessibility:
        return QStringLiteral("x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility");
    case PermissionType::InputMonitoring:
        return QStringLiteral("x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent");
    case PermissionType::FullDiskAccess:
        return QStringLiteral("x-apple.systempreferences:com.apple.preference.security?Privacy_AllFiles");
    case PermissionType::Microphone:
        return QStringLiteral("x-apple.systempreferences:com.apple.preference.security?Privacy_Microphone");
    case PermissionType::Camera:
        return QStringLiteral("x-apple.systempreferences:com.apple.preference.security?Privacy_Camera");
    case PermissionType::Notifications:
        return QStringLiteral("x-apple.systempreferences:com.apple.settings.notifications");
    }
    return {};
}

PermissionStatus boolApiStatus(bool granted, PermissionType type)
{
    if (granted) return PermissionStatus::Authorized;
    return wasRequested(type) ? PermissionStatus::Denied : PermissionStatus::NotDetermined;
}

/**
 * 全盘访问无公开查询 API，通过探测受保护目录判断：
 * TCC 拦截时 opendir 返回 EACCES/EPERM，目录不存在则无法判断。
 */
PermissionStatus probeFullDiskAccess()
{
    const auto probe = QDir::homePath() + QStringLiteral("/Library/Safari");
    if (DIR *dir = opendir(probe.toUtf8().constData())) {
        closedir(dir);
        return PermissionStatus::Authorized;
    }
    if (errno == ENOENT) return PermissionStatus::NotDetermined;
    return PermissionStatus::Denied;
}

PermissionStatus avToStatus(AVAuthorizationStatus status)
{
    switch (status) {
    case AVAuthorizationStatusAuthorized:
        return PermissionStatus::Authorized;
    case AVAuthorizationStatusNotDetermined:
        return PermissionStatus::NotDetermined;
    case AVAuthorizationStatusDenied:
    case AVAuthorizationStatusRestricted:
        return PermissionStatus::Denied;
    }
    return PermissionStatus::Unsupported;
}

/**
 * AVFoundation 授权回调为异步，用信号量同步等待用户决策。
 */
bool requestAvAccess(AVMediaType mediaType)
{
    dispatch_semaphore_t sema = dispatch_semaphore_create(0);
    __block bool granted = false;
    [AVCaptureDevice requestAccessForMediaType:mediaType completionHandler:^(BOOL ok) {
        granted = static_cast<bool>(ok);
        dispatch_semaphore_signal(sema);
    }];
    dispatch_semaphore_wait(sema, dispatch_time(DISPATCH_TIME_NOW, 30LL * NSEC_PER_SEC));
    dispatch_release(sema);
    return granted;
}

/**
 * UNUserNotificationCenter 要求进程处于 app bundle 环境，
 * 否则 currentNotificationCenter 会直接抛异常。
 */
bool inBundleEnvironment()
{
    NSBundle *bundle = [NSBundle mainBundle];
    return bundle.bundleURL != nil
        && [bundle objectForInfoDictionaryKey:@"CFBundleIdentifier"] != nil;
}

/**
 * 通知授权回调为异步，用信号量同步返回系统状态。
 */
PermissionStatus notificationStatus()
{
    if (!inBundleEnvironment()) return PermissionStatus::Unsupported;
    dispatch_semaphore_t sema = dispatch_semaphore_create(0);
    __block UNAuthorizationStatus result = UNAuthorizationStatusNotDetermined;
    [[UNUserNotificationCenter currentNotificationCenter]
        getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings *settings) {
        result = settings.authorizationStatus;
        dispatch_semaphore_signal(sema);
    }];
    dispatch_semaphore_wait(sema, dispatch_time(DISPATCH_TIME_NOW, 5LL * NSEC_PER_SEC));
    dispatch_release(sema);
    switch (result) {
    case UNAuthorizationStatusAuthorized:
    case UNAuthorizationStatusProvisional:
        return PermissionStatus::Authorized;
    case UNAuthorizationStatusDenied:
        return PermissionStatus::Denied;
    case UNAuthorizationStatusNotDetermined:
        return PermissionStatus::NotDetermined;
    }
    return PermissionStatus::Unsupported;
}

bool requestNotificationAccess()
{
    if (!inBundleEnvironment()) return false;
    dispatch_semaphore_t sema = dispatch_semaphore_create(0);
    __block bool granted = false;
    [[UNUserNotificationCenter currentNotificationCenter]
        requestAuthorizationWithOptions:UNAuthorizationOptionAlert | UNAuthorizationOptionBadge | UNAuthorizationOptionSound
        completionHandler:^(BOOL ok, NSError *) {
        granted = static_cast<bool>(ok);
        dispatch_semaphore_signal(sema);
    }];
    dispatch_semaphore_wait(sema, dispatch_time(DISPATCH_TIME_NOW, 30LL * NSEC_PER_SEC));
    dispatch_release(sema);
    return granted;
}

#endif // Q_OS_MACOS

} // namespace

Permission &Permission::instance()
{
    static Permission permission;
    return permission;
}

PermissionStatus Permission::status(PermissionType type) const
{
#ifdef Q_OS_MACOS
    switch (type) {
    case PermissionType::ScreenRecording:
        return boolApiStatus(CGPreflightScreenCaptureAccess(), type);
    case PermissionType::Accessibility:
        return boolApiStatus(AXIsProcessTrusted(), type);
    case PermissionType::InputMonitoring:
        return boolApiStatus(CGPreflightPostEventAccess(), type);
    case PermissionType::FullDiskAccess:
        return probeFullDiskAccess();
    case PermissionType::Microphone:
        return avToStatus([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio]);
    case PermissionType::Camera:
        return avToStatus([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo]);
    case PermissionType::Notifications:
        return notificationStatus();
    }
#endif
    return PermissionStatus::Unsupported;
}

bool Permission::request(PermissionType type)
{
#ifdef Q_OS_MACOS
    bool granted = false;
    switch (type) {
    case PermissionType::ScreenRecording:
        granted = static_cast<bool>(CGRequestScreenCaptureAccess());
        break;
    case PermissionType::Accessibility:
        granted = static_cast<bool>(AXIsProcessTrustedWithOptions(
            (__bridge CFDictionaryRef)@{static_cast<NSString *>(kAXTrustedCheckOptionPrompt): @YES}));
        break;
    case PermissionType::InputMonitoring:
        granted = static_cast<bool>(CGRequestPostEventAccess());
        break;
    case PermissionType::FullDiskAccess:
        granted = probeFullDiskAccess() == PermissionStatus::Authorized;
        break;
    case PermissionType::Microphone:
        granted = requestAvAccess(AVMediaTypeAudio);
        break;
    case PermissionType::Camera:
        granted = requestAvAccess(AVMediaTypeVideo);
        break;
    case PermissionType::Notifications:
        granted = requestNotificationAccess();
        break;
    }
    markRequested(type);
    return granted;
#endif
    return false;
}

void Permission::openSettings(PermissionType type) const
{
#ifdef Q_OS_MACOS
    const auto url = settingsUrl(type);
    if (url.isEmpty()) return;
    QProcess::startDetached(QStringLiteral("open"), {url});
#elif defined(Q_OS_WIN)
    QString url;
    switch (type) {
    case PermissionType::Microphone:
        url = QStringLiteral("ms-settings:privacy-microphone");
        break;
    case PermissionType::Camera:
        url = QStringLiteral("ms-settings:privacy-webcam");
        break;
    case PermissionType::Notifications:
        url = QStringLiteral("ms-settings:notifications");
        break;
    default:
        return;
    }
    QProcess::startDetached(QStringLiteral("explorer.exe"), {url});
#else
    (void)type;
#endif
}

} // namespace NezhaIDE::Utilities::Security
