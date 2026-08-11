#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QLabel>

namespace NezhaIDE::Views {

class HttpClientPanel : public QWidget {
    Q_OBJECT

public:
    explicit HttpClientPanel(QWidget *parent = nullptr);
    ~HttpClientPanel() override;

private:
    void setupUI();
    void onMethodClicked();
    void onSendClicked();
    void applyStyles();
    void setMethodStyle(const QString &method);
    void setStatusColor(int statusCode);

    QPushButton *method_btn_{};
    QLineEdit *url_input_{};
    QPushButton *send_btn_{};
    QTableWidget *params_table_{};
    QTableWidget *headers_table_{};
    QPlainTextEdit *body_editor_{};
    QTabWidget *request_tabs_{};
    QLabel *status_label_{};
    QLabel *time_label_{};
    QLabel *response_size_label_{};
    QTabWidget *response_tabs_{};
    QTableWidget *response_headers_table_{};
    QPlainTextEdit *response_body_{};
    QString method_{QStringLiteral("GET")};
};

} // namespace NezhaIDE::Views
