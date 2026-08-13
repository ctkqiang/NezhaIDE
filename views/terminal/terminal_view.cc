#include "terminal_view.h"
#include "src/services/theme_service.h"
#include <QFontDatabase>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTextCursor>
#include <cstdlib>

#ifdef Q_OS_WIN
#include <QProcess>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#ifdef __APPLE__
#include <util.h>
#else
#include <pty.h>
#endif
#endif

namespace NezhaIDE::Views {

TerminalView::TerminalView(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    setReadOnly(false);
    setUndoRedoEnabled(false);
    setMaximumBlockCount(kMaxScrollback);
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 8);
    setCursorWidth(2);
    setFrameShape(QFrame::NoFrame);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);

    connect(&Services::ThemeService::instance(), &Services::ThemeService::themeChanged,
            this, [this] { applyTheme(); });

    applyTheme();
    resetFormat();
}

TerminalView::~TerminalView() { killShell(); }

void TerminalView::applyTheme() {
    auto &ts = Services::ThemeService::instance();
    theme_bg_ = ts.qcolor(QStringLiteral("bg.primary"));
    theme_fg_ = ts.qcolor(QStringLiteral("text.primary"));

    QPalette p;
    p.setColor(QPalette::Base, theme_bg_);
    p.setColor(QPalette::Text, theme_fg_);
    p.setColor(QPalette::PlaceholderText, ts.qcolor(QStringLiteral("text.tertiary")));
    setPalette(p);

    // 滚动条由全局 style.scrollbar 提供，此处只覆盖文本区底色
    setStyleSheet(QStringLiteral(
        "QPlainTextEdit { border: none; background: %1; color: %2; }")
        .arg(theme_bg_.name(), theme_fg_.name()));

    fg_color_ = theme_fg_;
    bg_color_ = theme_bg_;
    resetFormat();
    viewport()->update();
}

void TerminalView::resetFormat() {
    bold_ = false; italic_ = false; underline_ = false; inverse_ = false;
    fg_color_ = theme_fg_;
    bg_color_ = theme_bg_;
    current_fmt_.setForeground(theme_fg_);
    current_fmt_.setBackground(theme_bg_);
    current_fmt_.setFontWeight(QFont::Normal);
    current_fmt_.setFontItalic(false);
    current_fmt_.setFontUnderline(false);
    setCurrentCharFormat(current_fmt_);
}

void TerminalView::resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);
    updatePtySize();
}

void TerminalView::updatePtySize() {
#ifndef Q_OS_WIN
    if (master_fd_ < 0) return;
    auto fm = fontMetrics();
    int cols = viewport()->width() / fm.horizontalAdvance(' ');
    int rows = viewport()->height() / fm.height();
    if (cols < 20) cols = 80;
    if (rows < 8) rows = 24;

    struct winsize ws{};
    ws.ws_col = static_cast<unsigned short>(cols);
    ws.ws_row = static_cast<unsigned short>(rows);
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;
    ioctl(master_fd_, TIOCSWINSZ, &ws);
#endif
}

bool TerminalView::startShell() {
#ifdef Q_OS_WIN
    const auto shell = qEnvironmentVariable("COMSPEC", QStringLiteral("cmd.exe"));
    shell_path_ = shell;
    process_ = new QProcess(this);
    connect(process_, &QProcess::readyReadStandardOutput, this, [this] {
        parseAnsiAndAppend(process_->readAllStandardOutput());
    });
    connect(process_, &QProcess::readyReadStandardError, this, [this] {
        parseAnsiAndAppend(process_->readAllStandardError());
    });
    connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus) { emit shellExited(code); });
    process_->start(shell, {QStringLiteral("/K")});
    return process_->waitForStarted(3000);
#else
    struct winsize ws{};
    ws.ws_col = 120;
    ws.ws_row = 40;

    const char *shell = std::getenv("SHELL");
    if (!shell || shell[0] == '\0') shell = "/bin/zsh";
    shell_path_ = QString::fromUtf8(shell);

    shell_pid_ = forkpty(&master_fd_, nullptr, nullptr, &ws);
    if (shell_pid_ < 0) return false;

    if (shell_pid_ == 0) {
        setsid();
        const char *args[] = {shell, "-l", nullptr};
        setenv("TERM", "xterm-256color", 1);
        setenv("COLORTERM", "truecolor", 1);
        execvp(shell, const_cast<char *const *>(args));
        _exit(127);
    }

    notifier_ = std::make_unique<QSocketNotifier>(master_fd_, QSocketNotifier::Read, this);
    connect(notifier_.get(), &QSocketNotifier::activated, this, &TerminalView::onPtyReadyRead);

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(QStringLiteral("\r\n"), current_fmt_);
    setTextCursor(cursor);

    return true;
#endif
}

void TerminalView::killShell() {
#ifdef Q_OS_WIN
    if (process_) {
        if (process_->state() != QProcess::NotRunning) {
            process_->kill();
            process_->waitForFinished(2000);
        }
        delete process_;
        process_ = nullptr;
    }
#else
    if (shell_pid_ > 0 && master_fd_ >= 0) {
        ::write(master_fd_, "\x03", 1);
        ::write(master_fd_, "exit\n", 5);
    }
    // 关闭 master 使 slave 侧读到 EOF，shell 收到 SIGHUP 自行退出
    if (master_fd_ >= 0) {
        notifier_.reset();
        ::close(master_fd_);
        master_fd_ = -1;
    }
    if (shell_pid_ > 0) {
        int status;
        for (int i = 0; i < 15; ++i) {
            if (waitpid(shell_pid_, &status, WNOHANG) == shell_pid_) {
                shell_pid_ = -1;
                return;
            }
            usleep(100000);
        }
        // macOS 上对 exiting 状态的进程同步 waitpid 可能永久阻塞（内核
        // 清理挂起），进程退出时僵尸会被 init 收割，因此强杀后不等待
        kill(shell_pid_, SIGKILL);
        shell_pid_ = -1;
    }
#endif
}

bool TerminalView::isRunning() const noexcept {
#ifdef Q_OS_WIN
    return process_ && process_->state() != QProcess::NotRunning;
#else
    return shell_pid_ > 0 && master_fd_ >= 0;
#endif
}

QString TerminalView::shellName() const {
    if (shell_path_.isEmpty()) return QStringLiteral("shell");
    int pos = qMax(shell_path_.lastIndexOf(QChar('/')), shell_path_.lastIndexOf(QChar('\\')));
    return pos >= 0 ? shell_path_.mid(pos + 1) : shell_path_;
}

void TerminalView::sendText(const QString &text) {
    if (!isRunning()) return;
    writeToPty(text.toUtf8());
}

void TerminalView::writeToPty(const QByteArray &data) {
#ifdef Q_OS_WIN
    if (process_) process_->write(data);
#else
    if (master_fd_ < 0) return;
    ::write(master_fd_, data.constData(), static_cast<size_t>(data.size()));
#endif
}

void TerminalView::keyPressEvent(QKeyEvent *event) {
    if (!isRunning()) { QPlainTextEdit::keyPressEvent(event); return; }

    QByteArray data;
    switch (event->key()) {
    case Qt::Key_Backspace: data = "\x7f"; break;
    case Qt::Key_Return:
    case Qt::Key_Enter:     data = "\r"; break;
    case Qt::Key_Tab:       data = "\t"; break;
    case Qt::Key_Escape:    data = "\x1b"; break;
    case Qt::Key_Up:        data = "\x1b[A"; break;
    case Qt::Key_Down:      data = "\x1b[B"; break;
    case Qt::Key_Right:     data = "\x1b[C"; break;
    case Qt::Key_Left:      data = "\x1b[D"; break;
    case Qt::Key_Home:      data = "\x1b[H"; break;
    case Qt::Key_End:       data = "\x1b[F"; break;
    case Qt::Key_Delete:    data = "\x1b[3~"; break;
    case Qt::Key_PageUp:    data = "\x1b[5~"; break;
    case Qt::Key_PageDown:  data = "\x1b[6~"; break;
    default:
        if (event->modifiers() & Qt::ControlModifier) {
            int key = event->key();
            if (key >= Qt::Key_A && key <= Qt::Key_Z)
                data = QByteArray(1, static_cast<char>(key - Qt::Key_A + 1));
        } else {
            QString text = event->text();
            if (!text.isEmpty()) data = text.toUtf8();
        }
        break;
    }

    if (!data.isEmpty()) { writeToPty(data); event->accept(); }
    else event->ignore();
}

#ifndef Q_OS_WIN
void TerminalView::onPtyReadyRead() {
    char buf[4096];
    ssize_t n = ::read(master_fd_, buf, sizeof(buf));
    if (n <= 0) { if (n < 0) killShell(); emit shellExited(0); return; }
    parseAnsiAndAppend(QByteArray(buf, static_cast<int>(n)));
    ensureCursorVisible();
}
#endif

static QColor ansiToColor(int code, const QColor &themeFg, const QColor &themeBg) {
    auto &ts = Services::ThemeService::instance();
    switch (code) {
    case 30: return themeFg;
    case 31: return ts.qcolor(QStringLiteral("git.deleted"));
    case 32: return ts.qcolor(QStringLiteral("git.added"));
    case 33: return ts.qcolor(QStringLiteral("git.modified"));
    case 34: return ts.qcolor(QStringLiteral("accent"));
    case 35: return QColor(0xD2, 0xA8, 0xFF);
    case 36: return QColor(0x79, 0xC0, 0xFF);
    case 37: return themeFg;
    case 90: return ts.qcolor(QStringLiteral("text.tertiary"));
    case 91: return QColor(0xF0, 0x88, 0x3E);
    case 92: return QColor(0x3F, 0xB9, 0x50);
    case 93: return QColor(0xF0, 0xC0, 0x00);
    case 94: return QColor(0x58, 0xA6, 0xFF);
    case 95: return QColor(0xD2, 0xA8, 0xFF);
    case 96: return QColor(0x79, 0xC0, 0xFF);
    case 97: return QColor(0xFF, 0xFF, 0xFF);
    default: return QColor();
    }
}

void TerminalView::processSgr(const QStringList &params) {
    if (params.isEmpty()) { resetFormat(); return; }

    for (const auto &p : params) {
        int code = p.toInt();
        if (code == 0) {
            bold_ = false; italic_ = false; underline_ = false; inverse_ = false;
            fg_color_ = theme_fg_;
            bg_color_ = theme_bg_;
            continue;
        }
        if (code == 1) bold_ = true;
        else if (code == 3) italic_ = true;
        else if (code == 4) underline_ = true;
        else if (code == 7) inverse_ = true;
        else if (code == 22) bold_ = false;
        else if (code == 23) italic_ = false;
        else if (code == 24) underline_ = false;
        else if (code == 27) inverse_ = false;
        else if (code >= 30 && code <= 37) {
            auto c = ansiToColor(code, theme_fg_, theme_bg_);
            if (c.isValid()) fg_color_ = c;
        } else if (code >= 40 && code <= 47) {
            auto c = ansiToColor(code - 10, theme_fg_, theme_bg_);
            if (c.isValid()) bg_color_ = c;
        } else if (code >= 90 && code <= 97) {
            auto c = ansiToColor(code, theme_fg_, theme_bg_);
            if (c.isValid()) fg_color_ = c;
        } else if (code >= 100 && code <= 107) {
            auto c = ansiToColor(code - 10, theme_fg_, theme_bg_);
            if (c.isValid()) bg_color_ = c;
        }
    }

    auto fg = inverse_ ? bg_color_ : fg_color_;
    auto bg = inverse_ ? fg_color_ : bg_color_;

    current_fmt_.setForeground(fg);
    current_fmt_.setBackground(bg);
    current_fmt_.setFontWeight(bold_ ? QFont::Bold : QFont::Normal);
    current_fmt_.setFontItalic(italic_);
    current_fmt_.setFontUnderline(underline_);
    setCurrentCharFormat(current_fmt_);
}

void TerminalView::parseAnsiAndAppend(const QByteArray &data) {
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);

    pending_ansi_ += QString::fromUtf8(data);
    QString output;
    int i = 0;

    while (i < pending_ansi_.size()) {
        QChar ch = pending_ansi_[i];

        if (ch == QChar(0x1B) && i + 1 < pending_ansi_.size() && pending_ansi_[i + 1] == QChar('[')) {
            if (!output.isEmpty()) { cursor.insertText(output, current_fmt_); output.clear(); }
            i += 2;
            QString codeStr;
            while (i < pending_ansi_.size() && pending_ansi_[i] != QChar('m')
                   && pending_ansi_[i] != QChar(';') && !pending_ansi_[i].isLetter()) {
                codeStr += pending_ansi_[i]; ++i;
            }
            if (i < pending_ansi_.size() && pending_ansi_[i] == QChar('m')) {
                processSgr(codeStr.split(';', Qt::SkipEmptyParts)); ++i;
            } else if (i < pending_ansi_.size() && pending_ansi_[i].isLetter()) {
                char cmd = pending_ansi_[i].toLatin1(); ++i;
                if (cmd == 'K') { cursor.movePosition(QTextCursor::End); cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor); cursor.removeSelectedText(); }
                else if (cmd == 'J') { cursor.movePosition(QTextCursor::End); cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor); }
            }
            pending_ansi_ = pending_ansi_.mid(i);
            i = 0;
            continue;
        }

        if (ch == QChar('\r')) {
            if (!output.isEmpty()) { cursor.insertText(output, current_fmt_); output.clear(); }
            cursor.movePosition(QTextCursor::End);
            cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            ++i; continue;
        }
        if (ch == QChar('\b') || ch == QChar(0x7F)) {
            if (!output.isEmpty()) output.chop(1);
            else { cursor.movePosition(QTextCursor::End); cursor.deletePreviousChar(); }
            ++i; continue;
        }
        if (ch == QChar('\a')) { ++i; continue; }
        if (ch == QChar('\t')) { output += QStringLiteral("        "); ++i; continue; }

        output += ch; ++i;
    }
    pending_ansi_ = pending_ansi_.mid(i);
    if (!output.isEmpty()) cursor.insertText(output, current_fmt_);
    setTextCursor(cursor);
}

} // namespace NezhaIDE::Views
