#include "http_client_panel.h"
#include "src/services/http.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include <QAction>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
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
        setStatusColor(resp.statusCode);
        status_label_->setText(QStringLiteral("%1 %2")
            .arg(resp.statusCode)
            .arg(QString::fromStdString(resp.statusText)));
        time_label_->setText(QStringLiteral("%1ms").arg(resp.elapsedMs));

        const auto bodySize = static_cast<qint64>(resp.body.size());
        response_size_label_->setText(bodySize < 1024
            ? QStringLiteral("%1 B").arg(bodySize)
            : QStringLiteral("%1 KB").arg(bodySize / 1024.0, 0, 'f', 1));

        response_body_->setPlainText(QString::fromStdString(resp.body));

        response_headers_table_->setRowCount(0);
        for (const auto &h : resp.headers) {
            const auto row = response_headers_table_->rowCount();
            response_headers_table_->insertRow(row);
            response_headers_table_->setItem(row, 0,
                new QTableWidgetItem(QString::fromStdString(h.name)));
            response_headers_table_->setItem(row, 1,
                new QTableWidgetItem(QString::fromStdString(h.value)));
        }

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

    method_btn_ = new QPushButton(method_, this);
    method_btn_->setCursor(Qt::PointingHandCursor);
    connect(method_btn_, &QPushButton::clicked, this, &HttpClientPanel::onMethodClicked);
    setMethodStyle(method_);

    url_input_ = new QLineEdit(this);
    url_input_->setPlaceholderText(LOC("http.url_placeholder"));

    send_btn_ = new QPushButton(LOC("http.send"), this);
    send_btn_->setFixedWidth(72);
    connect(send_btn_, &QPushButton::clicked, this, &HttpClientPanel::onSendClicked);

    urlBar->addWidget(method_btn_);
    urlBar->addWidget(url_input_, 1);
    urlBar->addWidget(send_btn_);

    request_tabs_ = new QTabWidget(this);
    request_tabs_->setObjectName(QStringLiteral("httpRequestTabs"));

    params_table_ = new QTableWidget(0, 2);
    params_table_->setHorizontalHeaderLabels({LOC("http.param_name"), LOC("http.param_value")});
    params_table_->horizontalHeader()->setStretchLastSection(true);
    params_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    params_table_->setColumnWidth(0, 180);

    headers_table_ = new QTableWidget(0, 2);
    headers_table_->setHorizontalHeaderLabels({LOC("http.header_name"), LOC("http.header_value")});
    headers_table_->horizontalHeader()->setStretchLastSection(true);
    headers_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    headers_table_->setColumnWidth(0, 180);

    body_editor_ = new QPlainTextEdit();
    body_editor_->setPlaceholderText(QStringLiteral("{\n  \"key\": \"value\"\n}"));

    request_tabs_->addTab(params_table_, LOC("http.params"));
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

    response_tabs_ = new QTabWidget(this);
    response_tabs_->setObjectName(QStringLiteral("httpResponseTabs"));

    response_body_ = new QPlainTextEdit();
    response_body_->setReadOnly(true);
    response_body_->setObjectName(QStringLiteral("httpResponseBody"));

    response_headers_table_ = new QTableWidget(0, 2);
    response_headers_table_->setHorizontalHeaderLabels({LOC("http.header_name"), LOC("http.header_value")});
    response_headers_table_->horizontalHeader()->setStretchLastSection(true);
    response_headers_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    response_headers_table_->setColumnWidth(0, 180);
    response_headers_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    response_tabs_->addTab(response_body_, LOC("http.body"));
    response_tabs_->addTab(response_headers_table_, LOC("http.response_headers"));

    responseLayout->addLayout(infoBar);
    responseLayout->addWidget(response_tabs_, 1);
    mainSplitter->addWidget(responseContainer);

    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setSizes({300, 300});

    mainLayout->addWidget(mainSplitter);
}

void HttpClientPanel::onMethodClicked()
{
    QMenu menu(this);
    menu.setStyleSheet(NezhaIDE::Services::ThemeService::instance().qss(QStringLiteral("style.menu")));

    const QStringList methods = {"GET", "POST", "PUT", "PATCH", "DELETE", "HEAD", "OPTIONS"};
    for (const auto &m : methods) {
        auto *action = menu.addAction(m);
        if (m == method_) action->setCheckable(true);
    }
    connect(&menu, &QMenu::triggered, this, [this](QAction *action) {
        method_ = action->text();
        method_btn_->setText(method_);
        setMethodStyle(method_);
    });

    menu.exec(method_btn_->mapToGlobal(QPoint(0, method_btn_->height())));
}

void HttpClientPanel::setMethodStyle(const QString &method)
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    auto color = [&](const QString &key) { return ts.color(key); };

    QString bg;
    if (method == QStringLiteral("GET")) bg = color(QStringLiteral("git.added"));
    else if (method == QStringLiteral("POST")) bg = color(QStringLiteral("accent"));
    else if (method == QStringLiteral("PUT") || method == QStringLiteral("PATCH")) bg = color(QStringLiteral("git.modified"));
    else if (method == QStringLiteral("DELETE")) bg = color(QStringLiteral("git.deleted"));
    else bg = color(QStringLiteral("text.tertiary"));

    method_btn_->setStyleSheet(
        QStringLiteral("QPushButton { background: %1; border: none; border-radius: 4px;"
        "padding: 5px 12px; font-size: 12px; font-weight: bold; color: #FFFFFF; }")
        .arg(bg));
}

void HttpClientPanel::setStatusColor(int statusCode)
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    QString color;
    if (statusCode >= 200 && statusCode < 300) color = ts.color(QStringLiteral("git.added"));
    else if (statusCode >= 300 && statusCode < 400) color = ts.color(QStringLiteral("accent"));
    else if (statusCode >= 400 && statusCode < 500) color = ts.color(QStringLiteral("git.modified"));
    else color = ts.color(QStringLiteral("git.deleted"));

    status_label_->setStyleSheet(
        QStringLiteral("QLabel { font-size: 13px; font-weight: bold; color: %1; padding: 2px 0; }")
        .arg(color));
}

void HttpClientPanel::onSendClicked()
{
    send_btn_->setEnabled(false);
    send_btn_->setText(QStringLiteral("..."));

    NezhaIDE::Model::HTTP::HttpRequest req;
    req.id = 1;
    req.url = url_input_->text().toStdString();

    if (method_ == QStringLiteral("GET")) req.method = NezhaIDE::Model::HTTP::HttpMethod::Get;
    else if (method_ == QStringLiteral("POST")) req.method = NezhaIDE::Model::HTTP::HttpMethod::Post;
    else if (method_ == QStringLiteral("PUT")) req.method = NezhaIDE::Model::HTTP::HttpMethod::Put;
    else if (method_ == QStringLiteral("PATCH")) req.method = NezhaIDE::Model::HTTP::HttpMethod::Patch;
    else if (method_ == QStringLiteral("DELETE")) req.method = NezhaIDE::Model::HTTP::HttpMethod::Delete;
    else if (method_ == QStringLiteral("HEAD")) req.method = NezhaIDE::Model::HTTP::HttpMethod::Head;
    else if (method_ == QStringLiteral("OPTIONS")) req.method = NezhaIDE::Model::HTTP::HttpMethod::Options;

    for (int i = 0; i < params_table_->rowCount(); ++i) {
        auto *nameItem = params_table_->item(i, 0);
        auto *valItem = params_table_->item(i, 1);
        if (nameItem && valItem && !nameItem->text().isEmpty()) {
            NezhaIDE::Model::HTTP::HttpParameter p;
            p.name = nameItem->text().toStdString();
            p.value = valItem->text().toStdString();
            req.queryParameters.push_back(std::move(p));
        }
    }

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
    url_input_->setStyleSheet(ts.qss(QStringLiteral("style.http_url_input")));
    send_btn_->setStyleSheet(ts.qss(QStringLiteral("style.primary_button")));
    params_table_->setStyleSheet(ts.qss(QStringLiteral("style.http_input")));
    headers_table_->setStyleSheet(ts.qss(QStringLiteral("style.http_input")));
    body_editor_->setStyleSheet(ts.qss(QStringLiteral("style.http_input")));
    request_tabs_->setStyleSheet(ts.qss(QStringLiteral("style.http_tab")));
    response_tabs_->setStyleSheet(ts.qss(QStringLiteral("style.http_tab")));
    response_body_->setStyleSheet(ts.qss(QStringLiteral("style.http_response")));
    response_headers_table_->setStyleSheet(ts.qss(QStringLiteral("style.http_input")));
    time_label_->setStyleSheet(ts.qss(QStringLiteral("style.status_label")));
    response_size_label_->setStyleSheet(ts.qss(QStringLiteral("style.status_label")));
    setMethodStyle(method_);
}

} // namespace NezhaIDE::Views
