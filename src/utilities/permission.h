//
// Created by 钟智强 on 2026/8/11.
//
#pragma once

#ifndef NEZHAIDE_PERMISSION_H
#define NEZHAIDE_PERMISSION_H

namespace NezhaIDE::Utilities::Security {

/**
 * 系统权限类型，对应 macOS TCC（Transparency, Consent, and Control）保护的服务。
 *
 * Windows / Linux 无等价的 TCC 概念，相关调用将返回 Unsupported。
 */
enum class PermissionType {
    ScreenRecording,  // 屏幕录制
    Accessibility,    // 辅助功能
    InputMonitoring,  // 输入监控
    FullDiskAccess,   // 完全磁盘访问
    Microphone,       // 麦克风
    Camera,           // 摄像头
    Notifications,    // 通知
};

/**
 * 权限状态。
 */
enum class PermissionStatus {
    NotDetermined,  // 尚未向用户请求
    Denied,         // 已被拒绝或受限
    Authorized,     // 已授权
    Unsupported,    // 当前平台不提供该权限
};

/**
 * 跨平台系统权限服务（单例）。
 *
 * macOS 上通过 TCC 查询与请求权限；请求时系统会弹出授权对话框，
 * 被拒绝后可调用 openSettings() 引导用户到系统设置手动开启。
 *
 * @note 屏幕录制/辅助功能/输入监控/全盘访问为系统级开关，request()
 *       仅能打开授权对话框（全盘访问无对话框，直接引导设置页）。
 *
 * @see https://developer.apple.com/documentation/technotes/tn3175-understanding-macos-tcc-protected-resources
 */
class Permission {
public:
    static Permission &instance();

    /**
     * 查询权限状态。
     *
     * @param type 权限类型
     * @return 当前授权状态；非 macOS 平台返回 Unsupported
     */
    [[nodiscard]] PermissionStatus status(PermissionType type) const;

    /**
     * 请求权限，触发系统授权对话框（异步等待用户决策）。
     *
     * @param type 权限类型
     * @return 用户是否授予权限；调用失败或平台不支持返回 false
     */
    [[nodiscard]] bool request(PermissionType type);

    /**
     * 打开系统设置中对应权限的配置页。
     *
     * @param type 权限类型
     */
    void openSettings(PermissionType type) const;

    Permission(const Permission &) = delete;
    Permission &operator=(const Permission &) = delete;

private:
    Permission() = default;
    ~Permission() = default;
};

} // namespace NezhaIDE::Utilities::Security

#endif //NEZHAIDE_PERMISSION_H
