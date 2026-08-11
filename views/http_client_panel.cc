#include "http_client_panel.h"
#include "src/services/http.h"
#include "src/services/theme_service.h"
#include "src/services/localization_service.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>

namespace NezhaIDE::Views {

HttpClientPanel::HttpClientPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    applyStyles();

    connect(&NezhaIDE::Services::HTTP::HttpClientService::instance(),
            &NezhaIDE::Services::HTTP::HttpClientService::responseReceived,
            this, [this](const NezhaIDE::Model::HTTP::HttpResponse &resp) {
        status_label_->setText(QStringLiteral("%1 %2")
            .arg(resp.statusCode)
            .arg(QString::fromStdString(resp.statusText)));
        time_label_->setText(QStringLiteral("%1ms").arg(resp.elapsedMs));
        response_body_->setPlainText(QString::fromStdString(resp.body));

        const auto bodySize = static_cast<qint64>(resp.body.size());
        if (bodySize < 1024) {
            response_size_label_->setText(QStringLiteral("%1 B").arg(bodySize));
        } else {
            response_size_label_->setText(QStringLiteral("%1 KB").arg(bodySize / 1024.0, 0, 'f', 1));
        }

        send_btn_->setEnabled(true);
        send_btn_->setText(QStringLiteral("Send"));
    });

    connect(&NezhaIDE::Services::HTTP::HttpClientService::instance(),
            &NezhaIDE::Services::HTTP::HttpClientService::requestError,
            this, [this](NezhaIDE::Model::HTTP::RequestId, const QString &error) {
        response_body_->setPlainText(error);
        status_label_->setText(QStringLiteral("Error"));
        send_btn_->setEnabled(true);
        send_btn_->setText(QStringLiteral("Send"));
    });
}

HttpClientPanel::~HttpClientPanel() = default;

void HttpClientPanel::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto *mainSplitter = new QSplitter(Qt::Vertical, this);
    mainSplitter->setHandleWidth(1);

    setupRequestList(mainSplitter);
    setupRequestEditor(mainSplitter);
    setupResponseArea(mainSplitter);

    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setStretchFactor(2, 1);
    mainSplitter->setSizes({120, 200, 200});

    mainLayout->addWidget(mainSplitter);
}

void HttpClientPanel::setupRequestList(QSplitter *)
{
    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(2);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(2);

    new_req_btn_ = new QPushButton(QStringLiteral("+"), container);
    new_req_btn_->setFixedSize(24, 24);
    new_req_btn_->setToolTip(QStringLiteral("New Request"));
    connect(new_req_btn_, &QPushButton::clicked, this, &HttpClientPanel::onNewRequest);

    del_req_btn_ = new QPushButton(QStringLiteral("-"), container);
    del_req_btn_->setFixedSize(24, 24);
    del_req_btn_->setToolTip(QStringLiteral("Delete Request"));
    connect(del_req_btn_, &QPushButton::clicked, this, &HttpClientPanel::onDeleteRequest);

    btnLayout->addWidget(new_req_btn_);
    btnLayout->addWidget(del_req_btn_);
    btnLayout->addStretch();

    request_list_ = new QListWidget(container);
    request_list_->setObjectName(QStringLiteral("httpRequestList"));

    layout->addLayout(btnLayout);
    layout->addWidget(request_list_, 1);
    container->setFixedHeight(120);
}

void HttpClientPanel::setupRequestEditor(QSplitter *mainSplitter)
{
    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    auto *urlBar = new QHBoxLayout();
    urlBar->setSpacing(4);

    method_combo_ = new QComboBox(container);
    method_combo_->addItems({"GET", "POST", "PUT", "PATCH", "DELETE", "HEAD", "OPTIONS"});
    method_combo_->setFixedWidth(80);

    url_input_ = new QLineEdit(container);
    url_input_->setPlaceholderText(QStringLiteral("https://api.example.com/endpoint"));

    send_btn_ = new QPushButton(QStringLiteral("Send"), container);
    send_btn_->setFixedWidth(64);
    connect(send_btn_, &QPushButton::clicked, this, &HttpClientPanel::onSendClicked);

    urlBar->addWidget(method_combo_);
    urlBar->addWidget(url_input_, 1);
    urlBar->addWidget(send_btn_);

    request_tabs_ = new QTabWidget(container);
    request_tabs_->setObjectName(QStringLiteral("httpRequestTabs"));

    headers_table_ = new QTableWidget(0, 2);
    headers_table_->setHorizontalHeaderLabels({QStringLiteral("Name"), QStringLiteral("Value")});
    headers_table_->horizontalHeader()->setStretchLastSection(true);
    headers_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    headers_table_->setColumnWidth(0, 140);

    body_editor_ = new QPlainTextEdit();
    body_editor_->setPlaceholderText(QStringLiteral("{\n  \"key\": \"value\"\n}"));

    request_tabs_->addTab(headers_table_, QStringLiteral("Headers"));
    request_tabs_->addTab(body_editor_, QStringLiteral("Body"));

    layout->addLayout(urlBar);
    layout->addWidget(request_tabs_, 1);

    mainSplitter->addWidget(container);
}

void HttpClientPanel::setupResponseArea(QSplitter *mainSplitter)
{
    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(2);

    auto *infoBar = new QHBoxLayout();
    infoBar->setSpacing(8);

    status_label_ = new QLabel(container);
    status_label_->setObjectName(QStringLiteral("httpStatusLabel"));
    time_label_ = new QLabel(container);
    time_label_->setObjectName(QStringLiteral("httpTimeLabel"));
    response_size_label_ = new QLabel(container);
    response_size_label_->setObjectName(QStringLiteral("httpSizeLabel"));

    infoBar->addWidget(status_label_);
    infoBar->addWidget(time_label_);
    infoBar->addWidget(response_size_label_);
    infoBar->addStretch();

    response_body_ = new QPlainTextEdit(container);
    response_body_->setReadOnly(true);
    response_body_->setObjectName(QStringLiteral("httpResponseBody"));

    layout->addLayout(infoBar);
    layout->addWidget(response_body_, 1);

    mainSplitter->addWidget(container);
}

void HttpClientPanel::onSendClicked()
{
    send_btn_->setEnabled(false);
    send_btn_->setText(QStringLiteral("..."));

    NezhaIDE::Model::HTTP::HttpRequest req;
    req.id = 1;
    req.url = url_input_->text().toStdString();

    const auto methodText = method_combo_->currentText();
    if (methodText == QStringLiteral("GET")) req.method = NezhaIDE::Model::HTTP::HttpMethod::Get;
    else if (methodText == QStringLiteral("POST")) req.method = NezhaIDE::Model::HTTP::HttpMethod::Post;
    else if (methodText == QStringLiteral("PUT")) req.method = NezhaIDE::Model::HTTP::HttpMethod::Put;
    else if (methodText == QStringLiteral("PATCH")) req.method = NezhaIDE::Model::HTTP::HttpMethod::Patch;
    else if (methodText == QStringLiteral("DELETE")) req.method = NezhaIDE::Model::HTTP::HttpMethod::Delete;
    else if (methodText == QStringLiteral("HEAD")) req.method = NezhaIDE::Model::HTTP::HttpMethod::Head;
    else if (methodText == QStringLiteral("OPTIONS")) req.method = NezhaIDE::Model::HTTP::HttpMethod::Options;

    for (int i = 0; i < headers_table_->rowCount(); ++i) {
        auto *nameItem = headers_table_->item(i, 0);
        auto *valItem = headers_table_->item(i, 1);
        if (nameItem && valItem && !nameItem->text().isEmpty()) {
            NezhaIDE::Model::HTTP::HttpHeader h;
            h.name = nameItem->text().toStdString();
            h.value = valItem->text().toStdString();
            req.headers.push_back(std::move(h));
        }
    }

    const auto bodyText = body_editor_->toPlainText();
    if (!bodyText.isEmpty()) {
        req.body.type = NezhaIDE::Model::HTTP::BodyType::Json;
        req.body.content = bodyText.toStdString();
    }

    NezhaIDE::Services::HTTP::HttpClientService::instance().send(req);
}

void HttpClientPanel::onNewRequest()
{
    request_list_->addItem(
        QStringLiteral("%1 %2")
            .arg(method_combo_->currentText())
            .arg(url_input_->text().isEmpty()
                ? QStringLiteral("New Request")
                : url_input_->text()));
}

void HttpClientPanel::onDeleteRequest()
{
    auto *item = request_list_->currentItem();
    if (item) {
        delete request_list_->takeItem(request_list_->row(item));
    }
}

void HttpClientPanel::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();

    setStyleSheet(
        QStringLiteral("QWidget { background: %1; }").arg(ts.color(QStringLiteral("bg.secondary"))));

    method_combo_->setStyleSheet(
        QStringLiteral("QComboBox { border: 1px solid %1; border-radius: 4px; padding: 4px 8px;"
        "background: %2; color: %3; font-size: 12px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background: %2; color: %3; selection-background-color: %4; }")
        .arg(ts.color(QStringLiteral("border")),
             ts.color(QStringLiteral("bg.tertiary")),
             ts.color(QStringLiteral("text.primary")),
             ts.color(QStringLiteral("overlay.selection"))));

    url_input_->setStyleSheet(
        QStringLiteral("QLineEdit { border: 1px solid %1; border-radius: 4px; padding: 4px 8px;"
        "background: %2; color: %3; font-size: 12px; }"
        "QLineEdit:focus { border-color: %4; }")
        .arg(ts.color(QStringLiteral("border")),
             ts.color(QStringLiteral("bg.tertiary")),
             ts.color(QStringLiteral("text.primary")),
             ts.color(QStringLiteral("accent"))));

    send_btn_->setStyleSheet(
        QStringLiteral("QPushButton { background: %1; color: %2; border: none; border-radius: 4px;"
        "padding: 4px 12px; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background: %3; }"
        "QPushButton:disabled { background: %4; color: %5; }")
        .arg(ts.color(QStringLiteral("accent")),
             ts.color(QStringLiteral("button.text")),
             ts.color(QStringLiteral("accent.hover")),
             ts.color(QStringLiteral("bg.tertiary")),
             ts.color(QStringLiteral("text.tertiary"))));

    headers_table_->setStyleSheet(
        QStringLiteral("QTableWidget { border: 1px solid %1; gridline-color: %1;"
        "background: %2; color: %3; font-size: 12px; }"
        "QHeaderView::section { background: %4; color: %3; border: none;"
        "padding: 4px 8px; font-size: 11px; font-weight: bold; }")
        .arg(ts.color(QStringLiteral("border")),
             ts.color(QStringLiteral("bg.tertiary")),
             ts.color(QStringLiteral("text.primary")),
             ts.color(QStringLiteral("bg.secondary"))));

    body_editor_->setStyleSheet(
        QStringLiteral("QPlainTextEdit { border: none; background: %1; color: %2;"
        "font-family: 'SF Mono', Menlo, monospace; font-size: 12px; }")
        .arg(ts.color(QStringLiteral("bg.tertiary")),
             ts.color(QStringLiteral("text.primary"))));

    request_tabs_->setStyleSheet(
        QStringLiteral("QTabWidget::pane { border: 1px solid %1; background: %2; }"
        "QTabBar::tab { background: transparent; color: %3; padding: 4px 12px;"
        "border: none; border-bottom: 2px solid transparent; font-size: 11px; }"
        "QTabBar::tab:selected { color: %4; border-bottom: 2px solid %4; }"
        "QTabBar::tab:hover { color: %5; }")
        .arg(ts.color(QStringLiteral("border")),
             ts.color(QStringLiteral("bg.tertiary")),
             ts.color(QStringLiteral("text.tertiary")),
             ts.color(QStringLiteral("accent")),
             ts.color(QStringLiteral("text.primary"))));

    response_body_->setStyleSheet(
        QStringLiteral("QPlainTextEdit { border: none; background: %1; color: %2;"
        "font-family: 'SF Mono', Menlo, monospace; font-size: 12px; }")
        .arg(ts.color(QStringLiteral("bg.tertiary")),
             ts.color(QStringLiteral("text.primary"))));

    status_label_->setStyleSheet(
        QStringLiteral("QLabel { font-size: 13px; font-weight: bold; color: %1; padding: 2px 0; }")
        .arg(ts.color(QStringLiteral("accent"))));
    time_label_->setStyleSheet(
        QStringLiteral("QLabel { font-size: 11px; color: %1; }")
        .arg(ts.color(QStringLiteral("text.tertiary"))));
    response_size_label_->setStyleSheet(
        QStringLiteral("QLabel { font-size: 11px; color: %1; }")
        .arg(ts.color(QStringLiteral("text.tertiary"))));

    request_list_->setStyleSheet(
        QStringLiteral("QListWidget { border: none; background: transparent; font-size: 11px; }"
        "QListWidget::item { padding: 3px 6px; color: %1; }"
        "QListWidget::item:hover { background: %2; }"
        "QListWidget::item:selected { background: %3; color: %4; }")
        .arg(ts.color(QStringLiteral("text.secondary")),
             ts.color(QStringLiteral("overlay.hover")),
             ts.color(QStringLiteral("overlay.selection")),
             ts.color(QStringLiteral("text.primary"))));

    new_req_btn_->setStyleSheet(
        QStringLiteral("QPushButton { border: none; color: %1; font-size: 14px; }"
        "QPushButton:hover { color: %2; }")
        .arg(ts.color(QStringLiteral("text.tertiary")),
             ts.color(QStringLiteral("text.primary"))));
    del_req_btn_->setStyleSheet(new_req_btn_->styleSheet());
}

} // namespace NezhaIDE::Views
