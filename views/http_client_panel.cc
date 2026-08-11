#include "http_client_panel.h"
#include "src/services/http.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include <QAction>
#include <QByteArray>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QMenu>
#include <QStringDecoder>
#include <QUrl>
#include <QVBoxLayout>
#include <atomic>
#include <tuple>

namespace NezhaIDE::Views {

namespace {

NezhaIDE::Model::HTTP::RequestId nextRequestId()
{
    static std::atomic<NezhaIDE::Model::HTTP::RequestId> next{1};
    return next.fetch_add(1);
}

QString formatSize(qint64 bytes)
{
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }
    if (bytes < 1024 * 1024) {
        return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    }
    return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 2);
}

QString decodeBody(const QByteArray &raw, const QString &contentType)
{
    if (raw.isEmpty()) return {};

    QString charset;
    const auto idx = contentType.indexOf(QStringLiteral("charset="), 0, Qt::CaseInsensitive);
    if (idx >= 0) {
        auto candidate = contentType.mid(idx + 8).trimmed();
        if (const auto quote = candidate.indexOf(QChar('"')); quote >= 0) {
            candidate = candidate.left(quote);
        }
        if (const auto semi = candidate.indexOf(QChar(';')); semi >= 0) {
            candidate = candidate.left(semi);
        }
        charset = candidate.trimmed();
    }

    if (!charset.isEmpty()) {
        auto decoder = QStringDecoder(charset.toUtf8());
        if (decoder.isValid()) {
            return decoder.decode(raw);
        }
    }

    const auto utf8 = QString::fromUtf8(raw);
    if (!utf8.contains(QChar::ReplacementCharacter)) {
        return utf8;
    }
    return QString::fromLatin1(raw);
}

QString prettyPrintBody(const QString &raw)
{
    const auto trimmed = raw.trimmed();
    if (trimmed.isEmpty() || (trimmed.front() != QChar('{') && trimmed.front() != QChar('['))) {
        return raw;
    }

    QJsonParseError err;
    const auto doc = QJsonDocument::fromJson(trimmed.toUtf8(), &err);
    return err.error == QJsonParseError::NoError
        ? QString::fromUtf8(doc.toJson(QJsonDocument::Indented))
        : raw;
}

}

HttpClientPanel::HttpClientPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("httpClientRoot"));
    setupUI();
    applyStyles();

    connect(&NezhaIDE::Services::ThemeService::instance(), &NezhaIDE::Services::ThemeService::themeChanged,
            this, [this] { applyStyles(); });

    connect(&NezhaIDE::Services::HTTP::HttpClientService::instance(),
            &NezhaIDE::Services::HTTP::HttpClientService::responseReceived,
            this, [this](const NezhaIDE::Model::HTTP::HttpResponse &resp) {
        sending_ = false;
        send_btn_->setText(LOC("http.send"));
        send_btn_->setObjectName(QStringLiteral("httpSendButton"));
        applySendButtonStyle();

        auto statusText = QString::fromStdString(resp.statusText);
        if (statusText.isEmpty()) {
            statusText = LOC("http.unknown_status");
        }
        setStatusPill(resp.statusCode, QStringLiteral("%1 %2").arg(resp.statusCode).arg(statusText));
        time_label_->setText(LOC("http.elapsed_ms").arg(resp.elapsedMs));
        size_label_->setText(formatSize(static_cast<qint64>(resp.body.size())));

        const auto contentType = QString::fromStdString(resp.contentType);
        const auto bodyText = prettyPrintBody(
            decodeBody(QByteArray::fromStdString(resp.body), contentType));
        response_body_->setPlainText(bodyText);

        response_headers_table_->setRowCount(0);
        for (const auto &h : resp.headers) {
            const auto row = response_headers_table_->rowCount();
            response_headers_table_->insertRow(row);
            auto *name = new QTableWidgetItem(QString::fromStdString(h.name));
            name->setFlags(name->flags() & ~Qt::ItemIsEditable);
            auto *value = new QTableWidgetItem(QString::fromStdString(h.value));
            value->setFlags(value->flags() & ~Qt::ItemIsEditable);
            response_headers_table_->setItem(row, 0, name);
            response_headers_table_->setItem(row, 1, value);
        }

        setResponseState(ResponseState::Done);
    });

    connect(&NezhaIDE::Services::HTTP::HttpClientService::instance(),
            &NezhaIDE::Services::HTTP::HttpClientService::requestError,
            this, [this](NezhaIDE::Model::HTTP::RequestId id, int statusCode, const QString &error) {
        if (id != current_request_id_) return;

        sending_ = false;
        send_btn_->setText(LOC("http.send"));
        send_btn_->setObjectName(QStringLiteral("httpSendButton"));
        applySendButtonStyle();

        setStatusPill(statusCode > 0 ? statusCode : -1,
            statusCode > 0 ? QStringLiteral("%1").arg(statusCode) : LOC("http.err"));
        time_label_->clear();
        size_label_->clear();
        response_body_->clear();
        response_headers_table_->setRowCount(0);

        showErrorState(LOC("http.error_title"), error);
        setResponseState(ResponseState::Error);
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
    mainSplitter->setChildrenCollapsible(false);

    auto *editorContainer = new QWidget(mainSplitter);
    auto *editorLayout = new QVBoxLayout(editorContainer);
    editorLayout->setContentsMargins(12, 12, 12, 8);
    editorLayout->setSpacing(8);

    buildRequestBar();

    auto *urlBar = new QHBoxLayout();
    urlBar->setSpacing(6);
    urlBar->addWidget(method_btn_);
    urlBar->addWidget(url_input_, 1);
    urlBar->addWidget(send_btn_);
    editorLayout->addLayout(urlBar);

    buildRequestTabs();
    editorLayout->addWidget(request_tabs_, 1);

    mainSplitter->addWidget(editorContainer);

    buildResponseArea();
    auto *responseContainer = new QWidget(mainSplitter);
    auto *responseLayout = new QVBoxLayout(responseContainer);
    responseLayout->setContentsMargins(12, 8, 12, 12);
    responseLayout->setSpacing(0);
    responseLayout->addWidget(response_stack_);

    mainSplitter->addWidget(responseContainer);

    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 3);
    mainSplitter->setSizes({340, 460});

    mainLayout->addWidget(mainSplitter);

    setResponseState(ResponseState::Empty);
}

void HttpClientPanel::buildRequestBar()
{
    method_btn_ = new QPushButton(this);
    method_btn_->setObjectName(QStringLiteral("httpMethodButton"));
    method_btn_->setCursor(Qt::PointingHandCursor);
    method_btn_->setMinimumWidth(88);
    connect(method_btn_, &QPushButton::clicked, this, &HttpClientPanel::onMethodClicked);
    setMethodStyle(method_);

    url_input_ = new QLineEdit(this);
    url_input_->setObjectName(QStringLiteral("httpUrlInput"));
    url_input_->setPlaceholderText(LOC("http.url_placeholder"));
    url_input_->setMinimumHeight(30);
    connect(url_input_, &QLineEdit::returnPressed, this, &HttpClientPanel::onSendClicked);

    send_btn_ = new QPushButton(LOC("http.send"), this);
    send_btn_->setObjectName(QStringLiteral("httpSendButton"));
    send_btn_->setCursor(Qt::PointingHandCursor);
    send_btn_->setMinimumWidth(88);
    send_btn_->setMinimumHeight(30);
    connect(send_btn_, &QPushButton::clicked, this, &HttpClientPanel::onSendClicked);
}

void HttpClientPanel::buildRequestTabs()
{
    request_tabs_ = new QTabWidget(this);
    request_tabs_->setObjectName(QStringLiteral("httpRequestTabs"));

    auto *paramsTab = buildKeyValueTab();
    params_table_ = paramsTab->findChild<QTableWidget *>(QStringLiteral("kvTable"));
    request_tabs_->addTab(paramsTab, LOC("http.params"));

    auto *headersTab = buildKeyValueTab();
    headers_table_ = headersTab->findChild<QTableWidget *>(QStringLiteral("kvTable"));
    request_tabs_->addTab(headersTab, LOC("http.headers"));

    auto *bodyContainer = new QWidget(this);
    auto *bodyLayout = new QVBoxLayout(bodyContainer);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(6);

    auto *bodyToolbar = new QHBoxLayout();
    bodyToolbar->setSpacing(6);

    auto *bodyLabel = new QLabel(LOC("http.body_type"), bodyContainer);
    bodyLabel->setObjectName(QStringLiteral("httpSectionLabel"));

    body_type_combo_ = new QComboBox(bodyContainer);
    body_type_combo_->setObjectName(QStringLiteral("httpBodyType"));
    body_type_combo_->addItem(LOC("http.body_none"));
    body_type_combo_->addItem(LOC("http.body_json"));
    body_type_combo_->addItem(LOC("http.body_raw"));
    body_type_combo_->addItem(LOC("http.body_xml"));
    body_type_combo_->addItem(LOC("http.body_form"));
    body_type_combo_->setMinimumHeight(28);
    body_type_combo_->setCursor(Qt::PointingHandCursor);
    body_type_combo_->setCurrentIndex(1);

    bodyToolbar->addWidget(bodyLabel);
    bodyToolbar->addWidget(body_type_combo_);
    bodyToolbar->addStretch();
    bodyLayout->addLayout(bodyToolbar);

    body_editor_ = new QPlainTextEdit(bodyContainer);
    body_editor_->setObjectName(QStringLiteral("httpBodyEditor"));
    body_editor_->setPlaceholderText(QStringLiteral("{\n  \"key\": \"value\"\n}"));
    body_editor_->setTabStopDistance(28.0);
    bodyLayout->addWidget(body_editor_, 1);

    request_tabs_->addTab(bodyContainer, LOC("http.body"));
}

QWidget *HttpClientPanel::buildKeyValueTab()
{
    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    auto *addBtn = new QPushButton(LOC("http.add_param"), container);
    addBtn->setObjectName(QStringLiteral("httpGhostButton"));
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setMinimumHeight(26);

    auto *hint = new QLabel(LOC("http.kv_hint"), container);
    hint->setObjectName(QStringLiteral("httpSectionLabel"));

    toolbar->addWidget(addBtn);
    toolbar->addWidget(hint);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    auto *table = new QTableWidget(0, 3, container);
    table->setObjectName(QStringLiteral("kvTable"));
    table->setHorizontalHeaderLabels({QString(), LOC("http.key"), LOC("http.value")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table->setColumnWidth(0, 36);
    table->setColumnWidth(1, 220);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(30);
    table->setSelectionBehavior(QAbstractItemView::SelectItems);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setShowGrid(false);
    table->setWordWrap(false);
    layout->addWidget(table, 1);

    connect(addBtn, &QPushButton::clicked, this, [this, table] {
        appendKeyValueRow(table);
    });

    appendKeyValueRow(table);
    appendKeyValueRow(table);

    return container;
}

void HttpClientPanel::appendKeyValueRow(QTableWidget *table)
{
    const auto row = table->rowCount();
    table->insertRow(row);

    auto *enabled = new QTableWidgetItem();
    enabled->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    enabled->setCheckState(Qt::Checked);
    table->setItem(row, 0, enabled);

    table->setItem(row, 1, new QTableWidgetItem());
    table->setItem(row, 2, new QTableWidgetItem());

    auto *removeBtn = new QPushButton(QStringLiteral("✕"), table);
    removeBtn->setObjectName(QStringLiteral("httpRemoveRow"));
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setFixedSize(22, 22);
    removeBtn->setToolTip(LOC("http.remove_row"));
    connect(removeBtn, &QPushButton::clicked, table, [table, row] {
        table->removeRow(row);
    });
    table->setCellWidget(row, 2, removeBtn);
}

void HttpClientPanel::buildResponseArea()
{
    response_stack_ = new QStackedWidget(this);
    response_stack_->setObjectName(QStringLiteral("httpResponseStack"));

    auto *statePage = new QWidget(response_stack_);
    auto *stateLayout = new QVBoxLayout(statePage);
    stateLayout->setAlignment(Qt::AlignCenter);
    stateLayout->setSpacing(6);

    empty_title_ = new QLabel(LOC("http.empty_title"), statePage);
    empty_title_->setObjectName(QStringLiteral("httpStateTitle"));
    empty_title_->setAlignment(Qt::AlignCenter);

    empty_detail_ = new QLabel(LOC("http.empty_detail"), statePage);
    empty_detail_->setObjectName(QStringLiteral("httpStateDetail"));
    empty_detail_->setAlignment(Qt::AlignCenter);
    empty_detail_->setWordWrap(true);

    stateLayout->addWidget(empty_title_);
    stateLayout->addWidget(empty_detail_);
    response_stack_->addWidget(statePage);

    auto *sendingPage = new QWidget(response_stack_);
    auto *sendingLayout = new QVBoxLayout(sendingPage);
    sendingLayout->setAlignment(Qt::AlignCenter);
    sendingLayout->setSpacing(6);
    auto *sendingTitle = new QLabel(LOC("http.sending_title"), sendingPage);
    sendingTitle->setObjectName(QStringLiteral("httpStateTitle"));
    sendingTitle->setAlignment(Qt::AlignCenter);
    auto *sendingDetail = new QLabel(LOC("http.sending_detail"), sendingPage);
    sendingDetail->setObjectName(QStringLiteral("httpStateDetail"));
    sendingDetail->setAlignment(Qt::AlignCenter);
    sendingDetail->setWordWrap(true);
    sendingLayout->addWidget(sendingTitle);
    sendingLayout->addWidget(sendingDetail);
    response_stack_->addWidget(sendingPage);

    auto *errorPage = new QWidget(response_stack_);
    auto *errorLayout = new QVBoxLayout(errorPage);
    errorLayout->setAlignment(Qt::AlignCenter);
    errorLayout->setSpacing(6);
    error_title_ = new QLabel(errorPage);
    error_title_->setObjectName(QStringLiteral("httpStateTitle"));
    error_title_->setAlignment(Qt::AlignCenter);
    error_detail_ = new QLabel(errorPage);
    error_detail_->setObjectName(QStringLiteral("httpStateDetail"));
    error_detail_->setAlignment(Qt::AlignCenter);
    error_detail_->setWordWrap(true);
    error_detail_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    errorLayout->addWidget(error_title_);
    errorLayout->addWidget(error_detail_);
    response_stack_->addWidget(errorPage);

    auto *donePage = new QWidget(response_stack_);
    auto *doneLayout = new QVBoxLayout(donePage);
    doneLayout->setContentsMargins(0, 0, 0, 0);
    doneLayout->setSpacing(8);

    auto *infoBar = new QHBoxLayout();
    infoBar->setSpacing(10);

    auto *responseLabel = new QLabel(LOC("http.response"), donePage);
    responseLabel->setObjectName(QStringLiteral("httpSectionLabel"));

    status_pill_ = new QLabel(donePage);
    status_pill_->setObjectName(QStringLiteral("httpStatusPill"));
    status_pill_->setAlignment(Qt::AlignCenter);

    time_label_ = new QLabel(donePage);
    time_label_->setObjectName(QStringLiteral("httpTimeLabel"));

    size_label_ = new QLabel(donePage);
    size_label_->setObjectName(QStringLiteral("httpSizeLabel"));

    infoBar->addWidget(responseLabel);
    infoBar->addSpacing(4);
    infoBar->addWidget(status_pill_);
    infoBar->addSpacing(4);
    infoBar->addWidget(time_label_);
    infoBar->addWidget(size_label_);
    infoBar->addStretch();
    doneLayout->addLayout(infoBar);

    response_tabs_ = new QTabWidget(donePage);
    response_tabs_->setObjectName(QStringLiteral("httpResponseTabs"));

    response_body_ = new QPlainTextEdit(response_tabs_);
    response_body_->setObjectName(QStringLiteral("httpResponseBody"));
    response_body_->setReadOnly(true);
    response_body_->setTabStopDistance(28.0);

    response_headers_table_ = new QTableWidget(0, 2, response_tabs_);
    response_headers_table_->setObjectName(QStringLiteral("httpHeadersTable"));
    response_headers_table_->setHorizontalHeaderLabels({LOC("http.key"), LOC("http.value")});
    response_headers_table_->horizontalHeader()->setStretchLastSection(true);
    response_headers_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    response_headers_table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    response_headers_table_->setColumnWidth(0, 260);
    response_headers_table_->verticalHeader()->setVisible(false);
    response_headers_table_->verticalHeader()->setDefaultSectionSize(28);
    response_headers_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    response_headers_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    response_headers_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    response_headers_table_->setShowGrid(false);

    response_tabs_->addTab(response_body_, LOC("http.response_body"));
    response_tabs_->addTab(response_headers_table_, LOC("http.response_headers"));

    doneLayout->addWidget(response_tabs_, 1);
    response_stack_->addWidget(donePage);
}

void HttpClientPanel::onMethodClicked()
{
    QMenu menu(this);
    menu.setStyleSheet(NezhaIDE::Services::ThemeService::instance().qss(QStringLiteral("style.menu")));

    const QStringList methods = {QStringLiteral("GET"), QStringLiteral("POST"), QStringLiteral("PUT"),
                                 QStringLiteral("PATCH"), QStringLiteral("DELETE"), QStringLiteral("HEAD"),
                                 QStringLiteral("OPTIONS")};
    for (const auto &m : methods) {
        auto *action = menu.addAction(m);
        action->setCheckable(true);
        action->setChecked(m == method_);
    }
    connect(&menu, &QMenu::triggered, this, [this](QAction *action) {
        method_ = action->text();
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

    method_btn_->setText(method + QStringLiteral(" ▾"));
    method_btn_->setStyleSheet(
        QStringLiteral("QPushButton { background: %1; border: none; border-radius: 5px;"
                       "padding: 5px 12px; font-size: 12px; font-weight: bold; color: #FFFFFF; }"
                       "QPushButton:hover { background: %2; }")
            .arg(bg, bg));
}

void HttpClientPanel::setStatusPill(int statusCode, const QString &text)
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    QString color;
    if (statusCode < 0) {
        color = ts.color(QStringLiteral("git.deleted"));
    } else if (statusCode >= 200 && statusCode < 300) {
        color = ts.color(QStringLiteral("git.added"));
    } else if (statusCode >= 300 && statusCode < 400) {
        color = ts.color(QStringLiteral("accent"));
    } else if (statusCode >= 400 && statusCode < 500) {
        color = ts.color(QStringLiteral("git.modified"));
    } else {
        color = ts.color(QStringLiteral("git.deleted"));
    }

    status_pill_->setStyleSheet(
        QStringLiteral("QLabel#httpStatusPill { background: %1; color: #FFFFFF;"
                       "border-radius: 10px; padding: 3px 12px;"
                       "font-size: 12px; font-weight: bold; }")
            .arg(color));
    status_pill_->setText(text);
}

void HttpClientPanel::setResponseState(ResponseState state)
{
    response_stack_->setCurrentIndex(static_cast<int>(state));
}

void HttpClientPanel::showErrorState(const QString &title, const QString &detail)
{
    error_title_->setText(title);
    error_detail_->setText(detail);
}

void HttpClientPanel::onSendClicked()
{
    if (sending_) {
        onCancelClicked();
        return;
    }

    const auto rawUrl = url_input_->text().trimmed();
    if (rawUrl.isEmpty()) {
        setStatusPill(-1, LOC("http.err"));
        time_label_->clear();
        size_label_->clear();
        showErrorState(LOC("http.url_empty_title"), LOC("http.url_empty_detail"));
        setResponseState(ResponseState::Error);
        return;
    }

    const auto url = normalizeUrl(rawUrl);
    const QUrl parsed(url);
    if (!parsed.isValid()
        || (parsed.scheme() != QStringLiteral("https") && parsed.scheme() != QStringLiteral("http"))) {
        setStatusPill(-1, LOC("http.err"));
        time_label_->clear();
        size_label_->clear();
        showErrorState(LOC("http.invalid_url_title"), LOC("http.invalid_url_detail").arg(rawUrl));
        setResponseState(ResponseState::Error);
        return;
    }
    url_input_->setText(url);

    sending_ = true;
    send_btn_->setText(LOC("http.cancel"));
    send_btn_->setObjectName(QStringLiteral("httpCancelButton"));
    applySendButtonStyle();
    setResponseState(ResponseState::Sending);

    auto req = collectRequest();
    current_request_id_ = req.id;
    NezhaIDE::Services::HTTP::HttpClientService::send(req);
}

void HttpClientPanel::onCancelClicked()
{
    if (current_request_id_ != 0) {
        NezhaIDE::Services::HTTP::HttpClientService::cancel(current_request_id_);
    }
    sending_ = false;
    current_request_id_ = 0;
    send_btn_->setText(LOC("http.send"));
    send_btn_->setObjectName(QString());
    applySendButtonStyle();
    setResponseState(ResponseState::Empty);
}

void HttpClientPanel::applySendButtonStyle()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    if (send_btn_->objectName() == QStringLiteral("httpCancelButton")) {
        send_btn_->setStyleSheet(ts.qss(QStringLiteral("style.http_cancel_button")));
    } else {
        send_btn_->setStyleSheet(ts.qss(QStringLiteral("style.primary_button")));
    }
}

NezhaIDE::Model::HTTP::HttpRequest HttpClientPanel::collectRequest() const
{
    using namespace NezhaIDE::Model::HTTP;

    HttpRequest req;
    req.id = nextRequestId();

    const auto m = method_;
    if (m == QStringLiteral("GET")) req.method = HttpMethod::Get;
    else if (m == QStringLiteral("POST")) req.method = HttpMethod::Post;
    else if (m == QStringLiteral("PUT")) req.method = HttpMethod::Put;
    else if (m == QStringLiteral("PATCH")) req.method = HttpMethod::Patch;
    else if (m == QStringLiteral("DELETE")) req.method = HttpMethod::Delete;
    else if (m == QStringLiteral("HEAD")) req.method = HttpMethod::Head;
    else if (m == QStringLiteral("OPTIONS")) req.method = HttpMethod::Options;

    req.url = url_input_->text().trimmed().toStdString();

    const auto collectRows = [](const QTableWidget *table) {
        std::vector<std::tuple<bool, std::string, std::string>> rows;
        for (int i = 0; i < table->rowCount(); ++i) {
            auto *enabledItem = table->item(i, 0);
            auto *nameItem = table->item(i, 1);
            auto *valueItem = table->item(i, 2);
            if (!nameItem) continue;
            const auto name = nameItem->text().trimmed();
            if (name.isEmpty()) continue;
            rows.emplace_back(
                enabledItem && enabledItem->checkState() == Qt::Checked,
                name.toStdString(),
                valueItem ? valueItem->text().toStdString() : std::string{});
        }
        return rows;
    };

    for (const auto &[enabled, name, value] : collectRows(params_table_)) {
        HttpParameter p;
        p.enabled = enabled;
        p.name = name;
        p.value = value;
        req.queryParameters.push_back(std::move(p));
    }

    bool hasContentType = false;
    for (const auto &[enabled, name, value] : collectRows(headers_table_)) {
        HttpHeader h;
        h.enabled = enabled;
        h.name = name;
        h.value = value;
        if (enabled && QString::fromStdString(name).compare(QStringLiteral("Content-Type"), Qt::CaseInsensitive) == 0) {
            hasContentType = true;
        }
        req.headers.push_back(std::move(h));
    }

    const auto bodyText = body_editor_->toPlainText();
    switch (body_type_combo_->currentIndex()) {
        case 1:
            req.body.type = BodyType::Json;
            req.body.content = bodyText.toStdString();
            if (!hasContentType) {
                HttpHeader h;
                h.name = "Content-Type";
                h.value = "application/json";
                req.headers.push_back(std::move(h));
            }
            break;
        case 2:
            req.body.type = BodyType::Raw;
            req.body.content = bodyText.toStdString();
            break;
        case 3:
            req.body.type = BodyType::Xml;
            req.body.content = bodyText.toStdString();
            if (!hasContentType) {
                HttpHeader h;
                h.name = "Content-Type";
                h.value = "application/xml";
                req.headers.push_back(std::move(h));
            }
            break;
        case 4:
            req.body.type = BodyType::FormUrlEncoded;
            req.body.content = bodyText.toStdString();
            if (!hasContentType) {
                HttpHeader h;
                h.name = "Content-Type";
                h.value = "application/x-www-form-urlencoded";
                req.headers.push_back(std::move(h));
            }
            break;
        default:
            break;
    }

    return req;
}

QString HttpClientPanel::normalizeUrl(QString url) const
{
    url = url.trimmed();
    if (!url.contains(QStringLiteral("://"))) {
        url.prepend(QStringLiteral("https://"));
    }
    return url;
}

void HttpClientPanel::applyStyles()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.http_panel")));
    url_input_->setStyleSheet(ts.qss(QStringLiteral("style.http_url_input")));
    params_table_->setStyleSheet(ts.qss(QStringLiteral("style.http_kv_table")));
    headers_table_->setStyleSheet(ts.qss(QStringLiteral("style.http_kv_table")));
    body_editor_->setStyleSheet(ts.qss(QStringLiteral("style.http_body_editor")));
    body_type_combo_->setStyleSheet(ts.qss(QStringLiteral("style.http_combo")));
    request_tabs_->setStyleSheet(ts.qss(QStringLiteral("style.http_tabs")));
    response_tabs_->setStyleSheet(ts.qss(QStringLiteral("style.http_tabs")));
    response_body_->setStyleSheet(ts.qss(QStringLiteral("style.http_response_body")));
    response_headers_table_->setStyleSheet(ts.qss(QStringLiteral("style.http_kv_table")));
    time_label_->setStyleSheet(ts.qss(QStringLiteral("style.http_meta_label")));
    size_label_->setStyleSheet(ts.qss(QStringLiteral("style.http_meta_label")));
    empty_title_->setStyleSheet(ts.qss(QStringLiteral("style.http_state_title")));
    empty_detail_->setStyleSheet(ts.qss(QStringLiteral("style.http_state_detail")));
    error_title_->setStyleSheet(ts.qss(QStringLiteral("style.http_state_title")));
    error_detail_->setStyleSheet(ts.qss(QStringLiteral("style.http_state_detail")));
    applySendButtonStyle();
    setMethodStyle(method_);
}

} // namespace NezhaIDE::Views
