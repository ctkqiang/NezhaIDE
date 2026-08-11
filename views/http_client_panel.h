#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include "src/model/http_request.h"

namespace NezhaIDE::Views {

class HttpClientPanel : public QWidget {
    Q_OBJECT

public:
    explicit HttpClientPanel(QWidget *parent = nullptr);
    ~HttpClientPanel() override;

private:
    enum class ResponseState { Empty, Sending, Error, Done };

    void setupUI();
    void buildRequestBar();
    void buildRequestTabs();
    void buildResponseArea();

    QWidget *buildKeyValueTab();
    void appendKeyValueRow(QTableWidget *table);
    void showErrorState(const QString &title, const QString &detail);

    void onMethodClicked();
    void onSendClicked();
    void onCancelClicked();
    void applyStyles();
    void setMethodStyle(const QString &method);
    void setStatusPill(int statusCode, const QString &text);
    void setResponseState(ResponseState state);
    void applySendButtonStyle();

    NezhaIDE::Model::HTTP::HttpRequest collectRequest() const;
    QString normalizeUrl(QString url) const;

    QPushButton *method_btn_{};
    QLineEdit *url_input_{};
    QPushButton *send_btn_{};
    QTableWidget *params_table_{};
    QTableWidget *headers_table_{};
    QComboBox *body_type_combo_{};
    QPlainTextEdit *body_editor_{};
    QTabWidget *request_tabs_{};

    QLabel *status_pill_{};
    QLabel *time_label_{};
    QLabel *size_label_{};
    QStackedWidget *response_stack_{};
    QTabWidget *response_tabs_{};
    QPlainTextEdit *response_body_{};
    QTableWidget *response_headers_table_{};
    QLabel *empty_title_{};
    QLabel *empty_detail_{};
    QLabel *error_title_{};
    QLabel *error_detail_{};

    QString method_{QStringLiteral("GET")};
    bool sending_{false};
    NezhaIDE::Model::HTTP::RequestId current_request_id_{};
};

} // namespace NezhaIDE::Views
