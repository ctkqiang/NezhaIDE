#pragma once

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
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
    void onSendClicked();
    void applyStyles();

    QComboBox *method_combo_{};
    QLineEdit *url_input_{};
    QPushButton *send_btn_{};
    QTableWidget *headers_table_{};
    QPlainTextEdit *body_editor_{};
    QTabWidget *request_tabs_{};
    QLabel *status_label_{};
    QLabel *time_label_{};
    QPlainTextEdit *response_body_{};
    QLabel *response_size_label_{};
};

} // namespace NezhaIDE::Views
