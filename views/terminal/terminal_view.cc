#include "terminal_view.h"
#include "src/services/theme_service.h"
#include <QKeyEvent>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextCursor>
#include <cstdlib>
#include <unistd.h>
#include <util.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>

namespace NezhaIDE::Views {

TerminalView::TerminalView(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setFont(QFont(QStringLiteral("SF Mono"), 12));
    setReadOnly(false);
    setUndoRedoEnabled(false);
    setMaximumBlockCount(kMaxScrollback);
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 8);
    setCursorWidth(2);

    QPalette p = palette();
    p.setColor(QPalette::Base, QColor(0x0D, 0x11, 0x17));
    p.setColor(QPalette::Text, QColor(0xE6, 0xED, 0xF3));
    setPalette(p);

    current_fmt_.setForeground(QColor(0xE6, 0xED, 0xF3));
    current_fmt_.setFontWeight(QFont::Normal);
    current_fmt_.setFontItalic(false);
    current_fmt_.setFontUnderline(false);
    setCurrentCharFormat(current_fmt_);

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);

    connect(this, &QPlainTextEdit::cursorPositionChanged, this, [this] {
        auto cursor = textCursor();
        cursor.movePosition(QTextCursor::End);
        setTextCursor(cursor);
    });
}

TerminalView::~TerminalView() {
    killShell();
}

bool TerminalView::startShell() {
    struct winsize ws{};
    ws.ws_col = 120;
    ws.ws_row = 40;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    shell_pid_ = forkpty(&master_fd_, nullptr, nullptr, &ws);
    if (shell_pid_ < 0) return false;

    if (shell_pid_ == 0) {
        setsid();

        const char *shell = std::getenv("SHELL");
        if (!shell || shell[0] == '\0') shell = "/bin/bash";

        const char *argv[] = {shell, "-l", nullptr};
        setenv("TERM", "xterm-256color", 1);
        setenv("COLORTERM", "truecolor", 1);
        execvp(shell, const_cast<char *const *>(argv));
        _exit(127);
    }

    notifier_ = std::make_unique<QSocketNotifier>(master_fd_, QSocketNotifier::Read, this);
    connect(notifier_.get(), &QSocketNotifier::activated, this, &TerminalView::onPtyReadyRead);

    QTextCursor cursor = textCursor();
    current_fmt_.setForeground(QColor(0xE6, 0xED, 0xF3));
    cursor.insertText(QStringLiteral("\r\n"), current_fmt_);
    setTextCursor(cursor);

    return true;
}

void TerminalView::killShell() {
    if (shell_pid_ > 0) {
        ::write(master_fd_, "\x03", 1);
        usleep(50000);
        ::write(master_fd_, "exit\n", 5);

        int status;
        pid_t result;
        for (int i = 0; i < 10; ++i) {
            result = waitpid(shell_pid_, &status, WNOHANG);
            if (result > 0) break;
            usleep(100000);
        }
        if (result <= 0) {
            kill(shell_pid_, SIGKILL);
            waitpid(shell_pid_, &status, 0);
        }
        shell_pid_ = -1;
    }
    if (master_fd_ >= 0) {
        notifier_.reset();
        ::close(master_fd_);
        master_fd_ = -1;
    }
}

bool TerminalView::isRunning() const noexcept {
    return shell_pid_ > 0 && master_fd_ >= 0;
}

void TerminalView::sendText(const QString &text) {
    if (!isRunning()) return;
    QByteArray data = text.toUtf8();
    writeToPty(data);
}

void TerminalView::keyPressEvent(QKeyEvent *event) {
    if (!isRunning()) {
        QPlainTextEdit::keyPressEvent(event);
        return;
    }

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
            if (key >= Qt::Key_A && key <= Qt::Key_Z) {
                data = QByteArray(1, static_cast<char>(key - Qt::Key_A + 1));
            } else if (key == Qt::Key_C) {
                killShell();
                emit shellExited(0);
                return;
            }
        } else {
            QString text = event->text();
            if (!text.isEmpty()) {
                data = text.toUtf8();
            }
        }
        break;
    }

    if (!data.isEmpty()) {
        writeToPty(data);
        event->accept();
    } else {
        event->ignore();
    }
}

void TerminalView::writeToPty(const QByteArray &data) {
    if (master_fd_ < 0) return;
    ::write(master_fd_, data.constData(), static_cast<size_t>(data.size()));
}

void TerminalView::onPtyReadyRead() {
    char buf[4096];
    ssize_t n = ::read(master_fd_, buf, sizeof(buf));
    if (n <= 0) {
        if (n < 0) killShell();
        emit shellExited(0);
        return;
    }
    QByteArray data(buf, static_cast<int>(n));
    parseAnsiAndAppend(data);
    ensureCursorVisible();
}

static int ansiColorToRgb(int code) {
    switch (code) {
    case 30: return 0x1F2329;
    case 31: return 0xE74C3C;
    case 32: return 0x27AE60;
    case 33: return 0xE67E22;
    case 34: return 0x3370FF;
    case 35: return 0xCF1F8B;
    case 36: return 0x0E7A7A;
    case 37: return 0xE6EDF3;
    case 90: return 0x6E7681;
    case 91: return 0xF0883E;
    case 92: return 0x3FB950;
    case 93: return 0xF0C000;
    case 94: return 0x58A6FF;
    case 95: return 0xD2A8FF;
    case 96: return 0x79C0FF;
    case 97: return 0xFFFFFF;
    default: return -1;
    }
}

void TerminalView::processSgr(const QStringList &params) {
    if (params.isEmpty()) {
        bold_ = false; italic_ = false; underline_ = false; inverse_ = false;
        fg_color_ = QColor(ansiColorToRgb(37));
        bg_color_ = QColor(0x0D, 0x11, 0x17);
        current_fmt_.setForeground(fg_color_);
        current_fmt_.setBackground(bg_color_);
        current_fmt_.setFontWeight(QFont::Normal);
        current_fmt_.setFontItalic(false);
        current_fmt_.setFontUnderline(false);
        setCurrentCharFormat(current_fmt_);
        return;
    }

    for (const auto &p : params) {
        int code = p.toInt();
        switch (code) {
        case 0:
            bold_ = false; italic_ = false; underline_ = false; inverse_ = false;
            fg_color_ = QColor(0xE6, 0xED, 0xF3);
            bg_color_ = QColor(0x0D, 0x11, 0x17);
            break;
        case 1: bold_ = true; break;
        case 3: italic_ = true; break;
        case 4: underline_ = true; break;
        case 7: inverse_ = true; break;
        case 22: bold_ = false; break;
        case 23: italic_ = false; break;
        case 24: underline_ = false; break;
        case 27: inverse_ = false; break;
        default:
            if (code >= 30 && code <= 37) fg_color_ = QColor(ansiColorToRgb(code));
            else if (code >= 40 && code <= 47) bg_color_ = QColor(ansiColorToRgb(code - 10));
            else if (code >= 90 && code <= 97) fg_color_ = QColor(ansiColorToRgb(code));
            else if (code >= 100 && code <= 107) bg_color_ = QColor(ansiColorToRgb(code - 10));
            break;
        }
    }

    if (inverse_) std::swap(fg_color_, bg_color_);

    current_fmt_.setForeground(fg_color_);
    current_fmt_.setBackground(bg_color_);
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
            if (!output.isEmpty()) {
                cursor.insertText(output, current_fmt_);
                output.clear();
            }
            i += 2;
            QString codeStr;
            while (i < pending_ansi_.size() && pending_ansi_[i] != QChar('m')
                   && pending_ansi_[i] != QChar(';') && !pending_ansi_[i].isLetter()) {
                codeStr += pending_ansi_[i];
                ++i;
            }
            if (i < pending_ansi_.size() && pending_ansi_[i] == QChar('m')) {
                processSgr(codeStr.split(';', Qt::SkipEmptyParts));
                ++i;
            } else if (i < pending_ansi_.size() && pending_ansi_[i].isLetter()) {
                char cmd = pending_ansi_[i].toLatin1();
                ++i;
                if (cmd == 'K') {
                    cursor.movePosition(QTextCursor::End);
                    cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
                    cursor.removeSelectedText();
                } else if (cmd == 'J') {
                    cursor.movePosition(QTextCursor::End);
                    cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
                } else if (cmd == 'H') {
                }
            }
            pending_ansi_ = pending_ansi_.mid(i);
            i = 0;
            continue;
        }

        if (ch == QChar('\r')) {
            if (!output.isEmpty()) {
                cursor.insertText(output, current_fmt_);
                output.clear();
            }
            cursor.movePosition(QTextCursor::End);
            cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            ++i;
            continue;
        }

        if (ch == QChar('\b') || ch == QChar(0x7F)) {
            if (!output.isEmpty()) {
                output.chop(1);
            } else {
                cursor.movePosition(QTextCursor::End);
                cursor.deletePreviousChar();
            }
            ++i;
            continue;
        }

        if (ch == QChar('\a')) { ++i; continue; }

        if (ch == QChar('\t')) {
            output += QStringLiteral("        ");
            ++i;
            continue;
        }

        output += ch;
        ++i;
    }

    pending_ansi_ = pending_ansi_.mid(i);

    if (!output.isEmpty()) {
        cursor.insertText(output, current_fmt_);
    }

    setTextCursor(cursor);
}

} // namespace NezhaIDE::Views
