#include "http_client_panel.h"
#include "src/services/http.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

namespace NezhaIDE::Views {

HttpClientPanel::HttpClientPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    applyStyles();

    connect(&NezhaIDE::Services::ThemeService::instance(), &NezhaIDE::Services::ThemeService::themeChanged,
            this, [this] { applyStyles(); });

    connect(&NezhaIDE::Services::HTTP::HttpClientService::instance(),
            &NezhaIDE::Services::HTTP::HttpClientService::responseReceived,
            this, [this](const NezhaIDE::Model::HTTP::HttpResponse &resp) {
        status_label_->setText(QStringLiteral("%1 %2")
            .arg(resp.statusCode)
            .arg(QString::fromStdString(resp.statusText)));
        time_label_->setText(QStringLiteral("%1ms").arg(resp.elapsedMs));

        const auto bodySize = static_cast<qint64>(resp.body.size());
        response_size_label_->setText(bodySize < 1024
            ? QStringLiteral("%1 B").arg(bodySize)
            : QStringLiteral("%1 KB").arg(bodySize / 1024.0, 0, 'f', 1));

        response_body_->setPlainText(QString::fromStdString(resp.body));
        send_btn_->setEnabled(true);
        send_btn_->setText(LOC("http.send"));
    });

    connect(&NezhaIDE::Services::HTTP::HttpClientService::instance(),
            &NezhaIDE::Services::HTTP::HttpClientService::requestError,
            this, [this](NezhaIDE::Model::HTTP::RequestId, const QString &error) {
        response_body_->setPlainText(error);
        status_label_->setText(QStringLiteral("Error"));
        send_btn_->setEnabled(true);
        send_btn_->setText(LOC("http.send"));
    });
}

HttpClientPanel::~HttpClientPanel() = default;

void HttpClientPanel::setupUI()
{
    setObjectName(QStringLiteral("httpClientRoot"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto *mainSplitter = new QSplitter(Qt::Vertical, this);
    mainSplitter->setHandleWidth(1);

    auto *editorContainer = new QWidget(this);
    auto *editorLayout = new QVBoxLayout(editorContainer);
    editorLayout->setContentsMargins(8, 8, 8, 4);
    editorLayout->setSpacing(6);

    auto *urlBar = new QHBoxLayout();
    urlBar->setSpacing(4);

    method_combo_ = new QComboBox(this);
    method_combo_->addItems({"GET", "POST", "PUT", "PATCH", "DELETE", "HEAD", "OPTIONS"});
    method_combo_->setFixedWidth(90);

    url_input_ = new QLineEdit(this);
    url_input_->setPlaceholderText(LOC("http.url_placeholder"));

    send_btn_ = new QPushButton(LOC("http.send"), this);
    send_btn_->setFixedWidth(72);
    connect(send_btn_, &QPushButton::clicked, this, &HttpClientPanel::onSendClicked);

    urlBar->addWidget(method_combo_);
    urlBar->addWidget(url_input_, 1);
    urlBar->addWidget(send_btn_);

    request_tabs_ = new QTabWidget(this);
    request_tabs_->setObjectName(QStringLiteral("httpRequestTabs"));

    headers_table_ = new QTableWidget(0, 2);
    headers_table_->setHorizontalHeaderLabels({LOC("http.header_name"), LOC("http.header_value")});
    headers_table_->horizontalHeader()->setStretchLastSection(true);
    headers_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    headers_table_->setColumnWidth(0, 180);

    body_editor_ = new QPlainTextEdit();
    body_editor_->setPlaceholderText(QStringLiteral("{\n  \"key\": \"value\"\n}"));

    request_tabs_->addTab(headers_table_, LOC("http.headers"));
    request_tabs_->addTab(body_editor_, LOC("http.body"));

    editorLayout->addLayout(urlBar);
    editorLayout->addWidget(request_tabs_, 1);
    mainSplitter->addWidget(editorContainer);

    auto *responseContainer = new QWidget(this);
    auto *responseLayout = new QVBoxLayout(responseContainer);
    responseLayout->setContentsMargins(8, 4, 8, 8);
    responseLayout->setSpacing(4);

    auto *infoBar = new QHBoxLayout();
    infoBar->setSpacing(12);

    auto *responseLabel = new QLabel(LOC("http.response"), responseContainer);
    responseLabel->setStyleSheet(
        QStringLiteral("QLabel { font-size: 11px; font-weight: bold;"
        "text-transform: uppercase; letter-spacing: 0.5px; }"));

    status_label_ = new QLabel(responseContainer);
    status_label_->setObjectName(QStringLiteral("httpStatusLabel"));
    time_label_ = new QLabel(responseContainer);
    time_label_->setObjectName(QStringLiteral("httpTimeLabel"));
    response_size_label_ = new QLabel(responseContainer);
    response_size_label_->setObjectName(QStringLiteral("httpSizeLabel"));

    infoBar->addWidget(responseLabel);
    infoBar->addWidget(status_label_);
    infoBar->addWidget(time_label_);
    infoBar->addWidget(response_size_label_);
    infoBar->addStretch();

    response_body_ = new QPlainTextEdit(responseContainer);
    response_body_->setReadOnly(true);
    response_body_->setObjectName(QStringLiteral("httpResponseBody"));

    responseLayout->addLayout(infoBar);
    responseLayout->addWidget(response_body_, 1);
    mainSplitter->addWidget(responseContainer);

    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setSizes({300, 300});

    mainLayout->addWidget(mainSplitter);
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

    NezhaIDE::Services::HTTP::HttpClientService::send(req);
}

void HttpClientPanel::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.http_panel")));
    method_combo_->setStyleSheet(ts.qss(QStringLiteral("style.http_input")));
    url_input_->setStyleSheet(ts.qss(QStringLiteral("style.http_url_input")));
    send_btn_->setStyleSheet(ts.qss(QStringLiteral("style.primary_button")));
    headers_table_->setStyleSheet(ts.qss(QStringLiteral("style.http_input")));
    body_editor_->setStyleSheet(ts.qss(QStringLiteral("style.http_input")));
    request_tabs_->setStyleSheet(ts.qss(QStringLiteral("style.http_tab")));
    response_body_->setStyleSheet(ts.qss(QStringLiteral("style.http_response")));
    status_label_->setStyleSheet(ts.qss(QStringLiteral("style.http_status_label")));
    time_label_->setStyleSheet(ts.qss(QStringLiteral("style.status_label")));
    response_size_label_->setStyleSheet(ts.qss(QStringLiteral("style.status_label")));
}

} // namespace NezhaIDE::Views
